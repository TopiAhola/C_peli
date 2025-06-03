/////////////////////////////////////////////////////////////
//HEADER FILES
#include <windows.h>
#include <stdint.h>
#include <xinput.h>


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

//Loads XInput library
internal void load_xinput(){ 
    
 
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



//GLOBALS
global_static bool program_running;      //this is toggled false when program needs to quit
global_static offscreen_bitmap backbuffer = {};  //backbuffer is a offscreen_bitmap struct
global_static offscreen_bitmap * p_backbuffer = &backbuffer;

/////////////////////////////////////////////////////////////
//FUNCTIONS


//Returns dimensions of RECT for a given window
window_dimensions get_window_dimensions(HWND window){
    RECT client_rect;
    GetClientRect(window, &client_rect);
    window_dimensions dimensions;
    dimensions.width = client_rect.right - client_rect.left;
    dimensions.height = client_rect.bottom - client_rect.top;
    return dimensions;
};


//Takes a pointer to a bitmap. Draws something to the given bitmap.  
internal void
draw_test(offscreen_bitmap * bitmap){
    
    OutputDebugStringA("draw test");

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
            
            //ANIMATED PER COLOR VERSION     

            UINT8 * p_color = (UINT8 *)p_pixel;
            *p_color = intensity; //blue from argument
            p_color++;
            *p_color = 0; //green to upper left
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
        OutputDebugStringA("Offscrren bitmap memory released:");
    }

    int bitmap_memory_size = bitmap->width*bitmap->height*bitmap->bytes_per_pixel;
    bitmap->p_memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);

    if(bitmap->p_memory){
        OutputDebugStringA("Offscrren bitmap memory reallocated:");
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
                draw_test(p_backbuffer);                
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


            default: {
                OutputDebugStringA("default\n");
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
            program_running = true;  
            MSG message;
            BOOL message_bool = 0;

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
                        OutputDebugStringA("message loop");                        
                       
                }
                
                //Gamepad and KB inputs

                //Button BOOL states:
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
                        OutputDebugStringA("Controller input get.");
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
                        OutputDebugStringA("No XInput controller.");
                    }
                    
                }
                


                //TODO: Test draw should animate background as program runs but doesnt:
                draw_test(p_backbuffer);
                window_dimensions dimensions = get_window_dimensions(window);
                HDC device_context = GetDC(window);
                update_window(device_context, p_backbuffer,0,0,dimensions.width,dimensions.height);
                ReleaseDC(window, device_context);
                //This works. Why DC has to be released?
                
                    

                //program loop ends
                OutputDebugStringA("program loop");
                Sleep(1);
            }

        } //TODO WINDOWHANDLE ERROR      
    } //TODO: Registerclass error 
    return 0;
}



/////////////////////////////////////////////////////////////
/*
New Windows Functions and structs

EXAMPLE

DWORD dwResult;    
for (DWORD i=0; i< XUSER_MAX_COUNT; i++ )
{
    XINPUT_STATE state;
    ZeroMemory( &state, sizeof(XINPUT_STATE) );

    // Simply get the state of the controller from XInput.
    dwResult = XInputGetState( i, &state );

    if( dwResult == ERROR_SUCCESS )
    {
        // Controller is connected
    }
    else
    {
        // Controller is not connected
    }
}

FUNCTIONS

//Return value is ERROR_SUCCESS OR ERROR_DEVICE_NOT_CONNECTED
DWORD XInputGetState(
  [in]  DWORD        dwUserIndex,
  [out] XINPUT_STATE *pState
);

//LoadLibrary 
HMODULE LoadLibraryA(
  [in] LPCSTR lpLibFileName
);



STRUCTS

//
typedef struct _XINPUT_STATE {
    DWORD dwPacketNumber;       //packetnumber iterates with every input so it is possible to see if input is polled often enough
    XINPUT_GAMEPAD Gamepad;
} XINPUT_STATE, *PXINPUT_STATE;



//XINPUT_GAMEPAD
typedef struct _XINPUT_GAMEPAD {
  WORD  wButtons;
  BYTE  bLeftTrigger;
  BYTE  bRightTrigger;
  SHORT sThumbLX;
  SHORT sThumbLY;
  SHORT sThumbRX;
  SHORT sThumbRY;
} XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;


wButtons bit values:

    XINPUT_GAMEPAD_DPAD_UP	0x0001
    XINPUT_GAMEPAD_DPAD_DOWN	0x0002
    XINPUT_GAMEPAD_DPAD_LEFT	0x0004
    XINPUT_GAMEPAD_DPAD_RIGHT	0x0008
    XINPUT_GAMEPAD_START	0x0010
    XINPUT_GAMEPAD_BACK	0x0020
    XINPUT_GAMEPAD_LEFT_THUMB	0x0040
    XINPUT_GAMEPAD_RIGHT_THUMB	0x0080
    XINPUT_GAMEPAD_LEFT_SHOULDER	0x0100
    XINPUT_GAMEPAD_RIGHT_SHOULDER	0x0200
    XINPUT_GAMEPAD_A	0x1000
    XINPUT_GAMEPAD_B	0x2000
    XINPUT_GAMEPAD_X	0x4000
    XINPUT_GAMEPAD_Y	0x8000









*/

