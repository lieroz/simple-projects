#include <functional>
#include <memory>


class Window {
 public:
  Window(const char* name);
  ~Window();

  Window& Init();
  Window& Show();
  Window& Loop();
  Window& SetDrawCallback(std::function<void()>&& drawCallback);

  void* GetContext() const;

 private:
  struct Impl;

  std::shared_ptr<Impl> impl = nullptr;
  std::function<void()> drawCallback = []() {};
};