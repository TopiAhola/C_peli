/////////////////////////////////////////////////////////////
//HEADER FILES
#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <cmath>    //for sin in sound test
#include <stdio.h>  //for sprint in performance debug


#include <C:\Users\valas\VSCProjects\C_peli\Build_environment\Build_current\game.cpp>



/////////////////////////////////////////////////////////////
//STRUCTS FOR PLATFORM CODE

struct offscreen_bitmap{
    BITMAPINFO info;
    void * p_memory = VirtualAlloc(0, (2500*1400*4), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    int width;
    int height;
    int bytes_per_pixel;
    int pitch;
} ;


struct window_dimensions {
    int width;
    int height;
};

//Timestamps
struct timestamps {
    //Counter ticks:    
    LARGE_INTEGER frame_start;
    LARGE_INTEGER message_loop_start;
    LARGE_INTEGER gamepad_input_start;
    LARGE_INTEGER game_function_start;
    LARGE_INTEGER game_function_end;
    LARGE_INTEGER copy_sound_start;
    LARGE_INTEGER copy_sound_end;
    LARGE_INTEGER flip_image_start;
    LARGE_INTEGER flip_image_end;

    //Seconds:
    float32 message_loop_duration;
    float32 input_duration;
    float32 game_function_duration;
    float32 copy_sound_duration;
    float32 flip_image_duration;
    float32 frame_start_to_end;
    float32 frame_end_to_end;
    float32 wait_time;
};


struct platform_frame_time_data {
    uint32 display_refresh_rate = 120;                  //TODO: get display rate from Windows
    uint32 frame_counter;

    float32 unbuffered_frame_time_avg;
    float32 unbuffered_frame_time_max;
    float32 target_frame_time;
    float32 calibrated_frame_time;

    float32 sound_delay_avg;
    float32 last_unbuffered_frame_times[10];
    float32 last_sound_delays[10]; 
    
    uint32 image_delayed_due_sound_count;
    uint32 sound_delayed_until_image_count; 
    //TODO: frame times to an array?
};

struct image_sound_delay {
    float32 sound_delay;
    float32 image_delay;
    uint32 image_flip_sound_sample;
    uint32 sound_delay_samples;
};  


/////////////////////////////////////////////////////////////
//GLOBALS
global_static bool program_running;      //this is toggled false when program needs to quit
global_static offscreen_bitmap backbuffer = {};  //backbuffer is a offscreen_bitmap struct
global_static offscreen_bitmap * p_backbuffer = &backbuffer;


/////////////////////////////////////////////////////////////
//TESTING LOADING WINDOWS FUNCTIONS MANUALLY
/*
These Windows function calls are routed to stub functions via pointers of the same type as the Windows function.
This lets the program run without a .dll library?
*/


////////////////////////////////
//XInput 

//Macros? What do they do?
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)

//Typedef so you can make pointers to stub functions?
typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);

//Stub functions
X_INPUT_GET_STATE(XInputGetState_stub){  return(0); }
X_INPUT_SET_STATE(XInputSetState_stub){ return(0); } 

//Global pointers to stubs
global_static x_input_get_state * p_XInputGetState_stub = XInputGetState_stub;
global_static x_input_set_state * p_XInputSetState_stub = XInputSetState_stub;

//Define function calls to use functions at pointer instead
#define XInputGetState p_XInputGetState_stub
#define XInputSetState p_XInputSetState_stub

//Loads XInput library, sets function calls to point at real Windows functions instead of stubs
//Win 11 doesn't have this library? Maybe put a copy in program folder, LoadLibrary does search there first?
//Win 11 has Xinput9_1_0.dll which has the functions?
internal void load_xinput(void){ 
    HMODULE XInputLibrary = LoadLibraryA("Xinput9_1_0.dll");
    OutputDebugStringA("load_xinput function\n");
    if(XInputLibrary){
        XInputGetState = (x_input_get_state *) GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *) GetProcAddress(XInputLibrary, "XInputSetState");
        OutputDebugStringA("Xinput.dll library functions loaded manually\n");
    }
 
}


////////////////////////////////////////////////////////////////
//SOUND

//DSound TODO: replace with XAudio


////////////////////////////////
//Create and return sound buffer

//Macro and typedef to get a pointer to direct sound function
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


internal LPDIRECTSOUNDBUFFER load_dsound(HWND window, INT32 samples_per_second, INT32 audio_buffer_size){  

    //return variable:
    LPDIRECTSOUNDBUFFER p_secondary_buffer;

    //Load the libraray
    HMODULE dsound_library = LoadLibraryA("dsound.dll");
    if(dsound_library){

        //Get a DirectSound object
        direct_sound_create * p_direct_sound_create = (direct_sound_create *) GetProcAddress(dsound_library, "DirectSoundCreate"); 
        LPDIRECTSOUND p_direct_sound_object;

        if(p_direct_sound_create && SUCCEEDED(p_direct_sound_create(0, &p_direct_sound_object,0))){ //Alternative? :p_direct_sound_create && DirectSoundCreate(0,0,0)==DS_OK
            WAVEFORMATEX wave_format = {};
            wave_format.cbSize = 0;
            wave_format.nChannels = 2;
            wave_format.wBitsPerSample = 16;            
            wave_format.nBlockAlign = ( (wave_format.nChannels*wave_format.wBitsPerSample) / 8 );            
            wave_format.nSamplesPerSec = samples_per_second; //from function argument
            wave_format.nAvgBytesPerSec = ( wave_format.nSamplesPerSec*wave_format.nBlockAlign );
            wave_format.wFormatTag = WAVE_FORMAT_PCM;

            if(SUCCEEDED(p_direct_sound_object->SetCooperativeLevel(window, DSSCL_PRIORITY))){
                    
                //Create primary buffer
                DSBUFFERDESC buffer_description = {};
                buffer_description.dwSize = sizeof(buffer_description);
                buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER; // is wrong! it is for DSBCAPS struct

                LPDIRECTSOUNDBUFFER p_primary_buffer;   //pointer to buffer object? (has methods)

                if(SUCCEEDED(p_direct_sound_object->CreateSoundBuffer(&buffer_description, &p_primary_buffer, 0))){
                    HRESULT error = p_primary_buffer->SetFormat(&wave_format);
                    if(SUCCEEDED(error)){
                        OutputDebugStringA("Primary sound buffer Set");


                    }
                    else {
                        //SetFormat(&wave_format) error in primary buffer
                        OutputDebugStringA("Error in: p_primary_buffer->SetFormat(&wave_format); \n");
                    }
                }
                else {
                    //Create Primary failed
                    OutputDebugStringA("CreateSoundBuffer failed for primary sound buffer\n");
                }

                
                //Create Secondary buffer 
                DSBUFFERDESC secondary_buffer_description = {};                
                secondary_buffer_description.dwFlags = DSBCAPS_GLOBALFOCUS; //some random bits set to test
                secondary_buffer_description.dwBufferBytes = audio_buffer_size;
                secondary_buffer_description.lpwfxFormat = &wave_format;
                secondary_buffer_description.dwReserved = 0;                
                //secondary_buffer_description.guid3DAlgorithm = DS3DALG_NO_VIRTUALIZATION; //No 3d sound :(
                secondary_buffer_description.dwSize = sizeof(secondary_buffer_description);

                //LPDIRECTSOUNDBUFFER p_secondary_buffer; //This will be defined at the function start to return it.  

                if(SUCCEEDED(p_direct_sound_object->CreateSoundBuffer(&secondary_buffer_description, &p_secondary_buffer, 0))){
                    //HRESULT second_error = p_secondary_buffer->SetFormat(&wave_format); SUCCEEDED(second_error)

                    DSBCAPS p_caps = {}; //Struct to put capabilities in. Only used to test if secondary buffer was created...   
                    p_caps.dwSize = sizeof(DSBCAPS);      //size is required to use the struct            
                    if(SUCCEEDED(p_secondary_buffer->GetCaps(&p_caps))){
                        OutputDebugStringA("Secondary sound buffer Set");


                    }
                    else {
                        //SetFormat(&wave_format) error in primary buffer
                        OutputDebugStringA("Error in: p_secondary_buffer->SetFormat(&wave_format); \n");
                    }
                }
                else {
                    //Create Secondary failed
                    OutputDebugStringA("CreateSoundBuffer failed for secondary sound buffer\n");
                }
                

            } 
            else {
                //Error p_direct_sound_object->SetCooperativeLevel 
                OutputDebugStringA("SetCooperativeLevel on direct sound onject failed");
            }       
        }         
        else {
            //Error calling DirectSoundCreate
            OutputDebugStringA("DirectSoundCreate erro\n");

        }
    } 
    else {
        //Error loading library
        OutputDebugStringA("dsound.dll library didnt load\n");
    }

    return p_secondary_buffer;
}






