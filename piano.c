#include <SDL2/SDL.h>

#define FREQ 44100.0

static double wave_part = 0;
static SDL_AudioSpec have;

void AudioCallback(void *userdata, Uint8 *stream, int len){
    extern SDL_AudioSpec have;
    // first create silence in the stream
    SDL_memset(stream, have.silence, len);
    int *keypressed = (int*)userdata;

    if (*keypressed) {
        // create a buffer for our generated audio
        Uint8* audio = SDL_malloc(sizeof(Uint8) * len);

        // generate our audio wave
        double tone = FREQ / 432.0;
        double half = tone / 2;
        double part = 0;
        for ( int i = 0; i < len; i++ ) {
            part = fmod(wave_part + i, tone);
            if ( part < half ) {
                audio[i] = 0;
            } else {
                audio[i] = 255;
            }
        }
        wave_part = part;

        // mix the wave into the stream
        SDL_MixAudioFormat(stream, audio, have.format, len, SDL_MIX_MAXVOLUME);
        SDL_free(audio);
    }
}

int main() {

    // No keyboard events unless we have video initialized
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("Could not init SDL: %s\n", SDL_GetError());
        return 1;
    }
    atexit(SDL_Quit);

    // Create an application window with the following settings:
    SDL_Window *window = SDL_CreateWindow(
            "Piano",                           // window title
            SDL_WINDOWPOS_UNDEFINED,           // initial x position
            SDL_WINDOWPOS_UNDEFINED,           // initial y position
            640,                               // width, in pixels
            480,                               // height, in pixels
            SDL_WINDOW_OPENGL                  // flags - see below
            );

    // Check that the window was successfully created
    if (window == NULL) {
        printf("Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    // setup audio
    int on = 0;
    SDL_AudioSpec want;
    SDL_AudioDeviceID dev;

    SDL_memset(&want, 0, sizeof(want));
    want.freq = FREQ;
    want.format = AUDIO_U8;
    want.channels = 1;
    want.samples = 1024;
    want.callback = AudioCallback;
    want.userdata = &on;
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_ANY_CHANGE);

    if (dev == 0) {
        SDL_Log("Failed to open audio: %s", SDL_GetError());
        return 1;
    } 

    if (have.format != want.format) { 
        SDL_Log("We didn't get requested audio format.");
    }

    SDL_PauseAudioDevice(dev, 0); /* start audio playing. */

    // setup main loop
    int fps = 60;
    int frametime = 1000 / fps; 
    int starttime, exectime;
    while (1) {
        starttime = SDL_GetTicks();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT:
                    goto done;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        goto done;
                    } else if (event.key.keysym.sym == SDLK_SPACE) {
                        on = 1;
                    }
                    break;
                case SDL_KEYUP:
                    if (event.key.keysym.sym == SDLK_SPACE) {
                        on = 0;
                    }
                    break;
                default:
                    break;
            }
        }
        exectime = SDL_GetTicks() - starttime;
        if (exectime < frametime) {
            SDL_Delay(frametime - exectime);
        }
    }

done: // cleanup
    SDL_CloseAudioDevice(dev);
    SDL_DestroyWindow(window);

    return 0;
}
