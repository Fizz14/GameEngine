#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "utils.h"
#include "happly.h"
#include "mesh.h"


void checkAndSetEdgeInfo(edgeInfo& ei, mesh* m) {
  struct VertexComparator {
    bool operator()(const SDL_Vertex& lhs, const SDL_Vertex& rhs) const {
      return std::tie(lhs.position.x, lhs.position.y) < std::tie(rhs.position.x, rhs.position.y);
    }
  };

  const float tolerance = 3.0f;

  auto addVertexToSet = [&](std::set<SDL_Vertex, VertexComparator>& vertexSet, const SDL_Vertex& vertex) {
    for (const auto& v : vertexSet) {
      if (std::fabs(v.position.x - vertex.position.x) < tolerance &&
          std::fabs(v.position.y - vertex.position.y) < tolerance) {
        return;
      }
    }
    vertexSet.insert(vertex);
  };

  for (const auto& f : m->faces) {
    SDL_Vertex va = m->vertex[f.a];
    SDL_Vertex vb = m->vertex[f.b];
    SDL_Vertex vc = m->vertex[f.c];
    SDL_Vertex vd = m->vertex[f.d];

    va.position.x += m->origin.x;
    va.position.y += m->origin.y;

    vb.position.x += m->origin.x;
    vb.position.y += m->origin.y;

    vc.position.x += m->origin.x;
    vc.position.y += m->origin.y;

    vd.position.x += m->origin.x;
    vd.position.y += m->origin.y;

    va.position.x = round(va.position.x);
    va.position.y = round(va.position.y);

    vb.position.x = round(vb.position.x);
    vb.position.y = round(vb.position.y);

    vc.position.x = round(vc.position.x);
    vc.position.y = round(vc.position.y);

    vd.position.x = round(vd.position.x);
    vd.position.y = round(vd.position.y);

    ei.first.position.x = round(ei.first.position.x);
    ei.first.position.y = round(ei.first.position.y);

    ei.second.position.x = round(ei.second.position.x);
    ei.second.position.y = round(ei.second.position.y);

    std::set<SDL_Vertex, VertexComparator> vertexSet;
    addVertexToSet(vertexSet, va);
    addVertexToSet(vertexSet, vb);
    addVertexToSet(vertexSet, vc);
    addVertexToSet(vertexSet, vd);
    addVertexToSet(vertexSet, ei.first);
    addVertexToSet(vertexSet, ei.second);
//    D(va.position.x);
//    D(va.position.y);
//    M("");
//    D(vb.position.x);
//    D(vb.position.y);
//    M("");
//    D(vc.position.x);
//    D(vc.position.y);
//    M("");
//    D(vd.position.x);
//    D(vd.position.y);
//    M("");
//    D(ei.first.position.x);
//    D(ei.first.position.y);
//    M("");
//    D(ei.second.position.x);
//    D(ei.second.position.y);
//    M("");

    if (vertexSet.size() <= 4) {
      m->edgeInfoSet = 1;
      ei.wallMesh = m;
      ei.indices = {f.a, f.b, f.c, f.d};
      return; // Exit the loop once a match is found
    }
  }
  W("Couldn't associate occluder edge with wall");
}

//this was changed from float to double to prevent tiny 1px gaps in occlusion from ggrids
//there are faster solutions (slight padding, additional occluder to bridge the gap
//if you're looking for a way to optimize
//maybe it doesn't matter?
vec3::vec3(float fx = 0, float fy = 0, float fz = 0) :x(fx), y(fy), z(fz) {}

mesh::mesh(){

}

