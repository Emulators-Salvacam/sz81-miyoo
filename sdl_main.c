/* z81/xz81, Linux console and X ZX81/ZX80 emulators.
 * Copyright (C) 1994 Ian Collier. z81 changes (C) 1995-2001 Russell Marks.
 * sz81 Copyright (C) 2007-2011 Thunor <thunorsif@hotmail.com>
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

/* Includes */
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL/SDL_main.h>
#include "sdl.h"
#include "common.h"
#include "sound.h"
#include "z80/z80.h"
#include "zx81.h"
#include "allmain.h"
#ifdef __amigaos4__
	#include "amiga.h"
#endif

/* Defines */

/* Variables */
//int hsize = ZX_VID_VGA_WIDTH, vsize = ZX_VID_VGA_HEIGHT; // Sorry about that!
int hsize = ZX_VID_X_WIDTH, vsize = ZX_VID_X_HEIGHT;
unsigned char *vptr;

int keyboard_buffer[MAX_KEYCODES];

struct {
	int nxtlin;
	int load_hook;
	int save_hook;
	int rsz81mem;
	int wsz81mem;
	int bigscreen;
	int fullscreen;
	int networking;
	int scale;
	int xres;
	int yres;
	int bdis;
	int edis;
	char filename[256];
} sdl_com_line;

struct {
	int state;		/* FALSE=video output/keyboard input disabled, TRUE=all active */
	int paused;		/* Via Pause key: TRUE=emulation on-hold, keyboard input disabled */
	int xoffset;
	int yoffset;
	SDL_TimerID timer_id;
	int m1not;
	int speed;		/* 5ms=400%, 10ms=200%, 20ms=100%, 30ms=66%, 40ms=50% */
	int frameskip;	        /* 0 to MAX_FRAMESKIP */
	int *model;		/* Points to z81's zx80: 0=ZX81, 1=ZX80 */
	#if defined(PLATFORM_MIYOO)
	int *fullscr;		/* 0=NO, 1=YES */
	#endif
	int ramsize;	        /* 1, 2, 3, 4, 16, 32, 48 or 56K */
	int invert;		/* This should really be in video but it's easier to put it here */
	int autoload;	        /* Set to TRUE when auto-loading or forced-loading */
	int networking;         /* enable calls to WIZ chip emulation */
	int bdis;
	int edis;
} sdl_emulator;

struct {
	int state;
	unsigned char data[4 * 1024];
} sdl_zx80rom;

struct keyrepeat sdl_key_repeat;

struct {
	int state;
	int volume;
	int device;		/* See DEVICE* defines in sdl_sound.h */
	int stereo;
	int ay_unreal;
	Uint16 buffer[SOUND_BUFFER_SIZE];
	int buffer_start;
	int buffer_end;
} sdl_sound;

struct {
	int state;
	unsigned char data[8 * 1024];
} sdl_zx81rom;

struct {
	int state;
	unsigned char data[4 * 1024];
} sdl_aszmicrom;

struct {
	int state;
	int xoffset;
	int yoffset;
	char dir[256];			/* The directory that files are loaded from and saved to */
	char *dirlist;			/* A list containing compatible entries from the directory */
	int dirlist_sizeof;		/* The size of each element within the list */
	int dirlist_count;		/* The count of files within the list */
	int dirlist_top;		/* The GUI list top as an index into the list */
	int dirlist_selected;	/* The selected item as an index into the list */
	char loaded[256];		/* The fullpath of the most recently loaded/saved file */
	int method;				/* The loading method to be implemented for certain methods */
	int sbpgscrunit;
} load_file_dialog;

struct {
	int state;
	int xoffset;
	int yoffset;
	int slots[9];		/* The slots currently saved to (existing state files) */
	int mode;			/* Are we loading or saving */
} save_state_dialog;


SDL_Joystick *joystick;
int joystick_dead_zone;


int show_input_id;
int current_input_id;
int runopts_emulator_speed;
int runopts_emulator_model;
int runopts_emulator_ramsize;
int runopts_emulator_m1not;
int runopts_sound_device;
int runopts_sound_stereo;
int runopts_sound_ay_unreal;

struct hotspot hotspots[MAX_HOTSPOTS];


struct bmpfont zx80font, zx81font, zx82font;	

/* Function prototypes */

/***************************************************************************
 * Update Screen                                                           *
 ***************************************************************************/
/* redraw the screen */

