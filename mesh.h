#ifndef mesh_h
#define mesh_h

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "globals.h"
#include "physfs.h"
#include "objects.h"
#include <iostream>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <array>
#include <string>
#include <map>
#include <set>

struct edgeInfo;

class vec2 {
public:
  float x = 0;
  float y = 0;
  vec2(float fx, float fy);
};

class vec3 {
public:
  float x = 0;
  float y = 0;
  float z = 0;
  vec3(float fx, float fy, float fz);
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

  SDL_Texture* trimTexture = NULL;

  bool storedInMeshVectors = 1;
  vector<SDL_Vertex> vbuffer;

  bool drawDiffuse = 1;
  bool drawShading = 1;
  bool hasTrim = 0;

  int topOrBottomShading = 0; //0 -> top 1 -> bottom 2 -> both 3 -> 3tall, top 4-> 3tall, bot

  string textureAddress = "";
  bool assetSharer = 0;

  SDL_Vertex* vertex = NULL;
  int* indices = NULL;
 
  vector<pair<float, float>> vertexExtraData = {};
  int numVertices = 0;
  int numIndices = 0;

  bool ggridPiece = 0; //turn off using the floors for height

  float sleepRadius = 0;

  vector<face> faces = {}; //for 3d data, for determining z of entities ontop.
  
  vector<SDL_Vertex> oGeo = {}; //screenspace geo for occluders
//  map<int, int> facePairing; //for finding quads in occluders
//  map<int, tuple<int, int, int, int>> twinMap; //maps the first int of facePairing to twin coordinates for where to draw the rectangle blocking the wall

  meshtype mtype = meshtype::FLOOR;

  bool edgeInfoSet = 0;
  vector<array<int, 2>> edgeDataStore = {};

  bool visible = 1;
  bool awake = 0;

  vector<vertex3d> vertices = {};
  vector<edgeInfo> storedWEdges = {};





  mesh();
  
  ~mesh();
};


// guide:
//four meshes:
//a floor, a wall, a collision, and an occluder
//
//
//specifications for each type: (just in case I forget how to model them)
//
//floor - quads/tris with two uv channels. Needs vertex colors with red for opacity (full red to be visible)
//wall - quads with two uv channels, same requirement for vertex colors as floor
//decorative - same as floor
//collision - completely vertical walls, inside corners should often be 90*
// !!! For collisions, ensure that the lower verts of the quad are at z=0 !!!
//occluder - edges in 3d space, no faces, no color channels, no uv data
//

class ggrid;

class chunk {
 public:


  bool standalone = 1; //0 if this belongs to a ggrid
  int value = 0; //if not standalone this is used for the ggrid
  ggrid* owner = nullptr;

  mesh* floor = 0;
  mesh* wall = 0;
  mesh* collision = 0;
  mesh* occluder = 0;
  mesh* decorative = 0;

  string path = "";

  string floortex = "";
  string walltex = "";
  vec3 origin = {0,0,0};
  float scale;

  chunk(string fpath, string ffloortex, string fwalltex, vec3 forigin, float fscale, int fstandalone, float fzscale);

  chunk(const chunk &a);
  chunk();

  ~chunk();
};

chunk* duplicateChunk(const chunk* original, vec3 newOrigin, vector<bool> whichMeshes);

mesh* loadMeshFromPly(string faddress, string taddress, vec3 forigin, float scale, meshtype fmtype, int standalone, float fzscale);

mesh* duplicateMesh(const mesh* original, vec3 origin);

// a Geometrygrid, which contains an Array of Data for generating Chunks for interiors
//
// basic Support includes the Ability to create and delete a Ggrid, select a Ggrid by index,
// and set the Cell of a Ggrid to a certain chunk.
//
class ggrid {
  public:

    int layer = 0;

    string walltexSTR = "";
    string floortexSTR = "";
    string trimtexSTR = "notrim";
    SDL_Texture* walltex = 0;
    SDL_Texture* floortex = 0;
    SDL_Texture* trimtex = 0;

    //1 if the ggrid is using another ggrid's texture
    bool borrowingWallTex = 0;
    bool borrowingFloorTex = 0;
    bool borrowingTrimTex = 0;

    bool hasWall = 0;
    bool hasFloor = 0;
    bool hasTrim = 0;

    int wallShading = 0; // 0 ->none
                         // 1 -> upper shading
                         // 2 -> lower shading
    bool hasBotShading = 0;


    //bounds of the grid
    int originX = 0; //in coords
    int originY = 0;
    int originZ = 0;
    int width = 0; //in blocks
    int height = 0;

    vector<vector<unsigned char>> chunkdata;

    vector<chunk*> chunks;

    ggrid();

    ~ggrid();
};

#endif
