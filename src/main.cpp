#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "chip8.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    chip8 emulator;

    try {
        emulator.loadROM("Pong.ch8");
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
            "CHIP-8 Emulator",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            64 * 10,
            32 * 10,
            SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
            window, -1, SDL_RENDERER_ACCELERATED
    );

    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool quit = false;
    SDL_Event e;

    const int CYCLES_PER_FRAME = 10; // ~600 Hz CPU
    const int FRAME_DELAY_MS = 16;   // ~60 FPS

    while (!quit) {

        /* -------------------- EVENTS -------------------- */
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                quit = true;
        }

        /* -------------------- INPUT -------------------- */
        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        emulator.setKey(0x1, keys[SDL_SCANCODE_1]);
        emulator.setKey(0x2, keys[SDL_SCANCODE_2]);
        emulator.setKey(0x3, keys[SDL_SCANCODE_3]);
        emulator.setKey(0xC, keys[SDL_SCANCODE_4]);

        emulator.setKey(0x4, keys[SDL_SCANCODE_Q]);
        emulator.setKey(0x5, keys[SDL_SCANCODE_W]);
        emulator.setKey(0x6, keys[SDL_SCANCODE_E]);
        emulator.setKey(0xD, keys[SDL_SCANCODE_R]);

        emulator.setKey(0x7, keys[SDL_SCANCODE_A]);
        emulator.setKey(0x8, keys[SDL_SCANCODE_S]);
        emulator.setKey(0x9, keys[SDL_SCANCODE_D]);
        emulator.setKey(0xE, keys[SDL_SCANCODE_F]);

        emulator.setKey(0xA, keys[SDL_SCANCODE_Z]);
        emulator.setKey(0x0, keys[SDL_SCANCODE_X]);
        emulator.setKey(0xB, keys[SDL_SCANCODE_C]);
        emulator.setKey(0xF, keys[SDL_SCANCODE_V]);

        /* -------------------- CPU -------------------- */
        for (int i = 0; i < CYCLES_PER_FRAME; ++i) {
            emulator.emulateCycle();
        }

        /* -------------------- RENDER -------------------- */
        if (emulator.shouldDraw()) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            uint8_t* gfx = emulator.getDisplay();

            for (int y = 0; y < 32; ++y) {
                for (int x = 0; x < 64; ++x) {
                    if (gfx[x + y * 64]) {
                        SDL_Rect r{ x * 10, y * 10, 10, 10 };
                        SDL_RenderFillRect(renderer, &r);
                    }
                }
            }

            SDL_RenderPresent(renderer);
            emulator.resetDrawFlag();
        }

        std::this_thread::sleep_for(
                std::chrono::milliseconds(FRAME_DELAY_MS)
        );
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
