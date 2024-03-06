#include <chrono>
#include <cmath>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

#define SHIP_SPEED 40.0f

#define SCREEN_WIDTH 224
#define SCREEN_HEIGHT 256

#define TICKS_PER_SECOND 60
#define NS_PER_SEC 1000000000
#define NS_PER_TIC (NS_PER_SEC / TICKS_PER_SECOND)

#define ROW_HEIGHT 16
#define ROW_WIDTH SCREEN_WIDTH - 32

#define PADDING 12

enum AlienTypeEnum { CYAN, RED, YELLOW, WHITE };

enum Move { LEFT, RIGHT, DOWN };

struct Vector2f {
  float x = 0;
  float y = 0;
};

struct Vector2i {
  int x = 0;
  int y = 0;
};

struct Box2f {
  Vector2f min;
  Vector2f max;
};

bool box_collide(Box2f a, Box2f b) {
  if (((a.min.x >= b.max.x) || (a.max.x <= b.min.x)) ||
      ((a.min.y >= b.max.y) || (a.max.y <= b.min.y))) {
    return false;
  }
  return true;
}

struct AlienType {
  Vector2i index;
  Vector2i size;
};

struct Alien {
  AlienTypeEnum type;
  int index;
  Vector2f pos;
  Move last_move;
};

struct Projectile {
  Vector2f pos;
  bool down;
};

struct Explosion {
  Vector2f pos;
  unsigned long long spawn_ns;
};

struct Barrier {
  Vector2f pos;
  int state;
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
    unsigned long long last_tick;
    unsigned long long tick_remainder;
    unsigned long long delta_ns;
    unsigned long long start;
    unsigned long long now;
    long double delta;
    int frames;
    int fps;
  } time;

  struct {
    struct {
      bool down, pressed;
    } left, right, shoot;
  } input;

  std::vector<Alien *> *aliens;
  std::vector<Projectile *> *projectiles;
  std::vector<Explosion *> *explosions;
  std::vector<Barrier *> *barriers;
  Move move;
  Move last_shuffle;
  float move_ticks;
  int stage_num_aliens;
  int lives;
} state;

using gameState = decltype(state);

AlienType *alien_sprites(AlienTypeEnum type) {

  AlienType *alien_data = new AlienType;

  switch (type) {

  case AlienTypeEnum::CYAN:
    alien_data->index = {3, 0};
    alien_data->size = {8, 8};
    break;

  case AlienTypeEnum::RED:
    alien_data->index = {2, 0};
    alien_data->size = {14, 8};
    break;

  case AlienTypeEnum::YELLOW:
    alien_data->index = {2, 1};
    alien_data->size = {12, 9};
    break;

  case AlienTypeEnum::WHITE:
    alien_data->index = {1, 0};
    alien_data->size = {16, 8};
    break;

  default:
    break;
  }

  return alien_data;
}

void init_stage(gameState *state) {

  int index = 0;
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 10; x++) {
      state->aliens->push_back(
          new Alien({AlienTypeEnum(rand() % 4),
                     index,
                     {(float)(10 + x * 14),
                      (float)SCREEN_HEIGHT - 100 + y * ROW_HEIGHT}}));
      index++;
    }
  }

  for (int y = 0; y < 2; y++) {
    for (int x = 0; x < 8; x++) {
      if (y == 0) {
        state->barriers->push_back(
            new Barrier({{(float)(10 - 2 + x * 28), (float)(20 + 12 * y)}, 0}));
      } else {
        state->barriers->push_back(
            new Barrier({{(float)(10 - 2 + x * 28), (float)(20 + 12 * y)}, 0}));
      }
    }
  }

  state->stage_num_aliens = state->aliens->size();
  state->move = Move::RIGHT;
}

