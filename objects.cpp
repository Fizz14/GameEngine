#include <SDL2/SDL.h>        
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <math.h>
#include <fstream>
#include <stdio.h>
#include <string>
#include <limits>
#include <stdlib.h>

#include "globals.cpp"

#define PI 3.14159265

#ifndef CLASSES
#define CLASSES

using namespace std;



class heightmap {
public:
	SDL_Surface* image = 0;
	string name;
	string binding;
	float magnitude = 0.278;

	heightmap(string fname, string fbinding) {
		image = IMG_Load(fbinding.c_str());
		name = fname;
		binding = fbinding;
		g_heightmaps.push_back(this);
	}

	~heightmap() {
		SDL_FreeSurface(image);
		g_heightmaps.erase(remove(g_heightmaps.begin(), g_heightmaps.end(), this), g_heightmaps.end());
	}

	Uint32 getpixel(SDL_Surface *surface, int x, int y) {
		int bpp = surface->format->BytesPerPixel;
		/* Here p is the address to the pixel we want to retrieve */
		Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch (bpp)
	{
		case 1:
			return *p;
			break;

		case 2:
			return *(Uint16 *)p;
			break;

		case 3:
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
				return p[0] << 16 | p[1] << 8 | p[2];
			else
				return p[0] | p[1] << 8 | p[2] << 16;
				break;

			case 4:
				return *(Uint32 *)p;
				break;

			default:
				return 0;
		}
	}
};

class navNode {
public:
	int x;
	int y;
	vector<navNode*> friends;
	vector<float> costs;
	float costFromSource = 0; //updated with dijkstras algorithm
	navNode* prev = nullptr; //updated with dijkstras algorithm
	string name = "";

	navNode(int fx, int fy) {
		M("navNode()" );
		x = fx;
		y = fy;
		g_navNodes.push_back(this);
	}

	void Add_Friend(navNode* newFriend) {
		friends.push_back(newFriend);
		float cost = pow(pow((newFriend->x - this->x), 2) + pow((newFriend->y - this->y), 2), 0.5);
		costs.push_back(cost);
	}

	void Update_Costs() {
		for (int i = 0; i < friends.size(); i++) {
			costs[i] = Distance(x, y, friends[i]->x, friends[i]->y);
		}
		
	}

	void Render(int red, int green, int blue) {
		SDL_Rect obj = {(this->x -g_camera.x - 20)* g_camera.zoom , ((this->y - g_camera.y - 20) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
		SDL_SetTextureColorMod(nodeDebug, red, green, blue);
		SDL_RenderCopy(renderer, nodeDebug, NULL, &obj);
	}
	
	~navNode() {
		M("~navNode()" );
		g_navNodes.erase(remove(g_navNodes.begin(), g_navNodes.end(), this), g_navNodes.end());
	}
};

void Update_NavNode_Costs(vector<navNode*> fnodes) {
	M("Update_NavNode_Costs()" );
	for (int i = 0; i < fnodes.size(); i++) {
		fnodes[i]->Update_Costs();
	}
}


class coord {
public:
	int x;
	int y;
};

class rect {
public:
	int x;
	int y;
	int width;
	int height;

	rect() {
		x=0;
		y=0;
		width=45;
		height=45;
	}

	rect(int a, int b, int c, int d) {
		x=a;
		y=b;
		width=c;
		height=d;
	}

	void render(SDL_Renderer * renderer) {
		SDL_Rect rect = { this->x, this->y, this->width, this->height};
		SDL_RenderFillRect(renderer, &rect);
	}
};

class tri {
public:
	//if type is true, the right angle corner will be at the bottom.
	//y1 > y2
	int x1; int y1;
	int x2; int y2;
	int type;
	float m; //slope
	int b; //offset

	tri() {
		x1=120; y1=120;
		x2=60; y2 =60;
		if(x2 < x1 && y2 > y1) {
			type = 0; //  :'
		}
		if(x2 < x1 && y2 < y1) {
			type = 1; //  :,
		}
		if(x2 > x1 && y2 < y1) {
			type = 2; //  ,:
		}
		if(x2 > x1 && y2 > y1) {
			type = 3; //  ':
		}
		m = (y1 -y2) / (x1 - x2);
		b = y1 - (m * x1);
	}

	tri(int fx1, int fy1, int fx2, int fy2) {
		x1=fx1; y1=fy1;
		x2=fx2; y2=fy2;
		if(x2 < x1 && y2 > y1) {
			type = 0; //  :'
		}
		if(x2 < x1 && y2 < y1) {
			type = 1; //  :,
		}
		if(x2 > x1 && y2 < y1) {
			type = 2; //  ,:
		}
		if(x2 > x1 && y2 > y1) {
			type = 3; //  ':
		}
		m = float(y1 -y2) / float(x1 - x2);
		b = y1 - (m * x1);
	}

	~tri() {
		g_triangles.erase(remove(g_triangles.begin(), g_triangles.end(), this), g_triangles.end());
	}

	void render(SDL_Renderer* renderer) {
		
		int tx1 = g_camera.zoom * (x1-g_camera.x);
		int tx2 = g_camera.zoom * (x2-g_camera.x);
		

		int ty1 = g_camera.zoom * (y1-g_camera.y);
		int ty2 = g_camera.zoom * (y2-g_camera.y);
		
		
		SDL_RenderDrawLine(renderer,  tx1, ty1, tx2, ty2);
		SDL_RenderDrawLine(renderer,  tx1, ty1, tx2, ty1);
		SDL_RenderDrawLine(renderer,  tx2, ty2, tx2, ty1);

		
	}
};


bool PointInsideRightTriangle(tri* t, int px, int py, int skin) {
	switch(t->type) {

		
		case(0):
			if(px > skin+ t->x2 && py > skin+ t->y1 && py < (t->m * px) -skin + t->b) {
				
				
				return true;
			}
		break;

		case(1):
			if(px > skin+ t->x2 && py < t->y1-skin && py > skin + (t->m * px) + t->b) {
				
				
				return true;
			}
		break;
		case(2):
			if(px < t->x2-skin && py < t->y1-skin && py > skin + (t->m * px) + t->b) {
				
				
				return true;
			}
		break;
		case(3):
			if(px < t->x2-skin && py > skin+ t->y1 && py < (t->m * px)-skin  + t->b) {
				
				
				return true;
			}
		break;

	}
	return false;
}

bool RectOverlap(rect a, rect b) {
	if (a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height && a.y + a.height > b.y) {
    	return true;
	} else {
		return false;
	}
}

bool RectOverlap(SDL_Rect a, SDL_Rect b) {
	if (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y) {
    	return true;
	} else {
		return false;
	}
}

//is a inside b?
bool RectWithin(rect a, rect b) {
	if (b.x < a.x && b.x + b.width > a.x + a.width && b.y < a.y && b.y + b.height > a.y + a.height) {
    	return true;
	} else {
		return false;
	}
}

bool TriRectOverlap(tri* a, int x, int y, int width, int height, int skin) {
	if(PointInsideRightTriangle(a, x, y +  height, skin)) {
		return 1;
	}
	if(PointInsideRightTriangle(a, x + width, y + height, skin)) {
		return 1;
	}
	if(PointInsideRightTriangle(a, x + width, y, skin)) {
		return 1;
	}
	if(PointInsideRightTriangle(a, x, y, skin )) {
		return 1;
	}
	//also check if the points of the triangle are inside the rectangle
	//skin usage is possibly redundant here
	if(a->x1 > x + skin && a->x1 < x + width - skin && a->y1 > y + skin && a->y1 < y + height - skin) {
		return 1;
	}
	if(a->x2 > x + skin && a->x2 < x + width - skin && a->y2 > y + skin&& a->y2 < y + height - skin) {
		return 1;
	}
	if(a->x2 > x + skin && a->x2 < x + width - skin && a->y1 > y + skin && a->y1 < y + height - skin) {
		return 1;
	}
	return 0;
}

SDL_Rect transformRect(SDL_Rect input) {
	SDL_Rect obj;
	obj.x = floor((input.x -g_camera.x) * g_camera.zoom);
	obj.y = floor((input.y - g_camera.y) * g_camera.zoom);
	obj.w = ceil(input.w * g_camera.zoom);
	obj.h = ceil(input.h * g_camera.zoom);		
	return obj;
}

rect transformRect(rect input) {
	rect obj;
	obj.x = floor((input.x -g_camera.x) * g_camera.zoom);
	obj.y = floor((input.y - g_camera.y) * g_camera.zoom);
	obj.width = ceil(input.width * g_camera.zoom);
	obj.height = ceil(input.height * g_camera.zoom);		
	return obj;
}

class collision {
public:
	rect bounds;
	bool active = true;
	int layer = 0;
	collision(int x1f, int y1f, int x2f, int y2f, int flayer) {
		bounds.x = x1f;
		bounds.y = y1f;
		bounds.width = x2f;
		bounds.height = y2f;
		layer = flayer;
		g_collisions[layer].push_back(this);
	}
	~collision() {
		int hold = layer;
		g_collisions[hold].erase(remove(g_collisions[hold].begin(), g_collisions[hold].end(), this), g_collisions[hold].end());
	}
};

bool LineTrace(int x1, int y1, int x2, int y2, int size = 30, int layer = 0) {
	float resolution = 10;
	
	// SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	for (float i = 1; i < resolution; i++) {
		int xsize = size * p_ratio;
		int xpos = (i/resolution) * x1 + (1 - i/resolution) * x2;
		int ypos = (i/resolution) * y1 + (1 - i/resolution) * y2;
		rect a = rect(xpos - xsize/2, ypos - size/2, xsize, size);
		SDL_Rect b = {((xpos- xsize/2) - g_camera.x) * g_camera.zoom, ((ypos- size/2) - g_camera.y) * g_camera.zoom, xsize, size};
		
		// SDL_RenderDrawRect(renderer, &b);
		
		for (int j = 0; j < g_collisions[layer].size(); j++) {
			if(RectOverlap(a, g_collisions[layer][j]->bounds)) {
				return false;
			}
		}
	}
	// SDL_RenderPresent(renderer);
	// SDL_RenderClear(renderer);
	// SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	return true;
}

class door {
public:
	float x;
	float y;
	float width = 50;
	float height = 50;
	float friction;
	rect bounds;
	string to_map;
	string to_point;
	

	door(SDL_Renderer * renderer, const char* fmap, string fto_point,  int fx, int fy, int fwidth, int fheight) {		
		M("door()");
		this->x = fx;
		this->y = fy;

		this->bounds.x = fx;
		this->bounds.y = fy;
		
		to_map = fmap;
		to_point = fto_point;

		this->width = fwidth;
		this->height = fheight;
		this->bounds.width = width;
		this->bounds.height = height;
		g_doors.push_back(this);
		
	}

	~door() {
		g_doors.erase(remove(g_doors.begin(), g_doors.end(), this), g_doors.end());
	}

};


class tile {
public:
	int x = 0;
	int y = 0;
	int z = 0; //represents layer. 0 default

	float width = 0;
	float height = 0;
	float xoffset = 0; //for aligning texture across the map
	float yoffset = 0;
	int texwidth = 0;
	int texheight = 0;
	bool wraptexture = 1; //should we tile the image or stretch it?
	bool wall = 0; //darken image if it is used for a wall as opposed to a floor
	bool asset_sharer = 0; //1 if the tile is sharing another tile's texture, and this texture should in this case not be deleted in the destructor

	SDL_Surface* image = 0;
	SDL_Texture* texture = 0;
	string fileaddress = "df"; //for checking if someone else has already loaded a texture
	string mask_fileaddress = "&"; //unset value
	
