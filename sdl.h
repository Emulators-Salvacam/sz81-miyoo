#ifndef _SDL_H_
#define _SDL_H_

/* sz81 Copyright (C) 2007-2011 Thunor <thunorsif@hotmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* This is how the SDL part of sz81 is organised :-
 * 
 *         sdl.h <----- sdl_engine.h ---> sdl_*.h
 *         ^   ^              ^
 *        /     \             |
 *  common.c   sdl_main.c   sdl_*.c
 *   sound.c
 *     z80.c
 */

/* Includes */
#include <SDL/SDL.h>

/* Defines */
#define MAX_KEYCODES 358	/* SDL stops at 322 and then I extend them */

#if defined(PLATFORM_DINGUX_A320)
   #define MAX_HOTSPOTS 239 
#else
   #define MAX_HOTSPOTS 199
#endif

#define MAX_FONTS 4

/* Interrupt types */
#define INTERRUPT_EMULATOR_RESET 3
#define INTERRUPT_EMULATOR_EXIT 4

/* Machine models */
#define MODEL_ZX81 0
#define MODEL_ZX80 1

/* Load file methods */
#define LOAD_FILE_METHOD_NONE 0
#define LOAD_FILE_METHOD_DETECT 1
#define LOAD_FILE_METHOD_AUTOLOAD 2
#define LOAD_FILE_METHOD_FORCEDLOAD 3
#define LOAD_FILE_METHOD_NAMEDLOAD 4
#define LOAD_FILE_METHOD_SELECTLOAD 5
#define LOAD_FILE_METHOD_SELECTLOADOK 6
#define LOAD_FILE_METHOD_STATELOAD 7

/* Save file methods */
#define SAVE_FILE_METHOD_NAMEDSAVE 1
#define SAVE_FILE_METHOD_UNNAMEDSAVE 2
#define SAVE_FILE_METHOD_STATESAVE 3

/* 16KB was fine for everything but the Wiz is currently experiencing
 * linear buffer overflow and so I'm quadrupling it for the Wiz only */
#if defined(PLATFORM_GP2X) && defined (TOOLCHAIN_OPENWIZ)
	#define SOUND_BUFFER_SIZE (1024 * 8 * 4)
#elif defined(PLATFORM_DINGUX_A320)
	#define SOUND_BUFFER_SIZE (1024 * 8 * 4)
#else
        #define SOUND_BUFFER_SIZE (1024 * 8)
#endif

#if defined(PLATFORM_MIYOO)
/* Full Screen */
#define FULL_SCREEN_NO 0
#define FULL_SCREEN_YES 1
#endif

/* Variables */
extern int keyboard_buffer[MAX_KEYCODES];

struct keyrepeat {
	int delay;
	int interval;
};
struct hotspot {
	int gid;						/* Group id for easy management */
	int flags;						/* An OR'd combination of HS_PROP_ properties */
	int hit_x, hit_y, hit_w, hit_h;	/* Hit box */
	int hl_x, hl_y, hl_w, hl_h;		/* Highlight box (if all UNDEFINED then use hitbox */
	int remap_id;					/* The main destination control id (could be UNDEFINED) */
};

struct bmpfont {
	SDL_Surface *original;
	SDL_Surface *scaled[MAX_FONTS];
	Uint32 fg_colour[MAX_FONTS];
	Uint32 requested[MAX_FONTS];
};

/* Function prototypes */
int sdl_init(void);
int sdl_com_line_process(int argc, char *argv[]);
int sdl_video_setmode(void);
unsigned char *vga_getgraphmem(void);
void sdl_keyboard_init(void);
void sdl_hotspots_init(void);
void sdl_rcfile_read(void);
int sdl_zxroms_init(void);
void sdl_component_executive(void);
void sdl_timer_init(void);
void sdl_zxprinter_init(void);
int keyboard_update(void);
void sdl_video_update(void);
int sdl_sound_init(int freq, int *stereo, int *sixteenbit);
void sdl_sound_callback(void *userdata, Uint8 *stream, int len);
void sdl_sound_frame(Uint16 *data, int len);
void sdl_sound_end(void);
int sdl_filetype_casecmp(char *filename, char *filetype);
int sdl_load_file(int parameter, int method);
int sdl_save_file(int parameter, int method);

#endif
