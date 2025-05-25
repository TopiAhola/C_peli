#include <windows.h>
#include <winuser.h>
#include <wingdi.h>
#include <windef.h>


//This function is for the WNDCLASS 
LRESULT CALLBACK main_window_callback(
    HWND window,                //handle to window in question
    UINT message,               //Search for: System-Defined Messages 
    WPARAM wparam,
    LPARAM lparam )
    {   
    LRESULT result = 0;
    switch(message){
        case WM_ACTIVATEAPP: {OutputDebugStringA("WM_ACTIVATEAPP\n");} break;

        case WM_SIZE: {OutputDebugStringA("WM_SIZE\n");} break;

        case WM_NCDESTROY: {OutputDebugStringA("WM_NCDESTROY\n");} break;

        case WM_CLOSE: {OutputDebugStringA("WM_CLOSE\n");} break;

        case WM_PAINT: {
            PAINTSTRUCT paintstruct1;
            HDC devicecontext1 = BeginPaint(window, &paintstruct1);
            int x = paintstruct1.rcPaint.top;
            int y = paintstruct1.rcPaint.left;            
            int w = paintstruct1.rcPaint.right - paintstruct1.rcPaint.left;
            int h = paintstruct1.rcPaint.bottom - paintstruct1.rcPaint.top;
            PatBlt(devicecontext1,x,y,w,h,PATCOPY); // (handle x,y,w,h , raster code)
            EndPaint(window, &paintstruct1);
            
        } break;


        default: {
            OutputDebugStringA("default\n");
            result = DefWindowProc(window, message, wparam, lparam);  //Calling the windows default window procedure for some of the messages DefWindowProc
        } break;
    }
    
    return result;
    }


int CALLBACK WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance, 
    LPSTR lpCmdLine,
    int CmdShow)
    {
    WNDCLASSA my_windowclass = {} ;   //WNDCLASS my_windowclass -class Kokeillaan WNDCLASS+W

    my_windowclass.style = CS_CLASSDC|CS_HREDRAW|CS_VREDRAW;  //Check explanation at the bottom
    my_windowclass.lpfnWndProc = main_window_callback;
    //my_windowclass.cbClsExtra;
    //my_windowclass.cbWndExtra;
    my_windowclass.hInstance;
    //my_windowclass.hIcon;
    //my_windowclass.hCursor;
    //my_windowclass.hbrBackground;
    //my_windowclass.lpszMenuName;      
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
                MSG message;
                while(1){
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
                        //windowhandle is NULL
                        break;
                    }
                
                }
        
        }
    } else {} //Registerclass error

    return 0;
}







/*
// WNDCLASS struct for making a window class
typedef struct tagWNDCLASSA {
  UINT      style;
  WNDPROC   lpfnWndProc;
  int       cbClsExtra;
  int       cbWndExtra;
  HINSTANCE hInstance;
  HICON     hIcon;
  HCURSOR   hCursor;
  HBRUSH    hbrBackground;
  LPCSTR    lpszMenuName;
  LPCSTR    lpszClassName;
} WNDCLASSA, *PWNDCLASSA, *NPWNDCLASSA, *LPWNDCLASSA;


// WNDPROC function for window procedures for my WNDCLASS:
WNDPROC Wndproc;

LRESULT Wndproc(
  HWND unnamedParam1,
  UINT unnamedParam2,
  WPARAM unnamedParam3,
  LPARAM unnamedParam4
)
{...}



// Default window procedure function for all procedures the program doesn't want to do itself:
LRESULT DefWindowProcA(
  [in] HWND   hWnd,
  [in] UINT   Msg,
  [in] WPARAM wParam,
  [in] LPARAM lParam
);

// Function for creating windows with extendex styles (Ex)
HWND CreateWindowExW(
  [in]           DWORD     dwExStyle,
  [in, optional] LPCWSTR   lpClassName,
  [in, optional] LPCWSTR   lpWindowName,
  [in]           DWORD     dwStyle,
  [in]           int       X,
  [in]           int       Y,
  [in]           int       nWidth,
  [in]           int       nHeight,
  [in, optional] HWND      hWndParent,
  [in, optional] HMENU     hMenu,
  [in, optional] HINSTANCE hInstance,
  [in, optional] LPVOID    lpParam
);


// GetMessage for getting something somewhere
BOOL GetMessage(
  [out]          LPMSG lpMsg,
  [in, optional] HWND  hWnd,
  [in]           UINT  wMsgFilterMin,
  [in]           UINT  wMsgFilterMax
);



//Begin- and EndPaint to paint stuff into window
HDC BeginPaint(
  [in]  HWND          hWnd,
  [out] LPPAINTSTRUCT lpPaint
);

BOOL EndPaint(
  [in] HWND              hWnd,
  [in] const PAINTSTRUCT *lpPaint
);

//PAINTSTRUCT contains info for Paint functions
typedef struct tagPAINTSTRUCT {
  HDC  hdc;
  BOOL fErase;
  RECT rcPaint;
  BOOL fRestore;
  BOOL fIncUpdate;
  BYTE rgbReserved[32];
} PAINTSTRUCT, *PPAINTSTRUCT, *NPPAINTSTRUCT, *LPPAINTSTRUCT;


//PatBlt paints rectangles....
BOOL PatBlt(
  [in] HDC   hdc,
  [in] int   x,
  [in] int   y,
  [in] int   w,
  [in] int   h,
  [in] DWORD rop
);





*/


