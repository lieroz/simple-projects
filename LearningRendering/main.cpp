#include "viddriver.h"
#include "window.h"


int main() {
  Window window{"MainWindow"};
  window.Init().Show();

  VidDriver vidDriver;

  vidDriver.Init(window.GetContext());

  window.SetDrawCallback([&vidDriver]() { vidDriver.Render(); }).Loop();
  return 0;
}