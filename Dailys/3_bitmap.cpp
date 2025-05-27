/////////////////////////////////////////////////////////////
//HEADER FILES
#include <windows.h>

/////////////////////////////////////////////////////////////
//DEFINES
#define local_static static   //2 names for static because static is scope dependent
#define global_static static  // for global statics
#define internal static       //static has 3rd meaning with functions limiting their callability

/////////////////////////////////////////////////////////////
//GLOBALS
global_static bool program_running;      //this is toggled false when program needs to quit

global_static BITMAPINFO bitmapinfo;    //TODO: bitmap stuff is here temporarily
global_static void * p_bitmap_memory;
global_static HBITMAP bitmaphandle;
global_static HDC bitmap_devicecontext;

/////////////////////////////////////////////////////////////
//FUNCTIONS



internal void resize_dib_section(int width, int height){
//Function to be called when window is resized. Resizes global static bitmapinfo

    if(bitmaphandle){       //Deletes old bitmaphandle to free memory
        DeleteObject(bitmaphandle);
    }

    //TODO: better memory management. Reusing old bitmaps or smt?
     
    bitmapinfo.bmiHeader.biSize = sizeof(bitmapinfo.bmiHeader);
    bitmapinfo.bmiHeader.biWidth = width;
    bitmapinfo.bmiHeader.biHeight = height;
    bitmapinfo.bmiHeader.biPlanes = 1;
    bitmapinfo.bmiHeader.biBitCount = 32;
    bitmapinfo.bmiHeader.biCompression = BI_RGB;
    bitmapinfo.bmiHeader.biSizeImage = 0;
    bitmapinfo.bmiHeader.biXPelsPerMeter = 0;       //For printing? Not important.
    bitmapinfo.bmiHeader.biYPelsPerMeter = 0;
    bitmapinfo.bmiHeader.biClrUsed = 0;
    bitmapinfo.bmiHeader.biClrImportant = 0;

    //TODO: check whether bitmap_devicecontext exsist before assingning it? Temp it is global wut that will change

    bitmap_devicecontext = CreateCompatibleDC(0);  //Creates a temp DC that is compatible with the screens DC...
    bitmaphandle = CreateDIBSection(
        bitmap_devicecontext,          // hdc,
        &bitmapinfo,            // pointer to bitmapinfo
        DIB_RGB_COLORS,         //usage,
        &p_bitmap_memory,          // **ppvBits, pointer to the void pointer
        0,                      // hSection,         if thi is NULL windows will allocate memory for the DIB
        0                       // offset
    );
    ReleaseDC(0,bitmap_devicecontext);      
}

//Function to update window Copies a rectangle to another rect, scales if needed
internal void update_window(HDC devicecontext,int x, int y, int w, int h ){
    StretchDIBits(devicecontext,
        x, y, w, h,       //destination
        x, y, w, h,       //source
        p_bitmap_memory,    //pointer to bitmap location
        &bitmapinfo,        //pointer to bitmapinfo
        DIB_RGB_COLORS,    //usage  
        SRCCOPY            //raster operation
    );

}
////////////////////////////////

//This function is for the WNDCLASS 
//Windows can use this when it wants. Main program can call this too, for example WM_PAINT to draw the window. 
LRESULT CALLBACK main_window_callback(
    HWND window,                //handle to window in question
    UINT message,               //Search for: System-Defined Messages 
    WPARAM wparam,
    LPARAM lparam )
    {   
        LRESULT result = 0;
        switch(message){
            case WM_ACTIVATEAPP: {OutputDebugStringA("WM_ACTIVATEAPP\n");} break;

            case WM_SIZE: {
                RECT client_rect;
                GetClientRect(window, &client_rect);        //clientrect is the rectangle inside a window program can draw to (borders not included, fullscreen has no borders)
                int width = client_rect.right - client_rect.left;            
                int height = client_rect.bottom - client_rect.top;
                resize_dib_section(width, height);
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
                HDC devicecontext1 = BeginPaint(window, &paintstruct1);
                int x = paintstruct1.rcPaint.top;
                int y = paintstruct1.rcPaint.left;            
                int w = paintstruct1.rcPaint.right - paintstruct1.rcPaint.left;
                int h = paintstruct1.rcPaint.bottom - paintstruct1.rcPaint.top;
                static DWORD raster_code = WHITENESS;       // static remembers its value like a global but is still limited in scope
                update_window(devicecontext1,x,y,w,h );
                EndPaint(window, &paintstruct1);
        
            } break;


            default: {
                OutputDebugStringA("default\n");
                result = DefWindowProc(window, message, wparam, lparam);  //Calling the windows default window procedure for some of the messages DefWindowProc
            } break;
        }
        return result;
    }
