#include "emu.hpp"

#include <spdlog/spdlog.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

using namespace chp;

Emulator::Emulator(const std::string &file) {
    memset(memory, 0, MEMORY_SIZE);
    memset(reg_V, 0, NUM_REGISTERS);

    // open file and copy program into memory
    FILE *fd = fopen(file.c_str(), "rb");
    if (!fd) {
        // TODO: handle this properly
        throw std::runtime_error("Failed to open file");
        return;
    }

    char byte;
    int i = 512;
    while ((byte = fgetc(fd)) != EOF) {
        memory[i++] = byte;
    }

    // Close the file
    fclose(fd);
}
Emulator::~Emulator() {}

void Emulator::cycle() {
    // emulate the CHIP8 cpu cycle
    if (switch_next) {
        switch_next = false;
        pc += 2;
        return;
    }

    // fetch
    // big to little endian
    opcode = memory[pc] << 8 | memory[pc + 1];
    int NNN = opcode & 0x0FFF;
    int NN = opcode & 0x00FF;
    int N = opcode & 0x000F;
    int X = (opcode >> 8) & 0x0F;
    int Y = (opcode >> 4) & 0x0F;

    // decode
    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x000F) {
                case 0x0000: // 0x00E0: Clears the screen
                    memset(graphics, 0, sizeof(graphics));
                    break;

                case 0x000E: // 0x00EE: Returns from subroutine
                    if (sp == 0) {
                        printf("Can't have sp less than 0");
                        break;
                    }
                    pc = stack[--sp];
                    break;

                default:
                    printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
            }
            break;
        case 0x1000: // 1NNN: Jump/goto to address NNN
            stack[sp] = pc;
            ++sp;
            pc = opcode & 0x0FFF;
            break;
        case 0x2000: // 2NNN: Calls subroutine at NNN
            stack[sp] = pc;
            ++sp;
            pc = opcode & 0x0FFF;
            break;
        case 0x3000: // 0x3XNN: if VX == NN
            if (reg_V[X] == NN) {
                switch_next = true;
            }
            pc += 2;
            break;
        case 0x4000: // 0x4XNN: if VX != NN
            if (reg_V[X] != NN) {
                switch_next = true;
            }
            pc += 2;
            break;
        case 0x5000: // 0x5XY0: if VX == VY
            if (reg_V[X] == reg_V[Y]) {
                switch_next = true;
            }
            pc += 2;
            break;
        case 0x6000: // 0x6XNN: Vx = NN
            reg_V[X] = NN;
            pc += 2;
            break;
        case 0x7000: // 0x7XNN: Vx += NN
            reg_V[X] += NN;
            pc += 2;
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
                    if (reg_V[(opcode & 0x00F0) >> 4] >
                        (0xFF - reg_V[(opcode & 0x0F00) >> 8]))
                        reg_V[0xF] = 1; // carry
                    else
                        reg_V[0xF] = 0;
                    reg_V[(opcode & 0x0F00) >> 8] +=
                        reg_V[(opcode & 0x00F0) >> 4];
                    break;
                case 0x0005: // 0x8XY5: VX -= VY
                    reg_V[0xF] = (reg_V[X] >= reg_V[Y]) ? 1 : 0;
                    reg_V[X] -= reg_V[Y];
                    break;
                case 0x0006: // 0x8XY6: VX >>= 1
                    reg_V[0xF] = reg_V[X] & 0x1;
                    reg_V[X] >>= 1;
                case 0x0007: // 0x8XY7: VX = VY - VX
                    reg_V[0xF] = (reg_V[Y] >= reg_V[X]) ? 1 : 0;
                    reg_V[X] = reg_V[Y] - reg_V[X];
                    break;
                case 0x000E: // 0x8XYE: VX <<= 1
                    reg_V[0xF] = reg_V[X] & (1 << (sizeof(unsigned short) - 1));
                    reg_V[X] <<= 1;
                    break;
            }
            pc += 2;
            break;
        case 0x9000: // 0x9XY0: if (VX != VY)
            if (reg_V[X] != reg_V[Y]) switch_next = true;
            pc += 2;
            break;
        case 0xA000: // ANNN: Sets I to the address NNN
            // Execute opcode
            I = NNN;
            pc += 2;
            break;
        case 0xB000: // BNNN: Jumps to address NNN + V0
            pc = reg_V[0] + NNN;
            break;
        case 0xC000: // 0xCXNN: Vx = rand() & NN
            // TODO: seed random
            reg_V[X] = rand() & NN;
            pc += 2;
            break;
        case 0xD000: // 0xDXYN: Display(Vx, Vy, NN)
            display(reg_V[X], reg_V[Y], NN);
            pc += 2;
            break;
        case 0xE000:
            switch (opcode & 0x000F) {
                case 0x000E: // 0xEX9E: if (key() == VX)
                    if (key() == reg_V[X]) {
                        switch_next = true;
                    }
                    break;
                case 0x0001: // 0xEXA1: if (key() != VX)
                    if (key() != reg_V[X]) {
                        switch_next = true;
                    }
                    break;
            }
            pc += 2;
            break;
        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0007: // 0xFX07: Vx = get_delay()
                    reg_V[X] = delay_timer;
                    break;
                case 0x000A: // 0xFX0A: Vx = get_key()
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
                    I = sprite_addr[X];
                    break;
                case 0x0033: // 0xFX33: TODO: this
                    memory[I] = reg_V[X] / 100;
                    memory[I + 1] = (reg_V[X] / 10) % 10;
                    memory[I + 2] = (reg_V[X] % 100) % 10;
                    pc += 2;
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
            pc += 2;
            break;
        default:
            spdlog::warn("Unknown opcode: 0x{:X}\n", opcode);
    }

    // execute

    // timers
    if (delay_timer > 0) delay_timer--;
    if (sound_timer > 0) {
        if (sound_timer == 1) spdlog::info("SOUND!");
        sound_timer--;
    }
}

void Emulator::quit() {
    state = State::QUIT;
}
