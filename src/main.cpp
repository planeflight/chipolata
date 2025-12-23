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

    chp::Emulator emu{"./res/space_invaders.ch8"};
    spdlog::info("Emulator successfully initialized!");

    bool running = true;
    uint64_t time = SDL_GetTicksNS();

    const int INSTRUCTIONS_PER_SECOND = 700;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_SPACE) {
                        emu.pause_toggle();
                    }
            }
        }

        SDL_PumpEvents();
        const bool *keys = SDL_GetKeyboardState(nullptr);
        if (keys[SDL_SCANCODE_ESCAPE]) {
            running = false;
        }
        emu.update_keys(keys);

        for (int i = 0; i < INSTRUCTIONS_PER_SECOND / FPS; ++i) {
            emu.cycle();
        }
        emu.timers();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        emu.render(renderer);

        SDL_RenderPresent(renderer);
        uint64_t delay =
            std::max(1.0 / FPS * 1000000000 - (SDL_GetTicksNS() - time), 0.0);
        // printf("delay: %ld", delay);
        SDL_DelayNS(delay);
        double dt = (SDL_GetTicksNS() - time) / 1000000000.0;
        // SDL_Delay(16);
        // printf("%lf\n", dt);
        time = SDL_GetTicksNS();
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    spdlog::info("Emulator successfully deinitialized!");
    return 0;
}