	tile(SDL_Renderer * renderer, const char* filename, const char* mask_filename, int fx, int fy, int fwidth, int fheight, bool fwall=0) {		
		this->x = fx;
		this->y = fy;
		this->width = fwidth;
		this->height = fheight;
		this->wall = fwall;
		
		fileaddress = filename;
		bool cached = false;
		//has someone else already made a texture?
		for(long long unsigned int i=0; i < g_tiles.size(); i++){
			if(g_tiles[i]->fileaddress == this->fileaddress && g_tiles[i]->mask_fileaddress == mask_filename) {
				//make sure both are walls or both aren't
				if(g_tiles[i]->wall == this->wall) {
					//M("sharing a texture" );
					cached = true;
					this->texture = g_tiles[i]->texture;
					SDL_QueryTexture(g_tiles[i]->texture, NULL, NULL, &texwidth, &texheight);
					this->asset_sharer = 1;
					break;
				}
			}
		}
		if(cached) {
						
		} else {
			image = IMG_Load(filename);
			texture = SDL_CreateTextureFromSurface(renderer, image);
			if(wall) {
				//make walls a bit darker
				SDL_SetTextureColorMod(texture, -40, -40, -40);
			} else {
				SDL_SetTextureColorMod(texture, -20, -20, -20);
			}
			
			SDL_QueryTexture(texture, NULL, NULL, &texwidth, &texheight);
			if(fileaddress.find("OCCLUSION") != string::npos) {
				SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_MOD);
				
			}
			if(mask_filename[0] != '&') {
				SDL_DestroyTexture(texture);
				SDL_Surface* smask = IMG_Load(mask_filename);
				SDL_Texture* mask = SDL_CreateTextureFromSurface(renderer, smask);
				SDL_Texture* diffuse = SDL_CreateTextureFromSurface(renderer, image);
				texture = MaskTexture(renderer, diffuse, mask);
				SDL_FreeSurface(smask);
				SDL_DestroyTexture(mask);
				SDL_DestroyTexture(diffuse);
				SDL_SetTextureColorMod(texture, -20, -20, -20);
			}
		}

		
		mask_fileaddress = mask_filename;

		
		this->xoffset = this->x % int(this->texwidth);
		this->yoffset = this->y % int(this->texheight);

		SDL_FreeSurface(image);
		g_tiles.push_back(this);
	}

	~tile() {
		M("~tile()" );
		g_tiles.erase(remove(g_tiles.begin(), g_tiles.end(), this), g_tiles.end());
		if(!asset_sharer) {
			SDL_DestroyTexture(texture);
		}
		
	}

	void render(SDL_Renderer * renderer, camera fcamera) {
		SDL_Point nowt = {0, 0};
		rect obj(floor((x -fcamera.x)* fcamera.zoom) , floor((y-fcamera.y) * fcamera.zoom), ceil(width * fcamera.zoom), ceil(height * fcamera.zoom));		
		rect cam(0, 0, fcamera.width, fcamera.height);
		
		if(RectOverlap(obj, cam)) {
			
			if(this->wraptexture) {	
				SDL_Rect srcrect;
				SDL_Rect dstrect;
				int ypos = 0;
				int xpos = 0;

				srcrect.x = xoffset;
				srcrect.y = yoffset;
				while(1) {
					if(srcrect.x == xoffset) {
						srcrect.w = texwidth - xoffset;
						dstrect.w = texwidth - xoffset;
					} else {
						srcrect.w = texwidth;
						dstrect.w = texwidth;
					}
					if(srcrect.y == yoffset) {
						
						srcrect.h = texheight - yoffset;
						dstrect.h = texheight - yoffset;
					} else {
						dstrect.h = texheight;
						srcrect.h = texheight;
					}
					

					
					
					

					dstrect.x = (x + xpos);
					dstrect.y = (y + ypos);
					
					
					
					
					//are we still inbounds?
					if(xpos + dstrect.w > this->width) {
						dstrect.w = this->width - xpos;
						if(dstrect.w + srcrect.x > texwidth) {
							dstrect.w = texwidth - srcrect.x;
						}
						srcrect.w = dstrect.w;
					}
					if(ypos + dstrect.h > this->height) {
						dstrect.h = this->height - ypos;
						if(dstrect.h + srcrect.y > texheight) {
							dstrect.h = texheight - srcrect.y;
						}
						srcrect.h = dstrect.h;
						
					}
					//are we still in the texture
					
					
					
					


					//transform
					dstrect.w = ceil(dstrect.w * fcamera.zoom);
					dstrect.h = ceil(dstrect.h * fcamera.zoom);

					dstrect.x = floor((dstrect.x - fcamera.x)* fcamera.zoom);
					dstrect.y = floor((dstrect.y - fcamera.y)* fcamera.zoom);
					

					SDL_RenderCopyEx(renderer, texture, &srcrect, &dstrect, 0, &nowt, SDL_FLIP_NONE);
					
					
					
					xpos += srcrect.w;
					srcrect.x = 0;
					
					if(xpos >= this->width) {
						xpos = 0;
						srcrect.x = xoffset;
						
						ypos += srcrect.h;
						srcrect.y = 0;	
						if(ypos >= this->height)
							break;
					}	
				}
			} else {
				SDL_Rect dstrect = { obj.x, obj.y, obj.width, obj.height};
				SDL_RenderCopy(renderer, texture, NULL, &dstrect);
			}

		}
	}
};

class weapon {
public:
	float maxCooldown = 400; //ms
	float cooldown = 0;
	//collision data, data about movement
	int max_combo = 2;
	int combo = 0; //increments
	float maxComboResetSeconds = 1;
	float comboResetSeconds = 0;
	bool canBeHeldDown = 1;
	float shotLifetime = 500; //ms
	int width = 20;
	int height = 20;
	float size = 0.1;
	SDL_Texture* texture;

	float xoffset = 5;

	//given some float time, return floats for x and y position
	//will be rotated/flipped
	float forward(float time) {
		return 12;
	} 

	float sideways(float time) {
		//return 10 * cos(time / 45);
		return 0;
	}

	weapon(string spriteadress) {
		string spritefile = "sprites/" + spriteadress + ".png";
		SDL_Surface* image = IMG_Load(spritefile.c_str());
		if(onionmode) {
			image = IMG_Load("sprites/onionmode/onion.png");
		}
		texture = SDL_CreateTextureFromSurface(renderer, image);
		SDL_QueryTexture(texture, NULL, NULL, &width, &height);
		width *= size;
		height *= size;
		SDL_FreeSurface(image);
		g_weapons.push_back(this);
	}
	~weapon() {
		SDL_DestroyTexture(texture);
		g_weapons.erase(remove(g_weapons.begin(), g_weapons.end(), this), g_weapons.end());
	}
};

//anything that exists in the 3d world
class actor {
public:
	float x = 0;
	float y = 0;
	float z = 0;
	float width = 0;
	float height = 0;
	SDL_Texture* texture;
	rect bounds;
	string name;

	//add entities and mapObjects to g_actors with dc
	actor() {
		M("actor()");
		g_actors.push_back(this);
	}
	
	~actor() {
		M("~actor()");
		g_actors.erase(remove(g_actors.begin(), g_actors.end(), this), g_actors.end());
	}

	virtual void render(SDL_Renderer * renderer, camera fcamera) {
		
	}
	
	
	int getOriginX() {
		return  x + width/2;
	}

	int getOriginY() {
		return y + bounds.y + bounds.height/2;
	}
};


class cshadow:public actor {
public:
	float size;
	entity* owner = 0;
	SDL_Surface* image = 0;
	SDL_Texture* texture = 0;
	bool asset_sharer = 0;
	int xoffset = 0;
	int yoffset = 0;

	cshadow(SDL_Renderer * renderer, float fsize) {
		size = fsize;
		//if there is another shadow steal his texture
		if( g_shadows.size() > 0) {
			texture = g_shadows[0]->texture;
			this->asset_sharer = 1;	
		} else {
			image = IMG_Load("sprites/shadow.png");
			texture = SDL_CreateTextureFromSurface(renderer, image);
			SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_MOD);
			SDL_FreeSurface(image);
		}
		
		g_shadows.push_back(this);
	}

	~cshadow() {
		M("~cshadow()" );
		if(!asset_sharer) { SDL_DestroyTexture(texture);}
		g_shadows.erase(remove(g_shadows.begin(), g_shadows.end(), this), g_shadows.end());
	}
	void render(SDL_Renderer* renderer, camera fcamera);
};


class mapObject:public actor {
public:
	
	float xoffset = 0;
	float yoffset = 64;
	bool wall; //to darken entwalls
	float sortingOffset = 0; //to make entites float in the air, or sort as if they are higher or lower than they are first use was for building diagonal walls
	float extraYOffset = 0; //used to encode a permanent y axis offset, for diagonal walls
	//0 - slant left, 1 - slant right, 2 - pillar
	int effect = 0;
	bool asset_sharer = 0;
	int framewidth = 0;
	int frameheight = 0;

	//potentially removable
	float curwidth;
	float curheight;

	mapObject(SDL_Renderer * renderer, string imageadress, int fx, int fy, int fz, int fwidth, int fheight, bool fwall = 0, float extrayoffset = 0, float fsortingoffset = 0) {
		M("mapObject() fake");
		
		name = imageadress;
		
		bool cached = false;
		wall = fwall;
		//has someone else already made a texture?
		for(long long unsigned int i=0; i < g_mapObjects.size(); i++){
			if(g_mapObjects[i]->name == this->name && g_mapObjects[i]->wall == this->wall) {
				//check if both are walls?
				cached = true;
				this->texture = g_mapObjects[i]->texture;
				this->asset_sharer = 1;
				break;
			}
		}
		if(cached) {
						
		} else {
			const char* plik = imageadress.c_str();
			SDL_Surface* image = IMG_Load(plik);
			texture = SDL_CreateTextureFromSurface(renderer, image);
			SDL_FreeSurface(image);
			if(fwall) {
				wall = 1;
				SDL_SetTextureColorMod(texture, -40, -40, -40);
			} else {
				//SDL_SetTextureColorMod(texture, -35, -35, -35);
				//SDL_SetTextureColorMod(texture, 0.8, 0.8, 0.8);
			}
			if(name.find("SHADING") != string::npos) {
				//SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
				//SDL_SetTextureAlphaMod(texture, 150);
				
			}
			if(name.find("OCCLUSION") != string::npos) {
				//cout << "blended " << name << endl;
				SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_MOD);
				//SDL_SetTextureAlphaMod(texture, 10);
				
			}
		}

		
		
		
		//used for tiling
		SDL_QueryTexture(texture, NULL, NULL, &this->framewidth, &this->frameheight);
		this->width = fwidth;
		this->x = fx;
		this->y = fy;
		this->z = fz;
		this->height = fheight;
		this->curheight = fheight;
		this->curwidth = fwidth;
		this->sortingOffset = fsortingoffset;
		this->xoffset = int(this->x) % int(this->framewidth);
		this->bounds.y = -55; //added after the ORIGIN was used for ent sorting rather than the FOOT.
		//this essentially just gives the blocks an invisible hitbox starting from their "head" so that their origin is in the middle
		//of the collision

