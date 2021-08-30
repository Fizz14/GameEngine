#include <iostream>
#include <vector>
#include <SDL2/SDL.h>        
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <algorithm>
#include <cmath> //pow
#include <math.h>  //sin()
#include <fstream> //loading
#include <vector>
#include <cctype> //make input lowercase for map console
#include <ctime>//debug clock
#include <string>
#include <map> //saves

#ifndef GLOBALS
#define GLOBALS

# define M_PI 3.14159265358979323846

using namespace std;

class coord;

class cshadow;

class actor;

class mapObject;

class entity;

class tile;

class door;

class rect;

class mapCollision;

class box;

class tri;

class textbox;

class ui;

class adventureUI;

class weapon;

class projectile;

class heightmap;

class navNode;

class worldsound;

class musicNode;

class cueSound;

class waypoint;

class trigger;

class listener;

vector<cshadow*> g_shadows;

vector<actor*> g_actors;

vector<mapObject*> g_mapObjects;

vector<entity*> g_entities;

vector<entity*> g_solid_entities;

vector<tile*> g_tiles;

vector<door*> g_doors;

vector<vector<box*>> g_boxs;

vector<textbox*> g_textboxes;

vector<ui*> g_ui;

vector<mapCollision*> g_mapCollisions;

vector<vector<tri*>> g_triangles;

vector<heightmap*> g_heightmaps;

vector<navNode*> g_navNodes;

vector<worldsound*> g_worldsounds;

vector<musicNode*> g_musicNodes;

vector<cueSound*> g_cueSounds;

vector<waypoint*> g_waypoints;

vector<trigger*> g_triggers;

vector<listener*> g_listeners;

vector<projectile*> g_projectiles;

vector<weapon*> g_weapons;

map<string, int> enemiesMap; //stores (file,cost) for enemies to be spawned procedurally in the map
int g_budget = 0; //how many points this map can spend on enemies;

bool boxsenabled = true; //affects both map editor and full game. Dont edit here

bool onionmode = 0; //hide custom graphics
bool freecamera = 0;
bool devMode = 0;
bool showDevMessages = 1;
bool showErrorMessages = 1;

//quick debug info
#define D(a) if(devMode && showDevMessages) {std::cout << #a << ": " << (a) << endl;}

template<typename T>
void M(T msg, bool disableNewline = 0) { if(!devMode || !showDevMessages) {return;} cout << msg; if(!disableNewline) { cout << endl; } }

//particularly for errors
template<typename T>
void E(T msg, bool disableNewline = 0) { if(!devMode || !showErrorMessages) {return;} cout << "ERROR: " << msg; if(!disableNewline) { cout << endl; } }

//for camera/window zoom
float scalex = 0.4;
float scaley = 0.4;
float min_scale = 0.1;
float max_scale = 2;
float old_WIN_WIDTH = 640; //used for detecting change in window width to recalculate scalex and y

//for visuals
float p_ratio = 1.151;
bool g_vsync = true;
SDL_Texture* background = 0;
bool g_backgroundLoaded = 0;
bool g_useBackgrounds = 1; //a user setting, if the user wishes to see black screens instead of colorful backgrounds
//x length times x_z_ratio is proper screen length in z
float XtoZ = 0.496; // 4/2.31, arctan (4/ 3.21) = 60 deg
float XtoY = 0.866;
float g_ratio = 1.618;
bool transition = 0;
int g_walldarkness = 55; //65, 75. could be controlled by the map unless you get crafty with recycling textures across maps
int g_platformResolution = 11; // 5, 11 //what size step to use for the tops of platforms. must be a factor of 55 I need this to be 11 for most players

int g_TiltResolution = 1; //1, 2, 4, 16 //what size step to use for triangular walls, 2 is almost unnoticable. must be a factor of 64
bool g_protagHasBeenDrawnThisFrame = 0;
//english
string g_font = "fonts/ShortStack-Regular.ttf";
//polish
//string g_font = "fonts/Itim-Regular.ttf";
//japanese
//string g_font = "fonts/GamjaFlower-Regular.ttf";
//chinese
//string g_font = "fonts/ZhiMangXing-Regular.ttf";
float g_fontsize = 0.031;
float g_transitionSpeed = 9; //3

//inventory
float attack_cooldown = 0;
vector<weapon*> AdventureWeaponSet;

//draw player thrue wall if he is covered
bool drawProtagGlimmer = 0;
//for each corner of protag image
bool protagGlimmerA = 0;
bool protagGlimmerB = 0;
bool protagGlimmerC = 0;
bool protagGlimmerD = 0;

//physics
float g_gravity = 290;

 

class camera {
public:
	float oldx = 0;
	float oldy = 0;
	float x = 200;
	float y = 200;
	float width = 640;
	float height = 480;
	float lag = 0.0;
	const float DEFAULTLAGACCEL = 0.01;
	float lagaccel = 0.01; //how much faster the camera gets while lagging
	float zoom = 1;
	float zoommod=1;
	int lowerLimitX = 0;
	int lowerLimitY = 0;
	int upperLimitX = 3000;
	int upperLimitY = 3000;
	bool enforceLimits = 0;

