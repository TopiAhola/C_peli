gcc ^
-g C:\Repositiot\c_peli\C_peli\Build_environment\Build_current\win32_game.cpp ^
-o C:\Repositiot\c_peli\C_peli\Build_environment\Build_current\win32_game_version3.exe ^
-L C:\Koodaus\C_compilers\MSYS2\ucrt64\lib -l gdi32 -l user32 -l onecore -l xinput ^
-D DEVELOPER_BUILD=1 -D PERFORMANCE_MODE=0^
-Wall ^
-Wextra ^
-I C:\Koodaus\C_compilers\MSYS2\ucrt64\include ^
-I C:\Repositiot\c_peli\C_peli\Build_environment\Build_current ^
-fdiagnostics-color=always