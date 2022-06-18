#ifndef objects_h
#define objects_h

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

#include "globals.h"
#include "lightcookietesting.h"

#include <utility>

#define PI 3.14159265

using namespace std;

void parseScriptForLabels(vector<string> &sayings) {
	//parse sayings for lables
	vector<pair<string, int>> symboltable;
	for(int i = 0; i < (int)sayings.size(); i++) {
		if(sayings[i][0] == '<') {
			pair<string, int> pushMeBack{ sayings[i].substr(1,sayings[i].length() - 2), i };
			symboltable.push_back(pushMeBack);
		}
	}

	for(int i = 0; i < (int)sayings.size(); i++) {
		int pos = sayings[i].find(":");
		if(pos != (int)string::npos) {
			for(auto y: symboltable) {
				D(sayings[i].substr(pos+1, sayings[i].length()-(pos+1)));
				if(sayings[i].substr(pos+1, sayings[i].length()-(pos+1)) == y.first) {
					
					D(sayings[i]);
					//sayings[i].erase(pos, sayings[i].length() - pos - 1);
					sayings[i].replace(pos, y.first.length() + 1, ":" + to_string(y.second + 3) );
					D(sayings[i]);
				}
			}
		}
	}
}

class heightmap {
public:
	SDL_Surface* image = 0;
	string name;
	string binding;
	float magnitude = 0.278; //0.278 was a former value. I'd love to expose this value but I cant think of a good way

	heightmap(string fname, string fbinding, float fmagnitude) {
		image = IMG_Load(fbinding.c_str());
		name = fname;
		binding = fbinding;
		
		magnitude = fmagnitude;
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
			//break;

		case 2:
			return *(Uint16 *)p;
			//break;

		case 3:
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
				return p[0] << 16 | p[1] << 8 | p[2];
			else
				return p[0] | p[1] << 8 | p[2] << 16;
				//break;

			case 4:
				return *(Uint32 *)p;
				//break;

			default:
				return 0;
		}
	}
};

class navNode {
public:
	int x;
	int y;
	int z = 0;
	vector<navNode*> friends;
	vector<float> costs;
	float costFromSource = 0; //updated with dijkstras algorithm
	navNode* prev = nullptr; //updated with dijkstras algorithm
	string name = "";
	bool enabled = 1; //closing doors can disable navNodes, so that entities will try to find another way

	navNode(int fx, int fy, int fz) {
		M("navNode()" );
		x = fx;
		y = fy;
		z = fz;
		g_navNodes.push_back(this);
		pair<int, int> pos;
		pos.first = fx; pos.second = fy;
		navNodeMap[pos] = this; 
	}

	void Add_Friend(navNode* newFriend) {
		friends.push_back(newFriend);
		float cost = pow(pow((newFriend->x - this->x), 2) + pow((newFriend->y - this->y), 2), 0.5);
		costs.push_back(cost);
	}

	void Update_Costs() {
		for (int i = 0; i < (int)friends.size(); i++) {
			costs[i] = XYWorldDistance(x, y, friends[i]->x, friends[i]->y);
		}
	}

	void Render(int red, int green, int blue) {
		SDL_Rect obj = {(int)((this->x -g_camera.x - 20)* g_camera.zoom) , (int)(((this->y - g_camera.y - 20) * g_camera.zoom)), (int)((40 * g_camera.zoom)), (int)((40 * g_camera.zoom))};
		SDL_SetTextureColorMod(nodeDebug, red, green, blue);
		SDL_RenderCopy(renderer, nodeDebug, NULL, &obj);
	}
	
	~navNode() {
		//M("~navNode()");
		//D(this);
		for (auto x : friends) {
			x->friends.erase(remove(x->friends.begin(), x->friends.end(), this), x->friends.end());
		}
		//M("got here");
		//changed if to while to try and solve a bug
		// !!! it's a crutch, find a way to remove it later
		while(count(g_navNodes.begin(), g_navNodes.end(), this)) {
			g_navNodes.erase(remove(g_navNodes.begin(), g_navNodes.end(), this), g_navNodes.end());
		}
	}
};

navNode* getNodeByPosition(int fx, int fy) {
	//narrow down our search signifigantly

	//auto p = navNodeMap.equal_range(make_pair(fx, fy));
	//think of the below numbers as the maximum distance, in worldpixels
	//that the player, or any target-entity can be from the proper node
	//that they really are closest to.
	//it has to be "caught" in this "search" for a lower bound
	//otherwise, the entity will go somewhere diagonal that was caught
	int MaxDistanceFromNode = 3;
	auto lowerbound = navNodeMap.lower_bound(make_pair(fx - (64 * MaxDistanceFromNode), fy-(45 * MaxDistanceFromNode)));
	auto upperbound = navNodeMap.upper_bound(make_pair(fx+(64 * MaxDistanceFromNode), fy+(45 * MaxDistanceFromNode)));
	//return lowerbound->second;
	float min_dist = 0;
	navNode* ret = lowerbound->second;
	bool flag = 1;

	for(auto q = lowerbound; q != upperbound; q++) {
		float dist = Distance(fx, fy, q->second->x, q->second->y);
		if( (dist < min_dist || flag) && q->second->enabled) {
			min_dist = dist;
			ret = q->second;
			flag = 0;
		}
	}

	//if every close node was disabled, I guess just take one of the disabled one :S
	if(flag) {
		for(auto q = lowerbound; q != upperbound; q++) {
			float dist = Distance(fx, fy, q->second->x, q->second->y);
			if( (dist < min_dist || flag) /*&& q->second->enabled*/) {
				min_dist = dist;
				ret = q->second;
				flag = 0;
			}
		}
	}

	return ret;
}

void RecursiveNavNodeDelete(navNode* a) {
	//copy friends array to new vector
	vector<navNode*> buffer;
	for (auto f : a->friends) {
		buffer.push_back(f);
	}
	
	//navNode* b = a->friends[0];
	delete a;
	for(auto f : buffer) {
		if(count(g_navNodes.begin(), g_navNodes.end(), f)) {
			RecursiveNavNodeDelete(f);
		}
	}
}

