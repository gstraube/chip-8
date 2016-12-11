#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#include <SDL.h>

#define WIDTH 64
#define HEIGHT 32
#define CYCLE_TIME 17

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

SDL_Window *window;
SDL_Renderer *renderer;

bool key_pressed[16];

void draw_in_window();

bool debug = false;
FILE *debug_output;

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

void set_v_register(uint16_t argument)
{
	uint8_t discriminator = argument & 0XF; 
	uint8_t target = (argument >> 8) & 0XF;
	uint8_t source = (argument >> 4) & 0xF;

	uint8_t value;
	switch (discriminator) {
		case 0x0:
			value = v_registers[source];	
			break;
		default:
			return;		
	}

	v_registers[target] = value;
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

	bool has_drawn = false;
	for (int i = 0; i < num_bytes; i++) {
		for (int j = 0; j < 8; j++) {
			uint8_t x_pos = (j + x_offset) % WIDTH;
			uint8_t y_pos = (i + y_offset) % HEIGHT;

			uint8_t filter = 1;

			for (int k = 0; k < (7 - j); k++) {
				filter = filter * 2;
			}

			uint8_t value = sprite[i] & filter;

			if (display[x_pos][y_pos] && value) {
				v_registers[15] = 1;	
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
				SDL_RenderDrawPoint(renderer, x_pos, y_pos);
				SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
			} 

			display[x_pos][y_pos] ^= value; 

			if (display[x_pos][y_pos]) {
				SDL_RenderDrawPoint(renderer, x_pos, y_pos);
				has_drawn = true;
			}
		}
	}

	if (has_drawn) {
		SDL_RenderPresent(renderer);
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

uint8_t map_key(SDL_Keycode key)
{
	switch (key) {
		case SDLK_4: return 0x1;
		case SDLK_5: return 0x2;
		case SDLK_6: return 0x3;
		case SDLK_7: return 0xC;
		case SDLK_r: return 0x4;
		case SDLK_t: return 0x5;
		case SDLK_z: return 0x6;
		case SDLK_u: return 0xD;
		case SDLK_f: return 0x7;
		case SDLK_g: return 0x8;
		case SDLK_h: return 0x9;
		case SDLK_j: return 0xE;
		case SDLK_v: return 0xA;
		case SDLK_b: return 0x0;
		case SDLK_n: return 0xB;
		case SDLK_m: return 0xF;
		default: return -1;
	}
}

void register_key(SDL_Keycode key)
{
	key_pressed[map_key(key)] = true;
}

void unregister_key(SDL_Keycode key)
{
	key_pressed[map_key(key)] = false;
}

void output_registers()
{
	if (debug) {
		for (int i = 0; i < 16; i++) {
			fprintf(debug_output, "V[%x]: %x\n", i, v_registers[i]);
		}

		fprintf(debug_output, "I: %x\n", i_register);
		fprintf(debug_output, "Delay timer: %x\n", delay_timer);
		fprintf(debug_output, "Sound timer: %x\n", sound_timer);

		fprintf(debug_output, "Program counter: %x\n", program_counter);

		fprintf(debug_output, "\n");
	}
}

int8_t run_emulation()
{
	SDL_Event event;
	bool has_user_quit = false;


	while (!has_user_quit) {
		struct timeval start, end;
		gettimeofday(&start, NULL);

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				has_user_quit = true;
			}
			if (event.type == SDL_KEYDOWN) {
				register_key(event.key.keysym.sym);
			}
			if (event.type == SDL_KEYUP) {
				unregister_key(event.key.keysym.sym);
			}
		}

		uint16_t instruction = 0; 	
		instruction = (memory[program_counter] << 8) | memory[program_counter + 1];

		uint8_t op_code = instruction >> 12;
		uint16_t argument = instruction & 0xFFF;

		uint8_t reg_number = argument >> 8;
		uint8_t value = argument & 0xFF;

		bool should_advance_program_counter = true;

		if (debug) {
			fprintf(debug_output, "Instruction: %x\n", instruction);
		}

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
			case 0x8:
				set_v_register(argument);
				break;
			case 0xC:
				v_registers[reg_number] = (rand() % (0xFF + 1)) & value; 
				break;
			case 0xD:
				draw(argument);
				break;
			case 0xE:
				if (value == 0xA1) {
					skip_next(!key_pressed[v_registers[reg_number]]);
				} else if (value == 0x9E) {
					skip_next(key_pressed[v_registers[reg_number]]);
				} else {
					printf("Encountered unknown op_code %x with argument %x\n", op_code, argument);
					return -1;
				}
				break;
			case 0xF:
				if (value == 0x15) {
					delay_timer = v_registers[reg_number];
				} else if (value == 0x1E) {
					i_register += v_registers[reg_number];
				} else if (value == 0x07) {
					v_registers[reg_number] = delay_timer;
				} else if (value == 0x65) {
					for (int i = 0; i <= reg_number; i++) {
						v_registers[i] = memory[i_register + i];
					}
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

		gettimeofday(&end, NULL);
		double diff = (end.tv_usec - start.tv_usec) / 1000.0;

		if (diff < CYCLE_TIME) {
			SDL_Delay(CYCLE_TIME - diff);
		}

		if (delay_timer) {
			delay_timer--;
		}

		output_registers();
	}	

	return 0;
}

void create_window()
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(WIDTH * 10, HEIGHT * 10, 0, &window, &renderer);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);

	if (SDL_RenderSetScale(renderer, 10.0f, 10.0f)) {
		printf("Error: %s\n", SDL_GetError());
	}

	SDL_RenderPresent(renderer);
}

int main(int argc, char **argv)
{
	srand(time(NULL));

	if (argc < 2) {
		printf("Usage: chip8 $file_name\n");

		return EXIT_FAILURE;
	}

	if (argc == 3 && strncmp("-debug", argv[2], 6) == 0) {
		debug = true;

		debug_output = fopen("debug.out", "w");

		if (!debug_output) {
			printf("Could not debug output file\n");

			return -1;
		} else {
			printf("Debug output will be written to debug.out\n");
		}
	}

	if (load_file(argv[1]) == -1) {
		return EXIT_FAILURE;
	} 

	create_window();

	run_emulation();
	SDL_DestroyWindow(window);
	SDL_Quit;

	return EXIT_SUCCESS;
}
