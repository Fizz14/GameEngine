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

// int compare_ent (actor* a, actor* b) {
//   	return a->getOriginY() + a->z < b->getOriginY() +b->z;
// }

int compare_ent (actor* a, actor* b) {
  	return a->y + a->z + a->sortingOffset < b->y + b->z + b->sortingOffset;
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
	if(argc > 2) {
		genericmode = (argv[2][0] == '1');
	}

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	IMG_Init(IMG_INIT_PNG);
	TTF_Init();
	
	window = SDL_CreateWindow("Carbin",
	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE /*| SDL_WINDOW_ALWAYS_ON_TOP*/);
	renderer = SDL_CreateRenderer(window, -1,  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048);
	SDL_RenderSetIntegerScale(renderer, SDL_FALSE);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "3"); 
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	
	SDL_RenderSetScale(renderer, scalex, scaley);
	//SDL_RenderSetLogicalSize(renderer, 1920, 1080); //for enforcing screen resolution

	//set global shadow-texture

	SDL_Surface* image = IMG_Load("textures/sprites/shadow.png");
	g_shadowTexture = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);


	entity* fomm;
	if(devMode) {
		//fomm = new entity(renderer, "fommconstruction"); 
		fomm = new entity(renderer, "fomm"); 
	} else {
		fomm = new entity(renderer, "fomm"); 
	}
	
	
	fomm->inParty = 1;
	party.push_back(fomm);
	fomm->footstep = Mix_LoadWAV("sounds/protag-step-1.wav");
	fomm->footstep2 = Mix_LoadWAV("sounds/protag-step-2.wav");
	protag = fomm;
	g_focus = protag;
	
	g_deathsound = Mix_LoadWAV("audio/sounds/game-over.wav");
	

	//for transition
	SDL_Surface* transitionSurface = IMG_Load("textures/engine/transition.png");

	int transitionImageWidth = transitionSurface->w;
	int transitionImageHeight = transitionSurface->h;

	SDL_Texture* transitionTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, transitionSurface->w, transitionSurface->h );
	SDL_SetTextureBlendMode(transitionTexture, SDL_BLENDMODE_BLEND);

	void* transitionPixelReference;
	int transitionPitch;

	float transitionDelta = transitionImageHeight;

	//font
	//g_font = (genericmode) ? "fonts/OpenSans-Regular.ttf" : "fonts/ShortStack-Regular.ttf";
	//g_font = "fonts/SecularOne-Regular.ttf";
	//g_font = "fonts/Itim-Regular.ttf";
	//g_font = "fonts/SawarabiMincho-Regular.ttf";
	//g_font = "fonts/Recursive_Monospace-SemiBold.ttf";
	g_font = "fonts/Monda-Bold.ttf";

	//setup UI
	adventureUIManager = new adventureUI(renderer);
	
	if(devMode) {
		init_map_writing(renderer);
		//done once, because textboxes aren't cleared during clear_map()
		nodeInfoText = new textbox(renderer, "", g_fontsize * WIN_WIDTH, 0, 0, WIN_WIDTH);
		config = "edit";
		nodeDebug = SDL_CreateTextureFromSurface(renderer, IMG_Load("textures/engine/walker.png"));
		
	}
	//set bindings from file
	ifstream bindfile;
	bindfile.open("user/configs/" + config + ".cfg");
	string line;
	for (int i = 0; i < 13; i++) {
		getline(bindfile, line);
		bindings[i] = SDL_GetScancodeFromName(line.c_str());
		//D(bindings[i]);
	}
	//set vsync and g_fullscreen from config
	string valuestr; int value;
	
	//get vsync
	getline(bindfile, line);
	valuestr = line.substr(line.find(' '), line.length());
	value = stoi(valuestr);
	g_vsync = value;
	
	//get g_fullscreen
	getline(bindfile, line);
	valuestr = line.substr(line.find(' '), line.length());
	value = stoi(valuestr);
	g_fullscreen = value;



	bindfile.close();
	
	//apply vsync
	//SDL_GL_SetSwapInterval(g_vsync);

	//apply fullscreen
	if(g_fullscreen) {
		SDL_GetCurrentDisplayMode(0, &DM);
		SDL_SetWindowSize(window, DM.w, DM.h);
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
	} else {
		SDL_SetWindowFullscreen(window, 0);
		
	}

	//initialize box matrix z
	for (int i = 0; i < g_layers; i++) {
		vector<box*> v = {};
		g_boxs.push_back(v);
	}

	for (int i = 0; i < g_layers; i++) {
		vector<tri*> v = {};
		g_triangles.push_back(v);
	}

	srand(time(NULL));
	if(devMode) {
		//!!!
		loadSave();
		//empty map or default map for map editing, perhaps a tutorial even
		load_map(renderer, "maps/editordefault/editordefault.map","a");
		g_map = "g";
		// protag->x = 100000;
		// protag->y = 100000;
		
		
	} else {
		//load the titlescreen
		load_map(renderer, "maps/" + g_map + "/" + g_map + ".map", "a");
		//srand(time(NULL));
	}

	ui* inventoryMarker = new ui(renderer, "textures/ui/non_selector.png", 0, 0, 0.15, 0.15, 2);
	inventoryMarker->show = 0;
	inventoryMarker->persistent = 1;

	textbox* inventoryText = new textbox(renderer, "1", 40, WIN_WIDTH * 0.8, 0, WIN_WIDTH * 0.2);
	inventoryText->show = 0;
	inventoryText->align = 1;

	bool storedJump = 0; //buffer the input from a jump if the player is off the ground, quake-style
	
	//software lifecycle text
	//new textbox(renderer, g_lifecycle.c_str(), 40,WIN_WIDTH * 0.8,0, WIN_WIDTH * 0.2);
	while (!quit) {
		//some event handling
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
					switch( event.key.keysym.sym ){
                    	case SDLK_TAB:
							g_holdingCTRL = 1;
							//protag->getItem(a, 1);
							break;
					}
					break;
				case SDL_KEYUP:
					switch( event.key.keysym.sym ){
                    	case SDLK_TAB:
							g_holdingCTRL = 0;
							break;
					}
					break;
				case SDL_MOUSEWHEEL:
				if(g_holdingCTRL) {
					if(event.wheel.y > 0) {
						wallstart -= 64;
					}
					else if(event.wheel.y < 0) {
						wallstart += 64;
					}
					if(wallstart < 0) {
						wallstart = 0;
					} else {
						if(wallstart > 64 * g_layers) {
							wallstart = 64 * g_layers;
						}
						if(wallstart > wallheight - 64) {
							wallstart = wallheight - 64;
						}
					}
				} else {
					if(event.wheel.y > 0) {
						wallheight -= 64;
					}
					else if(event.wheel.y < 0) {
						wallheight += 64;
					}
					if(wallheight < wallstart + 64) {
						wallheight = wallstart + 64;
					} else {
						if(wallheight > 64 * g_layers) {
							wallheight = 64 * g_layers;
						}
					}
					break;
				}
				case SDL_MOUSEBUTTONDOWN:
					if(event.button.button == SDL_BUTTON_LEFT){
						devinput[3] = 1;
					}
					if(event.button.button == SDL_BUTTON_MIDDLE){
						devinput[10] = 1;
					}
					if(event.button.button == SDL_BUTTON_RIGHT){
						devinput[4] = 1;
					}
					break;
				
				case SDL_QUIT:
					quit = 1;
					break;
			}
		}	

		ticks = SDL_GetTicks();
		elapsed = ticks - lastticks;
		lastticks = ticks;

		//cooldowns
		halfsecondtimer+=elapsed;
		use_cooldown -= elapsed / 1000;
		musicFadeTimer += elapsed;
		musicUpdateTimer += elapsed;

		
		if(inPauseMenu) {
			//if we're paused, freeze gametime
			elapsed = 0;
		}

		//INPUT
		getInput(elapsed);

		//spring
		if(input[8] && !oldinput[8] && protag->grounded && protag_can_move || input[8] && storedJump && protag->grounded && protag_can_move) {
			protag->zaccel = 180;
			storedJump = 0;
		} else { 
			if(input[8] && !oldinput[8] && !protag->grounded) {
				storedJump = 1;
			}
		}
		//if we die don't worry about not being able to switch because we can't shoot yet
		if(protag->hp <= 0) {protag->cooldown = 0;}
		//cycle right if the current character dies
		if( (input[9] && !oldinput[9]) || protag->hp <= 0) {
			//keep switching if we switch to a dead partymember
			int i = 0;
			
			if(party.size() > 1 && protag->cooldown <= 0) {
				do {
					M("Cycle party right");
					std::rotate(party.begin(),party.begin()+1,party.end());
					protag->tangible = 0;
					party[0]->tangible = 1;
					party[0]->x = protag->getOriginX() - party[0]->bounds.x - party[0]->bounds.width/2;
					party[0]->y = protag->getOriginY() - party[0]->bounds.y - party[0]->bounds.height/2;
					party[0]->z = protag->z;
					party[0]->xvel = protag->xvel;
					party[0]->yvel = protag->yvel;
					party[0]->zvel = protag->zvel;
					
					party[0]->animation = protag->animation;
					party[0]->flip = protag->flip;
					protag->zvel = 0;
					protag->xvel = 0;
					protag->yvel = 0;
					protag=party[0];
					protag->shadow->x = protag->x + protag->shadow->xoffset;
					protag->shadow->y = protag->y + protag->shadow->yoffset;
					g_focus = protag;
					//prevent infinite loop
					i++;
					if(i > 600) {M("Avoided infinite loop: no living partymembers yet no essential death. (Did the player's party contain at least one essential character?)"); break; quit = 1;}
				}while(protag->hp <= 0);
			}
		}
		//party swap
		if(input[10] && !oldinput[10]) {
			if(party.size() > 1 && protag->cooldown <= 0) {
				int i = 0;
				do {
					M("Cycle party left");
					std::rotate(party.begin(),party.begin()+party.size()-1,party.end());
					protag->tangible = 0;
					party[0]->tangible = 1;
					party[0]->x = protag->getOriginX() - party[0]->bounds.x - party[0]->bounds.width/2;
					party[0]->y = protag->getOriginY() - party[0]->bounds.y - party[0]->bounds.height/2;
					party[0]->z = protag->z;
					party[0]->xvel = protag->xvel;
					party[0]->yvel = protag->yvel;
					party[0]->zvel = protag->zvel;
					
					party[0]->animation = protag->animation;
					party[0]->flip = protag->flip;
					protag->zvel = 0;
					protag->xvel = 0;
					protag->yvel = 0;
					protag=party[0];
					protag->shadow->x = protag->x + protag->shadow->xoffset;
					protag->shadow->y = protag->y + protag->shadow->yoffset;
					g_focus = protag;
					i++;
					if(i > 600) {M("Avoided infinite loop: no living partymembers yet no essential death. (Did the player's party contain at least one essential character?)"); break; quit = 1;}
				} while(protag->hp <= 0);
			}
		}

		//background
		SDL_RenderClear(renderer);
		if(g_backgroundLoaded && g_useBackgrounds) { //if the level has a background and the user would like to see it
			SDL_RenderCopy(renderer, background, NULL, NULL);
		}
		

		
		
		for(auto n : g_entities) {
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
			entity* checkHim = searchEntities(g_triggers[i]->targetEntity);
			if(checkHim == nullptr) {continue;}
			rect movedbounds = rect(checkHim->x, checkHim->y - checkHim->bounds.height, checkHim->bounds.width, checkHim->bounds.height);
			if(RectOverlap(movedbounds, trigger) && (checkHim->z >= g_triggers[i]->z && checkHim->z <= g_triggers[i]->z + g_triggers[i]->zeight)  ) {
				adventureUIManager->blip = g_ui_voice; 
				adventureUIManager->sayings = &g_triggers[i]->script;
				adventureUIManager->talker = checkHim;
				checkHim->dialogue_index = -1;
				checkHim->sayings = g_triggers[i]->script;
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
			scalex = ((float)WIN_WIDTH / old_WIN_WIDTH);
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
		
		//moved this line to the start of the program, since the zoom could get screwed up
		//old_WIN_WIDTH = WIN_WIDTH;

		WIN_WIDTH /= scalex;
		WIN_HEIGHT /= scaley;
		
		g_camera.width = WIN_WIDTH;
		g_camera.height = WIN_HEIGHT;
		
		

		if(freecamera) {
			g_camera.update_movement(elapsed, camx, camy);
		} else {
			g_camera.update_movement(elapsed, (g_focus->getOriginX() - (g_camera.width/(2 * g_camera.zoom))), ((g_focus->getOriginY() - XtoZ * g_focus->z) - (g_camera.height/(2 * g_camera.zoom))) );
		}
		//update ui
		curTextWait += elapsed * text_speed_up;
		if(curTextWait >= textWait) {
			adventureUIManager->updateText();
			curTextWait = 0;
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
		// if(protagGlimmerA && protagGlimmerB && protagGlimmerC && protagGlimmerD && 0) {
		// 	SDL_SetTextureColorMod(protag->texture, 0,0,0);
		// 	SDL_SetTextureAlphaMod(protag->texture, 50);
		// 	//redraw player ontop of everything
		// 	//protag->sortingOffset = 1000000;
		// 	protag->render(renderer, g_camera);
		// } else {
		// 	//protag->sortingOffset = 0;
		// 	SDL_SetTextureColorMod(protag->texture, 255, 255, 255);
		// 	SDL_SetTextureAlphaMod(protag->texture, 255);
		// }

		
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
				SDL_Rect obj = {(g_waypoints[i]->x -g_camera.x - 20)* g_camera.zoom , ((g_waypoints[i]->y - 20 - g_camera.y - g_waypoints[i]->z * XtoZ) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
				SDL_RenderCopy(renderer, waypointIcon->texture, NULL, &obj);
				nodeInfoText->x = obj.x;
				nodeInfoText->y = obj.y - 20;
				nodeInfoText->updateText(g_waypoints[i]->name, 15, 15);
				nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}

			//doors
			for (int i = 0; i < g_doors.size(); i++) {
				SDL_Rect obj = {(g_doors[i]->x -g_camera.x)* g_camera.zoom , ((g_doors[i]->y - g_camera.y - ( g_doors[i]->zeight ) * XtoZ) * g_camera.zoom), (g_doors[i]->width * g_camera.zoom), (g_doors[i]->height * g_camera.zoom)};
				SDL_RenderCopy(renderer, doorIcon->texture, NULL, &obj);
				//the wall
				SDL_Rect obj2 = {(g_doors[i]->x -g_camera.x)* g_camera.zoom, ((g_doors[i]->y - g_camera.y - ( g_doors[i]->zeight ) * XtoZ) * g_camera.zoom), (g_doors[i]->width * g_camera.zoom), ( (g_doors[i]->zeight - g_doors[i]->z) * XtoZ * g_camera.zoom) + (g_doors[i]->height * g_camera.zoom)};
				SDL_RenderCopy(renderer, doorIcon->texture, NULL, &obj2);
				nodeInfoText->x = obj.x + 25;
				nodeInfoText->y = obj.y + 25;
				nodeInfoText->updateText(g_doors[i]->to_map + "->" + g_doors[i]->to_point, 15, 15);
				nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}

			for (int i = 0; i < g_triggers.size(); i++) {
				SDL_Rect obj = {(g_triggers[i]->x -g_camera.x)* g_camera.zoom , ((g_triggers[i]->y - g_camera.y - ( g_triggers[i]->zeight ) * XtoZ) * g_camera.zoom), (g_triggers[i]->width * g_camera.zoom), (g_triggers[i]->height * g_camera.zoom)};
				SDL_RenderCopy(renderer, triggerIcon->texture, NULL, &obj);
				//the wall
				SDL_Rect obj2 = {(g_triggers[i]->x -g_camera.x)* g_camera.zoom, ((g_triggers[i]->y - g_camera.y - ( g_triggers[i]->zeight ) * XtoZ) * g_camera.zoom), (g_triggers[i]->width * g_camera.zoom), ( (g_triggers[i]->zeight - g_triggers[i]->z) * XtoZ * g_camera.zoom) + (g_triggers[i]->height * g_camera.zoom)};
				SDL_RenderCopy(renderer, triggerIcon->texture, NULL, &obj2);

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
		if(!inPauseMenu && g_showHUD) {
			adventureUIManager->healthText->updateText( to_string(int(protag->hp)) + '/' + to_string(int(protag->maxhp)), WIN_WIDTH * g_fontsize, 0.9 ); 
			adventureUIManager->healthText->show = 1;
			
		} else {
			adventureUIManager->healthText->show = 0;
			
		}
		adventureUIManager->healthbox->show = g_showHUD;
		for(long long unsigned int i=0; i < g_ui.size(); i++){
			g_ui[i]->render(renderer, g_camera);
		}	
		for(long long unsigned int i=0; i < g_textboxes.size(); i++){
			g_textboxes[i]->render(renderer, WIN_WIDTH, WIN_HEIGHT);
		}	

		//draw pause screen
		if(inPauseMenu) {
			
			//iterate thru inventory and draw items on screen
			float defaultX = WIN_WIDTH * 0.05;
			float defaultY = WIN_WIDTH * 0.05;
			float x = defaultX;
			float y = defaultY;
			float maxX = WIN_WIDTH * 0.9;
			float maxY = WIN_HEIGHT * 0.60;
			float itemWidth = WIN_WIDTH * 0.07;
			float padding = WIN_WIDTH * 0.01;

	
			int i = 0;
			for(auto it =  mainProtag->inventory.rbegin(); it != mainProtag->inventory.rend(); ++it) {

				if(i < itemsPerRow * inventoryScroll) {
					//this item won't be rendered
					i++;
					continue;
				}

				
				
				SDL_Rect drect = {x, y, itemWidth, itemWidth};
				SDL_RenderCopy(renderer, it->first->texture, NULL, &drect);

				//draw number
				if(it->second != 1) {
					inventoryText->show = 1;
					inventoryText->updateText( to_string(it->second), 35, 100);
					inventoryText->boxX = (x + itemWidth ) / WIN_WIDTH;
					inventoryText->boxY = (y + itemWidth - inventoryText->boxHeight/2 ) / WIN_HEIGHT;
					inventoryText->worldspace = 1;
					inventoryText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
				}

				if(i == inventorySelection) {
					//this item should have the marker
					inventoryMarker->show = 1;
					inventoryMarker->x = x / WIN_WIDTH;
					inventoryMarker->y = y / WIN_HEIGHT;
					inventoryMarker->width = itemWidth / WIN_WIDTH;
					inventoryMarker->height = inventoryMarker->width * ((float)WIN_WIDTH/(float)WIN_HEIGHT);
					inventoryMarker->render(renderer, g_camera);
				}

				x += itemWidth + padding;
				if(x > maxX) {
					x = defaultX;
					y += itemWidth + padding;
					if(y > maxY) {
						//we filled up the entire inventory, so lets leave
						break;
					}
				}
				i++;
			}
			g_itemsInInventory = mainProtag->inventory.size();
		} else {
			inventoryMarker->show = 0;
			inventoryText->show = 0;
		}

		// ENTITY MOVEMENT
		//dont update movement while transitioning
		for(long long unsigned int i=0; i < g_entities.size(); i++){
			door* taken = g_entities[i]->update(g_boxs[g_entities[i]->layer], g_doors, elapsed);
			if(taken != nullptr) {
				//player took this door
				//clear level
				
				//we will now clear the map, so we will save the door's destination map as a string
				const string savemap = "maps/" + taken->to_map + "/" + taken->to_map + ".map";
				const string dest_waypoint = taken->to_point;

				//render this frame

				clear_map(g_camera);
				load_map(renderer, savemap, dest_waypoint);
				

				//clear_map() will also delete engine tiles, so let's re-load them (but only if the user is map-editing)
				if(devMode) { init_map_writing(renderer);}
				


				break;
			}
		}

		//did the protag die?
		if(protag->hp <= 0 && protag->essential) {
			playSound(-1, g_deathsound, 0);
			clear_map(g_camera);
			SDL_Delay(5000);
			load_map(renderer, "maps/sp-death/sp-death.map","a");
			protag->hp = 0.1;
			if(devMode) { init_map_writing(renderer);}
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
		if(fadeFlag && musicFadeTimer > 1000 && newClosest != 0) {
			fadeFlag = 0;
			Mix_HaltMusic();
			Mix_FadeInMusic(newClosest->blip, -1, 1000);
			
		}

		//wakeup manager if it is sleeping
		if(adventureUIManager->sleepflag) {
			adventureUIManager->continueDialogue();
		}
	}


	clear_map(g_camera);
	delete adventureUIManager;
	close_map_writing();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_FreeSurface(transitionSurface);
	SDL_DestroyTexture(transitionTexture);
	SDL_DestroyTexture(background);
	SDL_DestroyTexture(g_shadowTexture);
	IMG_Quit();
	Mix_CloseAudio();
	TTF_Quit();
	
	return 0;
}

int interact(float elapsed, entity* protag) {
	M("interact()");
	SDL_Rect srect;
		switch(protag->animation) {
			
		case 0:
			srect.h = protag->bounds.height;
			srect.w = protag->bounds.width;

			srect.x = protag->getOriginX() - srect.w/2;
			srect.y = protag->getOriginY() - srect.h/2;

			srect.y -= 55;
			
			srect = transformRect(srect);
			// if(drawhitboxes) {
			// 	SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
			// 	SDL_RenderFillRect(renderer, &srect);
			// 	SDL_RenderPresent(renderer);	
			// 	SDL_Delay(500);
			// }
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
			
			if(g_entities[i] != protag && RectOverlap(hisrect, srect)) {
				if(g_entities[i]->isWorlditem) {
					//add item to inventory

					//if the item exists, dont make a new one
					indexItem* a = nullptr;
					for(auto x : g_indexItems) {
						if(g_entities[i]->name == x->name) {
							a = x;
						}
					}
					//no resource found, so lets just make one
					if(a == nullptr) {
						a = new indexItem(g_entities[i]->name, 0);
					}
					
					protag->getItem(a, 1);
					delete g_entities[i];
					break;
				}
				if(g_entities[i]->talks && g_entities[i]->tangible) {
					if(g_entities[i]->animlimit != 0) {
						g_entities[i]->animate = 1;
					}
					//make ent look at player, if they have the frames
					if(g_entities[i]->turnToFacePlayer && g_entities[i]->yframes >= 5) {
						
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
					}
					
					adventureUIManager->blip = g_entities[i]->voice;
					adventureUIManager->sayings = &g_entities[i]->sayings;
					adventureUIManager->talker = g_entities[i];
					adventureUIManager->talker->dialogue_index = -1;
					g_forceEndDialogue = 0;
					adventureUIManager->continueDialogue();
					//removing this in early july to fix problem moving after a script changes map
					//may cause unexpected problems
					//protag_is_talking = 1;
					break;
				}
			}
			//no one to talk to, lets do x instead (heres where it goes)
			if(use_cooldown <= 0) {
				// if(inPauseMenu) {
				// 	inPauseMenu = 0;
				// 	adventureUIManager->hideInventoryUI();
				// } else {
				// 	inPauseMenu = 1;
				// 	adventureUIManager->showInventoryUI();
				// }
				
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

		if(keystate[bindings[9]]) {
			input[9] = 1;	
		} else {
			input[9] = 0;
		}

		if(keystate[bindings[10]]) {
			input[10] = 1;
		} else {
			input[10] = 0;
		}
		
		if(keystate[bindings[0]]) {
			if(inPauseMenu && SoldUIUp <= 0) {
				//if(inventorySelection - itemsPerRow >= 0) {
					inventorySelection-= itemsPerRow;
					
					
				//}
				SoldUIUp = (oldUIUp) ? 6 : 30;				
			} else {
				protag->move_up();
			}
			oldUIUp = 1;
		} else {
			oldUIUp = 0;
			SoldUIUp = 0;
		}
		SoldUIUp--;
		
		if(keystate[bindings[1]]) {
			if(inPauseMenu && SoldUIDown <= 0) {
				//if(inventorySelection + itemsPerRow < g_itemsInInventory) {
					

					if( ceil((float)(inventorySelection+1) / (float)itemsPerRow) < (g_itemsInInventory / g_inventoryRows) ) {
						inventorySelection += itemsPerRow;
					}
				//}
				SoldUIDown = (oldUIDown) ? 6 : 30;
			} else {
				protag->move_down();
			}
			oldUIDown = 1;
		} else {
			oldUIDown = 0;
			SoldUIDown = 0;
		}
		SoldUIDown--;
		
		if(keystate[bindings[2]]) {
			if(inPauseMenu && SoldUILeft <= 0) {
				if(inventorySelection > 0) {
					if(inventorySelection % itemsPerRow != 0) {
						inventorySelection--;
					}
				}
				SoldUILeft = (oldUILeft) ? 6 : 30;
			} else {
				protag->move_left();
			}
			oldUILeft = 1;
		} else {
			oldUILeft = 0;
			SoldUILeft = 0;
		}
		SoldUILeft--;
		
		
		//constrain inventorySelection based on itemsInInventory
		if(inventorySelection > g_itemsInInventory - 1) {
			//M(g_itemsInInventory - 1);
			inventorySelection = g_itemsInInventory - 1;
		}

		if(inventorySelection < 0) {
			inventorySelection = 0;
		}
		
		

		if(keystate[bindings[3]]) {
			if(inPauseMenu && SoldUIRight <= 0) {
				if(inventorySelection <= g_itemsInInventory) {
					//dont want this to wrap around
					if(inventorySelection % itemsPerRow != itemsPerRow -1 ) { 
						inventorySelection++;
					}
				}
				SoldUIRight = (oldUIRight) ? 6 : 30;
			} else {
				protag->move_right();
			}
			oldUIRight = 1;
		} else {
			oldUIRight = 0;
			SoldUIRight = 0;
		}
		SoldUIRight--;

		
		// //fix inventory input
		// if(inventorySelection < 0) {
		// 	inventorySelection = 0;
		// }

		//check if the stuff is onscreen
		if(inventorySelection >= (g_inventoryRows * itemsPerRow) + (inventoryScroll * itemsPerRow) ) {
			inventoryScroll++;
		} else {
			if(inventorySelection < (inventoryScroll * itemsPerRow) ) {
				inventoryScroll--;
			}
		}

		if(keystate[bindings[12]] && !old_pause_value && protag_can_move) {
			//pause menu
			if(inPauseMenu) {
				inPauseMenu = 0;
				adventureUIManager->hideInventoryUI();
			
			} else {
				inPauseMenu = 1;
				inventorySelection = 0;
				adventureUIManager->showInventoryUI();
			}
		}
		
		if(keystate[bindings[12]]) {
			old_pause_value = 1;
		} else {
			old_pause_value = 0;
		}
		
	} else {
		if(keystate[bindings[2]] && !left_ui_refresh) {
			if(adventureUIManager->askingQuestion) {
				adventureUIManager->response_index--;
				if(adventureUIManager->response_index < 0) {
					adventureUIManager->response_index++;
				}
			}
			left_ui_refresh = 1;
		} else if(!keystate[bindings[2]]){ left_ui_refresh = 0;}
		if(keystate[bindings[3]] && !right_ui_refresh) {
			if(adventureUIManager->askingQuestion) {
				adventureUIManager->response_index++;
				if(adventureUIManager->response_index > adventureUIManager->responses.size() - 1) {
					adventureUIManager->response_index--;
				}
			}
			right_ui_refresh = 1;
		} else if(!keystate[bindings[3]]) { right_ui_refresh = 0;}
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

	if(keystate[bindings[11]] && !old_z_value && !inPauseMenu) {
		if(protag_is_talking == 1) {
			if(!adventureUIManager->typing) {
				adventureUIManager->continueDialogue();
			}
		}	
	}
	dialogue_cooldown -= elapsed;
	if(keystate[bindings[11]] && !inPauseMenu) {
		
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
	//b = set box left corner
	//n = set box right corner
	if(keystate[SDL_SCANCODE_LSHIFT] && devMode) {
		protag->xmaxspeed = 145;
		protag->ymaxspeed = 145;
	}
	if(keystate[SDL_SCANCODE_LCTRL] && devMode) {
		protag->xmaxspeed = 20;
		protag->ymaxspeed = 20;
	}
	if(keystate[SDL_SCANCODE_CAPSLOCK] && devMode) {
		protag->xmaxspeed = 750;
		protag->ymaxspeed = 750;
	}
	
	if(keystate[SDL_SCANCODE_SLASH] && devMode) {
		
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
		//this is make-trigger
		devinput[0] = 1;
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
		//enable/disable collisions
		devinput[9] = 1;
	}
	if(keystate[SDL_SCANCODE_KP_2] && devMode) {
		//visualize boxs
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

	if(keystate[SDL_SCANCODE_COMMA] && devMode) {
		//make navnode box
		devinput[20] = 1;
	}

	if(keystate[SDL_SCANCODE_PERIOD] && devMode) {
		//make navnode box
		devinput[21] = 1;
	}

	if(keystate[SDL_SCANCODE_ESCAPE]) {
		if(devMode) {
			quit = 1;
		} else {
			if(inPauseMenu) {
				quit = 1;
			}
		}
	}
	if(keystate[SDL_SCANCODE_Q] && devMode && g_holdingCTRL) {
		
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
	
	if(keystate[SDL_SCANCODE_E] && devMode && g_holdingCTRL) {
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
		g_fullscreen = !g_fullscreen;
		if(g_fullscreen) {
			SDL_SetWindowFullscreen(window, 0);
			
		} else {
			SDL_GetCurrentDisplayMode(0, &DM);
			SDL_SetWindowSize(window, DM.w, DM.h);
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
		}
	}

	if(keystate[SDL_SCANCODE_1] && devMode) {
		devinput[16] = 1;
	}

	if(keystate[SDL_SCANCODE_2] && devMode) {
		devinput[17] = 1;
	}

	if(keystate[SDL_SCANCODE_3] && devMode) {
		devinput[18] = 1;
	}
}