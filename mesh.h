#ifndef mesh_h
#define mesh_h

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "globals.h"
#include "physfs.h"
#include <iostream>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <array>
#include <string>
#include <map>
#include <set>

class vec2 {
public:
  float x = 0;
  float y = 0;
  float z = 0;
  vec3(int fx, int fy, int fz);
};

class vec3 {
public:
  float x = 0;
  float y = 0;
  float z = 0;
  vec3(int fx, int fy, int fz);
};

//a 3d vertex
class vertex3d {
public:
  float x = 0;
  float y = 0;
  float z = 0;
  SDL_Color color = {0,0,0,255};
  float u = 0;
  float v = 0;

  float lu = 0;
  float lv = 0;

  array<float, 3> normal = {0,0,0};
};

class face {
public:
  unsigned int a = 0;
  unsigned int b = 0;
  unsigned int c = 0;
  unsigned int d = 100;
};

std::map<int, int> findQuadFaces(const std::vector<vertex3d>& vertices, const std::vector<face>& faces);

enum meshtype {
  FLOOR,
  V_WALL, //visual only, no collision
  COLLISION,
  OCCLUDER,
  DECORATIVE
};

class mesh {
public:
  vec3 origin = {0,0,0};
  SDL_Texture* texture = NULL;
  string textureAddress = "";
  bool assetSharer = 0;

  SDL_Vertex* vertex = NULL;
  int* indices = NULL;
 
  vector<pair<float, float>> vertexExtraData;
  int numVertices = 0;
  int numIndices = 0;

  float sleepRadius = 0;

  vector<face> faces; //for 3d data, for determining z of entities ontop.
  
  vector<SDL_Vertex> oGeo; //screenspace geo for occluders
//  map<int, int> facePairing; //for finding quads in occluders
//  map<int, tuple<int, int, int, int>> twinMap; //maps the first int of facePairing to twin coordinates for where to draw the rectangle blocking the wall

  meshtype mtype = meshtype::FLOOR;

  bool visible = 1;

  vector<vertex3d> vertices;

  mesh();

  ~mesh();
};


//four meshes:
//a floor, a wall, a collision, and an occluder
//
//
//specifications for each type: (just in case I forget how to model them)
//
//floor - quads/tris with two uv channels
//wall - quads with two uv channels
//collision - completely vertical walls, inside corners should often be 90*
//occluder - edges in 3d space, no faces, no colors, no channels
//
class chunk {
 public:
  mesh* floor = 0;
  mesh* wall = 0;
  mesh* collision = 0;
  mesh* occluder = 0;

  string path = "";

  string floortex = "";
  string walltex = "";
  vec3 origin = {0,0,0};
  float scale;

  chunk(string fpath, string ffloortex, string fwalltex, vec3 forigin, float fscale);

  ~chunk();
};

mesh* loadMeshFromPly(string faddress, string taddress, vec3 forigin, float scale, meshtype fmtype);

#endif
