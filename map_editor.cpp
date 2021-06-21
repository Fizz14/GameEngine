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
bool makingtile, makingcollision, makingdoor; //states
int lx, ly, rx, ry; //for map editing corners;
float grid = 64; //default is 64
int width, height; //for selection display
int selectIndex = 0; //select entity
bool tiling = 1; //make current texture tile
bool drawhitboxes = 0; // devMode; //visualize hitboxes in map editor with command
int debug_r =255, debug_g =255, debug_b= 50; //for draw color
int shine = 1;
bool occlusion = 1;
int wallheight = 128;
bool showMarker = 1;
int lastAction = 0; //for saving the user's last action to be easily repeated
int navMeshDensity = 2; //navnodes every two blocks
int limits[4] = {0};

string captex = "tiles/diffuse/granit.png"; 
string walltex = "tiles/diffuse/bricks.png"; 
string floortex = "tiles/diffuse/concrete.png"; 
string masktex = "&"; 

entity* selectPointer;
int layer = 0;
//these three variables are used in creating walls. By default, the v-key will only make rectangular collisions
bool autoMakeWalls = 1; 
bool makeCollisions = 1; 
bool autoMakeWallcaps = 1; 

tile* selection; 
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

bool fileExists (const std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);	
        return true;
    } else {
        return false;
    }   
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
        if(word == "collision") {
            //M("loading collisisons" << endl;
            iss >> s0 >> p1 >> p2 >> p3 >> p4 >> p5; 
            collision* c = new collision(p1, p2, p3, p4, p5);
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
        if(word == "ai") {
            //M("loading ai" << endl;
            iss >> s0 >> s1 >> p0 >> p1;
            const char* plik = s1.c_str();
            ai* e = new ai(renderer, plik);
            
            e->x = p0;
            e->y = p1;
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
            iss >> s0 >> p1 >> p2 >>p3 >> p4;
            tri* t = new tri(p1, p2, p3, p4);
            g_triangles.push_back(t);
        }
        if(word == "mapObject") {
            iss >> s0 >> s1 >> p1 >> p2 >> p3 >> p4 >> p5 >> p6 >> p7;
            //entity* e = new entity(renderer, s1, p1, p2, p3, p4, p6, p7, p8, p9);
            mapObject* e = new mapObject(renderer, s1, p1, p2, p3, p4, p5, p6, p7);
        }
        if(word == "door") {
            iss >> s0 >> s1 >> s2 >> p1 >> p2 >>p3 >> p4;
            const char* map = s1.c_str();
            door* d = new door(renderer, map, s2, p1, p2, p3, p4);
        }
        if(word == "trigger") {
            iss >> s0 >> s1 >> p1 >> p2 >>p3 >> p4;
            const char* binding = s1.c_str();
            trigger* t = new trigger(binding, p1, p2, p3, p4);
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
            iss >> s0 >> s1 >> p0 >> p1 >> p2 >> p3;
            const char* plik = s1.c_str();
            ui* u = new ui(renderer, plik, p0, p1, p2, p3);
        }

        if(word == "heightmap") {
            iss >> s0 >> s1 >> s2;
            heightmap* h = new heightmap(s2.c_str(), s1.c_str());
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
    Update_NavNode_Costs(g_navNodes);
    infile.close();

    double LoadingTook = ( std::clock() - debugClock ) / (double) CLOCKS_PER_SEC;
    cout << "Loading took " << LoadingTook << "s" << endl;

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

} 


//called on init if map_editing is true
void init_map_writing(SDL_Renderer* renderer) {
    selection = new tile(renderer, "tiles/engine/marker.png", "&", 0, 0, 0, 0, 1, 1, 1, 0, 0);
    marker = new tile(renderer, "tiles/engine/marker.png", "&", 0, 0, 0, 0, 1, 1, 1, 0, 0);
    worldsoundIcon = new tile(renderer, "tiles/engine/speaker.png", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
    listenerIcon = new tile(renderer, "tiles/engine/listener.png", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
    navNodeIcon = new tile(renderer, "tiles/engine/walker.png", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
    musicIcon = new tile(renderer, "tiles/engine/music.png", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
    cueIcon = new tile(renderer, "tiles/engine/cue.png", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
    waypointIcon = new tile(renderer, "tiles/engine/waypoint.png", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
    doorIcon = new tile(renderer, "tiles/engine/door.png", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
    triggerIcon = new tile(renderer, "tiles/engine/trigger.png", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
    marker->z = 2; //infront of selections
    marker->wraptexture=0;
}

//called every frame if map_editing is true
//each bool represents a button, if c is true, c was pressed that frame
void write_map(entity* mapent) {
    float temp = mapent->x - 64;
    float px = round(temp/grid)*grid;
    temp = mapent->y -mapent->height - 38;
    //flawed now that the grid has changed to 64,38
    float py = round(temp/(float)round(grid* XtoY))*(float)round(grid* XtoY);
    
  
    // needed for proper grid in negative x or y
    if(mapent->x < 0)
        px-=grid;

    if(mapent->y < mapent->height)
        py-=round(grid * XtoY);


    //set marker
    marker->x = px;
    marker->y = py;
    marker->width = grid;
    marker->height = round(grid * XtoY);
    //some debug drawing
    SDL_Rect drect;
    if(drawhitboxes) {
        for(int i = 0; i < g_entities.size(); i++) {
            if(g_entities[i]->wallcap == 1) {continue; }
            drect.x = (g_entities[i]->bounds.x + g_entities[i]->x -g_camera.x)*g_camera.zoom;
            drect.y = (g_entities[i]->bounds.y + g_entities[i]->y -g_camera.y)*g_camera.zoom;
            drect.w = g_entities[i]->bounds.width * g_camera.zoom;
            drect.h = g_entities[i]->bounds.height* g_camera.zoom;

            SDL_SetRenderDrawColor(renderer, 80, 150, 0, 255);
            SDL_RenderDrawRect(renderer, &drect);
            
        }
        int layer = 0;
        for (auto n: g_collisions) {
            for (long long unsigned int i = 0; i < n.size(); i++) {
                drect.x = (n[i]->bounds.x -g_camera.x)*g_camera.zoom;
                drect.y = (n[i]->bounds.y - (38 * layer) -g_camera.y)*g_camera.zoom;
                drect.w = n[i]->bounds.width * g_camera.zoom;
                drect.h = n[i]->bounds.height* g_camera.zoom;

                SDL_SetRenderDrawColor(renderer, 150 - layer * 38, 50, layer * 38, 255);
                SDL_RenderDrawRect(renderer, &drect);
            }
            layer++;
        }
        for (long long unsigned int i = 0; i < g_triangles.size(); i++) {
            SDL_SetRenderDrawColor(renderer, 150, 50, 0, 255);
            g_triangles[i]->render(renderer);
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
                
                SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
                
            }
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        }
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    //draw rectangle to visualize the selection
    if(makingcollision || makingtile || makingdoor) {
        //SDL_Rect dstrect = {rx, ry, abs(px-rx) + grid, abs(py-ry) + grid};
        
        if(lx < px) {selection->x = lx; } else {selection->x = px;}
        if(ly < py) {selection->y = ly; } else {selection->y = py;}
        
        selection->width = abs(lx-px) + grid;
        selection->height = abs(ly-py) + (round(grid * XtoY));
        if(tiling && !makingcollision && !makingdoor) {
            selection->xoffset = (int)selection->x % int(selection->texwidth);
            selection->yoffset = (int)selection->y % int(selection->texheight);
            selection->wraptexture = 1;
        } else {
            //so that the red boundaries of making a collision appear properly
            selection->xoffset = 0;
            selection->yoffset = 0;
        }
        

    } else {
        //hide it;
        selection->width = 0;
        selection->height = 0;
    }


    if(devinput[0] && !olddevinput[0] && !makingcollision) {
        lx = px;
        ly = py;
        makingcollision = 1;
        selection->image = IMG_Load("tiles/engine/trigger.png");
        selection->texture = SDL_CreateTextureFromSurface(renderer, selection->image);
        SDL_FreeSurface(selection->image);
        selection->wraptexture = 0;
        
    } else {
        if(devinput[0] && !olddevinput[0] && makingcollision) {
            makingcollision = 0;
            cout << g_triggers.size() << endl;
            trigger* t = new trigger("unset", selection->x, selection->y, selection->width, selection->height);
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
                string heightmap_fileaddress = "tiles/heightmaps/h-" + floortex.substr(14);
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
                        heightmap* e = new heightmap(floortex, heightmap_fileaddress);
                    }
                }
                M(g_heightmaps.size());
                
            }
        }
    }
    if(devinput[3] && !olddevinput[3] && makingcollision == 0) {
        lx = px;
        ly = py;
        makingcollision = 1;
        selection->image = IMG_Load("tiles/engine/wall.png");
        selection->texture = SDL_CreateTextureFromSurface(renderer, selection->image);
        SDL_FreeSurface(selection->image);
        
    } else {
        if(devinput[3] && !olddevinput[3] && makingcollision == 1) {
            if(makingcollision) {

                //makes a tile and also a hitbox
                
                //if(makeCollisions) {
                //    ofile << "collision " << to_string(selection->x) << " " << to_string(selection->y) << " " << to_string(selection->width) << " " << to_string(selection->height) << endl;
                //}
                //make "cap" or the top of the wall
                
                makingcollision = 0;


                //spawn related objects
                //string loadstr = "tiles/wall.png";
                if(makeCollisions) {
                    for (int i = 0; i < wallheight / 64; i++) {
                        collision* c = new collision(selection->x, selection->y, selection->width, selection->height, i);
                    }
                    D(wallheight);
                }
                const char* plik = walltex.c_str();
                if(autoMakeWallcaps) {
                    int step = 5;
                    for (int i = 0; i < selection->height; i+=step) {
                        mapObject* e = new mapObject(renderer, captex, selection->x, selection->y + i + step, wallheight, selection->width, step, 0);
                    }
                    
                }

                if(autoMakeWalls) {
                    int step = 64;
                    for (int i = 0; i < wallheight; i+=step) {
                        mapObject* e = new mapObject(renderer, walltex, selection->x, selection->y + selection->height, i, selection->width, 55, 1);
                    }
                    ////entity* e = new entity(renderer, selection->x, selection->y + selection->height, selection->width, wallheight, walltex, 1);
                    //entity* e = new entity(renderer, walltex, selection->x, selection->y + selection->height, 0, selection->width, (wallheight) * XtoZ + 2, 1);
                }

                if(shine == 1) {
                    //front
                    mapObject* f = new mapObject(renderer, "tiles/lighting/SMOOTHSHADING.png", selection->x, selection->y + selection->height + marker->height/2,  wallheight + 1, selection->width, marker->height);
                    
                    

                    //back
                    f = new mapObject(renderer, "tiles/lighting/SMOOTHSHADING.png", selection->x, selection->y + marker->height/2, wallheight + 1, selection->width, marker->height/2);
                    
                }

                if(shine == 2) {
                    mapObject* f = new mapObject(renderer, "tiles/lighting/SHARPSHADING.png", selection->x, selection->y + selection->height - wallheight + marker->height/2, 0, selection->width, marker->height);

                }

                if(shine == 3) {
                    mapObject* f = new mapObject(renderer, "tiles/lighting/SHARPBRIGHTSHADING.png", selection->x, selection->y + selection->height - wallheight + marker->height/2, 0, selection->width, marker->height);
                }

                if(occlusion) {
                    //front shading
                    mapObject* m = new mapObject(renderer, "tiles/lighting/OCCLUSION.png", selection->x, selection->y + selection->height + 19, 0, selection->width, marker->height);

                    //sides
                    int step = 5;
                    for (int i = 0; i < selection->height; i+=step) {
                        mapObject* u = new mapObject(renderer, "tiles/lighting/h-OCCLUSION.png", selection->x - 19, selection->y + i + 5, 0, selection->width + 45, step);
        
                    }
                    D(selection->height);
                    
                    
                    
                    //corner a
                    mapObject* a = new mapObject(renderer, "tiles/lighting/x-OCCLUSION.png",selection->x - (38 - 19), selection->y, 0, 32, 19, 0, 0, -20);
                    M("make corner");
                    //corner b
                    mapObject* b = new mapObject(renderer, "tiles/lighting/x-OCCLUSION.png",selection->x + selection->width, selection->y, 0, 32, 19, 0, 0, -20);

                    //corner c
                    mapObject* c = new mapObject(renderer, "tiles/lighting/x-OCCLUSION.png",selection->x - (38 - 19), selection->y + selection->height + (38 - 19), 0, 19, 19, 0, 0, -20);

                    //corner d
                    mapObject* d = new mapObject(renderer, "tiles/lighting/x-OCCLUSION.png",selection->x + selection->width, selection->y + selection->height + (38 - 19), 0, 19, 19, 0, 0, -20);
                }

                
            }
        }
    }

    if(devinput[20] && !olddevinput[20] && makingcollision == 0) {
        lx = px;
        ly = py;
        makingcollision = 1;
        selection->image = IMG_Load("tiles/engine/navmesh.png");
        selection->texture = SDL_CreateTextureFromSurface(renderer, selection->image);
        SDL_FreeSurface(selection->image);
        
    } else {
        if(devinput[20] && !olddevinput[20] && makingcollision == 1) {
            if(makingcollision) {
                for (int i = selection->x; i < selection->width + selection->x; i+= 64 * navMeshDensity) {
                    for (int j = selection->y; j < selection->height + selection->y; j+= 38 * navMeshDensity) {
                        D(j);
                        D(marker->height * navMeshDensity);
                        navNode* n = new navNode(i + marker->width/2, j + marker->height/2);
                    }
                }


                for (int i = 0; i < g_navNodes.size(); i++) {
                    for (int j = 0; j < g_collisions[0].size(); j++) {
                        rect node = rect(g_navNodes[i]->x, g_navNodes[i]->y, 30, 20);
                        if(RectOverlap(node, g_collisions[0][j]->bounds)) {
                            // D(g_collisions[j]->bounds.x);
                            // D(g_collisions[j]->bounds.y);
                            // D(g_collisions[j]->bounds.height);
                            // D(g_collisions[j]->bounds.width);
                            // D(node.x);
                            // D(node.y);
                            // D(node.height);
                            // D(node.width);
                            
                            delete g_navNodes[i];
                            i--;
                            break;
                        }
                    }
                }

                for (int i = 0; i < g_navNodes.size(); i++) {
                    for (int j = 0; j < g_navNodes.size(); j++) {
                        if(i == j) {continue;}
                        if(Distance(g_navNodes[i]->x, g_navNodes[i]->y, g_navNodes[j]->x, g_navNodes[j]->y) < 300 && LineTrace(g_navNodes[i]->x, g_navNodes[i]->y, g_navNodes[j]->x, g_navNodes[j]->y, 38)) {
                            g_navNodes[i]->Add_Friend(g_navNodes[j]);
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
                makingcollision = 0;
            }
        }
    }

    if(devinput[4] && !olddevinput[4] ) {
        //clear corners
        makingcollision =0;
        makingtile = 0;
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
        if(grid/2 > 0) {
            grid /= 2;
            M("Grid lowered to " + (int)grid);
        }
    }

    if(devinput[8] && !olddevinput[8]) {
        //grow grid
        grid *= 2;
        M("Grid raised to " + (int)grid);
    }

    if(devinput[9] && !olddevinput[9]) {
        
        collisionsenabled=!collisionsenabled;
        if(collisionsenabled) {
            M("collisions ON");
        } else {
            M("collisions OFF");
        }
    }

    if(devinput[10] && !olddevinput[10]) {
        
    }

    if(devinput[11] && !olddevinput[11]) { 
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
            if(word == "save") {
                if(line >> word) { 
                    //add warning for file overright
                    if(fileExists("maps/" + word + "/" + word + ".map")) {
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
                     

                    for (long long unsigned int i = 0; i < g_triangles.size(); i++) {
                        ofile << "triangle " << g_triangles[i]->x1 << " " << g_triangles[i]->y1 << " " << g_triangles[i]->x2 << " " << g_triangles[i]->y2 << endl;
                    }
                    for (long long unsigned int i = 0; i < g_entities.size(); i++) {
                        if(!g_entities[i]->inParty) {
                            if(dynamic_cast<ai*>(g_entities[i]) == nullptr) { //dont save monsters as entities
                                ofile << "entity " << g_entities[i]->name << " " << to_string(g_entities[i]->x) << " " << to_string(g_entities[i]->y) <<  " " << to_string(g_entities[i]->z) << endl;
                            }
                        }
                    }
                    
                    for (long long unsigned int i = 0; i < g_mapObjects.size(); i++) {    
                        ofile << "mapObject " << g_mapObjects[i]->name << " " << to_string(g_mapObjects[i]->x) << " " << to_string(g_mapObjects[i]->y) << " " << to_string(g_mapObjects[i]->z) << " " << to_string(g_mapObjects[i]->width) << " " << to_string(g_mapObjects[i]->height) <<  " " << g_mapObjects[i]->wall << " " << g_mapObjects[i]->extraYOffset << " "  << g_mapObjects[i]->sortingOffset << endl;
                    }

                    for (long long unsigned int i = 0; i < g_tiles.size(); i++) {
                        if(g_tiles[i]->fileaddress == "tiles/engine/marker.png" ) { continue; } //dont save map graphics
                        if(g_tiles[i]->fileaddress == "tiles/engine/speaker.png" ) { continue; }
                        if(g_tiles[i]->fileaddress == "tiles/engine/walker.png" ) { continue; }
                        if(g_tiles[i]->fileaddress == "tiles/engine/music.png" ) { continue; }
                        if(g_tiles[i]->fileaddress == "tiles/engine/cue.png" ) { continue; }
                        if(g_tiles[i]->fileaddress == "tiles/engine/waypoint.png" ) { continue; }
                        if(g_tiles[i]->fileaddress == "tiles/engine/trigger.png" ) { continue; }
                        if(g_tiles[i]->fileaddress == "tiles/engine/door.png" ) { continue; }
                        if(g_tiles[i]->fileaddress == "tiles/engine/listener.png" ) { continue; }
                        
                        ofile << "tile " << g_tiles[i]->fileaddress << " " << g_tiles[i]->mask_fileaddress << " " << to_string(g_tiles[i]->x) << " " << to_string(g_tiles[i]->y) << " " << to_string(g_tiles[i]->width) << " " << to_string(g_tiles[i]->height) << " " << g_tiles[i]->z << " " << g_tiles[i]->wraptexture << " " << g_tiles[i]->wall << " " << g_tiles[i]->dxoffset << " " << g_tiles[i]->dyoffset << endl;
                    }
                    for (long long unsigned int i = 0; i < g_heightmaps.size(); i++) {
                        
                        ofile << "heightmap " << g_heightmaps[i]->binding << " " << g_heightmaps[i]->name << endl;
                    }
                    for (long long unsigned int j = 0; j < g_collisions.size(); j++){
                        for (long long unsigned int i = 0; i < g_collisions[j].size(); i++){
                            ofile << "collision " << to_string(g_collisions[j][i]->bounds.x) << " " << to_string(g_collisions[j][i]->bounds.y) << " " << to_string(g_collisions[j][i]->bounds.width) << " " << to_string(g_collisions[j][i]->bounds.height) << " " << j << endl;
                        }
                    }
                    for (long long unsigned int i = 0; i < g_doors.size(); i++){
                        ofile << "door " << g_doors[i]->to_map << " " << g_doors[i]->to_point << " " << g_doors[i]->x << " " << g_doors[i]->y << " " << g_doors[i]->width << " " << g_doors[i]->height << endl;
                    }
                    for (long long unsigned int i = 0; i < g_triggers.size(); i++){
                        ofile << "trigger " << g_triggers[i]->binding << " " << g_triggers[i]->x << " " << g_triggers[i]->y << " " << g_triggers[i]->width << " " << g_triggers[i]->height << endl;
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
                    load_map(renderer, word.c_str(), "a");
                    init_map_writing(renderer);
                    break;
                }
            }
            if(word == "clear") {
                line >> word;
                if(word == "map") {
                    clear_map(g_camera);
                    load_map(renderer, "empty.map", "a");
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
                if(word == "leftbound") {
                    limits[0] = marker->x;
                    limits[1] = marker->y;
                    break;
                }
                if(word == "rightbound") {
                    limits[2] = marker->x + marker->width;
                    limits[3] = marker->y + marker->height;
                    break;
                }
            }
            if(word == "set" || word == "s") {
                line >> word;
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
                if(word == "occlusion" || word == "o") {
                    line >> occlusion;
                    break;
                }
                if(word == "wall" || word == "w") {
                    line >> walltex;
                    walltex = "tiles/diffuse/" + walltex + ".png";
                    break;
                }
                if(word == "floor" || word == "f") {
                    line >> floortex;
                    floortex = "tiles/diffuse/" + floortex + ".png";
                    break;
                }
                if(word == "cap" || word == "c") {
                    line >> captex;
                    captex = "tiles/diffuse/" + captex + ".png";
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
                        makeCollisions = input;
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
                if(word == "collisions") {
                    bool num;
                    if(line >> num) {
                        collisionsenabled = num;
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
                if(word == "integerscaling") {
                    bool num;
                    if(line >> num) {
                        integerscaling = num;
                    }
                    break;
                }
            }
            
            if(word == "adj" || word == "adjust") {
                //adjust
                line >> word;
                if(word == "boundx") {
                    g_entities[g_entities.size() - 1]->bounds.x = (g_entities[g_entities.size() - 1]->width/2 - g_entities[g_entities.size() - 1]->shadow->width/2);
                    int number;
                    line >> number;
                    g_entities[g_entities.size() - 1]->bounds.x += number;
                    break;
                }
                if(word == "boundy") {
                    g_entities[g_entities.size() - 1]->bounds.y = -1 * (g_entities[g_entities.size() - 1]->height - g_entities[g_entities.size() - 1]->shadow->height);
                    int number;
                    line >> number;
                    g_entities[g_entities.size() - 1]->bounds.y += number;
                    break;
                }
                if(word == "boundw") {
                    line >> g_entities[g_entities.size() - 1]->bounds.width;
                    break;
                }
                if(word == "boundh") {
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
                    for(auto x: limits) {x = 0;}
                }
            }
            if(word == "teleport" || word == "tp") {
                int x, y;
                line >> x >> y;
                mapent->x = x;
                mapent->y = y;
                break;
            }
            if(word == "entity" || word == "ent") {
                line >> entstring;
                float z = 0;
                line >> z;
                //actually spawn the entity in the world
                //string loadstr = "entities/" + entstring + ".ent";
                const char* plik = entstring.c_str();
                //entstring = loadstr;
                entity* e = new entity(renderer,  plik);
                e->x = px;
                e->y = py;
                e->xmaxspeed =0;
                e->ymaxspeed =0;
                e->stop_hori();
                e->stop_verti();
                e->z = mapent->z;
                e->shadow->x = e->x + e->shadow->xoffset;
		        e->shadow->y = e->y + e->shadow->yoffset;
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
                line >> entstring;
                float radius;
                line >> radius;
                cueSound* m = new cueSound(entstring, px + marker->width/2, py + marker->height/2, radius);
            }
            if(word == "way" || word == "w") {
                line >> entstring;
                waypoint* m = new waypoint(entstring, px + marker->width/2, py + marker->height/2);
            }
            if(word == "door" || word == "d") { //consider renaming this "link" or something other than "door" because it doesnt make doors
                string mapdest, waydest;
                line >> mapdest >> waydest;
                if(g_doors.size() > 0) {
                    g_doors[g_doors.size() - 1]->to_map = mapdest;
                    g_doors[g_doors.size() - 1]->to_point = waydest;
                    
                }
            }
            if(word == "trigger" || word == "t") {
                string fbinding;
                line >> fbinding;
                if(g_triggers.size() > 0) {
                    g_triggers[g_triggers.size() - 1]->binding = fbinding;
                    
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
            if(word == "chaser") {
                line >> entstring;
                chaser* amon = new chaser(renderer, entstring.c_str());
                amon->x = px;
                amon->y = py;
            }
            if(word == "navnode") {
                navNode* n = new navNode(px + marker->width/2, py + marker->height/2);
            }
            
            if(word == "navlink") {
                //delete nodes inside of walls
                for (int i = 0; i < g_navNodes.size(); i++) {
                    for (int j = 0; j < g_collisions.size(); j++) {
                        rect node = rect(g_navNodes[i]->x - 5, g_navNodes[i]->y - 5, g_navNodes[i]->x + 10, g_navNodes[i]->y + 10);
                        if(RectOverlap(node, g_collisions[0][j]->bounds)) {
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
    
    if(devinput[12] && !olddevinput[12]) {
        
        //wall's corner is in top left
        //make the gapcloser
        entity* e = new entity(renderer, captex, marker->x, marker->y- wallheight + 38, marker->width, marker->height, 0);
        e->sortingOffset = wallheight - 38;
        

        //make chip
        size_t pos = walltex.find("/");
        string awalltex = walltex.substr (pos + 1);
        pos = awalltex.find("/");
        awalltex = awalltex.substr (pos + 1); 
        awalltex = "tiles/sheared/a-" + awalltex;


        float extraoffset = ((int)marker->y + wallheight) % 90;
        float currentOffset = -22;
        for (int i = 0; i < wallheight; i+=38) {
            float extraoffset = ((int)marker->y + 38) % 90;
            D(awalltex);
            entity* f = new entity(renderer, awalltex, marker->x, marker->y + marker->height , marker->width, 2 * marker->height, 1, extraoffset);
            f->sortingOffset = currentOffset;
            SDL_SetTextureBlendMode(f->texture, SDL_BLENDMODE_BLEND);
            currentOffset += 38;
            marker->y-= 38;
        }
        marker->y += wallheight;
        
        
        
        if(shine == 1) {
            entity* g = new entity(renderer, "tiles/lighting/a-SMOOTHSHADING.png", marker->x, marker->y + marker->height - 22 - wallheight + 38, marker->width, 2 * marker->height, 1, extraoffset + 22);
            g->sortingOffset = wallheight -5;
            
        }

        if(shine == 2) {
            entity* g = new entity(renderer, "tiles/lighting/a-SHARPSHADING.png", marker->x, marker->y + marker->height - 22 - wallheight + 38, marker->width, 2 * marker->height, 1, extraoffset + 22);
            g->sortingOffset = wallheight -5;
        }

        if(shine == 3) {
            entity* g = new entity(renderer, "tiles/lighting/a-SHARPBRIGHTSHADING.png", marker->x, marker->y + marker->height - 22 - wallheight + 38, marker->width, 2 * marker->height, 1, extraoffset + 22);
            g->sortingOffset = wallheight -5;
        }
        extraoffset = ((int)marker->y + 38) % 90;
        if(occlusion) {
            entity* h = new entity(renderer, "tiles/lighting/a-occlusion.png", marker->x, marker->y + marker->height + 23, marker->width, 2 * marker->height, 1, extraoffset - 23);
            h->sortingOffset = -43;
            //sides as well
        }

        //make triangle
        tri* n = new tri(marker->x + marker->width, marker->y, marker->x, marker->y + marker->height);
        g_triangles.push_back(n);

    }
    if(devinput[13] && !olddevinput[13]) {
        //wall's corner is in top right
        //make the gapcloser
        entity* e = new entity(renderer, captex, marker->x, marker->y- wallheight + 38, marker->width, marker->height, 0);
        e->sortingOffset = wallheight - 38;

        //make chip
        size_t pos = walltex.find("/");
        string awalltex = walltex.substr (pos + 1);
        pos = awalltex.find("/");
        awalltex = awalltex.substr (pos + 1); 
        awalltex = "tiles/sheared/b-" + awalltex;

        float extraoffset = ((int)marker->y + wallheight) % 90;
        float currentOffset = -22;
        
        for (int i = 0; i < wallheight; i+=38) {
            float extraoffset = ((int)marker->y + 38) % 90;
            
            entity* f = new entity(renderer, awalltex, marker->x, marker->y + marker->height , marker->width, 2 * marker->height, 1, extraoffset);
            f->sortingOffset = currentOffset;
            currentOffset += 38;
            marker->y-= 38;
        }

        marker->y += wallheight;

        if(shine == 1) {
            entity* g = new entity(renderer, "tiles/lighting/b-SMOOTHSHADING.png", marker->x, marker->y + marker->height - 22 - wallheight + 38, marker->width, 2 * marker->height, 1, extraoffset + 22);
            g->sortingOffset = wallheight -5;
        }

        if(shine == 2) {
            entity* g = new entity(renderer, "tiles/lighting/b-SHARPSHADING.png", marker->x, marker->y + marker->height - 22 - wallheight + 38, marker->width, 2 * marker->height, 1, extraoffset + 22);
            g->sortingOffset = wallheight -5;
        }

        if(shine == 3) {
            entity* g = new entity(renderer, "tiles/lighting/b-SHARPBRIGHTSHADING.png", marker->x, marker->y + marker->height - 22 - wallheight + 38, marker->width, 2 * marker->height, 1, extraoffset + 22);
            g->sortingOffset = wallheight -5;
        }

        extraoffset = ((int)marker->y + 38) % 90;

        if(occlusion) {
            entity* h = new entity(renderer, "tiles/lighting/b-occlusion.png", marker->x, marker->y + marker->height + 23, marker->width, 2 * marker->height, 1, extraoffset - 23);
            h->sortingOffset = -43;
        }

        //make triangle
        tri* n = new tri(marker->x, marker->y, marker->x+ marker->width, marker->y + marker->height);
        g_triangles.push_back(n);
    }
    if(devinput[14] && !olddevinput[14]) {
        //wall's corner is in bottom right

        //make wall
        entity* d = new entity(renderer, walltex, marker->x, marker->y + marker->height, marker->width, wallheight, 1);
        d->sortingOffset = -4;
        
        //make chip
        size_t pos = captex.find("/");
        string acaptex = captex.substr (pos + 1);
        pos = acaptex.find("/");
        acaptex = acaptex.substr (pos + 1); 
        acaptex = "tiles/sheared/c-" + acaptex;
        
        entity* e = new entity(renderer, acaptex, marker->x + 1, marker->y + 1 + marker->height - 15 - wallheight + 38, marker->width -1, marker->height * 2 -15, 0);
        e->sortingOffset = wallheight - 38;

        //hide the crappy black edges
        entity* f = new entity(renderer, captex, marker->x + marker->width - 1, marker->y - wallheight + 38, 1, marker->height, 0);
        f->sortingOffset = wallheight + marker->height;
        entity* g = new entity(renderer, captex,  marker->x, marker->y + 1 - wallheight + 38, marker->width, 2, 0, 0);
        g->sortingOffset = wallheight + 32;
 

        //make triangle
        tri* n = new tri(marker->x, marker->y + marker->height, marker->x + marker->width, marker->y);
        g_triangles.push_back(n);
    }
    if(devinput[15] && !olddevinput[15]) {

        //make wall
        entity* d = new entity(renderer, walltex, marker->x, marker->y + marker->height, marker->width, wallheight, 1);
        d->sortingOffset = -4;
        
        //make chip
        size_t pos = captex.find("/");
        string acaptex = captex.substr (pos + 1);
        pos = acaptex.find("/");
        acaptex = acaptex.substr (pos + 1); 
        acaptex = "tiles/sheared/d-" + acaptex;
        
        entity* e = new entity(renderer, acaptex, marker->x + 1, marker->y + 1 + marker->height - 15 - wallheight + 38, marker->width - 2, marker->height * 2 -15, 0);
        e->sortingOffset = wallheight - 38;


        //hide the crappy black edges
        entity* f = new entity(renderer, captex, marker->x, marker->y - wallheight+ 38, 1, marker->height, 0);
        f->sortingOffset = wallheight + marker->height;
        entity* g = new entity(renderer, captex, marker->x, marker->y + 1 - wallheight + 38, marker->width, 2, 0);
        g->sortingOffset = wallheight + 32;
 

        //make triangle
        tri* n = new tri(marker->x + marker->width, marker->y + marker->height, marker->x, marker->y);
        g_triangles.push_back(n);
        
    }

    
    if(devinput[16] && !olddevinput[16]) {
        //delete entities at cursor
        //Door -> Entity -> Tile
        bool foundFlag = 0;
        SDL_Rect a = {marker->x, marker->y, marker->width, marker->height};
        for (long long unsigned int i = 0; i < g_entities.size(); i++) {
            if(g_entities[i]->inParty) { continue; }
            SDL_Rect b = {g_entities[i]->x, g_entities[i]->y - g_entities[i]->height, g_entities[i]->width, g_entities[i]->height};
            
            if(RectOverlap(a, b)) {
                delete g_entities[i];
                foundFlag = 1;
                break;
            }
            
        }
        if(!foundFlag) {
            for (long long unsigned int i = 0; i < g_tiles.size(); i++) {
                
                if(g_tiles[i] == marker) { continue; }
                if(g_tiles[i] == selection) { continue; }
                SDL_Rect b = {g_tiles[i]->x, g_tiles[i]->y, g_tiles[i]->width, g_tiles[i]->height};
                
                if(RectOverlap(a, b)) {
                    auto* holder = g_tiles[i];
                    g_tiles.erase(g_tiles.begin() + i);
                    delete holder;
                }
                break;
            }
        }
    }

        //make zslant block left
    if(devinput[17] && !olddevinput[17]) {
        //wall's corner is in top right
        //make the gapcloser
        entity* e = new entity(renderer, walltex, marker->x, marker->y- wallheight + 90, marker->width, marker->height, 1);
        e->sortingOffset = wallheight - 90;
        //make chip
        size_t pos = captex.find("/");
        string awalltex = captex.substr (pos + 1); 
        awalltex = "tiles/sheared/b-" + awalltex;
        D(awalltex);
        float extraoffset = ((int)marker->y + wallheight) % 90;
        float currentOffset = -22;
        
        for (int i = 0; i < wallheight; i+=38) {
            float extraoffset = ((int)marker->y + 38) % 90;
            
            entity* f = new entity(renderer, awalltex,  marker->x, marker->y + marker->height , marker->width, 2 * marker->height, 0, extraoffset);
            f->sortingOffset = currentOffset;
            currentOffset += 38;
            marker->y-= 38;
        }

        marker->y += wallheight;

        if(shine == 1) {
            entity* g = new entity(renderer, "tiles/lighting/b-SMOOTHSHADING.png", marker->x, marker->y + marker->height - 22 - wallheight + 90, marker->width, 2 * marker->height, 1, extraoffset + 22);
            g->sortingOffset = wallheight -5 -38;
            //g->extraYOffset = 38;
            //set it to not be a wallcap so that it doesnt render in two parts
            g->wallcap = 0;
        }

        if(shine == 2) {
            entity* g = new entity(renderer, "tiles/lighting/b-SHARPSHADING.png", marker->x, marker->y + marker->height - 22 - wallheight + 38, marker->width, 2 * marker->height, 1, extraoffset + 22);
            g->sortingOffset = wallheight -5;
        }

        if(shine == 3) {
            entity* g = new entity(renderer, "tiles/lighting/b-SHARPBRIGHTSHADING.png", marker->x, marker->y + marker->height - 22 - wallheight + 38, marker->width, 2 * marker->height, 1, extraoffset + 22);
            g->sortingOffset = wallheight -5;
        }

        extraoffset = ((int)marker->y + 38) % 90;


        //fix occlusion
        if(occlusion) {
            //add lower occlusion
        }

        //collision* c = new collision(marker->x, marker->y, marker->width, marker->height);
    }
    
    //make zslant block right
    if(devinput[18] && !olddevinput[18]) {
            //wall's corner is in top right
        //make the gapcloser
        entity* e = new entity(renderer, walltex, marker->x, marker->y- wallheight + 90, marker->width, marker->height, 1);
        e->sortingOffset = wallheight - 90;
        //make chip
        size_t pos = captex.find("/");
        string awalltex = captex.substr (pos + 1); 
        awalltex = "tiles/sheared/a-" + awalltex;

        float extraoffset = ((int)marker->y + wallheight) % 90;
        float currentOffset = -22;
        
        for (int i = 0; i < wallheight; i+=38) {
            float extraoffset = ((int)marker->y + 38) % 90;
            
            entity* f = new entity(renderer, awalltex, marker->x, marker->y + marker->height , marker->width, 2 * marker->height, 0, extraoffset);
            f->sortingOffset = currentOffset;
            currentOffset += 38;
            marker->y-= 38;
        }

        marker->y += wallheight;

        if(shine == 1) {
            entity* g = new entity(renderer, "tiles/lighting/a-SMOOTHSHADING.png", marker->x, marker->y + marker->height - 22 - wallheight + 90, marker->width, 2 * marker->height, 1, extraoffset + 22);
            g->sortingOffset = wallheight -5 -38;
            //g->extraYOffset = 38;
            //set it to not be a wallcap so that it doesnt render in two parts
            g->wallcap = 0;
        }

        if(shine == 2) {
            entity* g = new entity(renderer, "tiles/lighting/a-SHARPSHADING.png", marker->x, marker->y + marker->height - 22 - wallheight + 38, marker->width, 2 * marker->height, 1, extraoffset + 22);
            g->sortingOffset = wallheight -5;
        }

        if(shine == 3) {
            entity* g = new entity(renderer, "tiles/lighting/a-SHARPBRIGHTSHADING.png", marker->x, marker->y + marker->height - 22 - wallheight + 38, marker->width, 2 * marker->height, 1, extraoffset + 22);
            g->sortingOffset = wallheight -5;
        }

        extraoffset = ((int)marker->y + 38) % 90;


        //fix occlusion
        if(occlusion) {
            //add lower occlusion
        }

        //collision* c = new collision(marker->x, marker->y, marker->width, marker->height);
    }
    if(devinput[19] && !olddevinput[19]) {
        navNode* n = new navNode(px + marker->width/2, py + marker->height/2);
    }
    
    

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


    //reference to protag;
    entity* protagref;

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
		talkingBox = new ui(renderer, "ui/menu9patchblack.png", 0, 0.65, 1, 0.35);
		// talkingBox->x = 0;
		// talkingBox->y = 0.65;
		// talkingBox->width = 1;
		// talkingBox->height = 0.35;
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
                // talker->dialogue_index++;
                // continueDialogue();
                adventureUIManager->hideTalkingUI();
                talker = NULL;
            }

		}
	}
	void continueDialogue() {
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
        //write selfdata
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

        //read selfdata
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
		if(talker->sayings.at(talker->dialogue_index + 1).at(0) == '/') {
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
			protag_is_talking = 2;
			adventureUIManager->hideTalkingUI();
			//reset character's dialogue_index
			talker->dialogue_index = 0;
			//stop talker from bouncing
			talker->animate = 0;
            
            clear_map(g_camera);
			load_map(renderer, "maps/" + name  + "/" + name + ".map", dest_waypoint);

            //clear_map() will also delete engine tiles, so let's re-load them (but only if the user is map-editing)
            if(devMode) { init_map_writing(renderer);}
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

		//default - keep talking
		talker->dialogue_index++;
		pushText(talker);	
	}
};

