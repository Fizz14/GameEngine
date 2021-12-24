#ifndef globals_h
#define globals_h


#include <iostream>
#include <sstream>
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

class ramp;

class textbox;

class ui;

class adventureUI;

class attack;

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

class worldItem;

class indexItem;

class effectIndex;

class particle;

class collisionZone;

vector<cshadow*> g_shadows;

vector<entity*> g_entities;

vector<entity*> g_solid_entities;

vector<tile*> g_tiles;

vector<door*> g_doors;

vector<vector<box*>> g_boxs;

vector<textbox*> g_textboxes;

vector<ui*> g_ui;

vector<actor*> g_actors;

vector<mapObject*> g_mapObjects;

vector<mapCollision*> g_mapCollisions;

vector<vector<tri*>> g_triangles;

vector<vector<ramp*>> g_ramps;

vector<heightmap*> g_heightmaps;

vector<navNode*> g_navNodes;


struct cmpCoord {
    bool operator()(const pair<int, int> a, const pair<int, int> b) const {
		return a.first + a.second < b.first + b.second;
	}
};

map<pair<int, int>, navNode*, cmpCoord> navNodeMap;

vector<worldsound*> g_worldsounds;

vector<musicNode*> g_musicNodes;

vector<cueSound*> g_cueSounds;

vector<waypoint*> g_waypoints;

vector<trigger*> g_triggers;

vector<listener*> g_listeners;

vector<projectile*> g_projectiles;

vector<attack*> g_attacks;

vector<weapon*> g_weapons;

vector<worldItem*> g_worldItems;

vector<indexItem*> g_indexItems; 

vector<particle*> g_particles;

vector<collisionZone*> g_collisionZones;

map<string, int> enemiesMap; //stores (file,cost) for enemies to be spawned procedurally in the map
int g_budget = 0; //how many points this map can spend on enemies;

bool boxsenabled = 1; //affects both map editor and full game. Dont edit here

bool onionmode = 0; //hide custom graphics
bool genericmode = 0;
bool freecamera = 0;
bool devMode = 0;
bool canSwitchOffDevMode = 0;
bool inputRefreshCanSwitchOffDevMode = 0;
bool showDevMessages = 1;
bool showErrorMessages = 0;
bool showImportantMessages = 1;

//quick debug info
#define D(a) if(devMode && showDevMessages) {std::cout << #a << ": " << (a) << endl;}

template<typename T>
void M(T msg, bool disableNewline = 0) { if(!devMode || !showDevMessages) {return;} cout << msg; if(!disableNewline) { cout << endl; } }

//particularly for errors
template<typename T>
void E(T msg, bool disableNewline = 0) { if(!devMode || !showErrorMessages) {return;} cout << "ERROR: " << msg; if(!disableNewline) { cout << endl; } }

//Particularly important stuff, like loadtimes
template<typename T>
void I(T msg, bool disableNewline = 0) { if(!showImportantMessages) {return;} cout << "I: " << msg; if(!disableNewline) { cout << endl; } }

//Temporary debugging statements- I won't allow myself to block these
#define T(a) std::cout << #a << ": " << (a) << endl;


//for visuals
float p_ratio = 1.151;
bool g_vsync = true;
float g_background_darkness = 0; //0 - show bg, 1 - show black
SDL_Texture* background = 0;
bool g_backgroundLoaded = 0;
bool g_useBackgrounds = 1; //a user setting, if the user wishes to see black screens instead of colorful backgrounds
//x length times x_z_ratio is proper screen length in z
float XtoZ = 0.496; // 4/2.31, arctan (4/ 3.21) = 60 deg
float XtoY = 0.866;
float g_ratio = 1.618;
bool transition = 0;
int g_walldarkness = 55; //65, 75. could be controlled by the map unless you get crafty with recycling textures across maps
bool g_unlit = 0; //set to 1 if the user has the lowest graphical setting, to disable lighting in maps for performance. Don't eh, don't dev like this
int g_graphicsquality = 3; // 0 is least, 4 is max
float g_extraShadowSize = 20; //how much bigger are shadows in comparison to their hitboxes.
int g_fogofwarEnabled = 1;
int g_fogofwarRays = 100;

//for fow
SDL_Texture* result;
SDL_Texture* canvas;
SDL_Texture* light;
SDL_Texture* TextureA;
SDL_Texture* TextureB;
SDL_Texture* TextureC;
SDL_Texture* blackbarTexture;



