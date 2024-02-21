#include <iostream>
#include <SDL2/SDL.h>

#include "defs.h"

int main(int argc, char* args[]) {

    SDL_Window * window = NULL;
    SDL_Surface * window_surface = NULL;

    if (SDL_Init(SDL_INIT_EVERYTHING))
    {
        std::cout << "SDL_Init failed with error: " << SDL_GetError() << std::endl;
        exit(1);
    }

    window = SDL_CreateWindow("スパースインベーダー!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT,SDL_WINDOW_SHOWN);

    if(!window)
    {
        std::cout << "Failed to create window" << std::endl;
        return -1;
    }

    window_surface = SDL_GetWindowSurface(window);

    if(!window_surface)
    {
        std::cout << "Failed to create window surface" << std::endl; 
        return -1;
    }

    SDL_FillRect(window_surface, NULL, SDL_MapRGB(window_surface -> format, 0x00, 0x00, 0x00));

    SDL_UpdateWindowSurface(window);

    SDL_Event e;
    bool quit = false;
    while (quit == false)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                quit = true;
        }
    }

    return EXIT_SUCCESS;
}
