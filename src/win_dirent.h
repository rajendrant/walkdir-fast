// Wrapper over std::filesystem to create apis for opendir(), readdir()

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#define DT_REG      0x1
#define DT_BLK      0x2
#define DT_CHR      0x3
#define DT_DIR      0x4
#define DT_FIFO     0x5
#define DT_LABEL    0x6
#define DT_LNK      0x7
#define DT_SOCK     0x8
#define DT_UNKNOWN  0xf

struct dirent {
  char d_type;
  char d_name[1024];
  dirent(char type, const std::string &name) : d_type(type) {
    strncpy(d_name, name.c_str(), sizeof(d_name));
  }
};

typedef struct {
  std::vector<dirent> entries;
  size_t index;
} DIR;
DIR dirval;

DIR *opendir(const std::string &dirname) {
  dirval.index = 0;
  dirval.entries.clear();
  for (const auto &e : fs::directory_iterator(dirname)) {
    char type = fs::is_directory(e.path()) ? DT_DIR : DT_REG;
    dirval.entries.emplace_back(type, e.path().filename().string());
  }
  return &dirval;
}

void closedir(DIR*) {}

struct dirent *readdir(DIR *dirval) {
  if (dirval->index >= dirval->entries.size())
    return nullptr;
  dirval->index++;
  return &dirval->entries[dirval->index-1];
}
