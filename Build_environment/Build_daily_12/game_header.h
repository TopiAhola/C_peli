#if !defined(GAME_H) //This prevents defining this file twice in another file.




//TYPES

//Regular bool is 1 byte
typedef unsigned long int bool32;

typedef signed char int8;
typedef signed short int int16;
typedef signed int int32;          
typedef signed long long int int64;

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long int uint64;

typedef float float32;
typedef double float64;
typedef long double float128;

//CONSTANTS

float32 pi32 = 3.14159265f;


//STRUCTS

struct game_backbuffer{
    void * p_memory;
    int width;
    int height;
    int bytes_per_pixel;
    int pitch;
};

struct game_soundbuffer {    
    uint32 sample_rate = 48000;
    uint8 bytes_per_sample = 4;
    uint32 size = 192000;
    int16 * memory_p;

};


//FUNCTIONS

//time (for variable rate rendering), inputs, pointer to bitmap and sound buffer
static void game_update_and_render(game_backbuffer * bitmap, game_soundbuffer * soundbuffer, uint32 samples_used, int x_input, int y_input);


#define GAME_H
#endif
