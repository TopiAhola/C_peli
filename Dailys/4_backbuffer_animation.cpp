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
//TODO: some temp variables:
global_static int bitmap_width;
global_static int bitmap_height;

/////////////////////////////////////////////////////////////
//FUNCTIONS


internal void
draw_test(int loop){
    //Draw something
    int pitch = bitmap_width*4;  //lenght of a row in bitmap
    //stride is?
    int intensity = loop % 255;
    OutputDebugStringA("draw test");

    UINT8 * p_row = (UINT8 *)p_bitmap_memory; //cast void bitmap pointer to 8 BIT INT
    for(int y=0; y<bitmap_height; y++)
    {
        UINT32 * p_pixel = (UINT32*)p_row;     //recast to 32 bit to point argb values
        for(int x=0; x<bitmap_width; x++)
        {
            //p_pixel = p_pixel + x;    //iterates over pixels, SEGFAULTS

            //WINDOWS has colors BGRx but this has xRGB but little endian!
            //*p_pixel = 0x00500000;      //argb LITTLE ENDIAN MEANS VALUES HAVE SMALLER NUMBERS FIRST
            //It writes smallest bit to first byte so the hexa is still xRGB when read with 32 bit pointer
            
            //ANIMATED PER COLOR VERSION     

            UINT8 * p_color = (UINT8 *)p_pixel;
            *p_color = intensity; //blue
            p_color++;
            *p_color = 0; //green to upper left
            p_color++;
            *p_color = (200*x*y/bitmap_width/bitmap_height); //red bottom right
            p_color++;
            *p_color = 0; //buffer maybe use for some effect later
            p_color++;

            p_pixel++;
        }
        p_row += pitch; //Moves pointer to next row
    }

}


////////////////////////////////
internal void 
resize_dib_section(int width, int height){
//Function to be called when window is resized. Resizes global static bitmapinfo
    if(p_bitmap_memory){
        VirtualFree(p_bitmap_memory, 0, MEM_RELEASE);
    }

    bitmapinfo.bmiHeader.biSize = sizeof(bitmapinfo.bmiHeader);
    bitmapinfo.bmiHeader.biWidth = width;
    bitmapinfo.bmiHeader.biHeight = -height;        //Negative height defines bitmap starts top-left corner (positive is bottom-left)
    bitmapinfo.bmiHeader.biPlanes = 1;
    bitmapinfo.bmiHeader.biBitCount = 32;
    bitmapinfo.bmiHeader.biCompression = BI_RGB;
    bitmapinfo.bmiHeader.biSizeImage = 0;
    bitmapinfo.bmiHeader.biXPelsPerMeter = 0;       //For printing? Not important.
    bitmapinfo.bmiHeader.biYPelsPerMeter = 0;
    bitmapinfo.bmiHeader.biClrUsed = 0;
    bitmapinfo.bmiHeader.biClrImportant = 0;
    //Setting the temp globals: TODO: remove
    bitmap_width = width;
    bitmap_height = height;
    //TODO: check whether bitmap_devicecontext exsist before assingning it? Temp it is global wut that will change

    //Manual memory allocation for bitmap:
    int bytes_per_pixel = 4;    //32 bit argb
    int bitmap_memory_size = width*height*bytes_per_pixel;
    p_bitmap_memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
    if(p_bitmap_memory){
        OutputDebugStringA("Bitmap memory reallocated:");
    }
    
}
////////////////////////////////

//Function to update window Copies a rectangle to another rect, scales if needed
internal void 
update_window(HDC devicecontext, RECT *window_rect ,int x, int y, int w, int h ){
    int window_width = window_rect->right - window_rect->left;
    int window_height = window_rect->bottom - window_rect->top;


    StretchDIBits(devicecontext,
        x, y, window_width, window_height,       //destination xywh
        x, y, bitmap_width, bitmap_height,       //source  xywh
        p_bitmap_memory,    //pointer to bitmap location
        &bitmapinfo,        //pointer to bitmapinfo
        DIB_RGB_COLORS,    //usage  
        SRCCOPY            //raster operation
    );



}
////////////////////////////////

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
                RECT client_rect;
                GetClientRect(window, &client_rect); 

                HDC devicecontext1 = BeginPaint(window, &paintstruct1);
                int x = paintstruct1.rcPaint.top;
                int y = paintstruct1.rcPaint.left;            
                int w = paintstruct1.rcPaint.right - paintstruct1.rcPaint.left;
                int h = paintstruct1.rcPaint.bottom - paintstruct1.rcPaint.top;
                static DWORD raster_code = WHITENESS;       // static remembers its value like a global but is still limited in scope
                update_window(devicecontext1,&client_rect,x,y,w,h );
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


    
    if(RegisterClassA(&my_windowclass))
    { // Class has to be registered ? Function retruns an "ATOM" type thing but it isnt needed?

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
            
        if (windowhandle) 
        {                                  
            program_running = true;  
            MSG message;
            BOOL return_message = 0;

            while(program_running)
            {
                //message loop
                while(return_message = PeekMessage(        //PeekMessage gets messages if any
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
                        Sleep(500);
                }

                //first animation
                OutputDebugStringA("loop");
                Sleep(500);

   
            }

        } //TODO WINDOWHANDLE ERROR      
    } //TODO: Registerclass error 
    return 0;
}


/*
New Windows Functions

//Alloc inside the memspace of the calling process
//Use VirtualAllocEx to allocate to a different process
//Does other mem management stuff, see documentation
LPVOID VirtualAlloc(
  [in, optional] LPVOID lpAddress,
  [in]           SIZE_T dwSize,
  [in]           DWORD  flAllocationType,
  [in]           DWORD  flProtect
);

//HeapAlloc() is also a allocator

//
BOOL VirtualFree(
  [in] LPVOID lpAddress,
  [in] SIZE_T dwSize,       //has to be 0 if releasin mem allocated by VirtualAlloc()
  [in] DWORD  dwFreeType    // MEM_RELEASE OR MEM_DECOMMIT
);

//VirtualProtect() can be used to lock memory to prevent wirtes from
undeleted pointers, which is a bug





//Non blocking message get
BOOL PeekMessageA(
  [out]          LPMSG lpMsg,
  [in, optional] HWND  hWnd,
  [in]           UINT  wMsgFilterMin,
  [in]           UINT  wMsgFilterMax,
  [in]           UINT  wRemoveMsg
);


*/

