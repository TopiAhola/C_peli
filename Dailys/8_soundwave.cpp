/////////////////////////////////////////////////////////////
//HEADER FILES
#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>


/////////////////////////////////////////////////////////////
//DEFINES                      
                                // static remembers its value like a global but is still limited in scope
#define local_static static     //2 names for static because static is scope dependent
#define global_static static    // for global statics
#define internal static         //static has 3rd meaning with functions limiting use of their name?
 


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
//Writes test sound to given sound buffer 
internal void sound_test(LPDIRECTSOUNDBUFFER sound_buffer){
                
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
        lock_size = ( play_cursor_position - previous_play_cursor_position -1 );
        OutputDebugStringA("sound_test: Play cursor position > previous posttion.\n");


    } else if(play_cursor_position < previous_play_cursor_position) { 
        lock_size = ( (play_cursor_position + (buffer_size - previous_play_cursor_position) ) - 1 );
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
        int bytes_per_sample = 4;
        local_static DWORD sample_counter = 0;
        int value = 0;
        INT16 * half_sample_p;

        if(segment2_size == 0){
            //Segment 1 writes
            half_sample_p = (INT16 *) segment1_p; 
            for(int sample = 0; sample < ( segment1_size / bytes_per_sample ); sample++){

                //250 hz sound: 192 samples for 1 period 

                if( ( sample_counter / 192 ) % 2 ) { value = 1000;} // flips between 0 and 1 in 192 sample intervals
                else {value = -1000;}

                *half_sample_p = value;  //left channel 
                half_sample_p++;
                *half_sample_p = value;  //right channel
                half_sample_p++;
                sample_counter++;
            }


        } else if(segment2_size > 0) {
            //Segment 1 writes
            half_sample_p = (INT16 *) segment1_p;
            for(int sample = 0; sample < ( segment1_size / bytes_per_sample ); sample++){

                //250 hz sound: 192 samples for 1 period 

                if( ( sample_counter / 192 ) % 2 ) { value = 1000;} // flips between 0 and 1 in 192 sample intervals
                else {value = -1000;}

                *half_sample_p = value;  //left channel 
                half_sample_p++;
                *half_sample_p = value;  //right channel
                half_sample_p++;
                sample_counter++;
            }

            //Segment 2 writes
            half_sample_p = (INT16 *) segment2_p;
            for(int sample = 0; sample < ( segment2_size / bytes_per_sample ); sample++){

                //250 hz sound: 192 samples for 1 period 

                if( ( sample_counter / 192 ) % 2 ) { value = 1000;} // flips between 0 and 1 in 192 sample intervals
                else {value = -1000;}

                *half_sample_p = value;  //left channel 
                half_sample_p++;
                *half_sample_p = value;  //right channel
                half_sample_p++;
                sample_counter++;
            }

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



/////////////////////////////////////////////////////////////
//STRUCTS

struct offscreen_bitmap{
    BITMAPINFO info;
    void * p_memory = VirtualAlloc(0, (2500*1400*4), MEM_COMMIT, PAGE_READWRITE);
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

//KB input test globals TODO: remove?
int kb_x_input;
int kb_y_input;


/////////////////////////////////////////////////////////////
//FUNCTIONS

////////////////////////////////
//Takes a pointer to a bitmap. Draws something to the given bitmap.  
internal void
draw_test(offscreen_bitmap * bitmap, int y_input = 0 , int x_input = 0){
    
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
    bitmap->p_memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);

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
        kb_y_input = 0;
        switch(message){
            case WM_ACTIVATEAPP: {OutputDebugStringA("WM_ACTIVATEAPP\n");} break;

            case WM_SIZE: {
                window_dimensions dimensions = get_window_dimensions(window);
                resize_dib_section(p_backbuffer, dimensions.width, dimensions.height);
                draw_test(p_backbuffer, int(1));                
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
        
            } break;

            //Function key input. alt+f4 works if input fed into DefWindowProc or defined here
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP: {
                UINT32 virtual_key_code = wparam;
                LPARAM lparam_bits = lparam;
                bool alt_pressed = ( (lparam_bits & (1<<29)) == 1 ); //is alt key pressed at the same time as key in message

                if(virtual_key_code == VK_F4){ program_running = false; } //TODO add controlled shutdown sequence

            } break;
            
            
            //Keyboard input. 
            case WM_KEYDOWN:
            case WM_KEYUP: {
                UINT32 virtual_key_code = wparam;
                LPARAM lparam_bits = lparam;
                bool previous_state = ( lparam_bits & (1 << 30) );      //bit 30 indicates prev state 1 = was pressed
                bool pressed = ( (lparam_bits & (1 << 31)) == 0 );   //bit 31 indicates transition state 0 = is down

                switch(virtual_key_code){
                    //arrow keys
                    case VK_UP:         if(pressed){kb_y_input = -10;} else {kb_y_input = 0;}     break;  
                    case VK_LEFT:       if(pressed){kb_x_input = -10;} else {kb_x_input = 0;}      break;    
                    case VK_RIGHT:      if(pressed){kb_x_input = 10;} else {kb_x_input = 0;}      break;    
                    case VK_DOWN:       if(pressed){kb_y_input = 10;} else {kb_y_input = 0;}      break;  

                    //special keys
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

            default: {
                OutputDebugStringA("Callback function default\n");
                result = DefWindowProc(window, message, wparam, lparam);  //Calling the windows default window procedure for some of the messages DefWindowProc
            } break;
        }        
    return result;
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

            //program loop 
            while(program_running)
            {
                //message loop
                while(message_bool = PeekMessage(        //PeekMessage gets messages if any
                    &message,                           //Pointer to a location to put message into
                    0,                                  //Handle. 0 means getting messagess for all windows?
                    0,                                  //Filter for messages Can filter keyboard and mouse messages?
                    0,                                   //Filter max at 0 means no filtering
                    PM_REMOVE ))                          //Removes returned messages from queue  
                    {                                    //PeekMessage writes message to the given pointer and returns a "BOOL"
                        
                        if(message.message == WM_QUIT){ program_running = false;}
                        TranslateMessage(&message);                 
                        DispatchMessageA(&message);
                        OutputDebugStringA("message loop\n");                        
                       
                }
                
                //Gamepad input Keyboard input is done through the window Callback function                
                int test_y_input = kb_y_input; // For draw_test().
                int test_x_input = kb_x_input;


                //Gamepad button BOOL states:
                bool input_up,input_down,input_left,input_right,input_start,input_back,input_left_shoulder,input_right_shoulder,input_a,input_b,input_x,input_y;
                input_up=input_down=input_left=input_right=input_start=input_back=input_left_shoulder=input_right_shoulder=input_a=input_b=input_x=input_y=false;

                //Triggers
                BYTE input_left_trigger, input_right_trigger; //Trigger pull in range 0-255
                //sticks x,y positions
                SHORT LX,LY,RX,RY;
                
                DWORD input_error_code;
                for(int index = 0; index < XUSER_MAX_COUNT; index++ ){
                    XINPUT_STATE xinput_state;
                    ZeroMemory(&xinput_state, sizeof(xinput_state));
                    input_error_code = XInputGetState(index, &xinput_state);
                    
                    if(input_error_code == ERROR_SUCCESS){
                        OutputDebugStringA("Controller input get.\n");
                        WORD * p_buttons = &xinput_state.Gamepad.wButtons;

                        input_up= *p_buttons &  XINPUT_GAMEPAD_DPAD_UP;
                        input_down= *p_buttons & XINPUT_GAMEPAD_DPAD_DOWN;
                        input_left= *p_buttons & XINPUT_GAMEPAD_DPAD_LEFT;
                        input_right= *p_buttons & XINPUT_GAMEPAD_DPAD_RIGHT;
                        input_start= *p_buttons & XINPUT_GAMEPAD_START;
                        input_back= *p_buttons & XINPUT_GAMEPAD_BACK;
                        input_left_shoulder = *p_buttons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                        input_right_shoulder = *p_buttons &  XINPUT_GAMEPAD_RIGHT_SHOULDER;
                        input_a= *p_buttons & XINPUT_GAMEPAD_A;
                        input_b= *p_buttons & XINPUT_GAMEPAD_B;
                        input_x= *p_buttons & XINPUT_GAMEPAD_X;
                        input_y= *p_buttons & XINPUT_GAMEPAD_Y;         
                        
                        //Sticks
                        LX = xinput_state.Gamepad.sThumbLX;
                        LY = xinput_state.Gamepad.sThumbLY;
                        RX = xinput_state.Gamepad.sThumbRX;
                        RY = xinput_state.Gamepad.sThumbRY;
                                                                                                                                                        
                        //Triggers
                        input_left_trigger = xinput_state.Gamepad.bLeftTrigger;
                        input_right_trigger = xinput_state.Gamepad.bRightTrigger;

                    }
                    else{
                        OutputDebugStringA("No XInput controller.\n");
                    }
                    
                }
                

                
                //TODO: Test draw should animate background as program runs but doesnt:
                draw_test(p_backbuffer, test_y_input, test_x_input);
                window_dimensions dimensions = get_window_dimensions(window);
                HDC device_context = GetDC(window);
                update_window(device_context, p_backbuffer,0,0,dimensions.width,dimensions.height);
                ReleaseDC(window, device_context);
                //This works. Why DC has to be released?
                

                //sound output                
                sound_test(sound_buffer);  //Locks, writes and unlocks the buffer
                
                


                    

                //program loop ends
                OutputDebugStringA("program loop\n");
                Sleep(1);
            }

        } //TODO WINDOWHANDLE ERROR      
    } //TODO: Registerclass error 
    return 0;
}



/////////////////////////////////////////////////////////////
/*
New Windows Functions and structs

COM = component object model?


Windows Directsound object virtual functions

HRESULT Play(
         DWORD dwReserved1,
         DWORD dwPriority,
         DWORD dwFlags
)





*/

