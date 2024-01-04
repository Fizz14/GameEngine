all: globals.o lightcookies.o main.o map_editor.o objects.o specialobjects.o
	g++ main.o objects.o map_editor.o specialobjects.o lightcookies.o globals.o -std=c++17 -o out.exe -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -O0 -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1

globals.o: globals.cpp
	g++ globals.cpp -flto -std=c++17 -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1

lightcookies.o: lightcookies.cpp
	g++ lightcookies.cpp -flto -std=c++17 -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1

main.o: main.cpp
	g++ main.cpp -std=c++17 -flto -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1

map_editor.o: map_editor.cpp
	g++ map_editor.cpp -flto -std=c++17 -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1

objects.o: objects.cpp
	g++ objects.cpp -flto -std=c++17 -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1

specialobjects.o: specialobjects.cpp
	g++ specialobjects.cpp -flto -std=c++17 -c -Wno-narrowing -LC:\Users\Vrickt\OneDrive\Documents\dev\lib\x64\SDL2 -IC:\Users\Vrickt\OneDrive\Documents\dev\include -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -g -fmax-errors=1