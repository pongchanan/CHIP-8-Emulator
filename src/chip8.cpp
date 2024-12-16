#include <cstdint>
#include <fstream>
#include <chrono>
#include <random>

/*
mimic the Chip8 hardware

CHIP-8 Description
- 16 8-bit register (V0 - VF)
    - short term data storage
    - each register can hold value from 0x00 to 0xFF
    - VF is used as a flag for some instructions
- 4K Bytes of Memory
    - long term data storage
    - address space is from 0x000 to 0xFFF
        - 0x000 to 0x1FF: Chip 8 interpreter
        - 0x050 to 0x0A0: 16 build-in characters
        - 0x200 to 0xFFF: Program ROM and work RAM
- 16-bit index register (I)
    - used to point to locations in memory
    - can hold value from 0x000 to 0xFFF
        - we use 16-bit because 8-bit is not enough to address 4KB memory
- 16-bit program counter (PC)
    - used to store the currently executing address
    - can hold value from 0x000 to 0xFFF
        - we use 16-bit because 8-bit is not enough to address 4KB memory
- 8-bit stack pointer (SP)
    - used to point to the topmost level of the stack
    - can hold value from 0x00 to 0x0F
- 8-bit delay timer
    - used to do delay operations
    - decremented at a rate of 60Hz
- 8-bit sound timer
    - used to do sound operations
    - decremented at a rate of 60Hz
- 16 input keys
    - 0x0 to 0xF
    - used to take input from the user
- 64*32 pixel monochrome display
    - used to display graphics
    - each pixel can be on or off
*/

const unsigned int START_ADDRESS = 0x200;
const unsigned int FRONT_SIZE = 80;
const unsigned int FONT_START_ADDRESS = 0x50;
const unsigned int VIDEO_WIDTH = 64;
const unsigned int VIDEO_HEIGHT = 32;

