#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <SDL.h>

#include "nanovg.h"
#include "miniaudio.h"
#include "gl.h"

#define MAX_TONES (500)
#define SAMPLE_RATE (48000)
#define PI (3.141592653589793f)
#define PI2 (2.0f * PI)
#define BASE_FREQ (220.0f)
#define N_PHASORS (12)

struct globals {
	SDL_Window* window;
	int true_screen_width;
	int true_screen_height;
	int screen_width;
	int screen_height;
	float pixel_ratio;
	NVGcontext* vg;
} g;

static void populate_screen_globals()
{
	int prev_width = g.true_screen_width;
	int prev_height = g.true_screen_height;
	SDL_GL_GetDrawableSize(g.window, &g.true_screen_width, &g.true_screen_height);
	int w, h;
	SDL_GetWindowSize(g.window, &w, &h);
	g.pixel_ratio = (float)g.true_screen_width / (float)w;
	g.screen_width = g.true_screen_width / g.pixel_ratio;
	g.screen_height = g.true_screen_height / g.pixel_ratio;
	if ((g.true_screen_width != prev_width || g.true_screen_height != prev_height)) {
#ifdef DEBUG
		printf("%d×%d -> %d×%d (r=%f)\n", prev_width, prev_height, g.screen_width, g.screen_height, g.pixel_ratio);
#endif
	}
}

NVGcontext* nanovg_create_context();


ma_device audio_device;


struct phasor {
	float acc;
	float add;
};

struct phasor phasors[N_PHASORS];

struct tone {
	int id;
	int note;
	int overtone;
};

#define SCOPE_SIZE (1<<12)
float scope[SCOPE_SIZE];
int scope_pointer = 0;

SDL_mutex* sound_mutex;
struct tone tones[MAX_TONES];

static void audio_callback(ma_device* device, void* poutput, const void* pinput, ma_uint32 n_frames)
{
	const float global_amp = 0.1f;
	const int n_channels = 1;
	float* p = poutput;
	SDL_LockMutex(sound_mutex);
	for (int i = 0; i < n_frames; i++) {
		float sample = 0.0f;
		for (int j = 0; j < MAX_TONES; j++) {
			struct tone* tone = &tones[j];
			if (tone->id == 0) continue;

			int phasor_index = tone->note % N_PHASORS;
			float acc = phasors[phasor_index].acc;
			float theta = acc * (float)(((tone->note/N_PHASORS)+1) * tone->overtone);
			float amp = ((tone->overtone & 1) ? (1.0f) : (-1.0f)) / (float)tone->overtone;
			sample += sinf(theta) * amp * global_amp;
		}

		scope[scope_pointer] = sample;
		scope_pointer = (scope_pointer+1) & (SCOPE_SIZE-1);
		for (int j = 0; j < n_channels; j++) {
			*(p++) = sample;
		}

		// update phasors
		for (int i = 0; i < N_PHASORS; i++) {
			struct phasor* phasor = &phasors[i];
			phasor->acc += phasor->add;
			while (phasor->acc > PI2) phasor->acc -= PI2;
		}
	}
	SDL_UnlockMutex(sound_mutex);

}

static void tone_toggle(int id, int note, int overtone)
{
	assert(id > 0);
	SDL_LockMutex(sound_mutex);
	int stopped = 0;
	for (int i = 0; i < MAX_TONES; i++) {
		struct tone* tone = &tones[i];
		if (tone->id == id) {
			tone->id = 0;
			stopped = 1;
		}
	}
	if (!stopped) {
		for (int i = 0; i < MAX_TONES; i++) {
			struct tone* tone = &tones[i];
			if (tone->id == 0) {
				tone->id = id;
				tone->note = note;
				tone->overtone = overtone;
				break;
			}
		}
	}
	SDL_UnlockMutex(sound_mutex);
}

static void tone_kill(int id)
{
	assert(id > 0);
	SDL_LockMutex(sound_mutex);
	for (int i = 0; i < MAX_TONES; i++) {
		struct tone* tone = &tones[i];
		if (tone->id == id) {
			tone->id = 0;
		}
	}
	SDL_UnlockMutex(sound_mutex);
}

static void tone_kill_all()
{
	SDL_LockMutex(sound_mutex);
	int stopped = 0;
	for (int i = 0; i < MAX_TONES; i++) {
		struct tone* tone = &tones[i];
		tone->id = 0;
	}
	SDL_UnlockMutex(sound_mutex);
}

