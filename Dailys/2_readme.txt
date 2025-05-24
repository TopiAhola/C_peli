Window functions and structs had to be made using ANSI versions for some reason:
WNDCLASSA
RegisterClassA
CreateWindowExA

Otherwise strings (const wchar_t *) wouldn't work...