#pragma once

#include <functional>
#include <memory>

class VidDriver;

class Display {
 public:
  Display(const char* name, uint32_t width, uint32_t height);
  ~Display();

  Display& Init(std::weak_ptr<VidDriver> vidDriver);
  Display& Show();
  Display& Loop();
  Display& SetDrawCallback(std::function<void()>&& drawCallback);

  void* GetContext() const;

  void Resize(uint32_t width, uint32_t height);

  const uint32_t Width() const { return width; }
  const uint32_t Height() const { return height; }

 private:
  struct Impl;
  std::unique_ptr<Impl> impl = nullptr;

  std::function<void()> drawCallback = []() {};

  uint32_t width = 0;
  uint32_t height = 0;
};