		extraYOffset = extrayoffset;
		this->yoffset = int(this->y - this->height) % int(this->frameheight) + extrayoffset;
		if(fwall) {
			this->yoffset = int(this->y - this->height - (z * 0.5)) % int(this->frameheight) + extrayoffset;
		}
		g_mapObjects.push_back(this);
		
	}

	~mapObject() {
		g_mapObjects.erase(remove(g_mapObjects.begin(), g_mapObjects.end(), this), g_mapObjects.end());
	}

	void render(SDL_Renderer * renderer, camera fcamera) {
		SDL_Point nowt = {0, 0};

		rect obj(floor((x -fcamera.x)* fcamera.zoom) , floor((y-fcamera.y - height - XtoZ * z) * fcamera.zoom), ceil(width * fcamera.zoom), ceil(height * fcamera.zoom));		
		rect cam(0, 0, fcamera.width, fcamera.height);
		
		if(RectOverlap(obj, cam)) {
			/*
			//if its a wall, check if it is covering the player
			if(this->wall && protagForGlimmer != nullptr) {
				
				//make obj one block higher for the wallcap
				//obj.height -= 45;
				//obj.y += 45;
				if((protagForGlimmer->x - fcamera.x) * fcamera.zoom > obj.x && (protagForGlimmer->x - fcamera.x) * fcamera.zoom < obj.x + obj.width && (protagForGlimmer->y  - fcamera.y) * fcamera.zoom > obj.y && (protagForGlimmer->y  - fcamera.y) * fcamera.zoom < obj.y + obj.height) {
					protagGlimmerA = 1;
				}
				if((protagForGlimmer->x + protagForGlimmer->width - fcamera.x) * fcamera.zoom > obj.x && (protagForGlimmer->x + protagForGlimmer->width - fcamera.x) * fcamera.zoom < obj.x + obj.width && (protagForGlimmer->y  - fcamera.y) * fcamera.zoom > obj.y && (protagForGlimmer->y  - fcamera.y) * fcamera.zoom < obj.y + obj.height) {
					protagGlimmerB = 1;
				}
				if((protagForGlimmer->x - fcamera.x) * fcamera.zoom > obj.x && (protagForGlimmer->x - fcamera.x) * fcamera.zoom < obj.x + obj.width && (protagForGlimmer->y - protagForGlimmer->height - fcamera.y) * fcamera.zoom > obj.y && (protagForGlimmer->y - protagForGlimmer->height - fcamera.y) * fcamera.zoom < obj.y + obj.height) {
					protagGlimmerC = 1;
				}
				if((protagForGlimmer->x + protagForGlimmer->width - fcamera.x) * fcamera.zoom > obj.x && (protagForGlimmer->x + protagForGlimmer->width - fcamera.x) * fcamera.zoom < obj.x + obj.width && (protagForGlimmer->y - protagForGlimmer->height - fcamera.y) * fcamera.zoom > obj.y && (protagForGlimmer->y - protagForGlimmer->height - fcamera.y) * fcamera.zoom < obj.y + obj.height) {
					protagGlimmerD = 1;
				}
				
			}
			*/
			
			SDL_Rect srcrect;
			SDL_Rect dstrect;
			int ypos = 0;
			int xpos = 0;
			srcrect.x = xoffset;
			srcrect.y = yoffset;
			
			while(1) {
				if(srcrect.x == xoffset) {
						srcrect.w = framewidth - xoffset;
						dstrect.w = framewidth - xoffset;
					} else {
						srcrect.w = framewidth;
						dstrect.w = framewidth;
					}
					if(srcrect.y == yoffset) {
						
						srcrect.h = frameheight - yoffset;
						dstrect.h = frameheight - yoffset;
					} else {
						dstrect.h = frameheight;
						srcrect.h = frameheight;
					}

				dstrect.x = (x + xpos);
				dstrect.y = (y+ ypos- z * XtoZ );
				
				
				
				
				//are we still inbounds?
				if(xpos + dstrect.w > this->width) {
					dstrect.w = this->width - xpos;
					if(dstrect.w + srcrect.x > framewidth) {
						dstrect.w = framewidth - srcrect.x;
					}
					srcrect.w = dstrect.w;
				}
				if(ypos + dstrect.h > this->height ) {
					
					dstrect.h = this->height - ypos;
					if(dstrect.h + srcrect.y > frameheight) {
						dstrect.h = frameheight - srcrect.y;
					}
					srcrect.h = dstrect.h;
					
				}

				
				
				
				


				//transform
				dstrect.w = ceil(dstrect.w * fcamera.zoom);
				dstrect.h = ceil(dstrect.h * fcamera.zoom);

				dstrect.x = floor((dstrect.x - fcamera.x)* fcamera.zoom);
				dstrect.y = floor((dstrect.y - fcamera.y - height)* fcamera.zoom);
				

				SDL_RenderCopyEx(renderer, texture, &srcrect, &dstrect, 0, &nowt, SDL_FLIP_NONE );
				xpos += srcrect.w;
				srcrect.x = 0;
				
				if(xpos >= this->width) {
					
					xpos = 0;
					srcrect.x = xoffset;
					
					ypos += srcrect.h;
					srcrect.y = 0;	
					if(ypos >= this->height)
						break;
				}	
			}
		}
	}
};

class entity:public actor {
public:
	//dialogue
	vector<string> sayings;
	int dialogue_index = 0;

	//sounds
	Mix_Chunk* footstep;
	Mix_Chunk* footstep2;
	Mix_Chunk* voice;

	int footstep_reset = 0; //used for playing footsteps accurately with anim

	

	//movement
	float xagil = 0;
	float yagil = 0;
	float xaccel = 0;
	float yaccel = 0;
	float zaccel = 0;
	float xvel = 0;
	float yvel = 0;
	float zvel = 0;
	int layer = 0; //related to z, used for collisions
	bool grounded = 1; //is standing on ground
	float xmaxspeed = 0;
	float ymaxspeed = 0;
	float friction = 0;

	int update_z_time = 0; 
	int max_update_z_time = 1; //update zpos every x frames
	float oldz = 0;
	
	//combat
	weapon* hisWeapon;
	

	//animation
	bool animate = false; //does the squash/stretch animation for walking, talking... everything really
	float animtime = 0; //time since having started animating
	float animspeed;
	float animlimit; // the extent to the animation. 0.5 means halfway
	float curwidth;
	float curheight;
 	SDL_RendererFlip flip = SDL_FLIP_NONE; //SDL_FLIP_HORIZONTAL; // SDL_FLIP_NONE
	bool flipper = 0; //to flip when moving left or right

	int frame = 0; //current frame on SPRITESHEET
	int frameInAnimation = 0; //current frame in ANIMATION
	int animation = 0; //current animation, or the column of the spritesheet
	int defaultAnimation = 0;

	int framewidth = 120; //width of single frame
	int frameheight = 120; //height of frame
	int xframes = 1; //number of frames ACROSS the spritesheet
	int yframes = 1; //number of frames DOWN the spritesheet
	vector<coord*> framespots;
	bool up, down, left, right; //for chusing one of 8 animations for facing
	bool hadInput = 0; //had input this frame;
	int shooting = 0; //1 if character is shooting
	

	//object-related design
	bool dynamic = true; //true for things such as wallcaps. movement/collision is not calculated if this is false
	bool invincible = true; //so innocent people dont take damage from shots
	bool inParty = false;
	bool enemy = false;
	bool talks = false;
	bool wallcap = false; //used for wallcaps 
	cshadow * shadow = 0;
	
	//for textured entities (e.g. wallcap)
	float xoffset = 0;
	float yoffset = 64;
	bool wall; //to darken entwalls
	float sortingOffset = 0; //to make entites float in the air, or sort as if they are higher or lower than they are first use was for building diagonal walls
	float extraYOffset = 0; //used to encode a permanent y axis offset, for diagonal walls
	//0 - slant left, 1 - slant right, 2 - pillar
	int effect = 0;
	bool asset_sharer = 0;

	//self-data
	int data[25] = {0};
	
	

	//default constructor is called automatically for children
	entity() {
		//M("entity()" );
	};

	entity(SDL_Renderer * renderer, string filename, float sizeForDefaults = 1) {
		M("entity()" );
		M("THIS SHOULD BE RUNNING");
		SDL_Delay(500);
		//temporary until I edit the footstep effect
		
		
		ifstream file;
		bool using_default = 0;
		this->name = filename;

		string loadstr;
		//try to open from local map folder first
		
		loadstr = "maps/" + g_map + "/" + filename + ".ent";
		D(loadstr);
		const char* plik = loadstr.c_str();
		
		file.open(plik);
		
		if (!file.is_open()) {
			//load from global folder
			loadstr = "entities/" + filename + ".ent";
			const char* plik = loadstr.c_str();
			
			file.open(plik);
			
			if (!file.is_open()) {
				//just make a default entity
				using_default = 1;
				string newfile = "entities/default.ent";
				file.open(newfile);
			}
		}
		
		string temp;
		file >> temp;
		string spritefilevar;
		
		if(!using_default) {
			spritefilevar = temp; 
		} else {
			spritefilevar = "sprites/" + filename + ".png";
			M(spritefilevar );
		}
		
		const char* spritefile = spritefilevar.c_str();
		float size;
		string comment;
		file >> comment;
		file >> size;

		file >> comment;
		file >> this->xagil;
		file >> this->yagil;
		
		
		file >> comment;
		file >> this->xmaxspeed;
		file >> this->ymaxspeed;
		
		file >> comment;
		file >> this->friction;
		file >> comment;
		file >> this->bounds.width;
		file >> this->bounds.height;
		
		file >> comment;
		file >> this->bounds.x;
		file >> this->bounds.y;

		file >> comment;
		float fsize;
		file >> fsize;
		shadow = new cshadow(renderer, fsize);
		
		this->shadow->owner = this;

		file >> comment;
		file >> shadow->xoffset;
		file >> shadow->yoffset;		
		
		file >> comment;
		file >> this->animspeed;
		file >> this->animlimit;

		file >> comment;
		file >> this->defaultAnimation;
		animation = defaultAnimation;
		
		file >> comment;
		file >> this->framewidth;
		file >> this->frameheight;
		this->shadow->width = framewidth * fsize;
		this->shadow->height = framewidth * fsize * (1/p_ratio);
		
		

		//move shadow to feet
		//this->shadow->y -= this->shadow->height / 2;

		//(owner->height -(height/2))

		file >> comment;
		bool setcollisionfromshadow;
		file >> setcollisionfromshadow;
		if(setcollisionfromshadow) {
			this->bounds.width = this->shadow->width;
			this->bounds.height = this->shadow->height;
			this->bounds.x = this->shadow->x;
			this->bounds.y = this->shadow->y;
		}

		

		file >> comment;
		file >> this->dynamic;
		
		file >> comment;
		file >> this->talks;

		file >> comment;
		file >> this->enemy;
		file.close();
		
		//load dialogue file
		if(talks) {
			string txtfilename = "text/" + filename + ".txt";

			ifstream file(txtfilename);
			string line;

			//load voice
			getline(file, line);
			line = "sounds/voice-" + line +".wav";
			voice = Mix_LoadWAV(line.c_str());

			while(getline(file, line)) {
				sayings.push_back(line);
			}
			
			
		}
		
		SDL_Surface* image = IMG_Load(spritefile);
		if(onionmode) {
			image = IMG_Load("sprites/onionmode/onion.png");
			
		}
		
		texture = SDL_CreateTextureFromSurface(renderer, image);
		this->width = size * framewidth;
		this->height = size * frameheight;

		//move shadow to feet
		shadow->xoffset += width/2 - shadow->width/2;
		shadow->yoffset -= height - shadow->height;
		
		if(using_default) {
			int w, h;
			SDL_QueryTexture(texture, NULL, NULL, &w, &h);
			this->width = w * sizeForDefaults;
			this->height = h * sizeForDefaults;
			this->shadow->width = 0;
		}

		//move shadow to feet
		//this->bounds.y +=this->height/2;
		curwidth = width;
		curheight = height;
		
		if(!using_default) {
			xframes = image->w /framewidth;
			yframes = image->h /frameheight;
		
	

		for (int j = 0; j < image->h; j+=frameheight) {
			for (int i = 0; i < image->w; i+= framewidth) {
				coord* a = new coord(); 
				a->x = i;
				a->y = j;
				framespots.push_back(a);
			}
		}

		}
		SDL_FreeSurface(image);
		g_entities.push_back(this);
		
	}

	//used mostly for fake entities, e.g. wallcaps
	entity(SDL_Renderer * renderer, string imageadress, int fx, int fy, int fz, int fwidth, int fheight, bool fwall = 0, float extrayoffset = 0, float fsortingoffset = 0) {
		M("entity() fake");
		//deprecated and should be removed soon
		
	}

	~entity() {
		M("~entity()" );
		D(name);
		if (!wallcap) {
			delete shadow;
		}
		if(!asset_sharer) {
			SDL_DestroyTexture(texture);
		}
		for (auto p : framespots) {
			delete p;
		} 
		framespots.clear();
		//Mix_FreeChunk(footstep);
		//Mix_FreeChunk(footstep2);
		g_entities.erase(remove(g_entities.begin(), g_entities.end(), this), g_entities.end());
	}

	int getOriginX() {
		return  x + width/2;
	}

