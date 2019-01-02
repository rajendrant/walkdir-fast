#include "walkdir.h"

// #include <functional>

namespace {

#ifdef _WIN32
const char* PATH_SEPARATOR = "\\";
#else
const char* PATH_SEPARATOR = "/";
#endif

VectorFileEntry walkdir_internal(const std::vector<std::string> &dirs,
                                 const std::set<std::string> &ignoredNames,
                                 const std::vector<std::string> &ignoredStartNames,
                                 git_repository *repo,
                                 const std::string &rootDir) {
  VectorFileEntry res;
  for(const auto &dirname : dirs) {
    if (DIR *dir = opendir(dirname.c_str())) {
      while (struct dirent *entry = readdir(dir)) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
          continue;
        if (ignoredNames.find(entry->d_name) != ignoredNames.end())
          continue;
        if (std::any_of(ignoredStartNames.begin(),
                        ignoredStartNames.end(),
                        [entry](const auto &ignore){return !strncmp(entry->d_name, ignore.c_str(), ignore.size());})) {
          continue;
        }
        std::string fname = dirname + entry->d_name;
        if (repo) {
          int is_git_ignored = 0;
          const char *relative_fname = fname.c_str();
          if(rootDir[0]!=PATH_SEPARATOR[0])
            relative_fname += rootDir.size();
          if (git_ignore_path_is_ignored(&is_git_ignored, repo, relative_fname)==GIT_OK &&
            is_git_ignored==1) {
            continue;
          }
        }
        res.emplace_back(fname, entry->d_type, entry->d_ino);
      }
      closedir(dir);
    }
  }
  return res;
}

void thread_worker_walkdir(const std::string& rootDir,
                           const std::set<std::string> &ignoredNames,
                           const std::vector<std::string> &ignoredStartNames,
                           bool skipGitIgnored,
                           ThreadState &thread_state) {
  git_repository *repo=nullptr;
  if (skipGitIgnored && git_repository_open_ext(&repo, rootDir.c_str(), 0, NULL) != GIT_OK) {
    git_repository_free(repo);
    repo = nullptr;
  }
  while (true) {
    std::vector<std::string> dirs;
    thread_state.input.pop(dirs);
    if(dirs.empty()) break;
    thread_state.output.push(walkdir_internal(dirs, ignoredNames, ignoredStartNames, repo, rootDir));
    thread_state.count -= dirs.size();
  }
  if (repo) git_repository_free(repo);
}

std::set<std::string> ToSet(const Napi::Array &arr) {
  std::set<std::string> ret;
  for(uint32_t i=0; i<arr.Length(); i++) {
    ret.emplace(arr[i].ToString().Utf8Value());
  }
  return ret;
}

std::vector<std::string> ToVector(const Napi::Array &arr) {
  std::vector<std::string> ret;
  ret.reserve(arr.Length());
  for(uint32_t i=0; i<arr.Length(); i++) {
    ret.emplace_back(arr[i].ToString().Utf8Value());
  }
  return ret;
}

} // namespace

WalkDir::WalkDir(const Napi::CallbackInfo& info) : Napi::ObjectWrap<WalkDir>(info) {
  if (info.Length() != 7 || !info[0].IsString() || !info[1].IsBoolean() || !info[2].IsBoolean() ||
      !info[3].IsArray() || !info[4].IsArray() || !info[5].IsBoolean() || !info[6].IsNumber()) {
    Napi::TypeError::New(info.Env(), "Invalid arguments").ThrowAsJavaScriptException();
  }
  rootDir = info[0].As<Napi::String>();
  dirs_to_load.emplace_back(rootDir);
  followSymlinks = info[1].As<Napi::Boolean>();
  syncMode = info[2].As<Napi::Boolean>();
  ignoredNames = ToSet(info[3].As<Napi::Array>());
  ignoredStartNames = ToVector(info[4].As<Napi::Array>());
  skipGitIgnored = info[5].As<Napi::Boolean>();
  maxThreads = info[6].As<Napi::Number>().Uint32Value();
}

