How to get this code working on windows:

get SDL2_image.dll, SDL2_mixer.dll, SDL2_ttf.dll, SDL2.dll, zlib1.dll, libogg-0.dll, libfreetype-6.dll, libvorbis-0.dll, and libvorbisfile-3.dll and put them with the code.

You need lots of textures and configuration files to get it to work, check the googledrive.

Build with g++ main.cpp -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2  -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -Wno-narrowing; start a.exe;

make sure that the textures in textures/lighting are bmp and not png, this should be updated soon for the linux version.