	int getOriginY() {
		if(dynamic) {
			return y + bounds.y + bounds.height/2;
		} else {
			return y + bounds.y + bounds.height/2;
		}
	}

	void shoot();

	void render(SDL_Renderer * renderer, camera fcamera) {
		//if its a wallcap, tile the image just like a maptile
		if(wallcap && effect == 0) {
			
			

		SDL_Point nowt = {0, 0};

		rect obj(floor((x -fcamera.x)* fcamera.zoom) , floor((y-fcamera.y - height - XtoZ * z) * fcamera.zoom), ceil(width * fcamera.zoom), ceil(height * fcamera.zoom));		
		rect cam(0, 0, fcamera.width, fcamera.height);
		
		if(RectOverlap(obj, cam)) {
			/*
			//if its a wall, check if it is covering the player
			if(this->wall && protagForGlimmer != nullptr) {
				
				//make obj one block higher for the wallcap
				//obj.height -= 45;
				//obj.y += 45;
				if((protagForGlimmer->x - fcamera.x) * fcamera.zoom > obj.x && (protagForGlimmer->x - fcamera.x) * fcamera.zoom < obj.x + obj.width && (protagForGlimmer->y  - fcamera.y) * fcamera.zoom > obj.y && (protagForGlimmer->y  - fcamera.y) * fcamera.zoom < obj.y + obj.height) {
					protagGlimmerA = 1;
				}
				if((protagForGlimmer->x + protagForGlimmer->width - fcamera.x) * fcamera.zoom > obj.x && (protagForGlimmer->x + protagForGlimmer->width - fcamera.x) * fcamera.zoom < obj.x + obj.width && (protagForGlimmer->y  - fcamera.y) * fcamera.zoom > obj.y && (protagForGlimmer->y  - fcamera.y) * fcamera.zoom < obj.y + obj.height) {
					protagGlimmerB = 1;
				}
				if((protagForGlimmer->x - fcamera.x) * fcamera.zoom > obj.x && (protagForGlimmer->x - fcamera.x) * fcamera.zoom < obj.x + obj.width && (protagForGlimmer->y - protagForGlimmer->height - fcamera.y) * fcamera.zoom > obj.y && (protagForGlimmer->y - protagForGlimmer->height - fcamera.y) * fcamera.zoom < obj.y + obj.height) {
					protagGlimmerC = 1;
				}
				if((protagForGlimmer->x + protagForGlimmer->width - fcamera.x) * fcamera.zoom > obj.x && (protagForGlimmer->x + protagForGlimmer->width - fcamera.x) * fcamera.zoom < obj.x + obj.width && (protagForGlimmer->y - protagForGlimmer->height - fcamera.y) * fcamera.zoom > obj.y && (protagForGlimmer->y - protagForGlimmer->height - fcamera.y) * fcamera.zoom < obj.y + obj.height) {
					protagGlimmerD = 1;
				}
				
			}
			*/
			
			SDL_Rect srcrect;
			SDL_Rect dstrect;
			int ypos = 0;
			int xpos = 0;
			srcrect.x = xoffset;
			srcrect.y = yoffset;
			
			while(1) {
				if(srcrect.x == xoffset) {
						srcrect.w = framewidth - xoffset;
						dstrect.w = framewidth - xoffset;
					} else {
						srcrect.w = framewidth;
						dstrect.w = framewidth;
					}
					if(srcrect.y == yoffset) {
						
						srcrect.h = frameheight - yoffset;
						dstrect.h = frameheight - yoffset;
					} else {
						dstrect.h = frameheight;
						srcrect.h = frameheight;
					}

				dstrect.x = (x + xpos);
				dstrect.y = (y+ ypos- z * XtoZ );
				
				
				
				
				//are we still inbounds?
				if(xpos + dstrect.w > this->width) {
					dstrect.w = this->width - xpos;
					if(dstrect.w + srcrect.x > framewidth) {
						dstrect.w = framewidth - srcrect.x;
					}
					srcrect.w = dstrect.w;
				}
				if(ypos + dstrect.h > this->height ) {
					
					dstrect.h = this->height - ypos;
					if(dstrect.h + srcrect.y > frameheight) {
						dstrect.h = frameheight - srcrect.y;
					}
					srcrect.h = dstrect.h;
					
				}

				
				
				
				


				//transform
				dstrect.w = ceil(dstrect.w * fcamera.zoom);
				dstrect.h = ceil(dstrect.h * fcamera.zoom);

				dstrect.x = floor((dstrect.x - fcamera.x)* fcamera.zoom);
				dstrect.y = floor((dstrect.y - fcamera.y - height)* fcamera.zoom);
				

				SDL_RenderCopyEx(renderer, texture, &srcrect, &dstrect, 0, &nowt, SDL_FLIP_NONE );
				xpos += srcrect.w;
				srcrect.x = 0;
				
				if(xpos >= this->width) {
					
					xpos = 0;
					srcrect.x = xoffset;
					
					ypos += srcrect.h;
					srcrect.y = 0;	
					if(ypos >= this->height)
						break;
				}	
			}
		}
	
	} else {

			rect obj(floor((x -fcamera.x + (width-curwidth)/2)* fcamera.zoom) , floor(((y-curheight - z * XtoZ) - fcamera.y) * fcamera.zoom), ceil(curwidth * fcamera.zoom), ceil(curheight * fcamera.zoom));		
			rect cam(0, 0, fcamera.width, fcamera.height);
			
			if(RectOverlap(obj, cam)) {
				//set frame from animation
				// animation is y, frameInAnimation is x
				if(dynamic && hadInput) {
					if(up) {
						if(left) {
							animation = 1;
							flip = SDL_FLIP_NONE;
						} else if (right) {
							animation = 1;
							flip = SDL_FLIP_HORIZONTAL;
						} else {
							animation = 0;
							flip = SDL_FLIP_NONE;
						}
					} else if (down) {
						if(left) {
							animation = 3;
							flip = SDL_FLIP_NONE;
						} else if (right) {
							animation = 3;
							flip = SDL_FLIP_HORIZONTAL;
						} else {
							animation = 4;
							flip = SDL_FLIP_NONE;
						}
					} else {
						if(left) {
							animation = 2;
							flip = SDL_FLIP_NONE;
						} else if (right) {
							animation = 2;
							flip = SDL_FLIP_HORIZONTAL;
						} else {
							//default
							animation = 4;
							flip = SDL_FLIP_NONE;
						}
					}
				}
				hadInput = 0;

				frame = animation * xframes + frameInAnimation;
				SDL_Rect dstrect = { obj.x, obj.y, obj.width, obj.height};
				if(framespots.size() > 0) {
					SDL_Rect dstrect = { obj.x, obj.y, obj.width, obj.height};
					SDL_Rect srcrect = {framespots[frame]->x,framespots[frame]->y, framewidth, frameheight};
					const SDL_Point center = {0 ,0};
					if(texture != NULL) {
						SDL_RenderCopyEx(renderer, texture, &srcrect, &dstrect, 0, &center, flip);
					}
				} else {
					if(texture != NULL) {
						SDL_RenderCopy(renderer, texture, NULL, &dstrect);
					}
				}
			}
		}
	}

	void move_up() {
		yaccel = -1* yagil;
		if(shooting) { return;}
		up = true;
		down = false;
		hadInput = 1;
	}

	void stop_verti() {
		yaccel = 0;
		if(shooting) { return;}
		up = false;
		down = false;
	}

	void move_down() {
		yaccel = yagil;
		if(shooting) { return;}
		down = true;
		up = false;
		hadInput = 1;
	}

	void move_left() {
		xaccel = -1 * xagil;
		if(shooting) { return;}
		left = true;
		right = false;
		hadInput = 1;
	}

	void stop_hori() {
		xaccel = 0;
		if(shooting) { return;}
		left = false;
		right = false;
	}
	
	void move_right() {
		xaccel = xagil;
		if(shooting) { return;}
		right = true;
		left = false;
		hadInput = 1;
		
	}

	void shoot_up() {
		shooting = 1;
		up = true;
		down = false;
		hadInput = 1;
	}
	
	void shoot_down() {
		shooting = 1;
		down = true;
		up = false;
		hadInput = 1;
	}
	
	void shoot_left() {
		shooting = 1;
		left = true;
		right = false;
		hadInput = 1;
	}

	void shoot_right() {
		shooting = 1;
		xaccel = xagil;
		right = true;
		left = false;
		hadInput = 1;
	}

	template <class T>
	T* Get_Closest_Node(vector<T*> array) {
		float min_dist = 0;
		T* ret;
		bool flag = 1;

		//todo check for collisions
		for (int i = 0; i < array.size(); i++) {
			float dist = Distance(this->x, this->y, array[i]->x, array[i]->y);
			if(dist < min_dist || flag) {
				min_dist = dist;
				ret = array[i];
				flag = 0;
			}
		}
		return ret;
		
	}

