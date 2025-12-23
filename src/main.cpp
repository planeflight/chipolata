#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <iostream>

#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include "emu.hpp"

inline void print_usage() {
    spdlog::error(
        "Invalid Usage: chipolata <file> "
        "<optional-scale-factor> <optional-fps>");
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 4) {
        print_usage();
        return 1;
    }

    int fps = 60;
    int scale_factor = 20;
    std::string file;
    file = argv[1];
    if (argc > 2) {
        scale_factor = std::atoi(argv[2]);
        if (scale_factor <= 0) {
            scale_factor = 20;
            spdlog::error("Scale factor cannot be <= 0!");
            return 1;
        }
        if (argc == 4) {
            fps = std::atoi(argv[3]);
            if (fps <= 0) {
                fps = 20;
                spdlog::error("FPS cannot be <= 0!");
                return 1;
            }
        }
    }
    // finally init SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        spdlog::error("Failed to initialize SDL: {}.", SDL_GetError());
    }
    SDL_Window *window;
    SDL_Renderer *renderer;

    const int width = chp::WIDTH * scale_factor,
              height = chp::HEIGHT * scale_factor;

    if (!SDL_CreateWindowAndRenderer(
            "CHIPOLATA - PIRATED GAME", width, height, 0, &window, &renderer)) {
        spdlog::error("Failed to create window and renderer: {}.",
                      SDL_GetError());
        return -1;
    }

    // open the file and initialize emulator
    chp::Emulator emu{file};
    // if error
    if (emu.get_state() == chp::Emulator::State::QUIT) {
        return -1;
    }
    spdlog::info("Emulator successfully initialized!");
    spdlog::info("  Scale Factor: {}", scale_factor);
    spdlog::info("  FPS: {}", fps);

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

        for (int i = 0; i < INSTRUCTIONS_PER_SECOND / fps; ++i) {
            emu.cycle();
        }
        emu.timers();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        emu.render(renderer, scale_factor);

        SDL_RenderPresent(renderer);
        uint64_t delay =
            std::max(1.0 / fps * 1000000000.0 - (SDL_GetTicksNS() - time), 0.0);
        SDL_DelayNS(delay);
        time = SDL_GetTicksNS();
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    spdlog::info("Emulator successfully deinitialized!");
    return 0;
}