mesh::~mesh() {
  if(mtype == meshtype::FLOOR) {
    g_meshFloors.erase(remove(g_meshFloors.begin(), g_meshFloors.end(), this), g_meshFloors.end());
  } else if(mtype == meshtype::V_WALL) {
    g_meshVWalls.erase(remove(g_meshVWalls.begin(), g_meshVWalls.end(), this), g_meshVWalls.end());
  } else if(mtype == meshtype::COLLISION) {
    g_meshCollisions.erase(remove(g_meshCollisions.begin(), g_meshCollisions.end(), this), g_meshCollisions.end());
  } else if(mtype == meshtype::OCCLUDER) {
    g_meshOccluders.erase(remove(g_meshOccluders.begin(), g_meshOccluders.end(), this), g_meshOccluders.end());
  } else if(mtype == meshtype::DECORATIVE) {
    g_meshDecorative.erase(remove(g_meshDecorative.begin(), g_meshDecorative.end(), this), g_meshDecorative.end());
  }

  g_meshes.erase(remove(g_meshes.begin(), g_meshes.end(), this), g_meshes.end());

  if(assetSharer == 0 && texture != nullptr) {
    SDL_DestroyTexture(texture);
  }


}

chunk::chunk(string fpath, string ffloortex, string fwalltex, vec3 forigin, float fscale, int fstandalone) {
  g_chunks.push_back(this);

  //look for models to load from fpath

  path = fpath;
  floortex = ffloortex;
  walltex = fwalltex;
  origin = forigin;
  scale = fscale;
  standalone = fstandalone;
  //D(standalone);

  string baseAddr = "resources/static/meshes/" + fpath;

  string floorAddr = baseAddr + "-f.ply";
  string wallAddr = baseAddr + "-w.ply";
  string collisionAddr = baseAddr + "-c.ply";
  string occluAddr = baseAddr + "-o.ply";
  string decorAddr = baseAddr + "-d.ply";

  if(PHYSFS_exists(floorAddr.c_str())) {
    floor = loadMeshFromPly(floorAddr, floortex, origin, scale, meshtype::FLOOR, standalone);
    //M("Loaded floor " + floorAddr);
  }
  if(PHYSFS_exists(wallAddr.c_str())) {
    wall = loadMeshFromPly(wallAddr, walltex, origin, scale, meshtype::V_WALL, standalone);
    //M("Loaded wall " + wallAddr);
  }
  if(PHYSFS_exists(collisionAddr.c_str())) {
    collision = loadMeshFromPly(collisionAddr, "", origin, scale, meshtype::COLLISION, standalone);
    //M("Loaded collision " + collisionAddr);
  }
  if(g_useOccluding && PHYSFS_exists(occluAddr.c_str())) {
    occluder = loadMeshFromPly(occluAddr, "", origin, scale, meshtype::OCCLUDER, standalone);
    //M("Loaded occluder " + occluAddr);
  }
  if(PHYSFS_exists(decorAddr.c_str())) {
    decorative = loadMeshFromPly(decorAddr, floortex, origin, scale, meshtype::DECORATIVE, standalone);
    //M("Loaded decoration " + decorAddr);
  }

}

chunk::chunk() {

}

chunk::~chunk() {
  g_chunks.erase(remove(g_chunks.begin(), g_chunks.end(), this), g_chunks.end());
  if(owner != 0) {
    owner->chunks.erase(remove(owner->chunks.begin(), owner->chunks.end(), this), owner->chunks.end());
  }
}

