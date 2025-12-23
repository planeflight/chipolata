#ifndef EMU_HPP
#define EMU_HPP

#include <SDL3/SDL_render.h>

#include <cstddef>
#include <string>

namespace chp {

constexpr size_t MEMORY_SIZE = 4096;
constexpr size_t NUM_REGISTERS = 16;
constexpr size_t NUM_KEYS = 16;
constexpr size_t WIDTH = 64;
constexpr size_t HEIGHT = 32;

constexpr size_t FONTSET_SIZE = 80;
constexpr size_t FONT_START_ADDR = 0x50;
constexpr size_t PROG_START_ADDR = 0x200;

class Emulator {
  public:
    enum class State { QUIT, PAUSE, RUN };

    Emulator(const std::string &file);
    ~Emulator();

    void cycle();
    void timers();
    State get_state() const;

    void display(unsigned short x, unsigned short y, unsigned short height);
    void update_keys(const bool *sdl_keys);

    void render(SDL_Renderer *renderer, int scale_factor);

    void pause_toggle();

  private:
    // 0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
    // 0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
    // 0x200-0xFFF - Program ROM and work RAM
    unsigned char memory[MEMORY_SIZE];
    unsigned char reg_V[NUM_REGISTERS];

    unsigned short I = 0;                // index reg
    unsigned short pc = PROG_START_ADDR; // program counter reg

    unsigned char graphics[WIDTH * HEIGHT];
    unsigned short stack[16];
    unsigned short sp = 0;
    // HEX based keypad 0xF
    bool keys[NUM_KEYS];

    // current opcode
    unsigned short opcode = 0;

    State state = Emulator::State::RUN;
    int scale_factor = 1;

    unsigned char delay_timer = 0, sound_timer = 0;
};

} // namespace chp

#endif // EMU_HPP
