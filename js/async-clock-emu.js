// ----------- State and effects
let end = false;

let bus = { addr: 0, writeBit: 0, data: 0 };
let registers = { a: 0, pc: 0 }

function readBus(register) {
    return bus[register];
}

function writeBus(register, value) {
    return () => {
        bus[register] = value;
    };
}

function readRegister(register) {
    return registers[register];
}

function writeRegister(register, value) {
    return () => {
        registers[register] = value;
    };
}

function Run () {
    let components = [CPU(), Memory()];

    while(!end) {
        // Gather effects for clock cycle
        let effects = [];
        components.map((component) => {
            effects.push(...component.next().value);
        });

        // Apply effects
        effects.map((effect) => { effect(); });
    }

    console.log("HALT");
}

// ----------- CPU implementation
let CPUOps = [
    function* (param) {           // 0: LOAD A
        yield [writeBus('addr', param)];    // Cycle 1: Write to bus

        yield [];                           // NOP on latency

        let data = readBus('data');
        yield [writeRegister('a', data)];   // Cycle 3: Write to register
    },
    function* (param) {           // 1: STORE A
        let data = readRegister('a');

        yield [                             // Cycle 1: Write to bus
            writeBus('writeBit', true),
            writeBus('addr', param),
            writeBus('data', data)
        ];

        yield [];                           // Cycle 2: NOP on latency
    },
    function* (param) {           // 2: ADD
        yield [writeBus('addr', param)];    // Cycle 1: Write to bus

        yield [];                           // NOP on latency

        let data = readBus('data') + readRegister('a');
        yield [writeRegister('a', data)];   // Cycle 3: Write to register
    },
    function* () {           // 3: PRINT A
        console.log("PRINT: " + readRegister('a'));
        yield [];
    },
    function* () {           // 4: HALT
        end = true;
        yield [];
    }
];

function* CPU() {
    while(true) {
        let pc = readRegister('pc');    // Cycle 1: Turn off write flag, push PC to address
        yield [
            writeBus('writeBit', false),
            writeBus('addr', pc)
        ];

        pc += 1;                        // Cycle 2: Push PC+1 to bus, wait for PC in bus
        yield [writeBus('addr', pc)];

        let opcode = readBus('data');
        yield [];                       // Cycle 3: Wait for PC+1 on bus

        let param = readBus('data');    // Cycle 4 - n: Run operation
        let op = CPUOps[opcode](param);
        for (let tick of op) {
            yield tick;
        }

        pc += 1;                        // Cycle n + 1: increment and store PC, push to address
        yield [writeRegister('pc', pc)];
    }
}

// ----------- Program to add 42 + 73
let program = [
    0, 8, // LOAD A (#8)
    2, 9, // ADD (#9)
    3, 0, // PRINT A
    4, 0, // HALT
    42, 73
];

// ----------- Memory implementation
function* Memory() {
    let mem = program;

    while(true) {
        let addr = readBus('addr');
        let write = readBus('writeBit');

        if (write) {
            let data = readBus('data');
            mem[addr] = data;
            yield [];                           // One cycle write latency
        }

        yield [writeBus('data', mem[addr])];    // Write byte to bus
    }
}

Run();