chunk* duplicateChunk(const chunk* original, vec3 newOrigin) {
  if (!original) return nullptr; // Handle null input safely

  //chunk* result = new chunk(original->path, original->floortex, original->walltex, newOrigin, original->scale, original->standalone);
  chunk* result = new chunk();
  result->floortex = "";
  result->walltex = "";
  result->scale = 1;
  result->origin = newOrigin;

  //result->value = original->value;

  // Duplicate mesh pointers using `duplicateMesh`
  result->floor = original->floor ? duplicateMesh(original->floor, newOrigin) : nullptr;
  result->wall = original->wall ? duplicateMesh(original->wall, newOrigin) : nullptr;
  result->collision = original->collision ? duplicateMesh(original->collision, newOrigin) : nullptr;
  result->occluder = original->occluder ? duplicateMesh(original->occluder, newOrigin) : nullptr;
  result->decorative = original->decorative ? duplicateMesh(original->decorative, newOrigin) : nullptr;

  if(result->occluder != 0) {
    vector<array<int, 2>> edgeData = original->occluder->edgeDataStore;
    for(array<int,2> n : edgeData) {
      vertex3d first = result->occluder->vertices[n[0]];
      vertex3d second = result->occluder->vertices[n[1]];

      SDL_Vertex A;

      A.position.x = ((-first.x) * result->scale);
      //A.position.y = ((first.y * scale)) * XtoY - ((first.z * scale)) * XtoZ;
      A.position.y = ((first.y * result->scale)) * XtoY;
      A.position.x += newOrigin.x;
      A.position.y += newOrigin.y;
      A.color.r = 0;
      A.color.g = 0;
      A.color.b = 0;
      A.color.a = 255;

      SDL_Vertex B;

      B.position.x = ((-second.x) * result->scale);
      //B.position.y = ((second.y * scale)) * XtoY - ((second.z * scale)) * XtoZ;
      B.position.y = ((second.y * result->scale)) * XtoY;

      B.position.x += newOrigin.x;
      B.position.y += newOrigin.y;
      B.color.r = 0;
      B.color.g = 0;
      B.color.b = 0;
      B.color.a = 255;

      edgeInfo ei;
      ei.first = A;
      ei.firstZ = ((first.z * result->scale)) * XtoZ; //z is subtracted from y
      ei.second = B;
      ei.secondZ = ((second.z * result->scale)) * XtoZ; //z is subtracted from y

      //this was written with the assumption that all occluders have an accompanying wall

      if(g_meshVWalls.size() > 0 && g_meshVWalls[g_meshVWalls.size()-1]->edgeInfoSet == 0) {
        checkAndSetEdgeInfo(ei, g_meshVWalls[g_meshVWalls.size()-1]);
      }

      ei.type = 0;
      g_oEdges.emplace_back(ei);
    }
  }


  if( result->wall != nullptr) {
    for (const auto& f : result->wall->faces) {
      if(result->wall->vertices[f.a].color.r < 128 && result->wall->vertices[f.b].color.r < 128) {
        vertex3d first = original->wall->vertices[f.a];
        vertex3d second = original->wall->vertices[f.b];
        SDL_Vertex A;
  
        A.position.x = ((-first.x) * result->scale);
        A.position.y = ((first.y * result->scale)) * XtoY - ((first.z * result->scale)) * XtoZ;
        //A.position.y = ((first.y * scale)) * XtoY;
  
        SDL_Vertex B;
  
        B.position.x = ((-second.x) * result->scale);
        B.position.y = ((second.y * result->scale)) * XtoY - ((second.z * result->scale)) * XtoZ;
        //B.position.y = ((second.y * scale)) * XtoY;
  
        A.position.x += newOrigin.x + 64; //adding 64 is a bandaid solution and may cause problems later
                                          // !!!
        A.position.y += newOrigin.y;
        B.position.x += newOrigin.x + 64;
        B.position.y += newOrigin.y;
  
        edgeInfo ei;
        ei.first = A;
        ei.firstZ = ((first.z * result->scale)) * XtoZ; //z is subtracted from y
  
  
        ei.second = B;
        ei.secondZ = ((second.z * result->scale)) * XtoZ; //z is subtracted from y
  
  
        if(ei.first.position.x > ei.second.position.x) {
          swap(ei.first, ei.second);
        }
  
        ei.type = 1;
        g_wEdges.emplace_back(ei);
      }
  
  
      if(result->wall->vertices[f.a].color.r < 128 && result->wall->vertices[f.c].color.r < 128) {
        vertex3d first = result->wall->vertices[f.a];
        vertex3d second = result->wall->vertices[f.c];
        SDL_Vertex A;
  
        A.position.x = ((-first.x) * result->scale);
        A.position.y = ((first.y * result->scale)) * XtoY - ((first.z * result->scale)) * XtoZ;
  
        SDL_Vertex B;
  
        B.position.x = ((-second.x) * result->scale);
        B.position.y = ((second.y * result->scale)) * XtoY - ((second.z * result->scale)) * XtoZ;
  
        A.position.x += newOrigin.x + 64;
        A.position.y += newOrigin.y;
        B.position.x += newOrigin.x + 64;
        B.position.y += newOrigin.y;
  
        edgeInfo ei;
        ei.first = A;
        ei.second = B;
        g_wEdges.emplace_back(ei);
      }
  
      if(result->wall->vertices[f.c].color.r < 128 && result->wall->vertices[f.b].color.r < 128) {
        vertex3d first = result->wall->vertices[f.c];
        vertex3d second = result->wall->vertices[f.b];
        SDL_Vertex A;
  
        A.position.x = ((-first.x) * result->scale);
        A.position.y = ((first.y * result->scale)) * XtoY - ((first.z * result->scale)) * XtoZ;
  
        SDL_Vertex B;
  
        B.position.x = ((-second.x) * result->scale);
        B.position.y = ((second.y * result->scale)) * XtoY - ((second.z * result->scale)) * XtoZ;
  
        A.position.x += newOrigin.x + 64;
        A.position.y += newOrigin.y;
        B.position.x += newOrigin.x + 64;
        B.position.y += newOrigin.y;
  
        edgeInfo ei;
        ei.first = A;
        ei.second = B;
        g_wEdges.emplace_back(ei);
      }
    }
  }

  g_chunks.push_back(result); // Store in global chunk list

  return result;
}