	camera(float fx, float fy) {
		fx=x;
		fy=y;
	}
	void update_movement(float elapsed, float targetx, float targety) {
		if(!isfinite(targetx) || !isfinite(targety) ) { return; }
		
		if(lag == 0) {
			x=targetx;
			y=targety;
		} else {
			x += (targetx-oldx)  * (elapsed / 256) * lag;
			y += (targety-oldy)  * (elapsed / 256) * lag;

			oldx=x;
			oldy=y;	
			//if we're there, within a pixel, set the lagResetTimer to nothing
			if(abs(targetx - x) < 1.4 && abs(targety - y) < 1.4) {
				lag = 0;
			} else {
				//if not, consider increasing lag to catch up
				lag += lagaccel;
			}
		}
		

		if(enforceLimits) {
			if(x < lowerLimitX) { x = lowerLimitX ; }
			if(y < lowerLimitY) { y = lowerLimitY ; }

			if(x + width > upperLimitX) { x = upperLimitX - width; }
			if(y + height > upperLimitY) { y = upperLimitY - height; }
		}
	}

	void resetCamera() {
		enforceLimits = 0;
		lowerLimitX = 0;
		lowerLimitY = 0;
		upperLimitX = 0;
		upperLimitY = 0;
	}
};

//int WIN_WIDTH = 640; int WIN_HEIGHT = 480;
int WIN_WIDTH = 1280; int WIN_HEIGHT = 720;
//int WIN_WIDTH = 640; int WIN_HEIGHT = 360;
SDL_Window * window;
SDL_DisplayMode DM;
bool g_fullscreen = false;
camera g_camera(0,0);
entity* protag;

entity* g_focus;
vector<entity*> party;
float g_max_framerate = 120;
float g_min_frametime = 1/g_max_framerate * 1000;
SDL_Event event;
float ticks, lastticks, elapsed = 5, halfsecondtimer;
float camx = 0;
float camy = 0;
SDL_Renderer * renderer;
string g_map = "title";

//input
const Uint8* keystate = SDL_GetKeyboardState(NULL);
bool devinput[50] = {false};
bool input[16] = {false};
bool oldinput[16] = {false};
SDL_Scancode bindings[16];
bool left_ui_refresh = false; //used to detect when arrows is pressed and then released
bool right_ui_refresh = false;
bool quit = false;
string config = "default";
bool g_holdingCTRL = 0;

//sounds and music
float g_volume = 0;
bool g_mute = 0;
Mix_Chunk* g_ui_voice = Mix_LoadWAV("sounds/voice-normal.wav");
musicNode* closestMusicNode;
musicNode* newClosest;
int musicFadeTimer = 0;
bool fadeFlag = 0;
int musicUpdateTimer = 0;

//ui
bool protag_can_move = true;
int protag_is_talking = 0; //0 - not talking 1 - talking 2 - about to stop talking
adventureUI* adventureUIManager;
float textWait = 50; //seconds to wait between typing characters of text
float text_speed_up = 1; //speed up text if player holds button. set to 1 if the player isn't pressing it, or 1.5 if she is
float curTextWait = 0;
bool old_z_value = 1; //the last value of the z key. used for advancing dialogue, i.e. z key was down and is now up or was up and is now down if old_z_value != SDL[SDL_SCANCODE_Z]
float dialogue_cooldown = 0; //seconds until he can have dialogue again.

//debuging
SDL_Texture* nodeDebug;
clock_t debugClock;
string g_lifecycle = "Pre-Alpha";

//world
int g_layers = 12; //max blocks in world
float g_bhoppingBoost = 1; //the factor applied to friction whilst bhopping, not good, basically just airspeed modifier rn

//map editing
bool g_mousemode = 1;
entity* nudge = 0; //for nudging entities while map-editing
bool adjusting = 0; //wether to move selected entity or change its hitbox/shadow position

//userdata
string g_saveName = "A";

std::map<string, int> g_save = {};

int loadSave(string address) {
	g_save.clear();
	ifstream file;
	string line;

	address = "user/saves/" + address + ".txt";
	D(address);
	const char* plik = address.c_str();
	file.open(plik);
	
	string field = "";
	string value = "";

	//load fields
	while(getline(file, line)) {
		field = line.substr(0, line.find(' '));
		value = line.substr(line.find(" "), line.length()-1);
		D(value + "->" + field);
		try {
			g_save.insert( pair<string, int>(field, stoi(value)) );
		} catch(...) {
			M("Error writing");
			return -1;
		}
	}
	file.close();
	return 0;
}

int writeSave(string address) {
	ofstream file;
	
	address = "user/saves/" + address + ".txt";
	const char* plik = address.c_str();
	file.open(plik);

	auto it = g_save.begin();

	while (it != g_save.end() ) {
		file << it->first << " " << it->second << endl;
		D(it->first);
		D(it->second);
		M("---");
		it++;
	}
	file.close();
	return 0;
}

