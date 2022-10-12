#include <windows.h>

#include <format>
#include <system_error>

#include "window.h"

struct Window::Impl {
  HINSTANCE hInstance;
  WNDCLASS wc;
  HWND hwnd;
};

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam) {
  Window* window =
      reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (uMsg == WM_NCCREATE) {
    CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
    window = reinterpret_cast<Window*>(pCreate->lpCreateParams);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
  }

  if (window != nullptr) {
    switch (uMsg) {
      case WM_CLOSE:
        if (!DestroyWindow(hwnd)) {
          throw std::runtime_error(
              std::format("DestroyWindow failed: %d", GetLastError()));
        }
        break;
      case WM_DESTROY:
        PostQuitMessage(EXIT_SUCCESS);
        break;
    }
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

Window::Window(const char* name) {
  HINSTANCE hInstance = GetModuleHandle(nullptr);
  WNDCLASS wc = {
      .lpfnWndProc = WindowProc,
      .hInstance = hInstance,
      .lpszClassName = name,
  };
  if (!RegisterClass(&wc)) {
    throw std::runtime_error(
        std::format("RegisterClass failed: %d", GetLastError()));
  }
  impl = std::make_shared<Impl>(hInstance, wc);
}

Window::~Window() {
  if (!UnregisterClass(impl->wc.lpszClassName, impl->hInstance)) {
    throw std::runtime_error(
        std::format("UnregisterClass failed: %d", GetLastError()));
  }
}

Window& Window::Init() {
  impl->hwnd =
      CreateWindow(impl->wc.lpszClassName, nullptr, WS_OVERLAPPEDWINDOW,
                   CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                   nullptr, nullptr, impl->hInstance, this);
  if (impl->hwnd == nullptr) {
    throw std::runtime_error(
        std::format("CreateWindow failed: %d", GetLastError()));
  }
  return *this;
}

Window& Window::Show() {
  ShowWindow(impl->hwnd, SW_SHOW);
  return *this;
}

Window& Window::Loop() {
  for (MSG msg; GetMessage(&msg, nullptr, 0, 0);) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);

    drawCallback();
  }
  return *this;
}

Window& Window::SetDrawCallback(std::function<void()>&& drawCallback) {
  this->drawCallback = std::move(drawCallback);
  return *this;
}

void* Window::GetContext() const { return impl->hwnd; }