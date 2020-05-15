#pragma once

#include <array>

#define _LDA(addr)      0, addr,
#define _STA(addr)      1, addr,
#define _ADD(addr)      2, addr,
#define _PRINT()        3, 0,
#define _HALT()         4, 0,
#define _EQ(addr)       5, addr,
#define _JMP(addr)      6, addr,
#define _JZ(addr)       7, addr,

constexpr auto MEM_SIZE = 256;

const std::array<unsigned char, MEM_SIZE> initMem = {
    // Code: Simple counter from 0 to 255
    _LDA(14) // Load counter
    _ADD(15) // Increment
    _PRINT() // Print counter
    _STA(14) // Store counter
    _EQ(16)  // Check counter == 256
    _JZ(0)   // Loop
    _HALT()

    // Data
    0,      // #14
    1,      // #15
    255     // #16
};