void tick(gameState *state) {
  state->move_ticks += 1;

  int Move_speed = 3;

  if (state->move == Move::RIGHT || state->move == Move::LEFT) {
    state->last_shuffle = state->move;
  }

  for (int i = 0; i < state->aliens->size(); i++) {
    if (((int)state->move_ticks + i) % state->aliens->size() == 0) {
      switch (state->move) {
      case Move::RIGHT:
        state->aliens->at(i)->pos.x += Move_speed;
        break;
      case Move::LEFT:
        state->aliens->at(i)->pos.x -= Move_speed;
        break;
      case Move::DOWN:
        state->aliens->at(i)->pos.y -= ROW_HEIGHT;
        break;
      }

      state->aliens->at(i)->last_move = state->move;
    }

    if (rand() % 10000 < 20 || (abs(state->aliens->at(i)->pos.x - state->ship.pos->x) < 4 && (rand() % 100 < 1))) {
      state->projectiles->push_back(new Projectile(
          {{state->aliens->at(i)->pos.x + 2, state->aliens->at(i)->pos.y - 2},
           true}));
    }
  }

  bool all_moved = true;
  for (int i = 0; i < state->aliens->size(); i++) {
    if (state->aliens->at(i)->last_move != state->move) {
      all_moved = false;
      break;
    }
  }

  if (all_moved) {
    if (state->move == Move::DOWN) {
      switch (state->last_shuffle) {
      case Move::LEFT:
        state->move = Move::RIGHT;
        break;
      case Move::RIGHT:
        state->move = Move::LEFT;
        break;
      case Move::DOWN:
        assert(false);
        break;
      default:
        break;
      }
    }

    bool oob = false;

    while (true) {
      for (int i = 0; i < state->aliens->size(); i++) {
        switch (state->move) {
        case Move::RIGHT:
          if ((state->aliens->at(i)->pos.x + Move_speed) +
                  alien_sprites(state->aliens->at(i)->type)->size.x >=
              SCREEN_WIDTH - PADDING) {
            oob = true;
            break;
          }
          break;
        case Move::LEFT:
          if (state->aliens->at(i)->pos.x - Move_speed <= PADDING) {
            oob = true;
            break;
          }
          break;
        default:
          break;
        }
      }
      break;
    }

    if (oob) {
      state->move = Move::DOWN;
    }
  }
}

Box2f projectile_box(Projectile projectile) {
  Box2f box = {projectile.pos,
               Vector2f({projectile.pos.x + 2, projectile.pos.y + 7})};
  return box;
}

Box2f alien_box(Alien alien) {
  Box2f box = {alien.pos,
               Vector2f({alien.pos.x + alien_sprites(alien.type)->size.x,
                         alien.pos.y + alien_sprites(alien.type)->size.y})};
  return box;
}

Box2f barrier_box(Barrier barrier) {
  Vector2f size = {(float)14 - 2 * barrier.state, (float)6 - 2 * barrier.state};
  Box2f box = {barrier.pos,
               Vector2f({barrier.pos.x + size.x, barrier.pos.y + size.y})};
  return box;
}

Box2f ship_box(gameState *state) {
  Box2f box = {*state->ship.pos,Vector2f({state->ship.pos->x + 12, state->ship.pos->y + 10})};
  return box;
}

void update(gameState *state) {

  if (state->input.left.down) {
    state->ship.pos->x -= state->time.delta * SHIP_SPEED;
  }

  if (state->input.right.down) {
    state->ship.pos->x += state->time.delta * SHIP_SPEED;
  }

  if (state->input.shoot.pressed) {
    state->projectiles->push_back(new Projectile(
        {{state->ship.pos->x + 4, state->ship.pos->y + 11}, false}));
  }

  for (int i = 0; i < state->projectiles->size(); i++) {
    if (state->projectiles->at(i)->down) {
      state->projectiles->at(i)->pos.y -= 100 * state->time.delta;
    } else {
      state->projectiles->at(i)->pos.y += 100 * state->time.delta;
    }
  }

  Box2f shipbox = ship_box(state);
  for(int i = 0; i < state->projectiles->size(); i++) {
    if (box_collide(shipbox, projectile_box(*state->projectiles->at(i)))) {
      state->explosions->push_back(new Explosion(
          {{state->ship.pos->x + 2, state->ship.pos->y + 2},
           state->time.last_frame}));
      state->lives -= 1;
      state->projectiles->erase(state->projectiles->begin() + i);

      if(state->lives == 0) {
        exit(1);
      }
    }
  }

  // Collision projectiles & aliens
  for (int i = 0; i < state->aliens->size(); i++) {
    Box2f box = alien_box(*state->aliens->at(i));
    for (int j = 0; j < state->projectiles->size(); j++) {
      if (box_collide(box, projectile_box(*state->projectiles->at(j))) and
          !state->projectiles->at(j)->down) {
        state->explosions->push_back(new Explosion(
            {{state->aliens->at(i)->pos.x + 2, state->aliens->at(i)->pos.y + 2},
             state->time.last_frame}));
        state->projectiles->erase(state->projectiles->begin() + j);
        if (state->aliens->at(i) != NULL) {
          state->aliens->erase(state->aliens->begin() + i);
        }
      }
    }
    
    if (box_collide(box, shipbox)) {
      state->aliens->erase(state->aliens->begin() + i);
      state->explosions->push_back(new Explosion(
          {{state->aliens->at(i)->pos.x + 2, state->aliens->at(i)->pos.y + 2},
           state->time.last_frame}));

      state->explosions->push_back(
          new Explosion({{state->ship.pos->x + 2, state->ship.pos->y + 2},
                         state->time.last_frame}));
      state->lives -= 1;
      state->projectiles->erase(state->projectiles->begin() + i);

      if (state->lives == 0) {
        exit(1);
      }
    }
  }

  for (int i = 0; i < state->projectiles->size(); i++) {
    Box2f box = projectile_box( * state->projectiles->at(i));
    for (int j = 0; j < state->barriers->size(); j++) {
      if (state->projectiles->at(i)->down == true) {
        if(box_collide(box, barrier_box(*state->barriers->at(j)))) {
          state->barriers->at(j)->state += 1;
          state->projectiles->erase(state->projectiles->begin() + i);
        }
      }
    }
  }

  // Remove barriers
  for (int i = 0; i < state->barriers->size(); i++) {
    if (state->barriers->at(i)->state >= 4) {
      state->barriers->erase(state->barriers->begin() + i);
    }
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

  for (int i = 0; i < state->aliens->size(); i++) {
    draw_sprite(state, alien_sprites(state->aliens->at(i)->type)->index,
                state->aliens->at(i)->pos);
  }

  for (int i = 0; i < state->projectiles->size(); i++) {
    draw_sprite(state, Vector2i({1, int(state->time.now) % 2 + 1}),
                state->projectiles->at(i)->pos);
  }

  for (int i = 0; i < state->barriers->size(); i++) {
    draw_sprite(state, Vector2i({state->barriers->at(i)->state, 3}), state->barriers->at(i)->pos);
  }

  for (int i = 0; i < state->explosions->size(); i++) {
    int frame = (state->time.last_frame - state->explosions->at(i)->spawn_ns) /
                (0.25 * NS_PER_SEC);
    if (frame < 2) {
      draw_sprite(state, Vector2i({0, 1 + frame}),
                  state->explosions->at(i)->pos);
    } else {
      state->explosions->erase(state->explosions->begin() + i);
    }
  }

  for(int i = 0; i < state->lives; i++) {
    draw_sprite(state, Vector2i({0, 0}),
                Vector2f({(float)2 + 11 * i, (float)2}));
  }

  // Draw ship

  state->ship.pos->y = 4;
  draw_sprite(state, Vector2i{0, 0}, *state->ship.pos);

  float screen_scale = (float)state->window_size.y / (float)SCREEN_HEIGHT;

  // Draw texture to screen
  SDL_SetRenderTarget(state->renderer, NULL);
  SDL_SetRenderDrawColor(state->renderer, 20, 20, 20, 0xFF);
  SDL_RenderCopyEx(
      state->renderer, state->texture,
      makeRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT),
      makeRect(
          (int)((state->window_size.x - int(SCREEN_WIDTH * screen_scale)) / 2),
          0, (int)((float)SCREEN_WIDTH * screen_scale), state->window_size.y),
      0, NULL, SDL_FLIP_VERTICAL);
  SDL_RenderPresent(state->renderer);
}

