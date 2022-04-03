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

#ifdef OSS_SOUND_SUPPORT

/* Includes */
#include "sdl_engine.h"

/* Defines */

/* Variables */

extern struct sdl_emulator_ext{
    int state;      /* FALSE=video output/keyboard input disabled, TRUE=all active */
    int paused;     /* Via Pause key: TRUE=emulation on-hold, keyboard input disabled */
    int xoffset;
    int yoffset;
    SDL_TimerID timer_id;
    int m1not;
    int speed;      /* 5ms=400%, 10ms=200%, 20ms=100%, 30ms=66%, 40ms=50% */
    int frameskip;          /* 0 to MAX_FRAMESKIP */
    int *model;     /* Points to z81's zx80: 0=ZX81, 1=ZX80 */
    #if defined(PLATFORM_MIYOO)
    int *fullscr;       /* 0=NO, 1=YES */
    #endif
    int ramsize;            /* 1, 2, 3, 4, 16, 32, 48 or 56K */
    int invert;     /* This should really be in video but it's easier to put it here */
    int autoload;           /* Set to TRUE when auto-loading or forced-loading */
    int networking;         /* enable calls to WIZ chip emulation */
    int bdis;
    int edis;
} sdl_emulator;

extern struct {
	int state;
	int volume;
	int device;		/* See DEVICE* defines in sdl_sound.h */
	int stereo;
	int ay_unreal;
	Uint16 buffer[SOUND_BUFFER_SIZE];
	int buffer_start;
	int buffer_end;
} sdl_sound;

extern struct {
	int state;
	unsigned char data[8 * 1024];
} sdl_zx81rom;

extern struct {
	int state;
	unsigned char data[4 * 1024];
} sdl_aszmicrom;

extern struct {
  int state;
  int xoffset;
  int yoffset;
  char dir[256];      /* The directory that files are loaded from and saved to */
  char *dirlist;      /* A list containing compatible entries from the directory */
  int dirlist_sizeof;   /* The size of each element within the list */
  int dirlist_count;    /* The count of files within the list */
  int dirlist_top;    /* The GUI list top as an index into the list */
  int dirlist_selected; /* The selected item as an index into the list */
  char loaded[256];   /* The fullpath of the most recently loaded/saved file */
  int method;       /* The loading method to be implemented for certain methods */
  int sbpgscrunit;
} load_file_dialog;

extern struct {
  int state;
  int xoffset;
  int yoffset;
  int slots[9];   /* The slots currently saved to (existing state files) */
  int mode;     /* Are we loading or saving */
} save_state_dialog;

extern SDL_Joystick *joystick;
extern int joystick_dead_zone;

extern int show_input_id;
extern int current_input_id;
extern int runopts_emulator_speed;
extern int runopts_emulator_model;
extern int runopts_emulator_ramsize;
extern int runopts_emulator_m1not;
extern int runopts_sound_device;
extern int runopts_sound_stereo;
extern int runopts_sound_ay_unreal;
extern struct bmpfont zx80font, zx81font, zx82font;  

extern struct hotspot_ext {
	int gid;						/* Group id for easy management */
	int flags;						/* An OR'd combination of HS_PROP_ properties */
	int hit_x, hit_y, hit_w, hit_h;	/* Hit box */
	int hl_x, hl_y, hl_w, hl_h;		/* Highlight box (if all UNDEFINED then use hitbox */
	int remap_id;					/* The main destination control id (could be UNDEFINED) */
} hotspot;

/* Function prototypes */


/***************************************************************************
 * Sound Initialise                                                        *
 ***************************************************************************/
/* On exit: returns FALSE on success else
 *          returns TRUE on error */