////////////////////////////////
//Copies game output buffer to playing buffer. Returns amount of copied bytes 

internal sound_sample_counter
sound_buffer_copy(
        game_soundbuffer * game_sound_output, 
        LPDIRECTSOUNDBUFFER sound_buffer, 
        timestamps * current_timestamps,
        uint32 sound_delay_samples
    ){
    //Get properties of sound_buffer
    DSBCAPS capabilities = {};                   //Struct to put capabilities in.   
    capabilities.dwSize = sizeof(DSBCAPS);      //size is required to use the struct            
    sound_buffer->GetCaps(&capabilities);
    DWORD buffer_size = capabilities.dwBufferBytes;

    //Properties of source buffer
    int bytes_per_sample = game_sound_output->bytes_per_sample;
    uint32 bytes_per_second = game_sound_output->sample_rate * ((uint32)bytes_per_sample);

    //Get position of Play cursor
    local_static DWORD previous_play_cursor_position = 0;
    DWORD play_cursor_position;
    DWORD write_cursor_positon;
    DWORD write_cursor_offset = (DWORD)(sound_delay_samples * bytes_per_sample);
    sound_buffer->GetCurrentPosition(&play_cursor_position,&write_cursor_positon);

    //TODO: There should be only 1 frame counter in the program
    local_static DWORD frame_count = 0;
    frame_count++; 

    local_static DWORD samples_used_total = 0;
    local_static DWORD samples_used_average = 0;
    local_static uint32 samples_used_maximum = 0; 
    samples_used_average = samples_used_total / frame_count;
    local_static uint32 samples_used = 0;
    local_static uint32 previous_samples_used = 0;

     
    //Set lock segments
    DWORD lock_start_byte;   
    DWORD lock_size;
    LPVOID segment1_p;
    DWORD segment1_size;
    LPVOID segment2_p;          
    DWORD segment2_size;     
    

    // This is needed if offset is used with write cursor
    if((write_cursor_positon + write_cursor_offset) > buffer_size){
        lock_start_byte = (write_cursor_positon + write_cursor_offset) - buffer_size;
    } else if((write_cursor_positon + write_cursor_offset) <= buffer_size) {
        lock_start_byte = (write_cursor_positon + write_cursor_offset);
    } else {
        assert(0);  //buffer lock should always be inside the buffer
    }
    
    lock_size = 2 * bytes_per_sample * samples_used_average; 

    
    
    if(play_cursor_position > previous_play_cursor_position){
        samples_used = (play_cursor_position - previous_play_cursor_position) / bytes_per_sample;
    } else if(play_cursor_position < previous_play_cursor_position) {
        samples_used = ((buffer_size - previous_play_cursor_position) + play_cursor_position ) / bytes_per_sample;
    } else {
        samples_used = 0;
    }
    samples_used_total = samples_used_total + samples_used;

    /* TEST: REMOVE THIS
    int32 source_pointer_offset_half_sample = 2 * (samples_used_total - game_sound_output->last_write_sample_index);
    if(source_pointer_offset_half_sample < 0){source_pointer_offset_half_sample = 0;}
    */

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

        //Write to locked segments using pointers segment1_p, segment2_p        
        INT16 * source_half_sample_p = game_sound_output->memory_p; //TEST: comment out: + source_pointer_offset_half_sample;
        INT16 * half_sample_p;       
        

        //Segment 1 writes
        half_sample_p = (INT16 *) segment1_p;
        for(int sample = 0; sample < ( segment1_size / bytes_per_sample ); sample++){
            
            *half_sample_p = *source_half_sample_p;  //left channel 
            half_sample_p++;  source_half_sample_p++;

            *half_sample_p = *source_half_sample_p;  //right channel
            half_sample_p++;  source_half_sample_p++;
        }

        //Segment 2 writes
        half_sample_p = (INT16 *) segment2_p;
        for(int sample = 0; sample < ( segment2_size / bytes_per_sample ); sample++){  

            *half_sample_p = *source_half_sample_p;  //left channel 
            half_sample_p++;  source_half_sample_p++;

            *half_sample_p = *source_half_sample_p;  //right channel
            half_sample_p++;  source_half_sample_p++;
        }

        sound_buffer->Unlock(       
            segment1_p,
            segment1_size,
            segment2_p,
            segment2_size
        );
    }

    //Return value is a sample counter
    sound_sample_counter sample_counter;

    sample_counter.samples_used = samples_used;
    if(samples_used > sample_counter.samples_used_maximum){
        sample_counter.samples_used_maximum = samples_used;
    }
    sample_counter.samples_used_total = samples_used_total;
    sample_counter.samples_used_average = samples_used_average;


    //Set previous value as current
    previous_play_cursor_position = play_cursor_position;
    previous_samples_used = samples_used;

    return sample_counter;
}


/////////////////////////////////////////////////////////////
//INPUTS