//industry standard needs greater fogheight 
int g_fogheight = 18;
int g_fogwidth = 21;
int g_lastFunctionalX = 0; //for optimizing the FoW calcs
int g_lastFunctionalY = 0;
int g_fogMiddleX = 10;
int g_fogMiddleY = 9;

std::vector<std::vector<int> > g_fogcookies( g_fogwidth, std::vector<int>(g_fogheight));
//this second vector is for storing the cookies that are ontopof walls
//that way, we can draw too layers of shadow before actors and one after
std::vector<std::vector<int> > g_savedcookies( g_fogwidth, std::vector<int>(g_fogheight));

//data for two passes of cookies
std::vector<std::vector<int> > g_fc( g_fogwidth, std::vector<int>(g_fogheight));
std::vector<std::vector<int> > g_sc( g_fogwidth, std::vector<int>(g_fogheight));

//for having items bounce
float g_itemsinea = 0;
float g_itemsineb = 0;
float g_itemsinec = 0;

float g_elapsed_accumulator = 0;

// I've always bounced around thinking these matter and turning them down
// or deciding that they don't matter and pumping them up
// Here's what I know atm: the first value should be left at 11 prettymuch always
// 2 is okay - g_TiltResolution actually doesn't affect loading
// but it will effect CPU usage, particularly when the triangles are onscreen
// 2 or 4 for large maps, seems okay 1 is more detail than I think anyone needs.
int g_platformResolution = 11; // a factor of 55. 11 is fine.
float g_TiltResolution = 4; //1, 2, 4, 16 //what size step to use for triangular walls, 2 is almost unnoticable. must be a factor of 64
bool g_protagHasBeenDrawnThisFrame = 0;
bool g_loadingATM = 0; //are we loading a new map atm?
SDL_Texture* g_shadowTexture;
SDL_Texture* g_shadowTextureAlternate;
int g_flashtime = 300; //ms to flash red after taking damage
float g_cameraShove = 150; //factor of the screen to move the camera when shooting.
float g_cameraAimingOffsetX = 0;
float g_cameraAimingOffsetY = 0;
float g_cameraAimingOffsetXTarget = 0;
float g_cameraAimingOffsetYTarget = 0;
float g_cameraAimingOffsetLerpScale = 0.91;

//text
string g_font;
float g_fontsize = 0.031; // 0.021 - 0.04
float g_minifontsize = 0.01;
float g_transitionSpeed = 3; //3, 9

//inventory
float use_cooldown = 0; //misleading, its not for attacks at all
vector<attack*> AdventureattackSet;
int inPauseMenu = 0;
bool old_pause_value = 1; //wait until the user releases the button to not count extra presses
int inventoryScroll = 0; //how many rows in the inventory we've scrolled thru
int inventorySelection = 1; //which item in the inventory is selected
int itemsPerRow = ceil( ( 0.9 - 0.05 ) / ( 0.07 + 0.01) );
int g_inventoryColumns = ceil( (0.74 - 0.05) / 0.07);
int g_itemsInInventory = 0;
int g_inventoryRows = 4;
//for not counting extra presses in UI for shooting and moving axisen
int oldUIUp = 1;
int oldUIDown = 1;
int oldUILeft = 1;
int oldUIRight = 1;
int SoldUIUp = 1;
int SoldUIDown = 1;
int SoldUILeft = 1;
int SoldUIRight = 1;

//physics
float g_gravity = 220;
//These two variables contain the position of the hit of the last lineTrace()
int lineTraceX, lineTraceY;
 

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
	float zoommod = 1;
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

//zoom is really g_defaultZoom when screenwidth is STANDARD_SCREENWIDTH
int WIN_WIDTH = 640; int WIN_HEIGHT = 480;
//theres some warping if STANDARD_SCREENWIDTH < WIN_WIDTH but that shouldn't ever happen
//if in the future kids have screens with 10 million pixels across feel free to mod the game
const int STANDARD_SCREENWIDTH = 1080;
//int WIN_WIDTH = 1280; int WIN_HEIGHT = 720;
//int WIN_WIDTH = 640; int WIN_HEIGHT = 360;
int old_WIN_WIDTH = 0; //used for detecting change in window width to recalculate scalex and y
int saved_WIN_WIDTH = WIN_WIDTH; int saved_WIN_HEIGHT = WIN_HEIGHT;
SDL_Window * window;
SDL_DisplayMode DM;
bool g_fullscreen = false;
camera g_camera(0,0);
entity* protag;
entity* mainProtag; //for letting other entities use this ones inventory; game ends when this one dies