// Function to calculate the normal of a face
array<float, 3> calculateNormal(const vertex3d& v0, const vertex3d& v1, const vertex3d& v2) {
  array<float, 3> normal;
  float x1 = v1.x - v0.x;
  float y1 = v1.y - v0.y;
  float z1 = v1.z - v0.z;
  float x2 = v2.x - v0.x;
  float y2 = v2.y - v0.y;
  float z2 = v2.z - v0.z;

  normal[0] = y1 * z2 - z1 * y2;
  normal[1] = z1 * x2 - x1 * z2;
  normal[2] = x1 * y2 - y1 * x2;

  float length = sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
  normal[0] /= length;
  normal[1] /= length;
  normal[2] /= length;

  return normal;
}

// Function to calculate the vertex colors based on the light direction
void setVertexColors(vector<vertex3d>& vertices, const vector<face>& faces, const array<float, 3>& lightDir, meshtype mtype) {
  // Initialize vertex normals and counts
  for (auto& vertex : vertices) {
    vertex.normal = {0.0f, 0.0f, 0.0f};
  }

  // Accumulate normals for each vertex
  for (const auto& f : faces) {
    if(f.d < 100) {
      array<float, 3> normal = calculateNormal(vertices[f.a], vertices[f.b], vertices[f.c]);
      vertex3d* verticesArray[4] = {&vertices[f.a], &vertices[f.b], &vertices[f.c], &vertices[f.d]};
      for (int i = 0; i < 4; ++i) {
        verticesArray[i]->normal[0] += normal[0];
        verticesArray[i]->normal[1] += normal[1];
        verticesArray[i]->normal[2] += normal[2];
      }
    } else {
      array<float, 3> normal = calculateNormal(vertices[f.a], vertices[f.b], vertices[f.c]);
      vertex3d* verticesArray[3] = {&vertices[f.a], &vertices[f.b], &vertices[f.c]};
      for (int i = 0; i < 3; ++i) {
        verticesArray[i]->normal[0] += normal[0];
        verticesArray[i]->normal[1] += normal[1];
        verticesArray[i]->normal[2] += normal[2];
      }
    }
  }

  // Normalize and set vertex colors
  for (auto& vertex : vertices) {
    float length = sqrt(vertex.normal[0] * vertex.normal[0] + vertex.normal[1] * vertex.normal[1] + vertex.normal[2] * vertex.normal[2]);
    vertex.normal[0] /= length;
    vertex.normal[1] /= length;
    vertex.normal[2] /= length;

    float dotProduct = max(0.0f, vertex.normal[0] * lightDir[0] + vertex.normal[1] * lightDir[1] + vertex.normal[2] * lightDir[2]);
    if(mtype == meshtype::FLOOR || mtype == meshtype::DECORATIVE) {
      dotProduct = 0.75 + 0.3*dotProduct;
      if(dotProduct > 1) {dotProduct = 1;}
    }
    int intensity = 255 * dotProduct;

    if(mtype == meshtype::V_WALL) {
      //don't change red channel
      vertex.color.g = intensity;
      vertex.color.b = intensity;
      vertex.color.a = 255;
    } else {
      //vertex.color.r = intensity;
      vertex.color.g = intensity;
      vertex.color.b = intensity;
    }
  }
}


