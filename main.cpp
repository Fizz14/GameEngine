#include <iostream>
#include <vector>
#include <SDL2/SDL.h>        
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdlib.h>
#include "globals.cpp"
#include "objects.cpp"
#include "map_editor.cpp"


using namespace std;



// int compare_ent (entity* a, entity* b) {
//   return a->y+ a->z  + a->sortingOffset < b->y +b->z + b->sortingOffset;
// }


int compare_ent (actor* a, actor* b) {
  	return a->getOriginY() + a->z < b->getOriginY() +b->z;
}

void sort_by_y(vector<actor*> &g_entities) {
    stable_sort(g_entities.begin(), g_entities.end(), compare_ent);
}

void getInput(float& elapsed);
 

int main(int argc, char ** argv) {
	//load first arg into variable devmode
	if(argc > 1) {
		devMode = (argv[1][0] == '1');
	}

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	IMG_Init(IMG_INIT_PNG);
	TTF_Init();
	
	window = SDL_CreateWindow("Carbin",
	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE /*| SDL_WINDOW_ALWAYS_ON_TOP*/);
	renderer = SDL_CreateRenderer(window, -1,  SDL_RENDERER_ACCELERATED);

	Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048);
	SDL_RenderSetIntegerScale(renderer, SDL_FALSE);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best"); 
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_GL_SetSwapInterval(g_vsync);


	chaser* freller = new chaser(renderer, "protag");
	freller->inParty= 1;
	freller->footstep = Mix_LoadWAV("sounds/protag-step-1.wav");
	freller->footstep2 = Mix_LoadWAV("sounds/protag-step-2.wav");

	protag = party[0];
	g_camera.target = protag;
	
	SDL_RenderSetScale(renderer, scalex, scaley);
	//SDL_RenderSetLogicalSize(renderer, 1920, 1080);

	//for transition
	SDL_Surface* transitionSurface = IMG_Load("tiles/engine/transition.png");

	int transitionImageWidth = transitionSurface->w;
	int transitionImageHeight = transitionSurface->h;

	SDL_Texture* transitionTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, transitionSurface->w, transitionSurface->h );
	SDL_SetTextureBlendMode(transitionTexture, SDL_BLENDMODE_BLEND);

	void* transitionPixelReference;
	int transitionPitch;

	float transitionDelta = transitionImageHeight;

	//setup UI
	adventureUIManager = new adventureUI(renderer);
	adventureUIManager->protagref = protag;
	
	if(devMode) {
		init_map_writing(renderer);
		//done once, because textboxes aren't cleared during clear_map()
		nodeInfoText = new textbox(renderer, "Info", g_fontsize * WIN_WIDTH, 0, 0, WIN_WIDTH);
		config = "edit";
	}
	//set bindings from file
	ifstream bindfile;
	bindfile.open("user/configs/" + config + ".cfg");
	string line;
	for (int i = 0; i < 13; i++) {
		getline(bindfile, line);
		bindings[i] = SDL_GetScancodeFromName(line.c_str());
		D(bindings[i]);
	}
	bindfile.close();
	
	//initialize collision matrix z
	int top_layer = 12;
	for (int i = 0; i < top_layer; i++) {
		vector<collision*> v = {};
		g_collisions.push_back(v);
	}

	load_map(renderer, "maps/empty/empty.map", "a");
	srand (time(NULL));

	bool storedJump = 0; //store the input from a jump if the player is off the ground, quake-style

	while (!quit) {
		ticks = SDL_GetTicks();
		elapsed = ticks - lastticks;
		
		//lock framerate
		if(g_vsync && elapsed < g_min_frametime) {
			SDL_Delay(g_min_frametime - elapsed);
			ticks = SDL_GetTicks();
			elapsed = ticks - lastticks;	
		}
		lastticks = ticks;

		//cooldownsA
		halfsecondtimer+=elapsed;
		attack_cooldown -= elapsed / 1000;
		musicFadeTimer += elapsed;
		musicUpdateTimer += elapsed;

		// INPUT
		getInput(elapsed);


		//rearrange party
		if(input[9] && !oldinput[9]) {
			g_camera.lag = 4;
			g_camera.oldx = g_camera.x;
			g_camera.oldy = g_camera.y;
			g_camera.lagResetTimer = 200;
			std::rotate(party.begin(), party.begin()+1, party.end());
			protag = party[0];
			cout << party.size() << endl;
			for(auto x : party) {
				cout << x << " " << x->name << endl;
			}
			protag->friction = 0.2;
		}
		
		//jump
		if(input[8] && !oldinput[8] && protag->grounded && protag_can_move || input[8] && storedJump && protag->grounded && protag_can_move) {
			protag->zaccel = 350;
			storedJump = 0;
		} else { 
			if(input[8] && !oldinput[8] && !protag->grounded) {
				storedJump = 1;
			}
		}
		


		// ENTITY MOVEMENT
		//dont update movement while transitioning
		for(long long unsigned int i=0; i < g_entities.size(); i++){
			door* taken = g_entities[i]->update_movement(g_collisions[g_entities[i]->layer], g_doors, elapsed);
			if(taken != nullptr) {
				//player took this door
				//clear level
				
				//we will now clear the map, so we will save the door's destination map as a string
				const string savemap = "maps/" + taken->to_map + ".map";
				const string dest_waypoint = taken->to_point;

				//render this frame

				clear_map(g_camera);
				load_map(renderer, savemap, dest_waypoint);
				

				//clear_map() will also delete engine tiles, so let's re-load them (but only if the user is map-editing)
				if(devMode) { init_map_writing(renderer);}
				


				break;
			}
		}


		//update weapons
		for(auto n : g_weapons) {
			n->cooldown -= elapsed;
		}

		//update projectiles
		for(auto n : g_projectiles) {
			n->update(elapsed);
		}
		//delete projectiles with expired lifetimes
		for(int i = 0; i < g_projectiles.size(); i ++) {
			if(g_projectiles[i]->lifetime <= 0) {
				delete g_projectiles[i];
				i--;
			}
		}

		//triggers
		for (int i = 0; i < g_triggers.size(); i++) {
			if(!g_triggers[i]->active) {continue;}
			rect trigger = {g_triggers[i]->x, g_triggers[i]->y, g_triggers[i]->width, g_triggers[i]->height};
			rect movedbounds = rect(protag->x, protag->y - protag->bounds.height, protag->bounds.width, protag->bounds.height);
			if(RectOverlap(movedbounds, trigger)) {
				adventureUIManager->blip = NULL; //possibly narrarators voice
				adventureUIManager->sayings = &g_triggers[i]->script;
				adventureUIManager->talker = protag;
				protag->dialogue_index = -1;
				protag->sayings = g_triggers[i]->script;
				adventureUIManager->continueDialogue();
				g_triggers[i]->active = 0;
			}	
		}

		//listeners
		for (int i = 0; i < g_listeners.size(); i++) {
			if(g_listeners[i]->update()) {
				adventureUIManager->blip = NULL;
				adventureUIManager->sayings = &g_listeners[i]->script;
				adventureUIManager->talker = protag;
				protag->dialogue_index = -1;
				protag->sayings = g_listeners[i]->script;
				adventureUIManager->continueDialogue();
				g_listeners[i]->active = 0;
			}	
		}
		
			

		//update camera
		SDL_GetWindowSize(window, &WIN_WIDTH, &WIN_HEIGHT);

		//detect change in window
		if(old_WIN_WIDTH != WIN_WIDTH) {
			//user scaled window
			
			scalex *= WIN_WIDTH / old_WIN_WIDTH;
			if(scalex < min_scale) {
				scalex = min_scale;
			}
			if(scalex > max_scale) {
				scalex = max_scale;
			}
			scaley = scalex;
			SDL_RenderSetScale(renderer, scalex, scaley);
			SDL_GetWindowSize(window, &WIN_WIDTH, &WIN_HEIGHT);
		}
		
		old_WIN_WIDTH = WIN_WIDTH;

		WIN_WIDTH /= scalex;
		WIN_HEIGHT /= scaley;
		
		g_camera.width = WIN_WIDTH;
		g_camera.height = WIN_HEIGHT;
		
		

		if(freecamera) {
			g_camera.update_movement(elapsed, camx, camy);
		} else {
			g_camera.update_movement(elapsed);
		}
		//update ui
		curTextWait += elapsed * text_speed_up;
		if(curTextWait >= textWait) {
			adventureUIManager->updateText();
			curTextWait = 0;
		}
		
		//background
		SDL_RenderClear(renderer);
		
		//update AI
		// for (long long unsigned int i = 0; i < g_ais.size(); i++) {
		// 	g_ais[i]->update(protag, elapsed);
		// }

		//update party
		for (int i = 1; i < party.size(); i++) {
			party[i]->update(party[i-1], elapsed);
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

		//reset player glimmer (shadow thrue walls when covered) for this frame
		protagGlimmerA = 0;
		protagGlimmerB = 0;
		protagGlimmerC = 0;
		protagGlimmerD = 0;

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
		//if drawProtagGlimmer was set to 1, lets draw the glimmer on top of everything
		if(protagGlimmerA && protagGlimmerB && protagGlimmerC && protagGlimmerD && 0) {
			SDL_SetTextureColorMod(protag->texture, 0,0,0);
			SDL_SetTextureAlphaMod(protag->texture, 50);
			//redraw player ontop of everything
			protag->sortingOffset = 1000000;
			protag->render(renderer, g_camera);
		} else {
			protag->sortingOffset = 0;
			SDL_SetTextureColorMod(protag->texture, 255, 255, 255);
			SDL_SetTextureAlphaMod(protag->texture, 255);
		}

		
		//map editing
		if(devMode) {
				
			nodeInfoText->textcolor = {0, 0, 0};

			//draw nodes
			for (int i = 0; i < g_worldsounds.size(); i++) {
				SDL_Rect obj = {(g_worldsounds[i]->x -g_camera.x - 20)* g_camera.zoom , ((g_worldsounds[i]->y - g_camera.y - 20) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
				SDL_RenderCopy(renderer, worldsoundIcon->texture, NULL, &obj);
				nodeInfoText->x = obj.x;
				nodeInfoText->y = obj.y - 20;
				nodeInfoText->updateText(g_worldsounds[i]->name, 15, 15);
				nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}

			for (int i = 0; i < g_musicNodes.size(); i++) {
				SDL_Rect obj = {(g_musicNodes[i]->x -g_camera.x - 20)* g_camera.zoom , ((g_musicNodes[i]->y - g_camera.y - 20) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
				SDL_RenderCopy(renderer, musicIcon->texture, NULL, &obj);
				nodeInfoText->x = obj.x;
				nodeInfoText->y = obj.y - 20;
				nodeInfoText->updateText(g_musicNodes[i]->name, 15, 15);
				nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}

			for (int i = 0; i < g_cueSounds.size(); i++) {
				SDL_Rect obj = {(g_cueSounds[i]->x -g_camera.x - 20)* g_camera.zoom , ((g_cueSounds[i]->y - g_camera.y - 20) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
				SDL_RenderCopy(renderer, cueIcon->texture, NULL, &obj);
				nodeInfoText->x = obj.x;
				nodeInfoText->y = obj.y - 20;
				nodeInfoText->updateText(g_cueSounds[i]->name, 15, 15);
				nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}

			for (int i = 0; i < g_waypoints.size(); i++) {
				SDL_Rect obj = {(g_waypoints[i]->x -g_camera.x - 20)* g_camera.zoom , ((g_waypoints[i]->y - g_camera.y - 20) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
				SDL_RenderCopy(renderer, waypointIcon->texture, NULL, &obj);
				nodeInfoText->x = obj.x;
				nodeInfoText->y = obj.y - 20;
				nodeInfoText->updateText(g_waypoints[i]->name, 15, 15);
				nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}

			//doors
			for (int i = 0; i < g_doors.size(); i++) {
				SDL_Rect obj = {(g_doors[i]->x -g_camera.x)* g_camera.zoom , ((g_doors[i]->y - g_camera.y) * g_camera.zoom), (g_doors[i]->width * g_camera.zoom), (g_doors[i]->height * g_camera.zoom)};
				SDL_RenderCopy(renderer, doorIcon->texture, NULL, &obj);
				nodeInfoText->x = obj.x + 25;
				nodeInfoText->y = obj.y + 25;
				nodeInfoText->updateText(g_doors[i]->to_map + "->" + g_doors[i]->to_point, 15, 15);
				nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}

			for (int i = 0; i < g_triggers.size(); i++) {
				SDL_Rect obj = {(g_triggers[i]->x -g_camera.x)* g_camera.zoom , ((g_triggers[i]->y - g_camera.y) * g_camera.zoom), (g_triggers[i]->width * g_camera.zoom), (g_triggers[i]->height * g_camera.zoom)};
				SDL_RenderCopy(renderer, triggerIcon->texture, NULL, &obj);
				nodeInfoText->x = obj.x + 25;
				nodeInfoText->y = obj.y + 25;
				nodeInfoText->updateText(g_triggers[i]->binding, 15, 15);
				nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}

			//listeners
			for (int i = 0; i < g_listeners.size(); i++) {
				SDL_Rect obj = {(g_listeners[i]->x -g_camera.x - 20)* g_camera.zoom , ((g_listeners[i]->y - g_camera.y - 20) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
				SDL_RenderCopy(renderer, listenerIcon->texture, NULL, &obj);
				nodeInfoText->x = obj.x;
				nodeInfoText->y = obj.y - 20;
				nodeInfoText->updateText(g_listeners[i]->listenList.size() + " of " + g_listeners[i]->entityName, 15, 15);
				nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}

			write_map(protag);
			for(int i =0; i < 50; i++) {
				devinput[i]=0;
			}

		}
		
		//ui
		for(long long unsigned int i=0; i < g_ui.size(); i++){
			
			g_ui[i]->render(renderer, g_camera);
			
		}	
		for(long long unsigned int i=0; i < g_textboxes.size(); i++){
			g_textboxes[i]->render(renderer, WIN_WIDTH, WIN_HEIGHT);
		}	
		

		//worldsounds
		for (int i = 0; i < g_worldsounds.size(); i++) {
			g_worldsounds[i]->update(elapsed);
			
			
		}
		
		//transition
		{
			if (transition) {
				
				//onframe things
				SDL_LockTexture(transitionTexture, NULL, &transitionPixelReference, &transitionPitch);
				
				memcpy( transitionPixelReference, transitionSurface->pixels, transitionSurface->pitch * transitionSurface->h);
				Uint32 format = SDL_PIXELFORMAT_ARGB8888;
				SDL_PixelFormat* mappingFormat = SDL_AllocFormat( format );
				Uint32* pixels = (Uint32*)transitionPixelReference;
				int numPixels = transitionImageWidth * transitionImageHeight;
				Uint32 transparent = SDL_MapRGBA( mappingFormat, 0, 0, 0, 255 );
				// Uint32 halftone = SDL_MapRGBA( mappingFormat, 50, 50, 50, 128);
				transitionDelta += g_transitionSpeed + 0.02 * transitionDelta;
				for(int x = 0;  x < transitionImageWidth; x++) {
					for(int y = 0; y < transitionImageHeight; y++) {
						int dest = (y * transitionImageWidth) + x;
						int src =  (y * transitionImageWidth) + x;
						
						if(pow(pow(transitionImageWidth/2 - x,2) + pow(transitionImageHeight + y,2),0.5) < transitionDelta) {
							pixels[dest] = 0;
							
						} else {
							// if(pow(pow(transitionImageWidth/2 - x,2) + pow(transitionImageHeight + y,2),0.5) < 10 + transitionDelta) {
							// 	pixels[dest] = halftone;
							// } else {
								pixels[dest] = transparent;
							// }
						}
					}
				}

				ticks = SDL_GetTicks();
				elapsed = ticks - lastticks;

				SDL_UnlockTexture(transitionTexture);
				SDL_RenderCopy(renderer, transitionTexture, NULL, NULL);

				if(transitionDelta > transitionImageHeight + pow(pow(transitionImageWidth/2,2) + pow(transitionImageHeight,2),0.5)) {
					transition = 0;
					
				}
			} else {
				transitionDelta = transitionImageHeight;
			}
        
   	 	}

		SDL_RenderPresent(renderer);
		
		//update music
		if(musicUpdateTimer > 500) {
			musicUpdateTimer = 0;
			if(g_musicNodes.size() > 0) {
				newClosest = protag->Get_Closest_Node(g_musicNodes);
				if(closestMusicNode == nullptr && !g_mute) { 
					Mix_PlayMusic(newClosest->blip, -1); 
					closestMusicNode = newClosest; 
				} else { 

					//Segfaults, todo is initialize these musicNodes to have something
					if(newClosest->name != closestMusicNode->name) {
						//D(newClosest->name);
						if(newClosest->name == "silence") {
							Mix_FadeOutMusic(1000);
						}
						closestMusicNode = newClosest;
						//change music
						Mix_FadeOutMusic(1000);
						musicFadeTimer = 0;
						fadeFlag = 1;
						
					}
				}
			}
			//check for any cues
			for (auto x : g_cueSounds) {
				if(x->played == 0 && Distance(x->x, x->y, protag->x + protag->width/2, protag->y) < x->radius) {
					x->played = 1;
					playSound(-1, x->blip, 0);
				}
			}
			
		}
		if(fadeFlag && musicFadeTimer > 1000) {
			fadeFlag = 0;
			Mix_HaltMusic();
			Mix_FadeInMusic(newClosest->blip, -1, 1000);
			
		}
	}


	clear_map(g_camera);
	delete adventureUIManager;
	close_map_writing();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_FreeSurface(transitionSurface);
	SDL_DestroyTexture(transitionTexture);
	IMG_Quit();
	Mix_CloseAudio();
	TTF_Quit();
	TTF_Quit();
	
	return 0;
}

int interact(float elapsed, entity* protag) {
	SDL_Rect srect;
		switch(protag->animation) {
			
		case 0:
			srect.h = protag->bounds.height;
			srect.w = protag->bounds.width;

			srect.x = protag->getOriginX() - srect.w/2;
			srect.y = protag->getOriginY() - srect.h/2;

			srect.y -= 55;
			
			srect = transformRect(srect);
			if(drawhitboxes) {
				SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
				SDL_RenderFillRect(renderer, &srect);
				SDL_RenderPresent(renderer);	
				SDL_Delay(500);
			}
			break;
		case 1:
			srect.h = protag->bounds.height;
			srect.w = protag->bounds.width;

			srect.x = protag->getOriginX() - srect.w/2;
			srect.y = protag->getOriginY() - srect.h/2;

			srect.y -= 30;
			if(protag->flip == SDL_FLIP_NONE) {
				srect.x -= 30;
			} else {
				srect.x += 30;
			}

			srect = transformRect(srect);
			if(drawhitboxes) {
				SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
				SDL_RenderFillRect(renderer, &srect);
				SDL_RenderPresent(renderer);	
				SDL_Delay(500);
			}
			break;
		case 2:
			srect.h = protag->bounds.height;
			srect.w = protag->bounds.width;

			srect.x = protag->getOriginX() - srect.w/2;
			srect.y = protag->getOriginY() - srect.h/2;

			
			if(protag->flip == SDL_FLIP_NONE) {
				srect.x -= 55;
			} else {
				srect.x += 55;
			}

			srect = transformRect(srect);
			if(drawhitboxes) {
				SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
				SDL_RenderFillRect(renderer, &srect);
				SDL_RenderPresent(renderer);	
				SDL_Delay(500);
			}
			break;
		case 3:
			srect.h = protag->bounds.height;
			srect.w = protag->bounds.width;

			srect.x = protag->getOriginX() - srect.w/2;
			srect.y = protag->getOriginY() - srect.h/2;

			srect.y += 30;
			if(protag->flip == SDL_FLIP_NONE) {
				srect.x -= 30;
			} else {
				srect.x += 30;
			}

			srect = transformRect(srect);
			if(drawhitboxes) {
				SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
				SDL_RenderFillRect(renderer, &srect);
				SDL_RenderPresent(renderer);	
				SDL_Delay(500);
			}
			break;
		case 4:
			srect.h = protag->bounds.height;
			srect.w = protag->bounds.width;

			srect.x = protag->getOriginX() - srect.w/2;
			srect.y = protag->getOriginY() - srect.h/2;

			srect.y += 55;

			srect = transformRect(srect);
			if(drawhitboxes) {
				SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
				SDL_RenderFillRect(renderer, &srect);
				SDL_RenderPresent(renderer);	
				SDL_Delay(500);
			}
			break;
		}


		for (int i = 0; i < g_entities.size(); i++) {

			SDL_Rect hisrect = {g_entities[i]->x+ g_entities[i]->bounds.x, g_entities[i]->y + g_entities[i]->bounds.y, g_entities[i]->bounds.width, g_entities[i]->bounds.height};				
			hisrect = transformRect(hisrect);
			
			if(g_entities[i]->talks && RectOverlap(hisrect, srect)) {
				g_entities[i]->animate = 1;
				//make ent look at player
				
				int xdiff = (g_entities[i]->getOriginX()) - (protag->getOriginX());
				int ydiff = (g_entities[i]->getOriginY()) - (protag->getOriginY());
				int axdiff = ( abs(xdiff) - abs(ydiff) );
				if(axdiff > 0) {
					//xaxis is more important
					g_entities[i]->animation = 2;
					if(xdiff > 0) {
						g_entities[i]->flip = SDL_FLIP_NONE;
					} else {
						g_entities[i]->flip = SDL_FLIP_HORIZONTAL;
					}
				} else {
					//yaxis is more important
					g_entities[i]->flip = SDL_FLIP_NONE;
					M("check yaxis");
					if(ydiff > 0) {
						g_entities[i]->animation = 0;
					} else {
						g_entities[i]->animation = 4;
					}
				}
				if(abs(axdiff) < 45) {
					if(xdiff > 0) {
						g_entities[i]->flip = SDL_FLIP_NONE;
					} else {
						g_entities[i]->flip = SDL_FLIP_HORIZONTAL;
					}
					if(ydiff > 0) {
						g_entities[i]->animation = 1;
					} else {
						g_entities[i]->animation = 3;
					}
				}
				
				adventureUIManager->blip = g_entities[i]->voice;
				adventureUIManager->sayings = &g_entities[i]->sayings;
				adventureUIManager->talker = g_entities[i];
				adventureUIManager->talker->dialogue_index = -1;
				adventureUIManager->continueDialogue();
				protag_is_talking = 1;
				break;
			}
			//no one to talk to, lets do an attack instead
			if(attack_cooldown <= 0) {
				//blenderblade attack
				//M( "pow pow!");
				//attack_cooldown = AdventureWeaponSet[0]->maxCooldown;
			}

		}
	return 0;
}

void getInput(float &elapsed) {
	for (int i = 0; i < 16; i++) {
		oldinput[i] = input[i];
	}

	SDL_PollEvent(&event);

	if(keystate[SDL_SCANCODE_W]) {
		camy-= 4;
	}
	if(keystate[SDL_SCANCODE_A]) {
		camx-=4;
	}
	if(keystate[SDL_SCANCODE_S]) {
		camy+=4;
	}
	if(keystate[SDL_SCANCODE_D]) {
		camx+=4;
	}
	
	protag_can_move = !protag_is_talking;
	if(protag_can_move) {
		protag->shooting = 0;
		protag->left = 0;
		protag->right = 0;
		protag->up = 0;
		protag->down = 0;
		if(keystate[bindings[4]]) {
			protag->shoot_up();
		}
		if(keystate[bindings[5]]) {
			protag->shoot_down();
		}
		if(keystate[bindings[6]]) {
			protag->shoot_left();
		}
		if(keystate[bindings[7]]) {
			protag->shoot_right();
		}
		if(keystate[bindings[0]]) {
			protag->move_up();
		}
		if(keystate[bindings[1]]) {
			protag->move_down();
		}
		if(keystate[bindings[2]]) {
			protag->move_left();
		}
		if(keystate[bindings[3]]) {
			protag->move_right();
		}
		
	} else {
		if(keystate[bindings[6]] && !left_ui_refresh) {
			if(adventureUIManager->askingQuestion) {
				adventureUIManager->response_index--;
				if(adventureUIManager->response_index < 0) {
					adventureUIManager->response_index++;
				}
			}
			left_ui_refresh = 1;
		} else if(!keystate[bindings[6]]){ left_ui_refresh = 0;}
		if(keystate[bindings[7]] && !right_ui_refresh) {
			if(adventureUIManager->askingQuestion) {
				adventureUIManager->response_index++;
				if(adventureUIManager->response_index > adventureUIManager->responses.size() - 1) {
					adventureUIManager->response_index--;
				}
			}
			right_ui_refresh = 1;
		} else if(!keystate[bindings[7]]) { right_ui_refresh = 0;}
		protag->stop_hori();
		protag->stop_verti();
	}

	if(keystate[bindings[8]]) {
		input[8] = 1;
	} else {
		input[8] = 0;
	}

	if(keystate[bindings[9]]) {
		input[9] = 1;
	} else {
		input[9] = 0;
	}

	if(keystate[bindings[11]] && !old_z_value) {
		if(protag_is_talking == 1) {
			if(!adventureUIManager->typing) {
				adventureUIManager->continueDialogue();
			}
		}	
	}
	dialogue_cooldown -= elapsed;
	if(keystate[bindings[11]]) {
		
		if(protag_is_talking == 1) {
			//advance or speedup diaglogue
			text_speed_up = 50;
			
		}	
		if(protag_is_talking == 0) {
			if(dialogue_cooldown < 0) {
				interact(elapsed, protag);
			}
		}
		old_z_value = 1;
	} else {
		
		//reset text_speed_up
		text_speed_up = 1;
		old_z_value = 0;
	}

	if(protag_is_talking == 2) {
		protag_is_talking = 0;
		dialogue_cooldown = 500;
	}
	
	//map editing keys
	//z = set protag spawn
	//x = spawn entity
	//c = set tile left corner
	//v = set tile right corner
	//b = set collision left corner
	//n = set collision right corner
	if(keystate[SDL_SCANCODE_LSHIFT] && devMode) {
		protag->xmaxspeed = 250;
		protag->ymaxspeed = 250;
	}
	if(keystate[SDL_SCANCODE_TAB] && devMode) {
		protag->xmaxspeed = 20;
		protag->ymaxspeed = 20;
	}
	if(keystate[SDL_SCANCODE_CAPSLOCK] && devMode) {
		protag->xmaxspeed = 140;
		protag->ymaxspeed = 140;
	}
	
	
	if(keystate[SDL_SCANCODE_SLASH] && devMode) {
			devinput[0] = 1;
	}
	if(keystate[SDL_SCANCODE_X] && devMode) {
		devinput[1] = 1;
	}
	if(keystate[SDL_SCANCODE_C] && devMode) {
		devinput[2] = 1;
	}
	if(keystate[SDL_SCANCODE_V] && devMode) {
		devinput[3] = 1;
	}
	if(keystate[SDL_SCANCODE_B] && devMode) {
		devinput[4] = 1;
	}
	if(keystate[SDL_SCANCODE_N] && devMode) {
		devinput[5] = 1;
		
	}
	if(keystate[SDL_SCANCODE_M] && devMode) {
		devinput[6] = 1;
		
	}
	if(keystate[SDL_SCANCODE_KP_DIVIDE] && devMode) {
		//decrease gridsize
		devinput[7] = 1;
	}
	if(keystate[SDL_SCANCODE_KP_MULTIPLY] && devMode) {
		//increase gridsize
		devinput[8] = 1;
	}
	if(keystate[SDL_SCANCODE_KP_1] && devMode) {
		//enable/disable collision
		devinput[9] = 1;
	}
	if(keystate[SDL_SCANCODE_KP_2] && devMode) {
		//visualize collisions
		devinput[10] = 1;
	}

	if(keystate[SDL_SCANCODE_RETURN] && devMode) {
		//stop player first
		protag->stop_hori();
		protag->stop_verti();
		
		elapsed = 0;
		//pull up console
		devinput[11] = 1;
	}
	//for diagonal walls
	if(keystate[SDL_SCANCODE_KP_7] && devMode) {
		devinput[12] = 1;
	}
	if(keystate[SDL_SCANCODE_KP_8] && devMode) {
		devinput[13] = 1;
	}
	if(keystate[SDL_SCANCODE_KP_5] && devMode) {
		devinput[14] = 1;
	}
	if(keystate[SDL_SCANCODE_KP_4] && devMode) {
		devinput[15] = 1;
	}

	//make zslant left
	if(keystate[SDL_SCANCODE_KP_9]&& devMode) {
		devinput[17] = 1;
	}
	
	//make zslant right
	if(keystate[SDL_SCANCODE_KP_6]) {
		devinput[18] = 1;
	}

	if(keystate[SDL_SCANCODE_PERIOD] && devMode) {
		//make navnode
		devinput[19] = 1;
	}

	if(keystate[SDL_SCANCODE_COMMA] && devMode) {
		//make navnode box
		devinput[20] = 1;
	}

	if(keystate[SDL_SCANCODE_ESCAPE]) {
		quit = 1;
	}
	if(keystate[SDL_SCANCODE_Q]) {
		
		scalex-= 0.001 * elapsed;
		
		if(scalex < min_scale) {
			scalex = min_scale;
		}
		if(scalex > max_scale) {
			scalex = max_scale;
		}
		scaley = scalex;
		SDL_RenderSetScale(renderer, scalex, scaley);
	}
	
	if(keystate[SDL_SCANCODE_E]) {
		scalex+= 0.001 * elapsed;
		
		if(scalex < min_scale) {
			scalex = min_scale;
		}
		if(scalex > max_scale) {
			scalex = max_scale;
		}
		scaley = scalex;
		SDL_RenderSetScale(renderer, scalex, scaley);
		
	}
	if(keystate[SDL_SCANCODE_BACKSPACE]) {
		devinput[16] = 1;
	}

	if(keystate[bindings[0]] == keystate[bindings[1]]) {
		protag->stop_verti();
	}

	if(keystate[bindings[2]] == keystate[bindings[3]]) {
		protag->stop_hori();
	}
	if(keystate[SDL_SCANCODE_F] && !devMode) {
		fullscreen = !fullscreen;
		if(fullscreen) {
			SDL_SetWindowFullscreen(window, 0);	
		} else {
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
		}
	}

	
	

	//window control
	while( SDL_PollEvent( &event ) ){
		switch(event.type) {
			case SDL_QUIT:
				quit = 1;
				break;
		}
	}
}