int checkSaveField(string field) {
	std::map<string, int>::iterator it = g_save.find(field);
	if(it != g_save.end()) {
		return it->second;
	} else {
		return 0;
	}
}

void writeSaveField(string field, int value) {
	auto it = g_save.find(field);

	if (it == g_save.end()) {
		g_save.insert(std::make_pair(field, value));
	} else {
		g_save[field] = value;
	}
}

bool fileExists (const std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);	
        return true;
    } else {
        return false;
    }   
}

//combat
enum Status { none, stunned, slowed, buffed, marked };

void playSound(int channel, Mix_Chunk* sound, int loops) {
	if(!g_mute && sound != NULL) {
		Mix_PlayChannel(channel, sound, loops);
	}
}

SDL_Texture* MaskTexture(SDL_Renderer* renderer, SDL_Texture* mask, SDL_Texture* diffuse) {
	int w, h;
	SDL_QueryTexture(diffuse, NULL, NULL, &w, &h);
	SDL_Texture* result = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
	SDL_SetTextureBlendMode(result, SDL_BLENDMODE_BLEND);
	SDL_SetRenderTarget(renderer, result);

	SDL_SetTextureBlendMode(mask, SDL_BLENDMODE_MOD);
	SDL_SetTextureBlendMode(diffuse, SDL_BLENDMODE_NONE);

	SDL_SetRenderDrawColor(renderer, 0,0,0,0);
	SDL_RenderClear(renderer);

	SDL_RenderCopy(renderer, diffuse, NULL, NULL);
	SDL_RenderCopy(renderer, mask, NULL, NULL);

	SDL_SetRenderTarget(renderer, NULL);
	return result;
}

float Distance(int x1, int y1, int x2, int y2) {
	return pow(pow((x1 -x2), 2) + pow((y1 - y2), 2), 0.5);
}

template<class T>
navNode* getNodeByPosition(vector<T*> array, int fx, int fy) {
	//this is a placeholder solution for testing AI
	//this requires a binary search and sorted nodes to work reasonably for larger maps
	float min_dist = 0;
		navNode* ret;
		bool flag = 1;

		//just pitifully slow
		for (int i = 0; i < array.size(); i++) {
			float dist = Distance(fx, fy, array[i]->x, array[i]->y);
			if(dist < min_dist || flag) {
				min_dist = dist;
				ret = array[i];
				flag = 0;
			}
		}
		return ret;
}

//get cardinal points about a position
// 0 is 12oclock, and 2 is 3oclock and so on
vector<int> getCardinalPoint(int x, int y, float range, int index) {
	float angle = 0;
	switch(index) {
		case 6:
			angle = 0;
			break;
		case 7:
			angle = M_PI/4;
			break;
		case 0:
			angle = M_PI/2;
			break;
		case 1:
			angle = M_PI * (3.0/4.0);
			break;
		case 2:
			angle = M_PI;
			break;
		case 3:
			angle = M_PI * (5.0/4.0);
			break;
		case 4:
			angle = M_PI * (3.0/2.0);
			break;
		case 5:
			angle = M_PI * (7.0/4.0);
			break;
	}
	vector<int> ret;
	ret.push_back( x +( range * cos(angle)));
	ret.push_back(y + (range * sin(angle) * XtoY));
	return ret;
}

//measures distance in the world, not by the screen.
float XYWorldDistance(int x1, int y1, int x2, int y2) {
	y1 *= 1/XtoY;
	y2 *= 1/XtoY;
	return pow(pow((x1 -x2), 2) + pow((y1 - y2), 2), 0.5);
}

vector<string> splitString (string s, char delimiter) {
    int start, end;
	start = 0;
    string token;
    vector<string> ret;
	end = s.find(delimiter, start);
    while (end != string::npos) {
        token = s.substr (start, end - start);
        start = end + 1;
        ret.push_back (token);
		end = s.find(delimiter, start);
    }

    ret.push_back (s.substr (start));
    return ret;
}

int yesNoPrompt(string msg) {
	//warn for file overwrite
	const SDL_MessageBoxButtonData buttons[] = {
		{ /* .flags, .buttonid, .text */        0, 0, "Yes" },
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "No" },
	};
	const SDL_MessageBoxColorScheme colorScheme = {
		{ /* .colors (.r, .g, .b) */
			/* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
			{ 0,   0,   0 },
			/* [SDL_MESSAGEBOX_COLOR_TEXT] */
			{   200, 200, 200 },
			/* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
			{ 180, 180, 180 },
			/* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
			{   0,  0, 0 },
			/* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
			{  255, 255, 255 }
		}
	};
	const SDL_MessageBoxData messageboxdata = {
		SDL_MESSAGEBOX_INFORMATION, /* .flags */
		NULL, /* .window */
		"", /* .title */
		msg.c_str(), /* .message */
		SDL_arraysize(buttons), /* .numbuttons */
		buttons, /* .buttons */
		&colorScheme /* .colorScheme */
	};
	int buttonid;
	if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0) {
		SDL_Log("error displaying message box");
	}
	return buttonid;
}




#endif
