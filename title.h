#ifndef title_h
#define title_h

#include <iostream>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_image.h>
#include <SDL3/SDL_ttf.h>
#include <SDL3/SDL_mixer.h>
#include <string>
#include <unordered_map>

class ui;

class textbox;

class titleUI {
public:
  int bleh;
  ui* panel;
  ui* handMarker;
  ui* title;
  ui* titles;
  ui* titleExplosion;
  ui* bg;
  textbox* newText;
  textbox* continueText;
  textbox* endText;
  textbox* creditText;
  
  float handYOffset = 0.04;

  int option = 0;

  void hideAll();
  void showAll();

  titleUI(SDL_Renderer* renderer);
  ~titleUI();
};

void getTitleInput();

void TitleLoop();


#endif
