//
// Created by patel on 2025-08-18.
//

#include "chip8.hpp"
#include <cstring>   // for std::memset, std::memcpy
#include <iostream>  // optional, for debugging
#include <fstream>
#include <stdexcept>
#include <vector>


// CHIP-8 fontset (each character is 5 bytes, 16 characters = 80 bytes)
static const uint8_t chip8_fontset[80] = {
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

// Constructor
chip8::chip8() {
    // Reset program counter, opcode, and index register
    pc = 0x200;   // Programs start at memory location 0x200
    opcode = 0;
    I = 0;
    sp = 0;
    drawFlag = false;


    // Clear display, stack, registers, memory
    std::memset(gfx, 0, sizeof(gfx));
    std::memset(stack, 0, sizeof(stack));
    std::memset(V, 0, sizeof(V));
    std::memset(memory, 0, sizeof(memory));
    std::memset(key, 0, sizeof(key));

    // Reset timers
    delay_timer = 0;
    sound_timer = 0;

    // Load fontset into memory (at 0x50 by convention)
    for (int i = 0; i < 80; ++i) {
        memory[0x50 + i] = chip8_fontset[i];
    }

    std::cout << "CHIP-8 initialized. PC set to 0x200, fontset loaded.\n";
}

void chip8::loadROM(const std::string &filename) {
    std::ifstream rom(filename, std::ios::binary | std::ios::ate);
    if (!rom.is_open()) {
        throw std::runtime_error("Failed to open ROM: " + filename);
    }

    std::streampos size = rom.tellg();
    if (size > (4096 - 0x200)) {
        throw std::runtime_error("ROM too large to fit in memory");
    }

    std::vector<char> buffer(size);
    rom.seekg(0, std::ios::beg);
    rom.read(buffer.data(), size);
    rom.close();

    for (size_t i = 0; i < buffer.size(); ++i) {
        memory[0x200 + i] = static_cast<uint8_t>(buffer[i]);
    }
}

void chip8::emulateCycle() {
    // Fetch 2-byte opcode
    opcode = memory[pc] << 8 | memory[pc + 1];

    // Decode & execute
    switch (opcode & 0xF000) { // bitwise and for the first 4 bits of hex using the mask
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0x00E0: // CLS (clear the screen and move to next instruction)
                    std::memset(gfx, 0, sizeof(gfx));
                    pc += 2;
                    break;

                case 0x00EE: // RET (pop return address and continue)
                    --sp;
                    pc = stack[sp];
                 //   pc += 2; not needed?
                    break;


                default:
                    std::cerr << "Unknown opcode [0x0000]: "
                              << std::hex << opcode << "\n";
                    break;
            }
            break;

        case 0x1000: //1NNN Jump
            pc = opcode & 0x0FFF;
            break;

            //save return address and jump into subroutine
        case 0x2000: // CALL addr
            stack[sp] = pc + 2; // pc is current instruction, pc + 2 is next instruction (2 bytes)
            ++sp; //move stack pointer
            pc = opcode & 0x0FFF;
            break;

            //skip instruction if vx = nn
        case 0x3000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            if (V[x] == nn) pc += 4;
            else pc += 2;
            break;
        }

            //opposite of 3XNN, skip if not equal
        case 0x4000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            if (V[x] != nn) pc += 4;
            else pc += 2;
            break;
        }

        case 0x5000: {  // 5XY0
            if ((opcode & 0x000F) != 0) {
                std::cerr << "Invalid 5XY? opcode\n";
                break;
            }

            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            if (V[x] == V[y])
                pc += 4;
            else
                pc += 2;

            break;
        }
        case 0x9000: {  // 9XY0 (mirror of 5XY0, but it skips for not equal)
            if ((opcode & 0x000F) != 0) {
                std::cerr << "Invalid 5XY? opcode\n";
                break;
            }

            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            if (V[x] != V[y])
                pc += 4;
            else
                pc += 2;

            break;
        }

        case 0x6000: { //set vx to value of nn
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;

            V[x] = nn;
            pc += 2;
            break;
        }

            //CARRY NOT IMPLEMENTED YET
        case 0x7000: { //add value of nn to vx
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;

            V[x] += nn;
            pc += 2;
            break;
        }

        case 0x8000: { //ALU
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            switch (opcode & 0x000F) {
                case 0x0: { //VX is set to the value of VY
                    V[x] = V[y];
                    break;
                }

                case 0x1: { //bitwise OR and set it to x
                    V[x] = V[x] | V[y];
                    break;
                }

                case 0x2: { //bitwise AND
                    V[x] = V[x] & V[y];
                    break;
                }

                case 0x3: { //bitwise XOR
                    V[x] = V[x] ^ V[y];
                    break;

                }
                case 0x4: { //add
                    uint16_t sum = V[x] + V[y];

                    V[0xF] = (sum > 0xFF) ? 1 : 0; // carry flag
                    V[x] = sum & 0xFF;            // keep lower 8 bits

                    break;
                }
                case 0x5: { //subtract x-y
                    // Set VF = 1 if VX >= VY (no borrow), else 0
                    V[0xF] = (V[x] >= V[y]) ? 1 : 0;

                    V[x] = V[x] - V[y]; // wraps automatically if negative
                    break;
                }
                case 0x7: { //subtract y-x
                    // Set VF = 1 if VX >= VY (no borrow), else 0
                    V[0xF] = (V[y] >= V[x]) ? 1 : 0;

                    V[x] = V[y] - V[x]; // wraps automatically if negative
                    break;
                }

                case 0x6: { // 8XY6 - Shift right VX

                    V[0xF] = V[x] & 0x1;   // save least-significant bit
                    V[x] >>= 1; //shift

                    pc += 2;
                    break;
                }
                case 0xE: { // 8XYE - Shift left VX

                    V[0xF] = (V[x] & 0x80) >> 7; // MSB
                    V[x] <<= 1; //shift left

                    pc += 2;
                    break;
                }

                default:
                    break;
            }
            pc += 2;
            break;
        }
        case 0xA000: { //Set index register
            I = opcode & 0x0FFF;
            pc += 2;
            break;
        }
        case 0xB000: { // BNNN - Jump with offset (original CHIP-8)
            uint16_t addr = opcode & 0x0FFF;
            pc = addr + V[0];
            break;
        }
        case 0xC000: { //CNNN - Random
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            V[x] = (rand() & 0xFF) & nn;
            pc += 2;
            break;
        }
        case 0xD000: { // DXYN — Draw sprite at (VX, VY) with height N
            /*  Opcode format: D X Y N

                X = index of register VX
                Y = index of register VY
                N = number of sprite rows (height)

                Each sprite row is 8 pixels wide (1 byte).
                Sprite data starts at memory[I]. */


            // Extract X coordinate from VX
            uint8_t x = V[(opcode & 0x0F00) >> 8];

            // Extract Y coordinate from VY
            uint8_t y = V[(opcode & 0x00F0) >> 4];

            // Extract sprite height (number of rows)
            uint8_t height = opcode & 0x000F;

            // VF is the collision flag — reset it before drawing
            V[0xF] = 0;

            /*
                Loop over each row of the sprite.
                Each row is 1 byte = 8 horizontal pixels.
            */
            for (int row = 0; row < height; row++) {
                // Read one byte of sprite data from memory
                uint8_t spriteByte = memory[I + row];
                /*
                    Loop over each bit (pixel) in the sprite byte.
                    Bit 7 is the leftmost pixel.
                    Bit 0 is the rightmost pixel.
                */
                for (int col = 0; col < 8; col++) {
                    /*
                        Check if the current bit is set.

                        0x80 = 10000000
                        Shift right by col to test each bit.
                    */
                    if (spriteByte & (0x80 >> col)) {

                        // Compute wrapped screen coordinates
                        int px = (x + col) % 64;
                        int py = (y + row) % 32;
                        // Convert 2D position into 1D framebuffer index
                        int index = px + (py * 64);
                        /*
                            Collision detection:
                            If a pixel is already ON and we are about
                            to toggle it OFF, set VF = 1.
                        */
                        if (gfx[index] == 1)
                            V[0xF] = 1;
                        /*
                            XOR drawing:
                            - 0 ^ 1 = 1  (pixel turns on)
                            - 1 ^ 1 = 0  (pixel turns off)
                        */
                        gfx[index] ^= 1;
                    }
                }
            }

            // Move program counter to the next instruction
            pc += 2;

            // Signal the renderer that the screen needs to be redrawn
            drawFlag = true;

            break;
        }
        case 0xE000: {
            uint8_t x = (opcode & 0x0F00) >> 8;

            switch (opcode & 0x00FF) {

                case 0x9E: // EX9E - Skip if key in VX is pressed
                    if (key[V[x]] != 0)
                        pc += 4;
                    else
                        pc += 2;
                    break;

                case 0xA1: // EXA1 - Skip if key in VX is NOT pressed
                    if (key[V[x]] == 0)
                        pc += 4;
                    else
                        pc += 2;
                    break;

                default:
                    std::cerr << "Unknown opcode [0xE000]: "
                              << std::hex << opcode << "\n";
                    break;
            }
            break;
        }
        case 0xF000: {
            // X is the register index encoded in the opcode (FX??)
            uint8_t x = (opcode & 0x0F00) >> 8;

            // Look at the last byte to determine the exact FX instruction
            switch (opcode & 0x00FF) {

                // FX07 — Set VX = delay timer value
                case 0x07:
                    V[x] = delay_timer;   // Copy delay timer into VX
                    pc += 2;              // Move to next instruction
                    break;

                    // FX15 — Set delay timer = VX
                case 0x15:
                    delay_timer = V[x];   // Load delay timer from VX
                    pc += 2;
                    break;

                    // FX18 — Set sound timer = VX
                case 0x18:
                    sound_timer = V[x];   // Load sound timer from VX
                    pc += 2;
                    break;

                    // FX1E — Add VX to index register I
                case 0x1E:
                    I += V[x];            // Add VX to I

                    // Optional compatibility behavior:
                    // Set VF if I overflows past 0x0FFF
                    V[0xF] = (I > 0x0FFF) ? 1 : 0;

                    // Keep I within 12-bit address space
                    I &= 0x0FFF;

                    pc += 2;
                    break;

                    // FX0A — Wait for a key press, then store it in VX
                case 0x0A: {
                    bool keyPressed = false;

                    // Check all 16 keys
                    for (int i = 0; i < 16; i++) {
                        if (key[i]) {     // If key i is currently pressed
                            V[x] = i;     // Store key value in VX
                            keyPressed = true;
                            break;
                        }
                    }

                    // Only advance PC if a key was pressed
                    // Otherwise, this instruction repeats (blocks)
                    if (keyPressed)
                        pc += 2;

                    break;
                }

                    // FX29 — Set I to the font sprite address for digit in VX
                case 0x29:
                    // Each font character is 5 bytes
                    // Fontset starts at memory location 0x50
                    I = 0x50 + (V[x] & 0x0F) * 5;
                    pc += 2;
                    break;

                    // FX33 — Store BCD representation of VX at memory[I..I+2]
                case 0x33:
                    // Hundreds digit
                    memory[I] = V[x] / 100;

                    // Tens digit
                    memory[I + 1] = (V[x] / 10) % 10;

                    // Ones digit
                    memory[I + 2] = V[x] % 10;

                    pc += 2;
                    break;

                    // FX55 — Store registers V0 through VX in memory starting at I
                case 0x55:
                    for (int i = 0; i <= x; i++) {
                        memory[I + i] = V[i];
                    }
                    pc += 2;
                    break;

                    // FX65 — Load registers V0 through VX from memory starting at I
                case 0x65:
                    for (int i = 0; i <= x; i++) {
                        V[i] = memory[I + i];
                    }
                    pc += 2;
                    break;

                    // Unknown FX opcode
                default:
                    std::cerr << "Unknown FX opcode: "
                              << std::hex << opcode << "\n";
                    break;
            }

            break;
        }





            // TODO: Implement other opcodes

        default:
            std::cerr << "Unknown opcode: " << std::hex << opcode << "\n";
            break;
    }

// Update timers
    if (delay_timer > 0)
        --delay_timer;

    if (sound_timer > 0)
        --sound_timer;

}