mesh* loadMeshFromPly(string faddress, string taddress, vec3 forigin, float scale, meshtype fmtype, int standalone) {
  string address = faddress;
  vector<vertex3d> vertices;
  vector<face> faces;
  mesh* result = new mesh();
  result->origin = forigin;
  result->mtype = fmtype;


  //if a mesh is standalone, it needs a texture
  if(taddress != "" && standalone) {
    result->textureAddress = taddress;
    for(auto x : g_meshes) {
      if(x->textureAddress == result->textureAddress) {
        result->texture = x->texture;
        result->assetSharer = 1;
      }
    }

    if(!result->assetSharer) {
      result->texture = loadTexture(renderer, "resources/static/diffuse/" + result->textureAddress + ".qoi");
    }
  }

  if(fmtype == meshtype::FLOOR) {
    g_meshFloors.push_back(result);
  } else if (fmtype == meshtype::COLLISION) {
    g_meshCollisions.push_back(result);
  } else if(fmtype == meshtype::OCCLUDER) {
    g_meshOccluders.push_back(result);
  } else if(fmtype == meshtype::V_WALL) {
    g_meshVWalls.push_back(result);
  } else if(fmtype == meshtype::DECORATIVE) {
    g_meshDecorative.push_back(result); 
  }
  g_meshes.push_back(result);


  string binAddress = "";

  if (PHYSFS_exists(address.c_str())) {
    PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
    PHYSFS_file* myfile = PHYSFS_openRead(address.c_str());
    error = PHYSFS_getLastErrorCode();

    if (error != 0) {
      cerr << "Error opening file: " << address << " Error: " << PHYSFS_getErrorByCode(error) << endl;
      abort();
    }

    PHYSFS_sint64 filesize = PHYSFS_fileLength(myfile);
    char* buf = new char[filesize];
    PHYSFS_readBytes(myfile, buf, filesize);
    PHYSFS_close(myfile);

    // Use happly to parse the buffer
    istringstream plyStream(string(buf, filesize));
    happly::PLYData plyIn(plyStream);

    delete[] buf;

    // Get vertex data
    vector<array<double, 3>> vertexData = plyIn.getVertexPositions();
    vector<array<unsigned char, 3>> vertexColors;
    vector<array<double, 2>> vertexUVs;
    vector<array<double, 2>> vertexLUVs;

    // Check if the .ply file contains color and UV data
    if (plyIn.hasElement("vertex") && plyIn.getElement("vertex").hasProperty("red")) {
      vertexColors = plyIn.getVertexColors();
    }

    if (plyIn.hasElement("vertex") && plyIn.getElement("vertex").hasProperty("s")) {
      vertexUVs = plyIn.getVertexUVs();
    }

    if (plyIn.hasElement("vertex") && plyIn.getElement("vertex").hasProperty("u")) {
      vertexLUVs = plyIn.getVertexLUVs();
    }

    // Convert to vertex3d
    for (size_t i = 0; i < vertexData.size(); ++i) {
      vertex3d v;
      v.x = static_cast<float>(vertexData[i][0]);
      v.y = static_cast<float>(vertexData[i][1]);
      v.z = static_cast<float>(vertexData[i][2]);

      if (i < vertexUVs.size()) {
        v.u = static_cast<float>(vertexUVs[i][0]);
        v.v = 1 - static_cast<float>(vertexUVs[i][1]);
      }
      if( i < vertexLUVs.size()) {
        v.lu = static_cast<float>(vertexLUVs[i][0]);
        v.lv = 1 - static_cast<float>(vertexLUVs[i][1]);
      }

      if( i < vertexColors.size()) {
        v.color.r = vertexColors[i][0];
        v.color.g = vertexColors[i][1];
        v.color.b = vertexColors[i][2];
      }

      vertices.push_back(v);
    }


    // Get face data
    vector<vector<size_t>> faceIndices;
    if(fmtype != meshtype::OCCLUDER) { //occluders have edges and no faces
      faceIndices = plyIn.getFaceIndices<size_t>();
    }

    // Convert to face objects
    for (const auto& f : faceIndices) {
      if (f.size() == 4) {
        faces.push_back({f[0], f[1], f[2], f[3]});
      } else if (f.size() == 3 && (fmtype == meshtype::FLOOR || fmtype == meshtype::DECORATIVE)) {
        face n;
        n.a = f[0];
        n.b = f[1];
        n.c = f[2];
        n.d = 100;
        faces.push_back(n);
      } else {
        E("");
        E("Floors can have quads or tris, walls and collisions must have quads, and occluders can have edges.");
        D(f.size());
        D(faces.size());
        E("");
      }
    }

    {
      //now is the time to set texture coords procedurally and add loopcuts to reset the tex coords if needed

      //make sure that the first uv map for floors and walls
      //starts in the top-left corner, as-in, no uv coords less than 0 (either axis)
      //but greater than 1 is okay
      //Also, no face can have it's individual unwrap span larger than the distance from 0->1
      //subdivide in that case

    }



    if(
        fmtype == meshtype::COLLISION ||
        fmtype == meshtype::FLOOR ||
        fmtype == meshtype::DECORATIVE
      ) {
      const array<float, 3> lightDir = {0, 0.4472, 0.8944};
      setVertexColors(vertices, faces, lightDir, fmtype);
    } else if(fmtype == meshtype::V_WALL) {
      //const array<float, 3> lightDir = {0, 0.707, 0.707};
      //const array<float, 3> lightDir = {0, 1, 0};
      //setVertexColors(vertices, faces, lightDir, fmtype);
      for(int i = 0; i < vertices.size(); i++) {
        vertices[i].color.g = 220;
        vertices[i].color.b = 220;
        vertices[i].color.a = 255;
      }
    }



    // Transform 3D coordinates to 2D and set up SDL_Vertex array
    result->vertex = new SDL_Vertex[vertices.size()];
    result->vertexExtraData = vector<pair<float, float>>(vertices.size());

    result->faces = faces;
    for (const auto& f : faces) {
      int fail = 0;

      if( fmtype == meshtype::V_WALL) {
        result->vertices = vertices;
        if(vertices[f.a].color.r < 128 && vertices[f.b].color.r < 128) {
          vertex3d first = vertices[f.a];
          vertex3d second = vertices[f.b];
          SDL_Vertex A;

          A.position.x = ((-first.x) * scale);
          A.position.y = ((first.y * scale)) * XtoY - ((first.z * scale)) * XtoZ;
          //A.position.y = ((first.y * scale)) * XtoY;

          SDL_Vertex B;

          B.position.x = ((-second.x) * scale);
          B.position.y = ((second.y * scale)) * XtoY - ((second.z * scale)) * XtoZ;
          //B.position.y = ((second.y * scale)) * XtoY;

          A.position.x += forigin.x;
          A.position.y += forigin.y;
          B.position.x += forigin.x;
          B.position.y += forigin.y;

          edgeInfo ei;
          ei.first = A;
          ei.firstZ = ((first.z * scale)) * XtoZ; //z is subtracted from y


          ei.second = B;
          ei.secondZ = ((second.z * scale)) * XtoZ; //z is subtracted from y


          if(ei.first.position.x > ei.second.position.x) {
            swap(ei.first, ei.second);
          }

          ei.type = 1;
          g_wEdges.emplace_back(ei);
          fail++;
        }


        if(vertices[f.a].color.r < 128 && vertices[f.c].color.r < 128) {
          if(fail) {E("Bad V_WALL, make sure only the bottom verts have 0 red"); abort();}
          vertex3d first = vertices[f.a];
          vertex3d second = vertices[f.c];
          SDL_Vertex A;

          A.position.x = ((-first.x) * scale);
          A.position.y = ((first.y * scale)) * XtoY - ((first.z * scale)) * XtoZ;

          SDL_Vertex B;

          B.position.x = ((-second.x) * scale);
          B.position.y = ((second.y * scale)) * XtoY - ((second.z * scale)) * XtoZ;

          A.position.x += forigin.x;
          A.position.y += forigin.y;
          B.position.x += forigin.x;
          B.position.y += forigin.y;

          edgeInfo ei;
          ei.first = A;
          ei.second = B;
          g_wEdges.emplace_back(ei);
          fail++;
        }

        if(vertices[f.c].color.r < 128 && vertices[f.b].color.r < 128) {
          if(fail) {E("Bad V_WALL, make sure only the bottom verts have 0 red"); abort();}
          vertex3d first = vertices[f.c];
          vertex3d second = vertices[f.b];
          SDL_Vertex A;

          A.position.x = ((-first.x) * scale);
          A.position.y = ((first.y * scale)) * XtoY - ((first.z * scale)) * XtoZ;

          SDL_Vertex B;

          B.position.x = ((-second.x) * scale);
          B.position.y = ((second.y * scale)) * XtoY - ((second.z * scale)) * XtoZ;

          A.position.x += forigin.x;
          A.position.y += forigin.y;
          B.position.x += forigin.x;
          B.position.y += forigin.y;

          edgeInfo ei;
          ei.first = A;
          ei.second = B;
          g_wEdges.emplace_back(ei);
        }
      }

    }



    int index = 0;
    float maxDistanceFromOrigin = 0;
    for( auto v : vertices) {
      result->vertex[index].position.x = ((-v.x) * scale);
      result->vertex[index].position.y = ((v.y * scale)) * XtoY;
      float dist = Distance(result->vertex[index].position.x, result->vertex[index].position.y * XtoY, 0, 0);
      result->vertex[index].position.y -= ((v.z * scale)) * XtoZ;
      result->vertex[index].tex_coord.y = v.v;
      result->vertex[index].color = v.color;
      result->vertex[index].tex_coord.x = v.u;
      result->vertexExtraData[index].first = v.lu;
      result->vertexExtraData[index].second = v.lv;
      if(dist > maxDistanceFromOrigin) {
        maxDistanceFromOrigin = dist;
      }
      if(fmtype == meshtype::FLOOR || fmtype == meshtype::DECORATIVE) {
        result->vertex[index].color.a = v.color.r;
        result->vertex[index].color.r = v.color.g;
      }


      ++index;
    }

    result->numVertices = vertices.size();

    //now check to make sure that the alpha is not all 0
    if(devMode) {
      bool good = 0;
      for(int i = 0; i < result->numVertices; i++) {
        if(result->vertex[i].color.a != 0) { good = 1; break;}

      }
      if(!good) {
        //this is triggering sometimes??
        E("Mesh is completely transparent. (Make sure you paint it. The red channel will be used for opacity in this case) \nTHIS MESSAGE ONLY PRINTS IN DEVMODE.");
        D(faddress);
      }
    }

    result->indices = new int[faces.size() * 6];
    result->numIndices = 0;
    for (const auto& f : faces) {
      if(f.d < 100) {
        result->indices[result->numIndices] = f.a;
        result->numIndices++;
        result->indices[result->numIndices] = f.b;
        result->numIndices++;
        result->indices[result->numIndices] = f.c;
        result->numIndices++;

        result->indices[result->numIndices] = f.a;
        result->numIndices++;
        result->indices[result->numIndices] = f.c;
        result->numIndices++;
        result->indices[result->numIndices] = f.d;
        result->numIndices++;
      } else {
        result->indices[result->numIndices] = f.a;
        result->numIndices++;
        result->indices[result->numIndices] = f.b;
        result->numIndices++;
        result->indices[result->numIndices] = f.c;
        result->numIndices++;
      }
    }

    if(fmtype == meshtype::OCCLUDER) {
      vector<array<int, 2>> edgeData = plyIn.getEdges();
      result->edgeDataStore = edgeData; //store this for ggrid pieces
      result->vertices = vertices;
      for(array<int,2> n : edgeData) {
        vertex3d first = vertices[n[0]];
        vertex3d second = vertices[n[1]];

        SDL_Vertex A;

        A.position.x = ((-first.x) * scale);
        //A.position.y = ((first.y * scale)) * XtoY - ((first.z * scale)) * XtoZ;
        A.position.y = ((first.y * scale)) * XtoY;
        A.position.x += forigin.x;
        A.position.y += forigin.y;
        A.color.r = 0;
        A.color.g = 0;
        A.color.b = 0;
        A.color.a = 255;

        SDL_Vertex B;

        B.position.x = ((-second.x) * scale);
        //B.position.y = ((second.y * scale)) * XtoY - ((second.z * scale)) * XtoZ;
        B.position.y = ((second.y * scale)) * XtoY;

        B.position.x += forigin.x;
        B.position.y += forigin.y;
        B.color.r = 0;
        B.color.g = 0;
        B.color.b = 0;
        B.color.a = 255;

        edgeInfo ei;
        ei.first = A;
        ei.firstZ = ((first.z * scale)) * XtoZ; //z is subtracted from y
        ei.second = B;
        ei.secondZ = ((second.z * scale)) * XtoZ; //z is subtracted from y

        //this was written with the assumption that all occluders have an accompanying wall

        if(g_meshVWalls.size() > 0 && g_meshVWalls[g_meshVWalls.size()-1]->edgeInfoSet == 0) {
          checkAndSetEdgeInfo(ei, g_meshVWalls[g_meshVWalls.size()-1]);
        }

        ei.type = 0;
        g_oEdges.emplace_back(ei);
      }
    }



    result->sleepRadius = maxDistanceFromOrigin;
    result->faces = faces;
    for(auto& x : vertices) {
      x.x *= -scale;
      x.y *= scale * XtoY;
      x.z *= scale;
    }

    //needed for collisions and floors
    if(fmtype == meshtype::COLLISION ||
        fmtype == meshtype::FLOOR ||
        fmtype == meshtype::V_WALL ||
        fmtype == meshtype::DECORATIVE
      ) {
      result->vertices = vertices;
    }
  } else {
    cerr << "File does not exist: " << address << endl;
    //breakpoint();
  }
  return result;
}