int main(int argc, char *args[]) {

  srand(time(NULL));

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
  state.renderer =
      SDL_CreateRenderer(state.window, -1,
                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE |
                             SDL_RENDERER_PRESENTVSYNC);

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

  SDL_Surface *sprite_surface = SDL_CreateRGBSurfaceWithFormatFrom(
      data, width, height, 32, (width * 4), (SDL_PIXELFORMAT_ABGR8888));

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

  state.aliens = new std::vector<Alien *>();
  state.projectiles = new std::vector<Projectile *>();
  state.explosions = new std::vector<Explosion *>();
  state.barriers = new std::vector<Barrier *>();
  state.lives = 3;

  SDL_SetTextureColorMod(state.sprites, 0xFF, 0xFF, 0xFF);
  SDL_SetTextureAlphaMod(state.sprites, 0xFF);

  init_stage(&state);

  SDL_Event event;
  bool quit = false;

  while (quit == false) {

    unsigned long long now =
        std::chrono::time_point_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now())
            .time_since_epoch()
            .count();

    if (state.time.start == 0) {
      state.time.start = now;
    }

    state.time.now = (now - state.time.start) / NS_PER_SEC;

    if (state.time.last_frame == 0) {
      state.time.last_frame = now;
    }

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

    unsigned long long tick_time =
        state.time.tick_remainder + state.time.delta_ns;
    while (tick_time > NS_PER_TIC) {
      std::cout << "Tick time remaining: " << state.time.tick_remainder
                << std::endl;
      tick_time -= NS_PER_TIC;
      tick(&state);
    }
    state.time.tick_remainder = tick_time;
    std::cout << "Tick time remaining: " << state.time.tick_remainder
              << std::endl;

    keystates = SDL_GetKeyboardState(NULL);

    bool left =
        bool(keystates[SDL_SCANCODE_LEFT]) || bool(keystates[SDL_SCANCODE_A]);
    bool right =
        bool(keystates[SDL_SCANCODE_RIGHT]) || bool(keystates[SDL_SCANCODE_D]);
    bool shoot = bool(keystates[SDL_SCANCODE_SPACE]);

    state.input.left = {left, left};
    state.input.right = {right, right};
    state.input.shoot = {shoot, shoot};

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