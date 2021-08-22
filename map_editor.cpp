#include <iostream>
#include <vector>
#include <fstream> //loading
#include <string>
#include "objects.cpp"
#include <cstring> //loading
#include <sstream> //loading
#include <algorithm> //sort
#include <fstream>
#include <filesystem> //making directories
#include <regex> //for readings scripts

ofstream ofile;
bool olddevinput[50];
bool makingtile, makingbox, makingdoor; //states
int lx, ly, rx, ry; //for map editing corners;
float grid = 64; //default is 64
int width, height; //for selection display
int selectIndex = 0; //select entity
bool tiling = 1; //make current texture tile
bool drawhitboxes = 0; // devMode; //visualize hitboxes in map editor with command
int debug_r =255, debug_g =255, debug_b= 50; //for draw color
int shine = 0; 
bool occlusion = 0; //visualize occlusion (crappily) when map editing
int wallheight = 128;
int wallstart = 0;
bool showMarker = 1;
int lastAction = 0; //for saving the user's last action to be easily repeated
int navMeshDensity = 2; //navnodes every two blocks
int limits[4] = {0};
string textureDirectory = ""; //for choosing a file to load textures from, i.e. keep textures for a desert style level, a laboratory level, and a forest level separa
float mapeditorNavNodeTraceRadius = 100;  //for choosing the radius of the traces between navNodes when creating them in the editor. Should be the radius of the biggest entity
                                    //in the level, so that he does not get stuck on corners


string captex = "tiles/diffuse/grey.png"; 
string walltex = "tiles/diffuse/grey.png"; 
string floortex = "tiles/diffuse/grey.png"; 
string masktex = "&";
//vector of strings to be filled with each texture in the user's texture directory, for easier selection 
vector<string> texstrs;
//indexes representing which element of the array these textures make up
int captexIndex = 0;
int walltexIndex = 0;
int floortexIndex = 0;
ui* captexDisplay;
ui* walltexDisplay;
ui* floortexDisplay;

entity* selectPointer;
int layer = 0;
//these three variables are used in creating walls. By default, the v-key will only make rectangular boxs
bool autoMakeWalls = 1; 
bool makeboxs = 1; 
bool autoMakeWallcaps = 1; 

tile* selection; 
tile* markerz; 
tile* marker; 
tile* navNodeIcon; 
tile* worldsoundIcon; 
tile* listenerIcon; 
tile* musicIcon; 
tile* cueIcon; 
tile* waypointIcon; 
tile* doorIcon; 
tile* triggerIcon;
textbox* nodeInfoText;
string entstring = "lifter"; //last entity spawned;

string mapname = "";
string backgroundstr = "black";

//for sorting ui on mapload
int compare_ui (ui* a, ui* b) {
  	return a->priority < b->priority;
}

void sort_ui(vector<ui*> &g_ui) {
    stable_sort(g_ui.begin(), g_ui.end(), compare_ui);
}

void load_map(SDL_Renderer* renderer, string filename, string destWaypointName) {
    debugClock = clock();
    mapname = filename;

    //parse name from fileaddress
    unsigned first = mapname.find("/");
    unsigned last = mapname.find_last_of("/");

    g_map = mapname.substr(first + 1,last-first - 1);
    ifstream infile;
    infile.open (filename);
    string line;
    string word, s0, s1, s2, s3, s4;
    float p0, p1, p2, p3, p4, p5, p6, p7, p8, p9;
    
    background = 0;

    while (std::getline(infile, line)) {
        istringstream iss(line);
        word = line.substr(0, line.find(" "));
        if(word == "limits") {
            iss >> s0 >> p1 >> p2 >> p3 >> p4; 
            g_camera.lowerLimitX = p1;
            limits[0] = p1;
            g_camera.lowerLimitY = p2;
            limits[1] = p2;
            g_camera.upperLimitX = p3;
            limits[2] = p3;
            g_camera.upperLimitY = p4;
            limits[3] = p4;
            g_camera.enforceLimits = 1;
            
        }
        if(word == "bg" && g_useBackgrounds) {
            iss >> s0 >> backgroundstr;
            SDL_Surface* bs = IMG_Load(("tiles/backgrounds/" + backgroundstr + ".png").c_str());
	        background = SDL_CreateTextureFromSurface(renderer, bs);
            g_backgroundLoaded = 1;
            SDL_FreeSurface(bs);
        }
        if(word == "box") {
            //M("loading collisisons" << endl;
            iss >> s0 >> p1 >> p2 >> p3 >> p4 >> p5 >> s1 >> s2 >> p6 >> p7 >> p8 >> s3; 
            box* c = new box(p1, p2, p3, p4, p5, s1, s2, p6, p7, p8, s3.c_str());
        }
        if(word == "entity") {
            //M("loading entity" << endl;
            iss >> s0 >> s1 >> p0 >> p1 >> p2;
            const char* plik = s1.c_str();
            entity* e = new entity(renderer, plik);
            e->x = p0;
            e->y = p1;
            e->z = p2;
            e->shadow->x = e->x + e->shadow->xoffset;
            e->shadow->y = e->y + e->shadow->yoffset;
        }
        if(word == "tile") {
            //M("loading tile" << endl;
            iss >> s0 >> s1 >> s2 >> p1 >> p2 >> p3 >>p4 >> p5 >> p6 >> p7 >> p8 >> p9;
            const char* plik1 = s1.c_str();
            const char* plik2 = s2.c_str();
            tile* t = new tile(renderer, plik1, plik2, p1, p2, p3, p4, p5, p6, p7, p8, p9);
        }
        if(word == "triangle") {
            //M("loading triangle" << endl;
            iss >> s0 >> p1 >> p2 >>p3 >> p4 >> p5 >> s1 >> s2 >> p6 >> p7;
            tri* t = new tri(p1, p2, p3, p4, p5, s1, s2, p6, p7);
        }
        if(word == "mapObject") {
            iss >> s0 >> s1 >> s2 >> p1 >> p2 >> p3 >> p4 >> p5 >> p6 >> p7;
            mapObject* e = new mapObject(renderer, s1, s2.c_str(), p1, p2, p3, p4, p5, p6, p7);
        }
        if(word == "door") {
            iss >> s0 >> s1 >> s2 >> p1 >> p2 >>p3 >> p4;
            const char* map = s1.c_str();
            door* d = new door(renderer, map, s2, p1, p2, p3, p4);
        }
        if(word == "trigger") {
            iss >> s0 >> s1 >> p1 >> p2 >>p3 >> p4 >> s2;
            const char* binding = s1.c_str();
            trigger* t = new trigger(binding, p1, p2, p3, p4, s2);
        }
        if(word == "worldsound") {
            iss >> s0 >> s1 >> p1 >> p2;
            const char* sprite = s1.c_str();
            worldsound* w = new worldsound(sprite, p1, p2);
        }
        if(word == "music") {
            iss >> s0 >> s1 >> p1 >> p2;
            const char* sprite = s1.c_str();
            musicNode* m = new musicNode(sprite, p1, p2);
        }
        if(word == "cue") {
            
            iss >> s0 >> s1 >> p1 >> p2 >> p3;
            const char* sprite = s1.c_str();
            cueSound* c = new cueSound(sprite, p1, p2, p3);
        }
        if(word == "waypoint") {
            iss >> s0 >> s1 >> p1 >> p2;
            waypoint* w = new waypoint(s1, p1, p2);
        }
        
        if(word == "ui") {
            iss >> s0 >> s1 >> p0 >> p1 >> p2 >> p3 >> p4;
            const char* plik = s1.c_str();
            ui* u = new ui(renderer, plik, p0, p1, p2, p3, p4);
        }

        if(word == "heightmap") {
            iss >> s0 >> s1 >> s2 >> p0;
            heightmap* h = new heightmap(s2.c_str(), s1.c_str(), p0);
        }
        if(word == "navNode") {
            iss >> s0 >> p1 >> p2;
            navNode* n = new navNode(p1, p2);
        }
        if(word == "navNodeEdge") {
            iss >> s0 >> p1 >> p2;
            g_navNodes[p1]->Add_Friend(g_navNodes[p2]);
            while(iss >> p2) {
                g_navNodes[p1]->Add_Friend(g_navNodes[p2]);
            }
        }
        if(word == "listener") {
            iss >> s0 >> s1 >> p1 >> p2 >> s2 >> p3 >> p4;
            listener* l = new listener(s1, p1, p2, s2, p3, p4);
        }
    }
    
    {   
        mapObject* child;
        
        //build map from boxs
        for(int i = 0; i < g_boxs.size(); i++) {
            for(box* box : g_boxs[i]) {
                //handle rect
                child = new mapObject(renderer, box->walltexture, "&", box->bounds.x, box->bounds.y + box->bounds.height, i * 64, box->bounds.width, 55, 1);
                box->children.push_back(child);
                //if there's no box above, make a cap
                if(box->capped) {
                    //related to resolution of wallcap
                    int step = g_platformResolution;
                    for (int i = 0; i < box->bounds.height; i+=step) {
                        child = new mapObject(renderer, box->captexture, "&", box->bounds.x, box->bounds.y + i + step, box->layer * 64 + 64, box->bounds.width, step, 0);
                        box->children.push_back(child);
                    }
                    D(box->shineBot);
                    if(box->shineBot){
                        //shine
                        child = new mapObject(renderer, "tiles/lighting/SMOOTHSHADING.png",  "&", box->bounds.x, box->bounds.y + box->bounds.height + 55/2,  box->layer * 64 + 64, box->bounds.width, 55);
                        box->children.push_back(child);
                    }
                    if(box->shineTop){
                    //back
                    child = new mapObject(renderer, "tiles/lighting/SMOOTHSHADING.png",  "&", box->bounds.x, box->bounds.y + 55/2, box->layer * 64 + 64 + 1, box->bounds.width, 55/2);
                    box->children.push_back(child);
                    }
                    
                }
                //floor shadows

                    //front shading
                    if(box->shadeBot == 1) {
                        child = new mapObject(renderer, "tiles/lighting/OCCLUSION.png",  "&", box->bounds.x, box->bounds.y + box->bounds.height + 19 + 2, 64 * box->layer + 2, box->bounds.width, 55);
                        box->children.push_back(child);
                    }
                    //left
                    int step = g_platformResolution;
                    if(box->shadeLeft) {
                        for (int i = 0; i < box->bounds.height; i+=step) {
                            child = new mapObject(renderer, "tiles/lighting/h-OCCLUSION.png",  "&", box->bounds.x - 27, box->bounds.y + i + g_platformResolution, 64 * box->layer, 55/2, step);
                            box->children.push_back(child);
                        }
                    }
                    if(box->shadeRight) {
                        for (int i = 0; i < box->bounds.height; i+=step) { //5, 8   
                            child = new mapObject(renderer, "tiles/lighting/h-OCCLUSION.png",  "&", box->bounds.x + box->bounds.width, box->bounds.y + i + g_platformResolution, 64 * box->layer, 55/2, step);
                            box->children.push_back(child);
                        }
                    }
                    
                    
                    
                    //corner a
                    if(box->shadeLeft && box->shadeTop) {
                        child = new mapObject(renderer, "tiles/lighting/x-OCCLUSION.png", "&", box->bounds.x - (38 - 19), box->bounds.y, 64 * box->layer, 32, 19, 0, 0);
                        box->children.push_back(child);
                    }
                    //corner b
                    if(box->shadeRight && box->shadeTop) {
                        child = new mapObject(renderer, "tiles/lighting/x-OCCLUSION.png", "&", box->bounds.x + box->bounds.width, box->bounds.y, 64 * box->layer, 32, 19, 0, 0);
                        box->children.push_back(child);
                    }
                    //corner c
                    D(box->shadeBot);
                    if(box->shadeLeft && box->shadeBot){
                        child = new mapObject(renderer, "tiles/lighting/x-OCCLUSION.png", "&", box->bounds.x - (38 - 19), box->bounds.y + box->bounds.height + (38 - 19), 64 * box->layer, 19, 19, 0, 0);
                        box->children.push_back(child);
                    }
                    //corner d
                    if(box->shadeRight && box->shadeBot) {
                        child = new mapObject(renderer, "tiles/lighting/x-OCCLUSION.png", "&", box->bounds.x + box->bounds.width, box->bounds.y + box->bounds.height + (38 - 19), 64 * box->layer, 19, 19, 0, 0);
                        box->children.push_back(child);
                    }
                
                 
            }
        }
        for(vector<tri*> layer : g_triangles) {
            for(auto triangle : layer) {
                //handle tri
                if(triangle->type == 0) {
                    int step = g_platformResolution;
                    if(triangle->capped) {
                        for (int i = 0; i < 55; i+=step) {
                            child = new mapObject(renderer, triangle->captexture, "tiles/engine/a.png", triangle->x2, triangle->y1 + i + step, triangle->layer * 64 + 64, 64 - 1, step, 0);
                            triangle->children.push_back(child);
                        }
                        //diagonal shine
                        step = g_TiltResolution;
                        for (int i = 0; i < 64; i+=step) {
                            child = new mapObject(renderer, "tiles/lighting/SMOOTHSHADING.png", "&", triangle->x2 + i, triangle->y1 + 55 + 35 - (i * XtoY) - 1, triangle->layer * 64 + 64, step, 55, 0, (i * XtoY) + 0);
                            triangle->children.push_back(child);
                        }

                    }
                    
                    //a tile on the floor to help with the edge of the diagonal wall pieces
                    //this tile won't be saved, because it uses an engine maska
                    //tile* t = new tile(renderer, triangle->walltexture.c_str(), "tiles/engine/a.png", triangle->x2, triangle->y1 - 1, 64 - 1, 54 + 1,0, 1, 1, 0, 0);    
                    
                    step = g_TiltResolution;
                    int vstep = 64;
                    for (int j = triangle->layer * 64; j < triangle->layer * 64 + 64; j+=vstep) {
                        for (int i = 0; i < 64; i+=step) {
                            child = new mapObject(renderer, triangle->walltexture, "&", triangle->x2 + i, triangle->y1 + 55 - (i * XtoY) - 1, j, step,  32, 1, (i * XtoY));
                            triangle->children.push_back(child);
                        }
                    }

                    step = g_TiltResolution;
                    if(triangle->shaded) {
                        for (int i = 0; i < 64; i+=step) {
                            
                            child = new mapObject(renderer, "tiles/lighting/OCCLUSION.png", "&", triangle->x2 + i, triangle->y1 + 55 + 30 - (i * XtoY) - 1, triangle->layer * 64, step,  50, 0, (i * XtoY) + 0);
                            triangle->children.push_back(child);
                            
                        }
                    }
                                
                    
                } else {
                    if(triangle->type == 3) {
                        int step = g_platformResolution;
                        if(triangle->capped) {
                            for (int i = 0; i < 55; i+=step) {
                                child = new mapObject(renderer, triangle->captexture, "tiles/engine/b.png", triangle->x1 + 1, triangle->y1 + i + step, triangle->layer * 64 + 64, 64 - 1, step, 0);
                                triangle->children.push_back(child);
                            }
                            step = g_TiltResolution;
                            for (int i = 0; i < 64; i+=step) {
                                child = new mapObject(renderer, "tiles/lighting/SMOOTHSHADING.png", "&", triangle->x2 + i - 64, triangle->y1 + 55 + 35 - (((64 - step) - i) * XtoY) - 1, triangle->layer * 64 + 64, step,  55, 0, ((64 - i) * XtoY));
                                triangle->children.push_back(child);
                            }
                        
                        }
                        
                        //a tile on the floor to help with the edge of the diagonal wall pieces
                        //this tile won't be saved, because it uses an engine mask
                        //tile* t = new tile(renderer, triangle->walltexture.c_str(), "tiles/engine/b.png", triangle->x1 + 1, triangle->y1 - 1, 64 - 1, 54 + 1, 0, 1, 1, 0, 0);    
                        step = g_TiltResolution;
                        int vstep = 64;
                        for (int j = triangle->layer * 64; j < triangle->layer * 64 + 64; j+=vstep) {
                            for (int i = 0; i < 64; i+=step) {
                                child = new mapObject(renderer, triangle->walltexture, "&", triangle->x1 + i, triangle->y1 + 55 - (((64 - step) - i) * XtoY) - 1, j, step,  32, 1, ((64 - i) * XtoY));
                                triangle->children.push_back(child);
                            }
                        }

                        
                        step = g_TiltResolution;
                        if(triangle->shaded) {
                            for (int i = 0; i < 64; i+=step) {
                                
                                child = new mapObject(renderer, "tiles/lighting/OCCLUSION.png", "&", triangle->x2 + i - 64, triangle->y1 + 55 + 30 - (((64 - step) - i) * XtoY) - 1, 64 * triangle->layer, step,  50, 0, ((64 - i) * XtoY));
                                triangle->children.push_back(child);
                                
                            }
                        }
                    } else {
                        if(triangle->type == 2) {
                            if(triangle->capped) {
                                int step = g_platformResolution;
                                for (int i = 0; i < 55; i+=step) {
                                    child = new mapObject(renderer, triangle->captexture, "tiles/engine/c.png", triangle->x1 + 1, triangle->y2 + i + step, triangle->layer * 64 + 64, 64 - 1, step, 0);
                                    triangle->children.push_back(child);
                                }
                                 step = g_TiltResolution;
                                for (int i = 0; i < 64; i+=step) {
                                    child = new mapObject(renderer, "tiles/lighting/SMOOTHSHADING.png", "&", triangle->x2 + i - 64, triangle->y1 + 35 - (i * XtoY) - 1, triangle->layer * 64 + 64, step, 34, 0, (i * XtoY) + 0);
                                    triangle->children.push_back(child);
                                }
                            }
                            //child = new mapObject(renderer, triangle->captexture, "tiles/engine/c.png", triangle->x1 + 1, triangle->y2 + 55 + 1, triangle->layer * 64 + 64, 64 + 1, 54 + 1, 0, 0, 0);
                            //triangle->children.push_back(child);
                        } else {
                            
                            if(triangle->capped) {
                                int step = g_platformResolution;
                                for (int i = 0; i < 55; i+=step) {
                                    child = new mapObject(renderer, triangle->captexture, "tiles/engine/d.png", triangle->x2, triangle->y2 + i + step, triangle->layer * 64 + 64, 64 - 1, step, 0);
                                    triangle->children.push_back(child);
                                }
                                step = g_TiltResolution;
                                for (int i = 0; i < 64; i+=step) {
                                    child = new mapObject(renderer, "tiles/lighting/SMOOTHSHADING.png", "&", triangle->x2 + i, triangle->y1 + 35 - (((64 - step) - i) * XtoY) - 1, triangle->layer * 64 + 64, step,  34, 0, ((64 - i) * XtoY));
                                    triangle->children.push_back(child);
                                }
                            }
                        }
                    }
                }
                
            }
        }
    }

    Update_NavNode_Costs(g_navNodes);
    infile.close();

    //sort ui by priority
    sort_ui(g_ui);

    double LoadingTook = ( std::clock() - debugClock ) / (double) CLOCKS_PER_SEC;
    std::cout << "Loading took " << LoadingTook << "s" << endl;

    //map is loaded in, let's search for the proper waypoint
    for (int i = 0; i < g_waypoints.size(); i++) {
        if(g_waypoints[i]->name == destWaypointName) {
            protag->x = g_waypoints[i]->x - protag->width/2;
            protag->y = g_waypoints[i]->y + protag->bounds.height/2;
            break;
        }
        //throw error if protag coords are unset
    }

    //put party with protag
    for(auto m:party) {
        m->x = protag->x;
        m->y = protag->y;
    } 
    g_camera.x = (g_focus->getOriginX() - (g_camera.width/(2 * g_camera.zoom)));
	g_camera.y = ((g_focus->getOriginY() - XtoZ * g_focus->z) - (g_camera.height/(2 * g_camera.zoom)));
	g_camera.oldx = g_camera.x;
	g_camera.oldy = g_camera.y;
} 


