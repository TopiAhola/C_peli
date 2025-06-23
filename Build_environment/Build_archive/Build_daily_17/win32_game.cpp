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
//STRUCTS

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

internal int32
sound_buffer_copy(game_soundbuffer * game_sound_output, LPDIRECTSOUNDBUFFER sound_buffer){
//Get properties of sound_buffer
    DSBCAPS capabilities = {};                   //Struct to put capabilities in.   
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

    //Return value is number of samples copied
    local_static uint32 sample_counter = 0;

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

         
        //Write to locked segments using pointers segment1_p, segment2_p
        int bytes_per_sample = game_sound_output->bytes_per_sample;
        
        INT16 * source_half_sample_p = game_sound_output->memory_p;
        INT16 * half_sample_p;       
        

        //Segment 1 writes
        half_sample_p = (INT16 *) segment1_p;
        for(int sample = 0; sample < ( segment1_size / bytes_per_sample ); sample++){
            
            *half_sample_p = *source_half_sample_p;  //left channel 
            half_sample_p++;  source_half_sample_p++;

            *half_sample_p = *source_half_sample_p;  //right channel
            half_sample_p++;  source_half_sample_p++;

            sample_counter++;            
        }

        //Segment 2 writes
        half_sample_p = (INT16 *) segment2_p;
        for(int sample = 0; sample < ( segment2_size / bytes_per_sample ); sample++){  

            *half_sample_p = *source_half_sample_p;  //left channel 
            half_sample_p++;  source_half_sample_p++;

            *half_sample_p = *source_half_sample_p;  //right channel
            half_sample_p++;  source_half_sample_p++;

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
//BENCHMARKS        TODO: this should be a class?

//TODO: One struct for all benchmark times with one unified print function

//
struct perf_timer {
    int id = 0;
    long long time_start;
    unsigned long long cpu_start;
    long long freq;
    char timer_text_buffer[512];
};

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

//Prints alapsed time and CPU cycles for a given timer
internal void perf_timer_stop(perf_timer * timer_struct){

    unsigned long long cpu_cycle_count = __rdtsc(); 
    unsigned long long timer_cpu_cycles = cpu_cycle_count - timer_struct->cpu_start;

    LARGE_INTEGER end_time;
    QueryPerformanceCounter(&end_time);
    long long timer_cycles_passed = end_time.QuadPart - timer_struct->time_start;

    float time_passed_ms =  1000.0f * (float)timer_cycles_passed   / (float)(timer_struct->freq);     
    float cpu_cycles = ( (float)timer_cpu_cycles ) / 1000000.0f ;    
    
    sprintf(timer_struct->timer_text_buffer, "Timer %d: %.02f ms, %.02f M cpu cycles " ,timer_struct->id, time_passed_ms, cpu_cycles);
    OutputDebugStringA(timer_struct->timer_text_buffer);

}


/////////////////////////////////////////////////////////////
//FILE IO


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
//Function to update window bitmap size. 
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
                int w = paintstruct1.rcPaint.right - paintstruct1.rcPaint.left;
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
            bool ended_down; 
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

            if(virtual_key_code == VK_F4){ program_running = false; } //TODO add controlled shutdown sequence

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
            game_soundbuffer game_sound_output = {48000, 4, 192000, 0};
            game_sound_output.memory_p = (int16*) VirtualAlloc(0, game_sound_output.size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

            //Variable for total sound samples played
            int32 samples_used;

            //Variables for performance counter
            unsigned long long prev_cpu_cycle_count = 0;
            unsigned long long cpu_cycle_count = 0;
            LARGE_INTEGER prev_end_timer;
            prev_end_timer.QuadPart = 0;

            //Structs for perfromance timers
            perf_timer game_main_timer;  
            game_main_timer.id = 1; 
            
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
           
            //Temporary FPS targets
            uint16 display_refresh_rate = 120;                  //TODO: get display rate from Windows
            uint16 game_frame_rate = display_refresh_rate / 4;  // and set reasonalbe game FPS
            

            //Program loop 
            while(program_running)
            {
                //Start timer                
                LARGE_INTEGER start_timer;
                LARGE_INTEGER counter_freq;
                QueryPerformanceCounter(&start_timer);
                QueryPerformanceFrequency(&counter_freq);

                //Reset input struct                 
                input = {};
                copy_prev_input_values(&input, &prev_input);  //TODO: this may be redundant because windows remebres previous state too!
                input_events = {}; //Input events should be reset!

                //Message loop
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
                get_pad_inputs(&input);
                //get_pad_inputs(&input_events); //TODO: Pad inputs more granular than frame rate

                //Set old input
                prev_input = input;

                //GAME MAIN FUNCTION
                perf_timer_start(&game_main_timer);

                //Defined again every loop because backbuffer dimensions may change with window size.
                game_backbuffer bitmap;
                //game_backbuffer * bitmap_p = & bitmap; TODO: pointer is redundant?
                bitmap.p_memory = backbuffer.p_memory;
                bitmap.width = backbuffer.width;
                bitmap.height = backbuffer.height;
                bitmap.bytes_per_pixel = backbuffer.bytes_per_pixel;
                bitmap.pitch = backbuffer.pitch;

                //Fills image and sound buffers with latest data
                game_update_and_render(&game_memory, &bitmap, &game_sound_output, samples_used, input); 

                //Writes to buffer being played
                samples_used = sound_buffer_copy(&game_sound_output, sound_buffer);

                //Updates the image
                window_dimensions dimensions = get_window_dimensions(window);
                HDC device_context = GetDC(window);
                update_window(device_context, p_backbuffer,0,0,dimensions.width,dimensions.height);
                ReleaseDC(window, device_context);      //This works. Why DC has to be released?
                
                perf_timer_stop(&game_main_timer);


                //End timer
                cpu_cycle_count = __rdtsc(); //long long is 64 bit?
                unsigned long long program_loop_cpu_cycles = cpu_cycle_count - prev_cpu_cycle_count;

                LARGE_INTEGER end_timer;
                QueryPerformanceCounter(&end_timer);
                long long program_loop_time = end_timer.QuadPart - prev_end_timer.QuadPart;

                float frame_time =  1000.0f * (float)program_loop_time   / (float)(counter_freq.QuadPart); //milliseconds
                float fps = 1000.0f / frame_time;
                float cpu_cycles = ( (float)program_loop_cpu_cycles ) / 1000000.0f ;

                char fps_text_buffer[512];
                sprintf(fps_text_buffer, "Frame: %.02f ms, %.02f fps, %.02f M cpu cycles " ,frame_time, fps, cpu_cycles);
                OutputDebugStringA(fps_text_buffer);

                prev_cpu_cycle_count = cpu_cycle_count;
                prev_end_timer = end_timer;

                //Sleep for 1 ms to reduce loop speed TODO: remove this when loop is slow enough
                //Sleep(1);
            }

        } //TODO WINDOWHANDLE ERROR      
    } //TODO: Registerclass error 
    return 0;
} 



/////////////////////////////////////////////////////////////
/*
New Windows Functions and structs


*/
