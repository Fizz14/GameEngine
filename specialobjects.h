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

int specialObjectsInteract(entity* a); //return 1 to break out of the interaction, needed if the call of specialObjectsInteract did a map load and therefore deleted data

void specialObjectsOncePerFrame(float elapsed);

void specialObjectsMapWrite(entity* a, ofstream & ofile); //any entity which writes it's entitydata to the map needs a branch here

void specialObjectsRender(entity* a, SDL_FRect dstrect);

float exponentialCurve(int max, int exponent);

#endif