internal void 
get_pad_inputs(game_input * input){

    DWORD input_error_code;
    for(int index = 0; index < XUSER_MAX_COUNT; index++ ){
        //TODO: Pads are hard limited to 4 in game code, change this?
        XINPUT_STATE xinput_state;
        ZeroMemory(&xinput_state, sizeof(xinput_state));
        input_error_code = XInputGetState(index, &xinput_state);
        
        if(input_error_code == ERROR_SUCCESS){
            OutputDebugStringA("Controller input get.\n");
            WORD * p_buttons = &xinput_state.Gamepad.wButtons;

            input->pad[index].up = (bool) *p_buttons & XINPUT_GAMEPAD_DPAD_UP;
            input->pad[index].down = (bool) *p_buttons & XINPUT_GAMEPAD_DPAD_DOWN;
            input->pad[index].left = (bool) *p_buttons & XINPUT_GAMEPAD_DPAD_LEFT;
            input->pad[index].right = (bool) *p_buttons & XINPUT_GAMEPAD_DPAD_RIGHT;
            input->pad[index].start = (bool)*p_buttons & XINPUT_GAMEPAD_START;
            input->pad[index].back = (bool) *p_buttons & XINPUT_GAMEPAD_BACK;
            input->pad[index].left_shoulder = (bool) *p_buttons & XINPUT_GAMEPAD_LEFT_SHOULDER;
            input->pad[index].right_shoulder = (bool) *p_buttons &  XINPUT_GAMEPAD_RIGHT_SHOULDER;
            input->pad[index].a = (bool) *p_buttons & XINPUT_GAMEPAD_A;
            input->pad[index].b = (bool) *p_buttons & XINPUT_GAMEPAD_B;
            input->pad[index].x = (bool) *p_buttons & XINPUT_GAMEPAD_X;
            input->pad[index].y = (bool) *p_buttons & XINPUT_GAMEPAD_Y;         
            
            //Sticks
            input->pad[index].LX = (int16)xinput_state.Gamepad.sThumbLX;
            input->pad[index].LY = (int16)xinput_state.Gamepad.sThumbLY;
            input->pad[index].RX = (int16)xinput_state.Gamepad.sThumbRX;
            input->pad[index].RY = (int16)xinput_state.Gamepad.sThumbRY;
                                                                                                                                            
            //Triggers
            input->pad[index].left_trigger = (uint8)xinput_state.Gamepad.bLeftTrigger;
            input->pad[index].right_trigger = (uint8)xinput_state.Gamepad.bRightTrigger;

        }
        else{
            OutputDebugStringA("No XInput controller.\n");
        }                
    }
}

/////////////////////////////////////////////////////////////
//TIMING AND BENCHMARKING        

////////////////////////////////
//NEW TIMERS

internal void write_timestamp(LARGE_INTEGER * timestamp){
    LARGE_INTEGER counter;
    WINBOOL success = QueryPerformanceCounter(&counter);
    if(success){
        *timestamp = counter;
    }
}

internal void print_timestamps(timestamps * current_timestamps, timestamps * previous_timestamps, LARGE_INTEGER counter_freq){
    char text_buffer[1024];
    sprintf(text_buffer, 
        "DEBUG PRINT: frame_start: %i message_loop_start:%i gamepad_input_start:%i game_function_start: %i game_function_end: %i copy_sound_start:%i flip_image_start:%i flip_image_end:%i ",        
        current_timestamps->frame_start.QuadPart,
        current_timestamps->message_loop_start.QuadPart,
        current_timestamps->gamepad_input_start.QuadPart,
        current_timestamps->game_function_start.QuadPart,
        current_timestamps->game_function_end.QuadPart,
        current_timestamps->copy_sound_start.QuadPart,
        current_timestamps->flip_image_start.QuadPart,
        current_timestamps->flip_image_end.QuadPart
    );
    OutputDebugStringA(text_buffer);

    current_timestamps->message_loop_duration =    ((float32)(current_timestamps->gamepad_input_start.QuadPart - current_timestamps->message_loop_start.QuadPart)) / ((float32)counter_freq.QuadPart);
    current_timestamps->input_duration =           ((float32)(current_timestamps->game_function_start.QuadPart - current_timestamps->gamepad_input_start.QuadPart)) / ((float32)counter_freq.QuadPart);
    current_timestamps->game_function_duration =   ((float32)(current_timestamps->game_function_end.QuadPart - current_timestamps->game_function_start.QuadPart)) / ((float32)counter_freq.QuadPart);
    current_timestamps->copy_sound_duration =      ((float32)(current_timestamps->copy_sound_end.QuadPart - current_timestamps->copy_sound_start.QuadPart)) / ((float32)counter_freq.QuadPart);
    current_timestamps->flip_image_duration =      ((float32)(current_timestamps->flip_image_end.QuadPart - current_timestamps->flip_image_start.QuadPart)) / ((float32)counter_freq.QuadPart);
    current_timestamps->frame_start_to_end =       ((float32)(current_timestamps->flip_image_end.QuadPart - current_timestamps->frame_start.QuadPart)) / ((float32)counter_freq.QuadPart);
    current_timestamps->frame_end_to_end =         ((float32)(current_timestamps->flip_image_end.QuadPart - previous_timestamps->flip_image_end.QuadPart)) / ((float32)counter_freq.QuadPart);

    char text_buffer_2[1024];
    sprintf(text_buffer_2, 
        "DEBUG PRINT: Message loop duration:: %f  input_duration:%f  game_function_duration:%f  copy_sound_duration: %f  flip_image_duration: %f  frame_start_to_end:%f  frame_end_to_end:%f  wait_time:%f  ",        
        current_timestamps->message_loop_duration,
        current_timestamps->input_duration,
        current_timestamps->game_function_duration,
        current_timestamps->copy_sound_duration, 
        current_timestamps->flip_image_duration,
        current_timestamps->frame_start_to_end,
        current_timestamps->frame_end_to_end,
        current_timestamps->wait_time
    );
    OutputDebugStringA(text_buffer_2);

    float32 fps = 1.0f / current_timestamps->frame_end_to_end;
    float32 uncapped_fps = 1.0f / (current_timestamps->frame_end_to_end - current_timestamps->wait_time);

    char text_buffer_3[512];
    sprintf(text_buffer_3, 
        "DEBUG PRINT: Frame time: %f    Wait time: %f    FPS: %f    Uncapped FPS: %f ",        
        current_timestamps->frame_end_to_end,
        current_timestamps->wait_time,
        fps,
        uncapped_fps    
    );
    OutputDebugStringA(text_buffer_3);

}

////////////////////////////////
internal void
calculate_delay(
    image_sound_delay * delays,
    timestamps * timestamps, 
    platform_frame_time_data * frame_time_data, 
    LARGE_INTEGER counter_freq,
    LPDIRECTSOUNDBUFFER sound_buffer,
    game_soundbuffer * game_sound_buffer
) {
    //TODO: this code is duplicated in sound buffer copy.... simplify

    //TODO: sound delay needs to be in SAMPLES otherwise per byte delay will mess up sound channels

    //Get time to cursor    
    DWORD write_cursor_positon;
    DWORD play_cursor_position; 
    sound_buffer->GetCurrentPosition(&play_cursor_position, &write_cursor_positon);
    uint32 bytes_per_second = (uint32)game_sound_buffer->bytes_per_sample * game_sound_buffer->sample_rate;
    uint32 bytes_to_write_cursor = 0;

    if(write_cursor_positon < play_cursor_position) {
        bytes_to_write_cursor = game_sound_buffer->size - (uint32)play_cursor_position + (uint32)write_cursor_positon;
    } else {
        bytes_to_write_cursor = (uint32)(write_cursor_positon - play_cursor_position);
    }

    //See if sound can be written without delaying the image
    assert(bytes_per_second > 0);
    float32 time_to_write_cursor = ((float32)bytes_to_write_cursor) / ((float32)bytes_per_second);
    int64 current_ticks = timestamps->copy_sound_start.QuadPart - timestamps->frame_start.QuadPart;
    float32 current_time = ( (float32)current_ticks) / ( (float32)counter_freq.QuadPart ); 
    float32 time_to_flip = frame_time_data->target_frame_time - current_time - timestamps->flip_image_duration;
 
    
    if(time_to_write_cursor < time_to_flip){
        delays->sound_delay = time_to_flip;
        delays->image_delay = time_to_flip;
        frame_time_data->sound_delayed_until_image_count++;

    } else {
        delays->sound_delay = 0;
        delays->image_delay = time_to_write_cursor;
        frame_time_data->image_delayed_due_sound_count++;

    }

}


