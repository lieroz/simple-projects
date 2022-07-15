#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>

#include <filesystem>
#include <iostream>
#include <memory>

namespace fs = std::filesystem;

class Application {
 public:
 private:
};

class Renderer {
 public:
 private:
};

int main(int argc, char* argv[]) {
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_Window* window = SDL_CreateWindow("LearnVulkan", SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED, 1200, 720,
                                        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);

  for (bool running = true; running;) {
    for (SDL_Event event; SDL_PollEvent(&event);) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          break;
      }
    }
  }

  SDL_DestroyWindow(window);
  window = nullptr;

  SDL_Quit();
  return 0;
}