//called on init if map_editing is true
void init_map_writing(SDL_Renderer* renderer) {
    selection = new tile(renderer, "tiles/engine/marker.png", "&", 0, 0, 0, 0, 1, 1, 1, 0, 0);
    marker = new tile(renderer, "tiles/engine/marker.png", "&", 0, 0, 0, 0, 2, 0, 0, 0, 0);
    markerz = new tile(renderer, "tiles/engine/marker.png", "&", 0, 0, 0, 0, 2, 0, 0, 0, 0);
    worldsoundIcon = new tile(renderer, "tiles/engine/speaker.png", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
    listenerIcon = new tile(renderer, "tiles/engine/listener.png", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
    navNodeIcon = new tile(renderer, "tiles/engine/walker.png", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
    musicIcon = new tile(renderer, "tiles/engine/music.png", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
    cueIcon = new tile(renderer, "tiles/engine/cue.png", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
    waypointIcon = new tile(renderer, "tiles/engine/waypoint.png", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
    doorIcon = new tile(renderer, "tiles/engine/door.png", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
    triggerIcon = new tile(renderer, "tiles/engine/trigger.png", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);

    selection->software = 1;
    marker->software = 1;
    markerz->software = 1;
    worldsoundIcon->software = 1;
    listenerIcon->software = 1;
    navNodeIcon->software = 1;
    musicIcon->software = 1;
    cueIcon->software = 1;
    waypointIcon->software = 1;
    doorIcon->software = 1;
    triggerIcon->software = 1;
    
    floortexDisplay = new ui(renderer, floortex.c_str(), 0.0, 0.9, 0.1, 0.1, 0);
    walltexDisplay = new ui(renderer, walltex.c_str(), 0.1, 0.9, 0.1, 0.1, 0);
    captexDisplay = new ui(renderer, captex.c_str(), 0.2, 0.9, 0.1, 0.1, 0);

    texstrs.clear();
    string path = "tiles/diffuse/";
    for (const auto & entry : filesystem::directory_iterator(path)) {
        texstrs.push_back(entry.path());
    }
}

//called every frame if map_editing is true
//each bool represents a button, if c is true, c was pressed that frame
void write_map(entity* mapent) {
    float temp, px, py;

    if(g_mousemode){
        int mxint, myint;
        SDL_GetMouseState(&mxint, &myint);
        float percentx = (float)mxint/(float)WIN_WIDTH;
        float percenty =  (float)myint/(float)WIN_HEIGHT;
        temp =  percentx/scalex * g_camera.width + (g_camera.x - 32);
        px = round(temp/grid)*grid;
        temp = percenty/scalex * g_camera.height + (g_camera.y - 26);
        py =  round(temp/(float)round(grid* XtoY))*(float)round(grid* XtoY);
    } else {
        temp = mapent->x - 64;
        px = round(temp/grid)*grid;
        temp = mapent->y -mapent->height - 38;
        py = round(temp/(float)round(grid* XtoY))*(float)round(grid* XtoY);

        // needed for proper grid in negative x or y
        if(mapent->x < 0)
            px-=grid;

        if(mapent->y < mapent->height)
            py-=round(grid * XtoY);
    }
  
    


    //set marker
    marker->x = px;
    marker->y = py;
    marker->width = grid;
    marker->height = round(grid * XtoY);
    //some debug drawing
    SDL_FRect drect;
    if(drawhitboxes) {
        for(int i = 0; i < g_entities.size(); i++) {
            if(g_entities[i]->wallcap == 1) {continue; }
            drect.x = (g_entities[i]->bounds.x + g_entities[i]->x -g_camera.x)*g_camera.zoom;
            drect.y = (g_entities[i]->bounds.y + g_entities[i]->y -g_camera.y)*g_camera.zoom;
            drect.w = g_entities[i]->bounds.width * g_camera.zoom;
            drect.h = g_entities[i]->bounds.height* g_camera.zoom;

            SDL_SetRenderDrawColor(renderer, 80, 150, 0, 255);
            SDL_RenderDrawRectF(renderer, &drect);
            
        }
        int layer = 0;
        for (auto n: g_boxs) {
            for (long long unsigned int i = 0; i < n.size(); i++) {
                drect.x = (n[i]->bounds.x -g_camera.x)*g_camera.zoom;
                drect.y = (n[i]->bounds.y - (38 * layer) -g_camera.y)*g_camera.zoom;
                drect.w = n[i]->bounds.width * g_camera.zoom;
                drect.h = n[i]->bounds.height* g_camera.zoom;

                SDL_SetRenderDrawColor(renderer, 150 - layer * 38, 50, layer * 38, 255);
                SDL_RenderDrawRectF(renderer, &drect);
            }
            layer++;
        }
        for(auto n: g_projectiles) {
            drect.x = (n->x + n->bounds.x -g_camera.x)*g_camera.zoom;
            drect.y = (n->y + n->bounds.y - (0.5 * n->z) -g_camera.y)*g_camera.zoom;
            drect.w = n->bounds.width * g_camera.zoom;
            drect.h = n->bounds.height* g_camera.zoom;

            SDL_SetRenderDrawColor(renderer, 255, 70, 30, 255);
            SDL_RenderDrawRectF(renderer, &drect);
        }
        SDL_SetRenderDrawColor(renderer, 150, 50, 0, 255);
        for(int i = 0; i < g_layers; i++) {
            for (auto n : g_triangles[i]) {
                n->render(renderer);
            }
        }
        //draw navNodes
        for (int i = 0; i < g_navNodes.size(); i++) {
            //SDL_Rect obj = {(g_navNodes[i]->x -g_camera.x - 20)* g_camera.zoom , ((g_navNodes[i]->y - g_camera.y - 20) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
            //SDL_RenderCopy(renderer, navNodeIcon->texture, NULL, &obj);
            SDL_SetRenderDrawColor(renderer, 0, 100, 150, 255);
            for (int j = 0; j < g_navNodes[i]->friends.size(); j++) {
                int x1 = (g_navNodes[i]->x-g_camera.x)* g_camera.zoom;
                int y1 = (g_navNodes[i]->y-g_camera.y)* g_camera.zoom;
                int x2 = (g_navNodes[i]->friends[j]->x-g_camera.x)* g_camera.zoom;
                int y2 = (g_navNodes[i]->friends[j]->y-g_camera.y)* g_camera.zoom;
                
                SDL_RenderDrawLineF(renderer, x1, y1, x2, y2);
                
            }
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        }
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    //draw rectangle to visualize the selection
    if(makingbox || makingtile || makingdoor) {
        //SDL_Rect dstrect = {rx, ry, abs(px-rx) + grid, abs(py-ry) + grid};
        
        if(lx < px) {selection->x = lx; } else {selection->x = px;}
        if(ly < py) {selection->y = ly; } else {selection->y = py;}
        
        selection->width = abs(lx-px) + grid;
        selection->height = abs(ly-py) + (round(grid * XtoY));
        if(tiling && !makingbox && !makingdoor) {
            selection->xoffset = (int)selection->x % int(selection->texwidth);
            selection->yoffset = (int)selection->y % int(selection->texheight);
            selection->wraptexture = 1;
        } else {
            //so that the red boundaries of making a box appear properly
            selection->xoffset = 0;
            selection->yoffset = 0;
        }
        

    } else {
        //hide it;
        selection->width = 0;
        selection->height = 0;
    }


    if(devinput[0] && !olddevinput[0] && !makingbox) {
        lx = px;
        ly = py;
        makingbox = 1;
        selection->image = IMG_Load("tiles/engine/trigger.png");
        selection->texture = SDL_CreateTextureFromSurface(renderer, selection->image);
        SDL_FreeSurface(selection->image);
        selection->wraptexture = 0;
        
    } else {
        if(devinput[0] && !olddevinput[0] && makingbox) {
            makingbox = 0;
            trigger* t = new trigger("unset", selection->x, selection->y, selection->width, selection->height, "protag");
            //set to unactive so that if we walk into it, we dont crash
            t->active = false;
        }

    }
    if(devinput[1] && !olddevinput[1]) {
        // //entstring = "lifter";
        // M("spawned " + entstring);

        
        // //actually spawn the entity in the world
        // //entstring = "entities/" + entstring + ".ent";
        // const char* plik = entstring.c_str();
        // entity* e = new entity(renderer,  plik);
        // e->x = px;
        // e->y = py + mapent->height -(grid/2);
    }
    if(devinput[2] && !olddevinput[2] && makingtile == 0) {


        lx = px;
        ly = py;
        makingtile = 1;

        
        selection->image = IMG_Load(floortex.c_str());
        selection->texture = SDL_CreateTextureFromSurface(renderer, selection->image);
        SDL_QueryTexture(selection->texture, NULL, NULL, &selection->texwidth, &selection->texheight);
        SDL_FreeSurface(selection->image);

    } else {    
        if(devinput[2] && !olddevinput[2] && makingtile == 1) {
            if(makingtile) {                
                //ofile << "tile " << walltex << " " << to_string(selection->x) << " " << to_string(selection->y) << " " << to_string(selection->width) << " " << to_string(selection->height) << " " << layer << " " << tiling << " 0 " <<  endl;

                makingtile = 0;
                const char* plik = floortex.c_str();
                tile* t = new tile(renderer, plik, masktex.c_str(), selection->x, selection->y, selection->width, selection->height, layer, tiling, 0, 0, 0);
                
                t->z = layer;
                t->wraptexture = tiling;

                //check for heightmap
                string heightmap_fileaddress = "tiles/heightmaps/" + floortex.substr(14);
                D(heightmap_fileaddress);

                if(fileExists(heightmap_fileaddress)) {
                    //check if we already have a heightmap with the same bindings
                    bool flag = 1;
                    for (int i = 0; i < g_heightmaps.size(); i++) {
                        if(g_heightmaps[i]->binding == heightmap_fileaddress) {
                            flag = 0;
                            break;
                        }
                    }
                    if(flag) {
                        D(heightmap_fileaddress);
                        D(floortex);
                        heightmap* e = new heightmap(floortex, heightmap_fileaddress, 0.278); // this value corresponds to just over one block, or a comfortable slope up to one block.
                    }
                }
                M(g_heightmaps.size());
                
            }
        }
    }
    if(devinput[3] && !olddevinput[3] && makingbox == 0) {
        //mouse1
        //select entity first
        nudge = 0;
        for(auto n : g_entities) {
            if(n == protag) {continue;}
            if(RectOverlap(n->getMovedBounds(), marker->getMovedBounds())) {
                nudge = n;
            }
        }
        if(nudge == 0) {
            lx = px;
            ly = py;
            makingbox = 1;
            selection->image = IMG_Load("tiles/engine/wall.png");
            selection->texture = SDL_CreateTextureFromSurface(renderer, selection->image);
            SDL_FreeSurface(selection->image);
        }
    } else {
        if(devinput[3] && !olddevinput[3] && makingbox == 1) {
            if(makingbox) {

                //makes a tile and also a hitbox
                
                //if(makeboxs) {
                //    ofile << "box " << to_string(selection->x) << " " << to_string(selection->y) << " " << to_string(selection->width) << " " << to_string(selection->height) << endl;
                //}
                //make "cap" or the top of the wall
                
                makingbox = 0;
                box* c;

                //spawn related objects
                //string loadstr = "tiles/wall.png";
                if(makeboxs) {
                    for (int i = wallstart/64; i < wallheight / 64; i++) {
                        bool fcap = (!(i + 1 < wallheight/64));//&& autoMakeWallcaps;
                        //bool fcap = 1;
                        c = new box(selection->x, selection->y, selection->width, selection->height, i, walltex, captex, fcap, 0, 0, "0000");
                    }
                }
                const char* plik = walltex.c_str();
                if(autoMakeWallcaps) {
                    int step = g_platformResolution;
                    for (int i = 0; i < selection->height; i+=step) {
                        mapObject* e = new mapObject(renderer, captex, "&", selection->x, selection->y + i + step, wallheight, selection->width, step, 0);
                        c->children.push_back(e);
                    }
                    
                }

                if(autoMakeWalls) {
                    int step = 64;
                    for (int i = wallstart; i < wallheight; i+=step) {
                        mapObject* e = new mapObject(renderer, walltex, "&", selection->x, selection->y + selection->height, i, selection->width, 55, 1);
                        c->children.push_back(e);
                    }
                    ////entity* e = new entity(renderer, selection->x, selection->y + selection->height, selection->width, wallheight, walltex, 1);
                    //entity* e = new entity(renderer, walltex, selection->x, selection->y + selection->height, 0, selection->width, (wallheight) * XtoZ + 2, 1);
                }

                if(shine == 1) {
                    //front
                    //mapObject* f = new mapObject(renderer, "tiles/lighting/SMOOTHSHADING.png",  "&", selection->x, selection->y + selection->height + marker->height/2,  wallheight + 1, selection->width, marker->height);
                    
                    

                    //back
                    //f = new mapObject(renderer, "tiles/lighting/SMOOTHSHADING.png",  "&", selection->x, selection->y + marker->height/2, wallheight + 1, selection->width, marker->height/2);
                    
                }

                if(shine == 2) {
                    mapObject* f = new mapObject(renderer, "tiles/lighting/SHARPSHADING.png",  "&", selection->x, selection->y + selection->height - wallheight + marker->height/2, 0, selection->width, marker->height);

                }

                if(shine == 3) { 
                    mapObject* f = new mapObject(renderer, "tiles/lighting/SHARPBRIGHTSHADING.png",  "&", selection->x, selection->y + selection->height - wallheight + marker->height/2, 0, selection->width, marker->height);
                }

                if(occlusion) {
                    //front shading
                    mapObject* e = new mapObject(renderer, "tiles/lighting/OCCLUSION.png",  "&", selection->x, selection->y + selection->height + 19, 0, selection->width, marker->height);
                    c->children.push_back(e);
                    //left
                    int step = g_platformResolution;
                    for (int i = 0; i < selection->height; i+=step) {
                        mapObject* e = new mapObject(renderer, "tiles/lighting/h-OCCLUSION.png",  "&", selection->x - 19, selection->y + i + g_platformResolution, 0, 55/2, step);
                        c->children.push_back(e);
                    }
                    for (int i = 0; i < selection->height; i+=step) {
                        mapObject* e = new mapObject(renderer, "tiles/lighting/h-OCCLUSION.png",  "&", selection->x + selection->width, selection->y + i + g_platformResolution, 0, 55/2, step);
                        c->children.push_back(e);
                    }
                    
                    
                    
                    //corner a
                    e = new mapObject(renderer, "tiles/lighting/x-OCCLUSION.png", "&", selection->x - (38 - 19), selection->y, 0, 32, 19, 0, 0);
                    c->children.push_back(e);
                    //corner b
                    e = new mapObject(renderer, "tiles/lighting/x-OCCLUSION.png", "&", selection->x + selection->width, selection->y, 0, 32, 19, 0, 0);
                    c->children.push_back(e);
                    //corner c
                    e = new mapObject(renderer, "tiles/lighting/x-OCCLUSION.png", "&", selection->x - (38 - 19), selection->y + selection->height + (38 - 19), 0, 19, 19, 0, 0);
                    c->children.push_back(e);
                    //corner d
                    e = new mapObject(renderer, "tiles/lighting/x-OCCLUSION.png", "&", selection->x + selection->width, selection->y + selection->height + (38 - 19), 0, 19, 19, 0, 0);
                    c->children.push_back(e);
                }                
            }
        }
    }

    if(devinput[20] && !olddevinput[20] && makingbox == 0) {
        lx = px;
        ly = py;
        makingbox = 1;
        selection->image = IMG_Load("tiles/engine/navmesh.png");
        selection->texture = SDL_CreateTextureFromSurface(renderer, selection->image);
        SDL_FreeSurface(selection->image);
    } else {
        if(devinput[20] && !olddevinput[20] && makingbox == 1) {
            if(makingbox) {
                for (int i = selection->x; i < selection->width + selection->x; i+= 64 * navMeshDensity) {
                    for (int j = selection->y; j < selection->height + selection->y; j+= 38 * navMeshDensity) {
                        D(j);
                        D(marker->height * navMeshDensity);
                        navNode* n = new navNode(i + marker->width/2, j + marker->height/2);
                    }
                }


                //delete nodes too close to walls
                int checkinglayer = 0;
                float cullingdiameter = mapeditorNavNodeTraceRadius;
                for (int i = 0; i < g_navNodes.size(); i++) {
                    rect noderect = {g_navNodes[i]->x - cullingdiameter/2, g_navNodes[i]->y- cullingdiameter/2, cullingdiameter, cullingdiameter * XtoY};
                   for(int j = 0; j < g_boxs[layer].size(); j++) {
                        if(RectOverlap(g_boxs[layer][j]->bounds, noderect)) {
                            delete g_navNodes[i];
                            i--;
                            break;
                        }
                    }
                }

                for (int i = 0; i < g_navNodes.size(); i++) {
                    for (int j = 0; j < g_navNodes.size(); j++) {
                        if(i == j) {continue;}
                        if(Distance(g_navNodes[i]->x, g_navNodes[i]->y, g_navNodes[j]->x, g_navNodes[j]->y) < 300 && LineTrace(g_navNodes[i]->x, g_navNodes[i]->y, g_navNodes[j]->x, g_navNodes[j]->y, 0, mapeditorNavNodeTraceRadius)) {
                            //dont add a friend we already have
                            bool flag = 1;
                            for(auto x : g_navNodes[i]->friends) {
                                if(x == g_navNodes[j]) {
                                    flag = 0;
                                }
                            }
                            if(flag) {
                                g_navNodes[i]->Add_Friend(g_navNodes[j]);
                            }
                        }
                    }
                    
                }

                

                //delete nodes with no friends
                for (int i = 0; i < g_navNodes.size(); i++) {
                    if(g_navNodes[i]->friends.size() == 0) {
                        delete g_navNodes[i];
                        i--;
                    }
                }

                //delete friends which dont exist anymore
                for (int i = 0; i < g_navNodes.size(); i++) {
                    for (int j = 0; j < g_navNodes[i]->friends.size(); j++) {
                        auto itr = find(g_navNodes.begin(), g_navNodes.end(), g_navNodes[i]->friends[j]);
                        
                        if(itr == g_navNodes.end()) {
                            //friend has been deleted, remove as well from friend array
                            g_navNodes[i]->friends.erase(remove(g_navNodes[i]->friends.begin(), g_navNodes[i]->friends.end(), g_navNodes[i]->friends[j]), g_navNodes[i]->friends.end());
                            
                        }
                        
                    }
                    
                }
                
                Update_NavNode_Costs(g_navNodes);
                makingbox = 0;
            }
        }
    }

    if(devinput[4] && !olddevinput[4] ) {
        if(nudge != 0) {
            nudge = 0;
        } else {
            if(makingbox || makingtile){
                makingbox = 0;
                makingtile = 0;
            } else {
                for(auto n : g_entities) {
                    if(n == protag) {continue;}
                    if(RectOverlap(n->getMovedBounds(), marker->getMovedBounds())) {
                        delete n;
                        break;
                    }
                }

                vector<tri*> deleteTris;
                vector<box*> deleteRects;
                rect markerrect = {marker->x, marker->y, marker->width, marker->height};
                //delete block at position
                for (int i = 0; i < g_boxs.size(); i++) {
                    for(auto n : g_boxs[i]) {
                        if(RectOverlap(markerrect, n->bounds)) {
                            deleteRects.push_back(n);
                        }
                    }
                }
                for (int i = 0; i < g_triangles.size(); i++) {
                    for(auto n : g_triangles[i]) {
                        if(TriRectOverlap(n, marker->x + 6, marker->y + 6, marker->width - 12, marker->height - 12)) {
                            deleteTris.push_back(n);
                        }
                    }
                }
                
                for(auto n: deleteTris) {
                    for(auto child:n->children) {
                        delete child;
                    }
                    delete n;
                }
                for(auto n: deleteRects) {
                    for(auto child:n->children) {
                        delete child;
                    }
                    delete n;
                }

                //delete navmesh
                for(auto x : g_navNodes) {
                    rect b = {x->x - 10, x->x - 10, x->x + 10, x->y + 10};
                    if(RectOverlap(markerrect, b)) {
                        //delete x;
                    }
                }

                //rect markerrect = {marker->x, marker->y, marker->width, marker->height};
                for(int i = 0; i < g_tiles.size(); i++) {
                    //M(g_tiles[i]->width);
                    
                    if(g_tiles[i]->software == 1) {continue;}
                    if(RectOverlap(g_tiles[i]->getMovedBounds(), markerrect)) {
                        delete g_tiles[i];
                        break;
                    }
                }
            }
        }
    }
    if(devinput[5] && !olddevinput[5] && makingdoor == 0) {
        lx = px;
        ly = py;
        makingdoor = 1;
        selection->image = IMG_Load("tiles/engine/door.png");
        selection->wraptexture = 0;
        selection->texture = SDL_CreateTextureFromSurface(renderer, selection->image);
        SDL_FreeSurface(selection->image);
        
    } else {
        if(devinput[5] && !olddevinput[5] && makingdoor == 1) {

            makingdoor = 0;

            door* d = new door(renderer, "&", "A", selection->x, selection->y, selection->width, selection->height);
            
        }
    }

   
    if(devinput[7] && !olddevinput[7]) {
        //shrink grid
        // if(grid/2 > 0) {
        //     grid /= 2;
        //     M("Grid lowered to " + (int)grid);
        // }
    }

    if(devinput[8] && !olddevinput[8]) {
        //grow grid
        // grid *= 2;
        // M("Grid raised to " + (int)grid);
    }

    if(devinput[9] && !olddevinput[9]) {
        
        boxsenabled=!boxsenabled;
        if(boxsenabled) {
            M("collisions ON");
        } else {
            M("collisions OFF");
        }
    }

    if(devinput[10] && !olddevinput[10]) {
        //check for triangles at mouse
        vector<tri*> deleteMe;
        //rect markerrect = {marker->x, marker->y, marker->width, marker->height };
        for (int i = wallstart/64; i < wallheight/64; i++) {
            for(auto n : g_triangles[i]) {
                if(TriRectOverlap(n, marker->x + 6, marker->y + 6, marker->width - 12, marker->height - 12)) {
                    deleteMe.push_back(n);
                }
            }
        }
        if(deleteMe.size() != 0) {
            int type = deleteMe[0]->type;
            for (auto n:deleteMe) {
                for(auto child:n->children) {
                    delete child;
                }
                delete n;
            }
            //make a new triangle with a different type and current wallheight
            switch(type) {
                case 0:
                    M("0");
                    devinput[15] = 1;
                    break;
                case 1:
                    M("1");
                    devinput[14] = 1;
                    break;
                case 2:
                    M("2");
                    devinput[13] = 1;
                    break;
                case 3:
                    M("3");
                    devinput[12] = 1;
                    break;
            }
        } else {
            //we looked and didnt find anything, lets make on
            devinput[12] = 1;
        }
    }

    if(devinput[11] && !olddevinput[11]) { 
        if(nudge != 0) {
            nudge = 0;
        } else {
            //set material
            //textbox(SDL_Renderer* renderer, const char* content, float size, int x, int y, int width) {
            textbox* consoleDisplay = new textbox(renderer, "console",  WIN_WIDTH * g_fontsize / 2, 0, 0, 4000);
            string input;
            SDL_Event console_event;
            bool polling = 1;
            bool keypressed = 0;
            //const Uint8* keystate = SDL_GetKeyboardState(NULL);
            
            //turn off VSYNC because otherwise we jitter between this frame and the last while typing
            SDL_GL_SetSwapInterval(0);
            while(polling) {
                SDL_Delay(10);
                while( SDL_PollEvent( &console_event ) ){
                    
                    //update textbox
                    string renderinput = ">" + input;
                    SDL_Rect rect = {consoleDisplay->x, consoleDisplay->y, WIN_WIDTH, consoleDisplay->height };
                    consoleDisplay->updateText(renderinput,  WIN_WIDTH * g_fontsize / 2, WIN_WIDTH);
                    
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderFillRect(renderer, &rect);
                    consoleDisplay->render(renderer, 1, 1);
                    SDL_RenderPresent(renderer);
                    switch( console_event.type ){
                        case SDL_QUIT:
                            polling = 0;
                            quit = 1;
                            break;
                        
                        case SDL_KEYDOWN:
                            string name = SDL_GetKeyName( console_event.key.keysym.sym );
                            
                            
                            if(name == "Return") {
                                polling = false;
                                break;
                            }
                            if(name == "Space") {
                                input += " ";
                                break;
                            }
                            if(name == "Backspace") {
                                if(input.length() > 0) {
                                    input = input.substr(0, input.size()-1);
                                    break;
                                }
                            }
                            
                            //if the input wasnt a return or a space or a letter, dont take it at all
                            if(name.length() != 1) {break;}
                            input += name;
                            
                            
                            break;
                        
                    }
                    
                
                }
            }
            SDL_GL_SetSwapInterval(1);
            delete consoleDisplay;
            
            locale loc;
            transform(input.begin(), input.end(), input.begin(), [](unsigned char c){ return std::tolower(c); });
            istringstream line(input);
            string word = "";
            
            while(line >> word) {
                if(word == "show") {
                    line >> word;
                    if(word == "hitboxes") {
                        drawhitboxes = !drawhitboxes;
                        break;
                    }
                    if(word == "marker") {
                        showMarker = !showMarker;
                        if(showMarker) {
                            SDL_SetTextureAlphaMod(marker->texture, 255);
                        } else {
                            SDL_SetTextureAlphaMod(marker->texture, 0);
                        }
                        break;
                    }
                }
                if(word == "info") {
                    M(mapname);
                    
                    M("Entity Count: ");
                    M(g_entities.size());
                    M("MapObject Count:");
                    M(g_mapObjects.size());
                    M("Floor Count: ");
                    M(g_tiles.size() - 9);
                    // M("Number of Shadows: ");
                    // M(g_shadows.size());
                    M("Waypoint Count: ");
                    M(g_waypoints.size());
                    M("Door Count: ");
                    M(g_doors.size());
                    D(mapent->x);
                    D(mapent->y);
                    // int hosting = 0;
                    // int sharing = 0;
                    // for (auto x : g_entities) {
                    //     if(x->wallcap) {
                    //         if(x->asset_sharer) {
                    //             sharing++;
                    //         } else {
                    //             hosting++;
                    //         }
                    //     }
                    // }
                    // D(hosting);
                    // for (auto x : g_entities) {
                    //     if(x->wallcap) {
                    //         if(x->asset_sharer) {
                                
                    //         } else {
                    //             M(x->name);
                    //         }
                    //     }
                    // }
                    // D(sharing);
                    // for (auto x : g_entities) {
                    //     if(x->wallcap) {
                    //         if(x->asset_sharer) {
                    //             M(x->name);
                    //         } else {
                                
                    //         }
                    //     }
                    // }
                    // hosting = 0;
                    // sharing = 0;
                    // for (auto x : g_tiles) {
                    
                    //     if(x->asset_sharer) {
                    //         sharing++;
                    //     } else {
                    //         hosting++;
                    //     }
                    
                    // }
                    // D(hosting);
                    // for (auto x : g_tiles) {
                    //     if(x->asset_sharer) {
                            
                    //     } else {
                    //         M(x->fileaddress);
                    //     }
                    // }
                    // D(sharing);
                    // for (auto x : g_tiles) {
                    //     if(x->asset_sharer) {
                    //         M(x->fileaddress);
                    //     } else {
                            
                    //     }
                    // }
                    
                    
                    //how many textures are loaded
                    
                }
                if(word == "undo") {
                    //delete last collision
                    delete g_mapCollisions[g_mapCollisions.size() - 1];
                    //segfaults on save smt
                }
                if(word == "split") {
                    //split the box collisions into grid-sized blocks
                    //copy array
                    for(int i = 0; i < g_boxs.size(); i++) {
                        vector<box*> tempboxes;
                        for(auto b : g_boxs[i]) {
                            tempboxes.push_back(b);
                        }
                        for(auto b : tempboxes) {
                            int x = b->bounds.x;
                            int y = b->bounds.y;
                            while(x < b->bounds.width + b->bounds.x) {
                                while(y < b->bounds.height + b->bounds.y) {
                                    new box(x, y, 64, 55, b->layer, b->walltexture, b->captexture, b->capped, 0, 0, "0000");
                                    y+=55;
                                }   
                                x+= 64; 
                                y = b->bounds.y;
                            }

                            for(auto child:b->children) {
                                delete child;
                            }

                            delete b;
                        }
                    }
                }
                if(word == "join") {
                    //combine box collisions where possible, likely after a split operation
                    bool refresh = true; //wether to restart the operation
                    while(refresh) {
                        refresh = false;
                        for(int i = 0; i < g_boxs.size(); i++) {
                            for(auto him: g_boxs[i]) {
                                
                                //is there a collision in the space one block to the right of this block, and at the top?
                                //  * * x
                                //  * *
                                for(auto other: g_boxs[i]) {
                                    if(RectOverlap(rect(him->bounds.x + him->bounds.width + 2, him->bounds.y + 2, 64 - 4, 55 - 4), other->bounds)) {
                                        //does it have the same shineTop as him?
                                        if(him->shineTop == other->shineTop) {
                                            //does it have the same height and y position as him?
                                            if(him->bounds.y == other->bounds.y && him->bounds.height == other->bounds.height) {
                                                //does it have the same shineBot as him?
                                                if(him->shineBot == other->shineBot) {
                                                    //shading?
                                                    if(him->shadeBot == other->shadeBot && him->shadeTop == other->shadeTop) {
                                                        //textures?
                                                        if(him->captexture == other->captexture && him->walltexture == other->walltexture) {
                                                            //both capped or not capped?
                                                            if(him->capped == other->capped) {
                                                                //join the two blocks
                                                                string shadestring = "";
                                                                if(him->shadeTop) {shadestring+= "1";} else {shadestring+= "0";}
                                                                if(him->shadeBot) {shadestring+= "1";} else {shadestring+= "0";}
                                                                if(him->shadeLeft) {shadestring+= "1";} else {shadestring+= "0";}
                                                                if(other->shadeRight) {shadestring+= "1";} else {shadestring+= "0";}
                                                            
                                                                new box(him->bounds.x, him->bounds.y, him->bounds.width + other->bounds.width, him->bounds.height, him->layer, him->walltexture, him->captexture, him->capped, him->shineTop, him->shineBot, shadestring.c_str());
                                                                for(auto child:him->children) {
                                                                    delete child;
                                                                }
                                                                delete him; 
                                                                for(auto child:other->children) {
                                                                    delete child;
                                                                }
                                                                delete other;
                                                                refresh = true;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    //vertical pass
                    refresh = true;
                    while(refresh) {
                        refresh = false;
                        for(int i = 0; i < g_boxs.size(); i++) {
                            for(auto him: g_boxs[i]) {
                                //is there a collision in the space directly below this block
                                //  * * 
                                //  * *
                                //  x
                                for(auto other: g_boxs[i]) {
                                    if(RectOverlap(rect(him->bounds.x + 2, him->bounds.y + him->bounds.height + 2, 64 - 4, 55 - 4), other->bounds)) {
                                        //does it have the same width and x position as him?
                                        if(him->bounds.x == other->bounds.x && him->bounds.width == other->bounds.width) {
                                            if(him->shadeLeft == other->shadeLeft && him->shadeRight == other->shadeRight) {
                                                //textures?
                                                if(him->captexture == other->captexture && him->walltexture == other->walltexture) {
                                                    //both capped or not capped?
                                                    if(him->capped == other->capped) {
                                                        //join the two blocks
                                                        string shadestring = "";
                                                        if(him->shadeTop) {shadestring+= "1";} else {shadestring+= "0";}
                                                        if(other->shadeBot) {shadestring+= "1";} else {shadestring+= "0";}
                                                        if(him->shadeLeft) {shadestring+= "1";} else {shadestring+= "0";}
                                                        if(him->shadeRight) {shadestring+= "1";} else {shadestring+= "0";}
                                                    
                                                        new box(him->bounds.x, him->bounds.y, him->bounds.width, him->bounds.height + other->bounds.height, him->layer, him->walltexture, him->captexture, him->capped, him->shineTop, other->shineBot, shadestring.c_str());
                                                        for(auto child:him->children) {
                                                            delete child;
                                                        }
                                                        delete him; 
                                                        for(auto child:other->children) {
                                                            delete child;
                                                        }
                                                        delete other;
                                                        refresh = true;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if(word == "bake") {
                    //set capped
                    for(int i = 0; i < g_boxs.size(); i++) {
                        for(auto b : g_boxs[i]) {
                            rect inneighbor = {b->bounds.x + 2, b->bounds.y + 2, b->bounds.width - 4, b->bounds.height - 4};
                            //set capped for each box
                            if(i == g_layers) { continue; }
                            for(auto n : g_boxs[i+1]) {
                                if(RectOverlap(inneighbor, n->bounds)) {
                                    b->capped = false;
                                }
                            }
                        }
                    }
                    for(int i = 0; i < g_triangles.size(); i++) {
                        for(auto b : g_triangles[i]) {
                            //set capped for each box
                            if(i == g_layers) { continue; }
                            rect a = {((b->x1 + b->x2) /2) - 4, ((b->y1 + b->y2) / 2) - 4, 8, 8};
                            for(auto n : g_triangles[i+1]) {
                                if(TriRectOverlap(n, a)) {
                                    b->capped = false;
                                }
                            }
                        }
                    }
                    //get lighting data to mapcollisions
                    for(int i = 0; i < g_boxs.size(); i++) {
                        
                        for(auto b : g_boxs[i]) {
                            //shade
                            b->shadeTop = false;
                            b->shadeBot = false;
                            b->shadeLeft = false;
                            b->shadeRight = false;
                            rect uneighbor = {b->bounds.x + 2, b->bounds.y - 55 + 2, b->bounds.width - 4, 55 - 4};
                            rect dneighbor = {b->bounds.x + 2, b->bounds.y + 2 + b->bounds.height, b->bounds.width - 4, 55 - 4};b->shineTop = true;
                            b->shineBot = true;
                            //check for overlap with all other boxes
                            for(auto n : g_boxs[i]) {
                                if(n == b) {continue;}
                                
                                if(RectOverlap(n->bounds, uneighbor)) {
                                    b->shineTop = false;
                                }
                                if(RectOverlap(n->bounds, dneighbor)) {
                                    b->shineBot = false;
                                }
                            }
                            for(auto n : g_triangles[i]) {
                                if(TriRectOverlap(n, uneighbor.x, uneighbor.y, uneighbor.width, uneighbor.height)) {
                                    b->shineTop = false;
                                }
                                if(TriRectOverlap(n, dneighbor.x, dneighbor.y, dneighbor.width, dneighbor.height)) {
                                    b->shineBot = false; 
                                    b->shadeBot = 2; //new change
                                }
                                
                            }
                            if(!b->capped) {b->shineTop = false; b->shineBot = false;}

                            
                            uneighbor = {b->bounds.x + 2, b->bounds.y - 55 + 2, b->bounds.width - 4, 55 - 4};
                            dneighbor = {b->bounds.x + 2, b->bounds.y + 2 + b->bounds.height, b->bounds.width - 4, 55 - 4};
                            rect lneighbor = {b->bounds.x + 2 - 64, b->bounds.y + 2, 64 - 4, b->bounds.height - 4};
                            rect rneighbor = {b->bounds.x + 2 + b->bounds.width, b->bounds.y + 2, 64 - 4, b->bounds.height - 4};
                            //check for overlap with tiles if it is on layer 0 and with boxes and triangles a layer below for each dir
                            if(i == 0) {
                                for(auto t: g_tiles) {
                                    if(RectOverlap(t->getMovedBounds(), uneighbor)) {
                                        b->shadeTop = true;
                                    }
                                    if(RectOverlap(t->getMovedBounds(), dneighbor) && b->shadeBot != 2) {
                                        b->shadeBot = true;
                                    }
                                    if(RectOverlap(t->getMovedBounds(), lneighbor)) {
                                        b->shadeLeft = true;
                                    }
                                    if(RectOverlap(t->getMovedBounds(), rneighbor)) {
                                        b->shadeRight = true;
                                    }
                                }
                                //but, if there is a layer 0 block overlapping our neighbor rect, we want to disable that shading
                                //because that would be a random shadow under a block with visible corners
                                for(auto n: g_boxs[0]) {
                                    if(n == b) {continue;}
                                    if(RectOverlap(n->bounds, uneighbor)) {
                                        b->shadeTop = false;
                                    }
                                    if(RectOverlap(n->bounds, dneighbor) && b->shadeBot != 2) {
                                        b->shadeBot = false;
                                    }
                                    if(RectOverlap(n->bounds, lneighbor)) {
                                        b->shadeLeft = false;
                                    }
                                    if(RectOverlap(n->bounds, rneighbor)) {
                                        b->shadeRight = false;
                                    }
                                }
                                //same for tris
                                for(auto t: g_triangles[0]) {
                                    
                                    if(TriRectOverlap(t, uneighbor)) {
                                        //b->shadeTop = false;
                                    }
                                    if(TriRectOverlap(t, dneighbor) && b->shadeBot != 2) {
                                        b->shadeBot = false;
                                    }
                                    if(TriRectOverlap(t, lneighbor)) {
                                        b->shadeLeft = false;
                                    }
                                    if(TriRectOverlap(t, rneighbor)) {
                                        b->shadeRight = false;
                                    }
                                }
                            }
                            if(i > 0){
                                for(auto n: g_boxs[i-1]) {
                                    if(!n->capped) {continue;}
                                    if(RectOverlap(n->bounds, uneighbor)) {
                                        b->shadeTop = true;
                                    }
                                    if(RectOverlap(n->bounds, dneighbor)) {
                                        b->shadeBot = true;
                                    }
                                    if(RectOverlap(n->bounds, lneighbor)) {
                                        b->shadeLeft = true;
                                    }
                                    if(RectOverlap(n->bounds, rneighbor)) {
                                        b->shadeRight = true;
                                    }
                                }
                                for(auto t: g_triangles[i-1]) {
                                    if(!t->capped) {continue;}
                                    if(TriRectOverlap(t, uneighbor)) {
                                        b->shadeTop = true;
                                    }
                                    if(TriRectOverlap(t, dneighbor)) {
                                        b->shadeBot = true;
                                    }
                                    if(TriRectOverlap(t, lneighbor)) {
                                        b->shadeLeft = true;
                                    }
                                    if(TriRectOverlap(t, rneighbor)) {
                                        b->shadeRight = true;
                                    }
                                }
                                
                                
                            }
                        }
                    }

                    //bake triangles
                    for(int i = 0; i < g_layers; i++) {
                        for(auto tri : g_triangles[i]) {
                            tri->shaded = 0;
                            if(tri->layer == 0) {
                                for(auto tile : g_tiles) {
                                    if(TriRectOverlap(tri, tile->getMovedBounds())) {
                                        tri->shaded = 1;
                                    }
                                }
                            } else {
                                for(auto b : g_boxs[i-1]) {
                                    if(TriRectOverlap(tri, b->bounds.x + 2, b->bounds.y + 2,b->bounds.width - 4, b->bounds.height - 4)) {
                                        tri->shaded = 1;
                                    }
                                }
                            }
                        }
                    }
                }
                if(word == "save") {
                    if(line >> word) { 
                        
                        //add warning for file overright
                        if(word != g_map &&fileExists("maps/" + word + "/" + word + ".map")) {
                            if(yesNoPrompt("Map \"" + word + "\" already exists, would you like to overwrite?") == 1) { 
                                M("Canceled overwrite");
                                break;
                            }
                        }
                        
                        std::filesystem::create_directories("maps/" + word);
                        word = "maps/" + word  + "/" + word + ".map";
                        
                        ofile.open(word);

                        bool limitflag = 0;
                        for (auto x: limits) { if(x != 0) {limitflag = 1; }}
                        
                        if(limitflag) {
                            ofile << "limits";
                            for (auto x: limits) {ofile << " " << x; }
                            ofile << endl;
                        }
                        if(g_backgroundLoaded && backgroundstr != "") {
                            ofile << "bg " << backgroundstr << endl;
                        }
                        for(int i = 0; i < g_layers; i ++) {
                            for (auto n : g_triangles[i]) {
                                ofile << "triangle " << n->x1 << " " << n->y1 << " " << n->x2 << " " << n->y2 << " " << i << " " << n->walltexture << " " << n->captexture <<  " " << n->capped << " " << n->shaded << endl;
                            }
                        }
                        for (long long unsigned int i = 0; i < g_entities.size(); i++) {
                            if(!g_entities[i]->inParty) {
                                ofile << "entity " << g_entities[i]->name << " " << to_string(g_entities[i]->x) << " " << to_string(g_entities[i]->y) <<  " " << to_string(g_entities[i]->z) << endl;
                                
                            }
                        }
                        
                        for (long long unsigned int i = 0; i < g_mapObjects.size(); i++) {    
                            //ofile << "mapObject " << g_mapObjects[i]->name << " " << g_mapObjects[i]->mask_fileaddress << " " << to_string(g_mapObjects[i]->x) << " " << to_string(g_mapObjects[i]->y) << " " << to_string(g_mapObjects[i]->z) << " " << to_string(g_mapObjects[i]->width) << " " << to_string(g_mapObjects[i]->height) <<  " " << g_mapObjects[i]->wall << " " << g_mapObjects[i]->extraYOffset << " "  << g_mapObjects[i]->sortingOffset << endl;
                        }

                        for (long long unsigned int i = 0; i < g_tiles.size(); i++) {
                            //dont save map graphics
                            if(g_tiles[i]->fileaddress.find("engine") != string::npos ) { continue; }
                            //sheared tiles are made on map loading, so dont save em
                            if(g_tiles[i]->mask_fileaddress.find("engine") != string::npos ) { continue; }
                            //lighting, but not occlusion is also generated on map load
                            if(g_tiles[i]->fileaddress.find("lighting") != string::npos && !(g_tiles[i]->fileaddress.find("OCCLUSION") != string::npos)) { continue; }
                            
                            ofile << "tile " << g_tiles[i]->fileaddress << " " << g_tiles[i]->mask_fileaddress << " " << to_string(g_tiles[i]->x) << " " << to_string(g_tiles[i]->y) << " " << to_string(g_tiles[i]->width) << " " << to_string(g_tiles[i]->height) << " " << g_tiles[i]->z << " " << g_tiles[i]->wraptexture << " " << g_tiles[i]->wall << " " << g_tiles[i]->dxoffset << " " << g_tiles[i]->dyoffset << endl;
                        }
                        for (long long unsigned int i = 0; i < g_heightmaps.size(); i++) {
                            
                            ofile << "heightmap " << g_heightmaps[i]->binding << " " << g_heightmaps[i]->name << " " << g_heightmaps[i]->magnitude << endl;
                        }
                        for (long long unsigned int j = 0; j < g_layers; j++){
                            for (long long unsigned int i = 0; i < g_boxs[j].size(); i++){
                                string shadestring = "";
                                if(g_boxs[j][i]->shadeTop) {shadestring+= "1";} else {shadestring+= "0";}
                                if(g_boxs[j][i]->shadeBot == 1) {shadestring+= "1";} else {if(g_boxs[j][i]->shadeBot == 2) {shadestring+= "2";} else {shadestring+="0";}}
                                if(g_boxs[j][i]->shadeLeft) {shadestring+= "1";} else {shadestring+= "0";}
                                if(g_boxs[j][i]->shadeRight) {shadestring+= "1";} else {shadestring+= "0";}
                                ofile << "box " << to_string(g_boxs[j][i]->bounds.x) << " " << to_string(g_boxs[j][i]->bounds.y) << " " << to_string(g_boxs[j][i]->bounds.width) << " " << to_string(g_boxs[j][i]->bounds.height) << " " << j << " " << g_boxs[j][i]->walltexture << " " << g_boxs[j][i]->captexture << " " << g_boxs[j][i]->capped << " " << g_boxs[j][i]->shineTop << " " << g_boxs[j][i]->shineBot << " " << shadestring << endl;
                            }
                        }
                        for (long long unsigned int i = 0; i < g_doors.size(); i++){
                            ofile << "door " << g_doors[i]->to_map << " " << g_doors[i]->to_point << " " << g_doors[i]->x << " " << g_doors[i]->y << " " << g_doors[i]->width << " " << g_doors[i]->height << endl;
                        }
                        for (long long unsigned int i = 0; i < g_triggers.size(); i++){
                            ofile << "trigger " << g_triggers[i]->binding << " " << g_triggers[i]->x << " " << g_triggers[i]->y << " " << g_triggers[i]->width << " " << g_triggers[i]->height << " " << g_triggers[i]->targetEntity << endl;
                        }
                        for (int i = 0; i < g_worldsounds.size(); i++) {
                            ofile << "worldsound " << g_worldsounds[i]->name << " " << g_worldsounds[i]->x << " " << g_worldsounds[i]->y << endl;
                        }
                        for (int i = 0; i < g_musicNodes.size(); i++) {
                            ofile << "music " << g_musicNodes[i]->name << " " << g_musicNodes[i]->x << " " << g_musicNodes[i]->y << endl;
                        }
                        for (int i = 0; i < g_cueSounds.size(); i++) {
                            ofile << "cue " << g_cueSounds[i]->name << " " << g_cueSounds[i]->x << " " << g_cueSounds[i]->y << " " << g_cueSounds[i]->radius << endl;
                        }
                        for (int i = 0; i < g_waypoints.size(); i++) {
                            ofile << "waypoint " << g_waypoints[i]->name << " " << g_waypoints[i]->x << " " << g_waypoints[i]->y << endl;
                        }
                        for (int i = 0; i < g_navNodes.size(); i++) {
                            ofile << "navNode " << g_navNodes[i]->x << " " << g_navNodes[i]->y << endl;
                        }
                        for (int i = 0; i < g_navNodes.size(); i++) {
                            ofile << "navNodeEdge " << i << " ";
                            for (int j = 0; j < g_navNodes[i]->friends.size(); j++) {
                                auto itr = find(g_navNodes.begin(), g_navNodes.end(), g_navNodes[i]->friends[j]);
                                ofile << std::distance(g_navNodes.begin(), itr) << " ";
                            }
                            ofile << endl;
                        }
                        for (int i = 0; i < g_listeners.size(); i++) {
                            ofile << "listener " << g_listeners[i]->entityName << " " << g_listeners[i]->block << " " << g_listeners[i]->condition << " " << g_listeners[i]->binding << " " << g_listeners[i]->x << " " << g_listeners[i]->y << endl;
                        }
                        
                        
                        ofile.close();
                        break;
                    }
                }
                if(word == "delete") {
                    if(line >> word) {
                        std::filesystem::remove_all("maps/" + word);
                    }
                }
                if(word == "load") {
                    if(line >> word) {
                        //must close file before renaming it
                        ofile.close();
                        word = "maps/" + word + "/" + word + ".map";
                        clear_map(g_camera);
                        M("Lets check boxs size after clearing and before loading");
                        D(g_boxs[0].size());
                        load_map(renderer, word.c_str(), "a");
                        init_map_writing(renderer);
                        break;
                    }
                }
                if(word == "clear") {
                    line >> word;
                    if(word == "map") {
                        clear_map(g_camera);
                        //load_map(renderer, "empty.map", "a");
                        init_map_writing(renderer);
                        break;
                    }
                    if(word == "navnodes") {
                        int size = g_navNodes.size();
                        for(int i = 0; i < size; i++) {
                            delete g_navNodes[0];
                        }
                        break;
                    }
                    
                }
                if(word == "stake") {
                    //stake as in, stake something at the cursor
                    line >> word;
                    if(word == "leftbound" || word == "lb") {
                        limits[0] = marker->x;
                        limits[1] = marker->y;
                        break;
                    }
                    if(word == "rightbound" || word == "rb") {
                        limits[2] = marker->x + marker->width;
                        limits[3] = marker->y + marker->height;
                        break;
                    }
                }
                if(word == "shove") {
                    protag->x = 100000;
                    protag->y = 100000;
                }
                if(word == "set" || word == "s") {
                    line >> word;
                    
                    if(word == "navnodecorneringwidth" || word == "nncw" || word == "navcornering") {
                        //this is the width of a boxtrace used for testing if navnodes should be joined.
                        //if AI are getting caught on walls, increase this to about the width of their hitboxes
                        float inp;
                        if(line >> inp && inp > 0) {
                            mapeditorNavNodeTraceRadius = inp;
                        }
                        break;
                    }
                    if(word == "heightmapmagnitude" || word == "hmmg" || word == "heightmap") {
                        float inp;
                        if(line >> inp && inp > 0 && g_heightmaps.size() > 0) {
                            //the factor is to convert user input to world blocks
                            g_heightmaps.at(g_heightmaps.size() - 1)->magnitude = inp * (float)(64.00/255.00);
                        }
                        break;
                    }
                    if(word == "background" || word == "bg") {
                        string str;
                        
                        if(line >> str) {
                            backgroundstr = str;
                        }
                        if(g_backgroundLoaded) {
                            SDL_DestroyTexture(background);
                        }
                        SDL_Surface* bs = IMG_Load(("tiles/backgrounds/" + backgroundstr + ".png").c_str());
                        background = SDL_CreateTextureFromSurface(renderer, bs);
                        g_backgroundLoaded = 1;
                        SDL_FreeSurface(bs);
                        break;
                    }
                    if(word == "texturedirectory" || word == "td" || word == "theme") {
                        string str;
                        if(line >> str) {
                            textureDirectory = str + "/";
                        }

                        //re-populate the array of textures that we rotate thru for creating floors, walls, and caps
                        texstrs.clear();
                        string path = "tiles/diffuse/" + textureDirectory;
                        D(path);
                        if(!filesystem::exists(path)) {
                            M("Theme " + path + "not found");
                            break;
                        }
                        for (const auto & entry : filesystem::directory_iterator(path)) {
                            texstrs.push_back(entry.path());
                        }
                        floortexIndex = 0;
                        captexIndex = 0;
                        walltexIndex = 0;

                        floortex = texstrs[floortexIndex];
                        delete floortexDisplay;
                        floortexDisplay = new ui(renderer, floortex.c_str(), 0, 0.9, 0.1, 0.1, 0);
                        walltex = texstrs[walltexIndex];
                        delete walltexDisplay;
                        walltexDisplay = new ui(renderer, walltex.c_str(), 0.1, 0.9, 0.1, 0.1, 0);
                        captex = texstrs[captexIndex];
                        delete captexDisplay;
                        captexDisplay = new ui(renderer, captex.c_str(), 0.2, 0.9, 0.1, 0.1, 0);
                        

                        break;
                    }
                    if(word == "tdx") {
                        float number;
                        line >> number;
                        if(g_tiles.size() > 0){
                            g_tiles[g_tiles.size()-1]->dxoffset = number;
                        }
                        break;
                    }

                    if(word == "tdy") {
                        float number;
                        line >> number;
                        if(g_tiles.size() > 0){
                            g_tiles[g_tiles.size()-1]->dyoffset = number;
                        }
                        break;
                    }

                    if(word == "shine" || word == "sh") {
                        line >> shine;
                        break;
                    }
                    
                    if(word == "limits") {
                        int a, b, c, d;
                        line >> a >> b >> c >> d;
                        limits[0] = a;
                        limits[1] = b;
                        limits[2] = c;
                        limits[3] = d;
                        break;
                    }
                    if(word == "navdensity") {
                        line >> navMeshDensity;
                        break;
                    }
                    
                    if(word == "wallheight" || word == "wh") {
                        line >> wallheight;
                        wallheight *= 64;
                        break;
                    }
                    if(word == "wallstart" || word == "ws") {
                        line >> wallstart;
                        wallstart *= 64;
                        break;
                    }
                    if(word == "occlusion" || word == "o") {
                        line >> occlusion;
                        break;
                    }
                    if(word == "wall" || word == "w") {
                        line >> walltex;
                        walltex = "tiles/diffuse/" + textureDirectory + walltex + ".png";

                        break;
                    }
                    D("time to set the floor");
                    if(word == "floor" || word == "f") {
                        D("time to set the floor");
                        line >> floortex;
                        D(textureDirectory);
                        floortex = "tiles/diffuse/" + textureDirectory + floortex + ".png";
                        D(floortex);
                        break;
                    }
                    if(word == "cap" || word == "c") {
                        line >> captex;
                        captex = "tiles/diffuse/" + textureDirectory + captex + ".png";
                        break;
                    }
                    if(word == "mask" || word == "m") {
                        line >> masktex;
                        masktex = "tiles/masks/" + masktex + ".png";
                        break;
                    }
                    if(word == "layer") {
                        int number;
                        if(line >> number) {
                            layer = number;
                        };
                        break;
                    }
                    if(word == "mwalls") {
                        bool input;
                        if(line >> input) {
                            autoMakeWalls = input;
                        }
                        break;
                    }
                    if(word == "mwallscaps") {
                        bool input;
                        if(line >> input) {
                            autoMakeWalls = input;
                            autoMakeWallcaps = input;
                        }
                        break;
                    }
                    if(word == "msolid") {
                        bool input;
                        if(line >> input) {
                            makeboxs = input;
                        }
                        break;
                    }
                    if(word == "mcaps") {
                        bool input;
                        if(line >> input) {
                            autoMakeWallcaps = input;
                        }
                        break;
                    }
                    if(word == "my") {
                        line >> word;
                        if(word == "speed") {
                            float speed;
                            if(line >> speed) {
                                mapent->xmaxspeed = speed;
                                mapent->ymaxspeed = speed;
                                    
                                break;
                            }        
                        }
                        if(word == "z") {
                            float z;
                            if(line >> z) {
                                mapent->z = z;
                                break;
                            }        
                        }
                    }

                    if(word == "drawcolor") {
                        
                        if(line >> debug_r >> debug_g >> debug_b) {
                            
                            break;
                        }
                    }
                    if(word == "tiling") {
                        bool num;
                        if(line >> num) {
                            tiling = num;
                        }
                        break;
                    }
                    if(word == "boxs") {
                        bool num;
                        if(line >> num) {
                            boxsenabled = num;
                        }
                        break;
                    }
                    if(word == "grid") {
                        line >> grid;
                        break;
                    }
                    if(word == "mute") {
                        bool num;
                        if(line >> num) {
                            g_mute = num;
                        }
                        if(num == 1) {
                            Mix_HaltMusic();
                        }
                        break;
                    }
                }
                
                if(word == "adj" || word == "adjust") {
                    //adjust
                    line >> word;
                    if(word == "bx") {
                        //g_entities[g_entities.size() - 1]->bounds.x = (g_entities[g_entities.size() - 1]->width/2 - g_entities[g_entities.size() - 1]->shadow->width/2);
                        int number;
                        line >> number;
                        g_entities[g_entities.size() - 1]->bounds.x = number;
                        break;
                    }
                    if(word == "by") {
                        int number;
                        line >> number;
                        g_entities[g_entities.size() - 1]->bounds.y = number;
                        break;
                    }
                    if(word == "bw") {
                        line >> g_entities[g_entities.size() - 1]->bounds.width;
                        break;
                    }
                    if(word == "bh") {
                        line >> g_entities[g_entities.size() - 1]->bounds.height;
                        break;
                    }
                    if(word == "shadowx") {
                        line >> g_entities[g_entities.size() - 1]->shadow->x;
                        break;
                    }
                    if(word == "shadowy") {
                        line >> g_entities[g_entities.size() - 1]->shadow->y;
                        break;
                    }
                    
                }
                if(word == "where") {
                    M(px);
                    M(py);
                }
                if(word == "reset"  || word == "rs") {
                    line >> word;
                    if(word == "texturedirectory" || word == "td" || word == "theme") {
                        textureDirectory = "";
                        break;
                    }
                    if(word == "grid") {
                        grid = 60;
                        break;
                    }
                    if(word == "mask") {
                        masktex = "&";
                        break;
                    }
                    if(word == "me") {
                        mapent->x = 0;
                        mapent->y = 0;
                        break;
                    }
                    if(word == "my") {
                        line >> word;
                        if(word == "speed") {
                            mapent->xmaxspeed = 110;
                            mapent->ymaxspeed = 110;
                            break;
                        }
                        if(word == "spot" || word == "position") {
                            mapent->x = 0;
                            mapent->y = 0;    
                            break;
                        }
                    }
                    if(word == "bounds") {
                        g_camera.resetCamera();
                        limits[0] = 0;
                        limits[1] = 0;
                        limits[2] = 0;
                        limits[3] = 0;
                        
                    }
                }
                if(word == "teleport" || word == "tp") {
                    int x, y;
                    line >> x >> y;
                    mapent->x = x;
                    mapent->y = y;
                    break;
                }
                if(word == "solidify") {
                    line >> entstring;

                    entity* solidifyMe = 0;
                    solidifyMe = searchEntities(entstring);
                    if(solidifyMe != 0) {
                        solidifyMe->solidify();
                    }
                }
                if(word == "unsolidify") {
                    line >> entstring;

                    entity* solidifyMe = 0;
                    solidifyMe = searchEntities(entstring);
                    if(solidifyMe != 0) {
                        solidifyMe->unsolidify();
                    }
                }
                if(word == "entity" || word == "ent") {
                    line >> entstring;
                    float z = 0;
                    line >> z;
                    //actually spawn the entity in the world
                    //string loadstr = "entities/" + entstring + ".ent";
                    const char* plik = entstring.c_str();
                    entity* e = new entity(renderer,  plik);
                    e->x = px + marker->width/2 - (e->getOriginX());
                    e->y = py+ marker->height/2 - (e->getOriginY());
                    e->stop_hori();
                    e->stop_verti();
                    e->z = mapent->z;
                    e->shadow->x = e->x + e->shadow->xoffset;
                    e->shadow->y = e->y + e->shadow->yoffset;
                    nudge = e;
                    D(e->bounds.x);
                    break;
                }
                if(word == "sound" || word == "snd") {
                    line >> entstring;
                    worldsound* w = new worldsound(entstring, px + marker->width/2, py + marker->height/2);
                } 
                if(word == "music" || word == "m") {
                    line >> entstring;
                    musicNode* m = new musicNode(entstring, px + marker->width/2, py + marker->height/2);
                }
                if(word == "cue") {
                    //for cue, input soundname and radius
                    line >> entstring;
                    float radius;
                    line >> radius;
                    cueSound* m = new cueSound(entstring, px + marker->width/2, py + marker->height/2, radius);
                }
                if(word == "way" || word == "w") {
                    line >> entstring;
                    waypoint* m = new waypoint(entstring, px + marker->width/2, py + marker->height/2);
                }
                if(word == "wayatme" || word == "wam") {
                    line >> entstring;
                    waypoint* m = new waypoint(entstring, mapent->getOriginX(), mapent->getOriginY());
                }
                if(word == "linkdoor" || word == "ld" || word == "door" || word == "d") { //consider renaming this "link" or something other than "door" because it doesnt make doors
                    string mapdest, waydest;
                    line >> mapdest >> waydest;
                    if(g_doors.size() > 0) {
                        g_doors[g_doors.size() - 1]->to_map = mapdest;
                        g_doors[g_doors.size() - 1]->to_point = waydest;
                        
                    }
                }
                if(word == "trigger" || word == "t") {
                    string fbinding;
                    string fentity;
                    line >> fbinding;
                    line >> fentity;
                    if(g_triggers.size() > 0) {
                        g_triggers[g_triggers.size() - 1]->binding = fbinding;
                        if(fentity.length() > 0) {
                            D(fentity);
                            g_triggers[g_triggers.size() - 1]->targetEntity = fentity;
                        }
                    }
                }
                if(word == "listener" || word == "l") {
                    M("LISTENER EVENT ENTNAME BLOCK VALUE");
                    string fbinding, entstr, blockstr, valuestr;
                    line >> fbinding >> entstr >> blockstr >> valuestr;
                    if(valuestr != "") {
                        listener* g = new listener(entstr, stoi( blockstr), stoi(valuestr), fbinding, px + 0.5 * marker->width, py + 0.5 * marker->height);
                    }
                }
                if(word == "navnode") {
                    navNode* n = new navNode(px + marker->width/2, py + marker->height/2);
                }
                
                if(word == "navlink") {
                    //delete nodes inside of walls
                    for (int i = 0; i < g_navNodes.size(); i++) {
                        for (int j = 0; j < g_boxs.size(); j++) {
                            rect node = rect(g_navNodes[i]->x - 5, g_navNodes[i]->y - 5, g_navNodes[i]->x + 10, g_navNodes[i]->y + 10);
                            if(RectOverlap(node, g_boxs[0][j]->bounds)) {
                                delete g_navNodes[i];
                                break;
                            }
                        }
                    }

                    for (int i = 0; i < g_navNodes.size(); i++) {
                        for (int j = 0; j < g_navNodes.size(); j++) {
                            if(i == j) {continue;}
                            if(LineTrace(g_navNodes[i]->x, g_navNodes[i]->y, g_navNodes[j]->x, g_navNodes[j]->y, 80)) {
                                g_navNodes[i]->Add_Friend(g_navNodes[j]);
                            }
                        }
                        
                    }
                    //delete nodes with no friends
                    for (int i = 0; i < g_navNodes.size(); i++) {
                        if(g_navNodes[i]->friends.size() == 0) {
                            delete g_navNodes[i];
                        }
                    }
                    Update_NavNode_Costs(g_navNodes);
                }
        
            }

        }
    }
    
    if(devinput[12] && !olddevinput[12]) {
        //make triangle
        tri* n;
        for(int i = wallstart/64; i < wallheight / 64; i++){
            bool fcap = (!(i + 1 < wallheight/64));
            n = new tri(marker->x + marker->width, marker->y, marker->x, marker->y + marker->height, i, walltex, captex, fcap, 0);
        }
        if(autoMakeWallcaps) {
            int step = g_platformResolution;
            for (int i = 0; i < 55; i+=step) {
                mapObject* e = new mapObject(renderer, captex, "tiles/engine/a.png", marker->x, marker->y + i + step, wallheight, 64 - 1, step, 0);
                n->children.push_back(e);
            }
            
        }


        int step = 2;
        int vstep = 64;
        if(autoMakeWalls){
            //a tile on the floor to help with the edge of the diagonal wall pieces
            //this tile won't be saved, because it uses an engine mask
            //tile* t = new tile(renderer, walltex.c_str(), "tiles/engine/a.png", marker->x, marker->y - 1, 64 - 1, 54 + 1, layer, 1, 1, 0, 0);    
            for (int j = wallstart; j < wallheight; j+=vstep) {
                for (int i = 0; i < 64; i+=step) {
                    mapObject* e = new mapObject(renderer, walltex, "&", marker->x + i, marker->y + marker->height - (i * XtoY) - 1, j, step,  ceil(64 * XtoZ) + 1, 1, (i * XtoY));
                    n->children.push_back(e);
                }
            }
        }

        
    }
    if(devinput[13] && !olddevinput[13]) {
        tri* n;
        //make triangle
        for(int i = wallstart/64; i < wallheight / 64; i++){
            bool fcap = (!(i + 1 < wallheight/64));
            n = new tri(marker->x, marker->y, marker->x + marker->width, marker->y + marker->height, i, walltex, captex, fcap, 0);
        }
        if(autoMakeWallcaps) {
            int step = g_platformResolution;
            for (int i = 0; i < 55; i+=step) {
                mapObject* e = new mapObject(renderer, captex, "tiles/engine/b.png", marker->x + 1, marker->y + i + step, wallheight, 64 - 1, step, 0);
                n->children.push_back(e);
            }
            
        }

        //a tile on the floor to help with the edge of the diagonal wall pieces
        //tile* t = new tile(renderer, walltex.c_str(), "tiles/engine/b.png", marker->x + 1, marker->y - 1, 64 - 1, 54 + 1, layer, 1, 1, 0, 0);

        int step = 2;
        int vstep = 64;
        if(autoMakeWalls){
            for (int j = wallstart; j < wallheight; j+=vstep) {
                for (int i = 0; i < 64; i+=step) {
                    mapObject* e = new mapObject(renderer, walltex, "&", marker->x + i, marker->y + marker->height - (((64 - step) - i) * XtoY) - 1, j, step,  ceil(64 * XtoZ) + 1, 1, ((64 - i) * XtoY));
                    n->children.push_back(e);
                }
            }
        }
        
    }
    if(devinput[14] && !olddevinput[14]) {
        tri* n;
        //make triangle
        for(int i = wallstart/64; i < wallheight / 64; i++){
            bool fcap = (!(i + 1 < wallheight/64));
            n = new tri(marker->x, marker->y + marker->height, marker->x + marker->width, marker->y, i, walltex, captex, fcap, 0); 
        }
        int step = g_platformResolution;
        for (int i = 0; i < 55; i+=step) {
            mapObject* child = new mapObject(renderer, n->captexture, "tiles/engine/c.png", n->x1, n->y2 + i + step, n->layer * 64 + 64, 64 - 1, step, 0);
            n->children.push_back(child);
        }
    }
    if(devinput[15] && !olddevinput[15]) {
        tri* n;
        //make triangle
        for(int i = wallstart/64; i < wallheight / 64; i++){
            bool fcap = (!(i + 1 < wallheight/64));
            n = new tri(marker->x + marker->width, marker->y + marker->height, marker->x, marker->y, i, walltex, captex, fcap, 0); 
        }
        int step = g_platformResolution;
        for (int i = 0; i < 55; i+=step) {
            mapObject* child = new mapObject(renderer, n->captexture, "tiles/engine/d.png", n->x2, n->y2 + i + step, n->layer * 64 + 64, 64 - 1, step, 0);
            n->children.push_back(child);
        }
        
    }

    //change wall, cap, and floor textures
    if(devinput[16] && !olddevinput[16]) {
        if(!g_holdingCTRL) {
            floortexIndex++;
        } else {
            floortexIndex--;
        }
        if(floortexIndex == texstrs.size()) {
            floortexIndex = 0;
        }
        if(floortexIndex == -1) {
            floortexIndex = texstrs.size()-1;
        }
        
        floortex = texstrs[floortexIndex];
        delete floortexDisplay;
        floortexDisplay = new ui(renderer, floortex.c_str(), 0, 0.9, 0.1, 0.1, 0);
    }
    
    
    if(devinput[17] && !olddevinput[17]) {
       if(!g_holdingCTRL) {
            walltexIndex++;
        } else {
            walltexIndex--;
        }
        if(walltexIndex == texstrs.size()) {
            walltexIndex = 0;
        }
        if(walltexIndex == -1) {
            walltexIndex = texstrs.size()-1;
        }
       walltex = texstrs[walltexIndex];
       delete walltexDisplay;
        walltexDisplay = new ui(renderer, walltex.c_str(), 0.1, 0.9, 0.1, 0.1, 0);
    }

    if(devinput[18] && !olddevinput[18]) {
        if(!g_holdingCTRL) {
            captexIndex++;
        } else {
            captexIndex--;
        }
        if(captexIndex == texstrs.size()) {
            captexIndex = 0;
        }
        if(captexIndex == -1) {
            captexIndex = texstrs.size()-1;
        }
        captex = texstrs[captexIndex];
        delete captexDisplay;
        captexDisplay = new ui(renderer, captex.c_str(), 0.2, 0.9, 0.1, 0.1, 0);
    }
    
    //update position of markerz
    markerz->x = marker->x;
    float boffset = wallheight;
    boffset /= 2;
    markerz->y = marker->y - boffset;
    markerz->width = marker->width;
    markerz->height = marker->height;
    markerz->wraptexture = 0;

    marker->y -= wallstart/2;
    //done to prevent double keypresses
    for(int i = 0; i < 50; i++) {
        olddevinput[i] = devinput[i];
    }

}


void close_map_writing() {
    ofile.close();
}



//specific class, the ui that the player will use 90% of the time. Should have a spot for dialogue, inventory, pause screen.
class adventureUI {
public:
	ui* talkingBox = 0;
	textbox* talkingText = 0;
    textbox* responseText = 0;
	string pushedText; //holds what will be the total contents of the messagebox. 
	string curText; //holds what the user currently sees; e.g. half of the message because it hasnt been typed out yet
	bool typing = false; //true if text is currently being typed out to the window.
	Mix_Chunk* blip =  Mix_LoadWAV( "sounds/voice-bogged.wav" );
    Mix_Chunk* confirm_noise = Mix_LoadWAV( "sounds/peg.wav" );
	vector<string>* sayings;
	entity* talker = 0;
	bool askingQuestion = false; //set if current cue is a question
    string response = "tired"; //contains the last response the player gave to a question
    vector<string> responses; //contains each possible response to a question
    int response_index = 0; //number of response from array responses
    int sleepingMS = 0; //MS to sleep cutscene/script
    bool sleepflag = 0; //true for one frame after starting a sleep

	void showTalkingUI() {
		M("showTalkingUI()");
        talkingBox->show = 1;
		talkingText->show = 1;
        talkingText->updateText("",34, 34);
        responseText->show = 1;
        
	}
	void hideTalkingUI() {
		M("hideTalkingUI()");
        talkingBox->show = 0;
		talkingText->show = 0;
        responseText->show = 0;
        responseText->updateText("",34, 34);
	}

	adventureUI(SDL_Renderer* renderer) {
		talkingBox = new ui(renderer, "ui/menu9patchblack.png", 0, 0.65, 1, 0.35, 0);
		talkingBox->patchwidth = 213;
		talkingBox->patchscale = 0.4;
		talkingBox->is9patch = true;
        talkingBox->persistent = true;

		talkingText = new textbox(renderer, "I hope you find what your looking for. Extra text to get to four lines of dialogue in the dialogue box. We still need a little more, hang on... there we go", WIN_WIDTH *g_fontsize, 0, 0, 0.9);
		talkingText->boxWidth = 0.95;
		talkingText->width = 0.95;
		talkingText->boxHeight = 0.25;
		talkingText->boxX = 0.05;
		talkingText->boxY = 0.7;
		talkingText->worldspace = 1;

        responseText = new textbox(renderer, "Yes", WIN_WIDTH * 0.05, 0, 0, 0.9);
		responseText->boxWidth = 0.95;
		responseText->width = 0.95;
		responseText->boxHeight = 0.;
		responseText->boxX = 0.05;
		responseText->boxY = 0.9;
		responseText->worldspace = 1;
		
		hideTalkingUI();
	}
	
	~adventureUI() {
		M("~adventureUI()");
		Mix_FreeChunk(blip);
		Mix_FreeChunk(confirm_noise);
        delete talkingBox;
		delete talkingText;
	}


	void pushText(entity* ftalker) {
		talker = ftalker;
        if(sayings->at(talker->dialogue_index).at(0) == '%') {
            pushedText = sayings->at(talker->dialogue_index).substr(1);
        } else {
            pushedText = sayings->at(talker->dialogue_index);
        }
		curText = "";
		typing = true;
		showTalkingUI();
	}
	
	void updateText() {
		talkingText->updateText(curText, WIN_WIDTH *g_fontsize, 0.9);
        
        // if(sleepingMS > 1) {
        //     sleepingMS -= elapsed;
        //     this->hideTalkingUI();
        //     return;
        // } else {
        //     if(sleepflag){
        //         this->showTalkingUI();
        //         sleepflag = 0;
        //     }
        // }

        if(askingQuestion) {
            string former = "   ";
            string latter = "   ";
            if(response_index > 0) {
                former = " < ";
            }
            if(response_index < responses.size() - 1) {
                latter = " > ";
            }
            responseText->updateText(former + responses[response_index] + latter, WIN_WIDTH *g_fontsize, 0.9);
            responseText->show = 1;
            response = responses[response_index];
            
        } else {
            responseText->updateText("", WIN_WIDTH *g_fontsize, 0.9);
            responseText->show = 0;
        }
        
        if(pushedText != curText) {
			int index = curText.length();
			curText += pushedText.at(index);
			
			//Play a clank
			if(blip != NULL) {
                Mix_HaltChannel(6);    
                Mix_VolumeChunk(blip, 20);
                playSound( 6, blip, 0 );
            }
		} else {
			typing = false;

            //the absurdity of this check
            if(!protag_is_talking && talker == protag) { 
                //this means that a trigger has opened a dialogue prompt, and the text has been completed.
                //talker->dialogue_index++;
                //continueDialogue();
                //adventureUIManager->hideTalkingUI();
                //talker = NULL;
                
                //continueDialogue();
            }

		}
	}
	void continueDialogue() {
        protag_is_talking = 1;

        if(sleepingMS > 1) {
            sleepingMS -= elapsed;
            return;
        } else {
            if(sleepflag){
                this->showTalkingUI();
                sleepflag = 0;
            }
        }

        //showTalkingUI();
		if(talker->sayings.at(talker->dialogue_index + 1) == "#") {
			protag_is_talking = 2;
			adventureUIManager->hideTalkingUI();
			talker->dialogue_index = 0;
			talker->animate = 0;
            talker->flip = SDL_FLIP_NONE;
            talker->animation = talker->defaultAnimation;
			return;
		} 

		//question
		if(talker->sayings.at(talker->dialogue_index + 1).at(0) == '%') {
			//make a question
			talker->dialogue_index++;
			pushText(talker);
			askingQuestion = true;
            //put responses in responses vector
            int j = 1;
            string res = talker->sayings.at(talker->dialogue_index + j).substr(1);
            responses.clear();
            while (res.find(':') != std::string::npos) {
                responses.push_back(res.substr(0, res.find(':')));
                j++;
                res = talker->sayings.at(talker->dialogue_index + j).substr(1);
            }
			return;
		} else {
            askingQuestion = false;
		}
        //write selfdata 5->[4]
        if(regex_match (talker->sayings.at(talker->dialogue_index + 1), regex("[[:digit:]]+\\-\\>\\[[[:digit:]]+\\]"))) {
            string s = talker->sayings.at(talker->dialogue_index + 1);
            int value = stoi( s.substr(0, s.find('-')) ); s.erase(0, s.find('-') + 1);
            string blockstr = s.substr(s.find('[')); 
            blockstr.pop_back(); blockstr.erase(0, 1);
            int block = stoi (blockstr);
            talker->data[block] = value;
            talker->dialogue_index++;
			this->continueDialogue();
			return;
        }

        //read selfdata [5]
		if(regex_match (talker->sayings.at(talker->dialogue_index + 1), regex("\\[[[:digit:]]+\\]"))) {
            int j = 1;
            //parse which block of memory we are interested in
            string s = talker->sayings.at(talker->dialogue_index + 1);
            s.erase(0, 1);
            string blockstr = s.substr(0, s.find(']'));
            int block = stoi(blockstr);
            string res = talker->sayings.at(talker->dialogue_index + 1 + j);
            while (res.find('*') != std::string::npos) {
                
                //parse option
                // *15 29 -> if data is 15, go to line 29
                string s = talker->sayings.at(talker->dialogue_index + 1 + j);
                s.erase(0, 1);
                int condition = stoi( s.substr(0, s.find(':')));
                s.erase(0, s.find(':') + 1);
                int jump = stoi(s);
                if(talker->data[block] == condition) {
                    talker->dialogue_index = jump - 3;
                    this->continueDialogue();
                    return;
                }
                j++;
                res = talker->sayings.at(talker->dialogue_index + 1 + j);

            }
            talker->dialogue_index++;
            this->continueDialogue();
			return;
		}

		//comment
		if(talker->sayings.at(talker->dialogue_index + 1).substr(0,2) =="//") {
			talker->dialogue_index++;
			this->continueDialogue();
			return;
		}

        //option/jump
		if(talker->sayings.at(talker->dialogue_index + 1).at(0) == '*') {
            string s = talker->sayings.at(talker->dialogue_index + 1);
            s.erase(0, 1);
            string condition = s.substr(0, s.find(':')); s.erase(0, s.find(':') + 1);
            int jump = stoi(s);
            if(response == condition) {
                response = "";
                response_index = 0;
                playSound(-1, confirm_noise, 0);
                talker->dialogue_index = jump - 3;
                D(talker->dialogue_index);
                D(talker->sayings.at(talker->dialogue_index+1));
                this->continueDialogue();
            } else {
                talker->dialogue_index++;
                this->continueDialogue();
            }

			
			return;
		}

		//change map
		if(talker->sayings.at(talker->dialogue_index + 1).at(0) == '@') {
			string s = talker->sayings.at(talker->dialogue_index + 1);
			//erase '@'
			s.erase(0, 1);

			string name = s.substr(0, s.find(' ')); s.erase(0, s.find(' ') + 1);
			// int xpos = stoi(s.substr(0, s.find(' '))); s.erase(0, s.find(' ') + 1);
			// int ypos = stoi(s.substr(0, s.find(' '))); s.erase(0, s.find(' ') + 1);
            const string dest_waypoint = s.substr(0, s.find(' ')); s.erase(0, s.find(' ') + 1);

			//close dialogue
			
			adventureUIManager->hideTalkingUI();
			//reset character's dialogue_index
			talker->dialogue_index = 0;
			//stop talker from bouncing
			talker->animate = 0;
            
            clear_map(g_camera);
			load_map(renderer, "maps/" + name  + "/" + name + ".map", dest_waypoint);

            //clear_map() will also delete engine tiles, so let's re-load them (but only if the user is map-editing)
            if(devMode) { init_map_writing(renderer);}
            protag_is_talking = 0;
            protag_can_move = 1;
			return;
		}

		//spawn entity
		if(talker->sayings.at(talker->dialogue_index + 1).at(0) == '&') {
			string s = talker->sayings.at(talker->dialogue_index + 1);
			//erase '&'
			s.erase(0, 1);
			int xpos, ypos;
			string name = s.substr(0, s.find(' ')); s.erase(0, s.find(' ') + 1);
			xpos = stoi(s.substr(0, s.find(' '))); s.erase(0, s.find(' ') + 1);
			ypos = stoi(s.substr(0, s.find(' '))); s.erase(0, s.find(' ') + 1);
			entity* e = new entity(renderer, name.c_str());
            e->x = xpos;
            e->y = ypos;

			talker->dialogue_index++;
			this->continueDialogue();
			return;
		}

		//destroy entity
		if(talker->sayings.at(talker->dialogue_index + 1).at(0) == '^') {
			string s = talker->sayings.at(talker->dialogue_index + 1);
			//erase '&'
			s.erase(0, 1);
			//s is the name of the entity to destroy
			
			for (long long unsigned int i = 0; i < g_entities.size(); i++) {
				if(g_entities[i]->inParty) { continue; }
				SDL_Rect b = {g_entities[i]->x, g_entities[i]->y - g_entities[i]->height, g_entities[i]->width, g_entities[i]->height};
				
				if(g_entities[i]->name == s) {
					delete g_entities[i];
					break;
				}
            
        	}

			talker->dialogue_index++;
			this->continueDialogue();
			return;
		}

        //change cameratarget
        if(talker->sayings.at(talker->dialogue_index + 1).at(0) == '`') {
            string s = talker->sayings.at(talker->dialogue_index + 1);
			s.erase(0, 1);
			string name = s.substr(0, s.find(' ')); s.erase(0, s.find(' ') + 1);
            string transtr = "0";
            transtr = s.substr(0, s.find(' ')); s.erase(0, s.find(' ') + 1);
            float transitionspeed = stof(transtr);
            
            entity* hopeful = searchEntities(name);
			if(hopeful != nullptr) {
                g_focus = hopeful;
                if(transitionspeed != 0) {
                    g_camera.lag = transitionspeed;
                    g_camera.lagaccel = transitionspeed;
                } else {
                    g_camera.lag = 0;
                    g_camera.lagaccel = g_camera.DEFAULTLAGACCEL;
                }
            }
            
			talker->dialogue_index++;
			this->continueDialogue();
			return;
		}

        //sleep
        if(talker->sayings.at(talker->dialogue_index + 1).at(0) == ';') {
            string s = talker->sayings.at(talker->dialogue_index + 1);
			s.erase(0, 1);
            string msstr = "0";
            msstr = s.substr(0, s.find(' ')); s.erase(0, s.find(' ') + 1);
            int ms = 0;
            ms = stoi(msstr);
            sleepingMS = ms;
            talker->dialogue_index++;
            sleepflag = 1;
            this->continueDialogue();
            return;
        }

        //load savefile
        if(talker->sayings.at(talker->dialogue_index + 1).substr(0,5) == "/load") {
            string s = talker->sayings.at(talker->dialogue_index + 1);
			s.erase(0, 6);
            loadSave(s);

            talker->dialogue_index++;
            this->continueDialogue();
            return;
        }

        //write savefile
        if(talker->sayings.at(talker->dialogue_index + 1).substr(0,6) == "/write") {
            string s = talker->sayings.at(talker->dialogue_index + 1);
			s.erase(0, 7);
            writeSave(s);

            talker->dialogue_index++;
            this->continueDialogue();
            return;
        }

        //check savefield
        if(regex_match (talker->sayings.at(talker->dialogue_index + 1), regex("\\{([a-zA-Z0-9_]){1,}\\}"))) {
            M("Tried to check a save field");
            //
            string s = talker->sayings.at(talker->dialogue_index + 1);
			s.erase(0, 1);
            s.erase(s.length() - 1, 1);
            
            int value = checkSaveField(s);

            int j = 1;
            string res = talker->sayings.at(talker->dialogue_index + 1 + j);
            while (res.find('*') != std::string::npos) {
                string s = talker->sayings.at(talker->dialogue_index + 1 + j);
                s.erase(0, 1);
                int condition = stoi( s.substr(0, s.find(':')));
                s.erase(0, s.find(':') + 1);
                int jump = stoi(s);
                if(value == condition) {
                    talker->dialogue_index = jump - 3;
                    this->continueDialogue();
                    return;
                }
                j++;
                res = talker->sayings.at(talker->dialogue_index + 1 + j);

            }

            talker->dialogue_index++;
            this->continueDialogue();
			return;
		}

        //write to savefield
        if(regex_match (talker->sayings.at(talker->dialogue_index + 1), regex("[[:digit:]]+\\-\\>\\{([a-zA-Z0-9_]){1,}\\}"))) {
            M("tried to write to savedata");
            string s = talker->sayings.at(talker->dialogue_index + 1);
            s.erase(s.length() - 1, 1);

            string valuestr = s.substr(0, s.find('-'));
            int value = stoi(valuestr);

            string field = s.substr(s.find('{')+1, s.length() - 1);
            D(value);
            D(field);
            writeSaveField(field, value);
            talker->dialogue_index++;
            this->continueDialogue();
            return;
        }

        //unconditional jump
        if(talker->sayings.at(talker->dialogue_index + 1).at(0) == ':') {
            string s = talker->sayings.at(talker->dialogue_index + 1);
			s.erase(0, 1);
            string DIstr = "0";
            DIstr = s.substr(0, s.find(' ')); s.erase(0, s.find(' ') + 1);
            int DI = 0;
            DI = stoi(DIstr);
            talker->dialogue_index = DI - 3;
            this->continueDialogue();
            return;
        }

        //solidify entity
        if(talker->sayings.at(talker->dialogue_index + 1).substr(0,9) == "/solidify") {
            string s = talker->sayings.at(talker->dialogue_index + 1);
			s.erase(0, 10);
            
            entity* solidifyMe = 0;
            solidifyMe = searchEntities(s);
            if(solidifyMe != 0) {
                solidifyMe->solidify();
            }

            talker->dialogue_index++;
            this->continueDialogue();
            return;
        }

        //unsolidify entity
        if(talker->sayings.at(talker->dialogue_index + 1).substr(0,11) == "/unsolidify") {
            string s = talker->sayings.at(talker->dialogue_index + 1);
			s.erase(0, 12);
            entity* unsolidifyMe = 0;
            unsolidifyMe = searchEntities(s);
            if(unsolidifyMe != 0) {
                unsolidifyMe->unsolidify();
            }

            talker->dialogue_index++;
            this->continueDialogue();
            return;
        }

        //change animation data
        if(talker->sayings.at(talker->dialogue_index + 1).substr(0,5) == "/anim") {
            string s = talker->sayings.at(talker->dialogue_index + 1);
			s.erase(0, 6);
            vector<string> split = splitString(s, ' ');
            
            entity* ent = 0;
            ent = searchEntities(split[0]);
            if(ent != 0) {
                ent->animation = stoi(split[1]);
                ent->msPerFrame = stoi(split[2]);
                ent->frameInAnimation = stoi(split[3]);
                ent->loopAnimation = stoi(split[4]);
            }

            talker->dialogue_index++;
            this->continueDialogue();
            return;
        }
        
        //play sound
        if(talker->sayings.at(talker->dialogue_index + 1).substr(0,6) == "/sound") {
            string s = talker->sayings.at(talker->dialogue_index + 1);
			s.erase(0, 7);

            playSoundByName(s);

            talker->dialogue_index++;
            this->continueDialogue();
            return;
        }

        //hide textbox
        if(talker->sayings.at(talker->dialogue_index + 1).substr(0,7) == "/hideui") {
            //this->hideTalkingUI();
            
            talker->dialogue_index++;
            this->continueDialogue();
            return;
        }

		//default - keep talking
		talker->dialogue_index++;
		pushText(talker);	
	}
};

