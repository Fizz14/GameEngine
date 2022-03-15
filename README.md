Specifically, how to get this code working on windows:

get SDL2_image.dll, SDL2_mixer.dll, SDL2_ttf.dll, and SDL2.dll and put them with the code.

Build with g++ main.cpp -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2  -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -Wno-narrowing; start a.exe;

make sure that the textures in textures/lighting are bmp and not png, this should be updated soon for the linux version.