	//returns a pointer to a door that the player used
	virtual door* update_movement(vector<collision*> collisions, vector<door*> doors, float elapsed) {
		
		if(animate && !transition) {
			curwidth = width * ((sin(animtime*animspeed))   + (1/animlimit)) * (animlimit);
			curheight = height * ((sin(animtime*animspeed + PI))+ (1/animlimit)) * (animlimit);
			animtime += elapsed;
			if(this == protag && (abs(xvel) > 50 || abs(yvel) > 50) && (1 - sin(animtime * animspeed) < 0.01 || 1 - sin(animtime * animspeed + PI) < 0.01)) {
				if(footstep_reset) {
					footstep_reset = 0;
					if(1 - sin(animtime * animspeed) < 0.01) {
						//playSound(-1, footstep, 0);
					} else {
						//playSound(-1, footstep2, 0);
					}
					
					
				}
			} else {
				footstep_reset = 1;
			}
		} else {
			animtime = 0;
			curwidth = curwidth * 0.8 + width * 0.2;
			curheight = curheight * 0.8 + height* 0.2;
		}
		
		if(!dynamic) { return nullptr; }
		//should we animate?
		if(xaccel != 0 || yaccel != 0) {
			animate = 1;
		} else {  
			animate = 0;
		}
		
		

		//normalize accel vector
		float vectorlen = pow( pow(xaccel, 2) + pow(yaccel, 2), 0.5) / (xmaxspeed);
		if(xaccel != 0) {
			xaccel /=vectorlen;
		}
		if(yaccel != 0) {
			yaccel /=vectorlen;
			yaccel /= p_ratio;
		}
		
		
		if(xaccel > 0 && xvel < xmaxspeed) {
			xvel += xaccel * ((double) elapsed / 256.0);
		}
		
		if(xaccel < 0 && xvel > -1 * xmaxspeed) {
			xvel += xaccel * ((double) elapsed / 256.0);
		}

		if(yaccel > 0 && yvel < ymaxspeed) {
			yvel += yaccel* ((double) elapsed / 256.0);
		}
		
		if(yaccel < 0 && yvel > -1 * ymaxspeed) {
			yvel += yaccel* ((double) elapsed / 256.0);
		}

		rect movedbounds;
		bool ycollide = 0;
		bool xcollide = 0;
		

		

		//turn off collisions if using the map-editor
		if(collisionsenabled) {
			//..check door
			if(this == protag) {
				for (int i = 0; i < doors.size(); i++) {	
					//update bounds with new pos
					rect movedbounds = rect(bounds.x + x + xvel * ((double) elapsed / 256.0), bounds.y + y  + yvel * ((double) elapsed / 256.0), bounds.width, bounds.height);
					//did we walk into a door?
					if(RectOverlap(movedbounds, doors[i]->bounds)) {
						//take the door.
						return doors[i];
					}
				}
			}

			for (int i = 0; i < collisions.size(); i++) {	
				//update bounds with new pos
				rect movedbounds = rect(bounds.x + x, bounds.y + y  + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
				//uh oh, did we collide with something?
				if(RectOverlap(movedbounds, collisions[i]->bounds)) {
					ycollide = true;
					yvel = 0;
							


					
				}
				//update bounds with new pos
				movedbounds = rect(bounds.x + x + (2*xvel * ((double) elapsed / 256.0)), bounds.y + y, bounds.width, bounds.height);
				//uh oh, did we collide with something?
				if(RectOverlap(movedbounds, collisions[i]->bounds)) {
					//collision detected
					xcollide = true;
					xvel = 0;
					
				}
			}

			int ytemp = 0; //to be added to y to create sliding along diagonal walls, but cannot be applied mid-check
			//try for triangles
			bool overlap = 0;
			
			for(long long unsigned int i=0; i < g_triangles.size(); i++){
				//test both components of velocity first -- player can move diagonally and avoid wall
				if(!TriRectOverlap(g_triangles[i], this->x + (2* xvel * ((double) elapsed / 256.0)), this->y - this->bounds.height/2 + (2* yvel * ((double) elapsed / 256.0)), this->bounds.width, this->bounds.height, -1)) {
					continue;
					
					
				}
				if(TriRectOverlap(g_triangles[i], this->x + (2* xvel * ((double) elapsed / 256.0)), this->y - this->bounds.height/2, this->bounds.width, this->bounds.height, -2)) {
					
					if(!TriRectOverlap(g_triangles[i], this->x + (2* xvel * ((double) elapsed / 256.0)), this->y - this->bounds.height/2 - (2*xvel * ((double) elapsed / 256.0)), this->bounds.width+ 1, this->bounds.height, -1) ) {
						
						//yaccel = -1 * yagil;
						xvel *= 0.8;
						ytemp-=(2*xvel * ((double) elapsed / 256.0));
						
					} else {
						if(!TriRectOverlap(g_triangles[i], this->x + (2* xvel * ((double) elapsed / 256.0)), this->y - this->bounds.height/2 + ( 2* xvel * ((double) elapsed / 256.0)), this->bounds.width, this->bounds.height, -1) ) {
							//yaccel = yagil;
							xvel *= 0.8;
							ytemp+= (2*xvel * ((double) elapsed / 256.0)) ;
						} else {
							xcollide = true;
							//xvel = 0;
						}
					} 
					
				}
					
				
				if(TriRectOverlap(g_triangles[i], this->x, this->y - this->bounds.height/2 + (2* yvel * ((double) elapsed / 256.0)), this->bounds.width, this->bounds.height, -2)) {
					
					
					if(!TriRectOverlap(g_triangles[i], this->x - (2* yvel * ((double) elapsed / 256.0)), this->y - this->bounds.height/2 + (2* yvel * ((double) elapsed / 256.0)), this->bounds.width, this->bounds.height, -1) ) {
						//xaccel = -1 * xagil;
						yvel *= 0.8;
						x-= (2*yvel * ((double) elapsed / 256.0));
					
					} else {
						if(!TriRectOverlap(g_triangles[i], this->x + (2* yvel * ((double) elapsed / 256.0)), this->y - this->bounds.height/2 + (2* yvel * ((double) elapsed / 256.0)), this->bounds.width, this->bounds.height, -1) ) {
							//xaccel = 1 * xagil;
							yvel *= 0.8;
							x+= (2*yvel * ((double) elapsed / 256.0));
						} else {
							ycollide = true;
							//yvel = 0;
						}
					}
				}
				
			}
			y += ytemp;
		}
		


		if(!ycollide && !transition) { 
			y+= yvel * ((double) elapsed / 256.0);
			
		}

		//when coordinates are bungled, it isnt happening here
		if(!xcollide && !transition) { 
			x+= xvel * ((double) elapsed / 256.0);
		}
		
		yvel *= pow(friction, ((double) elapsed / 256.0));
		xvel *= pow(friction, ((double) elapsed / 256.0));

		float heightfloor = 0; //filled with floor z from heightmap
		if(g_heightmaps.size() > 0 /*&& update_z_time < 1*/) {
		bool using_heightmap = 0;
		int heightmap_index = 0;

		rect tilerect;
		rect movedbounds;
		//get what tile entity is on
		for (int i = g_tiles.size() - 1; i >= 0; i--) {
			if(g_tiles[i]->fileaddress == "tiles/marker.png") {continue; }
			tilerect = rect(g_tiles[i]->x, g_tiles[i]->y, g_tiles[i]->width, g_tiles[i]->height);
			movedbounds = rect(this->x + xvel * ((double) elapsed / 256.0), this->y + yvel * ((double) elapsed / 256.0) - this->bounds.height, this->bounds.width, this->bounds.height);
			if(RectOverlap(tilerect, movedbounds)) {
				for (int j = 0; j < g_heightmaps.size(); j++) {
					if(g_heightmaps[j]->name == g_tiles[i]->fileaddress) {
						heightmap_index = j;
						using_heightmap = 1;
						break;
					}
				}
				//current tile has no heightmap
				break;
			}
		}

			
			update_z_time = max_update_z_time;
			//update z position
			SDL_Color rgb = {0, 0, 0};
			heightmap* thismap = g_heightmaps[heightmap_index];
			Uint8 maxred = 0;
			if(using_heightmap) {
				//try each corner;
				thismap->image->w;
				//code for middle
				// Uint32 data = thismap->getpixel(thismap->image, (int)(this->x + xvel * ((double) elapsed / 256.0) + 0.5 * this->width) % thismap->image->w, (int)(this->y + yvel * ((double) elapsed / 256.0) - 0.5 * this->bounds.height) % thismap->image->h);
				// SDL_GetRGB(data, thismap->image->format, &rgb.r, &rgb.g, &rgb.b);
				Uint32 data = thismap->getpixel(thismap->image, (int)(this->x + xvel * ((double) elapsed / 256.0)) % thismap->image->w, (int)(this->y + yvel * ((double) elapsed / 256.0)) % thismap->image->h);
				SDL_GetRGB(data, thismap->image->format, &rgb.r, &rgb.g, &rgb.b);
				
				movedbounds = rect(this->x + xvel * ((double) elapsed / 256.0), this->y + yvel * ((double) elapsed / 256.0), 1, 1);
				if(RectOverlap(tilerect, movedbounds)) {
					maxred = max(maxred, rgb.r);
				}

			 	data = thismap->getpixel(thismap->image, (int)(this->x + xvel * ((double) elapsed / 256.0) + this->bounds.width) % thismap->image->w, (int)(this->y + yvel * ((double) elapsed / 256.0)) % thismap->image->h);
				SDL_GetRGB(data, thismap->image->format, &rgb.r, &rgb.g, &rgb.b);
				
				movedbounds = rect(this->x + xvel * ((double) elapsed / 256.0) + this->bounds.width, this->y + yvel * ((double) elapsed / 256.0), 1, 1);
				if(RectOverlap(tilerect, movedbounds)) {
					maxred = max(maxred, rgb.r);
				}

				data = thismap->getpixel(thismap->image, (int)(this->x + xvel * ((double) elapsed / 256.0)) % thismap->image->w, (int)(this->y + yvel * ((double) elapsed / 256.0) - this->bounds.height) % thismap->image->h);
				SDL_GetRGB(data, thismap->image->format, &rgb.r, &rgb.g, &rgb.b);
				
				movedbounds = rect(this->x + xvel * ((double) elapsed / 256.0), this->y + yvel * ((double) elapsed / 256.0) - this->bounds.height, 1, 1);
				if(RectOverlap(tilerect, movedbounds)) {
					maxred = max(maxred, rgb.r);
				}

				data = thismap->getpixel(thismap->image, (int)(this->x + xvel * ((double) elapsed / 256.0) + this->bounds.width) % thismap->image->w, (int)(this->y + yvel * ((double) elapsed / 256.0) - this->bounds.height) % thismap->image->h);
				SDL_GetRGB(data, thismap->image->format, &rgb.r, &rgb.g, &rgb.b);
				
				movedbounds = rect(this->x + xvel * ((double) elapsed / 256.0) + this->bounds.width, this->y + yvel * ((double) elapsed / 256.0) - this->bounds.height, 1, 1);
				if(RectOverlap(tilerect, movedbounds)) {
					maxred = max(maxred, rgb.r);
				}
				
			}
			//oldz = this->z;
			if(using_heightmap) {
				heightfloor = ((maxred * thismap->magnitude));
				
			}
			
			//float zmovement = abs(z - oldz)/24 + 1;
			//if(oldz == 0) {zmovement = 1;}
			//yvel /= zmovement;
			//xvel /= zmovement;
		} else {
			update_z_time--;
			//this->z = ((oldz) + this->z) / 2 ;
			
		}
		
		layer = max(z /64, 0.0f);
		layer = min(layer, (int)g_collisions.size() - 1);
		//should we fall?
		bool should_fall = 1;
		float floor = 0;
		if(layer > 0) {
			for (auto n : g_collisions[layer - 1]) {
				rect thisMovedBounds = rect(bounds.x + x + xvel * ((double) elapsed / 256.0), bounds.y + y + yvel * ((double) elapsed / 256.0), bounds.width, bounds.height);
				if(RectOverlap(n->bounds, thisMovedBounds)) {
					floor = 64 * (layer);
					break;
				}
			}
			
			
		}
		floor = max(floor, heightfloor);
		float shadowFloor = 0;
		
		//find actual floor for shadow
		if(layer > 0) {
			bool breakflag = 0;
			for(int i = layer - 1; i >= 0; i--) {
				for (auto n : g_collisions[i]) {
					rect thisMovedBounds = rect(bounds.x + x + xvel * ((double) elapsed / 256.0), bounds.y + y + yvel * ((double) elapsed / 256.0), bounds.width, bounds.height);
					if(RectOverlap(n->bounds, thisMovedBounds)) {
						shadowFloor = 64 * (i + 1);
						breakflag = 1;
						break;
					}
				}
				if(breakflag) {break;}
			}
		}
	
		this->shadow->z = max(shadowFloor, heightfloor);
		shadow->x = x + shadow->xoffset;
		shadow->y = y + shadow->yoffset;


		if(z > floor + 1) {
			zaccel -= g_gravity;
			grounded = 0;
		} else {
			grounded = 1;
			zvel = max(zvel, 0.0f);
			zaccel = max(zaccel, 0.0f);
			
		}
		
		
		zvel += zaccel * ((double) elapsed / 256.0);
		zvel *= pow(friction, ((double) elapsed / 256.0));
		z += zvel * ((double) elapsed / 256.0);
		z = max(z, floor + 1);
		
		
		z = max(z, heightfloor);
		layer = max(z /64, 0.0f);
		layer = min(layer, (int)g_collisions.size() - 1);

		if(shooting) {
			//spawn shot.
			shoot();
		}

		return nullptr;
	}
};

class projectile : public actor {
public:
	int layer;
	float xvel;
	float yvel;

	bool asset_sharer;

	bool animate = 0;
	int frame = 0;
	int animation = 0;
	int xframes = 0;
	int frameInAnimation = 0;
	
	int curheight = 0;
	int curwidth = 0;


	float maxLifetime = 0.4;
	float lifetime = 500;
	float angle = 0;
	weapon* gun;

	projectile(weapon* fweapon) {
		M("projectile()");
		gun = fweapon;
		texture = gun->texture;
		asset_sharer = 1;
		
		width = 30;
		height = 30;
		
		animate = 0;
		curheight = 30;
		curwidth = 30;
		g_projectiles.push_back(this);
	}

	~projectile() {
		M("~projectile()");
		g_projectiles.erase(remove(g_projectiles.begin(), g_projectiles.end(), this), g_projectiles.end());
	}

	void update(float elapsed) {
		rect bounds = {x, y, width, height};
		layer = max(z /64, 0.0f);
		for(auto n : g_collisions[layer]) {
			if(RectOverlap(bounds, n->bounds)) {
				lifetime = 0;
				return;
			}
		}

		if(lifetime <= 0) {
			return;
		}
		
		x += sin(angle) * gun->sideways(maxLifetime - lifetime) + cos(angle) * gun->forward(maxLifetime - lifetime) + xvel;
		y += XtoY * (sin(angle + M_PI / 2) * gun->sideways(maxLifetime - lifetime) + cos(angle + M_PI / 2) * gun->forward(maxLifetime - lifetime) + yvel);
		lifetime -= elapsed;
	}

