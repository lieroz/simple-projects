#include "display.h"

#include <windows.h>

#include <format>
#include <system_error>

#include "viddriver.h"

struct Display::Impl {
  HINSTANCE hInstance;

  WNDCLASS wc;
  HWND hwnd;

  std::weak_ptr<VidDriver> vidDriver;
};

static LRESULT CALLBACK DisplayProc(HWND hwnd,
                                    UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam) {
  Display* display =
      reinterpret_cast<Display*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (uMsg == WM_NCCREATE) {
    CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);

    display = reinterpret_cast<Display*>(pCreate->lpCreateParams);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(display));
  }

  if (display != nullptr) {
    switch (uMsg) {
      case WM_SIZE:
        display->Resize(LOWORD(lParam), HIWORD(lParam));
        return EXIT_SUCCESS;
      case WM_CLOSE:
        if (!DestroyWindow(hwnd)) {
          throw std::runtime_error(
              std::format("DestroyDisplay failed: %d", GetLastError()));
        }
        break;
      case WM_DESTROY:
        PostQuitMessage(EXIT_SUCCESS);
        break;
    }
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

Display::Display(const char* name, uint32_t width, uint32_t height)
    : width(width), height(height) {
  HINSTANCE hInstance = GetModuleHandle(nullptr);
  WNDCLASS wc = {
      .lpfnWndProc = DisplayProc,
      .hInstance = hInstance,
      .lpszClassName = name,
  };
  if (!RegisterClass(&wc)) {
    throw std::runtime_error(
        std::format("RegisterClass failed: %d", GetLastError()));
  }
  impl = std::make_unique<Impl>(hInstance, wc);
}

Display::~Display() {
  if (!UnregisterClass(impl->wc.lpszClassName, impl->hInstance)) {
    throw std::runtime_error(
        std::format("UnregisterClass failed: %d", GetLastError()));
  }
}

Display& Display::Init(std::weak_ptr<VidDriver> vidDriver) {
  impl->hwnd = CreateWindow(
      impl->wc.lpszClassName, nullptr, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
      CW_USEDEFAULT, width, height, nullptr, nullptr, impl->hInstance, this);
  if (impl->hwnd == nullptr) {
    throw std::runtime_error(
        std::format("CreateDisplay failed: %d", GetLastError()));
  }
  impl->vidDriver = vidDriver;
  return *this;
}

Display& Display::Show() {
  ShowWindow(impl->hwnd, SW_SHOW);
  return *this;
}

Display& Display::Loop() {
  for (MSG msg = {}; msg.message != WM_QUIT;) {
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {
      drawCallback();
    }
  }
  return *this;
}

Display& Display::SetDrawCallback(std::function<void()>&& drawCallback) {
  this->drawCallback = std::move(drawCallback);
  return *this;
}

void* Display::GetContext() const {
  return impl->hwnd;
}

void Display::Resize(uint32_t width, uint32_t height) {
  this->width = width;
  this->height = height;
  impl->vidDriver.lock()->ResizeSwapChain(width, height);
}
