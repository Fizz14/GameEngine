#include <SDL2/SDL.h>        
#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>
using namespace std;

vector<vector<int>> g_fogcookies;

SDL_Texture* addTextures(SDL_Renderer* renderer, vector<vector<int>> fogcookies, SDL_Texture* &illuminateMe, SDL_Texture* &lightspot, int widthOfIlluminateMe, int heightOfIlluminateMe, int paddingx, int paddingy) {
	SDL_SetRenderTarget(renderer, illuminateMe);
	SDL_RenderClear(renderer);
	for(int i = 0; i < fogcookies.size(); i++) {
		for(int j = 0; j < fogcookies[0].size(); j++) {
			if(fogcookies[i][j]) {
				//render the lightspot
				SDL_Rect dstrect = {i * (widthOfIlluminateMe / fogcookies.size()) - paddingx, j * (heightOfIlluminateMe/fogcookies[0].size()) - paddingy, widthOfIlluminateMe/fogcookies.size() + paddingx *2, heightOfIlluminateMe/fogcookies.size() + paddingy * 2};
				SDL_RenderCopy(renderer, lightspot, NULL, &dstrect);

				
			}
		} 
	}
	
	//illuminateMe is properly lit
	SDL_SetRenderTarget(renderer, NULL);
	return illuminateMe;
}

// void addTextures(SDL_Renderer* renderer, SDL_Texture* &canvas, SDL_Texture* &stamp) {
// 	SDL_SetRenderTarget(renderer, canvas);
// 	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
// 	SDL_RenderClear(renderer);
// 	SDL_RenderCopy(renderer, stamp, NULL, NULL);
// 	SDL_SetRenderTarget(renderer, NULL);
// }

SDL_Texture* IlluminateTexture(SDL_Renderer* renderer, SDL_Texture* &mask, SDL_Texture* &diffuse, SDL_Texture* &result) {
	SDL_SetRenderTarget(renderer, result);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, diffuse, NULL, NULL);
	SDL_RenderCopy(renderer, mask, NULL, NULL);
	SDL_SetRenderTarget(renderer, NULL);
	return result;
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Texture Blending",
	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 300, 300, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,  SDL_RENDERER_ACCELERATED);
	
    SDL_Surface* SurfaceA = IMG_Load("misc/a.png");
	SDL_Surface* SurfaceB = IMG_Load("misc/b.png");
	
	SDL_Texture* TextureA = SDL_CreateTextureFromSurface(renderer, SurfaceA);
	SDL_Texture* TextureB = SDL_CreateTextureFromSurface(renderer, SurfaceB);

	SDL_FreeSurface(SurfaceA);
	SDL_FreeSurface(SurfaceB);

	int xpos = 50;
	int ypos = 50;	
    SDL_Texture* TextureC;// = IlluminateTexture(renderer, TextureA, TextureB, result, xpos, ypos, 250, 250);

    //SDL_Surface* SurfaceD = IMG_Load("misc/test.png");
    //SDL_Texture* TextureD = SDL_CreateTextureFromSurface(renderer, SurfaceD);

	//SDL_RenderCopy(renderer, TextureD, NULL, NULL);
	SDL_Texture* result = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 300, 300);
	SDL_SetTextureBlendMode(result, SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(TextureA, SDL_BLENDMODE_MOD);
	SDL_SetTextureBlendMode(TextureB, SDL_BLENDMODE_NONE);

	SDL_SetRenderDrawColor(renderer, 0,0,0,0);
	SDL_RenderPresent(renderer);
	SDL_GL_SetSwapInterval(1);
    bool quit = false;
    SDL_Event event;
	float speed = 5;
	int ticks = 16;
	int lastticks = 16;
	int elapsed = 16;

	//textures for adding operation
	SDL_Texture* canvas = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 300, 300);
	
	SDL_Surface* lightSurface = IMG_Load("misc/light.png");
	SDL_Texture* light = SDL_CreateTextureFromSurface(renderer, lightSurface);
	SDL_FreeSurface(lightSurface);


	vector<int> column1 = {1, 1, 1, 1, 1};
	vector<int> column2 = {1, 1, 1, 1, 1};
	vector<int> column3 = {1, 1, 1, 1, 1};
	vector<int> column4 = {1, 1, 1, 1, 1};
	vector<int> column5 = {1, 1, 1, 1, 1};
	
	g_fogcookies.push_back(column1);
	g_fogcookies.push_back(column2);
	g_fogcookies.push_back(column3);
	g_fogcookies.push_back(column4);
	g_fogcookies.push_back(column5);



    while (!quit) { 
        while( SDL_PollEvent( &event ) ){
            switch(event.type) {

				case SDL_KEYDOWN:
					switch(event.key.keysym.sym) {
					case SDLK_ESCAPE: 
						quit = true;
						break;
					case SDLK_LEFT:
						g_fogcookies[0][0] = ! g_fogcookies[0][0];
						break;
					case SDLK_RIGHT:
						g_fogcookies[0][1] = ! g_fogcookies[0][1];xpos += speed;
						break;
					case SDLK_UP:
						g_fogcookies[1][0] = ! g_fogcookies[1][0];ypos -= speed;
						break;
					case SDLK_DOWN:
						g_fogcookies[1][1] = ! g_fogcookies[1][1];ypos+= speed;
						break;
					// cases for other keypresses
					}
					break;
				// cases for other events
				

				
				case SDL_QUIT:
                    quit = 1;
                    break;
            }
        }
		addTextures(renderer, g_fogcookies, canvas, light, 300, 300, 10, 10);


		SDL_Texture* TextureC = IlluminateTexture(renderer, TextureA, canvas, result);
		
		ticks = SDL_GetTicks();
		elapsed = ticks - lastticks;
		lastticks = ticks;
		
		//render graphics
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, TextureC, NULL, NULL);
		//SDL_RenderCopy(renderer, canvas, NULL, NULL);
		SDL_RenderPresent(renderer);
	}

}
