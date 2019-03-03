#include <unistd.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

typedef struct Key {
    bool on;
    double freq;
    double wave_part;
} Key;

static SDL_AudioSpec have;

void setupKeys(Key* keys) {
    // make sure the values in keys start at zero;
    SDL_memset(keys, 0, sizeof(Key) * 256);

    keys['1'].freq =            64.22;
    keys['1' + 128].freq =      68.04;
    keys['2'].freq =            72.08;
    keys['2' + 128].freq =      76.37;
    keys['3'].freq =            80.91;
    keys['4'].freq =            85.72;
    keys['4' + 128].freq =      90.82;
    keys['5'].freq =            96.22;
    keys['5' + 128].freq =     101.94;
    keys['6'].freq =           108.00;
    keys['6' + 128].freq =     114.42;
    keys['7'].freq =           121.23;
    keys['8'].freq =           128.43;
    keys['8' + 128].freq =     136.07;
    keys['9'].freq =           144.16;
    keys['9' + 128].freq =     152.74;
    keys['0'].freq =           161.82;
    keys['q'].freq =           171.44;
    keys['q' + 128].freq =     181.63;
    keys['w'].freq =           192.43;
    keys['w' + 128].freq =     203.88;
    keys['e'].freq =           216.00;
    keys['e' + 128].freq =     228.84;
    keys['r'].freq =           242.45;
    keys['t'].freq =           256.87;
    keys['t' + 128].freq =     272.14;
    keys['y'].freq =           288.33;
    keys['y' + 128].freq =     305.47;
    keys['u'].freq =           323.64;
    keys['i'].freq =           342.88;
    keys['i' + 128].freq =     363.27;
    keys['o'].freq =           384.87;
    keys['o' + 128].freq =     407.75;
    keys['p'].freq =           432.00;
    keys['p' + 128].freq =     457.69;
    keys['a'].freq =           484.90;
    keys['s'].freq =           513.74;
    keys['s' + 128].freq =     544.29;
    keys['d'].freq =           576.65;
    keys['d' + 128].freq =     610.94;
    keys['f'].freq =           647.27;
    keys['g'].freq =           685.76;
    keys['g'+ 128].freq =      726.53;
    keys['h'].freq =           769.74;
    keys['h'+ 128].freq =      815.51;
    keys['j'].freq =           864.00;
    keys['j'+ 128].freq =      915.38;
    keys['k'].freq =           969.81;
    keys['l'].freq =          1027.47;
    keys['l'+ 128].freq =     1088.57;
    keys['z'].freq =          1153.30;
    keys['z'+ 128].freq =     1221.88;
    keys['x'].freq =          1294.54;
    keys['c'].freq =          1371.51;
    keys['c' + 128].freq =    1453.07;
    keys['v'].freq =          1539.47;
    keys['v' + 128].freq =    1631.01;
    keys['b'].freq =          1728.00;
    keys['b' + 128].freq =    1830.75;
    keys['n'].freq =          1939.61;
    keys['m'].freq =          2054.95;

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
    SDL_Window *window;
    SDL_Renderer *renderer;
        
    if(SDL_CreateWindowAndRenderer(1300, 220, 0, &window, &renderer)) {
        printf("Could not create window and renderer: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Surface *image = IMG_Load("piano.png");
    if (image == NULL){
        printf("Failed to load image\n");
        printf("SDL Says: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, image);
    if (texture == NULL){
        printf("Failed to create texture: %s\n", SDL_GetError());
        return 1;
    }
    SDL_FreeSurface(image);

    // show image
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);


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
        int mods;
        switch(event.type) {
            case SDL_QUIT:
                goto done;
                break;
            case SDL_KEYDOWN:
                if (event.key.repeat) {
                    break;
                }
                key = event.key.keysym.sym;
                if (key == SDLK_ESCAPE) {
                    goto done;
                } else if (key >= 0 && key <= 127) {
                    mods = SDL_GetModState();
                    if (mods & KMOD_SHIFT) {
                        key += 128;
                    }
                    keys[key].on = true;
                }
                break;
            case SDL_KEYUP:
                key = event.key.keysym.sym;
                if (key >= 0 && key <= 127) {
                    mods = SDL_GetModState();
                    if (mods & KMOD_SHIFT) {
                        key += 128;
                    }
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