uint8_t fontset[FRONT_SIZE] = {
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

/*
Example font set: F
11110000
10000000
11110000
10000000
10000000
if you see only number 1 it will look like F
*/

class Chip8 {
    public:
        uint8_t registers[16]{};
        uint8_t memory[4096]{};
        uint16_t index{};
        uint16_t pc{};
        uint16_t stack[16]{};
        uint8_t sp{};
        uint8_t delayTimer{};
        uint8_t soundTimer{};
        uint8_t keypad[16]{};
        uint32_t video[64 * 32]{};
        uint16_t opcode{};

        std::default_random_engine randGen;
        std::uniform_int_distribution<uint8_t> randByte;

        Chip8();
        void LoadROM(char const* filename); 
        void OP_00E0(); // Clear the display
        void OP_00EE(); // Return from a subroutine
        void OP_1nnn(); // Jump to location nnn
        void OP_2nnn(); // Call subroutine at nnn
        void OP_3xkk(); // Skip next instruction if Vx = kk
        void OP_4xkk(); // Skip next instruction if Vx != kk
        void OP_5xy0(); // Skip next instruction if Vx = Vy
        void OP_6xkk(); // Set Vx = kk
        void OP_7xkk(); // Set Vx = Vx + kk
        void OP_8xy0(); // Set Vx = Vy
        void OP_8xy1(); // Set Vx = Vx | Vy
        void OP_8xy2(); // Set Vx = Vx & Vy
        void OP_8xy3(); // Set Vx = Vx ^ Vy
        void OP_8xy4(); // Set Vx = Vx + Vy, set VF = carry
        void OP_8xy5(); // Set Vx = Vx - Vy, set VF = NOT borrow
        void OP_8xy6(); // Set Vx = Vx >> 1
        void OP_8xy7(); // Set Vx = Vy - Vx, set VF = NOT borrow
        void OP_8xyE(); // Set Vx = Vx << 1
        void OP_9xy0(); // Skip next instruction if Vx != Vy
        void OP_Annn(); // Set index = nnn
        void OP_Bnnn(); // Jump to location nnn + V0
        void OP_Cxkk(); // Set Vx = random byte AND kk
        void OP_Dxyn(); // Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
        void OP_Ex9E(); // Skip next instruction if key with the value of Vx is pressed
        void OP_ExA1(); // Skip next instruction if key with the value of Vx is not pressed
        void OP_Fx07(); // Set Vx = delay timer value
        void OP_Fx0A(); // Wait for a key press, store the value of the key in Vx
        void OP_Fx15(); // Set delay timer = Vx
        void OP_Fx18(); // Set sound timer = Vx
        void OP_Fx1E(); // Set index = index + Vx
        void OP_Fx29(); // Set index = location of sprite for digit Vx
        void OP_Fx33(); // Store BCD representation of Vx in memory locations I, I+1, and I+2
        void OP_Fx55(); // Store registers V0 through Vx in memory starting at location I
        void OP_Fx65(); // Read registers V0 through Vx from memory starting at location I

        typedef void (Chip8::*Chip8Func)();
        Chip8Func table[0xF + 1];
        Chip8Func table0[0xE + 1];
        Chip8Func table8[0xE + 1];
        Chip8Func tableE[0xE + 1];
        Chip8Func tableF[0x65 + 1];

        void Table0();
        void Table8();
        void TableE();
        void TableF();
        void OP_NULL();

        void Cycle();
};

void Chip8::LoadROM(char const* filename) {
    // Open the file as a stream of binary and move the file pointer to the end
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    /*
    - std::ios::binary is flag that tells the stream to open the file in binary mode rather than text mode
    - std::ios::ate is flag that tells the stream move pointer to the end of the file
    */

   if (file.is_open()) {
        // Get size of file and allocate a buffer to hold the contents
        std::streampos size = file.tellg();
        char* buffer = new char[size];
        /*
        - std::streampos is a type that represents position in filestream
        - file.tellg() returns the current position of the file pointer
        */

        // Go back to begining of file and fill buffer
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        file.close();
        /*
        - file.seekg(0, std::ios::beg) moves the file pointer to the beginning of the file
        - file.read(buffer, size) reads the entire file into the buffer
        */

        // Load the ROM contents into the Chip8's memory, starting at 0x200
        for (long i = 0; i < size; i++) {
            memory[START_ADDRESS + i] = buffer[i];
        }

        // Free the buffer
        delete[] buffer;
   }
}

Chip8::Chip8() : randGen(std::chrono::system_clock::now().time_since_epoch().count()) {
    /*
    Chip8::Chip8() : randGen(std::chrono::system_clock::now().time_since_epoch().count())
    - randGen(std::chrono::system_clock::now().time_since_epoch().count()) is used to seed the random number generator. it use for initialization of the random number generator
    */

    // initialize the program counter
    pc = START_ADDRESS;

    // Load fonts into memory
    for (unsigned int i = 0; i < FRONT_SIZE; i++) {
        memory[FONT_START_ADDRESS + i] = fontset[i];
    }

    // init RNG
    randByte = std::uniform_int_distribution<uint8_t>(0, 255U);
    /*
    use randGen to generate random number between 0 and 255 and store it in randByte
    */

    // Setup the function pointer table
    table[0x0] = &Chip8::Table0;
    table[0x1] = &Chip8::OP_1nnn;
    table[0x2] = &Chip8::OP_2nnn;
    table[0x3] = &Chip8::OP_3xkk;
    table[0x4] = &Chip8::OP_4xkk;
    table[0x5] = &Chip8::OP_5xy0;
    table[0x6] = &Chip8::OP_6xkk;
    table[0x7] = &Chip8::OP_7xkk;
    table[0x8] = &Chip8::Table8;
    table[0x9] = &Chip8::OP_9xy0;
    table[0xA] = &Chip8::OP_Annn;
    table[0xB] = &Chip8::OP_Bnnn;
    table[0xC] = &Chip8::OP_Cxkk;
    table[0xD] = &Chip8::OP_Dxyn;
    table[0xE] = &Chip8::TableE;
    table[0xF] = &Chip8::TableF;

    for (size_t i = 0; i <= 0xE; i++)
    {
        table0[i] = &Chip8::OP_NULL;
        table8[i] = &Chip8::OP_NULL;
        tableE[i] = &Chip8::OP_NULL;
    }

    table0[0x0] = &Chip8::OP_00E0;
    table0[0xE] = &Chip8::OP_00EE;

    table8[0x0] = &Chip8::OP_8xy0;
    table8[0x1] = &Chip8::OP_8xy1;
    table8[0x2] = &Chip8::OP_8xy2;
    table8[0x3] = &Chip8::OP_8xy3;
    table8[0x4] = &Chip8::OP_8xy4;
    table8[0x5] = &Chip8::OP_8xy5;
    table8[0x6] = &Chip8::OP_8xy6;
    table8[0x7] = &Chip8::OP_8xy7;
    table8[0xE] = &Chip8::OP_8xyE;

    tableE[0x1] = &Chip8::OP_ExA1;
    tableE[0xE] = &Chip8::OP_Ex9E;

    for (size_t i = 0; i <= 0x65; i++)
    {
        tableF[i] = &Chip8::OP_NULL;
    }

    tableF[0x07] = &Chip8::OP_Fx07;
    tableF[0x0A] = &Chip8::OP_Fx0A;
    tableF[0x15] = &Chip8::OP_Fx15;
    tableF[0x18] = &Chip8::OP_Fx18;
    tableF[0x1E] = &Chip8::OP_Fx1E;
    tableF[0x29] = &Chip8::OP_Fx29;
    tableF[0x33] = &Chip8::OP_Fx33;
    tableF[0x55] = &Chip8::OP_Fx55;
    tableF[0x65] = &Chip8::OP_Fx65;
}

void Chip8::OP_00E0() {
    // Clear the display 00R0: CLS
    memset(video, 0, sizeof(video));
    /*
    is clear display by set all of pixel in doesplay to be 0
    */
}

void Chip8::OP_00EE() {
    // Return from a subroutine 00EE: RET
    --sp;
    pc = stack[sp];
    /*
    - --sp is decrement the stack pointer
    - pc = stack[sp] is set the program counter to the address at the top of the stack
    */
}

void Chip8::OP_1nnn() {
    // Jump to location nnn 1nnn: JP addr
    uint16_t address = opcode & 0x0FFFu;

    pc = address;
    /*
    - address = opcode & 0x0FFFu is get the address from the opcode
        - opcode is 16-bit instruction
        - 0x0FFFu is 12-bit mask
        - 0x0FFFu is used to get the address from the opcode
            - 0x0 is used to mask the first 4 bits of the opcode
                - 0x0 is 0000 0000 which is type of instruction
                    - 0x0 is for 0nnn system call
                    - 0x1 is for 1nnn jump to location nnn
                    - 0x2 is for 2nnn call subroutine at nnn
                    - 0x3 is for 3xkk skip next instruction if Vx = kk
            - FFF is used to mask the last 12 bits of the opcode
                - it for provide the additional information
    - pc = address is set the program counter to the address
    */
}

void Chip8::OP_2nnn() {
    // Call subroutine at nnn 2nnn: CALL addr
    uint16_t address = opcode & 0x0FFFu;

    stack[sp] = pc;
    ++sp;
    pc = address;
    /*
    - address = opcode & 0x0FFFu is get the address from the opcode
        - opcode is 16-bit instruction
        - 0x0FFFu is 12-bit mask
        - 0x0FFFu is used to get the address from the opcode
            - 0x0 is used to mask the first 4 bits of the opcode
                - 0x0 is 0000 0000 which is type of instruction
                    - 0x0 is for 0nnn system call
                    - 0x1 is for 1nnn jump to location nnn
                    - 0x2 is for 2nnn call subroutine at nnn
                    - 0x3 is for 3xkk skip next instruction if Vx = kk
            - FFF is used to mask the last 12 bits of the opcode
                - it for provide the additional information
    - stack[sp] = pc is store the current program counter on the stack
    - ++sp is increment the stack pointer
    - pc = address is set the program counter to the address
    */
}

void Chip8::OP_3xkk() {
    // Skip next instruction if Vx = kk 3xkk: SE Vx, byte

    // Extract the register index (Vx) from the opcode
    // (opcode & 0x0F00u) isolates the bits corresponding to Vx
    // >> 8u shifts the bits to the right to get the register index
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    // Extract the byte (kk) from the opcode
    // (opcode & 0x00FFu) isolates the last 8 bits of the opcode
    uint8_t byte = opcode & 0x00FFu;

    // Compare the value in register Vx with the byte (kk)
    // If they are equal, skip the next instruction by incrementing the program counter (pc) by 2
    if (registers[Vx] == byte) {
        pc += 2;
    }

    /*
    - uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        - Extracts the register index from the opcode
        - (opcode & 0x0F00u) isolates the bits corresponding to Vx
        - >> 8u shifts the bits to the right to get the register index
    - uint8_t byte = opcode & 0x00FFu;
        - Extracts the byte (kk) from the opcode
        - (opcode & 0x00FFu) isolates the last 8 bits of the opcode
    - if (registers[Vx] == byte) { pc += 2; }
        - Compares the value in register Vx with the byte (kk)
        - If they are equal, increments the program counter (pc) by 2 to skip the next instruction
    */
}

void Chip8::OP_4xkk() {
    // Skip next instruction if Vx != kk 4xkk: SNE Vx, byte

    // Extract the register index (Vx) from the opcode
    // (opcode & 0x0F00u) isolates the bits corresponding to Vx
    // >> 8u shifts the bits to the right to get the register index
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    // Extract the byte (kk) from the opcode
    // (opcode & 0x00FFu) isolates the last 8 bits of the opcode
    uint8_t byte = opcode & 0x00FFu;

    // Compare the value in register Vx with the byte (kk)
    // If they are not equal, skip the next instruction by incrementing the program counter (pc) by 2
    if (registers[Vx] != byte) {
        pc += 2;
    }

    /*
    - uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        - Extracts the register index from the opcode
        - (opcode & 0x0F00u) isolates the bits corresponding to Vx
        - >> 8u shifts the bits to the right to get the register index
    - uint8_t byte = opcode & 0x00FFu;
        - Extracts the byte (kk) from the opcode
        - (opcode & 0x00FFu) isolates the last 8 bits of the opcode
    - if (registers[Vx] != byte) { pc += 2; }
        - Compares the value in register Vx with the byte (kk)
        - If they are not equal, increments the program counter (pc) by 2 to skip the next instruction
    */
}

void Chip8::OP_5xy0() {
    // Skip next instruction if Vx = Vy 5xy0: SE Vx, Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] == registers[Vy]) {
        pc += 2;
    }
    /*
    we use difference bitwise because Vx is in the 8-11 bit and Vy is in the 4-7 bit
    */
}