static int tone_active(int id)
{
	for (int i = 0; i < MAX_TONES; i++) {
		struct tone* tone = &tones[i];
		if (tone->id == id) return 1;
	}
	return 0;
}


static void start_audio()
{
	for (int i = 0; i < N_PHASORS; i++) {
		struct phasor* phasor = &phasors[i];
		phasor->acc = 0;
		const float freq = BASE_FREQ * powf(2.0f, (float)i / (float)N_PHASORS);
		phasor->add = (freq / (float)SAMPLE_RATE) * PI2;
	}

	memset(tones, 0, sizeof tones);
	sound_mutex = SDL_CreateMutex();

	ma_device_config config = ma_device_config_init(ma_device_type_playback);
	config.playback.format   = ma_format_f32;
	config.playback.channels = 1;
	config.sampleRate        = SAMPLE_RATE;
	config.dataCallback      = audio_callback;
	config.pUserData         = NULL;

	if (ma_device_init(NULL, &config, &audio_device) != MA_SUCCESS) {
		assert(!"ma_device_init() failed");
	}

        ma_device_start(&audio_device);
}

static void stop_audio()
{
        ma_device_uninit(&audio_device);
}

int main(int argc, char** argv)
{
	assert(SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) == 0);
	atexit(SDL_Quit);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

	g.window = SDL_CreateWindow(
		"Overtones",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		1920, 1080,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
	assert(g.window != NULL);

	SDL_GLContext glctx = SDL_GL_CreateContext(g.window);
	assert(glctx);

	g.vg = nanovg_create_context();
	assert(g.vg != NULL);

	int font = nvgCreateFont(g.vg, "sans", "./nanovg/example/Roboto-Regular.ttf");
	assert(font != -1);

	start_audio();

	int exiting = 0;
	int fullscreen = 0;
	int iteration = 0;
	int mx = 0;
	int my = 0;
	int lastmy = 0;
	int leftdown = 0;
	int rightdown = 0;
	while (!exiting) {
		SDL_Event e;
		int click = 0;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				exiting = 1;
			} else if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_ESCAPE) {
					exiting = 1;
				} else if (e.key.keysym.sym == SDLK_f) {
					fullscreen = !fullscreen;
					SDL_SetWindowFullscreen(g.window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
				} else if (e.key.keysym.sym == SDLK_BACKSPACE) {
					tone_kill_all();
				}
			} else if (e.type == SDL_MOUSEMOTION) {
				mx = e.motion.x;
				my = e.motion.y;
			} else if (e.type == SDL_MOUSEBUTTONDOWN) {
				if (e.button.button == SDL_BUTTON_LEFT) {
					leftdown = 1;
				} else if (e.button.button == SDL_BUTTON_RIGHT) {
					rightdown = 1;
				}
				click = 1;
			} else if (e.type == SDL_MOUSEBUTTONUP) {
				if (e.button.button == SDL_BUTTON_LEFT) {
					leftdown = 0;
				} else if (e.button.button == SDL_BUTTON_RIGHT) {
					rightdown = 0;
				}
				click = 1;
			} else if (e.type == SDL_WINDOWEVENT) {
				if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
					populate_screen_globals();
				}
			}
		}

		glViewport(0, 0, g.true_screen_width, g.true_screen_height);
		glClearColor(0.3, 0.3, 0.3, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		nvgBeginFrame(g.vg, g.screen_width / g.pixel_ratio, g.screen_height / g.pixel_ratio, g.pixel_ratio);

		{
			nvgBeginPath(g.vg);
			SDL_LockMutex(sound_mutex);
			int sync = 0;
			int p = 0;
			for (int i = 0; i < SCOPE_SIZE; i++) {
				int ii = scope_pointer + i + SCOPE_SIZE/2;
				int ii0 = ii & (SCOPE_SIZE-1);
				int ii1 = (ii+1) & (SCOPE_SIZE-1);

				float v0 = scope[ii0];
				float v1 = scope[ii1];
				int p0 = v0 >= 0.0f;
				int p1 = v1 >= 0.0f;
				if (p1 && !p0 || i > (SCOPE_SIZE/4)) sync = 1;
				if (sync) {
					float x = p;
					if (x > g.screen_width) break;
					float y = g.screen_height/2 - v0*400;
					if (p == 0) {
						nvgMoveTo(g.vg, x, y);
					} else {
						nvgLineTo(g.vg, x, y);
					}
					p++;
				}
			}
			SDL_UnlockMutex(sound_mutex);
			nvgStrokeColor(g.vg, nvgRGBA(64,128,255,100));
			nvgStrokeWidth(g.vg, 2.0f);
			nvgStroke(g.vg);
		}

		nvgBeginPath(g.vg);
		nvgRect(g.vg, 0, my-1, g.screen_width, 2);
		nvgFillColor(g.vg, nvgRGB(255,0,0));
		nvgFill(g.vg);

		const int n_octaves = 6;
		const int steps_per_octave = 12;
		const int n_steps = n_octaves * steps_per_octave;
		const int wb[] = {1,0,1,0,1,1,0,1,0,1,0,1};
		const float y_scale = (float)g.screen_height / (float)n_steps;

		for (int i = 0; i < n_steps; i++) {
			int ii = i % steps_per_octave;
			int w = wb[ii];
			float y = g.screen_height - ((float)i * y_scale);
			nvgBeginPath(g.vg);
			nvgCircle(g.vg, 20, y, ii == 0 ? 6 : 4);
			nvgStrokeColor(g.vg, w ? nvgRGB(255,255,255) : nvgRGB(0,0,0));
			nvgStrokeWidth(g.vg, 2.0f);
			nvgStroke(g.vg);
		}

		char str[512];
		float myfreq = BASE_FREQ * powf(2.0f, (((float)g.screen_height-(float)my) / y_scale) / 12.0f);
		snprintf(str, sizeof str, "f0=%.1f", myfreq);
		nvgFontSize(g.vg, 20.0f);
		nvgTextAlign(g.vg, NVG_ALIGN_LEFT);
		nvgFillColor(g.vg, nvgRGB(0,0,0));
		nvgText(g.vg, 32, 32, str, NULL);
		nvgFillColor(g.vg, nvgRGB(255,50,50));
		nvgText(g.vg, 30, 30, str, NULL);


		const int C = 0;
		const int D = 2;
		const int E = 4;
		const int F = 5;
		const int G = 7;
		const int A = 9;
		const int B = 11;

		#if 0
		const int notes[] = {
			C,    E,   G,
			C,    F,   A,
			B-12, D,   F,   G,
			C,    G,   E,
		};
		#else
		const int notes[] = {
			C-12,
			D-12,
			E-12,
			F-12,
			G-12,
			A-12,
			B-12,
			C,
			D,
			E,
			F,
			G,
			A,
			B
		};
		#endif

		const int n_notes = sizeof(notes) / sizeof(*notes);

		const int n_overtones = 50;
		const float xstep = 80.0f;
		float x = 30;
		const float LOG2 = logf(2.0f);
		for (int i = 0; i < n_notes; i++) {
			int note = notes[i] + 12;

			float freq = BASE_FREQ * powf(2.0f, (float)note / 12.0f);
			for (int j = 0; j < n_overtones; j++) {
				int overtone = j+1;
				float ofreq = freq * (float)(overtone);
				if (ofreq > (SAMPLE_RATE/2)) continue;
				float noterel = (12.0f * logf(ofreq/BASE_FREQ)) / LOG2;
				float y = g.screen_height - ((float)noterel * y_scale);

				nvgBeginPath(g.vg);
				nvgRect(g.vg, x, y-1.5, xstep * 0.8, 3);
				int tone_id = 1 + (i * n_overtones) + overtone;
				int is_active = tone_active(tone_id);
				nvgFillColor(g.vg, nvgRGB(
					is_active ? 255 : 0,
					255/(overtone),
					is_active ? 255 : 0));
				nvgFill(g.vg);

				if ((leftdown || rightdown) && mx >= x && mx <= (x+xstep)) {
					if (leftdown && (click || lastmy != my)) {
						int my0 = my;
						int my1 = lastmy;
						if (my1 < my0) {
							int tmp = my0;
							my0 = my1;
							my1 = tmp;
						}
						int do_toggle = 0;
						do_toggle |= (click && (my0 <= (y+6) && (y-6) <= my1));
						do_toggle |= (!click && (my0 <= (y) && (y) < my1));
						if (do_toggle) {
							tone_toggle(tone_id, note, overtone);
						}
					}
					if (rightdown) {
						tone_kill(tone_id);
					}
				}
			}

			x += xstep;
		}

		nvgEndFrame(g.vg);

		SDL_GL_SwapWindow(g.window);

		iteration++;

		lastmy = my;
	}

	stop_audio();

	SDL_GL_DeleteContext(glctx);
	SDL_DestroyWindow(g.window);

	return EXIT_SUCCESS;
}