mesh* duplicateMesh(const mesh* original, vec3 origin) {
  if (!original) return nullptr; // Handle null input safely

  mesh* result = new mesh();

  // Copy primitive and STL container members
  result->origin = origin;
  result->textureAddress = original->textureAddress;
  //result->assetSharer = original->assetSharer;
  result->assetSharer = 1;
  result->numVertices = original->numVertices;
  result->numIndices = original->numIndices;
  result->sleepRadius = original->sleepRadius;
  result->mtype = original->mtype;
  result->drawDiffuse = original->drawDiffuse;
  //result->edgeInfoSet = original->edgeInfoSet;
  result->visible = original->visible;
  result->edgeDataStore = original->edgeDataStore;

  result->vertexExtraData = original->vertexExtraData;
  result->faces = original->faces;
  result->oGeo = original->oGeo;
  result->vertices = original->vertices;

  // Deep copy dynamically allocated data
  if (original->vertex) {
    result->vertex = new SDL_Vertex[original->numVertices];
    memcpy(result->vertex, original->vertex, sizeof(SDL_Vertex) * original->numVertices);
  } else {
    result->vertex = nullptr;
  }

  if (original->indices) {
    result->indices = new int[original->numIndices];
    memcpy(result->indices, original->indices, sizeof(int) * original->numIndices);
  } else {
    result->indices = nullptr;
  }

  // Push result into the correct global arrays
  switch (result->mtype) {
    case meshtype::FLOOR:
      g_meshFloors.push_back(result);
      break;
    case meshtype::COLLISION:
      g_meshCollisions.push_back(result);
      break;
    case meshtype::OCCLUDER:
      g_meshOccluders.push_back(result);
      break;
    case meshtype::V_WALL:
      g_meshVWalls.push_back(result);
      break;
    case meshtype::DECORATIVE:
      g_meshDecorative.push_back(result);
      break;
  }

  g_meshes.push_back(result);

  return result;
}

ggrid::ggrid() {
  g_ggrids.push_back(this);
}

ggrid::~ggrid() {
  SDL_DestroyTexture(walltex);
  SDL_DestroyTexture(floortex);
  g_ggrids.erase(remove(g_ggrids.begin(), g_ggrids.end(), this), g_ggrids.end());
}