void Chip8::OP_6xkk() {
    // Set Vx = kk 6xkk: LD Vx, byte
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] = byte;
    /*
    - uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        - Extracts the register index from the opcode
        - (opcode & 0x0F00u) isolates the bits corresponding to Vx
        - >> 8u shifts the bits to the right to get the register index
    - uint8_t byte = opcode & 0x00FFu;
        - Extracts the byte (kk) from the opcode
        - (opcode & 0x00FFu) isolates the last 8 bits of the opcode
    */
}

void Chip8::OP_7xkk() {
    // Set Vx = Vx + kk 7xkk: ADD Vx, byte
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] += byte;
    /*
    - uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        - Extracts the register index from the opcode
        - (opcode & 0x0F00u) isolates the bits corresponding to Vx
        - >> 8u shifts the bits to the right to get the register index
    - uint8_t byte = opcode & 0x00FFu;
        - Extracts the byte (kk) from the opcode
        - (opcode & 0x00FFu) isolates the last 8 bits of the opcode
    */
}

void Chip8::OP_8xy0() {
    // set Vx = Vy 8xy0: LD Vx, Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] = registers[Vy];
}

void Chip8::OP_8xy1() {
    // Set Vx = Vx | Vy 8xy1: OR Vx, Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] |= registers[Vy];
    /*
    |= is bitwise OR operator and store the result in the left operand
    */
}