////////////////////////////////



/////////////////////////////////////////////////////////////
//WINMAIN
int CALLBACK WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance, 
    LPSTR lpCmdLine,
    int CmdShow)
    {
    WNDCLASSA my_windowclass = {} ;   //WNDCLASS my_windowclass -class Kokeillaan WNDCLASS+W

    my_windowclass.style = CS_CLASSDC|CS_HREDRAW|CS_VREDRAW;  //Check explanation at the bottom
    my_windowclass.lpfnWndProc = main_window_callback;
    my_windowclass.hInstance;  
    my_windowclass.lpszClassName = "MyWindowClass"; //Väärä tyyppi? compiler onnistuu kuitenkin?


    
    if(RegisterClassA(&my_windowclass)){ // Class has to be registered ? Function retruns an "ATOM" type thing but it isnt needed?

        HWND windowhandle = CreateWindowExA(
            0,                                  //extended styles 0 = nothing
            my_windowclass.lpszClassName,       //classname
            "MyWindowClass",                         //name that may? overwrite class name
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
            
        if (windowhandle) {         //CreateWindowExW returns a handle to new window or NULL if fails.
            program_running = true;  
            MSG message;
            while(program_running){
                BOOL message_error = GetMessage(        //GetMessage gets messages from threads messaging queue Blocks something if no messages are available? 
                    &message,                           //Pointer to a location to put message into
                    0,                                  //Handle. 0 means getting messagess for all windows?
                    0,                                  //Filter for messages Can filter keyboard and mouse messages?
                    0                                   //Filter max at 0 means no filtering
                );                                      //Getmessage writes message to the given pointer and returns a "BOOL", -1 is error?
                
                if(message_error > 0){                  //Microsoft BOOL is not a actual boolean! Its integer
                    TranslateMessage(&message);                 //TODO Translatemessage 
                    DispatchMessage(&message);                  //
                } else{
                    //windowhandle is NULL or -1 (error)
                    break;
                }
                
            }
        }
    } else {} //Registerclass error
    return 0;
}


/*
New Windows Functions

void PostQuitMessage(
  [in] int nExitCode
);


//Creates a DIB
HBITMAP CreateDIBSection(
  [in]  HDC              hdc,
  [in]  const BITMAPINFO *pbmi,
  [in]  UINT             usage,
  [out] VOID             **ppvBits,
  [in]  HANDLE           hSection,
  [in]  DWORD            offset
);

//For sizing the DIB to the window:
int StretchDIBits(
  [in] HDC              hdc,
  [in] int              xDest,
  [in] int              yDest,
  [in] int              DestWidth,
  [in] int              DestHeight,
  [in] int              xSrc,
  [in] int              ySrc,
  [in] int              SrcWidth,
  [in] int              SrcHeight,
  [in] const VOID       *lpBits,
  [in] const BITMAPINFO *lpbmi,
  [in] UINT             iUsage,
  [in] DWORD            rop
);

//Creates bitmaps?
HBITMAP CreateDIBSection(
  [in]  HDC              hdc,
  [in]  const BITMAPINFO *pbmi,
  [in]  UINT             usage,
  [out] VOID             **ppvBits,
  [in]  HANDLE           hSection,
  [in]  DWORD            offset
);


//Returns a DC that is compatible with the argument, or if arg is 0 with the current screen
HDC CreateCompatibleDC(
  [in] HDC hdc
);

//Releases DC whatever that is
int ReleaseDC(
  [in] HWND hWnd,
  [in] HDC  hDC
);



*/


