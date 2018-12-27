#ifndef WALKDIR_H
#define WALKDIR_H

#include <napi.h>

#ifdef _WIN32
// https://github.com/tronkko/dirent
#include "dirent.h"
#else
#include <dirent.h>
#endif

#include <string>
#include <stack>
#include <set>
#include <vector>
#include <algorithm>

// Converted from the example at
// https://github.com/nodejs/node-addon-examples/blob/master/6_object_wrap/node-addon-api/

struct FileEntry {
  std::string fname;
  char type;
  FileEntry(const std::string &fname, char type) : fname(fname), type(type) {}
};

class WalkDir : public Napi::ObjectWrap<WalkDir> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  WalkDir(const Napi::CallbackInfo& info);

  Napi::Value GetNextFileEntries(const Napi::CallbackInfo& info);

private:
  bool followSymlinks;
  std::set<std::string> ignoredNames;
  std::vector<std::string> ignoredStartNames;

  std::set<ino_t> inodes;
  std::stack<std::string> stack;
};

#endif // WALKDIR_H
