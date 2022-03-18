Specifically, how to get this code working on windows:

get SDL2_image.dll, SDL2_mixer.dll, SDL2_ttf.dll, and SDL2.dll and put them with the code.

Build with g++ main.cpp -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2  -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -Wno-narrowing; start a.exe;

make sure that the textures in textures/lighting are bmp and not png, this should be updated soon for the linux version.

List of things to catch the linux-branch up on:

add error-checking for making an entity roam, in the /roam script-instruction AND when they roam, to make sure they have points to roam to. That way, if an entity is told to roam and there are no points, they won't crash the game and they won't stop chasing someone to do nothing.

add support for when entities have abilities with equal max cooldowns and min cooldowns, to just set it to min. (or when maxcooldown - mincooldown <= 0, use mincd)