	void render(SDL_Renderer * renderer, camera fcamera) {
		rect obj(floor((x -fcamera.x + (width-curwidth)/2)* fcamera.zoom) , floor(((y-curheight - z * XtoZ) - fcamera.y) * fcamera.zoom), ceil(curwidth * fcamera.zoom), ceil(curheight * fcamera.zoom));		
		rect cam(0, 0, fcamera.width, fcamera.height);
		
		if(RectOverlap(obj, cam)) {

			
			SDL_Rect dstrect = { obj.x, obj.y, obj.width, obj.height};
			if(0/*framespots.size() > 0*/) {
				//frame = animation * xframes + frameInAnimation;
				//SDL_Rect dstrect = { obj.x, obj.y, obj.width, obj.height};
				//SDL_Rect srcrect = {framespots[frame]->x,framespots[frame]->y, framewidth, frameheight};
				//const SDL_Point center = {0 ,0};
				//if(texture != NULL) {
				//	SDL_RenderCopyEx(renderer, texture, &srcrect, &dstrect, 0, &center, flip);
				//}
			} else {
				if(texture != NULL) {
					SDL_RenderCopy(renderer, texture, NULL, &dstrect);
				}
			}
		}
	}
};

void entity::shoot() {
	if(this->hisWeapon->cooldown <= 0) {
		M("shoot()");
		hisWeapon->cooldown = hisWeapon->maxCooldown;
		projectile* p = new projectile(hisWeapon);
		p->x = getOriginX() - p->width/2;
		p->y = getOriginY() - p->height/2;
		p->z = z;
		p->width = hisWeapon->width;
		p->height = hisWeapon->height;
		p->gun = this->hisWeapon;
		//inherit velo from parent
		p->xvel = xvel/15;
		p->yvel = yvel/15;
		//angle
		if(up) {
			if(left) {
				p->angle = 3 * M_PI / 4;
				
			} else if (right) {
				p->angle = M_PI / 4;
			} else {
				p->angle = M_PI / 2;
				
			}
		} else if (down) {
			if(left) {
				p->angle = 5 * M_PI / 4;
				
			} else if (right) {
				p->angle = 7 * M_PI / 4;
				
			} else {
				p->angle = 3 * M_PI / 2;
				
			}
		} else {
			if(left) {
				p->angle = M_PI;
				
			} else if (right) {
				p->angle = 0;
				
			} else {
				//default
				p->angle = 3 * M_PI / 4;
				
			}
		}
	
		//move it out of the shooter and infront
		p->x += cos(p->angle) * width/2;
		p->y += cos(p->angle + M_PI / 2) * height/2;
		
	}
	
}

class ai: public entity {
public:
	bool noticedPlayer = false;
	float updateSpeed = 500;
	float timeSinceLastUpdate = 0;
	navNode* dest;
	navNode* current;
	vector<navNode*> path;
	float dijkstraSpeed = 25; //how many updates to wait between calling dijkstra's algorithm
	float timeSinceLastDijkstra = -1;
	bool pathfinding = 0;
	float maxDistanceFromHome = 1400;
	float range = 3;
	int stuckTime = 0; //time since ai is trying to go somewhere but isn't moving
	int maxStuckTime = 10; //time waited before resolving stuckness
	float lastx = 0;
	float lasty = 0;

	ai() {};
	ai(SDL_Renderer* renderer, string filename) {
		
		
		ifstream file(filename);
		string temp;
		file >> temp;
		const char* spritefile = temp.c_str(); 
		float size;
		string comment;

		file >> comment;
		file >> size;
		
		file >> comment;
		file >> this->x;
		file >> this->y;

		file >> comment;
		file >> this->width;
		file >> this->height;
		
		file >> comment;
		file >> this->xagil;
		file >> this->yagil;
		
		
		file >> comment;
		file >> this->xmaxspeed;
		file >> this->ymaxspeed;
		
		file >> comment;
		file >> this->friction;

		file >> comment;
		file >> this->bounds.width;
		file >> this->bounds.height;
		
		file >> comment;
		file >> this->bounds.x;
		file >> this->bounds.y;
		

		file >> comment;
		float fsize;
		file >> fsize;
		shadow = new cshadow(renderer, fsize);
		this->shadow->owner = this;

		file >> comment;
		file >> shadow->y;
		file >> shadow->x;
		

		
		file >> comment;
		file >> this->animspeed;
		file >> this->animlimit;
		
		file >> comment;
		file >> this->framewidth;
		file >> this->frameheight;
		
		file >> comment;
		file >> this->talks;
		file >> comment;
		file >> this->enemy;
		

		file.close();
		SDL_Surface* image = IMG_Load(spritefile);
		if(onionmode) {
			image = IMG_Load("sprites/onionmode/onion.png");
			
		}
		texture = SDL_CreateTextureFromSurface(renderer, image);

		this->width = image->w * size;
		this->height = image->h * size;
		curwidth = width;
		curheight = height;
		xframes = image->w /framewidth;
		yframes = image->h /frameheight;

		for (int j = 0; j < yframes; j++) {
			for (int i = 0; i < xframes; i++) {
				coord* a = new coord(); 
				a->x = i*framewidth;
				a->y = j * frameheight;
				framespots.push_back(a);
			}
		}
		SDL_FreeSurface(image);
		g_entities.push_back(this);
		g_ais.push_back(this);
	}


	void pursueEntity(entity* target) {
		
	}
	
	void detectEntity(entity* target) {
		
	}
	
	virtual void update(entity* protag, float elapsed ) {
		
	}
};

class chaser: public ai {
public:
	chaser(SDL_Renderer * renderer, string filename, float sizeForDefaults = 1) {
		//M("entity()" );
		range = 1.2;
		ifstream file;
		bool using_default = 0;
		this->name = filename;
		string loadstr = "entities/" + filename + ".ent";
		const char* plik = loadstr.c_str();
		
		file.open(plik);
		
		if (!file.is_open()) {
			using_default = 1;
			string newfile = "entities/default.ent";
			file.open(newfile);
		}
		
		string temp;
		file >> temp;
		string spritefilevar;
		
		if(!using_default) {
			spritefilevar = temp; 
		} else {
			spritefilevar = "sprites/" + filename + ".png";
			M(spritefilevar );
		}
		
		const char* spritefile = spritefilevar.c_str();
		float size;
		string comment;
		file >> comment;
		file >> size;

		file >> comment;
		file >> this->xagil;
		file >> this->yagil;
		
		
		file >> comment;
		file >> this->xmaxspeed;
		file >> this->ymaxspeed;
		
		file >> comment;
		file >> this->friction;
		file >> comment;
		file >> this->bounds.width;
		file >> this->bounds.height;
		
		file >> comment;
		file >> this->bounds.x;
		file >> this->bounds.y;

		file >> comment;
		float fsize;
		file >> fsize;
		shadow = new cshadow(renderer, fsize);
		
		this->shadow->owner = this;

		file >> comment;
		file >> shadow->xoffset;
		file >> shadow->yoffset;
		
		


		file >> comment;
		file >> this->animspeed;
		file >> this->animlimit;

		file >> comment;
		file >> this->defaultAnimation;
		
		file >> comment;
		file >> this->framewidth;
		file >> this->frameheight;
		this->shadow->width = framewidth * fsize;
		this->shadow->height = framewidth * fsize * (1/p_ratio);
		
	

		

		file >> comment;
		bool setcollisionfromshadow;
		file >> setcollisionfromshadow;
		

		

		file >> comment;
		file >> this->dynamic;
		
		file >> comment;
		file >> this->talks;

		file >> comment;
		file >> this->enemy;
		file.close();
		
		//load dialogue file
		if(talks) {
			
			
			//string txtfilename = filename.substr(9, filename.length() - 13);

			string txtfilename = "text/" + filename + ".txt";

			ifstream file(txtfilename);
			string line;

			//load voice
			getline(file, line);
			line = "sounds/voice-" + line +".wav";
			//voice = Mix_LoadWAV(line.c_str());

			while(getline(file, line)) {
				sayings.push_back(line);
			}
			
			
		}
		
		SDL_Surface* image = IMG_Load(spritefile);
		if(onionmode) {
			image = IMG_Load("sprites/onionmode/onion.png");
			
		}
		
		texture = SDL_CreateTextureFromSurface(renderer, image);
		this->width = size * framewidth;
		this->height = size * frameheight;

		//move shadow to feet
		shadow->xoffset += width/2 - shadow->width/2;
		shadow->yoffset -= height - shadow->height;
		

		if(setcollisionfromshadow) {
			this->bounds.width = this->shadow->width;
			this->bounds.height = this->shadow->height;
			this->bounds.x = this->shadow->xoffset;
			this->bounds.y = this->shadow->yoffset;
		}

		if(using_default) {
			int w, h;
			SDL_QueryTexture(texture, NULL, NULL, &w, &h);
			this->width = w * sizeForDefaults;
			this->height = h * sizeForDefaults;
			
		}

		//move shadow to feet
		//this->bounds.y +=this->height/2;
		curwidth = width;
		curheight = height;
		
		if(!using_default) {
			xframes = image->w /framewidth;
			yframes = image->h /frameheight;
		
	

		for (int j = 0; j < image->h; j+=frameheight) {
			for (int i = 0; i < image->w; i+= framewidth) {
				coord* a = new coord();  
				a->x = i;
				a->y = j;
				framespots.push_back(a);
			}
		}

		}
		SDL_FreeSurface(image);

		//set current
		current = this->Get_Closest_Node(g_navNodes);
		dest = this->Get_Closest_Node(g_navNodes);

		g_entities.push_back(this);
		g_ais.push_back(this);
		this->inParty = 1;
		party.push_back(this);
	}

