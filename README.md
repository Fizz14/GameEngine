A game-engine written in C++ with SDL3 and Autofs.

To port to linux:
-change WinMain() to main()
-initialize g_linux to 1 in globals.cpp
-change line endings for saves/configs
