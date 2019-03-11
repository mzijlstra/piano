#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

typedef enum WaveForm {
    square,
    triangle,
    saw,
    noise,
    sine,
    opl2_1,
    opl2_2,
    opl2_3 
} WaveForm;

typedef struct Key {
    char tone[4];   // name of tone, eg A4
    double freq;    // frequency of tone, eg 440hz
    char key;       // keyboard scan code
    SDL_Rect rect;  // rectangle on screen
    bool on;        // currently playing
    double wave_part;// place to store wave position between frames
} Key;

typedef struct Keys {
    Key *white;     // pointer to array of 'white' keys
    int w_len;      // how many white keys there are
    Key *black;     // pointer to array of 'black' keys
    int b_len;      // how many black keys there are
} Keys;

static SDL_AudioSpec have;

// TODO create a .ini config file (can read using lib iniParser) 
static WaveForm wave = square;
static Sint8 volume = 10;
static double A4 = 432;

/* initializes arrays of black and white struct Key, starting from the key
 * indicated by s_key in s_octave, ending on e_key in e_octave
 *   alwas start and end with a white key
 */

bool keyToTone(Key* keys, int len, char *s, char c) {
    for (int i = 0; i < len; i++) {
        if (strncmp(keys[i].tone, s, 3) == 0) {
            keys[i].key = c;
            return true;
        }
    }
    return false;
}

