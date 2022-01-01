dev:
	g++ main.cpp -std=c++17 -o carbin.out -Wno-narrowing -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf; ./carbin.out 1

game:
	g++ main.cpp -std=c++17 -o carbin.out -Wno-narrowing -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf; ./carbin.out

build:
	g++ main.cpp -std=c++17 -o carbin.out -Wno-narrowing -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf;

test:
	./carbin.out 1 0

play:
	./carbin.out

debug:
	g++ main.cpp -std=c++17 -o carbin.out -Wno-narrowing -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf; gdb carbin.out

leaks:
	valgrind ./carbin.out 1 --show-leak-kinds=definite

gleaks:
	valgrind ./carbin.out 0 --show-leak-kinds=definite

windows:
	x86_64-w64-mingw32-g++ main.cpp -g -o carbin.out -L\usr\include -Wno-narrowing -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf; 

cookies:
	g++ lightcookietesting.cpp -std=c++17 -o cookies.out -Wno-narrowing -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf; ./cookies.out