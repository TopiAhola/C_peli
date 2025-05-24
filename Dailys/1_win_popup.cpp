#include <windows.h>
// Compiler näyttää automaattisesti laittavan User32.lib kirjaston COMPILERIN asetuksiin joten se löytää MessageBoxin?

int CALLBACK WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance, 
    LPSTR lpCmdLine,
    int CmdShow)
    {
    MessageBoxA(
        0,
        "Hurraa!", 
        "Tässä on otsikko", // Ääkköset ei näy oikein! 
        MB_ICONEXCLAMATION|MB_SETFOREGROUND|MB_OK);
        // Jostain syystä MessageBoxW ei toimi 
        // vaikka sen pitäisi ymmärtää wide (16B) tekstiä ?

    OutputDebugStringA("Debugstringi");  // Minne tämä tulostaa?
    return 0;
}

