#include <memory>

class VidDriver {
 public:
  VidDriver() = default;
  ~VidDriver();

  void Init(void* windowContext);

  void Render();

 private:
  struct Impl;

  std::shared_ptr<Impl> impl = nullptr;
};