void setupKeys(Keys* keys,
        char s_key, int s_octave, char e_key, int e_octave) {
    extern double A4;

    Key* white = keys->white;
    Key* black = keys->black;

    // start by makeing sure the everything is zero
    SDL_memset(white, 0, sizeof(Key) * keys->w_len);
    SDL_memset(black, 0, sizeof(Key) * keys->b_len);

    // generate tonal frequencies based on our A4 val
    double tone = A4;
    double tones[12];
    double next_octave = A4 * 2;
    for (int i = 0; i < 12; i++) {
        tones[i] = tone;
        tone = tone / 2 * 3;
        if (tone > next_octave) {
            tone = tone / 2;
        }
    }
    // sort them ascending
    double sorted[12];
    for (int i = 0; i < 12; i++) {
        double smallest = 100000;
        int small_pos = -1;
        for (int j = 0; j < 12; j++) {
            if (tones[j] < smallest) {
                smallest = tones[j];
                small_pos = j;
            }
        }
        sorted[i] = tones[small_pos];
        tones[small_pos] = 100000;
    }
    // go to requested start octave (cannot be > 4)
    int octave = 4;
    while (octave > s_octave) {
        for (int i = 0; i < 12; i++) {
            sorted[i] /= 2;
        }
        octave--;
    }
    // go to requested start key
    char bw[] = {'w','b','w','w','b','w','b','w','w','b','w','b'};
    int index = 0;
    char k = 'A';
    while (k < s_key) {
        index++;
        if (bw[index] == 'b') {
            index++;
        }
        k++;
    }
    // setup keys of the first (requested) octave
    int wi = 0;
    int bi = 0;
    while (index < 12) {
        if (bw[index] == 'w') {
            white[wi].tone[0] = k;
            white[wi].tone[1] = 0x30 + octave;
            white[wi].freq = sorted[index];
            wi++;
        } else { // 'b'
            black[bi].tone[0] = k;
            black[bi].tone[1] = 0x30 + octave;
            black[bi].tone[2] = '#';
            black[bi].freq = sorted[index];
            bi++;
        }
        if (index < 11 && bw[index + 1] == 'w') {
            k++;
        }
        index++;
    }
    octave++;
    for (int i = 0; i < 12; i++) {
        sorted[i] *= 2;
    }
    // do all the in-between octaves
    while (octave < e_octave) {
        k = 'A';
        for (index = 0; index < 12; index++) {
            if (bw[index] == 'w') {
                white[wi].tone[0] = k;
                white[wi].tone[1] = 0x30 + octave;
                white[wi].freq = sorted[index];
                wi++;
            } else { // 'b'
                black[bi].tone[0] = k;
                black[bi].tone[1] = 0x30 + octave;
                black[bi].tone[2] = '#';
                black[bi].freq = sorted[index];
                bi++;
            }
            if (index < 11 && bw[index + 1] == 'w') {
                k++;
            }
        }
        octave++;
        for (int i = 0; i < 12; i++) {
            sorted[i] *= 2;
        }
    }
    // then the last octave
    index = 0;
    k = 'A';
    while (k <= e_key && wi < keys->w_len) {
        if (bw[index] == 'w') {
            white[wi].tone[0] = k;
            white[wi].tone[1] = 0x30 + octave;
            white[wi].freq = sorted[index];
            wi++;
        } else { // 'b'
            black[bi].tone[0] = k;
            black[bi].tone[1] = 0x30 + octave;
            black[bi].tone[2] = '#';
            black[bi].freq = sorted[index];
            bi++;
        }
        if (index < 11 && bw[index + 1] == 'w') {
            k++;
        }
        index++;
    }

    // TODO these bindings should probably be read from a config file
    keyToTone(white, 36, "C2", '1');
    keyToTone(white, 36, "D2", '2');
    keyToTone(white, 36, "E2", '3');
    keyToTone(white, 36, "F2", '4');
    keyToTone(white, 36, "G2", '5');
    keyToTone(white, 36, "A3", '6');
    keyToTone(white, 36, "B3", '7');
    keyToTone(white, 36, "C3", '8');
    keyToTone(white, 36, "D3", '9');
    keyToTone(white, 36, "E3", '0');
    keyToTone(white, 36, "F3", 'q');
    keyToTone(white, 36, "G3", 'w');
    keyToTone(white, 36, "A4", 'e');
    keyToTone(white, 36, "B4", 'r');
    keyToTone(white, 36, "C4", 't');
    keyToTone(white, 36, "D4", 'y');
    keyToTone(white, 36, "E4", 'u');
    keyToTone(white, 36, "F4", 'i');
    keyToTone(white, 36, "G4", 'o');
    keyToTone(white, 36, "A5", 'p');
    keyToTone(white, 36, "B5", 'a');
    keyToTone(white, 36, "C5", 's');
    keyToTone(white, 36, "D5", 'd');
    keyToTone(white, 36, "E5", 'f');
    keyToTone(white, 36, "F5", 'g');
    keyToTone(white, 36, "G5", 'h');
    keyToTone(white, 36, "A6", 'j');
    keyToTone(white, 36, "B6", 'k');
    keyToTone(white, 36, "C6", 'l');
    keyToTone(white, 36, "D6", 'z');
    keyToTone(white, 36, "E6", 'x');
    keyToTone(white, 36, "F6", 'c');
    keyToTone(white, 36, "G6", 'v');
    keyToTone(white, 36, "A7", 'b');
    keyToTone(white, 36, "B7", 'n');
    keyToTone(white, 36, "C7", 'm');

    keyToTone(black, 25, "C2#", '1');
    keyToTone(black, 25, "D2#", '2');
    keyToTone(black, 25, "F2#", '4');
    keyToTone(black, 25, "G2#", '5');
    keyToTone(black, 25, "A3#", '6');
    keyToTone(black, 25, "C3#", '8');
    keyToTone(black, 25, "D3#", '9');
    keyToTone(black, 25, "F3#", 'q');
    keyToTone(black, 25, "G3#", 'w');
    keyToTone(black, 25, "A4#", 'e');
    keyToTone(black, 25, "C4#", 't');
    keyToTone(black, 25, "D4#", 'y');
    keyToTone(black, 25, "F4#", 'i');
    keyToTone(black, 25, "G4#", 'o');
    keyToTone(black, 25, "A5#", 'p');
    keyToTone(black, 25, "C5#", 's');
    keyToTone(black, 25, "D5#", 'd');
    keyToTone(black, 25, "F5#", 'g');
    keyToTone(black, 25, "G5#", 'h');
    keyToTone(black, 25, "A6#", 'j');
    keyToTone(black, 25, "C6#", 'l');
    keyToTone(black, 25, "D6#", 'z');
    keyToTone(black, 25, "F6#", 'c');
    keyToTone(black, 25, "G6#", 'v');
    keyToTone(black, 25, "A7#", 'b');
}