	void update(entity* targetEntity, float elapsed) {
		if(this == protag) { this->friction = 0.2; return;}
		pathfinding = !LineTrace(x + width/2, y, targetEntity->x + targetEntity->width / 2, targetEntity->y);		
		
		if(pathfinding) {
			if(g_navNodes.size() < 1) {return;}
			if(current == nullptr) {
				//This code is hit when sight to the target is lost
				current = Get_Closest_Node(g_navNodes);
				
				dest = targetEntity->Get_Closest_Node(g_navNodes);
				//uncommenting this should result in flawless pathfinding, but will often turn around when losing sight of the player
				//with current node density
				// if(!LineTrace(x, y, dest->x, dest->y, 45)) {
				// 	dest = current;
				// 	M("Lost sight to player and dont have LOS to place last sighted" );
				// }
				
			}
			extern bool drawhitboxes;
			if(drawhitboxes) {
				current->Render(0, 0, 255);
				dest->Render(255, 0, 0);
			}

			float ydist = 0;
			float xdist = 0;
			xdist = abs(dest->x - this->x);
			ydist = abs(dest->y - this->y);
			float vect = pow((pow(xdist,2)  + pow(ydist,2)), 0.5);
			float factor = 1;
			if(vect != 0) {
				factor = 1 / vect;
			}

			xdist *= factor;
			ydist *= factor;
				
			if(dest->x > x + width/2) {
				xaccel = this->xagil * xdist;
			} else {
				xaccel = -this->xagil * xdist;
			}
			if(dest->y > y) {
				yaccel = this->yagil * ydist;
			} else {
				yaccel = -this->yagil * ydist;
				
			}
			this->friction = 0.2;
			int prog = 0;
			if(abs(dest->y - y ) < 15) {
				prog ++;
			}
			if(abs(dest->x - (x +width/2)) < 15) {
				prog ++;
			}

			//detect stuckness while following path
			D(stuckTime);
			D(lastx - x);
			D(lasty - y);
			
			if(abs(lasty - y) < 1 && abs(lastx - x) < 1) {
				stuckTime ++;
			} else {
				//why the check?
				//as below, stuckTime can be set to negative to give us time to pathfind
				if(stuckTime > 0) { stuckTime = 0;}
			}
			if(stuckTime >= maxStuckTime) {
				//pathfind to dest
				current = Get_Closest_Node(g_navNodes);
				dest = current;
				timeSinceLastDijkstra = -1;
				//give time before deciding we are stuck again
				stuckTime = -99;
				//SDL_Delay(1000);
					
			} else {
				stuckTime++;
			}
			lastx = x;
			lasty = y;

			if(prog == 2) {
				if( path.size() > 0) {
					//take next node in path
					current = dest;
					dest = path.at(path.size() - 1);
					path.erase(path.begin() + path.size()-1);
				} else {
					//path completed
				}
			}

			if(timeSinceLastDijkstra < 0) {
			if(dest != nullptr) {
				current = dest;
			}
			current = dest;
			//do Dijkstra's algorithm on target and update path
			timeSinceLastDijkstra = dijkstraSpeed;
			M("Dijkstra()" );
			navNode* target = targetEntity->Get_Closest_Node(g_navNodes);
			vector<navNode*> bag;
			for (int i = 0; i < g_navNodes.size(); i++) {
				bag.push_back(g_navNodes[i]);
				g_navNodes[i]->costFromSource = numeric_limits<float>::max();
			}
			current->costFromSource = 0;
			int overflow = 300;
			
			while(bag.size() > 0) {
				overflow --;
				if(overflow < 0) { break; }
				
				navNode* u;
				float min_dist = numeric_limits<float>::max();
				for (int i = 0; i < bag.size(); i++) {
					if(bag[i]->costFromSource < min_dist){
						u = bag[i];
						min_dist = u->costFromSource;
					}
				}
				//u is closest node in bag
				bag.erase(remove(bag.begin(), bag.end(), u), bag.end());
				for (int i = 0; i < u->friends.size(); i++) {
					float alt = u->costFromSource + u->costs[i];
					if(alt < u->friends[i]->costFromSource) {
						u->friends[i]->costFromSource = alt;
						u->friends[i]->prev = u;
					}
				}
			}
			path.clear();
			while(target != nullptr) {
				path.push_back(target);
				if(target == current) {
					break;
				}
				target = target->prev;
			}
			
			dest = path.at(path.size() - 1);
		} else {
			timeSinceLastDijkstra --;
		}
			

			bool isDestFriendofCurrent = 0;
			for (auto x: current->friends) {
				if(x == dest) {
					isDestFriendofCurrent = 1;
				}
			}
			//D(isDestFriendofCurrent);
		
		} else {
			//has los to player, just pursue
			//clear current and dest
			current = nullptr;
			dest = nullptr;
			float ydist = 0;
			float xdist = 0;
			xdist = abs((targetEntity->x - (targetEntity->width / 2)) - this->x);
			ydist = abs(targetEntity->y - this->y);
			float vect = pow((pow(xdist,2)  + pow(ydist,2)), 0.5);
			float factor = 1;
			if(vect != 0) {
				factor = 1 / vect;
			}

			xdist *= factor;
			ydist *= factor;

			if(targetEntity->x + targetEntity->width/2 > x + width/2) {
				xaccel = this->xagil * xdist;
			} else {
				xaccel = -this->xagil * xdist;
			}
			if(targetEntity->y > y) {
				yaccel = this->yagil * ydist;

			} else {
				yaccel = -this->yagil * ydist;	
			}

			

			if(abs(xvel) + abs(yvel) > 10) {
				hadInput = 1;
				up = 0;
				down = 0;
				left = 0;
				right = 0;

				if(y > targetEntity->y && y - targetEntity->y > 30) {
					up = 1;
				} else {
					down = 1;
				}
				

				if(x > targetEntity->x && x - targetEntity->x > 30) {
					left = 1;
				} else {
					right = 1;
				}
				
				
			}
			this->friction = 0.2;
			int prog = 0;
			//check if within range with ellipse
			range = (this->width + targetEntity->width) / 100;
			float rhs = pow(((targetEntity->x +targetEntity->width/2) - (x + width/2)), 2)/pow((range * 64), 2) + pow((targetEntity->y - y), 2) / pow((range * 45), 2);
			if(1 >= rhs) {
				this->friction = 0.01;
				stop_verti();
				stop_hori();
				hadInput = 0;	
			} else {
				//found player, start attack etc
			}
			
		}
	}

};


void cshadow::render(SDL_Renderer * renderer, camera fcamera) {
	SDL_Rect dstrect = { ((this->x)-fcamera.x) *fcamera.zoom, (( (this->y - (XtoZ * z) ) ) -fcamera.y) *fcamera.zoom, (width * size), (height * size)* (637 /640) * 0.9};
	D(x);
	D(y);
	D(xoffset);
	D(yoffset);
	//SDL_Rect dstrect = {500, 500, 200, 200 };
	//dstrect.y += (owner->height -(height/2)) * fcamera.zoom;
	float temp;
	temp = width;
	temp*= fcamera.zoom;
	dstrect.w = temp;
	temp = height;
	temp*= fcamera.zoom;
	dstrect.h = temp;
	
	rect obj(dstrect.x, dstrect.y, dstrect.w, dstrect.h);

	rect cam(0, 0, fcamera.width, fcamera.height);

	//if(RectOverlap(obj, cam)) {
		SDL_RenderCopy(renderer, texture, NULL, &dstrect);	
	//}
}

class textbox {
public:
	SDL_Surface* textsurface = 0;
	SDL_Texture* texttexture = 0;
	SDL_Color textcolor = { 255, 255, 255 };
	SDL_Rect thisrect = {0, 0, 50, 50};
	string content = "Default text.";
	TTF_Font* font = 0;
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	bool show = true;

	int errorflag = 0;

	//used for drawing in worldspace
	float boxWidth = 50;
	float boxHeight = 50;
	float boxX = 0;
	float boxY = 0;

	float boxScale = 40;
	bool worldspace = false; //use worldspace or screenspace;

	textbox(SDL_Renderer* renderer, const char* fcontent, float size, int fx, int fy, int fwidth) {
		M("textbox()" );
		if(font != NULL) {
			TTF_CloseFont(font);
		}
		font = TTF_OpenFont(g_font.c_str(), size);
		content = fcontent;

		textsurface =  TTF_RenderText_Blended_Wrapped(font, content.c_str(), textcolor, fwidth * WIN_WIDTH);
		texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);

		int texW = 0;
		int texH = 0;
		x = fx;
		y = fy;
		SDL_QueryTexture(texttexture, NULL, NULL, &texW, &texH);
		this->width = texW;
		this->height = texH;
		thisrect = { fx, fy, texW, texH };

		g_textboxes.push_back(this);
	}
	~textbox() {
		M("~textbox()" );
		g_textboxes.erase(remove(g_textboxes.begin(), g_textboxes.end(), this), g_textboxes.end());
		TTF_CloseFont(font); 
		SDL_DestroyTexture(texttexture);
		SDL_FreeSurface(textsurface);
	}
	void render(SDL_Renderer* renderer, int winwidth, int winheight) {
		if(show) {
			if(worldspace) {
				
				SDL_Rect dstrect = {boxX * winwidth, boxY * winheight, width,  thisrect.h};
				SDL_RenderCopy(renderer, texttexture, NULL, &dstrect);
				
			} else {
				SDL_RenderCopy(renderer, texttexture, NULL, &thisrect);

			}
		}
	}
	void updateText(string content, int size, float fwidth) {
		TTF_CloseFont(font); 
		SDL_DestroyTexture(texttexture);
		SDL_FreeSurface(textsurface);
		font = TTF_OpenFont(g_font.c_str(), size);
		textsurface =  TTF_RenderText_Blended_Wrapped(font, content.c_str(), textcolor, fwidth * WIN_WIDTH);
		texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);
		int texW = 0;
		int texH = 0;
		SDL_QueryTexture(texttexture, NULL, NULL, &texW, &texH);
		width = texW;
		thisrect = { x, y, texW, texH };
		
	}
};

class ui {
public:
	float x;
	float y;
	float xagil;
	float yagil;
	float xaccel;
	float yaccel;
	float xvel;
	float yvel;
	float xmaxspeed;
	float ymaxspeed;
	float width = 0.5;
	float height = 0.5;
	float friction;
	bool show = true;
	SDL_Surface* image;
	SDL_Texture* texture;
	
	//for 9patch
	bool is9patch = 0;
	int patchwidth = 256; //213
	float patchscale = 0.4;


	ui(SDL_Renderer * renderer, const char* filename, float size) {
		M("ui()" );
		image = IMG_Load(filename);
		width = image->w * size;
		height = image->h * size;
		texture = SDL_CreateTextureFromSurface(renderer, image);
		g_ui.push_back(this);
		
	}

	~ui() {
		M("~ui()" );
		SDL_DestroyTexture(texture);
		SDL_FreeSurface(image);
		
	}

	void render(SDL_Renderer * renderer, camera fcamera, int winwidth, int winheight) {
		if(this->show) {
			if(is9patch) {
				if(winwidth != 0) {
					patchscale = winwidth;
					patchscale /= 4000;
				}
				int ibound = width * winwidth;
				int jbound = height * winheight;
				int scaledpatchwidth = patchwidth * patchscale;
				int i = 0; 
				while (i < ibound) {
					int j = 0;
					while (j < jbound) {
						SDL_Rect dstrect = {i + (x * winwidth), j + (y * winheight), scaledpatchwidth, scaledpatchwidth}; //change patchwidth in this declaration for sprite scale
						SDL_Rect srcrect;
						srcrect.h = patchwidth;
						srcrect.w = patchwidth;
						if(i==0) {
							srcrect.x = 0;

						} else {
						if(i + scaledpatchwidth >= ibound) {
							srcrect.x = 2 * patchwidth;
						}else {
							srcrect.x = patchwidth;
						}}
						if(j==0) {
							srcrect.y = 0;
						} else {
						if(j + scaledpatchwidth >= jbound) {
							srcrect.y = 2 * patchwidth;
						}else {
							srcrect.y = patchwidth;
						}}

						//shrink the last non-border tile to fit well.
						int newheight = jbound - (j + scaledpatchwidth); 
						if(j + (2 * scaledpatchwidth) >= jbound && newheight > 0) {
							
							dstrect.h = newheight;
							j+=  newheight;
						} else {
							j+= scaledpatchwidth;
						}

						int newwidth = ibound - (i + scaledpatchwidth); 
						if(i + (2 * scaledpatchwidth) >= ibound && newwidth > 0) {
							
							dstrect.w = newwidth;
						} else {
						}

						//done to fix occasional 1px gap. not a good fix
						dstrect.h += 1;
						SDL_RenderCopy(renderer, texture, &srcrect, &dstrect);
						
						
						
					}	
					//increment i based on last shrink
					int newwidth = ibound - (i + scaledpatchwidth); 
					if(i + (2 * scaledpatchwidth) >= ibound && newwidth > 0) {
						i+=  newwidth;
					} else {
						i+= scaledpatchwidth;
					}
				}
				
			} else {
				SDL_Rect dstrect = {x * winwidth, y * winheight, width * winwidth, height * winheight};
				SDL_RenderCopy(renderer, texture, NULL, &dstrect);
			}
		}
	}

	virtual void update_movement(vector<collision*> collisions, float elapsed) {
		if(xaccel > 0 && xvel < xmaxspeed) {
			xvel += xaccel * ((double) elapsed / 256.0);
		}
		
		if(xaccel < 0 && xvel > -1 * xmaxspeed) {
			xvel += xaccel * ((double) elapsed / 256.0);
		}

		if(yaccel > 0 && yvel < ymaxspeed) {
			yvel += yaccel* ((double) elapsed / 256.0);
		}
		
		if(yaccel < 0 && yvel > -1 * ymaxspeed) {
			yvel += yaccel* ((double) elapsed / 256.0);
		}

		rect movedbounds;
		bool ycollide = 0;
		bool xcollide = 0;
		y+= yvel * ((double) elapsed / 256.0);
		x+= xvel * ((double) elapsed / 256.0);
	
	}
};



class worldsound {
public:
	//a playable sound in the world, with a position
	Mix_Chunk* blip;
	float volumeFactor = 1;
	float maxDistance = 1200; //distance at which you can no longer hear the sound
	float minWait = 1;
	float maxWait = 5;

