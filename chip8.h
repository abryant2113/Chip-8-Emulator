#ifndef CHIP_8_H
#define CHIP_8_H

#include <stdint.h>

class Chip8 {
private:
    uint16_t stack[16];
    uint16_t pc;
    uint16_t I;
    uint16_t sp;
    uint16_t opcode;

    uint8_t memory[4096];

    bool key_pressed;
    
    unsigned char V[16];
    unsigned char delay_timer;
    unsigned char sound_timer;

    char * rom_buffer;

    void init();
    void fetchOpCode();
    void decodeOpCode();
    void executeOpCode();

public:
    uint8_t gfx[64 * 32];
    uint8_t key[16];
    bool drawFlag;

    Chip8();
    bool loadRom(char *);
    void emulateCycle();
};

#endif