int addFrequencies(Sint32 *audio, int alen, Key* keys, int klen) {
    extern WaveForm wave;
    extern SDL_AudioSpec have; // only works if we actually have Sint8
    extern Sint8 volume;

    // find which keys are pressed and mix in their frequencies
    int pressed = 0;
    for (int k = 0; k < klen; k++) {
        if (keys[k].on && keys[k].freq) {
            pressed += 1;

            double tone = have.freq / keys[k].freq;
            double part = 0;
            if (wave == square) {
                double half = tone / 2;
                for ( int i = 0; i < alen; i++ ) {
                    part = fmod(keys[k].wave_part + i, tone);
                    if ( part < half ) {
                        audio[i] -= volume;
                    } else {
                        audio[i] += volume;
                    }
                }
            } else if (wave == triangle) {
                double qtone = tone * 0.35;
                double htone = tone * 0.5;
                double ttone = tone * 0.75;
                for (int i = 0; i < alen; i++) {
                    part = fmod(keys[k].wave_part + i, tone);
                    if (part <= qtone) {
                        audio[i] = (part / qtone) * volume;
                    } else if (part <= htone) {
                        audio[i] = (1 - (part - qtone) / qtone) * volume; 
                    } else if (part <= ttone) {
                        audio[i] = (0 - (part - htone) / qtone) * volume;
                    } else { // the last quarter of the 'wave'
                        audio[i] = (-1 + (part - ttone) / qtone) * volume;
                    }
                }
            } else if (wave == saw) {
                for (int i = 0; i < alen; i++) {
                    part = fmod(keys[k].wave_part + i, tone);
                    audio[i] = (-1.0 + part/tone*2) * volume;
                }
            } else if (wave == noise) {
                for (int i = 0; i < alen; i++) {
                    audio[i] = ((random() % 20) - 10) * volume; 
                }
            } else if (wave == sine) {
                double sine_tone = tone / (2 * M_PI);
                for (int i = 0; i < alen; i++) {
                    part = fmod(keys[k].wave_part + i, tone);
                    audio[i] = sin(part / sine_tone) * volume;
                }
            } else if (wave == opl2_1) {
                double sine_tone = tone / (2 * M_PI);
                double htone = tone * 0.5;
                for (int i = 0; i < alen; i++) {
                    part = fmod(keys[k].wave_part + i, tone);
                    if (part <= htone) {
                        audio[i] = sin(part / sine_tone) * volume;
                    } else {
                        audio[i] = 0;
                    }
                }
            } else if (wave == opl2_2) {
                double sine_tone = tone / (2 * M_PI);
                double htone = tone * 0.5;
                for (int i = 0; i < alen; i++) {
                    part = fmod(keys[k].wave_part + i, tone);
                    if (part <= htone) {
                        audio[i] = sin(part / sine_tone) * volume;
                    } else {
                        audio[i] = -sin(part / sine_tone) * volume;
                    }
                }
            } else if (wave == opl2_3) {
                double sine_tone = tone / (2 * M_PI);
                double qtone = tone * 0.35;
                double htone = tone * 0.5;
                double ttone = tone * 0.75;
                for (int i = 0; i < alen; i++) {
                    part = fmod(keys[k].wave_part + i, tone);
                    if (part <= qtone) {
                        audio[i] = sin(part / sine_tone) * volume;
                    } else if (part <= htone) {
                        audio[i] = 0;
                    } else if (part <= ttone) {
                        audio[i] = -sin(part / sine_tone) * volume;
                    } else { // the last quarter of the 'wave'
                        audio[i] = 0;
                    }
                }
            }

            // store where we are in the wave, so that we can continue there
            // when generating the next sample
            keys[k].wave_part = part;
        }
    }
    return pressed;
}

void AudioCallback(void *userdata, Uint8 *stream, int len){
    Keys *keys = (Keys*)userdata;

    // first ensure silence in the stream
    SDL_memset(stream, have.silence, len);

    // create an audio stream to contain all pressed keys in one
    Sint32 *audio = SDL_malloc(sizeof(Sint32) * len);
    SDL_memset(audio, 0, sizeof(Sint32) * len);

    int pressed = 0;
    pressed += addFrequencies(audio, len, keys->white, keys->w_len);
    pressed += addFrequencies(audio, len, keys->black, keys->b_len);

    // normalize our audio into the stream
    if (pressed) {
        for (int i = 0; i < len; i++) {
            int val = audio[i] / pressed;
            if (val > 127) {
                val = 127;
            } else if (val < -128) {
                val = -128;
            }
            stream[i] = val;
        }
    }
    SDL_free(audio);
}

