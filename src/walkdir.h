#ifndef WALKDIR_H
#define WALKDIR_H

#include <napi.h>

#ifdef _WIN32
#define USE_STD_FS_API
#endif

#ifdef USE_STD_FS_API
#include "win_dirent.h"
#else
#include <dirent.h>
#endif

#include <string>
#include <set>
#include <vector>
#include <list>
#include <algorithm>
#include <thread>
#include <chrono>
#include <future>
#include <atomic>

#include "ConcurrentQueue.h"

extern "C"
{
#include "repository.h"

extern int git_libgit2_init();
extern int git_ignore_path_is_ignored(
       int *ignored,
       git_repository *repo,
       const char *path);
}

// Converted from the example at
// https://github.com/nodejs/node-addon-examples/blob/master/6_object_wrap/node-addon-api/

struct FileEntry {
  std::string fname;
  char type;
  ino_t ino;
  FileEntry(const std::string& fname, char type, ino_t ino) : fname(fname), type(type), ino(ino) {}
};

typedef std::vector<FileEntry> VectorFileEntry;

class ThreadState {
public:
  std::thread t;
  // When the thread needs to be stopped, the main thread will push an empty
  // vector to the |input|.
  ConcurrentQueue<std::vector<std::string>> input;
  ConcurrentQueue<VectorFileEntry> output;
  std::atomic_ulong count;

  ThreadState() : count(0) {}

  // move constructor
  ThreadState(ThreadState && a) : t(std::move(a.t)),
      input((std::move(a.input))), output(std::move(a.output)) {
    std::atomic_store(&count, (unsigned long)(a.count));
  }
  ThreadState(const ThreadState&) = delete;            // disable copying
  ThreadState& operator=(const ThreadState&) = delete; // disable assignment

  bool operator < (const ThreadState& a) const {
    return (count < a.count);
  }
};

class WalkDir : public Napi::ObjectWrap<WalkDir> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  WalkDir(const Napi::CallbackInfo& info);
  ~WalkDir();

  Napi::Value GetNextFileEntries(const Napi::CallbackInfo& info);
  Napi::Value AddLoadDirs(const Napi::CallbackInfo& info);
  Napi::Value CheckForFinished(const Napi::CallbackInfo& info);

private:
  void TerminateThreads();

  std::string rootDir;
  bool followSymlinks;
  bool syncMode;
  bool skipGitIgnored;
  std::set<std::string> ignoredNames;
  std::vector<std::string> ignoredStartNames;
  size_t maxThreads;

  std::set<ino_t> inodes;
  std::vector<std::string> dirs_to_load;
  std::list<ThreadState> thread_states_;
};

#endif // WALKDIR_H