////////////////////////////////
internal void
wait_for_frame_time(float32 delay, platform_frame_time_data * frame_time_data, timestamps * timestamps, LARGE_INTEGER counter_freq,  bool timer_pediod_set) {
    LARGE_INTEGER timer;
    int64 ticks_passed;
    float32 time_passed = 0;
    QueryPerformanceCounter(&timer);
    ticks_passed = timer.QuadPart - timestamps->frame_start.QuadPart;

    float32 target_wait_time = frame_time_data->target_frame_time - timestamps->flip_image_duration;

    if(delay > 0){

        //Sleep for whole ms if timer set to 1ms resolution
        if(timer_pediod_set){
            Sleep( (DWORD)(1000.0f * delay) );
        } 

        //Idle in a loop for sub ms precision
        while(time_passed < target_wait_time){
            QueryPerformanceCounter(&timer);
            ticks_passed = timer.QuadPart - timestamps->frame_start.QuadPart;
            time_passed = ( (float32)ticks_passed ) / (float32)(counter_freq.QuadPart);
            
        }

    } 

}

internal void
calibrate_frame_time(timestamps * current_timestamps, platform_frame_time_data * frame_time_data){

    //TODO: this doesn't work
    if(current_timestamps->frame_end_to_end > frame_time_data->target_frame_time){
        frame_time_data->calibrated_frame_time =- 0.001f;
    } else if (current_timestamps->frame_end_to_end < frame_time_data->target_frame_time){
        frame_time_data->calibrated_frame_time =+ 0.001f;
    } else {
        //Nothing
    }
}




////////////////////////////////
//OLD TIMERS

//TODO: Remove old becnmark stuff
////////////////////////////////
struct perf_timer {
    int id = 0;
    long long time_start;
    unsigned long long cpu_start;
    long long freq;
    char timer_text_buffer[512];
};

////////////////////////////////
//Writes start time to struct
internal void perf_timer_start(perf_timer * timer_struct){

    LARGE_INTEGER start_time_li;
    QueryPerformanceCounter(&start_time_li);
    timer_struct->time_start = start_time_li.QuadPart;

    LARGE_INTEGER freq; 
    QueryPerformanceFrequency(&freq);
    timer_struct->freq = freq.QuadPart;

    timer_struct->cpu_start = __rdtsc();

}

////////////////////////////////
//Prints alapsed time and CPU cycles for a given timer
internal void perf_timer_stop(perf_timer * timer_struct){

    unsigned long long cpu_cycle_count = __rdtsc(); 
    unsigned long long timer_cpu_cycles = cpu_cycle_count - timer_struct->cpu_start;

    LARGE_INTEGER end_time;
    QueryPerformanceCounter(&end_time);
    long long timer_cycles_passed = end_time.QuadPart - timer_struct->time_start;

    float time_passed_ms =  1000.0f * (float)timer_cycles_passed   / (float)(timer_struct->freq);     
    float cpu_cycles = ( (float)timer_cpu_cycles ) / 1000000.0f ;    
    
    sprintf(timer_struct->timer_text_buffer, "Debug: Timer %d: %.02f ms, %.02f M cpu cycles " ,timer_struct->id, time_passed_ms, cpu_cycles);
    OutputDebugStringA(timer_struct->timer_text_buffer);

}

////////////////////////////////
internal void
loop_timer_and_sleep(
    unsigned long long * cpu_cycle_count,
    unsigned long long * prev_cpu_cycle_count, 
    LARGE_INTEGER * end_timer,
    LARGE_INTEGER * prev_end_timer,
    LARGE_INTEGER * counter_freq,
    long long * program_loop_time,
    float * target_frame_time_ms,       //TODO: pointer is bigger than float32... Just pass the value.
    bool timer_pediod_set
){
    *cpu_cycle_count = __rdtsc(); //long long is 64 bit?
    unsigned long long program_loop_cpu_cycles = *cpu_cycle_count - *prev_cpu_cycle_count;
    
    QueryPerformanceCounter(end_timer);
    *program_loop_time = end_timer->QuadPart - prev_end_timer->QuadPart;

    float frame_time_ms =  1000.0f * ((float) (*program_loop_time)) / (float)(counter_freq->QuadPart); //milliseconds
    float fps = 1000.0f / frame_time_ms;
    float cpu_cycles_millions = ( (float)program_loop_cpu_cycles ) / 1000000.0f ;       
    float sleep_time =  *target_frame_time_ms - frame_time_ms;    
    
    char fps_text_buffer[512];
    sprintf(fps_text_buffer, "OBSOLETE: Debug: Frame: %.02f ms, %.02f fps if uncapped, %.02f M cpu cycles, sleep for: %.02f ms" ,frame_time_ms, fps, cpu_cycles_millions, sleep_time);
    OutputDebugStringA(fps_text_buffer);
    
    //Waits for frame time
    float delayed_frame_time_ms = frame_time_ms;
    if(sleep_time > 0){

        //Sleep for whole ms if timer set to 1ms resolution
        if(timer_pediod_set){
            Sleep( (DWORD)sleep_time);
        } 

        //Idle in a loop for sub ms precision
        while(delayed_frame_time_ms < *target_frame_time_ms){
            QueryPerformanceCounter(end_timer);
            *program_loop_time = end_timer->QuadPart - prev_end_timer->QuadPart;
            delayed_frame_time_ms =  1000.0f * ((float) (*program_loop_time)) / (float)(counter_freq->QuadPart); //milliseconds

        }

    } 

    *prev_cpu_cycle_count = *cpu_cycle_count;
    *prev_end_timer = *end_timer;

}


/////////////////////////////////////////////////////////////
//DEBUGGING


////////////////////////////////
//Frees memory allocated for file operation
internal bool32 platform_debug_free_file_memory(void * file_p ){  
    WINBOOL success = 0;       
    if(file_p){
        success = VirtualFree(file_p, 0 , MEM_RELEASE); //size 0 releases whole previously allocated block       
    } 
    return (bool32)success;
}


