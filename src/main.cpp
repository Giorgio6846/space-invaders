#include <iostream>

#include <chrono>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

struct {
  SDL_Window * window;
  SDL_Renderer * renderer;
  SDL_Texture * texture;
} game;

struct {
  int x;
  int y;
  float speed = 10; 
} ship;

struct {
  unsigned long long last_second;
  unsigned long long last_frame;
  unsigned long long delta_ns;
  long double delta;
  int frames;
  int fps; 
} state;

int main(int argc, char* args[]) {

  if (SDL_Init(SDL_INIT_EVERYTHING)) {
    std::cout << "SDL_Init failed with error: " << SDL_GetError() << std::endl;
    exit(1);
  }

  //Create window
  game.window = SDL_CreateWindow("スパースインベーダー!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1600, 900, SDL_WINDOW_SHOWN);

  if (!game.window) {
    std::cout << "Failed to create window" << std::endl;
    return -1;
  }

  //Create renderer
  game.renderer = SDL_CreateRenderer(game.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

  if (!game.renderer) {
    std::cout << "Failed to create renderer" << std::endl;
    return -1;
  }

  //Create backbuffer
  game.texture = SDL_CreateTexture(game.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 256, 144);
  
  if(!game.texture) {
    std::cout << "Failed to create texture" << std::endl;
    return -1;
  }

  SDL_Event event;
  bool quit = false;
  while (quit == false) {

    const int NS_PER_SEC = 1000000000;

    unsigned long long now = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

    state.delta_ns = now - state.last_frame;
    state.delta = (long double) (state.delta_ns / NS_PER_SEC);
    state.last_frame = now;  
    state.frames += 1;

    if ((now - state.last_second) > NS_PER_SEC) {
      state.last_second = now;
      state.fps = state.frames;
      state.frames = 0;
      std::cout << "FPS: " << state.fps << std::endl;
    }

    while (SDL_PollEvent(&event)) {
      switch (event.type)
      {
      case SDL_QUIT:
          quit = true;
        break;

      case SDL_KEYDOWN:
        switch (event.key.keysym.sym)
        {
        case SDLK_a:
          ship.x -= ship.speed * state.delta;
          break;
        
        case SDLK_d:
          ship.x += ship.speed * state.delta;
          break;
        
        default:
          break;
        }

      }
     }

    // Render
    SDL_SetRenderTarget(game.renderer, game.texture);
    SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 0xFF);
    SDL_RenderClear(game.renderer);

    SDL_SetRenderDrawColor(game.renderer, 0xFF, 0, 0xFF, 0xFF);
    SDL_Rect fillRect = {ship.x, ship.y, 16, 16};
    SDL_RenderFillRect(game.renderer, &fillRect);

    //Draw texture to screen
    SDL_SetRenderTarget(game.renderer, NULL);
    SDL_RenderCopyEx(game.renderer, game.texture, NULL, NULL, 0, NULL, SDL_FLIP_VERTICAL);
    SDL_RenderPresent(game.renderer);

  }

  //SDL_DestroyTexture(game.texture);
  //SDL_DestroyWindow(game.window);
  //SDL_DestroyRenderer(game.renderer);

  return EXIT_SUCCESS;
}