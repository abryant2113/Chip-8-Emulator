#include <iostream>
#include <fstream>
#include <cstddef>
#include <thread>
#include <cstring>
#include "chip8.h"
// contains the initializing and shutdown functions for SDL
#include <SDL2/SDL.h>
#include </usr/include/SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>


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

int main(int argc, char *argv[])
{
    Chip8 game = Chip8();
    uint32_t pixels[2048];

    int w = 1024;
    int h = 512;

    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        cout << "Error initializing SDL: %s\n", SDL_GetError();
    }

    if(argc < 2) {
        cout << "Error. Please supply a ROM file.";
    }

    game.loadRom(argv[1]);


    SDL_Window * win = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, 0);   

    SDL_Renderer * renderer = SDL_CreateRenderer(win, -1, 0);
    SDL_RenderSetLogicalSize(renderer, w, h);

    SDL_Texture * sdlTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);

    while (true) {
        game.emulateCycle();

        SDL_Event e;

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
                } else {
                    pixels[i] = 0xFF000000;
                }
            }

            SDL_UpdateTexture(sdlTexture, NULL, pixels, 64 * sizeof(Uint32));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }

        std::this_thread::sleep_for(std::chrono::microseconds(1200));
    }

    return 0;
}

