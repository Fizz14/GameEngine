A game-engine written in C++ with SDL3 and Autofs.


64b Win 11, powershell build with:

g++ objects.cpp -std=c++17 -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2  -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1; [console]::beep(500,300);
g++ map_editor.cpp -std=c++17 -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2  -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1; [console]::beep(500,300);
g++ lightcookies.cpp -std=c++17 -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2  -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1; [console]::beep(500,300);
g++ globals.cpp -std=c++17 -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2  -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1; [console]::beep(500,300);
g++ main.cpp -std=c++17 -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2  -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1; [console]::beep(500,300);
g++ main.o objects.o map_editor.o lightcookies.o globals.o -std=c++17 -o out.exe -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2  -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1; [console]::beep(500,300);
gdb ./out.exe


To port to linux:

-change WinMain() to main()

-initialize g_linux to 1 in globals.cpp

-change line endings for saves/configs