// Styles for WNDCLASS class windows
/*
Constant/value	Description
CS_BYTEALIGNCLIENT
0x1000
Aligns the window's client area on a byte boundary (in the x direction). This style affects the width of the window and its horizontal placement on the display.
CS_BYTEALIGNWINDOW
0x2000
Aligns the window on a byte boundary (in the x direction). This style affects the width of the window and its horizontal placement on the display.
CS_CLASSDC
0x0040
Allocates one device context to be shared by all windows in the class. Because window classes are process specific, it is possible for multiple threads of an application to create a window of the same class. It is also possible for the threads to attempt to use the device context simultaneously. When this happens, the system allows only one thread to successfully finish its drawing operation.
CS_DBLCLKS
0x0008
Sends a double-click message to the window procedure when the user double-clicks the mouse while the cursor is within a window belonging to the class.
CS_DROPSHADOW
0x00020000
Enables the drop shadow effect on a window. The effect is turned on and off through SPI_SETDROPSHADOW. Typically, this is enabled for small, short-lived windows such as menus to emphasize their Z-order relationship to other windows. Windows created from a class with this style must be top-level windows; they may not be child windows.
CS_GLOBALCLASS
0x4000
Indicates that the window class is an application global class. For more information, see the "Application Global Classes" section of About Window Classes.
CS_HREDRAW
0x0002
Redraws the entire window if a movement or size adjustment changes the width of the client area.
CS_NOCLOSE
0x0200
Disables Close on the window menu.
CS_OWNDC
0x0020
Allocates a unique device context for each window in the class.
CS_PARENTDC
0x0080
Sets the clipping rectangle of the child window to that of the parent window so that the child can draw on the parent. A window with the CS_PARENTDC style bit receives a regular device context from the system's cache of device contexts. It does not give the child the parent's device context or device context settings. Specifying CS_PARENTDC enhances an application's performance.
CS_SAVEBITS
0x0800
Saves, as a bitmap, the portion of the screen image obscured by a window of this class. When the window is removed, the system uses the saved bitmap to restore the screen image, including other windows that were obscured. Therefore, the system does not send WM_PAINT messages to windows that were obscured if the memory used by the bitmap has not been discarded and if other screen actions have not invalidated the stored image.
This style is useful for small windows (for example, menus or dialog boxes) that are displayed briefly and then removed before other screen activity takes place. This style increases the time required to display the window, because the system must first allocate memory to store the bitmap.
CS_VREDRAW
0x0001
Redraws the entire window if a movement or size adjustment changes the height of the client area.

*/



/* C ja C++ difference, in basic C structs cannot be used as types without declaring they are 
types with typedef ??

C:
struct cat {
    string name;
    int age;
}

typedef cat cat;
cat nemi; 

C++:
struct cat {
    string name;
    int age;
}

cat nemi; 

*/
