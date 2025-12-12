#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <iostream>

#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"

constexpr static int WIDTH = 1080, HEIGHT = 720;

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        spdlog::error("Failed to initialize SDL: {}.", SDL_GetError());
    }
    SDL_Window *window;
    SDL_Renderer *renderer;
    if (!SDL_CreateWindowAndRenderer(
            "CHIPOLATA - PIRATED GAME", WIDTH, HEIGHT, 0, &window, &renderer)) {
        spdlog::error("Failed to create window and renderer: {}.",
                      SDL_GetError());
        return -1;
    }
    spdlog::info("Emulator successfully initialized!");

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    spdlog::info("Emulator successfully deinitialized!");
    return 0;
}
