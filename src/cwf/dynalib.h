#ifndef CWF_DYNAMIC_LIBRARY_H__
#define CWF_DYNAMIC_LIBRARY_H__

#include <string>

namespace cwf {

struct DynamicLibrary {
public:
  DynamicLibrary(const std::string& filename) 
    : filename_(filename)
    , handle_(0) {}

  bool Load();
  void Close();
  void* GetProc(const char* name);

private:
  std::string filename_;
  void* handle_;
};

}
#endif // CWF_DYNAMIC_LIBRARY_H__
