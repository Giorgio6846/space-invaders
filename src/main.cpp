#include <iostream>
#include <math.h>
#include <string>
#include <vector>

#include <chrono>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

#define SHIP_SPEED 40.0f

#define SCREEN_WIDTH 224
#define SCREEN_HEIGHT 256

struct Vector2f {
  float x = 0;
  float y = 0;
};

struct Vector2i {
  int x = 0;
  int y = 0;
};

struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  SDL_Texture *sprites;

  Vector2i window_size;

  struct {
    Vector2f *pos = new Vector2f;
  } ship;

  struct {
    unsigned long long last_second;
    unsigned long long last_frame;
    unsigned long long delta_ns;
    long double delta;
    int frames;
    int fps;
  } time;

  struct {
    struct {
      bool down, pressed;
    } left, right, shoot;
  } input;

} state;

using gameState = decltype(state);

void update(gameState *state) {

  if (state->input.left.down) {
    state->ship.pos->x -= state->time.delta * SHIP_SPEED;
  }

  if (state->input.right.down) {
    state->ship.pos->x += state->time.delta * SHIP_SPEED;
  }
}

SDL_Rect *makeRect(int x, int y, int w, int h) {
  SDL_Rect *rect = new SDL_Rect;
  rect->x = x;
  rect->y = y;
  rect->w = w;
  rect->h = h;
  return rect;
}

void draw_sprite(gameState *state, Vector2i index, Vector2f pos) {
  const int SPRITE_SIZE = 16;

  // Debug purposes
  // SDL_RenderCopy(state->renderer, state->sprites, makeRect(index.x *
  // SPRITE_SIZE, index.y * SPRITE_SIZE, 64, 64), makeRect(int(pos.x),
  // int(pos.y), 64, 64));
  SDL_RenderCopy(state->renderer, state->sprites,
                 makeRect(index.x * SPRITE_SIZE, index.y * SPRITE_SIZE,
                          SPRITE_SIZE, SPRITE_SIZE),
                 makeRect(int(pos.x), int(pos.y), SPRITE_SIZE, SPRITE_SIZE));
}

void render(gameState *state) {

  // Render
  SDL_SetRenderTarget(state->renderer, state->texture);
  SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 0);
  SDL_RenderClear(state->renderer);

  // SDL_SetRenderDrawColor(state->renderer, 0xFF, 0, 0xFF, 0xFF);
  // SDL_RenderFillRect(state->renderer, makeRect((int)state->ship.pos->x,
  // (int)state->ship.pos->y, 16, 16));

  // Draw ship

  state->ship.pos->y = 4;
  draw_sprite(state, Vector2i{2, 0}, *state->ship.pos);

  float screen_scale = (float)state->window_size.y / (float)SCREEN_HEIGHT;

  // Draw texture to screen
  SDL_SetRenderTarget(state->renderer, NULL);
  SDL_SetRenderDrawColor(state->renderer, 20, 20, 20, 0xFF);
  SDL_RenderCopyEx(state->renderer, state->texture,
                   makeRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT),
                   makeRect((int)((state->window_size.x - int(SCREEN_WIDTH * screen_scale)) / 2),
                   0,
                   (int)((float)SCREEN_WIDTH * screen_scale),
                   state->window_size.y),
                   0, NULL, SDL_FLIP_VERTICAL);
  SDL_RenderPresent(state->renderer);
}

int main(int argc, char *args[]) {

  const Uint8 *keystates;

  if (SDL_Init(SDL_INIT_EVERYTHING)) {
    std::cout << "SDL_Init failed with error: " << SDL_GetError() << std::endl;
    exit(1);
  }

  if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
    std::cout << "Failed to initialize SDL_image" << SDL_GetError()
              << std::endl;
    exit(1);
  }

  // Create window
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
  state.window =
      SDL_CreateWindow("スパースインベーダー!", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 1600, 900, SDL_WINDOW_SHOWN);

  if (!state.window) {
    std::cout << "Failed to create window" << std::endl;
    return -1;
  }

  // Create renderer
  state.renderer = SDL_CreateRenderer(
      state.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_PRESENTVSYNC);

  if (!state.renderer) {
    std::cout << "Failed to create renderer" << std::endl;
    return -1;
  }

  // Create backbuffer
  state.texture =
      SDL_CreateTexture(state.renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);

  if (!state.texture) {
    std::cout << "Failed to create texture" << std::endl;
    return -1;
  }

  // load sprites
  stbi_set_flip_vertically_on_load(true);

  int width, height, channels;
  unsigned char *data =
      stbi_load("Resources/spritesheet.png", &width, &height, &channels, 4);

  if (!data) {
    std::cout << "Failed to load spritesheet" << std::endl;
    return -1;
  }

  // Todo: Due to an error with the PixelFormat or whatever the fuck it doesn't
  // recognize green in some formats
  SDL_Surface *sprite_surface = SDL_CreateRGBSurfaceWithFormatFrom(
      data, width, height, 8, (width * 4), (SDL_PIXELFORMAT_RGBA8888));

  // SDL_Surface * sprite_surface = IMG_Load("Resources/spritesheet.png");

  if (!sprite_surface) {
    std::cout << "Failed to create surface" << std::endl;
    return -1;
  }

  state.sprites = SDL_CreateTextureFromSurface(state.renderer, sprite_surface);

  if (!state.sprites) {
    std::cout << "Failed to create texture from surface" << SDL_GetError()
              << std::endl;
    return -1;
  }

  SDL_SetTextureColorMod(state.sprites, 0xFF, 0xFF, 0xFF);
  SDL_SetTextureAlphaMod(state.sprites, 0xFF);

  SDL_Event event;
  bool quit = false;
  const int NS_PER_SEC = 1000000000;

  while (quit == false) {

    unsigned long long now =
        std::chrono::time_point_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now())
            .time_since_epoch()
            .count();

    state.time.delta_ns = now - state.time.last_frame;
    state.time.delta =
        (long double)(state.time.delta_ns / (long double)NS_PER_SEC);
    state.time.last_frame = now;
    state.time.frames += 1;

    if ((now - state.time.last_second) > NS_PER_SEC) {
      state.time.last_second = now;
      state.time.fps = state.time.frames;
      state.time.frames = 0;
      std::cout << "FPS: " << state.time.fps << std::endl;
    }

    keystates = SDL_GetKeyboardState(NULL);

    state.input.left.down =
        bool(keystates[SDL_SCANCODE_LEFT]) || bool(keystates[SDL_SCANCODE_A]);
    state.input.right.down =
        bool(keystates[SDL_SCANCODE_RIGHT]) || bool(keystates[SDL_SCANCODE_D]);
    state.input.shoot.down = bool(keystates[SDL_SCANCODE_SPACE]);

    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        quit = true;
        break;

      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_a:
        case SDLK_LEFT:
          state.input.left.pressed = true;
          break;

        case SDLK_d:
        case SDLK_RIGHT:
          state.input.right.pressed = true;
          break;

        case SDLK_SPACE:
          state.input.shoot.pressed = true;
          break;

        default:
          break;
        }
      }
    }

    int w, h;
    SDL_GetWindowSize(state.window, &w, &h);
    state.window_size = Vector2i{w, h};

    update(&state);
    render(&state);
  }

  SDL_DestroyTexture(state.texture);
  SDL_DestroyWindow(state.window);
  SDL_DestroyRenderer(state.renderer);

  return EXIT_SUCCESS;
}