#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "defs.h"

#define WIDTH 320
#define HEIGHT 200

uint32_t framebuffer[WIDTH * HEIGHT];
const double TARGET_FRAMETIME = 1.0 / 35.0;

// DOOM PSX fire state variables and consts
#define FIRE_W WIDTH
#define FIRE_H HEIGHT
#define FIRE_PALETTE_SIZE 37

static u8 fire_pixels[FIRE_W * FIRE_H];
static const u32 fire_palette[FIRE_PALETTE_SIZE] = {
	0x070707, 0x1F0707, 0x2F0F07, 0x470F07, 0x571707, 0x671F07, 0x771F07,
	0x8F2707, 0x9F2F07, 0xAF3F07, 0xBF4707, 0xC74707, 0xDF4F07, 0xDF5707,
	0xDF5707, 0xD75F07, 0xD75F07, 0xD7670F, 0xCF6F0F, 0xCF770F, 0xCF7F0F,
	0xCF8717, 0xC78717, 0xC78F17, 0xC7971F, 0xBF9F1F, 0xBF9F1F, 0xBFA727,
	0xBFA727, 0xBFAF2F, 0xB7AF2F, 0xB7B72F, 0xB7B737, 0xCFCF6F, 0xDFDF9F,
	0xEFEFC7, 0xFFFFFF
};

void put_pixel(int x, int y, uint32_t color) {
	if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
		return;
	}
	framebuffer[(WIDTH*y)+x] = color;
}

void clear(uint32_t color) {
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			put_pixel(x, y, color);	
		}
	}	
}

int main (void) {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Event event;	

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr, "sdl init failed: %s \n", SDL_GetError());
		return EXIT_FAILURE;
	};

	window = SDL_CreateWindow(
		"fb", 
		WIDTH, 
		HEIGHT, 
		SDL_WINDOW_RESIZABLE
	);

	if (window == NULL) {
		fprintf(stderr, "sdl window not created: %s \n", SDL_GetError());
		SDL_Quit();
		return EXIT_FAILURE;
	}

	renderer = SDL_CreateRenderer(
		window, 
		NULL
	);

	if (renderer == NULL) {
		fprintf(stderr, "sdl renderer not created: %s \n", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return EXIT_FAILURE;
	}

	texture = SDL_CreateTexture(
		renderer, 
		SDL_PIXELFORMAT_XRGB8888, 
		SDL_TEXTUREACCESS_STREAMING, 
		WIDTH, 
		HEIGHT
	);

	if (texture == NULL) {
		fprintf(stderr, "sdl texture not created: %s \n", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return EXIT_FAILURE;
	}

	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
	SDL_SetWindowFullscreen(window, 1);

	uint8_t is_running = 1;
	uint64_t frame = 0;
	
	// Fire init
	for (int i = 0; i < FIRE_W * FIRE_H; i++) {
		fire_pixels[i] = 0;
	}
	// init bottom row with fire source intensity
	for (int i = 0; i < FIRE_W; i++) {
		fire_pixels[(FIRE_H-1) * FIRE_W + i] = FIRE_PALETTE_SIZE - 1;
	}


	while (is_running) {

		clear(0x000000);
		
		uint64_t start = SDL_GetPerformanceCounter();

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				is_running = 0;		
			}
		}
		
		// !!!
		// for (int y = 0; y < HEIGHT; y++) {
		// 	for (int x = 0; x < WIDTH; x++) {
		// 		put_pixel(x, y, 0x00ff00);	
		//	}
		// }

		// put_pixel(frame % WIDTH, HEIGHT/2, 0xFF0000); 
		
		/* Fire update */
		for (u32 x = 0; x < FIRE_W; x++) {
			for (u32 y = 1; y < FIRE_H; y++) {
				u32 src = FIRE_W * y + x;
				u32 rand_idx = rand() % 4;
				i32 dst = (i32)src - FIRE_W;
				i32 dst_x = (i32)(src % FIRE_W) - (i32)rand_idx + 1;
				if (dst < 0 || dst_x < 0 || dst_x >= FIRE_W) {
					continue;
				} else {
					dst = dst - (i32)(src % FIRE_W) + dst_x;
					u8 src_val = fire_pixels[src];
					u8 decay = rand_idx & 1;
					fire_pixels[dst] = (src_val > decay) ? (src_val - decay) : 0;
				}
			}
		}

		/* Fire render */
		for (u32 y = 0; y < FIRE_H; y++) {
			for (u32 x = 0; x < FIRE_W; x++) {
				u8 idx = fire_pixels[y * FIRE_W + x];
				put_pixel(x, y, fire_palette[idx]);
			}
		}

		SDL_UpdateTexture(
			texture,
			NULL,
			framebuffer,
			WIDTH * sizeof(uint32_t)
		);
		SDL_RenderClear(renderer);
		SDL_RenderTexture(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		uint64_t end = SDL_GetPerformanceCounter();
		double elapsed = (double)(end - start) / SDL_GetPerformanceFrequency();

		if (elapsed < TARGET_FRAMETIME) {
			SDL_Delay((TARGET_FRAMETIME - elapsed) * 1000.0);
		}

		frame+=1;
	}
	
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return EXIT_SUCCESS;
}
