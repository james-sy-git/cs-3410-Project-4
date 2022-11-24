#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "linkedlist.h"
#include "hashtable.h"
#include "riscv.h"

/************** BEGIN HELPER FUNCTIONS PROVIDED FOR CONVENIENCE ***************/
const int R_TYPE = 0;
const int I_TYPE = 1;
const int MEM_TYPE = 2;
const int U_TYPE = 3;
const int UNKNOWN_TYPE = 4;

/**
 * Return the type of instruction for the given operation
 * Available options are R_TYPE, I_TYPE, MEM_TYPE, UNKNOWN_TYPE
 */
static int get_op_type(char *op)
{
    const char *r_type_op[] = {"add", "sub", "and", "or", "xor", "slt", "sll", "sra"};
    const char *i_type_op[] = {"addi", "andi", "ori", "xori", "slti"};
    const char *mem_type_op[] = {"lw", "lb", "sw", "sb"};
    const char *u_type_op[] = {"lui"};
    for (int i = 0; i < (int)(sizeof(r_type_op) / sizeof(char *)); i++)
    {
        if (strcmp(r_type_op[i], op) == 0)
        {
            return R_TYPE;
        }
    }
    for (int i = 0; i < (int)(sizeof(i_type_op) / sizeof(char *)); i++)
    {
        if (strcmp(i_type_op[i], op) == 0)
        {
            return I_TYPE;
        }
    }
    for (int i = 0; i < (int)(sizeof(mem_type_op) / sizeof(char *)); i++)
    {
        if (strcmp(mem_type_op[i], op) == 0)
        {
            return MEM_TYPE;
        }
    }
    for (int i = 0; i < (int)(sizeof(u_type_op) / sizeof(char *)); i++)
    {
        if (strcmp(u_type_op[i], op) == 0)
        {
            return U_TYPE;
        }
    }
    return UNKNOWN_TYPE;
}
/*************** END HELPER FUNCTIONS PROVIDED FOR CONVENIENCE ****************/

registers_t *registers;
hashtable_t *memory;


void init(registers_t *starting_registers)
{
    registers = starting_registers;
    memory = ht_init(213); // allocating memory for simulated memory
    for (int i = 0; i < 32; ++i) // initializing all registers to 0
    {
        registers->r[i] = 0;
    }
}

void end()
{
    ht_free(memory);
    free(registers);
}


int pull_register(char *input)
{
    // Returns the register value of an instruction, with whitespaces and the "x" character stripped away
    char *xpointer = strchr(input, 'x');
    return atoi(xpointer + 1);
}

int to_decimal(char *immediate, int len)
{
    // Takes in an immediate and converts it from hex to decimal if "0x" is detected, otherwise returns the decimal value stripped of whitespace
    const int sign_extend = 0b11111111111111111111000000000000; // used to sign-extend a negative 12-bit immediate
    char hexholder[3];
    strcpy(hexholder, "0x");
    if (strstr(immediate, hexholder) != NULL) // if the hexadecimal signifier is found:
    {
        int hex_comp = (int)strtol(immediate, NULL, 0); // converts hexadecimal number into a decimal number
        if ((hex_comp >> 11) > 0 && len == 12) // e.g., MSB is 1: negative immediate
        {
            return hex_comp | sign_extend;
        }
        else
        {
            return hex_comp;
        }
    }
    else // decimal immediate
    {
        int comp = atoi(immediate);
        if ((comp >> 11) > 0) // e.g., MSB is 1: negative immediate
        {
            return comp | sign_extend;
        }
        else // positive immediate
        {
            return comp;
        }
    }
}

void r(char *instruction, char *op) // R-TYPE helper: ADD, SUB, AND, OR, XOR, SLT, SLL, SRA
{
    char *rdraw = strsep(&instruction, ","); // isolating rd
    int rd = pull_register(rdraw);
    char *rs1raw = strsep(&instruction, ","); // isolating rs1
    int rs1 = pull_register(rs1raw);
    int rs2 = pull_register(instruction); // only rs2 remains

    if (strcmp(op, "add") == 0)
    {
        registers->r[rd] = registers->r[rs1] + registers->r[rs2];
    }
    else if (strcmp(op, "sub") == 0)
    {
        registers->r[rd] = registers->r[rs1] - registers->r[rs2];
    }
    else if (strcmp(op, "and") == 0)
    {
        registers->r[rd] = registers->r[rs1] & registers->r[rs2];
    }
    else if (strcmp(op, "or") == 0)
    {
        registers->r[rd] = registers->r[rs1] | registers->r[rs2];
    }
    else if (strcmp(op, "xor") == 0)
    {
        registers->r[rd] = registers->r[rs1] ^ registers->r[rs2];
    }
    else if (strcmp(op, "slt") == 0)
    {
        if (registers->r[rs1] < registers->r[rs2])
        {
            registers->r[rd] = 1;
        }
        else
        {
            registers->r[rd] = 0;
        }       
    }
    else if (strcmp(op, "sll") == 0)
    {
        registers->r[rd] = registers->r[rs1] << registers->r[rs2];
    }
    else if (strcmp(op, "sra") == 0) // arithmetic right shift guaranteed, as per Ed Discussion #1833
    {
        registers->r[rd] = registers->r[rs1] >> registers->r[rs2];       
    }
}