	string name;
	int x = 0;
	int y = 0;
	float cooldown = 0;
	
	worldsound(string filename, int fx, int fy) {
		name = filename;
		M("worldsound()" );
		
		ifstream file;

		string loadstr;
		//try to open from local map folder first
		
		loadstr = "maps/" + g_map + "/" + filename + ".ws";
		D(loadstr);
		const char* plik = loadstr.c_str();
		
		file.open(plik);
		
		if (!file.is_open()) {
			loadstr = "worldsounds/" + filename + ".ws";
			const char* plik = loadstr.c_str();
			
			file.open(plik);
			
			if (!file.is_open()) {
				string newfile = "worldsounds/default.ws";
				file.open(newfile);
			}
		}
		
		string temp;
		file >> temp;
		temp = "sounds/" + temp + ".wav";
		
		blip = Mix_LoadWAV(temp.c_str());
		
		
		x = fx;
		y = fy;

		float tempFloat;
		file >> tempFloat;
		volumeFactor = tempFloat;

		file >> tempFloat;
		maxDistance = tempFloat;

		file >> tempFloat;
		minWait = tempFloat * 1000;
		
		file >> tempFloat;
		maxWait = tempFloat * 1000;
		D(tempFloat);
		g_worldsounds.push_back(this);
	}

	~worldsound() {
		M("~worldsound()" );
		Mix_FreeChunk(blip);
		g_worldsounds.erase(remove(g_worldsounds.begin(), g_worldsounds.end(), this), g_worldsounds.end());
	}

	void update(float elapsed) {
		float dist = Distance(x, y, g_camera.x + g_camera.width/2, g_camera.y + g_camera.height/2);
		
		//linear
		float cur_volume = ((maxDistance - dist)/maxDistance) * volumeFactor * 128;
		
		//logarithmic
		//float cur_volume = volume * 128 * (-log(pow(dist, 1/max_distance) + 2.718));
		//float cur_volume = (volume / (dist / max_distance)) * 128;
		

		if(cur_volume < 0) {
			cur_volume = 0;
		}
		Mix_VolumeChunk(blip, cur_volume);
		
		if(cooldown < 0) {
			//change volume
			
			
			
			playSound(-1, blip, 0);
			cooldown = rand() % (int)maxWait + minWait;
		} else {
			cooldown -= elapsed;
		}
	}
};

class musicNode {
public:
	Mix_Music* blip;
	string name = "empty";
	int x = 0;
	int y = 0;
	float radius = 1200;

	musicNode(string fileaddress, int fx, int fy) {
		name = fileaddress;
		fileaddress = "music/" + fileaddress + ".wav";
		blip = Mix_LoadMUS(fileaddress.c_str());
		x = fx;
		y = fy;
		g_musicNodes.push_back(this);
	}
	~musicNode() {
		Mix_FreeMusic(blip);
		g_musicNodes.erase(remove(g_musicNodes.begin(), g_musicNodes.end(), this), g_musicNodes.end());
	}
};

class cueSound {
public:
	Mix_Chunk* blip;
	string name = "empty";
	int x = 0;
	int y = 0;
	float radius = 1200;
	bool played = 0;
	cueSound(string fileaddress, int fx, int fy, int fradius) {
		name = fileaddress;
		fileaddress = "sounds/" + fileaddress + ".wav";
		blip = Mix_LoadWAV(fileaddress.c_str());
		x = fx;
		y = fy;
		radius = fradius;
		g_cueSounds.push_back(this);
	}
	~cueSound() {
		Mix_FreeChunk(blip);
		g_cueSounds.erase(remove(g_cueSounds.begin(), g_cueSounds.end(), this), g_cueSounds.end());
	}
};

class waypoint {
public:
	int x = 0;
	int y = 0;
	string name;
	waypoint(string fname, int fx, int fy) {
		name = fname;
		x = fx;
		y = fy;
		g_waypoints.push_back(this);
	}
	~waypoint() {
		g_waypoints.erase(remove(g_waypoints.begin(), g_waypoints.end(), this), g_waypoints.end());
	}
};

class trigger {
public:
	int x, y, width, height;
	string binding;
	vector<string> script;
	bool active = 1;
	trigger(string fbinding, int fx, int fy, int fwidth, int fheight) {
		x = fx;
		y = fy;
		width = fwidth;
		height = fheight;
		binding = fbinding;
		g_triggers.push_back(this);
		//open and read from the script file
		ifstream stream;
		string loadstr;
		//try to open from local map folder first
		
		loadstr = "maps/" + g_map + "/" + fbinding + ".event";
		D(loadstr);
		const char* plik = loadstr.c_str();
		
		stream.open(plik);
		
		if (!stream.is_open()) {
			stream.open("events/" + fbinding + ".event");
		}
		string line;
		
		while (getline(stream, line)) {
			script.push_back(line);
		}
		
		for(string x: script) {
			M(x);
		}
	
	}

	~trigger() {
		g_triggers.erase(remove(g_triggers.begin(), g_triggers.end(), this), g_triggers.end());
	}
};

class listener {
public:
	int x, y; //just for debugging
	string binding;
	int block = 0;
	int condition = 1;
	string entityName;
	vector<entity*> listenList;
	bool oneWay = 0; //does this become inactive after "hearing" the entity data once?

	vector<string> script;
	bool active = 1;
	listener(string fname, int fblock, int fcondition, string fbinding, int fx, int fy) {
		x = fx;
		y = fy;
		binding = fbinding;
		entityName = fname;
		block = fblock;
		condition = fcondition;

		g_listeners.push_back(this);
		//open and read from the script file
		ifstream stream;
		string loadstr;
		//try to open from local map folder first
		
		loadstr = "maps/" + g_map + "/" + fbinding + ".event";
		const char* plik = loadstr.c_str();
		
		stream.open(plik);
		
		if (!stream.is_open()) {
			stream.open("events/" + fbinding + ".event");
		}
		string line;
		
		while (getline(stream, line)) {
			script.push_back(line);
		}
		
		//build listenList from current entities
		for(auto x : g_entities) {
			if(x->name == entityName) {
				listenList.push_back(x);
			}
		}
	
	}

	~listener() {
		g_listeners.erase(remove(g_listeners.begin(), g_listeners.end(), this), g_listeners.end());
	}

	int update() {
		if (!active) {return 0;}
		for(auto x : listenList) {
			if(x->data[block] == condition) {
				//do nothing
			} else {
				return 0;
			}
		}
		if(oneWay) {active = 0;}
		return 1;
	}
};

void clear_map(camera& cameraToReset) {
	
	Mix_FadeOutMusic(1000);
	{
		//turn off VSYNC because otherwise we jitter between this frame and the last throughout the animation
		SDL_GL_SetSwapInterval(0);
		bool cont = false;
		float ticks = 0;
		float lastticks = 0;
		float transitionElapsed = 5;
		float mframes = 60;
		float transitionMinFrametime = 5;
		transitionMinFrametime = 1/mframes * 1000;
		
		
		SDL_Surface* transitionSurface = IMG_Load("tiles/engine/transition.png");

		int imageWidth = transitionSurface->w;
		int imageHeight = transitionSurface->h;

		SDL_Texture* transitionTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, transitionSurface->w, transitionSurface->h );
		SDL_SetTextureBlendMode(transitionTexture, SDL_BLENDMODE_BLEND);


		void* pixelReference;
		int pitch;

		float offset = imageHeight;
		
		while (!cont) {
			
			//onframe things
			SDL_LockTexture(transitionTexture, NULL, &pixelReference, &pitch);
			
			memcpy( pixelReference, transitionSurface->pixels, transitionSurface->pitch * transitionSurface->h);
			Uint32 format = SDL_PIXELFORMAT_ARGB8888;
			SDL_PixelFormat* mappingFormat = SDL_AllocFormat( format );
			Uint32* pixels = (Uint32*)pixelReference;
			int numPixels = imageWidth * imageHeight;
			Uint32 transparent = SDL_MapRGBA( mappingFormat, 0, 0, 0, 255);
			//Uint32 halftone = SDL_MapRGBA( mappingFormat, 50, 50, 50, 128);

			offset += g_transitionSpeed + 0.02 * offset;
			
			for(int x = 0;  x < imageWidth; x++) {
				for(int y = 0; y < imageHeight; y++) {


					int dest = (y * imageWidth) + x;
					int src =  (y * imageWidth) + x;
					
					if(pow(pow(imageWidth/2 - x,2) + pow(imageHeight + y,2),0.5) < offset) {
						pixels[dest] = transparent;
					} else {
						// if(pow(pow(imageWidth/2 - x,2) + pow(imageHeight + y,2),0.5) < 10 + offset) {
						// 	pixels[dest] = halftone;
						// } else {
							pixels[dest] = 0;
						// }
					}
					
				}
			}

	
			


			ticks = SDL_GetTicks();
			transitionElapsed = ticks - lastticks;
			//lock framerate
			if(transitionElapsed < transitionMinFrametime) {
				SDL_Delay(transitionMinFrametime - transitionElapsed);
				ticks = SDL_GetTicks();
				transitionElapsed = ticks - lastticks;
				
			}	
			lastticks = ticks;

			SDL_UnlockTexture(transitionTexture);
			SDL_RenderCopy(renderer, transitionTexture, NULL, NULL);

			SDL_RenderPresent(renderer);

			if(offset > imageHeight + pow(pow(imageWidth/2,2) + pow(imageHeight,2),0.5)) {
				cont = 1;
			}
		}
		SDL_FreeSurface(transitionSurface);
		SDL_DestroyTexture(transitionTexture);
		transition = 1;
		SDL_GL_SetSwapInterval(1);
	}


	cameraToReset.resetCamera();
	int size;
	size = g_entities.size();

	g_actors.clear();

	//copy protag to a pointer, clear the array, and re-add protag
	entity* hold_protag;
	D(protag->inParty);
	for(int i=0; i< size; i++) {
		if(g_entities[0]->inParty) {
			//remove from array without deleting
			g_entities.erase(remove(g_entities.begin(), g_entities.end(), g_entities[0]), g_entities.end());
		} else {
			delete g_entities[0];
		}
	}
	//push back any entities that were in the party
	for (int i = 0; i < party.size(); i++) {
		g_entities.push_back(party[i]);
		g_actors.push_back(party[i]);
	}
	
	size = g_mapObjects.size();
	for(int i = 0; i < size; i++) {
		delete g_mapObjects[0];
	}
	
	

	size = g_tiles.size();
	for(int i = 0; i < size; i++) {
		delete g_tiles[0];
	}

	size = g_navNodes.size();
	for(int i = 0; i < size; i++) {
		delete g_navNodes[0];
	}

	size = g_worldsounds.size();
	for(int i = 0; i < size; i++) {
		delete g_worldsounds[0];
	}

	size = g_musicNodes.size();
	for(int i = 0; i < size; i++) {
		delete g_musicNodes[0];
	}

	size = g_cueSounds.size();
	for(int i = 0; i < size; i++) {
		delete g_cueSounds[0];
	}

	size = g_waypoints.size();
	for(int i = 0; i < size; i++) {
		delete g_waypoints[0];
	}

	size = g_doors.size();
	for(int i = 0; i < size; i++) {
		delete g_doors[0];
	}

	size = g_triggers.size();
	for(int i = 0; i < size; i++) {
		delete g_triggers[0];
	}

	size = g_heightmaps.size();
	for(int i = 0; i < size; i++) {
		delete g_heightmaps[0];
	}

	size = g_listeners.size();
	for(int i = 0; i < size; i++) {
		delete g_listeners[0];
	}
	
	g_ais.clear();
	g_projectiles.clear();
	
	for (int j = 0; j < g_collisions.size(); j++) {
		size = g_collisions[j].size();
		for(int i = 0; i < size; i++) {
			delete g_collisions[j][0];
		}
	}
	
	size = g_triangles.size();
	for(int i = 0; i < size; i++) {
		delete g_triangles[0];

	}

}


//inventory


#endif