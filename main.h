#ifndef main_h
#define main_h

#include <SDL3/SDL.h>
#include <SDL3/SDL_image.h>
#include <SDL3/SDL_ttf.h>
#include <SDL3/SDL_mixer.h>

void updateWindowResolution();

void TitleLoop();

void ExplorationLoop();

void CombatLoop();

void toggleDevmode();

void toggleFullscreen();

void resetUnremarkableData();

bool segmentsInSamePlace(edgeInfo seg1, edgeInfo seg2, float tolerance);

void sortEdges(std::vector<edgeInfo>& edges, float px, float py); 

void drawUI();

#endif
