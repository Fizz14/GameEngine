#include "utils.h"
#include <string>
#include <sstream>

SDL_Texture* loadTexture(SDL_Renderer* renderer, string fileaddress)
{
  if(PHYSFS_exists(fileaddress.c_str())) 
  {
    PHYSFS_file* myfile = PHYSFS_openRead(fileaddress.c_str());
    PHYSFS_sint64 filesize = PHYSFS_fileLength(myfile);
    char* buf;
    buf = new char[filesize];
    int length_read = PHYSFS_readBytes(myfile, buf, filesize);
    SDL_RWops* myWop = SDL_RWFromMem(buf, filesize);
    SDL_Texture* texture = IMG_LoadTextureTyped_RW(renderer, myWop, 1, "QOI");
    PHYSFS_close(myfile);
    return texture;

  } else {
    E("FNF: " + fileaddress);
    abort();
    return nullptr;
  }
}

SDL_Surface* loadSurface(string fileaddress)
{
  if(PHYSFS_exists(fileaddress.c_str())) 
  {
    PHYSFS_file* myfile = PHYSFS_openRead(fileaddress.c_str());
    PHYSFS_sint64 filesize = PHYSFS_fileLength(myfile);
    char* buf;
    buf = new char[filesize];
    int length_read = PHYSFS_readBytes(myfile, buf, filesize);
    SDL_RWops* myWop = SDL_RWFromMem(buf, filesize);
    SDL_Surface* surface = IMG_LoadTyped_RW(myWop, 1, "QOI");
    PHYSFS_close(myfile);
    return surface;

  } else {
    E("FNF: " + fileaddress);
    abort();
    return nullptr;
  }
}

Mix_Chunk* loadWav(string fileaddress)
{
  if(PHYSFS_exists(fileaddress.c_str())) 
  {
    PHYSFS_file* myfile = PHYSFS_openRead(fileaddress.c_str());
    PHYSFS_sint64 filesize = PHYSFS_fileLength(myfile);
    char* buf;
    buf = new char[filesize];
    int length_read = PHYSFS_readBytes(myfile, buf, filesize);
    SDL_RWops* myWop = SDL_RWFromMem(buf, filesize);
    Mix_Chunk* myChunk = Mix_LoadWAV_RW(myWop, 1);
    PHYSFS_close(myfile);
    return myChunk;

  } else {
    E("FNF: " + fileaddress);
    abort();
    return nullptr;
  }
}

vector<string> loadText(string fileaddress)
{
  if(PHYSFS_exists(fileaddress.c_str())) 
  {
    PHYSFS_file* myfile = PHYSFS_openRead(fileaddress.c_str());
    PHYSFS_sint64 filesize = PHYSFS_fileLength(myfile);
    char* buf;
    buf = new char[filesize];
    int length_read = PHYSFS_readBytes(myfile, buf, filesize);

    PHYSFS_close(myfile);
    string myString(buf);
    vector<string> x = splitString(myString, '\n');
    x.pop_back();
    for(int i = 0; i < x.size(); i++) {
      x[i].pop_back();
    }
    delete buf;
    return x;

  } else {
    //E("FNF: " + fileaddress);
    //abort();
    return {};
  }

}

string loadTextAsString(string fileaddress)
{
  if(PHYSFS_exists(fileaddress.c_str())) 
  {
    PHYSFS_file* myfile = PHYSFS_openRead(fileaddress.c_str());
    PHYSFS_sint64 filesize = PHYSFS_fileLength(myfile);
    char* buf;
    buf = new char[filesize];
    int length_read = PHYSFS_readBytes(myfile, buf, filesize);

    PHYSFS_close(myfile);
    string myString(buf);
    delete buf;
    return myString;

  } else {
    //E("FNF: " + fileaddress);
    //abort();
    return {};
  }
}