int sdl_sound_init(int freq, int *stereo, int *sixteenbit) {
	SDL_AudioSpec desired, obtained;
	
	sdl_sound.buffer_start = sdl_sound.buffer_end = 0;
	
	desired.freq = freq;
	#if defined(PLATFORM_GP2X)
		desired.format = AUDIO_U16;
		#if defined(TOOLCHAIN_OPEN2X) || defined(TOOLCHAIN_OPENWIZ)
			desired.samples = 256;
		#else
			desired.samples = 1024;	/* This might be better at 512 */
		#endif
	#else
		desired.format = AUDIO_U8;	/* z81's default */
		desired.samples = 1024;		/* This might be better at 512 */
	#endif
	desired.channels = *stereo + 1;
	desired.callback = sdl_sound_callback;
	desired.userdata = NULL;

	/* Open the audio device */
	if (SDL_OpenAudio(&desired, &obtained) < 0 ) {
		fprintf(stderr, "%s: Couldn't open audio: %s\n", __func__, SDL_GetError());
		return TRUE;
	}

	sdl_sound.state = TRUE;

	*stereo = obtained.channels - 1;
	*sixteenbit = (obtained.format & 0xff) / 16;

	/* Start playing */
	SDL_PauseAudio(0);

	#ifdef SDL_DEBUG_SOUND
		printf("%s:\n", __func__);
		printf("  desired.freq=%i\n", desired.freq);
		printf("  desired.format=");
		if (desired.format == 0x0008) printf("AUDIO_U8\n");
		if (desired.format == 0x8008) printf("AUDIO_S8\n");
		if (desired.format == 0x0010) printf("AUDIO_U16LSB/AUDIO_U16\n");
		if (desired.format == 0x8010) printf("AUDIO_S16LSB/AUDIO_S16\n");
		if (desired.format == 0x1010) printf("AUDIO_U16MSB\n");
		if (desired.format == 0x9010) printf("AUDIO_S16MSB\n");
		printf("  desired.channels=%i\n", desired.channels);
		printf("  desired.samples=%i\n", desired.samples);
		printf("  obtained.freq=%i\n", obtained.freq);
		printf("  obtained.format=");
		if (obtained.format == 0x0008) printf("AUDIO_U8\n");
		if (obtained.format == 0x8008) printf("AUDIO_S8\n");
		if (obtained.format == 0x0010) printf("AUDIO_U16LSB/AUDIO_U16\n");
		if (obtained.format == 0x8010) printf("AUDIO_S16LSB/AUDIO_S16\n");
		if (obtained.format == 0x1010) printf("AUDIO_U16MSB\n");
		if (obtained.format == 0x9010) printf("AUDIO_S16MSB\n");
		printf("  obtained.channels=%i\n", obtained.channels);
		printf("  obtained.samples=%i\n", obtained.samples);
	#endif

	return FALSE;
}

/***************************************************************************
 * Sound Callback                                                          *
 ***************************************************************************/
/* When the audio device is ready for more data it calls this function which
 * could be running in another thread. The data will have been previously
 * stored in our linear sound buffer via the sound frame function below */

void sdl_sound_callback(void *userdata, Uint8 *stream, int len) {
	#if defined(SDL_DEBUG_SOUND) || defined(SDL_DEBUG_TIMING)
		static Uint32 lasttime = 0;
		static int Hz = 0;

		Hz++;
		if (SDL_GetTicks() - lasttime >= 1000) {
			printf("%s=%iHz", __func__, Hz);
			printf(" stream len=%i\n", len);
			lasttime = SDL_GetTicks();
			Hz = 0;
		}
	#endif

	/* Generate white noise for testing
	while (len-- > 0) {
		*(stream++) = (rand() % 256) / 8;
	} */

	/* Keep writing to the stream until it's full or our linear
	 * buffer's end reaches its start i.e. no more sound data */

	len /= 2;
	while (len--) {
		if (sdl_sound.buffer_start == sdl_sound.buffer_end) break;
		*(stream++) = sdl_sound.buffer[sdl_sound.buffer_start] & 0xff;
		*(stream++) = (sdl_sound.buffer[sdl_sound.buffer_start++] & 0xff00) >> 8;
		if (sdl_sound.buffer_start >= SOUND_BUFFER_SIZE) sdl_sound.buffer_start = 0;
	}
}

/***************************************************************************
 * Sound Frame                                                             *
 ***************************************************************************/
/* This function receives sound data from the emulator which we store in our
 * linear sound buffer. It is read later by the sound card via the above
 * callback which could be running in another thread and so SDL_LockAudio and
 * SDL_UnlockAudio are used to lockout the callback function to prevent it
 * from interfering with what we're doing here */

void sdl_sound_frame(Uint16 *data, int len) {
#ifdef SDL_DEBUG_SOUND
	static int ovfcnt = 0;
#endif	
	SDL_LockAudio();
	while (len--) {
		sdl_sound.buffer[sdl_sound.buffer_end++] = *(data++);
		if (sdl_sound.buffer_end >= SOUND_BUFFER_SIZE) sdl_sound.buffer_end = 0;
		if (sdl_sound.buffer_end == sdl_sound.buffer_start) {
			sdl_sound.buffer_start++;
			if (sdl_sound.buffer_start >= SOUND_BUFFER_SIZE) sdl_sound.buffer_start = 0;
#ifdef SDL_DEBUG_SOUND
			if (ovfcnt++ < 10) fprintf(stderr, "%s: Sound buffer overflow\n", __func__);
#endif
		}
	}
	SDL_UnlockAudio();
}

/***************************************************************************
 * Sound End                                                               *
 ***************************************************************************/

void sdl_sound_end(void) {
	if (sdl_sound.state) {
		sdl_sound.state = FALSE;
		SDL_CloseAudio();
	}
}

#endif	/* OSS_SOUND_SUPPORT */


