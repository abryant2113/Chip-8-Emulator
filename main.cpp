#include <iostream>
#include <fstream>
#include <cstddef>
#include <thread>
#include <cstring>
#include <string>
#include <cstdlib>
#include <stdlib.h>
#include <cerrno>
#include "chip8.h"
#include <SDL2/SDL.h>
#include </usr/include/SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>

using namespace std;

uint8_t keymap[16] = {
    SDLK_x,
    SDLK_1,
    SDLK_2,
    SDLK_3,
    SDLK_q,
    SDLK_w,
    SDLK_e,
    SDLK_a,
    SDLK_s,
    SDLK_d,
    SDLK_z,
    SDLK_c,
    SDLK_4,
    SDLK_r,
    SDLK_f,
    SDLK_v,
};

static void initLibraries()
{
    int err;

    err = SDL_Init(SDL_INIT_EVERYTHING);

    if (err) {
        cerr << "Error initializing SDL: %s\n", SDL_GetError();
        exit(err);
    }

    err = TTF_Init();

    if (TTF_Init() < 0) {
        cerr << "Error initializing TTF: %s\n", TTF_GetError();
        exit(err);
    }
}

static void printUsage()
{
    cout << "Usage: ./main [path to rom file] [debug mode: t/f]\n";
}

static void debugTrace()
{
    SDL_Event e;

    while(true) {
        SDL_PollEvent(&e);
        if (e.type == SDL_KEYDOWN) {
            if(e.key.keysym.sym == SDLK_ESCAPE)
                exit(0);
            if (e.key.keysym.sym == SDLK_SPACE)
                break;
        }
    }
}

static void updateDebugInfo(Chip8 game, char *debugString)
{
    char opcode[30];
    char pc[20];
    char sp[20];
    char IR[20];

    string opcodeStr, pcStr, spStr, IRStr, finalString;

    sprintf(opcode, "Opcode: %x ", game.getOpcode());
    sprintf(pc, "PC: %d ", game.getProgramCounter());
    sprintf(sp, "SP: %d ", game.getStackPointer());
    sprintf(IR, "IR: %d ", game.getInstructionReg());

    opcodeStr = opcode;
    pcStr = pc;
    spStr = sp;
    IRStr = IR;

    finalString = opcodeStr + " " + pcStr + " " + spStr + " " + IRStr;
    
    strcpy(debugString, finalString.c_str());
}

int main(int argc, char *argv[])
{
    Chip8 game = Chip8();
    uint32_t pixels[2048];

    bool debugMode = false;
    char *op_text;
    int h = 512;
    int w = 1024;

    SDL_Color debugTextColor;
    SDL_Rect gameSection, debugSection;
    SDL_Renderer * renderer;
    SDL_Surface *debugSurface;
    SDL_Texture *debugTexture;
    SDL_Texture *sdlTexture;
    SDL_Window * win;

    TTF_Font *gFont;

    initLibraries();

    if (argc < 2) {
        printUsage();
        exit(-1);
    }

    game.loadRom(argv[1]);

    if (argc == 3) {
        debugMode = true;
        cout << "Enabling debug mode!\n";
    }

    win = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, 0);   

    gameSection.x = 0;
    gameSection.y = 0;
    gameSection.w = 512;
    gameSection.h = 512;

    debugSection.x = 768;
    debugSection.y = 256;
    debugSection.w = 250;
    debugSection.h = 30;

    renderer = SDL_CreateRenderer(win, -1, 0);
    SDL_RenderSetLogicalSize(renderer, w, h);

    debugTextColor = { 255, 255, 255 };
    gFont = TTF_OpenFont( "16_true_type_fonts/arial.ttf", 28 );

    if (gFont == NULL) { 
        cerr << "Issues loading desired font. error %s\n", TTF_GetError();
        exit(ENOENT);
    }

    sdlTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);

    op_text = (char*)malloc(sizeof(char) * 200);

    while (true) {
        game.emulateCycle();

        SDL_Event e;

        if (debugMode)
            debugTrace();

        updateDebugInfo(game, op_text);

        debugSurface = TTF_RenderText_Solid(gFont, op_text, debugTextColor);
        debugTexture = SDL_CreateTextureFromSurface(renderer, debugSurface);

        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) exit(0);

            if(e.type == SDL_KEYDOWN) {
                if(e.key.keysym.sym == SDLK_ESCAPE)
                    exit(0);

                for(int i = 0; i < 16; i++) {
                    if(e.key.keysym.sym == keymap[i]) {
                        game.key[i] = 1;
                    }
                }
            }

            if(e.type == SDL_KEYUP) {
                for(int i = 0; i < 16; i++) {
                    if(e.key.keysym.sym == keymap[i]){
                        game.key[i] = 0;
                    }
                }
            }
        }
    
        if(game.drawFlag) {
            game.drawFlag = false;

            for(int i = 0; i < 2048; i++)
            {
                uint8_t pixel = game.gfx[i];

                if(pixel == 1) {
                    pixels[i] = 0xFFFFFFFF;
                } else {pixels[i] = 0xFF000000;
                    pixels[i] = 0xFF000000;
                }
            }

            SDL_UpdateTexture(sdlTexture, NULL, pixels, 64 * sizeof(Uint32));
        }            
        
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, sdlTexture, NULL, &gameSection);
        SDL_RenderCopy(renderer, debugTexture, NULL, &debugSection);
        SDL_RenderPresent(renderer);

        std::this_thread::sleep_for(std::chrono::microseconds(1200));
    }

    free(op_text);

    return 0;
}

