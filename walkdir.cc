#include "walkdir.h"

namespace {

#ifdef _WIN32
const char* PATH_SEPARATOR = "\\";
#else
const char* PATH_SEPARATOR = "/";
#endif

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
  if (info.Length() != 4 || !info[0].IsString() || !info[1].IsBoolean() ||
      !info[2].IsArray() || !info[3].IsArray()) {
    Napi::TypeError::New(info.Env(), "Invalid arguments").ThrowAsJavaScriptException();
  }
  stack.push(info[0].As<Napi::String>());
  followSymlinks = info[1].As<Napi::Boolean>();
  ignoredNames = ToSet(info[2].As<Napi::Array>());
  ignoredStartNames = ToVector(info[3].As<Napi::Array>());
}

Napi::Value WalkDir::GetNextFileEntries(const Napi::CallbackInfo& info) {
  Napi::Array ret = Napi::Array::New(info.Env(), 2);
  Napi::Array fnamesArr = Napi::Array::New(info.Env(), 0);
  Napi::Array typesArr = Napi::Array::New(info.Env(), 0);
  size_t index = 0;

  while(!stack.empty()) {
    const auto name = stack.top();
    stack.pop();
    if (DIR *dir = opendir(name.c_str())) {
      while (struct dirent *entry = readdir(dir)) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
          continue;
        if (inodes.find(entry->d_ino) != inodes.end())
          continue;
        inodes.insert(entry->d_ino);
        if (ignoredNames.find(entry->d_name) != ignoredNames.end())
          continue;
        const char* dname = entry->d_name;
        if (std::any_of(ignoredStartNames.begin(),
                        ignoredStartNames.end(),
                        [dname](const std::string &ignore){return !strncmp(dname, ignore.c_str(), ignore.size());})) {
          continue;
        }
        std::string fname = name + PATH_SEPARATOR + entry->d_name;
        if (entry->d_type == DT_DIR) {
          stack.push(fname);
          fnamesArr[index] = Napi::String::New(info.Env(), fname);
          typesArr[index] = Napi::Number::New(info.Env(), entry->d_type);
          index++;
        } else if (followSymlinks && entry->d_type == DT_LNK) {
          stack.push(fname);
        } else {
          fnamesArr[index] = Napi::String::New(info.Env(), fname);
          typesArr[index] = Napi::Number::New(info.Env(), entry->d_type);
          index++;
        }
      }
      closedir(dir);
    }
  }

  ret[0u] = fnamesArr;
  ret[1u] = typesArr;
  return ret;
}

Napi::Object WalkDir::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "WalkDir", {
    InstanceMethod("GetNextFileEntries", &WalkDir::GetNextFileEntries),
  });

  exports.Set("WalkDir", func);
  return exports;
}

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  return WalkDir::Init(env, exports);
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, InitAll)