////////////////////////////////
//Creates an empty file
internal bool
platform_debug_create_file(char* filename) {
    bool create_success = false;
    HANDLE filehandle = CreateFileA(filename,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
    
    if(filehandle != INVALID_HANDLE_VALUE){     
        create_success = true;
        CloseHandle(filehandle);

    } else{
        //TODO: bad filehandle error log       
    }

    return create_success;


}


////////////////////////////////
//Write a whole file
internal bool
platform_debug_write_file(char* filename, uint64 size, void * memory){
    bool write_success = false;
    HANDLE filehandle = CreateFileA(filename,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);

    if(filehandle != INVALID_HANDLE_VALUE){
        DWORD bytes_written;
        if(WriteFile(filehandle, memory, size, &bytes_written, 0)){
            if(size == bytes_written){
                write_success = true;
            }

        } else {
            //TODO: wrife fail loggging
        }

        CloseHandle(filehandle);

    } else{
        //TODO: bad filehandle error log       
    }

    return write_success;
}


////////////////////////////////
//Read file to memory
internal read_file_result
platform_debug_read_file(char* filename){
    read_file_result result = {};
    HANDLE filehandle = CreateFileA(filename,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
    if(filehandle != INVALID_HANDLE_VALUE){

        LARGE_INTEGER filesize;
        GetFileSizeEx(filehandle, &filesize);
        uint32 filesize32 = truncate_uint64(filesize.QuadPart);     //Max file size is 32 bit....

        if(void * memory = VirtualAlloc(0, filesize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE)){

            DWORD bytes_read;
            if(ReadFile(filehandle, memory, filesize32, &bytes_read, 0)){
                if(filesize32 == bytes_read){
                    result.size = bytes_read;
                    result.memory = memory;
                }

            } else {
                //TODO: wrife fail logging
                platform_debug_free_file_memory(memory);
            } 

        } else {
           //TODO VirtualAlloc fail logging 
        }

        CloseHandle(filehandle);

    } else {
        //TODO: bad filehandle error log       
    }

    return result;


}

internal void
platform_debug_draw_audio_cursor(
    offscreen_bitmap * p_backbuffer, 
    LPDIRECTSOUNDBUFFER sound_buffer,
    uint32 image_flip_sound_sample
){
    
    //Get sound buffer capabilities
    DSBCAPS capabilities = {};                   //Struct to put capabilities in.   
    capabilities.dwSize = sizeof(DSBCAPS);      //size is required to use the struct            
    sound_buffer->GetCaps(&capabilities);   


    //Get position of cursorS
    local_static DWORD previous_screen_play_cursor_position = 0;
    local_static DWORD previous_screen_write_cursor_position = 0;
    local_static DWORD previous_screen_image_flip_sound_byte = 0;
    DWORD play_cursor_position = 0;
    DWORD write_cursor_position = 0;        
    sound_buffer->GetCurrentPosition(&play_cursor_position,&write_cursor_position);

    //Normalize cursor position to bitmap size:    
    DWORD screen_play_cursor_position = (play_cursor_position * (DWORD)p_backbuffer->width) / capabilities.dwBufferBytes;
    DWORD screen_write_cursor_position = (write_cursor_position * (DWORD)p_backbuffer->width) / capabilities.dwBufferBytes;
    DWORD screen_image_flip_sound_byte = ((image_flip_sound_sample * 4) * (DWORD)p_backbuffer->width) / capabilities.dwBufferBytes;

    //Draw the buffer and cursors
    int buffer_bar_thickness = p_backbuffer->height /10;
    uint32 * row_pointer; 
    uint32 * pixel_pointer;
    uint8 * subpixel_pointer;
    for(int row=0; row < buffer_bar_thickness; row++){
        row_pointer =  (uint32 *)p_backbuffer->p_memory + (row * (p_backbuffer->pitch / p_backbuffer->bytes_per_pixel )) ;

        pixel_pointer = row_pointer + screen_play_cursor_position;
        subpixel_pointer = (uint8*)pixel_pointer; 
        *subpixel_pointer = 0;      //Blue
        subpixel_pointer++;
        *subpixel_pointer = 0;    //Green
        subpixel_pointer++;
        *subpixel_pointer = 255;      //Red

        pixel_pointer = row_pointer + screen_write_cursor_position;
        subpixel_pointer = (uint8*)pixel_pointer; 
        *subpixel_pointer = 0;      //Blue
        subpixel_pointer++;
        *subpixel_pointer = 255;    //Green
        subpixel_pointer++;
        *subpixel_pointer = 0;      //Red

        pixel_pointer = row_pointer + previous_screen_play_cursor_position;
        subpixel_pointer = (uint8*)pixel_pointer; 
        *subpixel_pointer = 0;      //Blue
        subpixel_pointer++;
        *subpixel_pointer = 0;    //Green
        subpixel_pointer++;
        *subpixel_pointer = 255;      //Red

        pixel_pointer = row_pointer + previous_screen_write_cursor_position;
        subpixel_pointer = (uint8*)pixel_pointer; 
        *subpixel_pointer = 0;      //Blue
        subpixel_pointer++;
        *subpixel_pointer = 255;    //Green
        subpixel_pointer++;
        *subpixel_pointer = 0;      //Red  
        
        //Expected image flip position:
        pixel_pointer = row_pointer + screen_image_flip_sound_byte;
        subpixel_pointer = (uint8*)pixel_pointer; 
        *subpixel_pointer = 255;      //Blue
        subpixel_pointer++;
        *subpixel_pointer = 255;    //Green
        subpixel_pointer++;
        *subpixel_pointer = 0;      //Red  

        //Previous expected image flip position
        pixel_pointer = row_pointer + previous_screen_image_flip_sound_byte;
        subpixel_pointer = (uint8*)pixel_pointer; 
        *subpixel_pointer = 255;      //Blue
        subpixel_pointer++;
        *subpixel_pointer = 255;    //Green
        subpixel_pointer++;
        *subpixel_pointer = 0;      //Red      





    }

    previous_screen_play_cursor_position = screen_play_cursor_position;
    previous_screen_write_cursor_position = screen_write_cursor_position;
    previous_screen_image_flip_sound_byte = screen_image_flip_sound_byte;
}



/////////////////////////////////////////////////////////////
//WINDOW FUNCTIONS



////////////////////////////////
//Returns dimensions of RECT for a given window
window_dimensions get_window_dimensions(HWND window){
    RECT client_rect;
    GetClientRect(window, &client_rect);
    window_dimensions dimensions;
    dimensions.width = client_rect.right - client_rect.left;
    dimensions.height = client_rect.bottom - client_rect.top;
    return dimensions;
};

////////////////////////////////
//Takes a pointer to an offscreen_bitmap and resizes it to argument size
internal void 
resize_dib_section(offscreen_bitmap * bitmap,int width, int height){


    bitmap->info.bmiHeader.biSize = sizeof(bitmap->info.bmiHeader);
    bitmap->info.bmiHeader.biWidth = width;
    bitmap->info.bmiHeader.biHeight = -height;        //Negative height defines bitmap starts top-left corner (positive is bottom-left)
    bitmap->info.bmiHeader.biPlanes = 1;
    bitmap->info.bmiHeader.biBitCount = 32;
    bitmap->info.bmiHeader.biCompression = BI_RGB;
    bitmap->info.bmiHeader.biSizeImage = 0;
    bitmap->info.bmiHeader.biXPelsPerMeter = 0;       //For printing? Not important.
    bitmap->info.bmiHeader.biYPelsPerMeter = 0;
    bitmap->info.bmiHeader.biClrUsed = 0;
    bitmap->info.bmiHeader.biClrImportant = 0;    
    bitmap->width = width;
    bitmap->height = height;
    bitmap->bytes_per_pixel = 4;
    bitmap->pitch = width*bitmap->bytes_per_pixel;
    //TODO: check whether bitmap_devicecontext exsist before assingning it? Temp it is global wut that will change

    //Manual memory allocation for bitmap:  
      if(bitmap->p_memory){
        VirtualFree(bitmap->p_memory, 0, MEM_RELEASE);
        OutputDebugStringA("Offscrren bitmap memory released:\n");
    }

    int bitmap_memory_size = bitmap->width*bitmap->height*bitmap->bytes_per_pixel;
    bitmap->p_memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    if(bitmap->p_memory){
        OutputDebugStringA("Offscrren bitmap memory reallocated:\n");
    }   
     
}

//////////////////////////////// 
// Copies a rectangle to another rect, scales if needed. Destination is at the pointer
internal void 
update_window(HDC devicecontext, offscreen_bitmap * bitmap ,int x, int y, int window_width, int window_height){

    StretchDIBits(devicecontext,
        x, y, window_width, window_height,       //destination xywh
        x, y, bitmap->width, bitmap->height,      //source  xywh
        bitmap->p_memory,                         //pointer to bitmap location
        &bitmap->info,                            //pointer to bitmap.info
        DIB_RGB_COLORS,                           //usage  
        SRCCOPY                                   //raster operation
    );
}



////////////////////////////////////////////////////////////////
//WINDOW CALLBACK FUNCTIONS


//This function is for the WNDCLASS 
//Windows can use this when it wants. Main program can call this too, for example WM_PAINT to draw the window. 
LRESULT CALLBACK 
main_window_callback(
    HWND window,                //handle to window in question
    UINT message,               //Search for: System-Defined Messages 
    WPARAM wparam,
    LPARAM lparam )
    {   
        LRESULT result = 0;

        switch(message){
            case WM_ACTIVATEAPP: {OutputDebugStringA("WM_ACTIVATEAPP\n");} break;

            case WM_SIZE: {
                window_dimensions dimensions = get_window_dimensions(window);
                resize_dib_section(p_backbuffer, dimensions.width, dimensions.height);                                
                OutputDebugStringA("WM_SIZE message\n");            
            } break;

            case WM_DESTROY: {            //TODO: destroy might be a error, window can be recreated
                program_running = false;
                OutputDebugStringA("WM_NCDESTROY\n");
            } break;

            case WM_CLOSE: {            //TODO: program should handle closing with a confirmation message first
                program_running = false;
                PostQuitMessage(0);       // Quits the application (different from just closing the window!)
                OutputDebugStringA("WM_CLOSE message\n");
            } break;
                        
            case WM_PAINT: {
                PAINTSTRUCT paintstruct1;
                window_dimensions dim = get_window_dimensions(window);

                HDC devicecontext1 = BeginPaint(window, &paintstruct1);
                int x = paintstruct1.rcPaint.top;
                int y = paintstruct1.rcPaint.left;            
                int w = paintstruct1.rcPaint.right - paintstruct1.rcPaint.left; //TODO: are w and h used variables?
                int h = paintstruct1.rcPaint.bottom - paintstruct1.rcPaint.top;
                //static DWORD raster_code = WHITENESS;       
                update_window(devicecontext1, p_backbuffer,x,y,dim.width,dim.height);
                EndPaint(window, &paintstruct1);
                OutputDebugStringA("WM_PAINT called\n");
            } break;         
            
            
            //Keyboard input was here. Should be handled in program loop now.
            case WM_KEYDOWN:
            case WM_KEYUP: {
                assert(0); //This shouldnt be called ever
            } break;

            default: {
                OutputDebugStringA("Callback function default\n");
                result = DefWindowProc(window, message, wparam, lparam);  //Calling the windows default window procedure for some of the messages DefWindowProc
            } break;
        }        
    return result;
}

////////////////////////////////////////////////////////////////
//INPUTS
internal void
copy_prev_input_values(game_input * input, game_input * prev_input){

    input->up.started_down =        prev_input->up.ended_down;
    input->down.started_down =      prev_input->down.ended_down;
    input->left.started_down =      prev_input->left.ended_down;
    input->right.started_down =     prev_input->right.ended_down;
    input->w.started_down =         prev_input->w.ended_down;
    input->a.started_down =         prev_input->a.ended_down;
    input->s.started_down =         prev_input->s.ended_down;
    input->d.started_down =         prev_input->d.ended_down;
    input->q.started_down =         prev_input->q.ended_down;
    input->e.started_down =         prev_input->e.ended_down;

    //Ended down value will change based on window keyboard messages
    input->up.ended_down =        prev_input->up.ended_down;
    input->down.ended_down =      prev_input->down.ended_down;
    input->left.ended_down =      prev_input->left.ended_down;
    input->right.ended_down =     prev_input->right.ended_down;
    input->w.ended_down =         prev_input->w.ended_down;
    input->a.ended_down =         prev_input->a.ended_down;
    input->s.ended_down =         prev_input->s.ended_down;
    input->d.ended_down =         prev_input->d.ended_down;
    input->q.ended_down =         prev_input->q.ended_down;
    input->e.ended_down =         prev_input->e.ended_down;

}


////////////////////////////////
//Writes keyboard inputs to input struct
internal void
keyboard_message_queue(MSG * message, game_input * input){
 
    UINT32 virtual_key_code = 0;
    LPARAM lparam_bits = 0;

    switch(message->message){
        case WM_QUIT:{ 
        program_running = false;} break;  //TODO: controlled shutdown?
        
        
        //Function key input. alt+f4 works if input fed into DefWindowProc or defined here
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP: {
            virtual_key_code = message->wParam;
            lparam_bits = message->lParam;
            bool alt_pressed = ( (lparam_bits & (1<<29)) == 1 ); //is alt key pressed at the same time as key in message

            if(virtual_key_code == VK_F4){ program_running = false; } //TODO add controlled shutdown sequence

        } break;

        case WM_KEYDOWN:
        case WM_KEYUP: {
            virtual_key_code = message->wParam;
            lparam_bits = message->lParam;
            bool previous_state = ( lparam_bits & (1 << 30) );      //bit 30 indicates prev state 1 = was pressed
            bool pressed = ( (lparam_bits & (1 << 31)) == 0 );   //bit 31 indicates transition state 0 = is down
            
            //bool started_down;  //TODO: This is currently copied with a function. Maybe switch to value provided by Windows?
            //bool ended_down;  //TODO: Remove this?

            uint16 transition_change = 0;
            if(previous_state == pressed){transition_change = 1;}            

            switch(virtual_key_code){
                //arrow keys                
                case VK_UP:     
                    {input->up.ended_down = pressed; input->up.transition_count + transition_change;} break;  
                case VK_LEFT:   
                    {input->left.ended_down = pressed; input->left.transition_count + transition_change;} break;    
                case VK_RIGHT:  
                    {input->right.ended_down = pressed; input->right.transition_count + transition_change;} break;    
                case VK_DOWN:   
                    {input->down.ended_down = pressed; input->down.transition_count + transition_change;} break;  
                

                //special keys //TODO: Skip these for now
                case VK_SPACE:       break;     //Space key
                case VK_RETURN:      break;     //Enter key
                case VK_LSHIFT:      break;     //Left Shift key
                case VK_RSHIFT:      break;     //Right Shift key
                case VK_LCONTROL:    break;		//Left Ctrl key
                case VK_RCONTROL:    break;		//Right Ctrl key
                case VK_MENU:        break; 	//Alt key
                case VK_LMENU:       break;     //Left Alt key
                case VK_RMENU:       break;     //Right Alt key
                case VK_TAB:         break;	    //Tab key                    
                case VK_CAPITAL:     break;	    //Caps lock key  

                //characters
                case 'W':
                case 'w':{input->w.ended_down = pressed; input->w.transition_count += transition_change;}  break;		//w key
                
                case 'A': 
                case 'a':{input->a.ended_down = pressed; input->a.transition_count += transition_change;}  break;		//a key
                
                case 'S': 
                case 's':{input->s.ended_down = pressed; input->s.transition_count += transition_change;}  break;		//s key
                
                case 'D': 
                case 'd':{input->d.ended_down = pressed; input->d.transition_count += transition_change;}  break;		//d key
                
                case 'Q': 
                case 'q':{input->q.ended_down = pressed; input->q.transition_count += transition_change;}  break;		//q key
                
                case 'E': 
                case 'e':{input->e.ended_down = pressed; input->e.transition_count += transition_change;}  break;		//e key


                //mouse buttons
                case VK_LBUTTON	:    break;        
                case VK_RBUTTON	:    break;        
                case VK_CANCEL:      break;      
                case VK_MBUTTON:     break;   
                case VK_XBUTTON1:    break;        
                case VK_XBUTTON2:    break;
            }

        } break;
    }    
}


////////////////////////////////
//Writes keyboard inputs to input_events struct
internal void keyboard_message_queue_events(MSG * message, game_input_events * input_events){
    UINT32 virtual_key_code = 0;
    LPARAM lparam_bits = 0;

    switch(message->message){
        case WM_QUIT:{ 
        program_running = false;} break;  //TODO: controlled shutdown?
        
        
        //Function key input. alt+f4 works if input fed into DefWindowProc or defined here
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP: {
            virtual_key_code = message->wParam;
            lparam_bits = message->lParam;
            bool alt_pressed = ( (lparam_bits & (1<<29)) == 1 ); //is alt key pressed at the same time as key in message

            if(virtual_key_code == VK_F4 && alt_pressed){ program_running = false; } //TODO add controlled shutdown sequence

        } break;

        case WM_KEYDOWN:
        case WM_KEYUP: {
            virtual_key_code = message->wParam;
            lparam_bits = message->lParam;
            bool previous_state = ( lparam_bits & (1 << 30) );      //bit 30 indicates prev state 1 = was pressed
            bool pressed = ( (lparam_bits & (1 << 31)) == 0 );   //bit 31 indicates transition state 0 = is down
            
            bool direction_input = false;
            keypress_event new_keypress;
            new_keypress.time = message->time;
            new_keypress.was_down = previous_state;

            switch(virtual_key_code){
                //arrow keys                
                case VK_UP:     
                    {new_keypress.key = 'U'; input_events->y_axis[input_events->y_amount] = new_keypress; input_events->y_amount++;} break;  
                case VK_LEFT:   
                    {new_keypress.key = 'L'; input_events->x_axis[input_events->x_amount] = new_keypress; input_events->x_amount++;} break;    
                case VK_RIGHT:  
                    {new_keypress.key = 'R'; input_events->x_axis[input_events->x_amount] = new_keypress; input_events->x_amount++;} break;    
                case VK_DOWN:   
                    {new_keypress.key = 'D'; input_events->y_axis[input_events->y_amount] = new_keypress; input_events->y_amount++;} break;  
                

                //special keys //TODO: Skip these for now
                case VK_SPACE:       break;     //Space key
                case VK_RETURN:      break;     //Enter key
                case VK_LSHIFT:      break;     //Left Shift key
                case VK_RSHIFT:      break;     //Right Shift key
                case VK_LCONTROL:    break;		//Left Ctrl key
                case VK_RCONTROL:    break;		//Right Ctrl key
                case VK_MENU:        break; 	//Alt key
                case VK_LMENU:       break;     //Left Alt key
                case VK_RMENU:       break;     //Right Alt key
                case VK_TAB:         break;	    //Tab key                    
                case VK_CAPITAL:     break;	    //Caps lock key  

                //characters
                case '0':     break;		//0 key
                case '1':     break;		//1 key
                case '2':     break;		//2 key
                case '3':     break;		//3 key
                case '4':     break;		//4 key
                case '5':     break;		//5 key
                case '6':     break;		//6 key
                case '7':     break;		//7 key
                case '8':     break;		//8 key
                case '9':     break;		//9 key
                case 'A':     break;		//A key
                case 'B':     break;		//B key
                case 'C':     break;		//C key
                case 'D':     break;		//D key
                case 'E':     break;		//E key
                case 'F':     break;		//F key
                case 'G':     break;		//G key
                case 'H':     break;		//H key
                case 'I':     break;		//I key
                case 'J':     break;		//J key
                case 'K':     break;		//K key
                case 'L':     break;		//L key
                case 'M':     break;		//M key
                case 'N':     break;		//N key
                case 'O':     break;		//O key
                case 'P':     break;		//P key
                case 'Q':     break;		//Q key
                case 'R':     break;		//R key
                case 'S':     break;		//S key
                case 'T':     break;		//T key
                case 'U':     break;		//U key
                case 'V':     break;		//V key
                case 'W':     break;		//W key
                case 'X':     break;		//X key
                case 'Y':     break;		//Y key
                case 'Z':     break;		//Z key

                //mouse buttons
                case VK_LBUTTON	:    break;        
                case VK_RBUTTON	:    break;        
                case VK_CANCEL:      break;      
                case VK_MBUTTON:     break;   
                case VK_XBUTTON1:    break;        
                case VK_XBUTTON2:    break;

            }

        } break;

    }
}

/////////////////////////////////////////////////////////////
//WINMAIN
int CALLBACK WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance, 
    LPSTR lpCmdLine,
    int CmdShow)
    {
    WNDCLASSA my_windowclass = {} ;   //WNDCLASS my_windowclass -class Kokeillaan WNDCLASS+W

    my_windowclass.style = CS_CLASSDC|CS_HREDRAW|CS_VREDRAW;  //HREDRAW or VREDRAW draws the whole window again 
    my_windowclass.lpfnWndProc = main_window_callback;
    my_windowclass.hInstance;  
    my_windowclass.lpszClassName = "MyGameWindowClass"; //Väärä tyyppi? compiler onnistuu kuitenkin?


    
    if(RegisterClassA(&my_windowclass))
    { // Class has to be registered ? Function retruns an "ATOM" type thing but it isnt needed?

        HWND window = CreateWindowExA(
            0,                                  //extended styles 0 = nothing
            my_windowclass.lpszClassName,       //classname
            "Name_at_createwindow",             //name that may? overwrite class name
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,     //window styles
            CW_USEDEFAULT,                      // x dimension does CW_USEDEFAULT actually use fome default value?
            CW_USEDEFAULT,                      // y dimension
            CW_USEDEFAULT,                      //height
            CW_USEDEFAULT,                      //width
            0,                                  //parent, 0 because there is none?
            0,                                  //hmenu 
            hInstance,                          //hInstance
            0                                   //lpParam can be used to pass paramater for smt.
        );
            
        if (window) 
        {   
            //Variables                             
            program_running = true;  
            MSG message;
            BOOL message_bool = 0;

            //Loads XInput Get and Set functions manually
            load_xinput();      

            //Create sound buffer
            LPDIRECTSOUNDBUFFER sound_buffer = load_dsound(window,48000,48000*4);      //Loads DSound TODO: replace with XAudio ! 44k samples per s, 4 bytes per sample (2 bytes per channel)
            sound_buffer->Play(0,0,DSBPLAY_LOOPING); //Buffer can be played even if empty     

            //Create buffer for game return sound to. Will be copied to sound_buffer to be played        
            game_soundbuffer game_sound_output = {0, 48000, 192000, 0, 4};
            game_sound_output.memory_p = (int16*) VirtualAlloc(0, game_sound_output.size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

            //Variable for sound samples played during loop
            sound_sample_counter sample_counter;
            
            //Structs for passing all inputs to the game //TODO: Event based input not used for now
            game_input input;
            game_input_events input_events;
            game_input prev_input = {};
            
            //Memory allocation for game    5G total, of which 4G is base and 1G is temp
            memory_pool game_memory = {};
            game_memory.base_memory_size = gigabytes(5);
            game_memory.base_memory = (uint8*) VirtualAlloc(0,game_memory.base_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            game_memory.temp_memory_size = gigabytes(1);
            game_memory.temp_memory = game_memory.base_memory + game_memory.temp_memory_size;
           
            //Set Windows timer to minimum resolution
            bool timer_pediod_set = false;
            UINT u_period = 1;
            MMRESULT timer_error_code = timeBeginPeriod(u_period);
            if(timer_error_code == TIMERR_NOERROR){timer_pediod_set = true;}

            //Struct for timestamps       
            timestamps previous_timestamps = {};
            timestamps current_timestamps = {};
            platform_frame_time_data frame_time_data = {};
            frame_time_data.display_refresh_rate = 120;                  //TODO: get display rate from Windows
            frame_time_data.target_frame_time = 1.0f / 18.0f;              //TODO: FRAME RATE BELLOW 30 WILL BREAK SOUND
            frame_time_data.calibrated_frame_time = frame_time_data.target_frame_time;


            //Counter frequency needs to be queried just once
            LARGE_INTEGER counter_freq;                
            QueryPerformanceFrequency(&counter_freq);


            //Program loop 
            while(program_running)            
            {
                //Start timer          
                write_timestamp(&current_timestamps.frame_start);

                //Reset input struct                 
                input = {};
                copy_prev_input_values(&input, &prev_input);  //TODO: this may be redundant because windows remebres previous state too!
                input_events = {}; //Input events should be reset!

                //Message loop
                write_timestamp(&current_timestamps.message_loop_start);

                while(message_bool = PeekMessage(        //PeekMessage gets messages if any //PeekMessage writes message to the given pointer and returns a "BOOL"
                    &message,                           //Pointer to a location to put message into
                    0,                                  //Handle. 0 means getting messagess for all windows?
                    0,                                  //Filter for messages Can filter keyboard and mouse messages?
                    0,                                   //Filter max at 0 means no filtering
                    PM_REMOVE ))                         //Removes returned messages from queue  
                    {                         
                    switch(message.message){
                        case WM_KEYDOWN: case WM_KEYUP: case WM_SYSKEYUP: case WM_SYSKEYDOWN: case WM_QUIT: {
                            //Get keyboard inputs into game_input struct
                            //keyboard_message_queue_events(&message, &input_events); //TODO: 
                            keyboard_message_queue(&message, &input);

                        } break;

                        default: {
                            //Other input go to regular message callback function
                            TranslateMessage(&message);                                     
                            DispatchMessageA(&message); 
                        } break;
                    }                                      
           
                    OutputDebugStringA("message loop\n");
                }       

                //Get gamepad inputs into game_input struct
                write_timestamp(&current_timestamps.gamepad_input_start);

                get_pad_inputs(&input);
                //get_pad_inputs(&input_events); //TODO: Pad inputs more granular than frame rate

                //Set old input
                prev_input = input;

                //GAME MAIN FUNCTION                             
                write_timestamp(&current_timestamps.game_function_start);

                //Defined again every loop because backbuffer dimensions may change with window size.
                game_backbuffer bitmap;
                //game_backbuffer * bitmap_p = & bitmap; TODO: pointer is redundant?
                bitmap.p_memory = backbuffer.p_memory;
                bitmap.width = backbuffer.width;
                bitmap.height = backbuffer.height;
                bitmap.bytes_per_pixel = backbuffer.bytes_per_pixel;
                bitmap.pitch = backbuffer.pitch;

                //Fills image and sound buffers with latest data
                game_update_and_render(&game_memory, &bitmap, &game_sound_output, sample_counter, input); 
                write_timestamp(&current_timestamps.game_function_end);

                //Writes to buffer being played. Returns number of used samples.
                write_timestamp(&current_timestamps.copy_sound_start);

                //Determine delay for either sound or image
                image_sound_delay delays = {};
                calculate_delay(&delays, &current_timestamps ,&frame_time_data, counter_freq, sound_buffer, &game_sound_output);
                current_timestamps.wait_time = delays.image_delay;


                //TODO: sound_buffer_copy has to take expected sound delay!
                sample_counter = sound_buffer_copy(&game_sound_output, sound_buffer, &current_timestamps, delays.sound_delay);
                write_timestamp(&current_timestamps.copy_sound_end);

#if DEVELOPER_BUILD
                //Debug display for audio buffer
                //TODO: draw sound and image delay!
                platform_debug_draw_audio_cursor(p_backbuffer, sound_buffer, delays.image_flip_sound_sample);
#endif

                //Wait for image flip time
                wait_for_frame_time(delays.image_delay, &frame_time_data, &current_timestamps , counter_freq, timer_pediod_set);

                //Update the image
                write_timestamp(&current_timestamps.flip_image_start);

                window_dimensions dimensions = get_window_dimensions(window);
                HDC device_context = GetDC(window);
                update_window(device_context, p_backbuffer,0,0,dimensions.width,dimensions.height);
                ReleaseDC(window, device_context);      //This works. Why DC has to be released?

                write_timestamp(&current_timestamps.flip_image_end);
         
#if DEVELOPER_BUILD
                //Print timestamps
                print_timestamps(&current_timestamps, &previous_timestamps, counter_freq);
#endif

                calibrate_frame_time(&current_timestamps, &frame_time_data);
                previous_timestamps = current_timestamps;
                //TODO: image flip may be constant so can be benchmarked just once at start?

            } //Program loop ends

            //Reset Windows timer resolution
            timeEndPeriod(u_period);

        } //TODO WINDOWHANDLE ERROR      
    } //TODO: Registerclass error 
    return 0;
} 



/////////////////////////////////////////////////////////////
/*
New Windows Functions and structs


*/