void update_scrn(void) {
	unsigned char *ptr, *optr, *cptr, d, dc;
	int x, y, a, mask;

	for (y = 0; y < ZX_VID_X_HEIGHT; y++) {
		ptr = scrnbmp + (y + ZX_VID_X_YOFS) * 
			ZX_VID_FULLWIDTH / 8 + ZX_VID_X_XOFS / 8;
		optr = scrnbmp_old + (ptr - scrnbmp);
		cptr = scrnbmpc + (y + ZX_VID_X_YOFS) * 
			ZX_VID_FULLWIDTH + ZX_VID_X_XOFS;
		for (x = 0; x < ZX_VID_X_WIDTH; x += 8, ptr++, optr++) {
			d = *ptr;
			if (d != *optr || refresh_screen || chromamode) {
				if (sdl_emulator.invert) d = ~d;
				for (a = 0, mask = 128; a < 8; a++, mask >>= 1) {
					if (chromamode) {
						vptr[y * 400 + x + a] = dc = *(cptr++);
					} else {
						vptr[y * 400 + x + a] = ((d&mask) ? 0 : 15);
					}
				}
			}
		}
	}

	/* now, copy new to old for next time */
	memcpy(scrnbmp_old, scrnbmp, ZX_VID_FULLHEIGHT * ZX_VID_FULLWIDTH / 8);

	refresh_screen = 0;

	sdl_video_update();
}

/***************************************************************************
 * Check Events                                                            *
 ***************************************************************************/
/* read keyboard and update keyports[] */

void check_events(void) {
	int b, y;

	keyboard_update();

	/* ugly, but there's no pleasant way to do this */
	if (sdl_emulator.state && !sdl_emulator.paused) {
		for (y = 0; y < 8; y++) {	/* 8 half-rows */
			b = 0;					/* we set bits in b as appropriate */
			switch (y) {			/* below comments given in order b1->b5 */
				/* left-hand side */
				case 0:	/* sft,z,x,c,v */
					if (keyboard_buffer[SDLK_LSHIFT] ||
						keyboard_buffer[SDLK_RSHIFT] ||
						keyboard_buffer[SDLK_LCTRL] ||
						keyboard_buffer[SDLK_RCTRL])
						b |= 1;
					if (keyboard_buffer[SDLK_z]) b |= 2;
					if (keyboard_buffer[SDLK_x]) b |= 4;
					if (keyboard_buffer[SDLK_c]) b |= 8;
					if (keyboard_buffer[SDLK_v]) b |= 16;
					break;
				case 1:	/* a,s,d,f,g */
					if (keyboard_buffer[SDLK_a]) b |= 1;
					if (keyboard_buffer[SDLK_s]) b |= 2;
					if (keyboard_buffer[SDLK_d]) b |= 4;
					if (keyboard_buffer[SDLK_f]) b |= 8;
					if (keyboard_buffer[SDLK_g]) b |= 16;
					break;
				case 2:	/* q,w,e,r,t */
					if (keyboard_buffer[SDLK_q]) b |= 1;
					if (keyboard_buffer[SDLK_w]) b |= 2;
					if (keyboard_buffer[SDLK_e]) b |= 4;
					if (keyboard_buffer[SDLK_r]) b |= 8;
					if (keyboard_buffer[SDLK_t]) b |= 16;
					break;
				case 3:	/* 1,2,3,4,5 */
					if (keyboard_buffer[SDLK_1]) b |= 1;
					if (keyboard_buffer[SDLK_2]) b |= 2;
					if (keyboard_buffer[SDLK_3]) b |= 4;
					if (keyboard_buffer[SDLK_4]) b |= 8;
					if (keyboard_buffer[SDLK_5]) b |= 16;
					break;
				/* right-hand side */
				case 4:	/* 0,9,8,7,6 */
					if (keyboard_buffer[SDLK_0]) b |= 1;
					if (keyboard_buffer[SDLK_9]) b |= 2;
					if (keyboard_buffer[SDLK_8]) b |= 4;
					if (keyboard_buffer[SDLK_7]) b |= 8;
					if (keyboard_buffer[SDLK_6]) b |= 16;
					break;
				case 5:	/* p,o,i,u,y */
					if (keyboard_buffer[SDLK_p]) b |= 1;
					if (keyboard_buffer[SDLK_o]) b |= 2;
					if (keyboard_buffer[SDLK_i]) b |= 4;
					if (keyboard_buffer[SDLK_u]) b |= 8;
					if (keyboard_buffer[SDLK_y]) b |= 16;
					break;
				case 6:	/* ent,l,k,j,h */
					if (keyboard_buffer[SDLK_RETURN]) b |= 1;
					if (keyboard_buffer[SDLK_l]) b |= 2;
					if (keyboard_buffer[SDLK_k]) b |= 4;
					if (keyboard_buffer[SDLK_j]) b |= 8;
					if (keyboard_buffer[SDLK_h]) b |= 16;
					break;
				case 7:	/* spc,dot,m,n,b */
					if (keyboard_buffer[SDLK_SPACE]) b |= 1;
					if (keyboard_buffer[SDLK_PERIOD]) b |= 2;
					if (keyboard_buffer[SDLK_m]) b |= 4;
					if (keyboard_buffer[SDLK_n]) b |= 8;
					if (keyboard_buffer[SDLK_b]) b |= 16;
					break;
			}
			/* some things need top bits to be 1 */
			keyports[y] = ((b ^ 31) | 0xe0);
		}
	}
}

