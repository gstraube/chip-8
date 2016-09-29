#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

uint8_t memory[4096];
uint16_t opcode;

uint8_t v_registers[16];
uint16_t i_register;

uint8_t delay_timer;
uint8_t sound_timer;

uint16_t program_counter;
uint8_t stack_pointer;

uint16_t stack[16];

uint8_t load_file(char *file_name)
{
	FILE *input = fopen(file_name, "r");
	if (!input) {
		printf("Could not read file %s\n", file_name);

		return -1;
	}

	fread(memory, sizeof(uint8_t), 4096, input);

	return 0;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: chip8 $file_name\n");
		
		return EXIT_FAILURE;
	}

	if (!load_file(argv[1])) {
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
