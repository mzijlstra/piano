#include <stdbool.h>
#include <SDL2/SDL.h>

typedef struct Key {
    bool on;
    double freq;
    double wave_part;
} Key;

static SDL_AudioSpec have;

void setupKeys(Key* keys) {
    // make sure the values in keys start at zero;
    SDL_memset(keys, 0, sizeof(Key) * 256);

    keys['a'].freq = 432.0;
    keys['s'].freq = 484.9; 
    keys['d'].freq = 513.74;
    keys['f'].freq = 576.65;
    keys['g'].freq = 647.27;
    keys['h'].freq = 685.76;
    keys['j'].freq = 769.74;
    keys['k'].freq = 864.00;
    keys['l'].freq = 969.81;
    keys[';'].freq = 1027.47;
    keys['\''].freq = 1153.3;

}

void AudioCallback(void *userdata, Uint8 *stream, int len){
    extern SDL_AudioSpec have; // only works if we actually have Uint8
    Key *keys = (Key*)userdata;

    // first ensure silence in the stream
    SDL_memset(stream, have.silence, len);

    // create an audio stream to contain all pressed keys in one
    Uint32 *audio = SDL_malloc(sizeof(Uint32) * len);
    SDL_memset(audio, 0, sizeof(Uint32) * len);

    // find which keys are pressed and mix in their frequencies
    int pressed = 0;
    for (int k = 0; k < 255; k++) {
        if (keys[k].on && keys[k].freq) {
            pressed += 1;

            // generate square wave into audio
            double tone = have.freq / keys[k].freq;
            double half = tone / 2;
            double part = 0;
            for ( int i = 0; i < len; i++ ) {
                part = fmod(keys[k].wave_part + i, tone);
                if ( part < half ) {
                    audio[i] += 0;
                } else {
                    audio[i] += 255;
                }
            }
            // store where we are in the wave, so that we can continue there
            // when generating the next sample
            keys[k].wave_part = part;
        }
    }

    // normalize our audio into the stream
    if (pressed) {
        for (int i = 0; i < len; i++) {
            stream[i] = audio[i] / pressed;
        }
    }
    SDL_free(audio);
}

int main() {
    // create our keys data structure
    Key keys[256];
    setupKeys(keys);

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
    SDL_AudioSpec want;
    SDL_AudioDeviceID dev;

    SDL_memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_U8;
    want.channels = 1;
    want.samples = 1024;
    want.callback = AudioCallback;
    want.userdata = keys;
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);

    if (dev == 0) {
        SDL_Log("Failed to open audio: %s", SDL_GetError());
        return 1;
    } 

    if (have.format != want.format) { 
        SDL_Log("We didn't get requested audio format.");
    }

    SDL_PauseAudioDevice(dev, 0); /* start audio playing. */

    // setup event driven main loop
    SDL_Event event;
    while (SDL_WaitEvent(&event)) {
        int key = 0;
        switch(event.type) {
            case SDL_QUIT:
                goto done;
                break;
            case SDL_KEYDOWN:
                key = event.key.keysym.sym;
                if (key == SDLK_ESCAPE) {
                    goto done;
                } else if (key >= 0 && key <= 127) {
                    keys[key].on = true;
                }
                break;
            case SDL_KEYUP:
                key = event.key.keysym.sym;
                if (key >= 0 && key <= 127) {
                    keys[key].on = false;
                }
                break;
            default:
                break;
        }
    }

done: // cleanup
    SDL_CloseAudioDevice(dev);
    SDL_DestroyWindow(window);

    return 0;
}