WalkDir::~WalkDir() {
  TerminateThreads();
}

Napi::Value WalkDir::GetNextFileEntries(const Napi::CallbackInfo& info) {
  while (thread_states_.size() < maxThreads) {
    thread_states_.emplace_back();
    thread_states_.back().t = std::thread(thread_worker_walkdir, std::ref(rootDir),
                          std::ref(ignoredNames), std::ref(ignoredStartNames),
                          skipGitIgnored,
                          std::ref(thread_states_.back()));
  }
  VectorFileEntry result;
  // result = walkdir_internal(dirs_to_load, ignoredNames, ignoredStartNames, nullptr);
  // dirs_to_load.clear();

  size_t chunk_size = dirs_to_load.size() / thread_states_.size();
  if (chunk_size < 100) chunk_size = 100;
  for(auto &thread : thread_states_) {
    if (chunk_size > dirs_to_load.size())
      chunk_size = dirs_to_load.size();
    if (!chunk_size) continue;
    std::vector<std::string> tmp;
    for(size_t i=0; i<chunk_size; i++)
      tmp.push_back(dirs_to_load[i]);
    auto end_it = std::next(dirs_to_load.begin(), chunk_size);
    // std::copy(dirs_to_load.begin(), end_it, tmp.begin());
    dirs_to_load.erase(dirs_to_load.begin(), end_it);
    thread.input.push(tmp);
    thread.count += chunk_size;
  }

  thread_states_.sort(std::less<ThreadState>());

  Napi::Array ret = Napi::Array::New(info.Env(), 0);
  size_t index = 0;
  for(auto &thread : thread_states_) {
    while (!thread.output.empty()) {
      VectorFileEntry result;
      thread.output.pop(result);
      for (const auto &entry : result) {
#ifndef USE_STD_FS_API
        if (inodes.find(entry.ino) != inodes.end())
          continue;
        inodes.insert(entry.ino);
#endif
        if (entry.type == DT_DIR) {
          ret[index] = Napi::String::New(info.Env(), entry.fname + PATH_SEPARATOR);
          index++;
        } else if (followSymlinks && entry.type == DT_LNK) {
          dirs_to_load.push_back(entry.fname);
        } else if (entry.type == DT_REG) {
          ret[index] = Napi::String::New(info.Env(), entry.fname);
          index++;
        }
      }
    }
  }

  return ret;
}

Napi::Value WalkDir::AddLoadDirs(const Napi::CallbackInfo& info) {
  if (info.Length() != 1 || !info[0].IsArray()) {
    Napi::TypeError::New(info.Env(), "Invalid arguments").ThrowAsJavaScriptException();
    return Napi::Boolean();
  }
  const auto &dirs = info[0].As<Napi::Array>();
  for(uint32_t i=0; i<dirs.Length(); i++) {
    dirs_to_load.emplace_back(dirs[i].ToString().Utf8Value());
  }
  return Napi::Boolean();
}

Napi::Value WalkDir::CheckForFinished(const Napi::CallbackInfo& info) {
  bool empty = dirs_to_load.empty();
  if (empty) {
    empty = std::all_of(thread_states_.begin(), thread_states_.end(),
      [](auto &t) {return t.count==0 && t.output.empty();});
  }
  if (empty) {
    TerminateThreads();
  }
  return Napi::Boolean::New(info.Env(), empty);
}

void WalkDir::TerminateThreads() {
  for(auto& thread: thread_states_) {
    thread.input.push(std::vector<std::string>());
  }
  for(auto& thread: thread_states_) {
    thread.t.join();
  }
  thread_states_.clear();
}

Napi::Object WalkDir::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "WalkDir", {
    InstanceMethod("GetNextFileEntries", &WalkDir::GetNextFileEntries),
    InstanceMethod("AddLoadDirs", &WalkDir::AddLoadDirs),
    InstanceMethod("CheckForFinished", &WalkDir::CheckForFinished),
  });

  exports.Set("WalkDir", func);
  return exports;
}

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  git_libgit2_init();
  return WalkDir::Init(env, exports);
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, InitAll)
