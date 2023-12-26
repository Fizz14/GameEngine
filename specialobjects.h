#ifndef specialobjects_h
#define specialobjects_h

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include "globals.h"
#include "objects.h"

void specialObjectsInit(entity* a);

void specialObjectsBump(entity* a, bool xcollide, bool ycollide);

void specialObjectsUpdate(entity * a, float elapsed);

void usableItemCode(usable* a);

#endif
