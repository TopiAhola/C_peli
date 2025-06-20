#if !defined(GAME_H) //This prevents defining this file twice in another file.


////////////////////////////////////////////////////////////////
//MACROS

#define kilobytes(value) ((int64)(value * 1024))
#define megabytes(value) ((int64)kilobytes(value) * 1024)
#define gigabytes(value) ((int64)megabytes(value) * 1024)


////////////////////////////////////////////////////////////////
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


//Better names for static
#define local_static static     //static is scope dependent
#define global_static static    // for global statics
#define internal static         //static has 3rd meaning with functions limiting use of their name to one translation unit?


////////////////////////////////////////////////////////////////
//CONSTANTS

float32 pi32 = 3.14159265f;


////////////////////////////////////////////////////////////////
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


struct keypress {
    uint32 time;
    char key;
    bool was_down;
};


//Gamepad TODO: This should use timestamps or curves for analog
struct pad_input {
    bool up;   
    bool down;
    bool left;
    bool right;
    bool start;
    bool back;
    bool left_shoulder;
    bool right_shoulder;
    bool a;
    bool b;
    bool x;
    bool y;          
    
    //Sticks    
    int16 LX;   
    int16 LY;   
    int16 RX;
    int16 RY; 
    
    //Triggers
    uint8 left_trigger; 
    uint8 right_trigger;
} ;


struct game_input {
    //TODO: Use arrays with timestamps for inputs

    //for testing
    int32 x_input;
    int32 y_input; 

    //Keyboard
    keypress kb[1000];

    //All gamepads in array
    pad_input pad[4];
    
};


struct memory_pool {
    uint64 base_memory_size; 
    uint8 * base_memory;

    uint64 temp_memory_size;
    uint8 * temp_memory;
};

struct read_file_result {
    uint64 size;
    void* memory;
};



////////////////////////////////////////////////////////////////
//DEBUGGING

#if DEVELOPER_BUILD       //SET DEVELOPER_BUILD=1 IN COMPILER ARGUMENTS
#define assert(expression) if(!(expression)){ *((int*) 0) = 0;}
internal bool platform_debug_create_file_debug(char* filename);
internal bool platform_debug_write_file(char* filename, uint64 size, void * memory);
internal read_file_result platform_debug_read_file(char* filename);
internal bool32 platform_debug_free_file_memory(void * file_p );


#else
#define assert(expression)

#endif


#if PERFORMANCE_MODE       //SET PERFORMANCE_MODE=1 IN COMPILER ARGUMENTS
#define test_function(error) {} //fast version of function

#else
#define test_function(error)    //slow version of function

#endif



////////////////////////////////////////////////////////////////
//FUNCTIONS

//time (for variable rate rendering), inputs, pointer to bitmap and sound buffer
static void game_update_and_render(memory_pool * game_memory, game_backbuffer * bitmap, game_soundbuffer * soundbuffer, uint32 samples_used, game_input input);



internal uint32
truncate_uint64(uint64 large_integer){
    assert(large_integer < 0xffffffff);
    uint32 truncated_int;
    truncated_int = (uint32)large_integer;  
    return truncated_int;
}





#define GAME_H
#endif