void Chip8::OP_8xy2() {
    // Set Vx = Vx & Vy 8xy2: AND Vx, Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] &= registers[Vy];
    /*
    &= is bitwise AND operator and store the result in the left operand
    */
}

void Chip8::OP_8xy3() {
    // Set Vx = Vx ^ Vy 8xy3: XOR Vx, Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] ^= registers[Vy];
    /*
    ^= is bitwise XOR operator and store the result in the left operand
    */
}

void Chip8::OP_8xy4() {
    // Set Vx = Vx + Vy, set VF = carry 8xy4: ADD Vx, Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    uint16_t sum = registers[Vx] + registers[Vy];

    if (sum > 255U) {
        registers[0xF] = 1;
    } else {
        registers[0xF] = 0;
    }

    registers[Vx] = sum & 0xFFu;
}

void Chip8::OP_8xy5() {
    // Set Vx = Vx - Vy, set VF = NOT borrow 8xy5: SUB Vx, Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] > registers[Vy]) {
        registers[0xF] = 1;
    } else {
        registers[0xF] = 0;
    }

    registers[Vx] -= registers[Vy];
}

void Chip8::OP_8xy6() {
    // Set Vx = Vx >> 1 8xy6: SHR Vx {, Vy}
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    // Save the least significant bit in VF before shifting
    registers[0xF] = (registers[Vx] & 0x1u);

    registers[Vx] >>= 1;
    /*
    we store LSB to use it later
    */
}