void Update_NavNode_Costs(vector<navNode*> fnodes) {
	M("Update_NavNode_Costs()" );
	for (int i = 0; i < (int)fnodes.size(); i++) {
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
	int z=0; //rarely used
	int width;
	int height;
	int zeight = 32;


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

	rect(int fx, int fy, int fz, int fw, int fh, int fzh) {
		x=fx;
		y=fy;
		width=fw;
		height=fh;
		z = fz;
		zeight = fzh;
	}

	void render(SDL_Renderer * renderer) {
		SDL_Rect rect = { this->x, this->y, this->width, this->height};
		SDL_RenderFillRect(renderer, &rect);
	}
};

bool LineTrace(int x1, int y1, int x2, int y2, bool display, int size, int layer, int resolution, bool visibility);

class pointOfInterest {
public:
	int x = 0;
	int y = 0;
	int index = 0;

	pointOfInterest(int fx, int fy, int findex) : x(fx), y(fy), index(findex) {
		if(findex > g_numberOfInterestSets) {index = 0;}
		g_setsOfInterest.at(index).push_back(this);	
		
	}

	//probably best to not call this when unloading a level
	~pointOfInterest() {
		g_setsOfInterest.at(index).erase(remove(g_setsOfInterest.at(index).begin(), g_setsOfInterest.at(index).end(), this), g_setsOfInterest.at(index).end());
	}
};

class mapCollision {
public:
	//related to saving/displaying the block
	string walltexture;
	string captexture;
	bool capped = false;
	bool hidden = 0; // 1 when this object is hidden behind something and doesn't need to be drawn. used for triangular walls

	//tiles created from the mapCollision, to be appropriately deleted
	vector<mapObject*> children;

	//tri and boxes which are part of the map are pushed back on 
	//an array of mapCollisions to be kept track of for deletion/undoing
	mapCollision() {
		M("mapCollision()");
		g_mapCollisions.push_back(this);
	}

	//copy constructor
	mapCollision(const mapCollision & other) {
		this->walltexture = other.walltexture;
		this->captexture = other.captexture;
		this->capped = other.capped;
		this->children = other.children;
		g_mapCollisions.push_back(this);
	}
	
	// //move constructor
	// mapCollision(mapCollision && other) {
	// 	this->walltexture = other.walltexture;
	// 	this->captexture = other.captexture;
	// 	this->capped = other.capped;
	// 	this->children = other.children;
	// }

	//copy assignment
	mapCollision& operator=(const mapCollision &other) {
		mapCollision*a;
		a->walltexture = other.walltexture;
		a->captexture = other.captexture;
		a->capped = other.capped;
		a->children = other.children;
		g_mapCollisions.push_back(a);
		return *a;
	}

	// //move assignment
	// mapCollision& operator=(mapCollision &&) {
	// 	this->walltexture = other.walltexture;
	// 	this->captexture = other.captexture;
	// 	this->capped = other.capped;
	// 	this->children = other.children;
	// }

	virtual ~mapCollision() {
		M("~mapCollision()");

		
		g_mapCollisions.erase(remove(g_mapCollisions.begin(), g_mapCollisions.end(), this), g_mapCollisions.end());
		
		//children.clear();
	}
	
};

class tri:public mapCollision {
public:
	int x1; int y1;
	int x2; int y2;
	int type;
	float m; //slope
	int b; //offset
	int layer = 0;
	bool shaded = 0;

	int x;
	int y;
	int width;
	int height;

	tri(int fx1, int fy1, int fx2, int fy2, int flayer, string fwallt, string fcapt, bool fcapped, bool fshaded) {
		M("tri()");
		x1=fx1; y1=fy1;
		x2=fx2; y2=fy2;
		layer = flayer;
		shaded = fshaded;
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
		walltexture = fwallt;
		captexture = fcapt;
		capped = fcapped;

		x = min(x1, x2);
		y = min(y1, y2);
		width = abs(x1 - x2);
		height = abs(y1 - y2);

		g_triangles[layer].push_back(this);
	}

	~tri() {
		M("~tri()");
		g_triangles[layer].erase(remove(g_triangles[layer].begin(), g_triangles[layer].end(), this), g_triangles[layer].end());
	}

	void render(SDL_Renderer* renderer) {

		int tx1 = g_camera.zoom * (x1-g_camera.x);
		int tx2 = g_camera.zoom * (x2-g_camera.x);
		

		int ty1 = g_camera.zoom * (y1-g_camera.y)- layer * 38;
		int ty2 = g_camera.zoom * (y2-g_camera.y)- layer * 38;
		
		
		SDL_RenderDrawLine(renderer,  tx1, ty1, tx2, ty2);
		SDL_RenderDrawLine(renderer,  tx1, ty1, tx2, ty1);
		SDL_RenderDrawLine(renderer,  tx2, ty2, tx2, ty1);

		
	}
};

//sortingfunction for optimizing fog and triangular walls
//sort based on x and y
inline int trisort(tri* one, tri* two) {
	return one->x < two->x || (one->x==two->x && one->y < two->y);
}

class ramp : public mapCollision {
public:
	int x, y;
	// int width = 64;
	// int height = 55;
	int layer = 0;
	int type; //0 means the higher end is north, 1 is east, and so on

	ramp(int fx, int fy, int flayer, int ftype, string fwallt, string fcapt) {
		x = fx;
		y = fy;
		layer = flayer;
		type = ftype;
		walltexture = fwallt;
		captexture = fcapt;
		g_ramps[layer].push_back(this);
	}

	~ramp() {
		g_ramps[layer].erase(remove(g_ramps[layer].begin(), g_ramps[layer].end(), this), g_ramps[layer].end());
	}
};


bool PointInsideRightTriangle(tri* t, int px, int py) {
	switch(t->type) {
		case(0):
			if(px >= t->x2 && py >= t->y1 && py < floor(t->m * px)  + t->b) {
				
				
				return true;
			}
			break;

		case(1):
			if(px >= t->x2 && py <= t->y1 && py > ceil(t->m * px) + t->b) {
				
				
				return true;
			}
			break;

		case(2):
			if(px <= t->x2 && py <= t->y1 && py > ceil(t->m * px) + t->b) {
				
				
				return true;
			}
			break;

		case(3):
			if(px <= t->x2 && py >= t->y1 && py < floor(t->m * px)  + t->b) {
				
				
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

bool RectOverlap3d(rect a, rect b) {
	return ( RectOverlap(a, b) && a.z < b.z + b.zeight && a.z + a.zeight > b.z);
}

bool ElipseOverlap(rect a, rect b) {
	//get midpoints
	coord midpointA = {a.x + a.width/2, a.y + a.height/2};
	coord midpointB = {b.x + b.width/2, b.y + b.height/2};
	
	//"unfold" crumpled y axis by multiplying by 1/XtoY
	midpointA.y *= 1/XtoY;
	midpointB.y *= 1/XtoY;

	//circle collision test using widths of rectangles as radiusen
	return (Distance(midpointA.x, midpointA.y, midpointB.x, midpointB.y) < (a.width + b.width)/2);
}
bool CylinderOverlap(rect a, rect b, int skin = 0) {

	//do an elipsecheck - then make sure the heights match up
	return (ElipseOverlap(a, b)) && ( (a.z >= b.z && a.z <= b.z + b.zeight) || (b.z >= a.z && b.z <= a.z + a.zeight));
}

bool RectOverlap(SDL_Rect a, SDL_Rect b) {
	if (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y) {
		return true;
	} else {
		return false;
	}
}

bool RectOverlap(SDL_FRect a, SDL_FRect b) {
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

bool TriRectOverlap(tri* a, int x, int y, int width, int height) {
	if(PointInsideRightTriangle(a, x, y +  height)) {
		return 1;
	}
	if(PointInsideRightTriangle(a, x + width, y + height)) {
		return 1;
	}
	if(PointInsideRightTriangle(a, x + width, y)) {
		return 1;
	}
	if(PointInsideRightTriangle(a, x, y)) {
		return 1;
	}
	//also check if the points of the triangle are inside the rectangle
	//skin usage is possibly redundant here
	if(a->x1 > x + 0 && a->x1 < x + width - 0 && a->y1 > y + 0 && a->y1 < y + height - 0) {
		return 1;
	}
	if(a->x2 > x + 0 && a->x2 < x + width - 0 && a->y2 > y + 0&& a->y2 < y + height - 0) {
		return 1;
	}
	if(a->x2 > x + 0 && a->x2 < x + width - 0 && a->y1 > y + 0 && a->y1 < y + height - 0) {
		return 1;
	}
	return 0;
}

bool TriRectOverlap(tri* a, rect r) {
	int x = r.x;
	int y = r.y;
	int width = r.width;
	int height = r.height;
	if(PointInsideRightTriangle(a, x, y +  height)) {
		return 1;
	}
	if(PointInsideRightTriangle(a, x + width, y + height)) {
		return 1;
	}
	if(PointInsideRightTriangle(a, x + width, y)) {
		return 1;
	}
	if(PointInsideRightTriangle(a, x, y)) {
		return 1;
	}
	//also check if the points of the triangle are inside the rectangle
	//skin usage is possibly redundant here
	if(a->x1 > x + 0 && a->x1 < x + width - 0 && a->y1 > y + 0 && a->y1 < y + height - 0) {
		return 1;
	}
	if(a->x2 > x + 0 && a->x2 < x + width - 0 && a->y2 > y + 0&& a->y2 < y + height - 0) {
		return 1;
	}
	if(a->x2 > x + 0 && a->x2 < x + width - 0 && a->y1 > y + 0 && a->y1 < y + height - 0) {
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

SDL_FRect transformRect(SDL_FRect input) {
	SDL_FRect obj;
	obj.x = ((input.x -g_camera.x) * g_camera.zoom);
	obj.y = ((input.y - g_camera.y) * g_camera.zoom);
	obj.w = (input.w * g_camera.zoom);
	obj.h = (input.h * g_camera.zoom);		
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

class box:public mapCollision {
public:
	rect bounds;
	bool active = true;
	int layer = 0;
	bool shineTop = 0;
	bool shineBot = 0;
	
	bool shadeTop = 0;

	//why is this one an int? because different values can affect the front corners.
	int shadeBot = 0; 
	//0 - no shading
	//1 - standard shading (corners will be there if their side is there)
	//2 - just corners, that way if this block is behind a triangle

	bool shadeLeft = 0;
	bool shadeRight = 0;


	box(int x1f, int y1f, int x2f, int y2f, int flayer, string &fwallt, string &fcapt, bool fcapped, bool fshineTop, bool fshineBot, const char* shading) {
		M("box()");
		bounds.x = x1f;
		bounds.y = y1f;
		bounds.z = flayer * 32;
		bounds.width = x2f;
		bounds.height = y2f;
		bounds.zeight = 32;
		layer = flayer;
		walltexture = fwallt;
		captexture = fcapt;
		capped = fcapped;
		shineTop = fshineTop;
		shineBot = fshineBot;
		shadeTop = (shading[0] == '1');

		switch (shading[1])
		{
		case '0':
			shadeBot = 0;
			break;
		case '1':
			shadeBot = 1;
			break;
		case '2':
			shadeBot = 2;
			break;
		}
		shadeLeft = (shading[2] == '1');
		shadeRight = (shading[3] == '1');
		g_boxs[layer].push_back(this);
	}

	// //copy constructor
	// box(const box& other) {
	// 	bounds.x = other.bounds.x;
	// 	bounds.y = other.bounds.y;
	// 	bounds.z = other.bounds.z;
	// 	bounds.width = other.bounds.width;
	// 	bounds.height = other.bounds.height;
	// 	bounds.zeight = other.bounds.zeight;
	// 	layer = other.layer;
	// 	walltexture = other.walltexture;
	// 	captexture = other.captexture;
	// 	capped = other.capped;
	// 	shineTop = other.shineTop;
	// 	shineBot = other.shineBot;
	// 	shadeTop = other.shadeTop;
	// 	shadeBot = other.shadeBot;
		
	// 	shadeLeft = other.shadeLeft;
	// 	shadeRight = other.shadeRight;
	// 	g_boxs[layer].push_back(this);
	// }

	// //swap function
	// void swapBoxes(box& first, box& second) {
	// 	using std::swap;
	// 	swap(first.bounds.x, second.bounds.x);
	// 	swap(first.bounds.y, second.bounds.y);
	// 	swap(first.bounds.z, second.bounds.z);
	// 	swap(first.bounds.width, second.bounds.width);
	// 	swap(first.bounds.height, second.bounds.height);
	// 	swap(first.bounds.zeight, second.bounds.zeight);
	// 	swap(first.layer, second.layer);
	// 	swap(first.walltexture, second.walltexture);
	// 	swap(first.captexture, second.captexture);
	// 	swap(first.capped, second.capped);
	// 	swap(first.shineTop, second.shineTop);
	// 	swap(first.shineBot, second.shineBot);
	// 	swap(first.shadeTop, second.shadeTop);
	// 	swap(first.shadeBot, second.shadeBot);
	// 	swap(first.shadeLeft, second.shadeLeft);
	// 	swap(first.shadeRight, second.shadeRight);
	// }

	// //copy-assignment-operator
    // box& operator=(box& other) {
	// 	swapBoxes(*this, other);
	// 	g_boxs[layer].push_back(this);

	// 	return *this;
	// }

	~box() {
		M("~box()");
		//this line crashes during easybake
		g_boxs[layer].erase(remove(g_boxs[layer].begin(), g_boxs[layer].end(), this), g_boxs[layer].end());
	}
};

// The idea of a collisionZone is to reduce overhead for large maps
// by having entities check if they are overlapping a collisionZone and only test for 
// collision with other walls/projectiles/entities also overlapping that collisionZone
// They will be able to be placed manually or procedurally
class collisionZone {
public:
	rect bounds = {0,0,10,10};
	vector<vector<box*>> guests;

	collisionZone(int x, int y, int width, int height) {
		bounds = {x, y, width, height};
		g_collisionZones.push_back(this);
		for (int i = 0; i < g_layers; i++) {
			vector<box*> v = {};
			guests.push_back(v);
		}
	}

	~collisionZone() {
		for (int i = 0; i < g_layers; i++) {
			guests[i].clear();
		}
	}

	//ATM we will be doing this on mapload
	void inviteAllGuests() {
		for(int i = 0; i < g_layers; i++) {
			for(int j = 0; j < (int)g_boxs[i].size(); j++){
				if(RectOverlap(this->bounds, g_boxs[i][j]->bounds)) {
					guests[i].push_back(g_boxs[i][j]);
				}
			}
		}
	}

	void debugRender(SDL_Renderer* renderer) {
		SDL_FRect rend = {(float)bounds.x, (float)bounds.y, (float)bounds.width, (float)bounds.height};
		rend = transformRect(rend);
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderDrawRectF(renderer, &rend);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	}
};



//cast a ray from the sky at a xy position and returns a z position of an intersection with a block
//have fun adding ramps *now* ;)
int verticalRayCast(int fx, int fy) {
	
	rect trace = {fx-5, fy-5, 10, 10};
	for (int i = g_layers - 1; i >= 0; i--) {
		for(auto x : g_boxs[i]) {
			if(RectOverlap(trace, x->bounds )) {
				//we hit a block
				return (i + 1) * 64;
			}
		}
	}

	//no hit
	return 0;
	
}

class door {
public:
	float x = 0;
	float y = 0;
	float z = 0;
	float width = 50;
	float height = 50;
	float zeight = 64;
	float friction;
	rect bounds;
	string to_map;
	string to_point;
	

	door(SDL_Renderer * renderer, const char* fmap, string fto_point,  int fx, int fy, int fz, int fwidth, int fheight, int fzeight) {		
		M("door()");
		this->x = fx;
		this->y = fy;
		this->z = fz;

		this->bounds.x = fx;
		this->bounds.y = fy;
		
		to_map = fmap;
		to_point = fto_point;

		this->width = fwidth;
		this->height = fheight;
		this->zeight = fzeight;

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
	float x = 0;
	float y = 0;
	int z = 0; //really just layer

	float width = 0;
	float height = 0;
	float xoffset = 0; //for aligning texture across the map, change em to floats later
	float yoffset = 0;
	float dxoffset = 0; //for changing texture coords
	float dyoffset = 0;
	int texwidth = 0;
	int texheight = 0;

	bool wraptexture = 1; //should we tile the image or stretch it?
	bool wall = 0; //darken image if it is used for a wall as opposed to a floor
	bool asset_sharer = 0; //1 if the tile is sharing another tile's texture, and this texture should in this case not be deleted in the destructor
	bool software = 0; //tiles used for the mapeditor/ui/etc are specially marked to avoid saving them or trying to delete them from a map

	SDL_Surface* image = 0;
	SDL_Texture* texture = 0;
	string fileaddress = "df"; //for checking if someone else has already loaded a texture
	string mask_fileaddress = "&"; //unset value

	
	
	tile(SDL_Renderer * renderer, const char* filename, const char* mask_filename, int fx, int fy, int fwidth, int fheight, int flayer, bool fwrap, bool fwall, float fdxoffset, float fdyoffset) {		
		this->x = fx;
		this->y = fy;
		this->z = flayer;
		this->width = fwidth;
		this->height = fheight;
		this->wall = fwall;
		this->wraptexture = fwrap;

		this->dxoffset = fdxoffset;
		this->dyoffset = fdyoffset;
		
		fileaddress = filename;
		bool cached = false;
		//has someone else already made a texture?
		for(int i=0; i < (int)g_tiles.size(); i++){
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
				SDL_SetTextureColorMod(texture, -65, -65, -65);
			} else {
				//SDL_SetTextureColorMod(texture, -20, -20, -20);
			}
			
			SDL_QueryTexture(texture, NULL, NULL, &texwidth, &texheight);
			if(fileaddress.find("OCCLUSION") != string::npos) {
				//SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_MOD);
				
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
			}
		}

		
		mask_fileaddress = mask_filename;

		
		this->xoffset = fmod(this->x, this->texwidth);
		this->yoffset = fmod(this->y, this->texheight);

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

	void reloadTexture() {
		if(asset_sharer) {
						
		} else {
			image = IMG_Load(fileaddress.c_str());
			texture = SDL_CreateTextureFromSurface(renderer, image);
			if(wall) {
				//make walls a bit darker
				SDL_SetTextureColorMod(texture, -65, -65, -65);
			} else {
				//SDL_SetTextureColorMod(texture, -20, -20, -20);
			}
			
			SDL_QueryTexture(texture, NULL, NULL, &texwidth, &texheight);
			if(fileaddress.find("OCCLUSION") != string::npos) {
				//SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_MOD);
				
			}
			if(mask_fileaddress[0] != '&') {
				SDL_DestroyTexture(texture);				
		
				SDL_Surface* smask = IMG_Load(mask_fileaddress.c_str());
				SDL_Texture* mask = SDL_CreateTextureFromSurface(renderer, smask);
				SDL_Texture* diffuse = SDL_CreateTextureFromSurface(renderer, image);

				texture = MaskTexture(renderer, diffuse, mask);
				SDL_FreeSurface(smask);
				SDL_DestroyTexture(mask);
				SDL_DestroyTexture(diffuse);
			}
			SDL_FreeSurface(image);
		}
	}
	
	void reassignTexture() {
		if(asset_sharer) {
			if(g_tiles.size() > 1) {
				for(unsigned int i=g_tiles.size() - 1; i > 0; i--){
					if(g_tiles[i]->mask_fileaddress == this->mask_fileaddress && g_tiles[i]->fileaddress == this->fileaddress && !g_tiles[i]->asset_sharer) {

						this->texture = g_tiles[i]->texture;

						this->asset_sharer = 1;
						this->texwidth = g_tiles[i]->texwidth;
						this->texheight = g_tiles[i]->texheight;
						break;
						
					}
				}
			}
		}
	}

	rect getMovedBounds() {
		return rect(x, y, width, height);
	}

	void render(SDL_Renderer * renderer, camera fcamera) {
		SDL_FPoint nowt = {0, 0};
		rect obj((x -fcamera.x)* fcamera.zoom, (y-fcamera.y) * fcamera.zoom, width * fcamera.zoom, height * fcamera.zoom);		
		rect cam(0, 0, fcamera.width, fcamera.height);
		
		//movement
		if((dxoffset !=0 || dyoffset != 0 ) && wraptexture) {
			xoffset += dxoffset;
			yoffset += dyoffset;
			if(xoffset < 0) {
				xoffset = texwidth;
			}
			if(xoffset > texwidth) {
				xoffset = 0;
			}
			if(yoffset < 0) {
				yoffset = texheight;
			}
			if(yoffset > texheight) {
				yoffset = 0;
			}
		}

		if(RectOverlap(obj, cam)) {
			
			if(this->wraptexture) {	
				SDL_FRect srcrect;
				SDL_FRect dstrect;
				float ypos = 0;
				float xpos = 0;

				srcrect.x = xoffset;
				srcrect.y = yoffset;
				while(1) {
					if(srcrect.x == xoffset) {
						srcrect.w = (texwidth - xoffset);
						dstrect.w = (texwidth - xoffset);
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

					dstrect.w = (dstrect.w * fcamera.zoom);
					dstrect.h = (dstrect.h * fcamera.zoom);

					dstrect.x = ((dstrect.x - fcamera.x)* fcamera.zoom);
					dstrect.y = ((dstrect.y - fcamera.y)* fcamera.zoom);
					
					SDL_Rect fsrcrect;
					fsrcrect.x = srcrect.x;
					fsrcrect.y = srcrect.y;
					fsrcrect.w = srcrect.w;
					fsrcrect.h = srcrect.h;
					
					SDL_RenderCopyExF(renderer, texture, &fsrcrect, &dstrect, 0, &nowt, SDL_FLIP_NONE);
					
					
					
					
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
				SDL_FRect dstrect = { (x -fcamera.x)* fcamera.zoom, (y-fcamera.y) * fcamera.zoom, width * fcamera.zoom, height * fcamera.zoom};
				SDL_RenderCopyF(renderer, texture, NULL, &dstrect);
			}

		}
	}
};


class attack {
public:
	float maxCooldown = 400; //ms
	float cooldown = 0;
	string name = "";
	bool canBeHeldDown = 1;
	float shotLifetime = 500; //ms
	int width = 20;
	int height = 20;
	float damage = 1;
	float spread = 0;
	float randomspread = 0;
	int range = 512; // max range, entities will try to be 0.8% of this to hit safely. in worldpixels
	float size = 0.1;
	float speed = 1;
	int numshots = 1;
	SDL_Texture* texture;

	bool melee = 0; //melee attacks are special in that the entity will try to go directly to their target, but might attack
					//before they reach them, depending on their range
					//if this distinction didn't exist, melee enemies would stutter while chasing their targets, because
					//as soon as they get in range they will stop.

	bool snake = 0; //can bullets travel through walls

	float xoffset = 5;

	bool assetsharer = 0;
	string spritename = "";


	//animation
	int framewidth;
	int frameheight;
	int xframes;
	int yframes;
	vector<coord> framespots;

	//given some float time, return floats for x and y position
	//will be rotated/flipped

	//i should update this with variables, which are expressions from an attack file
	float forward(float time) {
		return speed;
	} 

	float sideways(float time) {
		//return 10 * cos(time / 45);
		return 0;
	}

	//new param to attack()
	//entities that are deleted on map closure
	//can try to share attack graphics
	//but not entities that could possibly join the party
	attack(string filename, bool tryToShareTextures) {
		M("attack()");
		this->name = filename;
		ifstream file;
		string loadstr;
		//try to open from local map folder first
		
		loadstr = "maps/" + g_mapdir + "/attacks/" + filename + ".atk";

		D(loadstr);
		const char* plik = loadstr.c_str();
		
		file.open(plik);
		
		if (!file.is_open()) {
			//load from global folder
			loadstr = "static/attacks/" + filename + ".atk";
			//D(loadstr);
			const char* plik = loadstr.c_str();
			
			file.open(plik);
			
			if (!file.is_open()) {
				//just make a default entity
				string newfile = "static/attacks/default.atk";
				//D(loadstr);
				file.open(newfile);
			}
		}

		file >> spritename;

		string temp;
		temp = "maps/" + g_mapdir + "/sprites/" + spritename + ".bmp";
		if(!fileExists(temp)) {
			temp = "static/sprites/" + spritename + ".bmp";
			if(!fileExists(temp)) {
				temp = "static/sprites/default.bmp";
			}
		}

		//only try to share textures if this isnt an entity
		//that can ever be part of the party
		if(tryToShareTextures) {
			for(auto x : g_attacks){
				if(x->spritename == this->spritename) {
					this->texture = x->texture;
					assetsharer = 1;
					
				}
			}
		}

		file >> framewidth;
		file >> frameheight;
		if(!assetsharer) {

			SDL_Surface* image;
			image = IMG_Load(temp.c_str());
			
			texture = SDL_CreateTextureFromSurface(renderer, image);
			SDL_FreeSurface(image);
		}

		file >> this->maxCooldown;
		float size;
		file >> size;
		
		SDL_QueryTexture(texture, NULL, NULL, &width, &height);

		xframes = width / framewidth;
		yframes = height / frameheight;
		
	

		for (int j = 0; j < height; j+=frameheight) {
			for (int i = 0; i < width; i+= framewidth) {
				coord a; 
				a.x = i;
				a.y = j;
				framespots.push_back(a);
			}
		}

		width = framewidth * size;
		height = frameheight * size;


		
		file >> this->damage;
		file >> this->speed;
		file >> this->shotLifetime;
		file >> this->spread;
		this->spread *= M_PI;
		file >> this->randomspread;
		randomspread *= M_PI;
		file >> this->numshots;
		file >> this->range;
		this->melee = 0;
		file >> this->melee; //should we just barrel towards the target no matter what
		file >> this->snake;

		g_attacks.push_back(this);
	}

	~attack() {
		if(!assetsharer) {
			SDL_DestroyTexture(texture);
		}
		g_attacks.erase(remove(g_attacks.begin(), g_attacks.end(), this), g_attacks.end());
	}
};


class weapon {
public:
	string name;
	int combo = 0;
	float maxComboResetMS = 1000;
	float comboResetMS = 0;
	vector<attack*> attacks;

	weapon() {}

	//add constructor and field on entity object
	//second param should be 0 for entities
	//that could join the party and 1 otherwise
	weapon(string fname, bool tryToShareGraphics) {
		name = fname;

		ifstream file;
		string line;
		string address;
		
		//local
		address = "maps/" + g_mapdir + "/weapons/" + name + ".wep";
		if(!fileExists(address)) {
			address = "static/weapons/" + name + ".wep";
			if(!fileExists(address)) {
				address = "static/weapons/" + name + ".wep";
			}
		}

		string field = "";
		string value = "";
		file.open(address);

		while(getline(file, line)) {
			if(line == "&") { break; }
			field = line.substr(0, line.find(' '));
			attack* a = new attack(line, tryToShareGraphics);
			//a->faction = faction;
			attacks.push_back(a);
		}
		file >> maxComboResetMS;		
		file.close();
		g_weapons.push_back(this);
	}

	~weapon() {
		for(auto x: attacks) {
			delete x;
		}
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
	float zeight = 0;
	float sortingOffset = 0;
	float baseSortingOffset = 0;
	SDL_Texture* texture;
	rect bounds = {0, 0, 10, 10};
	string name = "unnamed";

	bool tangible = 1;

	//add entities and mapObjects to g_actors with dc
	actor() {
		//M("actor()");
		bounds.x = 0; bounds.y = 0; bounds.width = 10; bounds.height = 10;
		g_actors.push_back(this);
	}
	
	virtual ~actor() {
		//M("~actor()");
		g_actors.erase(remove(g_actors.begin(), g_actors.end(), this), g_actors.end());
	}

	virtual void render(SDL_Renderer * renderer, camera fcamera) {
		
	}
	
	
	int getOriginX() {
		return  x + bounds.x + bounds.width/2;
	}

	int getOriginY() {
		return y + bounds.y + bounds.height/2;
	}

	//for moving the object by its origin
	//this won't move the origin relative to the sprite or anything like that
	void setOriginX(float fx) {
		x = fx - bounds.x - bounds.width/2;
	}

	void setOriginY(float fy) {
		y = fy - bounds.y - bounds.height/2;
	}
};

inline int compare_ent (actor* a, actor* b) {
  	return a->y + a->z + a->sortingOffset < b->y + b->z + b->sortingOffset;
}

void sort_by_y(vector<actor*> &g_entities) {
    stable_sort(g_entities.begin(), g_entities.end(), compare_ent);
}


class effectIndex {
public:
	string texname = "default";
	bool OwnsTexture = 1;
 	SDL_Texture* texture;
 	int spawnNumber = 12;
	int spawnRadius = 1;
 	int plifetime = 5;
 	int pwidth = 50;
 	int pheight = 50;
	float pvelocityx = 0;
	float pvelocityy = 0;
	float pvelocityz = 0;
	float paccelerationx = 0;
	float paccelerationy = 0;
	float paccelerationz = 0;
	float pdeltasizex = -10;
	float pdeltasizey = 10;

 	effectIndex(string filename, SDL_Renderer* renderer);

 	//given coordinates, spawn particles in the leve
 	void happen(int fx, int fy, int fz);

 	~effectIndex();

};



class particle : public actor {
public:
 	int lifetime = 0;
	float velocityx = 0;
	float velocityy = 0;
	float velocityz = 0;
	float accelerationx = 0;
	float accelerationy = 0;
	float accelerationz = 0;
	float deltasizex = 0;
	float deltasizey = 0;
	SDL_Texture* texture;

 	particle(effectIndex* type) {
 		g_particles.push_back(this);
		lifetime = type->plifetime;
		width = type->pwidth;
		height = type->pheight;
		velocityx = type->pvelocityx;
		velocityy = type->pvelocityy;
		velocityz = type->pvelocityz;
		accelerationx = type->paccelerationx;
		accelerationy = type->paccelerationy;
		accelerationz = type->paccelerationz;
		deltasizex = type->pdeltasizex;
		deltasizey = type->pdeltasizey;
		texture = type->texture;
 	}
	
 	~particle() {
 		g_particles.erase(remove(g_particles.begin(), g_particles.end(), this), g_particles.end());
 	}

 	void update(int elapsed, camera fcamera) {
 		lifetime -= elapsed;
		
 		
		x += velocityx * elapsed;
		y += velocityy * elapsed;
		z += velocityz * elapsed;
		velocityx += accelerationx * elapsed;
		velocityy += accelerationy * elapsed;
		velocityz += accelerationz * elapsed;
		width += deltasizex * elapsed;
		height += deltasizey * elapsed;
		float zero = 0;
		width = max(zero,width);
		height = max(zero,height);
		z = max(zero,z);
 	}

	void render(SDL_Renderer* renderer, camera fcamera) {

 		SDL_FRect dstrect = { (x -fcamera.x -(width/2))* fcamera.zoom, (y-(z* XtoZ) - fcamera.y-(height/2)) * fcamera.zoom, width * fcamera.zoom, height * fcamera.zoom};	
	 	SDL_RenderCopyF(renderer, texture, NULL, &dstrect);	
		
	}
};

effectIndex::effectIndex(string filename, SDL_Renderer* renderer) {
	string existSTR;
	existSTR = "maps/" + g_mapdir + "/effects/" + filename + ".eft";
	if(!fileExists(existSTR)) {
		existSTR = "static/effects" + filename + ".eft";
		if(!fileExists(existSTR)) {
			existSTR = "static/effects/default.eft";
		}
	}
	ifstream file;
	file.open(existSTR);		
	string line;

	//name of texture
	file >> line;
	file >> line;
	texname = line;

	//has anyone already loaded this texture?
	//for(auto x : g_effectIndexes) {
		//if(x->texname == this->texname) {
			//this->OwnsTexture = 0;
			//this->texture = x->texture;
			//break;
		//}
	//}

	

	if(1) {
		existSTR = "maps/" + g_mapdir + "/effects/" + texname + ".bmp";
		if(!fileExists(existSTR)) {
			existSTR = "static/effects/	" + texname + ".bmp";
			if(!fileExists(existSTR)) {
				existSTR = "static/effects/default.bmp";
			}
		}
		
		SDL_Surface* image = IMG_Load(existSTR.c_str());
		if(image == NULL) {
			E("Couldn't load surface for effect");
		}
		
		texture = SDL_CreateTextureFromSurface(renderer, image);
		if(texture == NULL) {
			E("Couldn't load texture for effect");
		}
		SDL_FreeSurface(image);
	}

	//spawnNumber
	file >> line;
	file >> spawnNumber;

	//spawnRadius
	file >> line;
	file >> spawnRadius;
	spawnRadius *= 64;

	//plifetime
	file >> line;
	file >> plifetime;
	plifetime *= 1000;

	//pwidth
	file >> line;
	file >> pwidth;

	//pheight
	file >> line;
	file >> pheight;

	//pvelocityx
	file >> line;
	file >> pvelocityx;

	//pvelocityy
	file >> line;
	file >> pvelocityy;

	file >> line;
	file >> pvelocityz;

	//paccelerationx
	file >> line;
	file >> paccelerationx;

	//paccelerationy
	file >> line;
	file >> paccelerationy;

	//paccelerationz
	file >> line;
	file >> paccelerationz;

	//pdeltasizex
	file >> line;
	file >> pdeltasizex;

	//pdeltasizey
	file >> line;
	file >> pdeltasizey;

	g_effectIndexes.push_back(this);

}

//given coordinates, spawn particles in the level
void effectIndex::happen(int fx, int fy, int fz) {
	for(int i = 0; i < spawnNumber; i++) {
		particle* a = new particle(this);
		a->x = fx + (spawnRadius/2 - (rand() % spawnRadius));
		a->y = fy + (spawnRadius/2 - (rand() % spawnRadius));
		a->z = fz + (spawnRadius/2 - (rand() % spawnRadius));
	}
}

effectIndex::~effectIndex() {
	if(OwnsTexture) {
		SDL_DestroyTexture(texture);
	}

	g_effectIndexes.erase(remove(g_effectIndexes.begin(), g_effectIndexes.end(), this), g_effectIndexes.end());
}


class cshadow:public actor {
public:
	float size;
	actor* owner = 0;
	SDL_Surface* image = 0;
	SDL_Texture* texture = g_shadowTexture;
	int xoffset = 0;
	int yoffset = 0;
	int alphamod = 255;
	bool enabled = 1;

	cshadow(SDL_Renderer * renderer, float fsize) {
		size = fsize;
		g_shadows.push_back(this);
	}

	~cshadow() {
		//M("~cshadow()" );
		g_shadows.erase(remove(g_shadows.begin(), g_shadows.end(), this), g_shadows.end());
	}
	void render(SDL_Renderer* renderer, camera fcamera);
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
	SDL_RendererFlip flip = SDL_FLIP_NONE;
	
	int curheight = 0;
	int curwidth = 0;

	entity* owner = nullptr;
	float maxLifetime = 0.4;
	float lifetime = 500;
	float angle = 0;
	attack* gun;
	cshadow * shadow = 0;

	projectile(attack* fattack) {
		this->sortingOffset = 12;
		this->width = fattack->width;
		this->height = fattack->height;
		this->bounds.width = this->width;
		this->bounds.height = this->height * XtoY;
		this->bounds.y = this->height - this->bounds.height;
		
		this->gun = fattack;
		this->maxLifetime = fattack->shotLifetime;
		this->lifetime = fattack->shotLifetime;
		
		shadow = new cshadow(renderer, fattack->size);
		this->shadow->owner = this;
		shadow->width = width;
		shadow->height = bounds.height;
		

		gun = fattack;
		texture = gun->texture;
		asset_sharer = 1;
		
		animate = 0;
		curheight = height;
		curwidth = width;
		g_projectiles.push_back(this);
	}

	~projectile() {	
		g_projectiles.erase(remove(g_projectiles.begin(), g_projectiles.end(), this), g_projectiles.end());
		delete shadow;
		//make recursive projectile
	}

	void update(float elapsed) {
		
		
		rect bounds = {(int)x, (int)y, (int)width, (int)height};
		layer = max(z /64, 0.0f);
		if(!gun->snake) {
			for(auto n : g_boxs[layer]) {
				if(RectOverlap(bounds, n->bounds)) {
					playSound(0, g_bulletdestroySound, 0);
					lifetime = 0;
					return;
				}
			}
		}

		if(lifetime <= 0) {
			return;
		}
		
		x += (sin(angle) * gun->sideways(maxLifetime - lifetime) + cos(angle) * gun->forward(maxLifetime - lifetime) + xvel)* (elapsed / 16);
		y += (XtoY * (sin(angle + M_PI / 2) * gun->sideways(maxLifetime - lifetime) + cos(angle + M_PI / 2) * gun->forward(maxLifetime - lifetime) + yvel)) * (elapsed / 16);
		lifetime -= elapsed;

		//move shadow to feet
		shadow->x = x;
		shadow->y = y;
		
		float floor = 0;
		int layer;
		layer = max(z /64, 0.0f);
		layer = min(layer, (int)g_boxs.size() - 1);
		if(layer > 0) {

			//this means that if the bullet is above a block that isnt right under it, the shadow won't be seen, but it's not worth it atm to change
			rect thisMovedBounds = rect(x,  y, width, height);
			for (auto n : g_boxs[layer - 1]) {
				if(RectOverlap(n->bounds, thisMovedBounds)) {
					floor = 64 * (layer);
					break;
				}
			}
		}
		shadow->z = floor;
	}

	void render(SDL_Renderer * renderer, camera fcamera) {
		SDL_FRect obj = {(x -fcamera.x)* fcamera.zoom, ((y- (z + zeight) * XtoZ) - fcamera.y) * fcamera.zoom, width * fcamera.zoom, height * fcamera.zoom};		
		SDL_FRect cam = {0, 0, fcamera.width, fcamera.height};
		
		if(RectOverlap(obj, cam)) {

			
			
			if(gun->framespots.size() > 1) {
				frame = animation * gun->xframes + frameInAnimation;
				SDL_Rect srcrect = {gun->framespots[frame].x, gun->framespots[frame].y, gun->framewidth, gun->frameheight};
				const SDL_FPoint center = {0 ,0};
				if(texture != NULL) {
					SDL_RenderCopyExF(renderer, texture, &srcrect, &obj, 0, &center, flip);
				}
			} else {
				if(texture != NULL) {
					SDL_RenderCopyF(renderer, texture, NULL, &obj);
				}
			}
		}
	}
};



class mapObject:public actor {
public:
	
	float xoffset = 0;
	float yoffset = 64;
	bool wall; //to darken entwalls
	//float sortingOffset = 0; //to make entites float in the air, or sort as if they are higher or lower than they are first use was for building diagonal walls
	float extraYOffset = 0; //used to encode a permanent y axis offset, for diagonal walls
	//0 - slant left, 1 - slant right, 2 - pillar
	int effect = 0;
	bool asset_sharer = 0;
	int framewidth = 0;
	int frameheight = 0;
	bool diffuse = 1; //is this mapobject used for things such as walls or floors, as opposed to props or lighting
	string mask_fileaddress = "&"; //unset value
	SDL_Texture* alternative = nullptr; //representing the texture tinted as if it were the opposite of the texture in terms of shading
	mapCollision* parent = nullptr;

	mapObject(SDL_Renderer * renderer, string imageadress, const char* mask_filename, float fx, float fy, float fz, float fwidth, float fheight, bool fwall = 0, float extrayoffset = 0) {
		//M("mapObject() fake");
		
		name = imageadress;
		mask_fileaddress = mask_filename;
		
		bool cached = false; 	
		wall = fwall;
		
		//this could further be improved by going thru g_boxs and g_triangles
		//as there are always less of those than mapobjects
		//heres an idea: could we copy textures if the wall field arent equal instead of loading them again
		//techincally, this could be a big problem, so there could be a finit number of mapObjects we check
		//in a huge map, loading a map object with a texture that hasnt been loaded yet could take forever
		//!!!
		//if you're having problems with huge maps, revisit this
		if(g_mapObjects.size()  > 1) {
			for(unsigned int i=g_mapObjects.size() - 1; i > 0; i--){
				if(g_mapObjects[i]->mask_fileaddress == mask_filename && g_mapObjects[i]->name == this->name) {
					if(g_mapObjects[i]->wall == this->wall) {
						cached = true;
						this->texture = g_mapObjects[i]->texture;
						this->alternative =g_mapObjects[i]->alternative;
						this->asset_sharer = 1;
						this->framewidth = g_mapObjects[i]->framewidth;
						this->frameheight = g_mapObjects[i]->frameheight;
						break;
					} else {
						cached = true;
						this->texture = g_mapObjects[i]->alternative;
						this->alternative = g_mapObjects[i]->texture;
						
						this->asset_sharer = 1;
						this->framewidth = g_mapObjects[i]->framewidth;
						this->frameheight = g_mapObjects[i]->frameheight;
						break;
					}
				}
			}
		}
		if(cached) {
						
		} else {
			const char* plik = imageadress.c_str();
			SDL_Surface* image = IMG_Load(plik);
			texture = SDL_CreateTextureFromSurface(renderer, image);
			alternative = SDL_CreateTextureFromSurface(renderer, image);
			

			if(mask_filename[0] != '&') {
				
				//the SDL_SetHint() changes a flag from 3 to 0 to 3 again. 
				//this effects texture interpolation, and for masked entities such as wallcaps, it should
				//be off.

				SDL_DestroyTexture(texture);
				SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
				SDL_Surface* smask = IMG_Load(mask_filename);
				SDL_Texture* mask = SDL_CreateTextureFromSurface(renderer, smask);
				SDL_Texture* diffuse = SDL_CreateTextureFromSurface(renderer, image);
				//SDL_SetTextureColorMod(diffuse, -65, -65, -65);
				texture = MaskTexture(renderer, diffuse, mask);
				SDL_FreeSurface(smask);
				SDL_DestroyTexture(mask);
				SDL_DestroyTexture(diffuse);
				SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "3");
			}
			SDL_FreeSurface(image);

			if(fwall) {
				wall = 1;
				SDL_SetTextureColorMod(texture, -g_walldarkness, -g_walldarkness, -g_walldarkness);
			} else {
				SDL_SetTextureColorMod(alternative, -g_walldarkness, -g_walldarkness, -g_walldarkness);
			}
			if(name.find("SHADING") != string::npos) {
				SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
				diffuse = 0;
				//SDL_SetTextureAlphaMod(texture, 150);
			}
			if(name.find("OCCLUSION") != string::npos) {
				//SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_MOD);
				diffuse = 0;
			}
			SDL_QueryTexture(texture, NULL, NULL, &this->framewidth, &this->frameheight);
		}

		
		
		
		//used for tiling
		
		
		this->x = fx;
		this->y = fy;
		this->z = fz;
		this->width = fwidth;
		this->height = fheight;

		//crappy solution for porting to windows
		if(this->framewidth == 0) { this->framewidth = 64;}
		this->xoffset = int(this->x) % int(this->framewidth);
		this->bounds.y = -55; //added after the ORIGIN was used for ent sorting rather than the FOOT.
		//this essentially just gives the blocks an invisible hitbox starting from their "head" so that their origin is in the middle
		//of the box

		extraYOffset = extrayoffset;
		this->yoffset = ceil(fmod(this->y - this->height+ extrayoffset, this->frameheight));
		//this->yoffset = 33;
		if(fwall ) {
			//this->yoffset = int(this->y + this->height - (z * XtoZ) + extrayoffset) % int(this->frameheight);
			
			//most favorable approach- everything lines up, even diagonal walls.
			//why is the texture height multiplied by 4? It just has to be a positive number, so multiples of the frameheight
			//are added in because of the two negative terms
			this->yoffset = (int)(4 * this->frameheight - this->height - (z * XtoZ)) % this->frameheight;
		}
		
		g_mapObjects.push_back(this);
		
	}

	//There was a problem when I ported to windows, where mapobjects which used masks for their textures (e.g. the tops of triangular walls)
	//would lose their textures. This function exists to reload them after the windowsize has been changed.
	void reloadTexture() {
		if(asset_sharer) {
						
		} else {
			//delete our existing texture
			SDL_DestroyTexture(texture);
			SDL_DestroyTexture(alternative);

			const char* plik = name.c_str();
			SDL_Surface* image = IMG_Load(plik);
			texture = SDL_CreateTextureFromSurface(renderer, image);
			alternative = SDL_CreateTextureFromSurface(renderer, image);
			

			if(mask_fileaddress[0] != '&') {
				
				//the SDL_SetHint() changes a flag from 3 to 0 to 3 again. 
				//this effects texture interpolation, and for masked entities such as wallcaps, it should
				//be off.

				SDL_DestroyTexture(texture);
				SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
				SDL_Surface* smask = IMG_Load(mask_fileaddress.c_str());
				SDL_Texture* mask = SDL_CreateTextureFromSurface(renderer, smask);
				SDL_Texture* diffuse = SDL_CreateTextureFromSurface(renderer, image);
				//SDL_SetTextureColorMod(diffuse, -65, -65, -65);
				texture = MaskTexture(renderer, diffuse, mask);
				SDL_FreeSurface(smask);
				SDL_DestroyTexture(mask);
				SDL_DestroyTexture(diffuse);
				SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "3");
			}
			SDL_FreeSurface(image);

			if(wall) {
				SDL_SetTextureColorMod(texture, -g_walldarkness, -g_walldarkness, -g_walldarkness);
			} else {
				SDL_SetTextureColorMod(alternative, -g_walldarkness, -g_walldarkness, -g_walldarkness);
			}
			if(name.find("SHADING") != string::npos) {
				SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
				//SDL_SetTextureAlphaMod(texture, 150);
			}
			if(name.find("OCCLUSION") != string::npos) {
				//SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_MOD);
			}
			//SDL_QueryTexture(texture, NULL, NULL, &this->framewidth, &this->frameheight);
		}
	}

	//this is used with reloadTexture for any mapObjects which are asset-sharers
	void reassignTexture() {
		if(asset_sharer) {
			if(g_mapObjects.size()  > 1) {
			for(unsigned int i=g_mapObjects.size() - 1; i > 0; i--){
				if(g_mapObjects[i]->mask_fileaddress == this->mask_fileaddress && g_mapObjects[i]->name == this->name && !g_mapObjects[i]->asset_sharer) {
					if(g_mapObjects[i]->wall == this->wall) {
						this->texture = g_mapObjects[i]->texture;
						this->alternative =g_mapObjects[i]->alternative;
						this->asset_sharer = 1;
						this->framewidth = g_mapObjects[i]->framewidth;
						this->frameheight = g_mapObjects[i]->frameheight;
						break;
					} else {
						this->texture = g_mapObjects[i]->alternative;
						this->alternative = g_mapObjects[i]->texture;
						
						this->asset_sharer = 1;
						this->framewidth = g_mapObjects[i]->framewidth;
						this->frameheight = g_mapObjects[i]->frameheight;
						break;
					}
				}
			}
		}
		}
	}

	~mapObject() {
		if(!asset_sharer) {
			SDL_DestroyTexture(alternative);
		}
		g_mapObjects.erase(remove(g_mapObjects.begin(), g_mapObjects.end(), this), g_mapObjects.end());
	}

	rect getMovedBounds() {
		return rect(bounds.x + x, bounds.y + y, bounds.width, bounds.height);
	}

	void render(SDL_Renderer * renderer, camera fcamera) {
	

		SDL_FPoint nowt = {0, 0};

		SDL_FRect obj; // = {(floor((x -fcamera.x)* fcamera.zoom) , floor((y-fcamera.y - height - XtoZ * z) * fcamera.zoom), ceil(width * fcamera.zoom), ceil(height * fcamera.zoom))};
		obj.x = (x -fcamera.x)* fcamera.zoom;
		obj.y = (y-fcamera.y - height - XtoZ * z) * fcamera.zoom;
		obj.w = width * fcamera.zoom;
		obj.h = height * fcamera.zoom;

		SDL_FRect cam;
		cam.x = 0;
		cam.y = 0;
		cam.w = fcamera.width;
		cam.h = fcamera.height;
		
		if(RectOverlap(obj, cam)) {
			
			//if its a wall, check if it is covering the player
			if(this->wall && protag != nullptr) {
				
				// //make obj one block higher for the wallcap
				// //obj.height -= 45;
				// //obj.y += 45;
				// bool blocking = 0;
				
				// if(!g_protagHasBeenDrawnThisFrame && RectOverlap( transformRect(protag->getMovedBounds() ), transformRect( this->getMovedBounds() ) ) ) {
				// 	//objects are draw on top of each other
				// 	blocking = 1;
				// 	M("blocking protag");
				// }
				// M("did it block?");
				// if(blocking) {
				// 	SDL_SetTextureAlphaMod(texture, 160);
				// } else {
				// 	SDL_SetTextureAlphaMod(texture, 255);
				// }
				
			}
			

			
			
			SDL_Rect srcrect;
			SDL_FRect dstrect;
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
					//seems to cause seams, seemingly.
					//lets add one to the dstrect.w
					//although ceiling it might be better
					//all of the numbers above are ints
					//so rounding shouldnt be a problem
					//dstrect.w++;
					//fucks with shadows
				}
				if(ypos + dstrect.h > this->height ) {
					
					dstrect.h = this->height - ypos;
					if(dstrect.h + srcrect.y > frameheight) {
						dstrect.h = frameheight - srcrect.y;
					}
					srcrect.h = dstrect.h;
					//dstrect.h ++;
				}

				
				
				
				


				//transform
				//if(diffuse) {
					// dstrect.x = floor((dstrect.x - fcamera.x)* fcamera.zoom);
					// dstrect.y = floor((dstrect.y - fcamera.y - height)* fcamera.zoom);
					// dstrect.w = floor(dstrect.w * fcamera.zoom);
					// dstrect.h = floor(dstrect.h * fcamera.zoom);
				//} else {
					dstrect.x = ((dstrect.x - fcamera.x)* fcamera.zoom);
					dstrect.y = ((dstrect.y - fcamera.y - height)* fcamera.zoom);
					dstrect.w = (dstrect.w * fcamera.zoom);
					dstrect.h = (dstrect.h * fcamera.zoom);
				//}

				
				// if(!this->wall && this->diffuse) {
 				// 	SDL_SetTextureAlphaMod(texture, 160);
				// 	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
				// }

				SDL_RenderCopyExF(renderer, texture, &srcrect, &dstrect, 0, &nowt, SDL_FLIP_NONE );
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

//an item in someone's inventory, which can be used during events (not an actor)
//instances are not static.
class indexItem { 
public: 
	SDL_Texture* texture = 0; 
	string name = "Error"; 
	bool isKeyItem = 0; //key item, can it be sold
	vector<string> script;
	indexItem(string fname, bool fisKeyItem) : name(fname), isKeyItem(fisKeyItem) {

		//search worlditems for an item with the same texture
		string lstr;

		//is there a special sprite for how it appears in the inv?
		//if not, just use the standard one

		//check local first
		if(fileExists("maps/" + g_mapdir + "/items/" + fname + "-inv.bmp")) {
			lstr = "maps/" + g_mapdir + "/items/" + fname + "-inv.bmp";
		} else if(fileExists("maps/" + g_mapdir + "/items/" + fname + ".bmp")){
			lstr = "maps/" + g_mapdir + "/items/" + fname + ".bmp";
		} else if(fileExists("static/items/" + fname + "-inv.bmp")) {
			lstr = "static/items/" + fname + "-inv.bmp";
		} else {
			if(fileExists("static/items/" + fname + ".bmp")) {
				lstr = "static/items/" + fname + ".bmp";
			} else {
				//failsafe - load an image we know we have
				lstr = "static/sprites/default.bmp";
			}
		}
		
		
		bool storeThis = true;
		for(auto x : g_indexItems) {
			//M(x->name);
			if(this->name == x->name) {
				storeThis = false;
			}
		}
		if(storeThis) {
			SDL_Surface* temp = IMG_Load(lstr.c_str());
			texture = SDL_CreateTextureFromSurface(renderer, temp);
			SDL_FreeSurface(temp);
			g_indexItems.push_back(this);
		}

		//script
		ifstream stream;

		//check local dir
		string loadstr = "static/items/" + fname + ".txt";
		if(fileExists("maps/" + g_mapdir + "/items/" + fname + ".txt")) {
			loadstr = "maps/" + g_mapdir + "/items/" + fname + ".txt";
		}

		const char* plik = loadstr.c_str();
		stream.open(plik);
		string line;
		while (getline(stream, line)) {
			script.push_back(line);
		}
		
		parseScriptForLabels(script);
		I(script.size());
	}

	~indexItem() {
		g_indexItems.erase(remove(g_indexItems.begin(), g_indexItems.end(), this), g_indexItems.end());
		SDL_DestroyTexture(texture);
	}
};

//work as INSTANCES and not INDEXES
class ability {
public:
	int lowerCooldownBound = 40000;
	int upperCooldownBound = 60000;
	
	int cooldownMS = 50000;
	
	float lowerRangeBound = 0; //in worldpixels
	float upperRangeBound = 5;
	
	//we need a way to make abilities feel right, in terms of how they're charged
	//for instance, it would be odd if as soon as the player found an enemy, they suddenly used three abilities almost
	//at the same time, the moment they came into range
	//and, it would be odd if there were abilities that were meant to be seen rarely with high cooldowns, 
	//but were never used because the player would break range before it could happen
	//so abilities must be set with different ways of cooling down
	// 0 -> reset -> ability is reset when out of range. The ability is only charged when in range
	// 1 -> stable -> ability isn't reset, but it only charges when in range.
	// 2 -> accumulate -> ability charges regardless of range, meaning it will often activate as soon as the enemy finds the player


	int resetStableAccumulate = 1;

	vector<string> script;

	string name = "unset";

	ability(string binding) {
		name = binding;
		ifstream stream;
		string loadstr;
		//try to open from local map folder first
		
		loadstr = "maps/" + g_mapdir + "/scripts/" + binding + ".txt";
		//D(loadstr);
		const char* plik = loadstr.c_str();
		
		stream.open(plik);
		
		if (!stream.is_open()) {
			stream.open("static/scripts/" + binding + ".txt");
		}
		string line;

		getline(stream, line);

		while (getline(stream, line)) {
			script.push_back(line);
		}
		
		parseScriptForLabels(script);
		
	}

};

entity* searchEntities(string fname);

//this is a testament to how bad I am at programming
//dont as I have done

class adventureUI {
public:

	bool playersUI = 1; //is this the UI the player will use
						//for talking, using items, etc?

	bool executingScript = 0;

	ui* talkingBox = 0;
	textbox* talkingText = 0;
    textbox* responseText = 0;
    textbox* escText = 0;
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
    long long unsigned int response_index = 0; //number of response from array responses
    int sleepingMS = 0; //MS to sleep cutscene/script
    bool sleepflag = 0; //true for one frame after starting a sleep
    bool mobilize = 0; //used to allow the player to move during /sleep calls

    ui* inventoryA = 0; //big box, which has all of the items that the player has
    ui* inventoryB = 0; //small box, which will let the player quit or close the inventory

	ui* crosshair = 0; //for guiding player to objectives

    textbox* healthText = 0;

    int countEntities = 0; //used atthemoment for /lookatall to count how many entities we've looked at

	void showTalkingUI();
	void hideTalkingUI();
    

    void showInventoryUI();

    void hideInventoryUI();

	adventureUI(SDL_Renderer* renderer);
	
	~adventureUI();

	void pushText(entity* ftalker);
	
	void updateText();

	void continueDialogue();
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

	entity* owner = nullptr;
	
	worldsound(string filename, int fx, int fy) {
		name = filename;
		M("worldsound()" );
		
		ifstream file;

		string loadstr;
		//try to open from local map folder first
		
		loadstr = "maps/" + g_mapdir + "/worldsounds/" + filename + ".ws";
		D(loadstr);
		const char* plik = loadstr.c_str();
		
		file.open(plik);
		
		if (!file.is_open()) {
			loadstr = "static/worldsounds/" + filename + ".ws";
			const char* plik = loadstr.c_str();
			
			file.open(plik);
			
			if (!file.is_open()) {
				string newfile = "static/worldsounds/default.ws";
				file.open(newfile);
			}
		}
		
		string temp;
		file >> temp;
		string existSTR;
		existSTR = "maps/" + g_mapdir + "/sounds/" + temp + ".wav";
		D(existSTR);
		if(!fileExists(existSTR)) {
			existSTR = "static/sounds/" + temp + ".wav";
			if(!fileExists(existSTR)) {
				existSTR = "static/sounds/default.wav";
			}
		}
		
		
		blip = Mix_LoadWAV(existSTR.c_str());
		
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
		//D(tempFloat);
		g_worldsounds.push_back(this);
	}

	~worldsound() {
		M("~worldsound()" );
		Mix_FreeChunk(blip);
		g_worldsounds.erase(remove(g_worldsounds.begin(), g_worldsounds.end(), this), g_worldsounds.end());
	}

	void update(float elapsed);
};

class entity :public actor {
public:
	//dialogue
	vector<string> sayings;
	int dialogue_index = 0;

	//sounds
	Mix_Chunk* footstep;
	Mix_Chunk* footstep2;
	Mix_Chunk* voice;

	//a set of sounds that are attached to this entity
	vector<worldsound*> mobilesounds;
	
	//for musical entities
	Mix_Music* theme = 0;
	float musicRadius = 50;

	int footstep_reset = 0; //used for playing footsteps accurately with anim

	

	//basic movement
	float xagil = 0;
	float xaccel = 0;
	float yaccel = 0;
	float zaccel = 0;
	float xvel = 0;
	float yvel = 0;
	float zvel = 0;

	//this is for making entities point where they are trying to walk
	float walkingxaccel = 0;
	float walkingyaccel = 0;

	
	int layer = 0; //related to z, used for boxs
	bool grounded = 1; //is standing on ground
	float xmaxspeed = 0;
	float ymaxspeed = 0;
	float friction = 0;
	float baseFriction = 0;	
	int recalcAngle = 0; //a timer for when to draw sprites with diff angle

	float currentAirBoost = 1;
	float slowSeconds = 0;
	float slowPercent = 0;


	int update_z_time = 0; 
	int max_update_z_time = 1; //update zpos every x frames
	float oldz = 0;

	//there are navNode* called dest and current, but those 
	//are low-level, and updated fickely.
	//destination will be pursued via pathfinding each frame, if set.
	navNode* Destination = nullptr; 
	
	
	

	//animation
	bool animate = false; //does the squash/stretch animation for walking, talking... everything really
	float animtime = 0; //time since having started animating
	float animspeed = 0;
	float animlimit = 0.5; // the extent to the animation. 0.5 means halfway
	float curwidth = 0;
	float curheight = 0;
	bool turnToFacePlayer = true; //face player when talking
	SDL_RendererFlip flip = SDL_FLIP_NONE; //SDL_FLIP_HORIZONTAL; // SDL_FLIP_NONE
	
	float floatheight = 0; //how far up to float, worlditems use this to bounce
	int bounceindex = 0;

	int frame = 0; //current frame on SPRITESHEET
	int msPerFrame = 0; //how long to wait between frames of animation, 0 being infinite time (no frame animation)
	int msTilNextFrame = 0; //accumulater, when it reaches msPerFrame we advance frame
	int frameInAnimation = 0; //current frame in ANIMATION
	bool loopAnimation = 1; //start over after we finish
	int animation = 4; //current animation, or the column of the spritesheet
	int defaultAnimation = 4;
	bool scriptedAnimation = 0; //0 means the character is animated based on movement. 1 means the character is animated based on a script.

	int framewidth = 120; //width of single frame
	int frameheight = 120; //height of frame
	int xframes = 1; //number of frames ACROSS the spritesheet
	int yframes = 1; //number of frames DOWN the spritesheet
	vector<coord> framespots;
	bool up = 0;bool down = 0; bool left = 0; bool right = 0; //for chusing one of 8 animations for facing
	bool hadInput = 0; //had input this frame;
	int shooting = 0; //1 if character is shooting

	int opacity = 255; //opacity from 0 to 255, used for hiding shaded entities.	
	
	//object-related design
	bool dynamic = true; //true for things such as wallcaps. movement/box is not calculated if this is false
	bool inParty = false;
	bool talks = false;
	bool wallcap = false; //used for wallcaps
	cshadow * shadow = 0;
	bool rectangularshadow = 0;
	bool isAI = 0;	


	//stuff for orbitals
	bool isOrbital = false;
	entity* parent = nullptr;
	string parentName = "null";
	float angularPosition = 0;
	float angularSpeed = 10;
	float orbitRange = 1;
	int orbitOffset = 0; //the frames of offset for an orbital. 
	

	vector<entity*> children;
	
	//for textured entities (e.g. wallcap)
	float xoffset = 0;
	float yoffset = 64;
	bool wall = 0; //to darken entwalls
	//float sortingOffset = 0; //to make entites float in the air, or sort as if they are higher or lower than they are first use was for building diagonal walls
	float extraYOffset = 0; //used to encode a permanent y axis offset, for diagonal walls
	//0 - slant left, 1 - slant right, 2 - pillar
	int effect = 0;
	bool asset_sharer = 0;

	//self-data
	int data[255] = {0};

	//combat
	weapon* hisweapon = 0;
	bool canFight = 1;
	bool invincible = 0;
	//float invincibleMS = 0; //ms before setting invincible to 0
	bool agrod = 0; //are they fighting a target?
	
	//these two values are for having enemies travel 
	int poiIndex = 0;
	bool traveling = 0;   
	//this is for the way that the entity will travel. 
	//Roam means pick a random poi, go there, and then pick another random one, etc.
	//patrol means go through the nodes in a loop
	travelstyle myTravelstyle = roam;
	int currentPoiForPatrolling = 0;

	//this variable is set to one for one frame
	//if this entity has reached a poi and will start looking again for one.
	bool readyForNextTravelInstruction = 0; 
	
	entity* target = nullptr; //who are they fighting?
	int targetFaction = -10; //what faction are they fighting at the moment? If agrod and they lose their target, they will pick a random visible target having this faction
	coord dcoord; //place where they want to stand
	int faction = 0; //0 is player, 1 is most enemies
	bool essential = 0; //if this entity dies in your party, does the game end?
	int flashingMS = 0; //ms to flash red after taking damage
	int spinningMS = 0; //have they initiated a dodge backwards recently
	int lastSpinFrame = 0; //used for animating characters whilst dodging

	//ability-system
	//enemies need a way to call their own scripts
	//without worrying about being interupted by the player
	//just don't have them use the dialog-box
	adventureUI* myScriptCaller = nullptr;
	vector<ability> myAbilities;
	float autoAgroRadius = -1;
	float enrageSpeedbuff = 0;
	//aiIndex is used for having AI use their own patrolPoints
	//and not someone-else's
	int aiIndex = 0;
	
	//stats/leveling
	float level = 1; //affects damage
	float damagemultiplier = 0.2; // how much damage do you gain per level
	float maxhp = 2;
	float hp = 2;


	float dhpMS = 0; //how long to apply dhp (dot, healing)
	float dhp = 0;
	string weaponName = "";
	int cost = 1000; //cost to spawn in map and then xp granted to killer
	float cooldown = 0;  //anything about the usage of attacks needs to be moved here since the attack object

	Status status = none;
	float statusMS = 0; //how long to apply status before it becomes "none"
	float statusMag = 0; //magnitude of status

	//pathfinding
	float updateSpeed = 1500;
	float timeSinceLastUpdate = 0;
	navNode* dest = nullptr;
	navNode* current = nullptr;
	vector<navNode*> path;
	// !!! was 800 try to turn this down some hehe
	float dijkstraSpeed = 100; //how many updates to wait between calling dijkstra's algorithm
	float timeSinceLastDijkstra = -1;
	bool pathfinding = 0;
	float maxDistanceFromHome = 1400;
	float range = 3;
	int stuckTime = 0; //time since ai is trying to go somewhere but isn't moving
	int maxStuckTime = 8; //time waited before resolving stuckness
	float lastx = 0;
	float lasty = 0;

	int semisolid = 0; //push away close entities
	int storedSemisolidValue = 0; //semisolid has to be stored for ents that start as not solid but can later have values of 1 or 2

	//inventory
	//std::map<indexItem*, int> inventory = {};
	std::vector<std::pair<indexItem*, int> > inventory;
	
	//worlditem
	bool isWorlditem = 0;

	//used for setting the nararator
	bool persistentHidden = 0;

	//used for preserving the arms, but it can be used for other stuff
	bool persistentGeneral = 0;

	//misc scripting stuff
	bool banished = 0;
	bool solid = 0;
	bool canBeSolid = 0; //if the entity starts out as solid, which may be toggled
	bool semisolidwaittoenable = 0; //ignore collision with entities spawned at the player at first if not spawned during map loading
									// e.g. wards
	bool createdAfterMapLoad = 0; //stalker-enemies shouldn't be blocked by semisolid items created by the player
	bool navblock = 0; //disable overlapping navnodes when loaded into the level
	vector<navNode*> overlappedNodes; //a collection of navnodes that the entity overlaps. doors can disable the ones they overlap
	//when an entity is banished by a script, it will jump into the air and become intangible when its z velocity hits 0

	//default constructor is called automatically for children
	entity() {
		//M("entity()" );
	};

	entity(SDL_Renderer * renderer, string filename, float sizeForDefaults = 1) {
		M("entity()");

		ifstream file;
		//bool using_default = 0;
		this->name = filename;

		string loadstr;
		//try to open from local map folder first
		
		loadstr = "maps/" + g_mapdir + "/entities/" + filename + ".ent";
		const char* plik = loadstr.c_str();
		
		file.open(plik);
		
		if (!file.is_open()) {
			//load from global folder
			loadstr = "static/entities/" + filename + ".ent";
			const char* plik = loadstr.c_str();
			
			file.open(plik);
			
			if (!file.is_open()) {
				//just make a default entity
				//using_default = 1;
				string newfile = "static/entities/default.ent";
				file.open(newfile);
			}
		}
		
		string temp;
		file >> temp;
		string spritefilevar;
		if(temp.substr(0,3) == "sp-") {
			spritefilevar = "engine/" + temp + ".bmp";
		} else {
			spritefilevar = "static/sprites/" + temp + ".bmp";
		}

		//check local folder
		if(fileExists("maps/" + g_mapdir + "/sprites/" + filename + ".bmp")) {spritefilevar = "maps/" + g_mapdir + "/sprites/" + filename + ".bmp";}
		
		const char* spritefile = spritefilevar.c_str();
		float size;
		string comment;
		file >> comment;
		file >> size;

		file >> comment;
		file >> this->xagil;
		
		
		file >> comment;
		file >> this->xmaxspeed;
		
		file >> comment;
		file >> this->friction;
		baseFriction = friction;

		file >> comment;
		float twidth, theight, tzeight;
		file >> twidth;
		file >> theight;
		file >> tzeight;
		bounds.width = twidth * 64;
		bounds.height = theight * 55;
		bounds.zeight = tzeight * 64;
		
		
		file >> comment;
		file >> this->bounds.x;
		file >> this->bounds.y;

		file >> comment;
		file >> this->sortingOffset;
		this->baseSortingOffset = sortingOffset;
		file >> comment;
		float fsize;
		file >> fsize;
		shadow = new cshadow(renderer, fsize);
		
		
		this->shadow->owner = this;


		file >> comment;
		file >> shadow->xoffset;
		file >> shadow->yoffset;		
		
		int tempshadowSoffset;
		file >> comment;
		file >> tempshadowSoffset;

		file >> comment;
		file >> this->animspeed;
		file >> this->animlimit;

		file >> comment;
		file >> this->turnToFacePlayer;
		
		file >> comment;
		file >> this->framewidth;
		file >> this->frameheight;
		this->shadow->width = this->bounds.width * fsize;
		this->shadow->height = this->bounds.height * fsize;
		

		//bigger shadows have bigger sortingoffsets
		shadow->sortingOffset = 65 * (shadow->height / 44.4) + tempshadowSoffset;
		//sortingOffset += 8;

		file >> comment;
		file >> this->dynamic;
		bool solidifyHim = 0;
		
		file >> comment;
		file >> solidifyHim;

		file >> comment;
		file >> semisolid;
		if(!g_loadingATM) {
			if(semisolid) {
				storedSemisolidValue = semisolid;
				semisolidwaittoenable = 1; //disabled semi
				semisolid = 0;
			}
		}

		file >> comment;
		file >> navblock;
		if(solidifyHim) {
			this->solidify();
			this->canBeSolid = 1;        
		}
		


		file >> comment;
		file >> this->rectangularshadow;
		if(rectangularshadow) {shadow->texture = g_shadowTextureAlternate;} 

		file >> comment;
		file >> this->faction;
		if(faction != 0) {
			canFight = 1;
		}
		

		file >> comment;
		file >> this->weaponName;

		file >> comment;
		file >> this->agrod;

		file >> comment;
		file >> maxhp;
		hp = maxhp;

		file >> comment;
		file >> invincible;

		file >> comment;
		file >> cost;

		file >> comment;
		file >> essential;

		file >> comment;
		file >> this->persistentGeneral;

		file >> comment;
		file >> parentName;
		if(parentName != "null") {
			entity* hopeful = searchEntities(parentName);
			if(hopeful != nullptr) {
				this->isOrbital = 1;
				this->parent = hopeful;
				parent->children.push_back(this);
			
				curwidth = width;
				curheight = height;		
			
			}
		}

		file >> comment;
		file >> orbitRange;

		file >> comment;
		file >> orbitOffset;

	
		if(canFight) {
			//check if someone else already made the attack
			//bool cached = 0;
			hisweapon = new weapon(weaponName, this->faction != 0);
		}

		
		
		//load dialogue file
		
		string txtfilename = "";
		//open from local folder first
		if (fileExists("maps/" + g_mapdir + "/scripts/" + filename + ".txt")) {
			txtfilename = "maps/" + g_mapdir + "/scripts/" + filename + ".txt";
		} else {
			txtfilename = "static/scripts/" + filename + ".txt";
		}
		ifstream nfile(txtfilename);
		string line;

		//load voice
		string voiceSTR = "static/sounds/voice-normal.wav";
		voice = Mix_LoadWAV(voiceSTR.c_str());

		int overflowprotect = 5000;
		int i = 0;
		while(getline(nfile, line)) {
			sayings.push_back(line);
			if(i > overflowprotect) {
				E("Prevented overflow reading script for entity " + name);
				throw("Overflow");
			}	
			i++;
		}
	

		parseScriptForLabels(sayings);


		//has another entity already loaded this texture
		for (auto x : g_entities) {
			if(x->name == this->name && !x->isWorlditem) {
				texture = x->texture;
				this->asset_sharer = 1;
			}
		}
		if(!asset_sharer) {
			SDL_Surface* image = IMG_Load(spritefile);
			texture = SDL_CreateTextureFromSurface(renderer, image);
			SDL_FreeSurface(image);
		}
		
		
		this->width = size * framewidth;
		this->height = size * frameheight;

		//move shadow to feet
		shadow->xoffset += width/2 - shadow->width/2;
		shadow->yoffset += height - shadow->height/2;
		
		

		this->bounds.x += width/2 - bounds.width/2;
		this->bounds.y += height - bounds.height/2;
		

		shadow->width += g_extraShadowSize;
		shadow->height += g_extraShadowSize * XtoY;
		
		shadow->xoffset -= 0.5 * g_extraShadowSize;
		shadow->yoffset -= 0.5 * g_extraShadowSize * XtoY;

		shadow->x = x + shadow->xoffset;
		shadow->y = y + shadow->yoffset;

		int w, h;
		SDL_QueryTexture(texture, NULL, NULL, &w, &h);

		//make entities pop in unless this is a mapload
		if(!transition && !isOrbital) {
			curwidth = 0;
			curheight = 0;
		} else {
			curwidth = width;
			curheight = height;
		}

		if(1) {
			xframes = w / framewidth;
			yframes = h / frameheight;
		
	

			for (int j = 0; j < h; j+=frameheight) {
				for (int i = 0; i < w; i+= framewidth) {
					coord a;
					a.x = i;
					a.y = j;
					framespots.push_back(a);
				}
			}

			//if we don't have the frames just set anim to 0
			if(!(yframes > 3) ) {
				animation = 0;
				defaultAnimation = 0;
			}
		}

		//disabled nodes underneath if we are set to (e.g. doors)
		
		
		g_entities.push_back(this);
		
		file >> comment;
		//spawn everything on spawnlist
		int overflow = 100;
		for(;;) {

			string line;
			file >> line;
			if(line == "}" || line == "") {break;};
			overflow--;
			if(overflow < 0) {E("Bad spawnlist."); break;}
			entity* a = new entity(renderer, line);
			if(a->parentName == this->name) {
				a->parent = this;
			}
		}

		//music and radius
		file >> comment;
		string musicname = "0";
		file >> musicname;
		string fileExistsSTR;
		if(musicname != "0") {
			fileExistsSTR = "maps/" + g_mapdir + "/music/" + musicname + ".ogg";
			if(!fileExists(musicname)) {	
				fileExistsSTR = "static/music/" + musicname + ".ogg";
			}
			this->theme = Mix_LoadMUS(fileExistsSTR.c_str());
			g_musicalEntities.push_back(this);
		}
		file >> comment;
		file >> musicRadius;
		//convert blocks to worldpixels
		musicRadius *= 64;

		//worldsound
		file >> comment;
		//spawn everything on spawnlist
		overflow = 100;
		for(;;) {

			string line;
			file >> line;
			if(line == "}" || line == "") {break;};
			overflow--;
			if(overflow < 0) {E("Bad soundlist."); break;}
			worldsound* a = new worldsound(line, 0, 0);
			this->mobilesounds.push_back(a);
			a->owner = this;
		}

		//load ai-data 
		string AIloadstr;
		if(fileExists("maps/" + g_mapdir + "/ai/" + filename + ".ai")) {
			this->isAI = 1;
			AIloadstr = "maps/" + g_mapdir + "/ai/" + filename + ".ai";
		} else if(fileExists("static/ai/"+filename + ".ai")) {
			this->isAI = 1;
			AIloadstr = "static/ai/" + filename + ".ai";
		}
		if(this->isAI) {
			
			ifstream stream;
			const char* plik = AIloadstr.c_str();
			stream.open(plik);

			string line;
			string comment;

			stream >> comment; //abilities_and_radiuses_formate...
			stream >> comment; //abilities 
			stream >> comment; // {

			//take in each ability
			bool hasAtleastOneAbility = 0;
			for(;;) {
				if(! (stream >> line) ) {break;};
				if(line[0] == '}') {break;}
				hasAtleastOneAbility = 1;
				//line contains the name of an ability
				ability newAbility = ability(line);
				
				//they're stored as seconds in the configfile, but ms in the object
				float seconds = 0;
				stream >> seconds;
				newAbility.lowerRangeBound = seconds * 64;
				stream >> seconds;
				newAbility.upperRangeBound = seconds * 64;
				stream >> seconds;
				newAbility.lowerCooldownBound = seconds * 1000;
				stream >> seconds;
				newAbility.upperCooldownBound = seconds * 1000;


				char rsa;
				stream >> rsa;
				if(rsa == 'R') {
					newAbility.resetStableAccumulate = 0;
				} else if (rsa == 'S') {
					newAbility.resetStableAccumulate = 1;
				} else if (rsa == 'A') {
					newAbility.resetStableAccumulate = 2;
				}

				//

				newAbility.cooldownMS = (newAbility.lowerCooldownBound + newAbility.upperCooldownBound) / 2;
				this->myAbilities.push_back(newAbility);
			}

			//give us a way to call scripts if we have an ability
			if(hasAtleastOneAbility) {
				// !!! make another constructor that doesn't have a dialogbox
				myScriptCaller = new adventureUI(renderer);
				myScriptCaller->playersUI = 0;
				myScriptCaller->talker = this;

			}

			stream >> comment; //ai_index
			stream >> this->aiIndex;

			stream >> comment; //auto_Agro_radius
			stream >> autoAgroRadius;
			
			stream >> comment; //enrage_speed_bonus
			stream >> enrageSpeedbuff;

			stream >> comment; //target_faction
			stream >> targetFaction;

			stream.close();

		}
		

		file.close();
	}

	//for worlditems, load the ent file but use a texture by name
	entity(SDL_Renderer * renderer, int idk,  string texturename) {
		M("entity()");
		sortingOffset = 16;
		ifstream file;
		//bool using_default = 0;
		this->name = texturename;
		isWorlditem = 1;
		this->faction = -1;
		string loadstr;

		//load from global folder
		loadstr = "engine/worlditem.ent";
		const char* plik = loadstr.c_str();
		
		file.open(plik);
			

		
		string temp;
		file >> temp;
		string spritefilevar;
		

		spritefilevar = "static/items/" + texturename + ".bmp";
		SDL_Surface* image = IMG_Load(spritefilevar.c_str());
		texture = SDL_CreateTextureFromSurface(renderer, image);
		M(spritefilevar );

		
		string comment;
		file >> comment;
		float size;
		file >> size;

		file >> comment;
		file >> this->xagil;
		
		
		file >> comment;
		file >> this->xmaxspeed;
		
		file >> comment;
		file >> this->friction;

		file >> comment;
		float twidth, theight, tzeight;
		file >> twidth;
		file >> theight;
		file >> tzeight;
		bounds.width = twidth * 64;
		bounds.height = theight * 55;
		bounds.zeight = tzeight * 32;
		
		
		file >> comment;
		file >> this->bounds.x;
		file >> this->bounds.y;

		file >> comment;
		file >> this->sortingOffset;

		file >> comment;
		float fsize;
		file >> fsize;

		shadow = new cshadow(renderer, fsize);
		if(rectangularshadow) {shadow->texture = g_shadowTextureAlternate;} 
		
		this->shadow->owner = this;


		file >> comment;
		file >> shadow->xoffset;
		file >> shadow->yoffset;		
		
		int tempshadowSoffset;
		file >> comment;
		file >> tempshadowSoffset;

		file >> comment;
		file >> this->animspeed;
		file >> this->animlimit;

		file >> comment;
		file >> this->turnToFacePlayer;
		
		file >> comment;
		file >> this->framewidth;
		file >> this->frameheight;
		this->shadow->width = framewidth * fsize;
		this->shadow->height = framewidth * fsize * (1/p_ratio);

		//bigger shadows have bigger sortingoffsets
		shadow->sortingOffset = 65 * (shadow->height / 44.4) + tempshadowSoffset;
		//sortingOffset = 8;
		

		file >> comment;
		file >> this->dynamic;
		bool solidifyHim = 0;
		
		file >> comment;
		file >> solidifyHim;
		if(solidifyHim) {
			this->solidify();
		}
		
		file >> comment;
		file >> semisolid;

		file >> comment;
		file >> navblock;

		file >> comment;
		file >> this->rectangularshadow;

		file >> comment;
		file >> this->faction;

		if(faction != 0) {
			canFight = 1;
		}
		

		file >> comment;
		file >> this->weaponName;

		file >> comment;
		file >> this->agrod;

		file >> comment;
		file >> maxhp;

		hp = maxhp;

		file >> comment;
		file >> invincible;

		file >> comment;
		file >> cost;

		file >> comment;
		file >> essential;

		this->width = size * framewidth;
		this->height = size * frameheight;

		//move shadow to feet
		shadow->xoffset += width/2 - shadow->width/2;
		shadow->yoffset += height - shadow->height/2;
		
		
		this->bounds.x += width/2 - bounds.width/2;
		this->bounds.y += height - bounds.height/2;


		shadow->width += g_extraShadowSize;
		shadow->height += g_extraShadowSize * XtoY;
		
		shadow->xoffset -= 0.5 * g_extraShadowSize;
		shadow->yoffset -= 0.5 * g_extraShadowSize * XtoY;

		

		int w, h;
		SDL_QueryTexture(texture, NULL, NULL, &w, &h);

		xframes = 1;
		yframes = 1;
		frame = 0;
		framewidth = w;
		frameheight = h;
		coord a;
		a.x = 0;
		a.y = 0;
		framespots.push_back(a);
	
		SDL_FreeSurface(image);
		g_entities.push_back(this);
	}


	~entity() {
		M("~entity()" );
		if (!wallcap) {
			delete shadow;
		}
		if(!asset_sharer) {
			SDL_DestroyTexture(texture);
		}

		if(myScriptCaller!= nullptr){
			delete myScriptCaller;
		}

		for(auto ws : mobilesounds) {
			delete ws;
		}

		//delete hisweapon;
		//if this entity is talking or driving a script, a: the game is probably broken and b: we're about to crash
		
		if(this == g_talker) {
			g_forceEndDialogue = 1;
		}

		if(solid) {
			g_solid_entities.erase(remove(g_solid_entities.begin(), g_solid_entities.end(), this), g_solid_entities.end());
		}

		for(auto x : this->children) {
			x->tangible = 0;
		}

		g_entities.erase(remove(g_entities.begin(), g_entities.end(), this), g_entities.end());
		
	}

	float getOriginX() {
		return x + bounds.x + bounds.width/2;
	}

	float getOriginY() {
		return y + bounds.y + bounds.height/2;
	}

	rect getMovedBounds() {
		return rect(bounds.x + x, bounds.y + y, z, bounds.width, bounds.height, bounds.zeight);
	}

	void solidify() {
		//consider checking member field for solidness, and updating
		this->solid = 1;
		//shouldnt cause a crash anyway
		g_solid_entities.push_back(this);
	}

	void unsolidify() {
		this->solid = 0;
		g_solid_entities.erase(remove(g_solid_entities.begin(), g_solid_entities.end(), this), g_solid_entities.end());
	}

	void shoot();

	void render(SDL_Renderer * renderer, camera fcamera) {
		
		
		if(!tangible) {return;}
		if(this == protag) { g_protagHasBeenDrawnThisFrame = 1; }
		//if its a wallcap, tile the image just like a maptile
			
			

		//SDL_FPoint nowt = {0, 0};
		
		rect obj(((floor(x) -fcamera.x + (width-floor(curwidth))/2)* 1) , (((floor(y) - ((floor(curheight) * (XtoY) + (height * (1-XtoY)))) - (floor(z) + floatheight) * XtoZ) - fcamera.y) * 1), (floor(curwidth) * 1), (floor(curheight) * 1));		
		rect cam(0, 0, fcamera.width, fcamera.height);
		
		if(RectOverlap(obj, cam)) {
			//set frame from animation
			// animation is y, frameInAnimation is x
			if(hadInput) {
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
			SDL_FRect dstrect = { (float)obj.x, (float)obj.y, (float)obj.width, (float)obj.height};
			//genericmode has just one frame
			if(isWorlditem) {frame = 0;}
			
			

			if(framespots.size() > 1) {
				//int spinOffset = 0;
				int framePlusSpinOffset = frame;
				if(spinningMS > 0) {
					//change player frames to make a spin effect
				}
				
			
				SDL_Rect srcrect = {framespots[framePlusSpinOffset].x,framespots[framePlusSpinOffset].y, framewidth, frameheight};
				const SDL_FPoint center = {0 ,0};
				if(flashingMS > 0) {
					SDL_SetTextureColorMod(texture, 255, 255 * (1-((float)flashingMS/g_flashtime)), 255 * (1-((float)flashingMS/g_flashtime)));
				}

				if(texture != NULL) {
					SDL_RenderCopyExF(renderer, texture, &srcrect, &dstrect, 0, &center, flip);
				}
				if(flashingMS > 0) {
					SDL_SetTextureColorMod(texture, 255, 255, 255);
				}
			} else {
				if(flashingMS > 0) {
					SDL_SetTextureColorMod(texture, 255, 255 * (1 - ((float)flashingMS/g_flashtime)), 255 * (1-((float)flashingMS/g_flashtime)));
				}
				if(texture != NULL) {
					SDL_RenderCopyF(renderer, texture, NULL, &dstrect);
				}
				if(flashingMS > 0) {
					SDL_SetTextureColorMod(texture, 255, 255, 255);
				}
			}
		}
		
	}

	void move_up() {
		//y-=xagil;
		yaccel = -1* xagil;
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
		//y+=xagil;
		yaccel = xagil;
		if(shooting) { return;}
		down = true;
		up = false;
		hadInput = 1;
	}

	void move_left() {
		//x-=xagil;
		xaccel = -1 * xagil;
		//x -= 3;
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
		//x+=xagil;
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

	// !!! horrible implementation, if you have problems with pathfinding efficiency try making this not O(n) (lol)
	template <class T>
	T* Get_Closest_Node(vector<T*> array) {
		float min_dist = 0;
		T* ret = nullptr;
		bool flag = 1;

		int cacheX = getOriginX();
		int cacheY = getOriginY();

		//todo check for boxs
		if(array.size() == 0) {return nullptr;}
		for (long long unsigned int i = 0; i < array.size(); i++) {
			float dist = Distance(cacheX, cacheY, array[i]->x, array[i]->y);
			if(dist < min_dist || flag) {
				min_dist = dist;
				ret = array[i];
				flag = 0;
			}
		}
		return ret;
	}



	//returns a pointer to a door that the player used
	virtual door* update(vector<door*> doors, float elapsed) {
		if(!tangible) {return nullptr;}
		for(auto t : mobilesounds) {
			t->x = getOriginX();
			t->y = getOriginY();
		}
		if(isOrbital) {
			this->z = parent->z -10 - (parent->height - parent->curheight);
			
		
			float angle = convertFrameToAngle(parent->frame, parent->flip == SDL_FLIP_HORIZONTAL);

	

			//orbitoffset is the number of frames, counter-clockwise from facing straight down
			float fangle = angle;
			fangle += (float)orbitOffset * (M_PI/4);
			fangle = fmod(fangle , (2* M_PI)); 

			this->setOriginX(parent->getOriginX() + cos(fangle) * orbitRange);
			this->setOriginY(parent->getOriginY() + sin(fangle) * orbitRange);

			this->sortingOffset = baseSortingOffset + sin(fangle) * 21 + 10 + (parent->height - parent->curheight);

			if(yframes == 8) {
				this->animation = convertAngleToFrame(fangle);
				this->flip = SDL_FLIP_NONE;
			} else {
				this->flip = parent->flip;
				this->animation = parent->animation;
				if(yframes == 1) {
					this->animation = 0;
				}
			}
			
			//update shadow
			float heightfloor = 0;
			layer = max(z /64, 0.0f);
			layer = min(layer, (int)g_boxs.size() - 1);
			//should we fall?
			//bool should_fall = 1;
			float floor = 0;
			if(layer > 0) {
				//!!!
				rect thisMovedBounds = rect(bounds.x + x + xvel * ((double) elapsed / 256.0), bounds.y + y + yvel * ((double) elapsed / 256.0), bounds.width, bounds.height);
				//rect thisMovedBounds = rect(bounds.x + x, bounds.y + y, bounds.width, bounds.height);
				for (auto n : g_boxs[layer - 1]) {
					if(RectOverlap(n->bounds, thisMovedBounds)) {
						floor = 64 * (layer);
						break;
					}
				}
				for (auto n : g_triangles[layer - 1]) {
					if(TriRectOverlap(n, thisMovedBounds.x, thisMovedBounds.y, thisMovedBounds.width, thisMovedBounds.height)) {
						floor = 64 * (layer);
						break;
					}
				
				}
			
			
				float shadowFloor = floor;
				floor = max(floor, heightfloor);
			
				bool breakflag = 0;
				for(int i = layer - 1; i >= 0; i--) {
					for (auto n : g_boxs[i]) {
						if(RectOverlap(n->bounds, thisMovedBounds)) {
							shadowFloor = 64 * (i + 1);
							breakflag = 1;
							break;
						}
					}
					if(breakflag) {break;}
					for (auto n : g_triangles[i]) {
						if(TriRectOverlap(n, thisMovedBounds.x, thisMovedBounds.y, thisMovedBounds.width, thisMovedBounds.height)) {
							shadowFloor = 64 * (i + 1);
							breakflag = 1;
							break;
						}
					
					}
					if(breakflag) {break;}
				}
				if(breakflag == 0) {
					//just use heightmap
					shadowFloor = floor;
				}
				this->shadow->z = shadowFloor;
			} else { 
				this->shadow->z = heightfloor;
				floor = heightfloor;
			}
			shadow->x = x + shadow->xoffset;
			shadow->y = y + shadow->yoffset;



			return nullptr;
		}
		if(msPerFrame != 0) {
			msTilNextFrame += elapsed;
			if(msTilNextFrame > msPerFrame && xframes > 1) {
				msTilNextFrame = 0;
				frameInAnimation++;
				if(frameInAnimation == xframes) {
					if(loopAnimation) {
						if(scriptedAnimation) {
							frameInAnimation = 0;
						} else {
							frameInAnimation = 1;
						}
					} else {
						frameInAnimation = xframes - 1;
						msPerFrame = 0;
						//!!! slightly ambiguous. open to review later
						scriptedAnimation = 0;
					}
				}
			}
		}
		if(animate && !transition && animlimit != 0) {
			curwidth = (curwidth * 0.8 + width * 0.2) * ((sin(animtime*animspeed))   + (1/animlimit)) * (animlimit);
			curheight = (curheight * 0.8 + height* 0.2) * ((sin(animtime*animspeed + PI))+ (1/animlimit)) * (animlimit);
			animtime += elapsed;
			if(this == protag && ( pow( pow(xvel,2) + pow(yvel, 2), 0.5) > 40 ) && (1 - sin(animtime * animspeed) < 0.01 || 1 - sin(animtime * animspeed + PI) < 0.01)) {
				if(footstep_reset && grounded) {
					footstep_reset = 0;
					if(1 - sin(animtime * animspeed) < 0.04) {
						playSound(-1, g_footstep_a, 0);
					} else {
						playSound(-1, g_footstep_b, 0);
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
		
		
		//should we animate?
		if( (xaccel != 0 || yaccel != 0) || !grounded ) {
			animate = 1;
			if( (!scriptedAnimation) && grounded) {
				msPerFrame = 100;
			} else {
				msPerFrame = 0;
			}
		} else {  
			animate = 0;
			if(!scriptedAnimation) {
				msPerFrame = 0;
				frameInAnimation = 0;
			}
		}

		//should we enabled semisolidness? (for entities spawned after map-load far from protag)
		if(semisolidwaittoenable) {
			if(!CylinderOverlap(this->getMovedBounds(), protag->getMovedBounds())) {
				this->semisolid = storedSemisolidValue;
				this->semisolidwaittoenable = 0;
			}
		}
		
		if(!dynamic) { return nullptr; }


		//normalize accel vector
		float vectorlen = pow( pow(xaccel, 2) + pow(yaccel, 2), 0.5) / (xmaxspeed);
		if(xaccel != 0) {
			xaccel /=vectorlen;
		}
		if(yaccel != 0) {
			yaccel /=vectorlen;
			yaccel /= p_ratio;
		}
		if(xaccel > 0) {
			xvel += xaccel * ((double) elapsed / 256.0);
		}
		
		if(xaccel < 0) {
			xvel += xaccel * ((double) elapsed / 256.0);
		}

		if(yaccel > 0) {
			yvel += yaccel* ((double) elapsed / 256.0);
		}
		
		if(yaccel < 0) {
			yvel += yaccel* ((double) elapsed / 256.0);
		}

		rect movedbounds;
		bool ycollide = 0;
		bool xcollide = 0;
		

		

		//turn off boxs if using the map-editor
		if(boxsenabled) {
			//..check door
			if(this == protag) {
				for (int i = 0; i < (int)doors.size(); i++) {	
					//update bounds with new posj
					rect movedbounds = rect(bounds.x + x + xvel * ((double) elapsed / 256.0), bounds.y + y  + yvel * ((double) elapsed / 256.0), bounds.width, bounds.height);
					//did we walk into a door?
					if(RectOverlap(movedbounds, doors[i]->bounds) && (protag->z > doors[i]->z && protag->z < doors[i]->z + doors[i]->zeight)  ) {
						//take the door.
						return doors[i];
					}
				}
			}
			for (auto n : g_solid_entities) {	
				if(n == this) {continue;}
				if(!n->tangible) {continue;}
				//update bounds with new pos
				rect thismovedbounds = rect(bounds.x + x, bounds.y + y  + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
				rect thatmovedbounds = rect(n->bounds.x + n->x, n->bounds.y + n->y, n->bounds.width, n->bounds.height);
				//uh oh, did we collide with something?
				if(RectOverlap(thismovedbounds, thatmovedbounds)) {
					ycollide = true;
					yvel = 0;
				}
				//update bounds with new pos
				thismovedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y, bounds.width, bounds.height);
				//uh oh, did we collide with something?
				if(RectOverlap(thismovedbounds, thatmovedbounds)) {
					xcollide = true;
					xvel = 0;
				}
			}

			vector<box*> boxesToUse = {};
			
			int usedCZ = 0;
			rect movedbounds = rect(bounds.x + x, bounds.y + y  + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
			//see if we overlap any collisionzones
			for(auto x : g_collisionZones) {
				if(RectOverlap(movedbounds, x->bounds)) {
					usedCZ++;
					boxesToUse.insert(boxesToUse.end(), x->guests[layer].begin(), x->guests[layer].end());
				}
			}
			

			if(usedCZ == 0) {
				for (int i = 0; i < (int)g_boxs[layer].size(); i++) {	
					
					//update bounds with new pos
					rect movedbounds = rect(bounds.x + x, bounds.y + y  + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
					
					//don't worry about boxes if we're not even close
					rect sleepbox = rect(g_boxs[layer].at(i)->bounds.x - 150, g_boxs[layer].at(i)->bounds.y-150, g_boxs[layer].at(i)->bounds.width+300, g_boxs[layer].at(i)->bounds.height+300);
					if(!RectOverlap(sleepbox, movedbounds)) {continue;} 

					//uh oh, did we collide with something?
					if(RectOverlap(movedbounds, g_boxs[layer].at(i)->bounds)) {
						ycollide = true;
						yvel = 0;
								


						
					}
					//update bounds with new pos
					movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y, bounds.width, bounds.height);
					//uh oh, did we collide with something?
					if(RectOverlap(movedbounds, g_boxs[layer].at(i)->bounds)) {
						//box detected
						xcollide = true;
						xvel = 0;
						
					}

					
				}
				for (int i = 0; i < (int)g_boxs[layer].size(); i++) {
					movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
					//uh oh, did we collide with something?
					if(RectOverlap(movedbounds, g_boxs[layer].at(i)->bounds)) {
						//box detected
						xcollide = true;
						ycollide = true;
						xvel = 0;
						yvel = 0;
						continue;
					}
				}
			} else {
				for (int i = 0; i < (int)boxesToUse.size(); i++) {	
					
					//update bounds with new pos
					rect movedbounds = rect(bounds.x + x, bounds.y + y  + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
					
					//don't worry about boxes if we're not even close
					rect sleepbox = rect(boxesToUse.at(i)->bounds.x - 150, boxesToUse.at(i)->bounds.y-150, boxesToUse.at(i)->bounds.width+300, boxesToUse.at(i)->bounds.height+300);
					if(!RectOverlap(sleepbox, movedbounds)) {continue;} 

					//uh oh, did we collide with something?
					if(RectOverlap(movedbounds, boxesToUse.at(i)->bounds)) {
						ycollide = true;
						yvel = 0;
								


						
					}
					//update bounds with new pos
					movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y, bounds.width, bounds.height);
					//uh oh, did we collide with something?
					if(RectOverlap(movedbounds, boxesToUse.at(i)->bounds)) {
						//box detected
						xcollide = true;
						xvel = 0;
						
					}

					
				}
				for (int i = 0; i < (int)boxesToUse.size(); i++) {
					movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
					//uh oh, did we collide with something?
					if(RectOverlap(movedbounds, boxesToUse.at(i)->bounds)) {
						//box detected
						xcollide = true;
						ycollide = true;
						xvel = 0;
						yvel = 0;
						continue;
					}
				}
			}

			//if we didnt use a cz, use all boxes
			
			
			
			//how much to push the player to not overlap with triangles.
			float ypush = 0;
			float xpush = 0;
			float jerk = -abs( pow(pow(yvel,2) + pow(xvel, 2), 0.5))/2; //how much to push the player aside to slide along walls
			//float jerk = -1;
			//!!! try counting how many triangles the ent overlaps with per axis and disabling jerking
			//if it is more than 1
			
			//and try setting jerk to a comp of the ent velocity to make the movement faster and better-scaling

			//can still get stuck if we walk diagonally into a triangular wall

			for(auto n : g_triangles[layer]){
				rect movedbounds = rect(bounds.x + x, bounds.y + y  + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
				if(TriRectOverlap(n, movedbounds.x, movedbounds.y, movedbounds.width, movedbounds.height)) {
					//if we move the player one pixel up will we still overlap?
					if(n->type == 3 || n->type == 2)  {
						xpush = jerk;
					}

					if(n->type == 0 || n->type == 1) {
						xpush = -jerk; ;
					}
					
					
					ycollide = true;
					yvel = 0;
					
				}

				movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y, bounds.width, bounds.height);
				if(TriRectOverlap(n, movedbounds.x, movedbounds.y, movedbounds.width, movedbounds.height)) {
					//if we move the player one pixel up will we still overlap?
					if(n->type == 1 || n->type == 2) {
						ypush = jerk;
					}

					if(n->type == 0 || n->type == 3){
						ypush = -jerk;
					}
					
					xcollide = true;
					xvel = 0;
					
				}

				movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
				if(TriRectOverlap(n, movedbounds.x, movedbounds.y, movedbounds.width, movedbounds.height)) {					
					xcollide = true;
					ycollide = true;
					xvel = 0;
					yvel = 0;
					continue;

					
				}
				
			}
			
			yvel += ypush;
			xvel += xpush;
			// yaccel += 12 * ypush;
			// xaccel += 12 * xpush;
			
		}
		



		if(!ycollide && !transition) { 
			y+= yvel * ((double) elapsed / 256.0);
			
		}

		//when coordinates are bungled, it isnt happening here
		if(!xcollide && !transition) { 
			x+= xvel * ((double) elapsed / 256.0);
		}
		if(slowSeconds > 0) {
			slowSeconds -= elapsed/1000;
			friction = baseFriction * slowPercent;
		} else {
			friction = baseFriction;
		}
		
		
		if(grounded) {
			yvel *= pow(friction, ((double) elapsed / 256.0));
			xvel *= pow(friction, ((double) elapsed / 256.0));
		} else {
			yvel *= pow(friction*this->currentAirBoost, ((double) elapsed / 256.0));
			xvel *= pow(friction*this->currentAirBoost, ((double) elapsed / 256.0));
			this->currentAirBoost += (g_deltaBhopBoost * ((double) elapsed / 256.0)) * min(pow( pow(x,2) + pow(y, 2), 0.2) / xmaxspeed, 1.0);
			if(this->currentAirBoost > g_maxBhoppingBoost) {
				this->currentAirBoost = g_maxBhoppingBoost;
			}
			// !!! make this not suck
		}

		float heightfloor = 0; //filled with floor z from heightmap
		if(g_heightmaps.size() > 0 /*&& update_z_time < 1*/) {
			bool using_heightmap = 0;
			bool heightmap_is_tiled = 0;
			tile* heighttile = nullptr;
			int heightmap_index = 0;
			
			rect tilerect;
			rect movedbounds;
			//movedbounds = rect(this->x + xvel * ((double) elapsed / 256.0), this->y + yvel * ((double) elapsed / 256.0) - this->bounds.height, this->bounds.width, this->bounds.height);
			movedbounds = rect(this->getOriginX(), this->getOriginY(), 0, 0);
			//get what tile entity is on
			//this is poorly set up. It would be better if heightmaps were assigned to tiles, obviously, with a pointer
			bool breakflag = 0;

			for (int i = (int)g_tiles.size() - 1; i >= 0; i--) {
				if(g_tiles[i]->fileaddress == "textures/marker.bmp") {continue; }
				tilerect = rect(g_tiles[i]->x, g_tiles[i]->y, g_tiles[i]->width, g_tiles[i]->height);
				
				if(RectOverlap(tilerect, movedbounds)) {
					for (int j = 0; j < (int)g_heightmaps.size(); j++) {
						//M("looking for a heightmap");
						//D(g_heightmaps[j]->name);
						//D(g_tiles[i]->fileaddress);
						if(g_heightmaps[j]->name == g_tiles[i]->fileaddress) {
							//M("found it");
							heightmap_index = j;
							using_heightmap = 1;
							breakflag = 1;
							heightmap_is_tiled = g_tiles[i]->wraptexture;
							heighttile = g_tiles[i];
							break;
						}
					}
					if(breakflag) {break;} //we found it, leave

					//current texture has no mask, keep looking
				}
			}
		

			
			update_z_time = max_update_z_time;
			//update z position
			SDL_Color rgb = {0, 0, 0};
			heightmap* thismap = g_heightmaps[heightmap_index];
			Uint8 maxred = 0;
			if(using_heightmap) {
				//try each corner;
				//thismap->image->w;
				//code for middle
				//Uint32 data = thismap->heighttilegetpixel(thismap->image, (int)(this->x + xvel * ((double) elapsed / 256.0) + 0.5 * this->width) % thismap->image->w, (int)(this->y + yvel * ((double) elapsed / 256.0) - 0.5 * this->bounds.height) % thismap->image->h);
				Uint32 data;
				if(heightmap_is_tiled) {
					data = thismap->getpixel(thismap->image, (int)(getOriginX()) % thismap->image->w, (int)(getOriginY()) % thismap->image->h);
				} else {
					//tile is not tiled, so we have to get clever with the heighttile pointer

					data = thismap->getpixel(thismap->image, (int)( ((this->getOriginX() - heighttile->x) /heighttile->width) * thismap->image->w), (int)( ((this->getOriginY() - heighttile->y) /heighttile->height) * thismap->image->h));
				}
				SDL_GetRGB(data, thismap->image->format, &rgb.r, &rgb.g, &rgb.b);
				if(RectOverlap(tilerect, movedbounds)) {
					maxred = rgb.r;
				}
				// //code for each corner:
				// Uint32 data = thismap->getpixel(thismap->image, (int)(this->x + xvel * ((double) elapsed / 256.0)) % thismap->image->w, (int)(this->y + yvel * ((double) elapsed / 256.0)) % thismap->image->h);
				// SDL_GetRGB(data, thismap->image->format, &rgb.r, &rgb.g, &rgb.b);
				
				// movedbounds = rect(this->x + xvel * ((double) elapsed / 256.0), this->y + yvel * ((double) elapsed / 256.0), 1, 1);
				// if(RectOverlap(tilerect, movedbounds)) {
				// 	maxred = max(maxred, rgb.r);
				// }

				// data = thismap->getpixel(thismap->image, (int)(this->x + xvel * ((double) elapsed / 256.0) + this->bounds.width) % thismap->image->w, (int)(this->y + yvel * ((double) elapsed / 256.0)) % thismap->image->h);
				// SDL_GetRGB(data, thismap->image->format, &rgb.r, &rgb.g, &rgb.b);
				
				// movedbounds = rect(this->x + xvel * ((double) elapsed / 256.0) + this->bounds.width, this->y + yvel * ((double) elapsed / 256.0), 1, 1);
				// if(RectOverlap(tilerect, movedbounds)) {
				// 	maxred = max(maxred, rgb.r);
				// }

				// data = thismap->getpixel(thismap->image, (int)(this->x + xvel * ((double) elapsed / 256.0)) % thismap->image->w, (int)(this->y + yvel * ((double) elapsed / 256.0) - this->bounds.height) % thismap->image->h);
				// SDL_GetRGB(data, thismap->image->format, &rgb.r, &rgb.g, &rgb.b);
				
				// movedbounds = rect(this->x + xvel * ((double) elapsed / 256.0), this->y + yvel * ((double) elapsed / 256.0) - this->bounds.height, 1, 1);
				// if(RectOverlap(tilerect, movedbounds)) {
				// 	maxred = max(maxred, rgb.r);
				// }

				// data = thismap->getpixel(thismap->image, (int)(this->x + xvel * ((double) elapsed / 256.0) + this->bounds.width) % thismap->image->w, (int)(this->y + yvel * ((double) elapsed / 256.0) - this->bounds.height) % thismap->image->h);
				// SDL_GetRGB(data, thismap->image->format, &rgb.r, &rgb.g, &rgb.b);
				
				// movedbounds = rect(this->x + xvel * ((double) elapsed / 256.0) + this->bounds.width, this->y + yvel * ((double) elapsed / 256.0) - this->bounds.height, 1, 1);
				// if(RectOverlap(tilerect, movedbounds)) {
				// 	maxred = max(maxred, rgb.r);
				// }
				
			}
			//oldz = this->z;
			if(using_heightmap) {
				heightfloor = ((maxred * thismap->magnitude));
			}
			
		} else {
			//update_z_time--;
			//this->z = ((oldz) + this->z) / 2 ;
			
		}
		
		layer = max(z /64, 0.0f);
		layer = min(layer, (int)g_boxs.size() - 1);
		//should we fall?
		//bool should_fall = 1;
		float floor = 0;
		if(layer > 0) {
			//!!!
			rect thisMovedBounds = rect(bounds.x + x + xvel * ((double) elapsed / 256.0), bounds.y + y + yvel * ((double) elapsed / 256.0), bounds.width, bounds.height);
			//rect thisMovedBounds = rect(bounds.x + x, bounds.y + y, bounds.width, bounds.height);
			for (auto n : g_boxs[layer - 1]) {
				if(RectOverlap(n->bounds, thisMovedBounds)) {
					floor = 64 * (layer);
					break;
				}
			}
			for (auto n : g_triangles[layer - 1]) {
				if(TriRectOverlap(n, thisMovedBounds.x, thisMovedBounds.y, thisMovedBounds.width, thisMovedBounds.height)) {
					floor = 64 * (layer);
					break;
				}
				
			}
			
			
			float shadowFloor = floor;
			floor = max(floor, heightfloor);
			
			bool breakflag = 0;
			for(int i = layer - 1; i >= 0; i--) {
				for (auto n : g_boxs[i]) {
					if(RectOverlap(n->bounds, thisMovedBounds)) {
						shadowFloor = 64 * (i + 1);
						breakflag = 1;
						break;
					}
				}
				if(breakflag) {break;}
				for (auto n : g_triangles[i]) {
					if(TriRectOverlap(n, thisMovedBounds.x, thisMovedBounds.y, thisMovedBounds.width, thisMovedBounds.height)) {
						shadowFloor = 64 * (i + 1);
						breakflag = 1;
						break;
					}
					
				}
				if(breakflag) {break;}
			}
			if(breakflag == 0) {
				//just use heightmap
				shadowFloor = floor;
			}
			this->shadow->z = shadowFloor;
		} else { 
			this->shadow->z = heightfloor;
			floor = heightfloor;
		}

		//try ramps?
		//!!! can crash if the player gets too high
		
		if(layer < g_layers) { 
			for(auto r : g_ramps[this->layer]) {
				rect a = rect(r->x, r->y, 64, 55);
				rect movedBounds = rect(bounds.x + x, bounds.y + y + yvel * ((double) elapsed / 256.0), bounds.width, bounds.height);
				if(RectOverlap(movedBounds, a)) {
					if(r->type == 0) {
						//contribute to protag z based on how far we are along
						//the y axis
						float push = (55 - abs((((float)movedBounds.y - (float)r->y ))))/55;
						
						M(push);
						float possiblefloor = r->layer * 64 + 64 * push; 
						if( abs(this->z - possiblefloor ) < 15) {
							floor = possiblefloor; 
							this->shadow->z = floor + 1;
						}
						
					} else {
						if(r->type == 1) {
							float push = (64 - abs((( (float)(movedBounds.x + movedBounds.width) - (float)(r->x + 64) ))))/64;
							
							float possiblefloor = r->layer * 64 + 64 * push; 
							if( abs(this->z - possiblefloor ) < 15) {
								floor = possiblefloor; 
								this->shadow->z = floor + 1;
							}
						} else {
							if(r->type == 2) {
								//contribute to protag z based on how far we are along
								//the y axis
								float push = (55 - abs((( (float)(movedBounds.y + movedBounds.height) - (float)(r->y + 55) ))))/55;
								
								float possiblefloor = r->layer * 64 + 64 * push; 
								if( abs(this->z - possiblefloor ) < 15) {
									floor = possiblefloor; 
									this->shadow->z = floor + 1;
								}
								
							} else {
								float push = (64 - abs((( (float)(movedBounds.x) - (float)(r->x) ))))/64;
								
								float possiblefloor = r->layer * 64 + 64 * push; 
								if( abs(this->z - possiblefloor ) < 15) {
									floor = possiblefloor; 
									this->shadow->z = floor + 1;
								}
							}
						}
					}
				}
			}
		}
		
		if(z > floor + 1) {
			zaccel -= g_gravity * ((double) elapsed / 256.0);
			grounded = 0;
		} else {
			// !!! maybe revisit this to let "character" entities have fallsounds
			if(grounded == 0 && this == protag) {
				//play landing sound
				playSound(-1, g_land, 0);

				if(!storedJump) { 
					//penalize the player for not bhopping
					protag->slowPercent = g_jump_afterslow;
					protag->slowSeconds = g_jump_afterslow_seconds;
					protag->currentAirBoost = g_defaultBhoppingBoost;
						
				}
			}
			grounded = 1;
			zvel = max(zvel, 0.0f);
			zaccel = max(zaccel, 0.0f);
			
		}
		
		
		zvel += zaccel * ((double) elapsed / 256.0);

		//for banish animation from scripts
		if(this->banished && zvel <= 0) {
			this->dynamic = 0; 
			SDL_SetTextureAlphaMod(this->texture, 127); 
			//if we are solid, disable nodes beneath
			if(this->canBeSolid) {

				for(auto x : overlappedNodes) {
					
					x->enabled = 1;
					// for(auto y : x->friends) {
					// 	y->enabled = 1;
					// }
				}
				
			}
			return nullptr;
		}
		
		//zvel *= pow(friction, ((double) elapsed / 256.0));
		z += zvel * ((double) elapsed / 256.0);
		z = max(z, floor + 1);
		
		
		z = max(z, heightfloor);
		layer = max(z /64, 0.0f);
		layer = min(layer, (int)g_boxs.size() - 1);

		shadow->x = x + shadow->xoffset;
		shadow->y = y + shadow->yoffset;

		//update combat
		if(isWorlditem) {return nullptr;}

		
		hisweapon->comboResetMS+=elapsed;

		if(shooting) {
			//spawn shot.
			shoot();
		}

		//check for everyone, even if they are invincible
		if(1) {
			//check for projectile box
			vector<projectile*> projectilesToDelete;
			for(auto x : g_projectiles) {
				rect thatMovedBounds = rect(x->x + x->bounds.x, x->y + x->bounds.y, x->bounds.width, x->bounds.height);
				thatMovedBounds.z = x->z;
				thatMovedBounds.zeight = thatMovedBounds.width * XtoZ;
				rect thisMovedBounds = rect(this->x + bounds.x, y + bounds.y, bounds.width, bounds.height);
				thisMovedBounds.z = this->z;
				thisMovedBounds.zeight = this->bounds.zeight;
				if(x->owner->faction != this->faction && RectOverlap3d(thatMovedBounds, thisMovedBounds)) {

					//destroy projectile
					projectilesToDelete.push_back(x);

					if(!invincible) {
						//take damage
						this->hp -= x->gun->damage;
						this->flashingMS = g_flashtime;
						if(this->faction != 0) {
							playSound(1, g_enemydamage, 0);
						} else {
							if(this == protag) {
								playSound(2, g_playerdamage, 0);
							} else {
								playSound(3, g_npcdamage, 0);
							}
						}
					}

					if(this->weaponName == "unarmed") {break;}	

					//under certain conditions, agro the entity hit and set his target to the shooter
					if(target == nullptr) {
						target = x->owner;
						targetFaction = x->owner->faction;
						agrod = 1;

						//agro all of the boys on this's team who aren't already agrod, and set their target to a close entity from x's faction
						//WITHIN A RADIUS, because it doesnt make sense to agro everyone on the map.
						
						for (auto y : g_entities) {
							if(y->tangible && y != this && y->faction == this->faction && (y->agrod == 0 || y->target == nullptr) && XYWorldDistance(y->x, y->y, this->x, this->y) < g_earshot) {
								y->targetFaction = x->owner->faction;
								y->agrod = 1;
							}
						}
						
					}
				}
			}
			for(auto x:projectilesToDelete) {
				delete x;
			}
		}
		

		//alert nearby friends who arent fighting IF we are agrod
		if(agrod) {
			for (auto y : g_entities) {
				if(y->tangible && y != this && y->faction == this->faction && (y->agrod == 0 || y->target == nullptr) && XYWorldDistance(y->x, y->y, this->x, this->y) < g_earshot) {
					y->agrod = 1;

					if(this->target != nullptr) {
						y->targetFaction = this->targetFaction;
						if(y->target == nullptr) {
							y->target = this->target;
						}
					}
				}
			}
		}

		if(isAI) {

			//check the auto-agro-range
			if(target == nullptr) {
				for(auto x : g_entities) {
					if(x->faction == this->targetFaction) {
						if(XYWorldDistance(x->getOriginX(), x->getOriginY(), this->getOriginX(), this->getOriginY()) < this->autoAgroRadius * 64) {
							
							if(LineTrace(x->getOriginX(), x->getOriginY(), this->getOriginX(), this->getOriginY(), false, 30, 0, 10, false)) {
								this->traveling = 0;
								this->target = x;
								this->agrod = 1;
							}
						}
					}
				}
			}



			//likely has abilities to use
			//are any abilities ready?
			for(auto &x : myAbilities) {
				//accumulate-abilities will always decrease CD
				if(x.resetStableAccumulate == 2) {
					x.cooldownMS -= elapsed;
				}
				if(target == nullptr) {if(x.resetStableAccumulate == 0) {x.cooldownMS = x.upperCooldownBound;};continue;}
				float dist = XYWorldDistance(this->getOriginX(), this->getOriginY(), target->getOriginX(), target->getOriginY());
				float inRange = 0;
				if(dist <= x.upperRangeBound  && dist >= x.lowerRangeBound) {
					inRange = 1;
					if(x.resetStableAccumulate != 2) {
						x.cooldownMS -= elapsed;
					}
				} else {
					//reset-abilities are reset when out of range
					if(x.resetStableAccumulate == 0) {
						x.cooldownMS = (x.lowerCooldownBound + x.upperCooldownBound)/2;
					}
				}

				if(x.cooldownMS < 0) {
					
					//do we acknowledge the player's existance?
					if(target != nullptr) {
					
						//are we in range to the player?
						
						if( inRange && !myScriptCaller->executingScript) {

							I(this->name + " used " + x.name + " at " + target->name + ".");
							
							//this->dialogue_index = 1;
							this->sayings = x.script;
							this->myScriptCaller->executingScript = 1;
							this->dialogue_index = -1;	
							this->myScriptCaller->talker = this;
							this->myScriptCaller->continueDialogue();
							if(x.upperCooldownBound - x.lowerCooldownBound <= 0) {
								x.cooldownMS = x.lowerCooldownBound;
							} else {
								x.cooldownMS = ( fmod(rand(),  (x.upperCooldownBound - x.lowerCooldownBound)) ) + x.lowerCooldownBound;
							}
							//I("Set his cooldown to " + to_string(x.cooldownMS)); 
							//I(x.upperCooldownBound);
							//I(x.lowerCooldownBound);
						}
					
					}
				}
			}
		}

		//re-evaluate target
		if(agrod && target == nullptr && targetFaction != faction) {
			//find a nearby target
			//!!! inefficient
			for(auto x : g_entities) {
				if(x->faction == this->targetFaction && XYWorldDistance(this->x, this->y, x->x, x->y) < g_earshot) {
					this->target = x;
				}
			}

		} 


		//de-agro
		if(agrod && target != nullptr) {
			if(XYWorldDistance(this->x, this->y, target->x, target->y) > g_earshot * 1.5) {
				this->target = nullptr;
			}
		}

		//apply statuseffect

		
		//check if he has died
		if(hp <= 0) {
			if(this != protag) {
				tangible = 0;
				return nullptr;
			}
		}



		if(!canFight) {
			return nullptr;
		}



		//push him away from close entities
		//if we're even slightly stuck, don't bother
		if(stuckTime < 2 && this->dynamic) {
			
			for(auto x : g_entities) {
				if(this == x) {continue;} 
				bool m = CylinderOverlap(this->getMovedBounds(), x->getMovedBounds());
				
				//entities with a semisolid value of 2 are only solid to the player
				bool solidfits = 0;
				if(this == protag) {
					solidfits = x->semisolid;
				} else {
					solidfits = (x->semisolid == 1);
				}
				if(solidfits && x->tangible && m) {		
					//push this one slightly away from x
					float r = pow( max(Distance(getOriginX(), getOriginY(), x->getOriginX(), x->getOriginY()), (float)10.0 ), 2);
					float mag =  30000/r;
					float xdif = (this->getOriginX() - x->getOriginX());
					float ydif = (this->getOriginY() - x->getOriginY());
					float len = pow( pow(xdif, 2) + pow(ydif, 2), 0.5);
					float normx = xdif/len;
					float normy = ydif/len;
					
					if(!isnan(mag * normx) && !isnan(mag * normy)) {
						xvel += normx * mag;
						yvel += normy * mag;
					}

				}
			}
		}
		flashingMS -= elapsed;
		if(this->inParty) {
			return nullptr;
		}

		//shooting ai
		if(agrod) {
			//do we have a target?
			if(target != nullptr) {
				//check if target is still valid
				if(target->hp <= 0 || !target->tangible) {
					//can we get a new target from the same faction that we are agrod against?
					bool setNewTarget = 0;
					for(auto x : g_entities) {
						if(x->tangible && x->faction == target->faction && LineTrace(this->getOriginX(), this->getOriginY(), x->getOriginX(), x->getOriginY(), false, 30, this->layer, 10, 0)) {
							target = x;
							setNewTarget = 1;
							break;
						} 
					}
					if(!setNewTarget){
						for(auto x : g_entities) {
							if(x->tangible && x->faction == target->faction) {
								target = x;
								setNewTarget = 1;
								break;
							} 
						}
					}
					if(!setNewTarget) {
						target = nullptr;
						//agrod = 0;
						//I'd like to check if there are entities we can't see but we can path too but its so much work :(
						
						Destination = nullptr;
					}
				} else {
					//re-evaluate target if someone else has delt high damage to us recently

					//this constant is what factor of the range must be had to the player
					//in these types of games, humans seem to shoot even when they are out 
					// of range, so lets go with that
					shooting = 0;
					float distanceToTarget = XYWorldDistance(target->getOriginX(), target->getOriginY(), getOriginX(), getOriginY());
					if(distanceToTarget < this->hisweapon->attacks[hisweapon->combo]->range) {
						shooting = 1;
					}
					
					float xvector;
					float yvector;
					//bool recalcAngle = 0; //should we change his angle in the first place?
					//combatrange is higher than shooting range because sometimes that range is broken while a fight is still happening, so he shouldnt turn away
					if(distanceToTarget < this->hisweapon->attacks[hisweapon->combo]->range * 1.7) {
						//set vectors from target
						xvector = (this->getOriginX()) - (target->getOriginX());
						yvector = (this->getOriginY()) - (target->getOriginY());
						recalcAngle = 1;
					} else {
						//set vectors from velocity
						xvector = -walkingxaccel;
						yvector = -walkingyaccel;
						walkingxaccel = 0;
						walkingxaccel = 0;
						//if he's not traveling very fast it looks natural to not change angle
						//recalcAngle+= elapsed;
						//if(Distance(0,0,xaccel, yaccel) > this->xmaxspeed * 0.8) {recalcAngle = 1;}
					}
					
					if(Distance(0,0,xvector, yvector) > 0) {
						recalcAngle = -1000; //update every second
						float angle = atan2(yvector, xvector);
					
						flip = SDL_FLIP_NONE;
						up = 0; down = 0; left = 0; right = 0;
						if(angle < -7 * M_PI / 8 || angle >= 7 * M_PI / 8) {
							animation = 2;
							flip = SDL_FLIP_HORIZONTAL;
							right = 1;
							//M("A");
						} else if (angle < 7 * M_PI / 8 && angle >= 5 * M_PI / 8) {
							animation = 1;
							flip = SDL_FLIP_HORIZONTAL;
							right = 1;
							up = 1;
							//M("B");
						} else if (angle < 5 * M_PI / 8 && angle >= 3 * M_PI / 8) {
							animation = 0;
							up = 1;
							//M("C");
						} else if (angle < 3 * M_PI / 8 && angle >= M_PI / 8) {
							animation = 1;
							up = 1;
							left = 1;
							//M("D");
						} else if (angle < M_PI / 8 && angle >= - M_PI / 8) {
							animation = 2;
							left = 1;
							//M("E");
						} else if (angle < - M_PI / 8 && angle >= - 3 * M_PI / 8) {
							animation = 3;
							left = 1;
							down = 1;
							//M("F");
						} else if (angle < - 3 * M_PI / 8 && angle > - 5 * M_PI / 8) {
							animation = 4;
							down = 1;
							//M("G");
						} else if (angle < - 5 * M_PI / 8 && angle > - 7 * M_PI / 8) {
							flip = SDL_FLIP_HORIZONTAL;
							animation = 3;
							right = 1;
							down = 1;
							//M("H");
						}
					}

					//now that we have a direction, shoot
					if(shooting) {
						shoot();
					}
					
				}
			} else {
				//another placeholder - target is protag
				if(faction != 0) {
					extern entity* protag;
					target = protag;
				}
				if(targetFaction != -1) {
					//set target to first visible enemy from target faction
					for(auto x : g_entities) {
						if(x->faction == this->targetFaction ) {
							//found an entity that we have vision to
							this->target = x;
						}
					}
				}
			}
		} else if(dynamic) {
			
			//code for becoming agrod when seeing a hostile entity will go here
			//face the direction we are moving
			//bool recalcAngle = 0; //should we change his angle in the first place?
			//combatrange is higher than shooting range because sometimes that range is broken while a fight is still happening, so he shouldnt turn away
			
			//set vectors from velocity
			float xvector = -walkingxaccel;
			float yvector = -walkingyaccel;

			//if he's not traveling very fast it looks natural to not change angle
			//recalcAngle = 1
			if(Distance(0,0,xvector, yvector) > 0) {
				recalcAngle = -1000;
				float angle = atan2(yvector, xvector);
				flip = SDL_FLIP_NONE;
				up = 0; down = 0; left = 0; right = 0;
				if(angle < -7 * M_PI / 8 || angle >= 7 * M_PI / 8) {
					animation = 2;
					flip = SDL_FLIP_HORIZONTAL;
					right = 1;
					//M("A");
				} else if (angle < 7 * M_PI / 8 && angle >= 5 * M_PI / 8) {
					animation = 1;
					flip = SDL_FLIP_HORIZONTAL;
					right = 1;
					up = 1;
					//M("B");
				} else if (angle < 5 * M_PI / 8 && angle >= 3 * M_PI / 8) {
					animation = 0;
					up = 1;
					//M("C");
				} else if (angle < 3 * M_PI / 8 && angle >= M_PI / 8) {
					animation = 1;
					up = 1;
					left = 1;
					//M("D");
				} else if (angle < M_PI / 8 && angle >= - M_PI / 8) {
					animation = 2;
					left = 1;
					//M("E");
				} else if (angle < - M_PI / 8 && angle >= - 3 * M_PI / 8) {
					animation = 3;
					left = 1;
					down = 1;
					//M("F");
				} else if (angle < - 3 * M_PI / 8 && angle > - 5 * M_PI / 8) {
					animation = 4;
					down = 1;
					//M("G");
				} else if (angle < - 5 * M_PI / 8 && angle > - 7 * M_PI / 8) {
					flip = SDL_FLIP_HORIZONTAL;
					animation = 3;
					right = 1;
					down = 1;
					//M("H");
				}
			}

			//here's the code for roaming/patrolling
			//we aren't agrod.
			if(traveling) {
				if(readyForNextTravelInstruction && g_setsOfInterest.at(poiIndex).size() != 0) {
					readyForNextTravelInstruction = 0;
					if(myTravelstyle == roam) {
						//generate random number corresponding to an index of our poi vector
						int random = rand() % (int)g_setsOfInterest.at(poiIndex).size();
						pointOfInterest* targetDest = g_setsOfInterest.at(poiIndex).at(random);
						Destination = getNodeByPosition(targetDest->x, targetDest->y);
					} else if(myTravelstyle == patrol) {
						currentPoiForPatrolling++;
						if(currentPoiForPatrolling > (int)g_setsOfInterest.at(poiIndex).size()-1) {currentPoiForPatrolling = 0;}
						pointOfInterest* targetDest = g_setsOfInterest.at(poiIndex).at(currentPoiForPatrolling);
						Destination = getNodeByPosition(targetDest->x, targetDest->y);
					}
				} else {
					//should we be ready for our next travel-instruction?
					if(Destination != nullptr && XYWorldDistance(this->getOriginX(), this->getOriginY(), Destination->x, Destination->y) < 128) {
						readyForNextTravelInstruction = 1;
					}
				}
			}
		}
	
		//navigate
		if(Destination != nullptr) {
			BasicNavigate(Destination);
		}

		//walking ai
		if(agrod) {
			if(timeSinceLastDijkstra - elapsed < 0) {
				//need to update our Destination member variable, dijkstra will
				//be called this frame
				if(target != nullptr && target->tangible) {
					//requirements for a valid place to navigate to
					//must be
					//1 - in range
					//2 - have LOS to player (we can even check if the attack pierces walls later)
					//3 - not be a wall
					//hopefully thats enuff, because i dont really want to make an algorithm
					//to check if a spot is easy to walk to, but its possible.

					//how will we write the algorithm?
					//check the cardinal points of the player having 2/3 of the range
					//thats eight places to check.
					//start with the closest first, otherwise every dijkstra call the AI
					//would basically walk thru the player between these cardinal points
					//if the closest cardinal place fails the second or third requirements
					//try testing another
					//if none work, (e.g. player is standing in the middle of a small room)
					//repeat with half of 2/3 of the range. it might just be better to have them navigate
					//to the player

					//use frame to get prefered cardinal point
					int index = 0;
					switch(animation) {
						case 0:
							//facing up
							index = 0;
							break;
						case 1:
							//facing upleft or upright
							if(flip == SDL_FLIP_HORIZONTAL) {
								index = 1;
							} else {
								index = 7;
							}
							break;
						case 2:
							//facing upleft or upright
							if(flip == SDL_FLIP_HORIZONTAL) {
								index = 2;
							} else {
								index = 6;
							}
							break;
						case 3:
							//facing upleft or upright
							if(flip == SDL_FLIP_HORIZONTAL) {
								index = 3;
							} else {
								index = 5;
							}
							break;
						case 4:
							//facing upleft or upright
							index = 4;
							break;
					} 

					//do we have line of sicht to this destination? if not, lets just run at the player.
					// for (int i = 0; i < 8; i++) {
					// 	//show all cardinalpoints
					// 	vector<int> ret = getCardinalPoint(target->getOriginX(), target->getOriginY(), 200, i);
					// 	SDL_FRect rend;
					// 	int size = 20; 
					// 	rend.x = ret[0] - size/2;
					// 	rend.y = ret[1] - size/2;
					// 	rend.w = size;
					// 	rend.h = size;
					// 	M(ret[0]);
					// 	M(ret[1]);
					// 	M("thats all");
					// 	//SDL_Delay(600);
					// 	rend = transformRect(rend);
					// 	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
					// 	SDL_RenderDrawRectF(renderer, &rend);
					// 	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
						
					// }
					// SDL_RenderPresent(renderer);
					// SDL_Delay(10);

					//use this code for strafing
					//rotate clockwise
					if(this->hisweapon->attacks[hisweapon->combo]->name == "strafeleft") {
						index ++;
						if(index == 8) {index = 0;}
					}

					//rotate counterclockwise
					if(this->hisweapon->attacks[hisweapon->combo]->name == "straferight") {
						index --;
						if(index == -1) {index = 7;}
					}
					if(this->hisweapon->attacks[hisweapon->combo]->name == "strafe") {
						int flip = rand() % 2;
						if(flip == 1) {
							index --;
							if(index == -1) {index = 7;}
						} else {
							index ++;
							if(index == 8) {index = 0;}
						}
					}

					if(this->hisweapon->attacks[hisweapon->combo]->name == "approach" && (int)hisweapon->attacks.size() > hisweapon->combo) {
						this->hisweapon->attacks[hisweapon->combo]->range = this->hisweapon->attacks[hisweapon->combo + 1]->range;
					}

					//!!! some easy optimization can be done here
					vector<int> ret;
					if(this->hisweapon->attacks[hisweapon->combo]->melee)  {
						ret = getCardinalPoint(target->getOriginX(), target->getOriginY(), 0, index);
						Destination = getNodeByPosition(ret[0], ret[1]);
					} else {
						ret = getCardinalPoint(target->getOriginX(), target->getOriginY(), this->hisweapon->attacks[hisweapon->combo]->range, index);
						

						if( LineTrace(ret[0], ret[1], target->getOriginX(), target->getOriginY(), false, 30, 0, 10, 0) && abs(target->z- verticalRayCast(ret[0], ret[1])) < 32 ) {
							//M("There's a good position, keeping my distance");
							//vector<int> ret = getCardinalPoint(target->x, target->y, 200, index);
							
							Destination = getNodeByPosition(ret[0], ret[1]);
						
						} else {
							//Can't get our full range, so use the values in LineTraceX and LineTraceY
							extern int lineTraceX, lineTraceY;
							//Destination = getNodeByPosition(target->getOriginX(), target->getOriginY());
							Destination = getNodeByPosition(lineTraceX, lineTraceY);
						
						}
					}
				}
			}
			
			//detect stuckness- if we're stuck, try heading to a random nearby node for a moment
			if(abs(lastx - x) < 0.2 && abs(lasty - y) < 0.2) {
				stuckTime++;
			} else {
				stuckTime = 0;
			}
			lastx = x;
			lasty = y;
			if(stuckTime > maxStuckTime) {
				//spring to get over obstacles
				//this->zaccel = 350;

				current = Get_Closest_Node(g_navNodes);
				if(current != nullptr) {
					int c = rand() % current->friends.size();
					Destination = current->friends[c];
					dest = Destination;
					stuckTime = 0;
				}
			}
		} else {
			//patroling/idling/wandering behavior would go here
			if(Destination == nullptr) {
				xvel = 0;
				yvel = 0;
				stop_hori();
				stop_verti();
			}
		}

		

		return nullptr;
	}

	//all-purpose pathfinding function
	void BasicNavigate(navNode* ultimateTargetNode) {
		if(g_navNodes.size() < 1) {return;}
		if(current == nullptr) {
			current = Get_Closest_Node(g_navNodes);
			dest = current;
		}
		
		// current->Render(255,0,0);
		// dest->Render(0,255,0);
		// Destination->Render(0,0,255);

		float ydist = 0;
		float xdist = 0;

		xdist = abs(dest->x - getOriginX());
		ydist = abs(dest->y - getOriginY());

		float vect = pow((pow(xdist,2)  + pow(ydist,2)), 0.5);
		float factor = 1;

		if(vect != 0) {
			factor = 1 / vect;
		}

		xdist *= factor;
		ydist *= factor;
			
		if(dest->x > getOriginX()) {
			xaccel = this->xagil * xdist;
			walkingxaccel = xaccel;
		} else {
			xaccel = -this->xagil * xdist;
			walkingxaccel = xaccel;
		}
		if(dest->y > getOriginY()) {
			yaccel = this->xagil * ydist;
			walkingyaccel = yaccel;
		} else {
			yaccel = -this->xagil * ydist;
			walkingyaccel = yaccel;
		}
		//spring to get over obstacles
		if(dest->z > this->z + 32 && this->grounded) {
			this->zaccel = 200;
		}

		int prog = 0;
		if(abs(dest->y - getOriginY() ) < 64) {
			prog ++;
		}
		if(abs(dest->x - getOriginX()) < 55) {
			prog ++;
		}

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
			//M("Starting Dijkstra");
			if(dest != nullptr) {
				current = dest;
			}
			current = dest;
			//randomized time to space out dijkstra calls -> less framedrops
			timeSinceLastDijkstra = dijkstraSpeed + rand() % 500;
			navNode* targetNode = ultimateTargetNode;
			vector<navNode*> bag;
			for (int i = 0; i < (int)g_navNodes.size(); i++) {
				
				bag.push_back(g_navNodes[i]);
				
				
				g_navNodes[i]->costFromSource = numeric_limits<float>::max();
			}
			
			current->costFromSource = 0;
			int overflow = 500;
			
			while(bag.size() > 0) {
				overflow --;
				if(overflow < 0) { break; }

						
				navNode* u;
				float min_dist = numeric_limits<float>::max();
				bool setU = false;

				
				
				for (int i = 0; i < (int)bag.size(); i++) { 	
					
					// !!! the second condition was added early december 2021
					// it it could cause problems
					if(bag[i]->costFromSource < min_dist && bag[i]->enabled){
						u = bag[i];
						min_dist = u->costFromSource;
						setU = true;
					}
				}

				// if(!setU) {
				// 	for(auto x : current->friends) {
				// 		if(x->enabled && LineTrace(this->getOriginX(), this->getOriginY(), x->x, x->y)) {
				// 			u = x;
				// 			setU = 1;
				// 			break;
				// 		}
						
				// 	}
				// 	if(!setU){
				// 		for(auto x : current->friends){
				// 			for(auto y : x->friends) {
				// 				if(y->enabled && LineTrace(this->getOriginX(), this->getOriginY(), y->x, y->y)){
				// 					u = y;
				// 					setU = 1;
				// 					break;
				// 				}
				// 			}
				// 		}
				// 	}
				// }

				if(!setU){
					//could issue an error msg
					break;
				}

				//u is closest node in bag
				bag.erase(remove(bag.begin(), bag.end(), u), bag.end());
				for (long long unsigned int i = 0; i < u->friends.size(); i++) {
					if(u->enabled) {
						
						float alt = u->costFromSource + u->costs[i];
						if(alt < u->friends[i]->costFromSource && (u->friends[i]->z + 64 >= u->z)) {
							if(u->friends[i]->enabled) {
								u->friends[i]->costFromSource = alt;
								u->friends[i]->prev = u;
							} else {
								u->friends[i]->prev = nullptr;
							}
						}
					} else {
						u->prev = nullptr;
					}
				}
				
			}
			path.clear();
			int secondoverflow = 350;
			
			while(targetNode != nullptr) {
				secondoverflow--;
				if(secondoverflow < 0) { M("prevented a PF crash."); break;} //preventing this crash results in pathfinding problems
				
				path.push_back(targetNode);
				
				if(targetNode == current) {
					break;
				}
				targetNode = targetNode->prev;
			}

			for(auto x : path) {
				if(!x->enabled){
					M("TRYING TO USE DISABLED NODE");
					current = getNodeByPosition(getOriginX(), getOriginY());
					dest = current;
				}
			}

			
			dest = path.at(path.size() - 1);
			//M("Finished Dikjstra");
		} else {
			timeSinceLastDijkstra -= elapsed;
		}
	
	}

	//functions for inv
	//add an item to an entities
	int getItem(indexItem* a, int count) {
		for(auto& x : inventory) {
			if(x.first->name == a->name) {
				x.second += count;
				return 0;
			}
		}
		pair<indexItem*, int> pushMeBack{ a, count };

		inventory.push_back( pushMeBack );
		return 0;
	}

	//returns 0 if the transaction was successful, and 1 otherwise
	int loseItem(indexItem* a, int count) {
		M("loseItem()");
		for(auto& x : inventory) {
			if(x.first->name == a->name) {
				M("found a match");
				if(x.second > count) {
					x.second-=count;	
					return 0;
				} else {

					for(auto y : inventory) {
						D(y.first->name);
					}
					delete x.first;
					x.second = 0;
					
					D(inventory.size());
					inventory.erase(remove(inventory.begin(), inventory.end(), x), inventory.end());
					D(inventory.size());
					return 0;
				}
			}
		}
		return 1;
	}

	//returns 0 if the entity has the nummer of items
	//returns 1 if the entity does not have the proper nummer
	int checkItem(indexItem* a, int count) {
		for(auto x : inventory) {
			if(x.first == a) {
				if(x.second >= count) {
					return 0;
				}
			}
		}
		return 1;
	}
};

//search entity by name
entity* searchEntities(string fname) {
	if(fname == "protag") {
		return protag;
	}
	for(auto n : g_entities) {
		if(n->name == fname && n->tangible) {
			return n;
		}
	}
	return nullptr;
}

//return list of tangible entities with the name
vector<entity*> gatherEntities(string fname) {
	vector<entity*> ret = {};
	for(auto n : g_entities) {
		if(n->name == fname && n->tangible) {
			ret.push_back(n);
		}
	}
	return ret;
}


int loadSave() {
	g_save.clear();
	ifstream file;
	string line;
	
	string address = "user/saves/" + g_saveName + ".txt";
	//D(address);
	const char* plik = address.c_str();
	file.open(plik);
	
	string field = "";
	string value = "";

	M("time to load a save");
	//load fields
	while(getline(file, line)) {
		if(line == "&") { break;}
		field = line.substr(0, line.find(' '));
		value = line.substr(line.find(" "), line.length()-1);
		
		//D(value + "->" + field);
		try {
			g_save.insert( pair<string, int>(field, stoi(value)) );
		} catch(...) {
			M("Error writing");
			return -1;
		}
		
	}
	M("time to load the map");
	file >> g_mapOfLastSave >> g_waypointOfLastSave;
	getline(file,line);
	getline(file,line);
	
	//delete current party
	//int repetitions = (int) party.size();
	// for(auto x : party) {
	// 	M("about to delete someone from the old party");
	// 	//possibly unsafe
	// 	//delete x;
	// }
	party.clear();

	M("Lets load the party");
	
	bool setMainProtag = 0;
	//load party
	while(getline(file, line)) {
		if(line == "&") {M("Thats it!"); break;}
		
		auto tokens = splitString(line, ' ');
		string name = tokens[0];
		float level = stoi(tokens[1]);
		float currentHP = stof(tokens[2]);
		
		entity* a = new entity(renderer, name);
		a->level = level;
		a->hp = currentHP;
		party.push_back(a);
		a->tangible = 0;
		a->inParty = 1;
		M("added an entity to the party " + name);
		if(a->essential) {
			mainProtag = a;
			setMainProtag = 1;
		}
	}

	party[0]->tangible = 1;
	if(setMainProtag) {
		protag = mainProtag;
	} else {
		//feck
		E("No essential entity found in save");
		protag = party[0];
		mainProtag = protag;
	}
	
	g_focus = protag;

	//load inventory
	while(getline(file, line)) {
		if(line == "&") { break;}
		field = line.substr(0, line.find(' '));
		value = line.substr(line.find(" "), line.length()-1);
		indexItem* a = new indexItem(field, 0);
		protag->getItem(a, stoi(value));
	}

	file.close();

	for(auto x : g_entities) {
		x->children.clear(); // might be a leak here
	}

	//re-attach persistent orbitals
	for(auto x : g_entities) {
		if(x->persistentGeneral && x->parentName != "null") {
			entity* hopeful = searchEntities(x->parentName);
			if(hopeful != nullptr) {
				x->isOrbital = 1;
				x->parent = hopeful;
				x->parent->children.push_back(x);		
			}
		
		}
	}

	return 0;
}

int writeSave() {
	ofstream file;
	
	string address = "user/saves/" + g_saveName + ".txt";
	const char* plik = address.c_str();
	file.open(plik);

	auto it = g_save.begin();

	while (it != g_save.end() ) {
		file << it->first << " " << it->second << endl;
		it++;
	}
	file << "&" << endl; //token to stop writing saveflags
	file << g_mapdir + "/" + g_map << " " << g_waypoint << endl;
	file << "&" << endl;

	//write party
	for(auto x : party) {
		file << x->name << " " << x->level << " " << x->hp << endl;
	}
	file << "&" << endl;
	//write protag's inventory
	extern entity* protag;
	for(auto x : protag->inventory) {
		file << x.first->name << " " << x.second << endl;
	}
	file << "&" << endl; //token to stop writing inventory
	

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

void entity::shoot() {
	//M("shoot()");
	if(!tangible) {return;}
	if(this->cooldown <= 0) {
		//M("pow pow");
		if(hisweapon->comboResetMS > hisweapon->maxComboResetMS) {
			//waited too long- restart the combo
			hisweapon->combo = 0;
		}
		for(float i = 0; (i < this->hisweapon->attacks[hisweapon->combo]->numshots); i++) {
			if(i > 1000) {
				//M("Handled an infinite loop");
				quit = 1;
				return;
				
			}
			cooldown = hisweapon->attacks[hisweapon->combo]->maxCooldown;
			projectile* p = new projectile(hisweapon->attacks[hisweapon->combo]);
			p->owner = this;
			p->x = getOriginX() - p->width/2;
			p->y = getOriginY() - p->height/2;
			p->z = z + 20;
			p->zeight = p->width * XtoZ;
			p->animation = this->animation;
			p->flip = this->flip;
			
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

			//give it an angle based on attack spread
			if(hisweapon->attacks[hisweapon->combo]->randomspread != 0) {
				float randnum = (((float) rand()/RAND_MAX) * 2) - 1;
				p->angle += (randnum * hisweapon->attacks[hisweapon->combo]->randomspread);
			}
			if(hisweapon->attacks[hisweapon->combo]->spread != 0) {
				p->angle += ( (i - ( hisweapon->attacks[hisweapon->combo]->numshots/2) ) * hisweapon->attacks[hisweapon->combo]->spread);
			}

			//move it out of the shooter and infront
			p->x += cos(p->angle) * p->bounds.width;
			p->y += cos(p->angle + M_PI / 2) * p->bounds.height;


			
			
		}
	  	hisweapon->combo++;
	
		if(hisweapon->combo > (int)(hisweapon->attacks.size() - 1)) {hisweapon->combo = 0;}
		hisweapon->comboResetMS = 0;
	}
}

void cshadow::render(SDL_Renderer * renderer, camera fcamera) {
	if(!owner->tangible) {return;}

	//update alpha
	if(enabled && alphamod != 255) {
		alphamod+=elapsed;
		if(alphamod > 255) {
			alphamod = 255;
		}
	} else {
		if(!enabled && alphamod != 0) {
			alphamod -=elapsed;
			if(alphamod < 0) {
				alphamod = 0;
			}
		}
	}
	//dont clog up other textures forever
	if(alphamod == 0) {return;}

	Uint8 whydoihavetodothis = 255;
	SDL_GetTextureAlphaMod(this->texture, &whydoihavetodothis);
	
	if(whydoihavetodothis != alphamod) {
		//update texture with proper alphamod
		SDL_SetTextureAlphaMod(this->texture, alphamod);
	}

	SDL_FRect dstrect = { ((this->x)-fcamera.x) *fcamera.zoom, (( (this->y - (XtoZ * z) ) ) -fcamera.y) *fcamera.zoom, (float)(width * size), (float)((height * size)* (637 /640) * 0.9)};
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

	if(RectOverlap(obj, cam)) {
		SDL_RenderCopyF(renderer, texture, NULL, &dstrect);	
	}
}

//returns true if there was no hit
//visibility is 1 to check for just navblock (very solid) entities
bool LineTrace(int x1, int y1, int x2, int y2, bool display = 0, int size = 30, int layer = 0, int resolution = 10, bool visibility = 0) {
	//float resolution = 10;
	
	if(display) {
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	}
	for (float i = 1; i < resolution; i++) {
		int xsize = size * p_ratio;
		int xpos = (i/resolution) * x1 + (1 - i/resolution) * x2;
		int ypos = (i/resolution) * y1 + (1 - i/resolution) * y2;
		rect a = rect(xpos - xsize/2, ypos - size/2, xsize, size);
		SDL_Rect b = {(int)(((xpos- xsize/2) - g_camera.x) * g_camera.zoom), (int)(((ypos- size/2) - g_camera.y) * g_camera.zoom), (int)(xsize), (int)(size)};

		if(display) {
			SDL_RenderDrawRect(renderer, &b);
		}	
		
		
		for (long long unsigned int j = 0; j < g_boxs[layer].size(); j++) {
			if(RectOverlap(a, g_boxs[layer][j]->bounds)) {
				lineTraceX = a.x + a.width/2;
				lineTraceY = a.y + a.height/2;
				return false;
			}
		}
		for(auto x : g_solid_entities) {
			if(visibility) {
				if(!x->navblock) {continue;}
			}
			if(RectOverlap(a, x->getMovedBounds())) {
				lineTraceX = a.x + a.width/2;
				lineTraceY = a.y + a.height/2;
				return false;
			}
		}
	}
	return true;
}

class textbox {
public:
	SDL_Surface* textsurface = 0;
	SDL_Texture* texttexture = 0;
	SDL_Color textcolor = { 245, 245, 245 };
	SDL_FRect thisrect = {0, 0, 50, 50};
	string content = "Default text.";
	TTF_Font* font = 0;
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	bool show = true;
	int align = 0;

	int errorflag = 0;

	//used for drawing in worldspace
	float boxWidth = 50;
	float boxHeight = 50;
	float boxX = 0;
	float boxY = 0;

	float boxScale = 40;
	bool worldspace = false; //use worldspace or screenspace;

	textbox(SDL_Renderer* renderer, const char* fcontent, float size, float fx, float fy, float fwidth) {
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
		thisrect = { fx, fy, (float)texW, (float)texH };
		
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
				if(align == 1) {
					SDL_FRect dstrect = {(boxX * winwidth)-width, boxY * winheight, (float)width,  (float)thisrect.h};
					SDL_RenderCopyF(renderer, texttexture, NULL, &dstrect);
				} else {
					if(align == 0) {
						SDL_FRect dstrect = {boxX * winwidth, boxY * winheight, (float)width,  (float)thisrect.h};
						SDL_RenderCopyF(renderer, texttexture, NULL, &dstrect);
					} else {
						//center text
						SDL_FRect dstrect = {(boxX * winwidth)-width/2, boxY * winheight, (float)width,  (float)thisrect.h};
						SDL_RenderCopyF(renderer, texttexture, NULL, &dstrect);
					}
				}
				
				
			} else {
				if(align == 1) {
					SDL_FRect dstrect = {(boxX * winwidth)-width, boxY * winheight, (float)width,  (float)thisrect.h};
					SDL_RenderCopyF(renderer, texttexture, NULL, &dstrect);
				} else {
					if(align == 0) {
						SDL_FRect dstrect = {boxX * winwidth, boxY * winheight, (float)width,  (float)thisrect.h};
						SDL_RenderCopyF(renderer, texttexture, NULL, &dstrect);
					} else {
						//center text
						SDL_FRect dstrect = {(boxX * winwidth)-width/2, boxY * winheight, (float)width,  (float)thisrect.h};
						SDL_RenderCopyF(renderer, texttexture, NULL, &dstrect);
					}
				}
			}
		}
	}
	void updateText(string content, float size, float fwidth) {
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
		thisrect = { (float)x, (float)y, (float)texW, (float)texH };
		
	}
};

class ui {
public:
	float x;
	float y;

	float xagil;

	float xaccel;
	float yaccel;

	float xvel;
	float yvel;

	float xmaxspeed;

	float width = 0.5;
	float height = 0.5;

	float friction;

	bool show = true;
	SDL_Surface* image;
	SDL_Texture* texture;

	string filename = "";

	bool mapSpecific = 0;
	
	//for 9patch
	bool is9patch = 0;
	int patchwidth = 256; //213
	float patchscale = 0.4;

	bool persistent = 0;
	int priority = 0; //for ordering, where the textbox has priority 0 and 1 would put it above

	int shrinkPixels = 0; //used for shrinking a ui element by an amount of pixels, usually in combination with some other element intended as a border
	
	float heightFromWidthFactor = 0; //set this to 0.5 or 1 and the height of the element will be held to that ratio of the width, even if the screen's ratio changes.

	ui(SDL_Renderer * renderer, const char* ffilename, float fx, float fy, float fwidth, float fheight, int fpriority) {
		M("ui()" );
		filename = ffilename;
		image = IMG_Load(filename.c_str());
		
		width = fwidth;
		height = fheight;
		x = fx;
		y = fy;
		texture = SDL_CreateTextureFromSurface(renderer, image);
		g_ui.push_back(this);
		priority = fpriority;
		SDL_FreeSurface(image);
	}

	virtual ~ui() {
		M("~ui()" );
		SDL_DestroyTexture(texture);
		
		g_ui.erase(remove(g_ui.begin(), g_ui.end(), this), g_ui.end());
	}

	void render(SDL_Renderer * renderer, camera fcamera) {
		if(this->show) {
			if(is9patch) {
				if(WIN_WIDTH != 0) {
					patchscale = WIN_WIDTH;
					patchscale /= 4000;
				}
				int ibound = width * WIN_WIDTH;
				int jbound = height * WIN_HEIGHT;
				int scaledpatchwidth = patchwidth * patchscale;
				int i = 0; 
				while (i < ibound) {
					int j = 0;
					while (j < jbound) {
						SDL_FRect dstrect = {i + (x * WIN_WIDTH), j + (y * WIN_HEIGHT), (float)scaledpatchwidth, (float)scaledpatchwidth}; //change patchwidth in this declaration for sprite scale
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
						SDL_RenderCopyF(renderer, texture, &srcrect, &dstrect);
						
						
						
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
				if(heightFromWidthFactor != 0) {
					SDL_FRect dstrect = {x * WIN_WIDTH + (shrinkPixels / scalex), y * WIN_HEIGHT + (shrinkPixels / scalex), width * WIN_WIDTH - (shrinkPixels / scalex) * 2,  heightFromWidthFactor * (width * WIN_WIDTH - (shrinkPixels / scalex) * 2) };
					SDL_RenderCopyF(renderer, texture, NULL, &dstrect);
				} else {
					SDL_FRect dstrect = {x * WIN_WIDTH + (shrinkPixels / scalex), y * WIN_HEIGHT + (shrinkPixels / scalex), width * WIN_WIDTH - (shrinkPixels / scalex) * 2, height * WIN_HEIGHT - (shrinkPixels / scalex) * 2};
					SDL_RenderCopyF(renderer, texture, NULL, &dstrect);
				}
			}
		}
	}

	virtual void update_movement(vector<box*> boxs, float elapsed) {
		if(xaccel > 0 /*&& xvel < xmaxspeed*/) {
			xvel += xaccel * ((double) elapsed / 256.0);
		}
		
		if(xaccel < 0 /*&& xvel > -1 * xmaxspeed*/) {
			xvel += xaccel * ((double) elapsed / 256.0);
		}

		if(yaccel > 0 /*&& yvel < ymaxspeed*/) {
			yvel += yaccel* ((double) elapsed / 256.0);
		}
		
		if(yaccel < 0 /*&& yvel > -1 * ymaxspeed*/) {
			yvel += yaccel* ((double) elapsed / 256.0);
		}

		rect movedbounds;
		//bool ycollide = 0;
		//bool xcollide = 0;
		y+= yvel * ((double) elapsed / 256.0);
		x+= xvel * ((double) elapsed / 256.0);
		
	}
};


void worldsound::update(float elapsed) {
	//!!! you can update this with the middle of the camera instead of the focused actor
	float dist = Distance(x, y, g_focus->getOriginX(), g_focus->getOriginY());
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


class musicNode {
public:
	Mix_Music* blip;
	string name = "empty";
	int x = 0;
	int y = 0;
	float radius = 1200;

	musicNode(string fileaddress, int fx, int fy) {
		name = fileaddress;
		
		string temp = "maps/" + g_mapdir + "/music/" + fileaddress + ".ogg";
		if(!fileExists(temp)) {
			temp = "static/music/" + fileaddress + ".ogg";
		}

		
		
		blip = Mix_LoadMUS(temp.c_str());
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
		string existSTR;
		existSTR = "maps/" + g_mapdir + "/sounds/" + fileaddress + ".wav"; 
		if(!fileExists(existSTR)) {
			existSTR = "static/sounds/" + fileaddress + ".wav";
			if(!fileExists(existSTR)) {
				existSTR = "static/sounds/defaults.wav";
			}
		}
		blip = Mix_LoadWAV(existSTR.c_str());
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

//play a sound by name at a position
void playSoundByName(string fname, float xpos, float ypos) {
	Mix_Chunk* sound = 0;
	for (auto s : g_cueSounds) {
		if (s->name == fname) {
			sound = s->blip;
			break;
		}
	}
	if(sound == NULL) {
		E("Soundcue " + fname + " not found in level." + " Not critical.");
		return;
	}

	//!!! this could be better if it used the camera's position
	float dist = XYWorldDistance(g_focus->getOriginX(), g_focus->getOriginY(), xpos, ypos);
	const float maxDistance = 1200; //a few screens away
	float cur_volume = (maxDistance - dist)/maxDistance * 128;
	if(cur_volume < 0) {cur_volume = 0;}
	M(cur_volume);
	Mix_VolumeChunk(sound, cur_volume);
	if(!g_mute && sound != NULL) {
		Mix_PlayChannel(0, sound,0);
	}
}

//play a sound given a string of its name. just make sure there's a cue with the same name
void playSoundByName(string fname) {
	Mix_Chunk* sound = 0;
	for (auto s : g_cueSounds) {
		if (s->name == fname) {
			sound = s->blip;
			break;
		}
	}
	if(sound == NULL) {
		E("Soundcue " + fname + " not found in level." + " Not critical.");
	}

	if(!g_mute && sound != NULL) {
		Mix_PlayChannel(0, sound,0);
	}
}

class waypoint {
public:
	float x = 0;
	float y = 0;
	int z = 0;
	string name;
	waypoint(string fname, float fx, float fy, int fz) {
		name = fname;
		x = fx;
		y = fy;
		z = fz;
		g_waypoints.push_back(this);
	}
	~waypoint() {
		g_waypoints.erase(remove(g_waypoints.begin(), g_waypoints.end(), this), g_waypoints.end());
	}
};

class trigger {
public:
	int x, y, z, width, height, zeight;
	string binding;
	vector<string> script;
	bool active = 1;

	string targetEntity = "protag"; //what entity will activate the trigger

	trigger(string fbinding, int fx, int fy, int fz, int fwidth, int fheight, int fzeight, string ftargetEntity) {
		x = fx;
		y = fy;
		z = fz;
		width = fwidth;
		height = fheight;
		zeight = fzeight;
		binding = fbinding;
		targetEntity = ftargetEntity;
		g_triggers.push_back(this);
		//open and read from the script file
		ifstream stream;
		string loadstr;
		//try to open from local map folder first
		
		loadstr = "maps/" + g_mapdir + "/" + fbinding + ".txt";
		//D(loadstr);
		const char* plik = loadstr.c_str();
		
		stream.open(plik);
		
		if (!stream.is_open()) {
			stream.open("scripts/" + fbinding + ".txt");
		}
		string line;

		getline(stream, line);

		while (getline(stream, line)) {
			script.push_back(line);
		}
		
		parseScriptForLabels(script);
		// for(auto x : script) {
		// 	//D(x);
		// }
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
		parseScriptForLabels(script);
		M("Check item script");
		for(auto x : script) {
			D(x);
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
	g_budget = 0;
	enemiesMap.clear();
	g_musicalEntities.clear();
	Mix_FadeOutMusic(1000);
	g_objective = 0;
	adventureUIManager->crosshair->show = 0;
	{
		
		//SDL_GL_SetSwapInterval(0);
		bool cont = false;
		float ticks = 0;
		float lastticks = 0;
		float transitionElapsed = 5;
		float mframes = 60;
		float transitionMinFrametime = 5;
		transitionMinFrametime = 1/mframes * 1000;
		
		
		SDL_Surface* transitionSurface = IMG_Load("engine/transition.bmp");

		int imageWidth = transitionSurface->w;
		int imageHeight = transitionSurface->h;

		SDL_Texture* transitionTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, transitionSurface->w, transitionSurface->h );
		SDL_SetTextureBlendMode(transitionTexture, SDL_BLENDMODE_BLEND);


		void* pixelReference;
		int pitch;

		float offset = imageHeight;

		
		SDL_Texture* frame = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, WIN_WIDTH, WIN_HEIGHT);
		SDL_SetRenderTarget(renderer, frame);

		//render current frame to texture -- this is gonna get weird
		{
			
			if(g_backgroundLoaded && g_useBackgrounds) { //if the level has a background and the user would like to see it
				SDL_RenderCopy(renderer, background, NULL, NULL);
			}
			
			for(auto n : g_entities) {
				n->cooldown -= elapsed;
			}
			
			
			//tiles
			for(long long unsigned int i=0; i < g_tiles.size(); i++){
				if(g_tiles[i]->z ==0) {
					g_tiles[i]->render(renderer, g_camera);
				}
			}

			for(long long unsigned int i=0; i < g_tiles.size(); i++){
				if(g_tiles[i]->z ==1) {
					g_tiles[i]->render(renderer, g_camera);
				}
			}


			SDL_Rect FoWrect;

			//sort		
			sort_by_y(g_actors);
			for(long long unsigned int i=0; i < g_actors.size(); i++){
				g_actors[i]->render(renderer, g_camera);
			}

			for(long long unsigned int i=0; i < g_tiles.size(); i++){
				if(g_tiles[i]->z == 2) {
					g_tiles[i]->render(renderer, g_camera);
				}
			}



			//Fogofwar
			if(g_fogofwarEnabled && !devMode) {

				// int functionalX = g_focus->getOriginX();
				// int functionalY = g_focus->getOriginY();

				// functionalX -= functionalX % 64;
				// functionalX += 32;
				// functionalY -= functionalY % 55;
				// functionalY += 26;

				// if(functionalX != g_lastFunctionalX || functionalY != g_lastFunctionalY) {
				// 	bool flipper = 0;
				// 	for(int i = 0; i < g_fogcookies.size(); i++) {
				// 		for(int j = 0; j < g_fogcookies[0].size(); j++) {
				// 			flipper = !flipper;
				// 			int xpos = ((i - g_fogMiddleX) * 64) + functionalX;
				// 			int ypos = ((j - g_fogMiddleY) * 55) + functionalY;
				// 			if(LineTrace(functionalX, functionalY, xpos, ypos, 0, 15, 0, 15, 1)) {
				// 				g_fogcookies[i][j] = 1;
				// 				g_fc[i][j] = 1;

				// 				g_sc[i][j] = 1;
				// 			} else {
				// 				g_fogcookies[i][j] = 0;
								
				// 				g_fc[i][j] = 0;
				// 				g_sc[i][j] = 0;
				// 			}
				// 		}
				// 	}
				// }

				//save cookies that are just dark because they are inside of walls to g_savedcookies
				// for(int i = 0; i < g_fogcookies.size(); i++) {
				// 	for(int j = 0; j < g_fogcookies[0].size(); j++) {
				// 		int xpos = ((i - 10) * 64) + functionalX;
				// 		int ypos = ((j - 9) * 55) + functionalY;
				// 		//is this cookie in a wall? or behind a wall
				// 		if(!LineTrace(xpos, ypos, xpos, ypos, 0, 15, 0, 2, 1)) {
				// 			g_fc[i][j] = 1;
									
				// 		}	
				// 		if(!LineTrace(xpos, ypos + 55, xpos, ypos +55, 0, 15, 0, 2, 1)) {
				// 			g_fc[i][j] = 1;	
				// 		}
				// 	}
				// }

				// g_lastFunctionalX = functionalX;
				// g_lastFunctionalY = functionalY;

				//these are the corners and the center
				// g_fogcookies[0][0] = 1;
				// g_fogcookies[20][0] = 1;
				// g_fogcookies[20][17] = 1;
				// g_fogcookies[0][17] = 1;
				// g_fogcookies[10][9] = 1; 

				int px = -(int)g_focus->getOriginX() % 64;
				
				//offset us to the protag's location
				//int yoffset =  ((g_focus->y- (g_focus->z + g_focus->zeight) * XtoZ)) * g_camera.zoom;
				//the zeight is constant at level 2  for now
				int yoffset =  (g_focus->getOriginY() ) * g_camera.zoom;
				
				//and then subtract half of the screen
				yoffset -= yoffset % 55;
				yoffset -= (g_fogheight * 55 + 12)/2;
				yoffset -= g_camera.y;

				//we do this nonsense to keep the offset on the grid
				//yoffset -= yoffset % 55;

				//px = 64 - px - 64;
				//py = 55 - py - 55;
				// 50 50
				SDL_SetRenderTarget(renderer, NULL);
				addTextures(renderer, g_fc, canvas, light, 500, 500, 250, 250, 0);


				TextureC = IlluminateTexture(renderer, TextureA, canvas, result);
				
				//render graphics
				FoWrect = {px - 23, yoffset +15, g_fogwidth * 64 + 50, g_fogheight * 55 + 18};
				SDL_SetRenderTarget(renderer, frame);
				SDL_RenderCopy(renderer, TextureC, NULL, &FoWrect);
				
				//do it for z = 64
				FoWrect.y -= 64 * XtoZ;
				SDL_RenderCopy(renderer, TextureC, NULL, &FoWrect);
				

				SDL_SetRenderTarget(renderer, NULL);
				addTextures(renderer, g_sc, canvas, light, 500, 500, 250, 250, 1);


				TextureC = IlluminateTexture(renderer, TextureA, canvas, result);
				SDL_SetRenderTarget(renderer, frame);
			
				//render graphics
				FoWrect.y -= 67 * XtoZ;
				SDL_RenderCopy(renderer, TextureC, NULL, &FoWrect);
			
				//black bars :/
				SDL_Rect topbar = {px, FoWrect.y - 5000, 1500, 5000};
				SDL_RenderCopy(renderer, blackbarTexture, NULL, &topbar);
				
				SDL_Rect botbar = {px, FoWrect.y +  g_fogheight * 55 + 12, 1500, 5000};
				SDL_RenderCopy(renderer, blackbarTexture, NULL, &botbar);
				SDL_RenderPresent(renderer);

			}

			
			
			//ui
			// if(!inPauseMenu && g_showHUD) {
			// 	// !!! segfaults on mapload sometimes
			// 	adventureUIManager->healthText->updateText( to_string(int(protag->hp)) + '/' + to_string(int(protag->maxhp)), WIN_WIDTH * g_minifontsize, 0.9); 
			// 	adventureUIManager->healthText->show = 1;
				
			// } else {
			// 	adventureUIManager->healthText->show = 0;
				
			// }

			// //move the healthbar properly to the protagonist
			// rect obj; // = {( , (((protag->y - ((protag->height))) - protag->z * XtoZ) - g_camera.y) * g_camera.zoom, (protag->width * g_camera.zoom), (protag->height * g_camera.zoom))};		
			// obj.x = ((protag->x -g_camera.x) * g_camera.zoom);
			// obj.y = (((protag->y - ((floor(protag->height)* 0.9))) - protag->z * XtoZ) - g_camera.y) * g_camera.zoom;
			// obj.width = (protag->width * g_camera.zoom);
			// obj.height = (floor(protag->height) * g_camera.zoom);

			// protagHealthbarA->x = (((float)obj.x + obj.width/2) / (float)WIN_WIDTH) - protagHealthbarA->width/2.0;
			// protagHealthbarA->y = ((float)obj.y) / (float)WIN_HEIGHT;
			// protagHealthbarB->x = protagHealthbarA->x;
			// protagHealthbarB->y = protagHealthbarA->y;
			
			// protagHealthbarC->x = protagHealthbarA->x;
			// protagHealthbarC->y = protagHealthbarA->y;
			// protagHealthbarC->width = (protag->hp / protag->maxhp) * 0.05;
			// adventureUIManager->healthText->boxX = protagHealthbarA->x + protagHealthbarA->width/2;
			// adventureUIManager->healthText->boxY = protagHealthbarA->y - 0.005;
			
			for(long long unsigned int i=0; i < g_ui.size(); i++){
				g_ui[i]->render(renderer, g_camera);
			}	
			for(long long unsigned int i=0; i < g_textboxes.size(); i++){
				g_textboxes[i]->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}	

			//SDL_RenderPresent(renderer);
		}
		
		
		SDL_SetRenderTarget(renderer, NULL);
		while (!cont) {
			
			//onframe things
			SDL_LockTexture(transitionTexture, NULL, &pixelReference, &pitch);
			
			memcpy( pixelReference, transitionSurface->pixels, transitionSurface->pitch * transitionSurface->h);
			Uint32 format = SDL_PIXELFORMAT_ARGB8888;
			SDL_PixelFormat* mappingFormat = SDL_AllocFormat( format );
			Uint32* pixels = (Uint32*)pixelReference;
			//int numPixels = imageWidth * imageHeight;
			Uint32 transparent = SDL_MapRGBA( mappingFormat, 0, 0, 0, 255);
			//Uint32 halftone = SDL_MapRGBA( mappingFormat, 50, 50, 50, 128);

			offset += g_transitionSpeed + 0.02 * offset;
			
			for(int x = 0;  x < imageWidth; x++) {
				for(int y = 0; y < imageHeight; y++) {


					int dest = (y * imageWidth) + x;
					//int src =  (y * imageWidth) + x;
					
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

			SDL_RenderClear(renderer);
			//render last frame
			SDL_RenderCopy(renderer, frame, NULL, NULL);	
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
	size = (int)g_entities.size();

	g_actors.clear();



	//copy protag to a pointer, clear the array, and re-add protag
	entity* hold_narra = nullptr;
	vector<entity*> persistentEnts;
	//D(protag->inParty);
	for(int i=0; i< size; i++) {
		if(g_entities[0]->inParty) {
			//remove from array without deleting
			g_entities.erase(remove(g_entities.begin(), g_entities.end(), g_entities[0]), g_entities.end());

			g_actors.erase(remove(g_actors.begin(), g_actors.end(), g_entities[0]), g_actors.end());
		} else if (g_entities[0]->persistentHidden) {
			//do nothing because nar is handled differently now
			if(hold_narra == nullptr) {
				hold_narra = g_entities[0];
				g_entities.erase(remove(g_entities.begin(), g_entities.end(), g_entities[0]), g_entities.end());
				g_actors.erase(remove(g_actors.begin(), g_actors.end(), g_entities[0]), g_actors.end());

			} else {
				throw("critical error");
			}
		} else if(g_entities[0]->persistentGeneral) {
			persistentEnts.push_back(g_entities[0]);
			g_entities.erase(remove(g_entities.begin(), g_entities.end(), g_entities[0]), g_entities.end());
			g_actors.erase(remove(g_actors.begin(), g_actors.end(), g_entities[0]), g_actors.end());


			
		} else {
			
			delete g_entities[0];
		}
	}
	//push back any entities that were in the party
	for (long long unsigned int i = 0; i < party.size(); i++) {
		g_entities.push_back(party[i]);
		g_actors.push_back(party[i]);
	}
	//push back any shadows we want to keep
	for(auto n : g_shadows) {
		g_actors.push_back(n);
	}
		
	//push back any ents that were persisent (arms)
	for(auto n : persistentEnts) {
		g_entities.push_back(n);
		g_actors.push_back(n);
	}

	//push the narrarator back on
	g_entities.push_back(hold_narra);
	g_actors.push_back(hold_narra);

	size = (int)g_tiles.size();
	for(int i = 0; i < size; i++) {
		delete g_tiles[0];
	}

	size = (int)g_navNodes.size();
	for(int i = 0; i < size; i++) {
		delete g_navNodes[0];
	}

	vector<worldsound*> savedSounds;

	size = (int)g_worldsounds.size();
	for(int i = 0; i < size; i++) {
		if(g_worldsounds[0]->owner == nullptr) {
			delete g_worldsounds[0];
		} else {
			savedSounds.push_back(g_worldsounds[0]);
			g_worldsounds.erase(remove(g_worldsounds.begin(), g_worldsounds.end(), g_worldsounds[0]), g_worldsounds.end());
		}
	}

	size = (int)g_musicNodes.size();
	for(int i = 0; i < size; i++) {
		delete g_musicNodes[0];
	}

	size = (int)g_cueSounds.size();
	for(int i = 0; i < size; i++) {
		delete g_cueSounds[0];
	}

	size = (int)g_waypoints.size();
	for(int i = 0; i < size; i++) {
		delete g_waypoints[0];
	}

	size = (int)g_doors.size();
	for(int i = 0; i < size; i++) {
		delete g_doors[0];
	}

	size = (int)g_triggers.size();
	for(int i = 0; i < size; i++) {
		delete g_triggers[0];
	}

	size = (int)g_heightmaps.size();
	for(int i = 0; i < size; i++) {
		delete g_heightmaps[0];
	}

	size = (int)g_listeners.size();
	for(int i = 0; i < size; i++) {
		delete g_listeners[0];
	}

	size = (int)g_effectIndexes.size();
	for(int i = 0; i < size; i++) {
		delete g_effectIndexes[i];
	}

	g_particles.clear();

	size = g_attacks.size();
	bool contflag = 0;
	for(int i = 0; i < size; i++) {
		for(auto x : protag->hisweapon->attacks) {
			if(x == g_attacks[0]) {
				swap(g_attacks[0], g_attacks[g_attacks.size()-1]);
				contflag = 1;
				break;

				
			}
			
		}
		if(!contflag) {
			delete g_attacks[0];
		}
	}

	vector<weapon*> persistentweapons;
	size = (int)g_weapons.size();
	for(int i = 0; i < size; i++) {
		bool partyOwned = false;
		//check if party members own the weapons
		for(auto x: party) {
			if(x->hisweapon->name == g_weapons[0]->name) {
				partyOwned = true;
			}
		}
		if(partyOwned) {
			persistentweapons.push_back(g_weapons[0]);
			g_weapons.erase(remove(g_weapons.begin(), g_weapons.end(), g_weapons[0]), g_weapons.end());
		} else {
			delete g_weapons[0];
		}
	}

	for(auto x : persistentweapons) {
		g_weapons.push_back(x);
	}

	vector<ui*> persistentui;
	size = (int)g_ui.size();
	for(int i = 0; i < size; i++) {
		if(g_ui[0]->persistent) {
			persistentui.push_back(g_ui[0]);
			g_ui.erase(remove(g_ui.begin(), g_ui.end(), g_ui[0]), g_ui.end());
		} else {
			delete g_ui[0];
		}
	}

	for(auto x : persistentui) {
		g_ui.push_back(x);
	}
	
	g_solid_entities.clear();

	//unloading takes too long- probably because whenever a mapObject is deleted we search the array
	//lets try clearing the array first, because we should delete everything properly afterwards anyway
	g_mapObjects.clear();

	//new, delete all mc, which will automatycznie delete the others
	//here's where we could save some textures if we're going to a map in the same level, might be worth it
	size = (int)g_mapCollisions.size();
	for (int i = 0; i < size; i++) {
		//M("Lets delete a mapCol");
		int jsize = (int)g_mapCollisions[0]->children.size();
		for (int j = 0; j < jsize; j ++) {
			//M("Lets delete a mapCol child");
			delete g_mapCollisions[0]->children[j];
		}
		delete g_mapCollisions[0];
	}

	for(auto x : g_collisionZones) {
		delete x;
	}
	g_collisionZones.clear();

	//clear layers of boxes and triangles
	for(long long unsigned int i = 0; i < g_boxs.size(); i++) {
		g_boxs[i].clear();
	}
	for(long long unsigned int i = 0; i < g_triangles.size(); i++) {
		g_triangles[i].clear();
	}
	for(long long unsigned int i = 0; i < g_ramps.size(); i++) {
		g_ramps[i].clear();
	}
	if(g_backgroundLoaded && background != 0) {
		M("deleted background");
		SDL_DestroyTexture(background);
		extern string backgroundstr;
		backgroundstr = "";
		background = 0;
	}

	for(int i = 0; i < g_numberOfInterestSets; i++) {
		while(g_setsOfInterest[i].size() > 0) {
			delete g_setsOfInterest[i][0];
		}
	}
	

	newClosest = 0;
}

//an item in the world, bouncing around to be picked up
class worldItem : public entity {
public:
	
	//to make items bounce, store an int from 0 to 2
	//sine will be done every frame three times
	//and every item's z will be modified based on the 
	// height of the sine corresponding to their index
	

	//make an entity from the file worlditem.ent
	worldItem(string fname, bool fisKeyItem) : entity(renderer, 5, fname) {
		isWorlditem = 1;
		name = "ITEM-" + fname;
		bounceindex = rand() % 3;
		g_worldItems.push_back(this);
		
	}

	~worldItem() {
		g_worldItems.erase(remove(g_worldItems.begin(), g_worldItems.end(), this), g_worldItems.end());
	}
};






#endif
