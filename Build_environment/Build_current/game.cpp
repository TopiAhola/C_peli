////////////////////////////////////////////////////////////////
//Includes

#include <C:\Users\valas\VSCProjects\C_peli\Build_environment\Build_current\game_header.h>
#include <cmath> //for sin

//Temp includes for debugging:
//#include <windows.h>
//#include <dsound.h>


////////////////////////////////////////////////////////////////
//DEFINES
#define local_static static     //2 names for static because static is scope dependent
#define global_static static    // for global statics
#define internal static         //static has 3rd meaning with functions limiting use of their name?


////////////////////////////////////////////////////////////////
//SOUND

//Writes test sound to given sound buffer 
internal void sound_test(game_soundbuffer * soundbuffer, uint32 samples_used, int32 x_input, int32 y_input){  
    //TODO: This crashes with negative or overflowing frequency       
    //Continous sine wave
    local_static int16 amplitude = 1000;
    local_static uint32 freq = 100;     
    amplitude += (10 * x_input );
    freq += y_input;
    if(freq <= 0){freq = 1;} 

    uint32 sample_counter = samples_used;  //Wave is genereted from the point it has been copied to playing buffer
    uint32 period = (soundbuffer->sample_rate / freq);

     //Write to buffer memory
    int16 value;
    int16 * channel_p = soundbuffer->memory_p;  
    for(int sample = 0; sample < ( soundbuffer->size / soundbuffer->bytes_per_sample ); sample++){
        
        //Sine wave function value
        float32 phase = sinf( (2.0f * pi32 * ( (float32)sample_counter) / ((float32)period) ) );
        value = (int16)(amplitude * phase) ;

        *channel_p = value;  //left channel 
        channel_p++;
        *channel_p = value;  //right channel
        channel_p++;
        sample_counter++;        
    }
}

////////////////////////////////////////////////////////////////
//VIDEO


/* Struct defined in header and passed to platform code
struct game_backbuffer{
    void * p_memory;
    int width;
    int height;
    int bytes_per_pixel;
    int pitch;
};
*/

//Takes a pointer to a bitmap. Draws something to the given bitmap.  
internal void
draw_test(game_backbuffer * bitmap, int32 x_input = 0 , int32 y_input = 0){
   
    //location for a green spot in bitmap
    int32 pixels_per_frame = 10;
    local_static int32 x_location = 0;  
    local_static int32 y_location = 0;  
    x_location = x_location + (x_input * pixels_per_frame);
    y_location = y_location + (y_input * pixels_per_frame); 

    if(x_location > bitmap->width){x_location = bitmap->width;}
    if(x_location < 0){x_location = 0;}
    if(y_location > bitmap->height){x_location = bitmap->height;}
    if(y_location < 0){y_location = 0;}


    //TODO: Setting green color this way is convoluted
    uint8 green_spot;
    uint8 * p_green_spot = & green_spot; 
   

    local_static int intensity = 0;                     //intensity increases every function call until it overflows    
    int pitch = bitmap->width*bitmap->bytes_per_pixel;  //lenght of a row in bitmap
        
    
    uint8 * p_row = (uint8 *)bitmap->p_memory;          //cast void bitmap pointer to 8 BIT INT
    for(int y=0; y<bitmap->height; y++)
    {   

        uint32 * p_pixel = (uint32*)p_row;              //recast to 32 bit to point argb values
        for(int x=0; x<bitmap->width; x++){   
            
            //Animated background with a green movable crosshair  

            if (x == x_location or y == y_location){   //set lines at x and y location
                *p_green_spot = 255;
            } else {
                *p_green_spot = 0;
            }        

            uint8 * p_color = (uint8 *)p_pixel;
            *p_color = intensity;               //blue from argument
            p_color++;
            *p_color = *p_green_spot;           //green value from if statement for crosshair
            p_color++;
            *p_color = (200*x*y/bitmap->width/bitmap->height); //red bottom right
            p_color++;
            *p_color = 0;                       //buffer maybe use for some effect later
            p_color++;

            p_pixel++;
        }
        p_row += pitch;             //Moves pointer to next row
    }
    intensity++;

}

////////////////////////////////////////////////////////////////
//TESTS


//Tests memory allocation
internal void
write_to_memory( uint8 * location, uint64 amount){        
    for(int64 n=0; n<amount; n++){
        *location = 255;
        location++;
    }
}

////////////////////////////////////////////////////////////////
//BENCMARKS
//TODO: Platform agnostic benchmarks


////////////////////////////////////////////////////////////////

internal void 
game_update_and_render(
    memory_pool * game_memory,
    game_backbuffer * bitmap, 
    game_soundbuffer * soundbuffer, 
    uint32 samples_used, 
    game_input input
){
    //Produce normalized inputs for x, y in -1...1 range.
    bool use_pad_only = false;
    int8 use_pad_index = 0;    
    
    int32 x_input = 0;
    int32 y_input = 0;

    if(use_pad_only){
        x_input = input.pad[use_pad_index].LX / 32768;  //int16  range -32768 ... 32767
        y_input = input.pad[use_pad_index].LY / 32768;  //normalized range = -1 ... 1

    } else {
        for(int16 event; event<input.other_amount; event++){
            //TODO: This will set other input than directions
        }

        //TODO: per millisecond input processing. This version will average inputs in x and y directions.
        //TODO: wasdown is not used for now?
        //TODO would make more sense to just write numbers and skin this switch? At least if direction keys are NEVER used for not-steering.
        bool x_down_continues = false;
        bool y_down_continues = false;

        if(input.x_amount != 0){            
            for(uint16 event = 0; event < input.x_amount; event++){
                
                switch(input.x_axis[event].key){
                    case 'R': x_input++; break;
                    case 'L': x_input--; break;
                }
            }

            x_input = (x_input / input.x_amount); //Average inputs together

        } else { x_input = 0;}

        if(input.y_amount != 0){
            for(uint16 event = 0; event < input.y_amount; event++){
                switch(input.y_axis[event].key){    
                    case 'U': y_input--; break;
                    case 'D':  y_input++; break;
                }       
            }

            y_input = (y_input / input.y_amount); //Average inputs together

        } else { y_input = 0;}

    }

    //TODO: Maybe a normalized location value for rendering and sound
    draw_test(bitmap, x_input, y_input);
    sound_test(soundbuffer,samples_used, x_input, y_input);

    
    char * filename = "read_target";
    read_file_result test_file_struct = platform_debug_read_file(filename);
    assert(test_file_struct.size);    
    platform_debug_write_file("write target", test_file_struct.size, (void *)test_file_struct.memory);
    platform_debug_free_file_memory(test_file_struct.memory);
    

    //write_to_memory(game_memory->base_memory, gigabytes(4));  //Writes 4 GB for testing 
    
}

  