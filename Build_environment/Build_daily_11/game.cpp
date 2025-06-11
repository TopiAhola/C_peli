#include <C:\Users\valas\VSCProjects\C_peli\Build_environment\Build_daily_11\game_header.h>

//Temp includes for debugging:
#include <windows.h>
#include <dsound.h>
#include <cmath>



////////////////////////////////////////////////////////////////
//DEFINES
#define local_static static     //2 names for static because static is scope dependent
#define global_static static    // for global statics
#define internal static         //static has 3rd meaning with functions limiting use of their name?


////////////////////////////////////////////////////////////////
//SOUND

//Writes test sound to given sound buffer 
internal void sound_test(LPDIRECTSOUNDBUFFER sound_buffer, int x_input, int y_input){
                
    //Get properties of sound_buffer
    DSBCAPS capabilities = {};                                  //Struct to put capabilities in.   
    capabilities.dwSize = sizeof(DSBCAPS);      //size is required to use the struct            
    sound_buffer->GetCaps(&capabilities);
    DWORD buffer_size = capabilities.dwBufferBytes;

    //Get position of Play cursor
    local_static DWORD previous_play_cursor_position = 0;
    DWORD play_cursor_position;     
    sound_buffer->GetCurrentPosition(&play_cursor_position,0);
       
    //Set lock segments
    DWORD lock_start_byte = previous_play_cursor_position;  
    DWORD lock_size;
    LPVOID segment1_p;
    DWORD segment1_size;
    LPVOID segment2_p;          
    DWORD segment2_size;    

    if(play_cursor_position > previous_play_cursor_position){ 
        lock_size = ( play_cursor_position - previous_play_cursor_position  );
        OutputDebugStringA("sound_test: Play cursor position > previous posttion.\n");


    } else if(play_cursor_position < previous_play_cursor_position) { 
        lock_size = ( (play_cursor_position + (buffer_size - previous_play_cursor_position) )  );
        OutputDebugStringA("sound_test: Play cursor position < previous posttion.\n");

    } else {
        lock_size = 0;
        OutputDebugStringA("sound_test: Play cursor position == previous posttion.\n");
    }
    
    if(lock_size > 0){        
        sound_buffer->Lock(         
            lock_start_byte,
            lock_size, 
            &segment1_p,
            &segment1_size,
            &segment2_p,
            &segment2_size,
            0
        );

        //Sine wave
        local_static INT16 amplitude = 1000;
        local_static int freq = 100;
        
        amplitude += (10 * x_input );
        freq += y_input;
        int period = (48000 / freq);;            // = samplerate / frequency TODO: get buffer sample rate automatically!
        float pi = 3.14159265f;      
    

        //Write to locked segments using pointers segment1_p, segment2_p
        int bytes_per_sample = 4;
        local_static int sample_counter = 0;
        INT16 value = 0;
        INT16 * half_sample_p;       
        

        //Segment 1 writes
        half_sample_p = (INT16 *) segment1_p;
        for(int sample = 0; sample < ( segment1_size / bytes_per_sample ); sample++){
            

            float phase = sinf( (2.0f * pi * ( (float)sample_counter) / ((float)period) ) );
            value = (INT16)(amplitude * phase) ;

            *half_sample_p = value;  //left channel 
            half_sample_p++;
            *half_sample_p = value;  //right channel
            half_sample_p++;
            sample_counter++;
            
        }

        //Segment 2 writes
        half_sample_p = (INT16 *) segment2_p;
        for(int sample = 0; sample < ( segment2_size / bytes_per_sample ); sample++){

            float phase = sinf( ( (2.0f * pi * ((float)sample_counter) ) / ((float)period) ) );
            value = (INT16)(amplitude * phase) ;

            *half_sample_p = value;  //left channel 
            half_sample_p++;
            *half_sample_p = value;  //right channel
            half_sample_p++;
            sample_counter++;

        }

        sound_buffer->Unlock(       
            segment1_p,
            segment1_size,
            segment2_p,
            segment2_size
        );
    }

    previous_play_cursor_position = play_cursor_position;
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
draw_test(game_backbuffer * bitmap, int x_input = 0 , int y_input = 0){
    
    OutputDebugStringA("draw test\n");

    
    //location for a green spot in bitmap
    local_static int y_location = 0;  
    y_location = y_location + y_input; 
    local_static int x_location = 0;  
    x_location = x_location + x_input;
    UINT8 green_spot;
    UINT8 * p_green_spot = & green_spot; 
   

    local_static int intensity = 0; //intensity increases every function call until it overflows    
    int pitch = bitmap->width*bitmap->bytes_per_pixel;  //lenght of a row in bitmap
        
    
    UINT8 * p_row = (UINT8 *)bitmap->p_memory; //cast void bitmap pointer to 8 BIT INT
    for(int y=0; y<bitmap->height; y++)
    {   

        UINT32 * p_pixel = (UINT32*)p_row;     //recast to 32 bit to point argb values
        for(int x=0; x<bitmap->width; x++)
        {   
            //p_pixel = p_pixel + x;    //iterates over pixels, SEGFAULTS

            //WINDOWS has colors BGRx but this has xRGB but little endian!
            //*p_pixel = 0x00500000;      //argb LITTLE ENDIAN MEANS VALUES HAVE SMALLER NUMBERS FIRST
            //It writes smallest bit to first byte so the hexa is still xRGB when read with 32 bit pointer
            
            //Animated background with a green movable crosshair  

            if (x == x_location or y == y_location){   //set lines at x and y location
                *p_green_spot = 255;
            } else {
                *p_green_spot = 0;
            }          

            
            UINT8 * p_color = (UINT8 *)p_pixel;
            *p_color = intensity; //blue from argument
            p_color++;
            *p_color = *p_green_spot; //green value from if statement for crosshair
            p_color++;
            *p_color = (200*x*y/bitmap->width/bitmap->height); //red bottom right
            p_color++;
            *p_color = 0; //buffer maybe use for some effect later
            p_color++;

            p_pixel++;
        }
        p_row += pitch; //Moves pointer to next row
    }
    intensity++;

}

////////////////////////////////////////////////////////////////
//BENCMARKS
//TODO: Platform agnostic benchmarks


////////////////////////////////////////////////////////////////

internal void 
game_update_and_render(game_backbuffer * bitmap, int x_input, int y_input){
    draw_test(bitmap, x_input, y_input);

}

  