void Chip8::OP_8xy7() {
    // Set Vx = Vy - Vx, set VF = NOT borrow 8xy7: SUBN Vx, Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vy] > registers[Vx]) {
        registers[0xF] = 1;
    } else {
        registers[0xF] = 0;
    }

    registers[Vx] = registers[Vy] - registers[Vx];

    /*
    it like OP_8xy5 but swap order of Vx and Vy
    */
}

void Chip8::OP_8xyE() {
    // Set Vx = Vx << 1 8xyE: SHL Vx {, Vy}
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    // Save MSB in VF
    registers[0xF] = (registers[Vx] & 0x80u) >> 7u;

    registers[Vx] <<= 1;
}

void Chip8::OP_9xy0() {
    // Skip next instruction if Vx != Vy 9xy0: SNE Vx, Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] != registers[Vy]) {
        pc += 2;
    }
}

void Chip8::OP_Annn() {
    // Set index = nnn Annn: LD I, addr
    uint8_t address = opcode & 0x0FFFu;

    index = address;
}

void Chip8::OP_Bnnn() {
    // Jump to location nnn + V0 Bnnn: JP V0, addr
    uint16_t address = opcode & 0x0FFFu;

    pc = registers[0] + address;
}

void Chip8::OP_Cxkk() {
    // Set Vx = random byte AND kk Cxkk: RND Vx, byte
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] = randByte(randGen) & byte;
}

void Chip8::OP_Dxyn() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    uint8_t height = opcode & 0x000Fu;

    // Wrap if going beyond screen boundaries
    uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
    uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

    // Set collusion flag to 0
    registers[0xF] = 0;

    for (unsigned int row = 0; row < height; ++row) {
        uint8_t spriteByte = memory[index + row];

        for (unsigned int col = 0; col < 8; ++col) {
            uint8_t spritePixel = spriteByte & (0x80u >> col);
            uint32_t *screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];

            // Sprite pixel is on
            if (spritePixel) {
                // Screen pixel also on - collision
                if (*screenPixel == 0xFFFFFFFF) {
                    registers[0xF] = 1;
                }

                // XOR with the sprite pixel
                *screenPixel ^= 0xFFFFFFFF;
            }
        }
    }

    /*
    - each sprite have width exactly 8 pixel
    - if sprite collide with the screen pixel, set the collision flag to 1
    */
}