//zoom is planned to be 1.0 for a resolution of 1920 pixels across
float g_defaultZoom = 0.85;
float g_zoom_mod = 1; //for devmode
bool g_update_zoom = 0; //update the zoom this frame

float scalex = 1;
float scaley = scalex;
float min_scale = 0.1;
float max_scale = 2;

entity* g_focus;
vector<entity*> party;
float g_max_framerate = 120;
float g_min_frametime = 1/g_max_framerate * 1000;
SDL_Event event;
float ticks, lastticks, elapsed = 0, halfsecondtimer;
float camx = 0;
float camy = 0;
SDL_Renderer * renderer;

// g_map specifies the name of the map, g_mapdir specifies the folder with in maps the map is in.
// so its maps/{g_mapdir}/{g_map}.map
string g_map = "sp-title";
string g_mapdir = "sp-title";
string g_waypoint;
string g_mapOfLastSave = "sp-title";
string g_waypointOfLastSave = "a";

//input
const Uint8* keystate = SDL_GetKeyboardState(NULL);
bool devinput[50] = {false};
bool input[16] = {false};
bool oldinput[16] = {false};
SDL_Scancode bindings[16];
bool left_ui_refresh = false; //used to detect when arrows is pressed and then released
bool right_ui_refresh = false;
bool fullscreen_refresh = true;
bool quit = false;
string config = "default";
bool g_holdingCTRL = 0;
//this is most noticable with a rifle, but sometimes when you try to shoot
//diagonally, you press one button (e.g. up) a frame or so early then the other (e.g. left)
//as a result, the game instantly shoots up and its unnacceptable.
//this is variable is the amount of frames to wait between getting input and shooting
const int g_diagonalHelpFrames = 4;
int g_cur_diagonalHelpFrames = 0;

//sounds and music
float g_volume = 0;
float g_music_volume = 0.8;
float g_sfx_volume = 1;
bool g_mute = 0;
Mix_Chunk* g_ui_voice;
Mix_Chunk* g_menu_open_sound;
Mix_Chunk* g_menu_close_sound;
Mix_Chunk* g_menu_manip_sound;


Mix_Chunk* g_land;
Mix_Chunk* g_footstep_a;
Mix_Chunk* g_footstep_b;

Mix_Chunk* g_deathsound;
musicNode* closestMusicNode;
musicNode* newClosest;
int musicFadeTimer = 0;
bool fadeFlag = 0;
int musicUpdateTimer = 0;
Mix_Chunk* g_bulletdestroySound;
Mix_Chunk* g_playerdamage;
Mix_Chunk* g_enemydamage;
Mix_Chunk* g_npcdamage;
Mix_Chunk* g_s_playerdeath;

std::map<string, Mix_Chunk> g_static_sounds = {};


//ui
bool protag_can_move = true;
int protag_is_talking = 0; //0 - not talking 1 - talking 2 - about to stop talking
adventureUI* adventureUIManager;
float textWait = 50; //seconds to wait between typing characters of text
float text_speed_up = 1; //speed up text if player holds button. set to 1 if the player isn't pressing it, or 1.5 if she is
float curTextWait = 0;
bool old_z_value = 1; //the last value of the z key. used for advancing dialogue, i.e. z key was down and is now up or was up and is now down if old_z_value != SDL[SDL_SCANCODE_Z]
float g_healthbarBorderSize = 0;
bool g_showHUD = 0;

//scripts
float dialogue_cooldown = 0; //seconds until he can have dialogue again.
entity* g_talker = 0; //drives scripts, must be referenced before deleting an entity to avoid crashes
bool g_forceEndDialogue = 0; //used to end dialogue when the talking entity has been destroyed.

//debuging
SDL_Texture* nodeDebug;
clock_t debugClock;
string g_lifecycle = "Alpha";

//world
int g_layers = 12; //max blocks in world
float g_bhoppingBoost = 20; //the factor applied to friction whilst airborn, not good, basically just airspeed modifier rn

