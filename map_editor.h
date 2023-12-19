#ifndef mapeditor_h
#define mapeditor_h

#include <iostream>
#include <vector>
#include <fstream> //loading
#include <string>
#include "objects.h"
#include <cstring>   //loading
#include <sstream>   //loading
#include <algorithm> //sort
#include <fstream>
#include <filesystem> //making directories
#include <regex>      //for readings scripts


using namespace std;


// for sorting ui on mapload
int compare_ui(ui *a, ui *b);

void sort_ui(vector<ui *> &g_ui);

void populateMapWithEntities();

void load_map(SDL_Renderer *renderer, string filename, string destWaypointName);

void changeTheme(string str);

//add support to save effectIndexes to maps
//that way we don't have to load textures in 3 ms
bool mapeditor_save_map(string word);

// called on init if map_editing is true
void init_map_writing(SDL_Renderer *renderer);

// called every frame if map_editing is true
// each bool represents a button, if c is true, c was pressed that frame
void write_map(entity *mapent);

void close_map_writing();


#endif