void Chip8::OP_Ex9E() {
    // Skip next instruction if key with the value of Vx is pressed Ex9E: SKP Vx
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t key = registers[Vx];

    if (keypad[key]) {
        pc += 2;
    }
}

void Chip8::OP_ExA1() {
    // Skip next instruction if key with the value of Vx is not pressed ExA1: SKNP Vx
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t key = registers[Vx];

    if (!keypad[key]) {
        pc += 2;
    }
}

void Chip8::OP_Fx07() {
    // Set Vx = delay timer value Fx07: LD Vx, DT
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[Vx] = delayTimer;
}

void Chip8::OP_Fx0A() {
    // Wait for a key press, store the value of the key in Vx Fx0A: LD Vx, K
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    bool keyPress = false;

    for (int i = 0; i < 16; ++i) {
        if (keypad[i]) {
            registers[Vx] = i;
            keyPress = true;
        }
    }

    // If no key is pressed, return and try again
    if (!keyPress) {
        pc -= 2;
    }
}

void Chip8::OP_Fx15() {
    // Set delay timer = Vx Fx15: LD DT, Vx
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    delayTimer = registers[Vx];
}

void Chip8::OP_Fx18() {
    // Set sound timer = Vx Fx18: LD ST, Vx
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    soundTimer = registers[Vx];
}

void Chip8::OP_Fx1E() {
    // Set index = index + Vx Fx1E: ADD I, Vx
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    index += registers[Vx];
}

void Chip8::OP_Fx29() {
    // Set index = location of sprite for digit Vx Fx29: LD F, Vx
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t digit = registers[Vx];

    index = FONT_START_ADDRESS + (5 * digit);
}

void Chip8::OP_Fx33() {
    // Store BCD representation of Vx in memory locations I, I+1, and I+2 Fx33: LD B, Vx
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t value = registers[Vx];

    // Ones place
    memory[index + 2] = value % 10;
    value /= 10;

    // Tens place
    memory[index + 1] = value % 10;
    value /= 10;

    // Hundreds place
    memory[index] = value % 10;
}

void Chip8::OP_Fx55() {
    // Store registers V0 through Vx in memory starting at location I Fx55: LD [I], Vx
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    for (uint8_t i = 0; i <= Vx; ++i) {
        memory[index + i] = registers[i];
    }
}

void Chip8::OP_Fx65() {
    // Read registers V0 through Vx from memory starting at location I Fx65: LD Vx, [I]
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    for (uint8_t i = 0; i <= Vx; ++i) {
        registers[i] = memory[index + i];
    }
}

void Chip8::Table0()
{
	((*this).*(table0[opcode & 0x000Fu]))();
}

void Chip8::Table8()
{
	((*this).*(table8[opcode & 0x000Fu]))();
}

void Chip8::TableE()
{
	((*this).*(tableE[opcode & 0x000Fu]))();
}

void Chip8::TableF()
{
	((*this).*(tableF[opcode & 0x00FFu]))();
}

void Chip8::OP_NULL()
{}

void Chip8::Cycle() {
    // fetch
    opcode = (memory[pc] << 8u) | memory[pc + 1];

    // increment program counter
    pc += 2;

    // decode and execute
    ((*this).*(table[(opcode & 0xF000u) >> 12u]))();

    // update timers
    if (delayTimer > 0) {
        --delayTimer;
    }

    if (soundTimer > 0) {
        if (soundTimer == 1) {
            // Beep
            // std::cout << "BEEP!" << std::endl;
        }
        --soundTimer;
    }
}