//map editing, mapeditor, map-editor
bool g_mousemode = 1;
bool keyboard_marker_vertical_modifier_refresh = 0;
bool keyboard_marker_vertical_modifier_refresh_b = 0;

entity* nudge = 0; //for nudging entities while map-editing
bool adjusting = 0; //wether to move selected entity or change its hitbox/shadow position
bool g_autoSetThemesFromMapDirectory = 0; //if 1, loading a map will also set the texturedirectory/theme to the mapdir

//userdata - will be set on some file-select-screen
string g_saveName = "a";

std::map<string, int> g_save = {};

//movement
float g_dash_cooldown = 1000;
float g_max_dash_cooldown = 1000;
float g_jump_afterslow = 0.1;
float g_jump_afterslow_seconds = 0;
bool storedJump = 0;


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
float g_earshot = 1000; //how close do entities need to be to join their friends in battle

void playSound(int channel, Mix_Chunk* sound, int loops) {
	//M("play sound");
	if(!g_mute && sound != NULL) {
		Mix_Volume(channel, g_sfx_volume * 128);
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

float XYDistance(int x1, int y1, int x2, int y2) {
	return pow(pow((x1 -x2), 2) + pow((y1 - y2), 2)*XtoY, 0.5);
}


//old crappy code
// template<class T>
// navNode* getNodeByPosition(vector<T*> array, int fx, int fy) {
// 	//this is a placeholder solution for testing AI
// 	//this requires a binary search and sorted nodes to work reasonably for larger maps
// 	float min_dist = 0;
// 		navNode* ret;
// 		bool flag = 1;

// 		//just pitifully slow
// 		for (int i = 0; i < array.size(); i++) {
// 			float dist = Distance(fx, fy, array[i]->x, array[i]->y);
// 			if(dist < min_dist || flag) {
// 				min_dist = dist;
// 				ret = array[i];
// 				flag = 0;
// 			}
// 		}
// 		return ret;
// }



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

//convert frame of sprite to angle
float convertFrameToAngle(int frame, bool flipped) {
	if(flipped){
		if(frame == 0) {return M_PI/2;}
		if(frame == 1) {return (M_PI * 3)/4;}
		if(frame == 2) {return M_PI;}
		if(frame == 3) {return (M_PI * 5)/4;}
		if(frame == 4) {return (M_PI * 6)/4;}
	} else {
		if(frame == 0) {return M_PI/2;}
		if(frame == 1) {return (M_PI * 1)/4;}
		if(frame == 2) {return 0;}
		if(frame == 3) {return (M_PI * 7)/4;}
		if(frame == 4) {return (M_PI * 6)/4;}
	}

	return 0;
}

//convert an angle to a sprite's frame, for eight-frame sprites (arms)
int convertAngleToFrame(float angle) {
	vector<float> angles = {0, (M_PI*1)/4, M_PI/2, (M_PI * 3)/4, M_PI, (M_PI * 5)/4, (M_PI * 6)/4, (M_PI * 7)/4, M_PI * 2};
	for(int i = 0; i < angles.size(); i++) {
		if(angles[i] + M_PI/8 > angle) {
			//this rather silly check is done to accomodate certain values of orbitOffset that would  push angle to not quite fit normally
			//this change came with the ninth entry in the vector of angles
			if(i == 8) {
				i = 0;
			}
			return 7-i;
		}
	}

	return 0;
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

// string stringToHex(string input) {
// 	stringstream ss;
// 	for(auto x : input){
// 		ss << std::hex << int(x);
// 	}
// 	string ret ( ss.str() );

// 	return ret;
// }

// string hexToString(const string& in) {
//     string ret;

//     size_t cnt = in.length() / 2;

//     for (size_t i = 0; cnt > i; ++i) {
//         uint32_t s = 0;
//         stringstream ss;
//         ss << std::hex << in.substr(i * 2, 2);
//         ss >> s;

//         ret.push_back(static_cast<unsigned char>(s));
//     }

//     return ret;
// } 	

// //these are both shite, fix em later
// string intToHex(int input) {
// 	string s = to_string(input);
// 	return stringToHex(s);
// }

// int hexToInt(string input) {
// 	return stoi(hexToString(input));
// }

//TUDO:
//set minimum window width and height to prevent crashes wenn the window is very small

#endif
