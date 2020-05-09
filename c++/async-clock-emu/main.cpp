#include <iostream>
#include <vector>
#include <list>
#include <experimental/generator>
#include <functional>

// ----------- Utility functions

typedef std::function<void(void)> effect;
typedef std::experimental::generator<std::list<effect>> clockable;
typedef std::function<clockable(char)> clockable_func;
#define clockable_call(func) for (auto val : func()) { co_yield val; }
#define clockable_call(func, param) for (auto val : func(param)) { co_yield val; }
#define clock(func, effects) func++; auto eff = *func; effects.splice(effects.end(), eff);

// ----------- System definition

bool end = false;

char bus[] = { 0, 0, 0 };
enum Bus { Addr = 0, Data = 1, WriteBit = 2 };

char registers[] = { 0, 0 };
enum Registers { A = 0, PC = 1 };

char ReadBus(Bus reg) {
    return bus[reg];
}

effect WriteBus(Bus reg, char val) {
    return [reg, val]() { bus[reg] = val; };
}

char ReadRegister(Registers reg) {
    return registers[reg];
}

effect WriteRegister(Registers reg, char val) {
    return [reg, val]() { registers[reg] = val; };
}

// ----------- CPU implementation

clockable LDA(char param) {
    co_yield{ WriteBus(Bus::Addr, param) };         // Cycle 1: Write to bus
    co_yield{};                                     // NOP on latency

    auto data = ReadBus(Bus::Data);
    co_yield{ WriteRegister(Registers::A, data) }; // Cycle 3: Write to register
}

clockable STA(char param) {
    auto data = ReadRegister(Registers::A);
    co_yield{                                       // Cycle 1: Write to bus
        WriteBus(Bus::WriteBit, 1),
        WriteBus(Bus::Addr, param),
        WriteBus(Bus::Data, data)
    };
    co_yield{};                                     // Cycle 2: NOP on latency

    co_yield{ WriteRegister(Registers::A, param) };
}

clockable Add(char param) {
    co_yield{ WriteBus(Bus::Addr, param) };         // Cycle 1: Write to bus
    co_yield{};                                     // NOP on latency

    auto data = ReadBus(Bus::Data) + ReadRegister(Registers::A);
    co_yield{ WriteRegister(Registers::A, data) };  // Cycle 3: Write to register
}

clockable Print(char param) {
    std::cout << "PRINT: " << (int)ReadRegister(Registers::A) << std::endl;
    co_yield{};
}

clockable Halt(char param) {
    end = true;
    co_yield{};
}

std::vector<clockable_func> CPUOps = { LDA, STA, Add, Print, Halt };

clockable CPU() {
    co_yield{};
    while (true) {
        auto pc = ReadRegister(Registers::PC);

        co_yield{
            WriteBus(Bus::WriteBit, 0),
            WriteBus(Bus::Addr, pc)
        };

        pc++;
        co_yield{ WriteBus(Bus::Addr, pc) };

        auto opcode = ReadBus(Bus::Data);
        co_yield{};

        auto param = ReadBus(Bus::Data);
        clockable_call(CPUOps[opcode], param);

        pc++;
        co_yield{ WriteRegister(Registers::PC, pc) };
    }
}

// ----------- Memory implementation

clockable Memory() {
    // Program to add 42 + 73
    char mem[] = {
        0, 8, // LOAD A (#8)
        2, 9, // ADD (#9)
        3, 0, // PRINT A
        4, 0, // HALT
        42, 73
    };

    co_yield{};
    while (true) {
        auto addr = ReadBus(Bus::Addr);
        auto write = ReadBus(Bus::WriteBit);

        if (write) {
            auto data = ReadBus(Bus::Data);
            mem[addr] = data;
            co_yield{};
        }

        co_yield{ WriteBus(Bus::Data, mem[addr]) };
    }
}

// ----------- Memory implementation

int main()
{
    auto memory = Memory();
    auto cpu = CPU();
    auto components = { memory.begin(), cpu.begin() };

    while (!end) {
        std::list<effect> effects;

        for (auto component : components) {
            clock(component, effects);
        }

        for (auto effect : effects) {
            effect();
        }
    }
}
