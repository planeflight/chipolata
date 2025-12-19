#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <iostream>

#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include "emu.hpp"

constexpr static int WIDTH = chp::WIDTH * chp::SCALE_FACTOR,
                     HEIGHT = chp::HEIGHT * chp::SCALE_FACTOR;

constexpr static int FPS = 60;

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

    chp::Emulator emu{"./res/IBMLogo.ch8"};
    spdlog::info("Emulator successfully initialized!");

    bool running = true;
    uint64_t time = SDL_GetTicksNS();
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
            }
        }

        const bool *keys = SDL_GetKeyboardState(nullptr);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        emu.render(renderer);

        SDL_RenderPresent(renderer);
        SDL_DelayNS(std::max(
            (double)FPS / 1000000000 - (SDL_GetTicksNS() - time), 0.0));
        time = SDL_GetTicksNS();

        emu.cycle();
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    spdlog::info("Emulator successfully deinitialized!");
    return 0;
}
