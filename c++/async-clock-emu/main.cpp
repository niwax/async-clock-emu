#include <iostream>
#include <array>
#include <list>
#include <experimental/generator>
#include <functional>
#include <chrono>
#include "code.h"

// ----------- Utility functions

struct effect {
    bool wBus[3];
    bool wRegisters[2];
    unsigned char vBus[3];
    unsigned char vRegisters[2];

    void Apply();
};

effect operator+(effect const& lhs, effect const& rhs) {
    effect total = lhs;

    for (int i = 0; i < 3; i++) {
        if (rhs.wBus[i]) {
            total.wBus[i] = true;
            total.vBus[i] = rhs.vBus[i];
        }
    }

    for (int i = 0; i < 2; i++) {
        if (rhs.wRegisters[i]) {
            total.wRegisters[i] = true;
            total.vRegisters[i] = rhs.vRegisters[i];
        }
    }

    return total;
}

typedef std::experimental::generator<effect> clockable;
typedef std::function<clockable(unsigned char)> clockable_func;
#define clockable_call(func, param) for (auto val : func(param)) { co_yield val; }
#define clock(func, effects) func++; auto eff = *func; effects = effects + eff;

#define CONSOLE_OUTPUT // Allow console output in CPU Ops

// ----------- System definition

bool end = false;

unsigned char bus[] = { 0, 0, 0 };
enum Bus { Addr = 0, Data = 1, WriteBit = 2 };

unsigned char registers[] = { 0, 0 };
enum Registers { A = 0, PC = 1 };

void effect::Apply() {
    for (int i = 0; i < 3; i++) {
        if (this->wBus[i]) {
            bus[i] = this->vBus[i];
        }
    }
    for (int i = 0; i < 2; i++) {
        if (this->wRegisters[i]) {
            registers[i] = this->vRegisters[i];
        }
    }
};

unsigned char ReadBus(Bus reg) {
    return bus[reg];
}

effect WriteBus(Bus reg, unsigned char val) {
    effect e = { 0 };

    e.wBus[reg] = true;
    e.vBus[reg] = val;

    return e;
}

unsigned char ReadRegister(Registers reg) {
    return registers[reg];
}

effect WriteRegister(Registers reg, unsigned char val) {
    effect e = { 0 };

    e.wRegisters[reg] = true;
    e.vRegisters[reg] = val;

    return e;
}

effect EmptyEffect() {
    effect e = { 0 };
    return e;
}

// ----------- CPU implementation

clockable LDA(unsigned char param) {
    co_yield WriteBus(Bus::Addr, param);            // Cycle 1: Write to bus
    co_yield EmptyEffect();                         // NOP on memory read latency

    auto data = ReadBus(Bus::Data);
    co_yield WriteRegister(Registers::A, data);     // Cycle 3: Write to register
}

clockable STA(unsigned char param) {
    auto data = ReadRegister(Registers::A);
    co_yield WriteBus(Bus::WriteBit, 1) +           // Cycle 1: Write to bus
             WriteBus(Bus::Addr, param) +
             WriteBus(Bus::Data, data);
}

clockable Add(unsigned char param) {
    co_yield WriteBus(Bus::Addr, param);            // Cycle 1: Write to bus
    co_yield EmptyEffect();                         // NOP on latency

    auto data = ReadBus(Bus::Data) + ReadRegister(Registers::A);
    co_yield WriteRegister(Registers::A, data);     // Cycle 3: Write to register
}

clockable Print(unsigned char param) {
#ifdef CONSOLE_OUTPUT
    std::cout << "PRINT: " << (int)ReadRegister(Registers::A) << std::endl;
#endif

    co_yield EmptyEffect();
}

clockable Halt(unsigned char param) {
    end = true;
    co_yield EmptyEffect();                         // Generator has to have at least one yield
}

clockable EQ(unsigned char param) {
    co_yield WriteBus(Bus::Addr, param);            // Cycle 1: Write to bus
    co_yield EmptyEffect();                         // NOP on latency

    auto a = ReadRegister(Registers::A);
    auto data = ReadBus(Bus::Data);
    unsigned char cmp = (a == data);

    co_yield WriteRegister(Registers::A, cmp);
}

clockable JMP(unsigned char param) {
    co_yield WriteRegister(Registers::PC, param);
}

clockable JZ(unsigned char param) {
    auto pc = ReadRegister(Registers::PC);
    auto a = ReadRegister(Registers::A);

    if (a == 0) {
        co_yield WriteRegister(Registers::PC, param);
    }
    else {
        pc += 2;
        co_yield WriteRegister(Registers::PC, pc);
    }
}

const std::array<const clockable_func, 8> CPUOps = { LDA, STA, Add, Print, Halt, EQ, JMP, JZ }; // Simple opcode lookup table

clockable CPU() {
    co_yield EmptyEffect();                         // This clock will be read on initialization and any effects ignored
    while (true) {
        auto pc = ReadRegister(Registers::PC);

        co_yield WriteBus(Bus::WriteBit, 0) +       // Start PC opcode read
                 WriteBus(Bus::Addr, pc);

        pc++;                                       // Start PC+1 opcode read, mask PC read latency
        co_yield WriteBus(Bus::Addr, pc);

        auto opcode = ReadBus(Bus::Data);           // Read PC opcode from bus
        co_yield EmptyEffect();

        auto param = ReadBus(Bus::Data);            // Read PC+1 opcode from bus
        clockable_call(CPUOps[opcode], param);      // Execute operation

        if (opcode < 6) {                           // Don't modify PC on jumps
            pc++;
            co_yield WriteRegister(Registers::PC, pc);
        }
    }
}

// ----------- Memory implementation

clockable Memory() {
    // Initialize from code.h
    auto mem = initMem;

    co_yield EmptyEffect();
    while (true) {
        auto addr = ReadBus(Bus::Addr);
        auto write = ReadBus(Bus::WriteBit);

        if (write) {
            auto data = ReadBus(Bus::Data);
            mem[addr] = data;
        }

        co_yield WriteBus(Bus::Data, mem[addr]);
    }
}

// ----------- Execution loop

int main()
{
    using namespace std::chrono;

    auto memory = Memory();
    auto cpu = CPU();
    const auto components = { memory.begin(), cpu.begin() };

    int clocks = 0;
    auto start = high_resolution_clock::now();

    while (!end) {
        effect effects = EmptyEffect();

        for (auto component : components) {
            clock(component, effects);
        }

        effects.Apply();

        clocks++;

        if (clocks == 1000000) {
            break;
        }
    }

    auto time = duration_cast<microseconds>(high_resolution_clock::now() - start);
    std::cout << clocks << " cycles in " << ((float)time.count() / 1000) << " ms (" << (int)((float)clocks / (float)time.count() * 1000000.0f) << " ops/s)";
}
