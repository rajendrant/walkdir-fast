// Wrapper over std::filesystem to create apis for opendir(), readdir()

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#define DT_REG      0x1
#define DT_DIR      0x4

struct FS_dirent {
  char d_type;
  char d_name[1024];
  dirent(char type, const std::string &name) : d_type(type) {
    strncpy(d_name, name.c_str(), sizeof(d_name));
  }
};

typedef struct {
  std::vector<FS_dirent> entries;
  size_t index;
} FS_DIR;
FS_DIR dirval;

FS_DIR *opendir(const std::string &dirname) {
  dirval.index = 0;
  dirval.entries.clear();
  for (const auto &e : fs::directory_iterator(dirname)) {
    char type = fs::is_directory(e.path()) ? DT_DIR : DT_REG;
    dirval.entries.emplace_back(type, e.path().filename().string());
  }
  return &dirval;
}

void closedir(FS_DIR*) {}

struct FS_dirent *readdir(DIR *dirval) {
  if (dirval->index >= dirval->entries.size())
    return nullptr;
  dirval->index++;
  return &dirval->entries[dirval->index-1];
}
