#include "emu.hpp"

#include <spdlog/spdlog.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdexcept>

using namespace chp;

const uint8_t fontset[FONTSET_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Emulator::Emulator(const std::string &file) {
    srand(time(nullptr));
    memset(memory, 0, MEMORY_SIZE);
    memset(reg_V, 0, NUM_REGISTERS);
    memset(graphics, 0, sizeof(graphics));
    memset(keys, 0, NUM_KEYS);

    // open file and copy program into memory
    FILE *fd = fopen(file.c_str(), "rb");
    if (!fd) {
        spdlog::error("Failed to open file '{}'", file);
        state = State::QUIT;
        return;
    }

    // Get/check rom size
    fseek(fd, 0, SEEK_END);
    const size_t rom_size = ftell(fd);
    const size_t max_size = MEMORY_SIZE - PROG_START_ADDR;
    rewind(fd);

    if (rom_size > max_size) {
        spdlog::error("Rom file is too big!");
        state = State::QUIT;
        return;
    }

    if (fread(&memory[PROG_START_ADDR], rom_size, 1, fd) != 1) {
        throw std::runtime_error("Could not read ROM file into memory.");
    }

    // Close the file
    fclose(fd);

    for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
        memory[FONT_START_ADDR + i] = fontset[i];
    }
}

Emulator::~Emulator() {}

void Emulator::cycle() {
    // emulate the CHIP8 cpu cycle
    if (state != State::RUN) {
        return;
    }

    // fetch
    // big to little endian
    opcode = memory[pc] << 8 | memory[pc + 1];
    // printf(
    //     "opcode: %x %x %x\n", opcode, opcode & 0xF000, (opcode >> 12) &
    //     0x0F);
    int NNN = opcode & 0x0FFF;
    int NN = opcode & 0x00FF;
    int N = opcode & 0x000F;
    int X = (opcode >> 8) & 0x0F;
    int Y = (opcode >> 4) & 0x0F;
    uint16_t s = reg_V[Y] + reg_V[X];

    pc += 2;

    // decode
    switch (opcode & 0xF000) {
        case 0x0000:
            if (opcode == 0x00E0) // 0x00E0: Clears the screen
            {
                memset(graphics, 0, WIDTH * HEIGHT);
            } else if (opcode == 0x00EE) // 0x00EE: Returns from subroutine
            {
                if (sp == 0) {
                    printf("Can't have sp less than 0");
                    break;
                }
                pc = stack[--sp];
            }
            break;
        case 0x1000: // 1NNN: Jump/goto to address NNN
            pc = NNN;
            break;
        case 0x2000: // 2NNN: Calls subroutine at NNN
            stack[sp] = pc;
            ++sp;
            pc = NNN;
            break;
        case 0x3000: // 0x3XNN: if VX == NN
            if (reg_V[X] == NN) {
                pc += 2;
            }
            break;
        case 0x4000: // 0x4XNN: if VX != NN
            if (reg_V[X] != NN) {
                pc += 2;
            }
            break;
        case 0x5000: // 0x5XY0: if VX == VY
            if (reg_V[X] == reg_V[Y]) {
                pc += 2;
            }
            break;
        case 0x6000: // 0x6XNN: Vx = NN
            reg_V[X] = NN;
            break;
        case 0x7000: // 0x7XNN: Vx += NN
            reg_V[X] += NN;
            break;
        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0000: // 0x8XY0: VX = VY
                    reg_V[X] = reg_V[Y];
                    break;
                case 0x0001: // 0x8XY1: VX |= Vy
                    reg_V[X] |= reg_V[Y];
                    break;
                case 0x0002: // 0x8XY2: VX &= Vy
                    reg_V[X] &= reg_V[Y];
                    break;
                case 0x0003: // 0x8XY3: VX ^= Vy
                    reg_V[X] ^= reg_V[Y];
                    break;
                case 0x0004: // 0x8XY4: VX += VY
                    if (s > 0xFF)
                        reg_V[0xF] = 1; // carry
                    else
                        reg_V[0xF] = 0;
                    reg_V[X] = s & 0xFF;
                    break;
                case 0x0005: // 0x8XY5: VX -= VY
                    reg_V[0xF] = (reg_V[X] > reg_V[Y]) ? 1 : 0;
                    reg_V[X] -= reg_V[Y];
                    break;
                case 0x0006: // 0x8XY6: VX >>= 1
                    reg_V[0xF] = reg_V[X] & 0x1;
                    reg_V[X] >>= 1;
                    break;
                case 0x0007: // 0x8XY7: VX = VY - VX
                    reg_V[0xF] = (reg_V[Y] > reg_V[X]) ? 1 : 0;
                    reg_V[X] = reg_V[Y] - reg_V[X];
                    break;
                case 0x000E: // 0x8XYE: VX <<= 1
                    reg_V[0xF] = (reg_V[X] & 0x80) >> 7;
                    reg_V[X] <<= 1;
                    break;
            }
            break;
        case 0x9000: // 0x9XY0: if (VX != VY)
            if (reg_V[X] != reg_V[Y]) pc += 2;
            break;
        case 0xA000: // ANNN: Sets I to the address NNN
            // Execute opcode
            I = NNN;
            break;
        case 0xB000: // BNNN: Jumps to address NNN + V0
            pc = reg_V[0] + NNN;
            break;
        case 0xC000: // 0xCXNN: Vx = rand() & NN
            reg_V[X] = (rand() % 256) & NN;
            break;
        case 0xD000: // 0xDXYN: Display(Vx, Vy, N)
            display(reg_V[X], reg_V[Y], N);
            break;
        case 0xE000:
            switch (opcode & 0x000F) {
                case 0x000E: // 0xEX9E: if (key() == VX)
                    if (keys[reg_V[X]]) {
                        pc += 2;
                    }
                    break;
                case 0x0001: // 0xEXA1: if (key() != VX)
                    if (!keys[reg_V[X]]) {
                        pc += 2;
                    }
                    break;
            }
            break;
        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0007: // 0xFX07: Vx = get_delay()
                    reg_V[X] = delay_timer;
                    break;
                case 0x000A: // 0xFX0A: Vx = get_key()
                    for (int i = 0; i < 16; ++i) {
                        if (keys[i]) {
                            reg_V[X] = i;
                            return; // foudn a key press
                        }
                    }
                    // no key press found
                    pc -= 2;
                    break;
                case 0x0015: // 0xFX15: delay_timer(Vx)
                    delay_timer = reg_V[X];
                    break;
                case 0x0018: // 0xFX18: soudn_timer(Vx)
                    sound_timer = reg_V[X];
                    break;
                case 0x001E: // 0xFX1E: I += Vx
                    I += reg_V[X];
                    break;
                case 0x0029: // 0xFX29: I = sprite_addr[Vx]
                    I = FONT_START_ADDR + (5 * reg_V[X]);
                    break;
                case 0x0033: // 0xFX33:
                    memory[I] = reg_V[X] / 100;
                    memory[I + 1] = (reg_V[X] / 10) % 10;
                    memory[I + 2] = (reg_V[X] % 100) % 10;
                    break;
                case 0x0055: // 0xFX55: reg_dump(Vx, &I)
                    for (int i = 0; i <= X; ++i) {
                        memory[I + i] = reg_V[i];
                    }
                    break;
                case 0x0065: // 0xFX65: reg_load(Vx, &I)
                    for (int i = 0; i <= X; ++i) {
                        reg_V[i] = memory[I + i];
                    }
                    break;
            }
            break;
        default:
            spdlog::warn("Unknown opcode: 0x{:X}\n", opcode);
    }
}

