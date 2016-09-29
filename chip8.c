#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

uint8_t memory[4096];
uint16_t opcode;

uint8_t v_registers[16];
uint16_t i_register;

uint8_t delay_timer;
uint8_t sound_timer;

uint16_t program_counter = 512;
uint8_t stack_pointer;

uint16_t stack[16];

int8_t load_file(char *file_name)
{
	FILE *input = fopen(file_name, "r");
	if (!input) {
		printf("Could not read file %s\n", file_name);

		return -1;
	}

	fread(&memory[512], sizeof(uint8_t), 4096, input);

	return 0;
}

int8_t run_emulation()
{
	for (;;) {
		uint16_t instruction = 0; 	
		instruction = (memory[program_counter] << 8) | memory[program_counter + 1];

		uint8_t op_code = instruction >> 12;
		uint16_t argument = instruction & 8191;
		switch (op_code) {
			case 0xa:
				i_register = argument;
				program_counter = program_counter + 2;
				break;
			case 0x2:
				stack[stack_pointer] = program_counter; 
				stack_pointer++;
				program_counter = argument;
				break;
			default:
				printf("Encountered unknown op_code %x with argument %x\n", op_code, argument);
				return -1;
		}

	}

	return 0;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: chip8 $file_name\n");

		return EXIT_FAILURE;
	}

	if (load_file(argv[1]) == -1) {
		return EXIT_FAILURE;
	} 

	run_emulation();

	return EXIT_SUCCESS;
}
