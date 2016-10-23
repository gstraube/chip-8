#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include <GL/glew.h>
#include <GL/glut.h>

#define WIDTH 64
#define HEIGHT 32

uint8_t memory[4096];
uint16_t opcode;

uint8_t v_registers[16];
uint16_t i_register;

uint8_t delay_timer;
uint8_t sound_timer;

uint16_t program_counter = 512;
uint8_t stack_pointer;

uint16_t stack[16];

uint8_t display[WIDTH][HEIGHT] = {0};

void draw_display();

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

void draw(uint16_t argument)
{
	uint8_t num_bytes = argument & 0xF;
	uint8_t y_offset = v_registers[(argument >> 4) & 0xF];
	uint8_t x_offset = v_registers[(argument >> 8) & 0xF];

	uint8_t *sprite = calloc(num_bytes, sizeof(uint8_t));

	v_registers[15] = 0;

	for (uint8_t i = 0; i < num_bytes; i++) {
		sprite[i] = memory[i_register + i];
	}

	for (int i = 0; i < num_bytes; i++) {
		for (int j = 0; j < 8; j++) {
			uint8_t x_pos = (i + x_offset) % WIDTH;
			uint8_t y_pos = (j + y_offset) % HEIGHT;

			uint8_t relevant_bit = sprite[i] << j;
			uint8_t value = relevant_bit >> (7 - j);

			if (display[x_pos][y_pos] && value) {
				v_registers[15] = 1;	
			} 

			display[x_pos][y_pos] ^= value; 
		}
	}
}

void advance_program_counter() {
	program_counter += 2;
}

void skip_next(bool cond)
{
	if (cond) {
		advance_program_counter();
	}
}

int8_t run_emulation()
{
	for (;;) {
		uint16_t instruction = 0; 	
		instruction = (memory[program_counter] << 8) | memory[program_counter + 1];

		uint8_t op_code = instruction >> 12;
		uint16_t argument = instruction & 0xFFF;

		uint8_t reg_number = argument >> 8;
		uint8_t value = argument & 0xFF;

		bool should_advance_program_counter = true;

		switch (op_code) {
			case 0xa:
				i_register = argument;
				break;
			case 0x1:
				program_counter = argument;
				should_advance_program_counter = false;
				break;
			case 0x2:
				stack_pointer++;
				stack[stack_pointer] = program_counter + 2; 
				program_counter = argument;
				should_advance_program_counter = false;
				break;
			case 0x3:
				skip_next(v_registers[reg_number] == value);
				break;
			case 0x4:
				skip_next(v_registers[reg_number] != value);
				break;
			case 0x6:
				v_registers[reg_number] = value;
				break;
			case 0x7:
				v_registers[reg_number] += value;
				break;
			case 0xC:
				v_registers[reg_number] = (rand() % (0xFF + 1)) & value; 
				break;
			case 0xD:
				draw(argument);
				break;
			case 0xF:
				if (value == 0x15) {
					delay_timer = v_registers[reg_number];
				} else {
					printf("Encountered unknown op_code %x with argument %x\n", op_code, argument);
					return -1;
				}
				break;
			case 0x0:
				if (argument == 0x0EE) {
					program_counter = stack[stack_pointer];
					stack_pointer--;
				} else {
					printf("Encountered unknown op_code %x with argument %x\n", op_code, argument);
					return -1;
				}
				should_advance_program_counter = false;
				break;
			default:
				printf("Encountered unknown op_code %x with argument %x\n", op_code, argument);
				return -1;
		}

		if (should_advance_program_counter) {
			advance_program_counter();
		}

	}

	return 0;
}

void draw_display()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	GLubyte pixels[HEIGHT][WIDTH][3];

	for (uint8_t i = 0; i < HEIGHT; i++) {
		for (uint8_t j = 0; j < WIDTH; j++) {
			uint8_t c = 255;
			if (!display[WIDTH - 1 - j][HEIGHT - 1 - i]) {
				c = 0;
			}
			pixels[i][j][0] = (GLubyte) c;
			pixels[i][j][1] = (GLubyte) c;
			pixels[i][j][2] = (GLubyte) c;
		}
	}

	glPixelZoom(10.0f, 10.0f);
	glDrawPixels(WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	glFlush();
}

uint8_t init_display(int *argc, char **argv)
{
	glutInitWindowSize(640, 320);
	glutInitWindowPosition(0, 0);
	glutInit(argc, argv);
	glutCreateWindow("chip-8");
	glutDisplayFunc(draw_display);

	return 0;
}

int main(int argc, char **argv)
{

	srand(time(NULL));

	if (argc != 2) {
		printf("Usage: chip8 $file_name\n");

		return EXIT_FAILURE;
	}

	if (load_file(argv[1]) == -1) {
		return EXIT_FAILURE;
	} 

	init_display(&argc, argv);

	run_emulation();

	glutMainLoop();

	return EXIT_SUCCESS;
}
