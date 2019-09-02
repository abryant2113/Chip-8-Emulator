#include <iostream>
#include <ctime>
#include "chip8.h"

unsigned char chip8_fontset[80] =
    {
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

Chip8::Chip8()
{
    init();
};

void Chip8::init()
{
    pc = 0x200;
    opcode = 0;
    I = 0;
    sp = 0;

    for (int i = 0; i < 2048; i++)
    {
        gfx[i] = 0;
    }

    for (int i = 0; i < 16; i++)
    {
        stack[i] = 0;
        key[i] = 0;
        V[i] = 0;
    }

    for (int j = 0; j < 4096; j++)
    {
        memory[j] = 0;
    }

    for (int i = 0; i < 80; i++)
    {
        memory[i] = chip8_fontset[i];
    }

    delay_timer = 0;
    sound_timer = 0;
}

bool Chip8::loadRom(char *file_path)
{

    rom_buffer = 0;

    std::cout << "Loading ROM: " << file_path << "\n";

    FILE *rom = fopen(file_path, "rb");

    if (rom == NULL)
    {
        std::cout << "Failed to open ROM";
    }

    // iterates over the length of the rom file and captures the size
    fseek(rom, 0, SEEK_END);
    long rom_size = ftell(rom);
    // brings the pointer back to the beginning of the file
    rewind(rom);

    std::cout << rom_size;

    rom_buffer = (char *)malloc(sizeof(char) * rom_size);

    if (rom_buffer == NULL)
    {
        std::cerr << "Couldn't allocate memory for ROM" << std::endl;
    }

    // reads in data from the file
    size_t result = fread(rom_buffer, sizeof(char), (size_t)rom_size, rom);

    if (result != rom_size)
    {
        std::cerr << "Something went wrong during the ROM read.\n"
                  << std::endl;
    }

    for (size_t i = 0; i < rom_size; i++)
    {
        std::cout << "RomBuffer[" << i << "] " << rom_buffer[i];
        std::cout << "\n";
    }

    if ((4096 - 512) < rom_size)
    {
        std::cout << "Rom is too large in size.";
        return false;
    }

    // loads the rom data into memory
    for (size_t j = 0; j < rom_size; j++)
    {
        memory[j + 512] = (uint8_t)rom_buffer[j];
    }

    fclose(rom);

    free(rom_buffer);

    return true;
}

void Chip8::emulateCycle()
{
    fetchOpCode();
    decodeOpCode();
}

void Chip8::fetchOpCode()
{
    // each index in our memory array is one byte in length and each opcode instruction is
    // two bytes in length, so we should merge the two instructions together like so
    opcode = memory[pc] << 8 | memory[pc + 1];
}

void Chip8::decodeOpCode()
{

    std::cout << "Reading opcode: " << (opcode) << "\n";

    switch (opcode & 0xF000)
    {

    case 0x0000:
        switch (opcode & 0x000F)
        {
        // clears the screen
        case 0x0000:
            printf("Clearing screen...\n");
            // iterates over the graphics memory and clears all bits
            for (int i = 0; i < 2048; i++)
                gfx[i] = 0;

            pc += 2;
            drawFlag = true;
            break;
        // return from subroutine
        case 0x000E:
            printf("Return from subroutine.\n");
            sp--;
            pc = stack[sp];
            pc += 2;
            break;
        default:
            printf("Invalid opcode: [0xxx]: %.4X\n", opcode);
            exit(EXIT_FAILURE);
        }
        // call, display, or flow
        break;
    case 0x1000:
        // goto NNN; -- Updates the program counter to jump to the lower 12 bits of the instruction
        printf("goto NNN\n");
        pc = opcode & 0x0FFF;
        break;
    case 0x2000:
        // calls a subroutine at a specified address
        // push the current program counter onto the stack
        printf("calling subroutine...\n");
        stack[sp++] = pc;
        // update program counter to be at the address of the subroutine
        pc = opcode & 0x0FFF;
        break;
    case 0x3000:
        // skips the next instruction if VX equals NN -- if(Vx==NN)
        // must shift the resulting index a byte to the right to prevent an IOBE.
        if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
        {
            pc += 4;
        }
        else
        {
            pc += 2;
        }
        break;
    case 0x4000:
        // skips the next instruction if VX != NN
        if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
        {
            pc += 4;
        }
        else
        {
            pc += 2;
        }
        break;
    case 0x5000:
        // skips the next instruction if VX equals VY
        if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 8])
        {
            pc += 4;
        }
        else
        {
            pc += 2;
        }
        break;
    case 0x6000:
        // sets VX to NN
        V[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF);
        pc += 2;
        break;
    case 0x7000:
        // Adds NN to Vx (Carry flag is not changed)
        V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
        pc += 2;
        break;
    case 0x8000:
        switch (opcode & 0x000F)
        {
        // assign Vx = Vy
        case 0x0000:
            V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
            pc += 2;
            break;
        // assign Vx to Vx or Vy (Bitwise Or)
        case 0x0001:
            V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] | V[(opcode & 0x00F0) >> 4];
            pc += 2;
            break;
        // Sets Vx to Vx and Vy (Bitwise and)
        case 0x0002:
            V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] & V[(opcode & 0x00F0) >> 4];
            pc += 2;
            break;
        // sets Vx to Vx xor Vy (exclusive or)
        case 0x0003:
            V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] ^ V[(opcode & 0x00F0) >> 4];
            pc += 2;
            break;
        // Adds Vy to Vx. VF is set to 1 when there's a carry and 0 when there isn't
        case 0x0004:
            V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
            // there's a carry out if Vx + Vy < Vy
            if (V[(opcode & 0x00F0) >> 4] > (V[(opcode & 0x0F00) >> 8]))
            {
                V[0xF] = 1;
            }
            else
            {
                V[0xF] = 0;
            }
            pc += 2;
            break;
        // Vy is subtracted from Vx. Vf is set to 0 when there's a borrow, and 1 when there isn't
        case 0x0005:
            if (V[(opcode & 0x0F00) >> 8] < V[(opcode & 0x00F0) >> 4])
            {
                V[0xF] = 0;
            }
            else
            {
                V[0xF] = 1;
            }
            V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
            pc += 2;
            break;
        // sets the least significant bit of Vx to Vf
        case 0x0006:
            V[0xF] = (V[(opcode & 0x0F00) >> 8] & 0x1);
            V[(opcode & 0x0F00) >> 8] >>= 1;
            pc += 2;
            break;
        // sets vx to vy minus vx. vf is set to 0 when there's a borrow and 1 when there isn't
        case 0x0007:
            if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
            {
                V[0xF] = 0;
            }
            else
            {
                V[0xF] = 1;
            }
            V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
            pc += 2;
            break;
        // stores the most significant bit of Vx in Vf and then shifts Vx to the left by one
        case 0x000E:
            V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x80;
            V[(opcode & 0x0F00) >> 8] <<= 1;
            pc += 2;
            break;
        default:
            printf("Invalid opcode: [8xxx]: %.4X\n", opcode);
            exit(EXIT_FAILURE);
        }
        break;
    // skip the next instruction if Vx doesn't equal Vy
    case 0x9000:
        if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
        {
            pc += 4;
        }
        else
        {
            pc += 2;
        }
        break;
    // Sets I to the address NNN
    case 0xA000:
        std::cout << opcode << "Sets I to the address NNN\n";
        I = opcode & 0x0FFF;
        pc += 2;
        break;
    // Jumps to the address NNN plus V0
    case 0xB000:
        std::cout << opcode << " Jumps to the address NNN plus V0\n";
        pc = (opcode & 0x0FFF) + V[0];
        break;
    // sets vx to the result of a bitwise and operation on a random number
    case 0xC000:
        srand(time(NULL));
        V[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF) & (std::rand() % 256);
        pc += 2;
        break;
    // Draws a sprite at coord (Vx, Vy) that has a width of 8 pixels and a height of N pixels
    case 0xD000:
    {
        unsigned short x = V[(opcode & 0x0F00) >> 8];
        unsigned short y = V[(opcode & 0x00F0) >> 4];
        unsigned short height = opcode & 0x000F;
        unsigned short pixel;

        V[0xF] = 0;
        for (int yline = 0; yline < height; yline++)
        {
            pixel = memory[I + yline];
            for (int xline = 0; xline < 8; xline++)
            {
                if ((pixel & (0x80 >> xline)) != 0)
                {
                    if (gfx[(x + xline + ((y + yline) * 64))] == 1)
                    {
                        V[0xF] = 1;
                    }
                    else 
                    {
                        V[0xF] = 0;
                    }
                    gfx[x + xline + ((y + yline) * 64)] ^= 1;
                }
            }
        }

        drawFlag = true;
        pc += 2;
        break;
    }
    case 0xE000:
        switch (opcode & 0x000F)
        {
        // skips the next instruction if the key stored in Vx is pressed
        case 0x000E:
            if (key[V[(opcode & 0x0F00) >> 8]] != 0)
            {
                pc += 4;
            }
            else
            {
                pc += 2;
            }
            break;
        // skips the next instruction if the key stored in Vx isn't pressed
        case 0x0001:
            if (key[V[(opcode & 0x0F00) >> 8]] != 0)
            {
                pc += 2;
            }
            else
            {
                pc += 4;
            }
            break;

        default:
            printf("Invalid opcode: [Exxx]: %.4X\n", opcode);
            exit(EXIT_FAILURE);
        }
        break;
    case 0xF000:
        switch (opcode & 0x00FF)
        {
        // sets vx to the value of the delay timer
        case 0x0007:
            V[(opcode & 0x0F00) >> 8] = delay_timer;
            pc += 2;
            break;
        // a key press is awaited, and then stored in Vx
        case 0x000A:
            key_pressed = false;

            for (int j = 0; j < 16; j++)
            {
                if (key[j] != 0)
                {
                    V[(opcode & 0x0F00) >> 8] = j;
                    key_pressed = true;
                }
            }

            if (key_pressed == false)
            {
                return;
            }

            pc += 2;
            break;
        // sets the delay timer to vx
        case 0x0015:
            delay_timer = V[(opcode & 0x0F00) >> 8];
            pc += 2;
            break;
        // sets the sound timer to vx
        case 0x0018:
            sound_timer = V[(opcode & 0x0F00) >> 8];
            pc += 2;
            break;
        // adds vx to i
        case 0x001E:
            if (I + V[(opcode & 0x0F00) >> 8] > 0xFFF)
            {
                V[0xF] = 1;
            }
            else
            {
                V[0xF] = 0;
            }
            I += V[(opcode & 0x0F00) >> 8];
            pc += 2;
            break;
        // sets I to the location of the sprite for the character in Vx -- each sprite is 5 bytes long
        case 0x0029:
            I = V[(opcode & 0x0F00) >> 8] * 0x5;
            pc += 2;
            break;
        // stores the binary-coded decimal rep of Vx
        case 0x0033:
            memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
            memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
            memory[I + 2] = V[(opcode & 0x0F00) >> 8] % 10;
            pc += 2;
            break;
        // store v0 to vx in memory starting at address I
        case 0x0055:
            for (int j = 0; j < (opcode & 0x0F00); j++)
            {
                memory[I + j] = V[j];
            }
            pc += 2;
            break;
        // fills v0 to vx with values from memory starting at address I
        case 0x0065:
            for (int j = 0; j < (opcode & 0x0F00); j++)
            {
                V[j] = memory[I + j];
            }
            pc += 2;
            break;
        default:
            printf("Invalid opcode: [Fxxx]: %.4X\n", opcode);
            exit(EXIT_FAILURE);
        }
        break;
    default:
        printf("Invalid opcode: [xxxx]: %.4X\n", opcode);
        exit(EXIT_FAILURE);
    }

    if (delay_timer > 0)
        delay_timer--;

    if (sound_timer > 0)
        if (sound_timer == 1)
            // sound will be played here

    --sound_timer;
}