void Emulator::timers() {
    // timers
    if (delay_timer > 0) delay_timer--;
    if (sound_timer > 0) {
        if (sound_timer == 1) spdlog::info("SOUND!");
        sound_timer--;
    }
}

Emulator::State Emulator::get_state() const {
    return state;
}

void Emulator::display(unsigned short x,
                       unsigned short y,
                       unsigned short height) {
    x = x % WIDTH;
    y = y % HEIGHT;
    reg_V[0xF] = 0;
    // each row
    for (unsigned short i = 0; i < height; ++i) {
        unsigned short value = memory[I + i];
        // each pixel in the row
        for (unsigned short j = 0; j < 8; ++j) {
            if (i + y >= HEIGHT || j + x >= WIDTH) continue;
            // if current pixel is 1
            if (((0x80 >> j) & value) != 0) {
                // if theres already 1 on graphics, collision
                if (graphics[(i + y) * WIDTH + x + j] == 1) {
                    reg_V[0xF] = 1;
                }
                graphics[(i + y) * WIDTH + x + j] ^= 1;
            }
        }
    }
}

void Emulator::update_keys(const bool *sdl_keys) {
    keys[1] = sdl_keys[SDL_SCANCODE_1];
    keys[2] = sdl_keys[SDL_SCANCODE_2];
    keys[3] = sdl_keys[SDL_SCANCODE_3];
    keys[0xC] = sdl_keys[SDL_SCANCODE_4];
    keys[4] = sdl_keys[SDL_SCANCODE_Q];
    keys[5] = sdl_keys[SDL_SCANCODE_W];
    keys[6] = sdl_keys[SDL_SCANCODE_E];
    keys[0xD] = sdl_keys[SDL_SCANCODE_R];
    keys[7] = sdl_keys[SDL_SCANCODE_A];
    keys[8] = sdl_keys[SDL_SCANCODE_S];
    keys[9] = sdl_keys[SDL_SCANCODE_D];
    keys[0xE] = sdl_keys[SDL_SCANCODE_F];
    keys[0xA] = sdl_keys[SDL_SCANCODE_Z];
    keys[0] = sdl_keys[SDL_SCANCODE_X];
    keys[0xB] = sdl_keys[SDL_SCANCODE_C];
    keys[0xF] = sdl_keys[SDL_SCANCODE_V];
}

void Emulator::render(SDL_Renderer *renderer, int scale_factor) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            if (graphics[y * WIDTH + x] != 0) {
                SDL_FRect rect;
                rect.x = x * scale_factor, rect.y = y * scale_factor,
                rect.w = scale_factor, rect.h = scale_factor;
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
}

void Emulator::pause_toggle() {
    state = state == State::PAUSE ? State::RUN : State::PAUSE;
}
