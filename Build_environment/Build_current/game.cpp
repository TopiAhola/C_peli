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
internal void sound_test(game_soundbuffer * soundbuffer , float32 x_location_relative, float32 y_location_relative, uint32 frame_counter){  
    //TODO: if bad struct in argumets will this crash?  
    //TODO: Remove whole struct, average can be counted here locally...  
    

//#if PERFORMANCE_MODE == 1  //Write less samples based on samples used data

//Continous sine wave
    float32 amplitude = 4000.0f * x_location_relative;
    uint32 freq =  (uint32)( 400.0f * y_location_relative );     
    if(freq <= 0){freq = 1;} //No smaller than 1 frequencies
    
    //Write up to 2 frames of sound. This will cause audio cuts if frame time fluctuates 2x
    uint32 samples_to_write = 0; 
    float32 sample_number = 0;

    //TODO: Phase of the sinewave should be function of game time. There is no timer currently other than audio sample usage!

    //Write to buffer memory
    float32 period = (float32)( (soundbuffer->sample_rate ) / freq );
    int16 value;
    int16 * channel_p = soundbuffer->memory_p;  
    for(uint32 sample = 0; sample < samples_to_write; sample++){
        
        //Sine wave function value
        float32 phase = sinf(( 2.0f * pi32 * ( sample_number /  period ) ));
        value = (int16)(amplitude * phase) ;

        *channel_p = value;  //left channel 
        channel_p++;
        *channel_p = value;  //right channel
        channel_p++;
        sample_number++;
    } 
    

    //TODO: remove this ? 
    //soundbuffer->last_write_sample_index = 0;


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
draw_test(game_backbuffer * bitmap, int32 x_location, int32 y_location){
   

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
//INPUTS

internal void
get_keyboard_inputs(float32 * x_input, float32 * y_input, game_input * input){
    static bool moving_right = false;
    static bool moving_up = false;

    //X-axis
    if(!input->left.started_down && input->left.ended_down){
        moving_right = false;

    } else if(!input->right.started_down && input->right.ended_down) {
        moving_right = true;
    } 
                  
    if(input->right.ended_down && input->left.ended_down){
        //If both pressed resolve based on which was pressed down this frame
        if(moving_right){
            *x_input += 1.0f;
        } else {
            *x_input -= 1.0f;
        }

    } else if(input->right.ended_down){
        *x_input += 1.0f;
    } else if(input->left.ended_down){
        *x_input -= 1.0f;
    }

    //Y-axis
    if(!input->down.started_down && input->down.ended_down){
        moving_up = false;

    } else if(!input->up.started_down && input->up.ended_down) {
        moving_up = true;
    } 
                  
    if(input->down.ended_down && input->up.ended_down){
        //If both pressed resolve based on which was pressed down this frame
        if(moving_up){
            *y_input -= 1.0f;
        } else {
            *y_input += 1.0f;
        }

    } else if(input->down.ended_down){
        *y_input += 1.0f;
    } else if(input->up.ended_down){
        *y_input -= 1.0f;
    }
    
}

//Derive input values from input timestamps TODO: Not used for now
internal void 
get_inputs_from_events(int32 * x_input, int32 * y_input, game_input_events * input) {
    bool use_pad_only = false;
    int8 use_pad_index = 0;    

    //TODO: This needs to return floats in range -1...1
    if(use_pad_only){
        *x_input = input->pad[use_pad_index].LX / 32768;  //int16  range -32768 ... 32767
        *y_input = input->pad[use_pad_index].LY / 32768;  //normalized range = -1 ... 1

    } else {
        for(int16 event; event<input->other_amount; event++){
            //TODO: This will set other input than directions
        }

        //TODO: per millisecond input processing. This version will average inputs in x and y directions.
        //TODO: wasdown is not used for now?
        //TODO would make more sense to just write numbers and skin this switch? At least if direction keys are NEVER used for not-steering.
        bool x_down_continues = false;
        bool y_down_continues = false;

        if(input->x_amount != 0){            
            for(uint16 event = 0; event < input->x_amount; event++){
                
                switch(input->x_axis[event].key){
                    case 'R': *x_input++; break;
                    case 'L': *x_input--; break;
                }
            }

            *x_input = (*x_input / input->x_amount); //Average inputs together

        } else { x_input = 0;}

        if(input->y_amount != 0){
            for(uint16 event = 0; event < input->y_amount; event++){
                switch(input->y_axis[event].key){    
                    case 'U': *y_input--; break;
                    case 'D':  *y_input++; break;
                }       
            }

            *y_input = (*y_input / input->y_amount); //Average inputs together

        } else { y_input = 0;}

    }
}



////////////////////////////////////////////////////////////////
//TESTS


//Tests memory allocation
internal void
write_to_memory( uint8 * location, uint64 amount){        
    for(uint64 n=0; n<amount; n++){
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
    game_input input
){
    //Count the frame
    local_static uint32 frame_counter = 0;      
    frame_counter =+ 1;                 //Has to start from 1

    //Produce normalized inputs for x, y in -1...1 range.   
    float32 x_input = 0;
    float32 y_input = 0;
    //get_inputs_from_events(&x_input, &y_input, &input);   //TODO: event based input
    get_keyboard_inputs(&x_input, &y_input, &input); //Simple input parsing


    //location for a green spot in bitmap and sound
    float32 pixels_per_frame = 10.0f;
    local_static int32 x_location = bitmap->width / 2;  
    local_static int32 y_location = bitmap->height / 2;  
    x_location = x_location + (int32)(x_input * pixels_per_frame);
    y_location = y_location + (int32)(y_input * pixels_per_frame);  

    if(x_location > bitmap->width){x_location = bitmap->width;}
    if(x_location < 0){x_location = 0;}
    if(y_location > bitmap->height){y_location = bitmap->height;}
    if(y_location < 0){y_location = 0;}

    float32 x_location_relative = (float32)x_location / (float32)bitmap->width;
    float32 y_location_relative = (float32)y_location / (float32)bitmap->height;

    //Draw and sound test
    draw_test(bitmap, x_location, y_location);
    sound_test(soundbuffer, x_location_relative, y_location_relative, frame_counter);

    
    char * filename = "read_target";
    read_file_result test_file_struct = platform_debug_read_file(filename);
    assert(test_file_struct.size);    
    platform_debug_write_file("write target", test_file_struct.size, (void *)test_file_struct.memory);
    platform_debug_free_file_memory(test_file_struct.memory);
    

    write_to_memory(game_memory->base_memory, gigabytes(4));  //Writes 4 GB for testing 
    

}

  