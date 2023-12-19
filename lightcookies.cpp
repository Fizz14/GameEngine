#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>

#include "lightcookies.h"
#include "globals.h"

using namespace std;

SDL_Texture *addTextures(SDL_Renderer *renderer, vector<vector<int>> fogcookies, SDL_Texture *&illuminateMe, SDL_Texture *&lightspot, int widthOfIlluminateMe, int heightOfIlluminateMe, int paddingx, int paddingy, int layer)
{
	SDL_SetRenderTarget(renderer, illuminateMe);
	SDL_RenderClear(renderer);
	paddingy = paddingx * (heightOfIlluminateMe / widthOfIlluminateMe);
	SDL_Texture *usingThisTexture = lightspot;

	for (long long unsigned int i = 0; i < fogcookies.size(); i++)
	{
		for (long long unsigned int j = 0; j < fogcookies[0].size(); j++)
		{
			if (fogcookies[i][j])
			{
				usingThisTexture = lightspot;

				if (layer)
				{
					if (g_shc[i][j] == 0)
					{
						usingThisTexture = lightc;
					}
					else if (g_shc[i][j] == 1)
					{
						usingThisTexture = lightb;
					}
					else if (g_shc[i][j] == 2)
					{
						usingThisTexture = lighta;
					}
					else if (g_shc[i][j] == 3)
					{
						usingThisTexture = lightd;
					}
					else if (g_shc[i][j] == 4)
					{
						usingThisTexture = lightcro;
					}
					else if (g_shc[i][j] == 5)
					{
						usingThisTexture = lightbro;
					}
					else if (g_shc[i][j] == 6)
					{
						usingThisTexture = lightaro;
					}
					else if (g_shc[i][j] == 7)
					{
						usingThisTexture = lightdro;
					}
					else if (g_shc[i][j] == 8)
					{
						usingThisTexture = lightcri;
					}
					else if (g_shc[i][j] == 9)
					{
						usingThisTexture = lightbri;
					}
					else if (g_shc[i][j] == 10)
					{
						usingThisTexture = lightari;
					}
					else if (g_shc[i][j] == 11)
					{
						usingThisTexture = lightdri;
					}
				}

				// render the lightspot
				SDL_Rect dstrect = {(int)(i * ((widthOfIlluminateMe) / fogcookies.size())),
														(int)(j * ((heightOfIlluminateMe) / fogcookies[0].size())),
														(int)((widthOfIlluminateMe + paddingx) / fogcookies.size()),
														(int)((heightOfIlluminateMe + paddingy) / fogcookies[0].size())};

				if (g_graphicsquality != 0)
				{
					SDL_SetTextureAlphaMod(usingThisTexture, fogcookies[i][j]);
				}

				SDL_RenderCopy(renderer, usingThisTexture, NULL, &dstrect); //no error
			}
		}
	}

	// illuminateMe is properly lit
	SDL_SetRenderTarget(renderer, NULL);
	return illuminateMe;
}

SDL_Texture *IlluminateTexture(SDL_Renderer *renderer, SDL_Texture *&mask, SDL_Texture *&diffuse, SDL_Texture *&result)
{
	SDL_SetRenderTarget(renderer, result);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, diffuse, NULL, NULL);
	SDL_RenderCopy(renderer, mask, NULL, NULL);
	SDL_SetRenderTarget(renderer, NULL);
	return result;
}

