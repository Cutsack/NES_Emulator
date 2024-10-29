// main.cpp
#include <SDL.h>
#include <iostream>
#include "CPU.h"
#include "Memory.h"
#include "PPU.h"
#include "Controller.h"
#include "Cartridge.h"

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("NES Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256 * 2, 240 * 2, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create renderer and texture
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);

    // Load cartridge
    Cartridge cartridge("D:\\ROMS\\Mario\\color_test.nes");

    if (!cartridge.Load()) {
        std::cout << "Failed to load ROM" << std::endl;
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize components
    PPU ppu(&cartridge);
    Memory memory(&cartridge);
    CPU cpu(&memory, &ppu);
    Controller controller1;

    memory.ConnectPPU(&ppu);
    memory.ConnectController(&controller1);

    cpu.Reset();
    ppu.Reset();

    // Emulation loop
    bool running = true;
    SDL_Event event;
    uint32_t lastTime = SDL_GetTicks();

    while (running && cpu.running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                bool pressed = (event.type == SDL_KEYDOWN);
                switch (event.key.keysym.sym) {
                case SDLK_z:
                    controller1.SetButtonState(0, pressed); // A Button
                    break;
                case SDLK_x:
                    controller1.SetButtonState(1, pressed); // B Button
                    break;
                case SDLK_RSHIFT:
                case SDLK_c:
                    controller1.SetButtonState(2, pressed); // Select Button
                    break;
                case SDLK_RETURN:
                    controller1.SetButtonState(3, pressed); // Start Button
                    break;
                case SDLK_UP:
                    controller1.SetButtonState(4, pressed); // Up
                    break;
                case SDLK_DOWN:
                    controller1.SetButtonState(5, pressed); // Down
                    break;
                case SDLK_LEFT:
                    controller1.SetButtonState(6, pressed); // Left
                    break;
                case SDLK_RIGHT:
                    controller1.SetButtonState(7, pressed); // Right
                    break;
                }
            }
        }

        // Emulate CPU cycles
        if (cpu.cycles == 0) {
            cpu.ExecuteInstruction();
        }
        cpu.cycles--;

        // PPU cycles (three PPU cycles per CPU cycle)
        for (int i = 0; i < 3; ++i) {
            ppu.Clock();

            // Handle NMI
            if (ppu.nmi) {
                ppu.nmi = false;
                cpu.NMI();
            }
        }

        // Render frame if ready
        if (ppu.FrameReady()) {
            SDL_UpdateTexture(texture, NULL, ppu.GetFrameBuffer(), 256 * sizeof(uint32_t));

            // Scale the output to fit the window
            SDL_Rect srcRect = { 0, 0, 256, 240 };
            SDL_Rect dstRect = { 0, 0, 256 * 2, 240 * 2 }; // Scale by 2x

            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, &srcRect, &dstRect);
            SDL_RenderPresent(renderer);
        }

        // Frame limiting (optional)
        uint32_t currentTime = SDL_GetTicks();
        if (currentTime - lastTime < 16) {
            SDL_Delay(16 - (currentTime - lastTime));
        }
        lastTime = currentTime;
    }

    // Clean up
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