/***************************************************************************
 * Main                                                                    *
 ***************************************************************************/
/* Originally z81 was set-up via the command-line interface (CLI) and
 * sz81 managed what it could around z81, but now sz81 is going to
 * manage everything and therefore needs to initialise first */

#ifdef __cplusplus
	extern "C"
#endif

int main(int argc, char *argv[]) {
	int retval = 0;

	#ifdef __amigaos4__
		amiga_open_libs();
	#endif

	/* Initialise sz81 variables, SDL, WM icon, local data dir */
	retval = sdl_init();
	if (!retval) {

		/* sz81 supports just the necessary options via the CLI, such
		 * as an initial video resolution and window/fullscreen */
		retval = sdl_com_line_process(argc, argv);
		if (!retval) {

			/* I personally like to dump this about here so that if
			 * something goes wrong then it's clear to the user where
			 * everything is supposed to be */
			fprintf(stdout, "PACKAGE_DATA_DIR is %s\n", PACKAGE_DATA_DIR);

			/* Set the video mode, set-up component screen offsets,
			 * initialise fonts, icons, vkeyb and control bar */ 
			retval = sdl_video_setmode();
			if (!retval) {

				/* This is a reminder that sz81 is still emulating
				 * the SVGAlib API ;) */
				vptr = vga_getgraphmem();

				/* Initialise the keyboard buffer, open a joystick
				 * and set-up control remappings */
				sdl_keyboard_init();

				/* Read the project's rcfile if it exists and update
				 * variables that are defined within it */
				sdl_rcfile_read();

				/* Initialise the hotspots now (this will set the
				 * default initial vkeyb hotspot for the model) */
				sdl_hotspots_init();

				/* Load both the ZX80 and ZX81 ROMs */
				sdl_zxroms_init();

				/* Initialise the emulator timer */
				sdl_timer_init();

				/* Synchronise */
				sdl_component_executive();
				
				while (interrupted != INTERRUPT_EMULATOR_EXIT) {

					interrupted = 0;

					/* Initialise printer variables */
					zxpinit();

					/* Initialise the printer file */
					sdl_zxprinter_init();

					/* Initialise the ZX80 or ZX81 */
					zx81_initialise();

					/* Initialise the required ROM and RAM */
					initmem();

					#ifdef OSS_SOUND_SUPPORT
						/* z81 has a variable 'sound' that is set to true
						 * if the user requests sound else false. At this
						 * point if 'sound' is true then the sound system is
						 * initialised which should result in sound_enabled=1.
						 * Henceforth the 'sound' variable is ignored and
						 * sound_enabled is used to check the sound state */
						if (sound) sound_init();
					#endif

					/* And off we go... */
					/* printf("%s: Entering mainloop, autoload=%i\n",
						__func__, sdl_emulator.autoload);	temp temp */
					mainloop();

					#ifdef OSS_SOUND_SUPPORT
						/* If sound_enabled=1 then sound is operational */
						if (sound_enabled) {
							sound_end();
							sound_reset();
						}
					#endif

					/* Close any open printer file */
					zxpclose();

					/* Reinitialise variables at the top of z80.c */
					z80_reset();

					/* Reinitialise variables at the top of common.c */
					common_reset();
				}
			}
		}
	}

	/* clean_up_before_exit will tidy up here before returning to the
	 * OS (it was registered earlier with atexit within sdl_init) */

	return retval;
}

