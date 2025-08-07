#if !defined(GAME_HEADER) //This prevents defining this file twice in another file.


//Temporary stuff for testing:



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

//BUFFERS

struct game_backbuffer{
    void * p_memory;
    int width;
    int height;
    int bytes_per_pixel;
    int pitch;
};


struct game_soundbuffer {  
    int16 * memory_p;  
    uint32 sample_rate = 48000;    
    uint32 size = 192000;   
    uint32 last_write_sample_index;    
    uint32 bytes_per_sample = 4;
};

//SOUND




//TODO: remove this, sound output should be function of time not samples
struct sound_sample_counter {
    uint32 samples_used;
    uint32 samples_used_maximum;  
    uint32 samples_used_total;
    uint32 samples_used_average;

};



//INPUTS

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


//Key status at the end of logging period 
struct keypress { 
    uint16 transition_count;    
    bool started_down;
    bool ended_down;
};

//Simple input structure
struct game_input {

    //Keyboard 
    keypress up;
    keypress down;
    keypress left;
    keypress right;
    keypress w;
    keypress a;
    keypress s;
    keypress d;
    keypress q;
    keypress e;

    //All gamepads in array
    pad_input pad[4];
};  


//Keypress events logged  with timestamps for higher physics framerate
struct keypress_event {
    uint32 time;
    char key;
    bool was_down;
    bool is_down;
};

//Event based input structute TODO: Gamepad input is once per frame only
struct game_input_events {

    //Keyboard
    int32 x_amount;
    int32 y_amount;
    int32 other_amount;    
    keypress_event x_axis[1000];
    keypress_event y_axis[1000];
    keypress_event other[1000];    

    //All gamepads in array
    pad_input pad[4];
    
};


//MEMORY

struct memory_pool {
    uint64 base_memory_size; 
    uint8 * base_memory;

    uint64 temp_memory_size;
    uint8 * temp_memory;
};


//FILE IO

struct read_file_result {
    uint64 size;
    void* memory;
};



////////////////////////////////////////////////////////////////
//DEBUGGING

#if DEVELOPER_BUILD    //SET DEVELOPER_BUILD=1 IN COMPILER ARGUMENTS
#define assert(expression) if(!(expression)){ *((int*) 0) = 0;}



#else
#define assert(expression)

#endif


#if PERFORMANCE_MODE       //SET PERFORMANCE_MODE=1 IN COMPILER ARGUMENTS
#define test_function(error) {} //fast version of function

#else
#define test_function(error)    //slow version of function

#endif

//TODO: These functions should only be defined in DEVELOPER_BUILD
internal bool platform_debug_create_file_debug(char* filename);
internal bool platform_debug_write_file(char* filename, uint64 size, void * memory);
internal read_file_result platform_debug_read_file(char* filename);
internal bool32 platform_debug_free_file_memory(void * file_p );

////////////////////////////////////////////////////////////////
//FUNCTIONS

//time (for variable rate rendering), inputs, pointer to bitmap and sound buffer
static void game_update_and_render(memory_pool * game_memory, game_backbuffer * bitmap, game_soundbuffer * soundbuffer, sound_sample_counter samples_used, game_input input);




internal uint32
truncate_uint64(uint64 large_integer){
    assert(large_integer < 0xffffffff);
    uint32 truncated_int;
    truncated_int = (uint32)large_integer;  
    return truncated_int;
}





#define GAME_HEADER
#endif
