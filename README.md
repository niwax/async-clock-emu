# async-clock-emu
This project implements a very basic cycle-accurate emulator based on asynchronous generators as inspired by a [discussion between Matt Godbolt and Jason Turner](https://www.youtube.com/watch?v=uEyJ0z2iDhg).

The basic structure is a number of hardware components implemented as generators that emit cycle-specific side effect using yield. These side effects are gathered and applied in a central loop on a fixed clock cycle.

Note that the C++ implementation depends on certain experimental C++20 features. For MSVC, these have to be activated using the /await option in VS2017 or higher.