int main() {
    extern WaveForm wave;
    // create our keys data structures
    Key white[36];
    Key black[25];
    Keys keys;
    keys.white = white;
    keys.w_len = 36;
    keys.black = black;
    keys.b_len = 25;
    setupKeys(&keys, 'C', 2, 'C', 7);

    // we cannot make this a console only application because there are 
    // no keyboard events unless we have video initialized
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("Could not init SDL: %s\n", SDL_GetError());
        return 1;
    }
    atexit(SDL_Quit);

    // Create an application window showing a piano image
    SDL_Window *window;
    SDL_Renderer *renderer;
        
    if(SDL_CreateWindowAndRenderer(1296, 220, 0, &window, &renderer)) {
        printf("Could not create window and renderer: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Rect white_keys[36];
    for (int i =0; i<36; i++) {
        white_keys[i].x = i * 36;
        white_keys[i].y = 0;
        white_keys[i].w = 36;
        white_keys[i].h = 220;
    }
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderFillRects(renderer, white_keys, 36);
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xaa);
    SDL_RenderDrawRects(renderer, white_keys, 36);

    SDL_Rect black_keys[25];
    int dist = 24;
    for (int i = 0; i < 25; i++) {
        black_keys[i].x = dist + i * 36;
        black_keys[i].y = 0;
        black_keys[i].w = 24;
        black_keys[i].h = 140;
        if (i % 5 == 1 || i % 5 == 4) {
            dist += 36;
        }
    }
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xaa);
    SDL_RenderFillRects(renderer, black_keys, 25);
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderDrawRects(renderer, black_keys, 25);

    SDL_RenderPresent(renderer);


    // setup audio
    SDL_AudioSpec want;
    SDL_AudioDeviceID dev;

    SDL_memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_S8;
    want.channels = 1;
    want.samples = 1024;
    want.callback = AudioCallback;
    want.userdata = &keys;
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
        int mods = 0;
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
                } else if (key == SDLK_F1) {
                    wave = square;
                } else if (key == SDLK_F2) {
                    wave = triangle;
                } else if (key == SDLK_F3) {
                    wave = saw;
                } else if (key == SDLK_F4) {
                    wave = noise;
                } else if (key == SDLK_F5) {
                    wave = sine;
                } else if (key == SDLK_F6) {
                    wave = opl2_1;
                } else if (key == SDLK_F7) {
                    wave = opl2_2;
                } else if (key == SDLK_F8) {
                    wave = opl2_3;
                } else if (key == SDLK_MINUS) {
                    volume -= 1;
                    if (volume < 1) {
                        volume = 1;
                    }
                } else if (key == SDLK_EQUALS) {
                    volume += 1;
                    if (volume < 0) {
                        volume = 127;
                    }
                } else if (key >= 0 && key <= 127) {
                    mods = SDL_GetModState();
                    if (mods & KMOD_SHIFT) {
                        for (int i = 0; i < keys.b_len; i++) {
                            if (keys.black[i].key == key) {
                                keys.black[i].on = true;
                                break;
                            }
                        }
                    } else {
                        for (int i = 0; i < keys.w_len; i++) {
                            if (keys.white[i].key == key) {
                                keys.white[i].on = true;
                                break;
                            }
                        }
                    }
                }
                break;
            case SDL_KEYUP:
                key = event.key.keysym.sym;
                if (key >= 0 && key <= 127) {
                    mods = SDL_GetModState();
                    if (mods & KMOD_SHIFT) {
                        for (int i = 0; i < keys.b_len; i++) {
                            if (keys.black[i].key == key) {
                                keys.black[i].on = false;
                                break;
                            }
                        }
                    } else {
                        for (int i = 0; i < keys.w_len; i++) {
                            if (keys.white[i].key == key) {
                                keys.white[i].on = false;
                                break;
                            }
                        }
                    }
                }
                break;
            default:
                break;
        }
    }

done: // cleanup
    SDL_CloseAudioDevice(dev);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}