void i(char *instruction, char *op) // I-TYPE helper: ADDI, ANDI
{
    char *rdraw = strsep(&instruction, ","); // isolating rd
    int rd = pull_register(rdraw);
    char *rs1raw = strsep(&instruction, ","); // isolating rs1
    int rs1 = pull_register(rs1raw);
    int imm = to_decimal(instruction, 12); // only immediate remains
    if (strcmp(op, "addi") == 0)
    {
        registers->r[rd] = registers->r[rs1] + imm;
    }
    else // andi
    {
        registers->r[rd] = registers->r[rs1] & imm;
    }
}

void mem(char *instruction, char *op) // MEM-TYPE helper: LW, LB, SW, SB
{
    char *rs1raw = strsep(&instruction, ","); // isolating rs1/rd
    int rs1 = pull_register(rs1raw);
    char *immraw = strsep(&instruction, "("); // isolating immediate
    int imm = to_decimal(immraw, 12);
    char *rs2raw = strsep(&instruction, ")"); // isolating rs2
    int rs2 = pull_register(rs2raw);

    int store;
    int load;
    int sum = registers->r[rs2] + imm; // calculating offset
    int whichbyte = sum % 4; // used for byte addressing
    const int byte0bitmask = 0b00000000000000000000000011111111; // bitmask to extract byte 0
    const int byte1bitmask = 0b00000000000000001111111100000000; // bitmask to extract byte 1
    const int byte2bitmask = 0b00000000111111110000000000000000; // bitmask to extract byte 2
    const unsigned int byte3bitmask = 0b11111111000000000000000000000000; // bitmask to extract byte 3
    const unsigned int lssign_extend = 0b11111111111111111111111100000000; // used to sign-extend 
    int lowerbyte = registers->r[rs1] & byte0bitmask;

    if (strcmp(op, "lw") == 0)
    {
        int load_val = 0;
        for (int i = 0; i < 4; ++i)
        {
            load_val += ht_get(memory, sum + i);
        }
        registers->r[rs1] = load_val;
    }

    else if (strcmp(op, "lb") == 0)
    {
        int temp;
        if (whichbyte == 0)
        {
            load = ht_get(memory, sum);
        }
        else if (whichbyte == 1)
        {
            temp = ht_get(memory, sum) >> 8;
            load = temp; // shifting by 1 byte to correctly load into register
        }
        else if (whichbyte == 2)
        {
            temp = ht_get(memory, sum) >> 16;
            load = temp; // shifting by 2 bytes
        }
        else if (whichbyte == 3)
        {     
            temp = ht_get(memory, sum) >> 24; 
            load = temp; // shifting by 3 bytes
        }
        if ((load >> 7) > 0) // e.g., MSB is 1: negative load value
        {
            load = load | lssign_extend;
        }
        registers->r[rs1] = load;
    }

    else if (strcmp(op, "sw") == 0)
    {
        int byte_0 = registers->r[rs1] & byte0bitmask; // bitmasking the register value (a word) into four bytes to be stored in their own addresses
        int byte_1 = registers->r[rs1] & byte1bitmask;
        int byte_2 = registers->r[rs1] & byte2bitmask;
        int byte_3 = registers->r[rs1] & byte3bitmask;
        ht_add(memory, sum, byte_0);
        ht_add(memory, sum+1, byte_1);
        ht_add(memory, sum+2, byte_2);
        ht_add(memory, sum+3, byte_3);
    }

    else // SB
    {
        if (whichbyte == 1) 
        {
            store = lowerbyte << 8; // left-shifting by one byte to store in first byte
        }
        else if (whichbyte == 2)
        {
            store = lowerbyte << 16; // left-shift 2 bytes
        }
        else if (whichbyte == 3)
        {
            store = lowerbyte << 24; // left-shift 3 bytes
        }
        else
        {
            store = lowerbyte;
        }
        ht_add(memory, sum, store);
    }
}

void u(char *instruction) // U-TYPE helper: LUI
{
    char *rdraw = strsep(&instruction, ",");
    int rd = pull_register(rdraw);
    int imm = to_decimal(instruction, 20);

    registers->r[rd] = imm << 12; // left-shifting by 12 bits, as per LUI functionality
}

void step(char *instruction)
{
    // Extracts and returns the substring before the first space character,
    // by replacing the space character with a null-terminator.
    // `instruction` now points to the next character after the space
    // See `man strsep` for how this library function works
    char *op = strsep(&instruction, " ");
    // Uses the provided helper function to determine the type of instruction
    int op_type = get_op_type(op);
    // Skip this instruction if it is not in our supported set of instructions
    if (op_type == UNKNOWN_TYPE)
    {
        return;
    }
    else if (op_type == R_TYPE)
    {
        r(instruction, op);
    }
    else if (op_type == I_TYPE)
    {
        i(instruction, op);
    }
    else if (op_type == MEM_TYPE)
    {
        mem(instruction, op);
    }
    else // U-Type instruction
    {
        u(instruction);
    }   
    registers->r[0] = 0; // always resets 0 register to 0 before every instruction
}
