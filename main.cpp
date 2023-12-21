#include <iostream>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdlib.h>
#include <chrono>
#include <thread>

#include "globals.h"
#include "objects.h"
#include "map_editor.h"
#include "lightcookies.h"

using namespace std;

void flushInput();

void getInput(float &elapsed);

void toggleDevmode();

void toggleFullscreen();

void protagMakesNoise();


int WinMain()
{

  //devMode = 0;
  devMode = 1;

  canSwitchOffDevMode = devMode;

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
  TTF_Init();

  window = SDL_CreateWindow("Game",
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALWAYS_ON_TOP);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  SDL_SetWindowMinimumSize(window, 100, 100);

  SDL_SetWindowPosition(window, 1280, 720);

  // for( int i = 0; i < SDL_GetNumRenderDrivers(); ++i ) {
  // 	SDL_RendererInfo rendererInfo = {};
  // 	SDL_GetRenderDriverInfo( i, &rendererInfo );
  // 	if( rendererInfo.name != std::string( "opengles2" ) ) {
  // 		//provide info about improper renderer
  // 		continue;
  // 	}

  // 	renderer = SDL_CreateRenderer( window, i, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
  // 	break;
  // }

  Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
  SDL_RenderSetIntegerScale(renderer, SDL_FALSE);
 
  //used to be set to 3, changed to 0 to allow "totems" (objects in the world
  //made of several entities) to not have thin lines between their parts
  //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "3");
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

  SDL_RenderSetScale(renderer, scalex * g_zoom_mod, scalex * g_zoom_mod);

  // for brightness
  // reuse texture for transition, cuz why not
  SDL_Surface *brightness_a_s = IMG_Load("engine/transition.bmp");
  SDL_Texture *brightness_a = SDL_CreateTextureFromSurface(renderer, brightness_a_s);
  SDL_FreeSurface(brightness_a_s);

  SDL_Surface *brightness_b_s = IMG_Load("engine/black-diffuse.bmp");
  SDL_Texture *brightness_b = SDL_CreateTextureFromSurface(renderer, brightness_b_s);
  SDL_FreeSurface(brightness_b_s);

  // SDL_RenderSetLogicalSize(renderer, 1920, 1080); //for enforcing screen resolution

  // entities will be made here so have them set as created during loadingtime and not arbitrarily during play
  g_loadingATM = 1;

  // set global shadow-texture

  //!!! move these to the engine folder
  SDL_Surface *image = IMG_Load("engine/shadow.bmp");
  g_shadowTexture = SDL_CreateTextureFromSurface(renderer, image);
  SDL_FreeSurface(image);
  image = IMG_Load("engine/shadow-square.bmp");
  g_shadowTextureAlternate = SDL_CreateTextureFromSurface(renderer, image);
  SDL_FreeSurface(image);

  // narrarator holds scripts caused by things like triggers
  //  !!! reduce first launch overhead by
  //  making the narrarator use a sprite with 1 pixel
  narrarator = new entity(renderer, "engine/sp-joseph");
  narrarator->tangible = 0;
  narrarator->persistentHidden = 1;

  g_pelletNarrarator = new entity(renderer, "engine/sp-joseph");
  g_pelletNarrarator->tangible = 0;
  g_pelletNarrarator->persistentHidden = 1;

  g_backpackNarrarator = new entity(renderer, "engine/sp-joseph");
  g_backpackNarrarator->tangible = 0;
  g_backpackNarrarator->persistentHidden = 1;

  // entity* fomm;
  // //fomm = new entity(renderer, "fomm");

  // fomm->inParty = 1;
  // party.push_back(fomm);
  // fomm->footstep = Mix_LoadWAV("sounds/protag-step-1.wav");
  // fomm->footstep2 = Mix_LoadWAV("sounds/protag-step-2.wav");
  // protag = fomm;
  // g_cameraShove = protag->hisweapon->attacks[0]->range/2;
  // g_focus = protag;

  // g_deathsound = Mix_LoadWAV("audio/sounds/game-over.wav");

  // protag healthbar
//  ui *protagHealthbarA = new ui(renderer, "static/ui/healthbarA.bmp", 0, 0, 0.05, 0.02, -3);
//  protagHealthbarA->persistent = 1;
//  ui *protagHealthbarB = new ui(renderer, "static/ui/healthbarB.bmp", 0, 0, 0.05, 0.02, -2);
//  protagHealthbarB->persistent = 1;
//  protagHealthbarB->shrinkPixels = 1;
//
//  ui *protagHealthbarC = new ui(renderer, "static/ui/healthbarC.bmp", 0, 0, 0.05, 0.02, -1);
//  protagHealthbarC->persistent = 1;
//  protagHealthbarC->shrinkPixels = 1;
//
//  protagHealthbarA->show = g_showHealthbar;
//  protagHealthbarB->show = g_showHealthbar;
//  protagHealthbarC->show = g_showHealthbar;


  // for transition
  SDL_Surface *transitionSurface = IMG_Load("engine/transition.bmp");

  int transitionImageWidth = transitionSurface->w;
  int transitionImageHeight = transitionSurface->h;

  SDL_Texture *transitionTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, transitionSurface->w, transitionSurface->h);
  SDL_SetTextureBlendMode(transitionTexture, SDL_BLENDMODE_BLEND);

  void *transitionPixelReference;
  int transitionPitch;

  float transitionDelta = transitionImageHeight;

  // font
  g_font = "engine/fonts/Rubik-Bold.ttf";

  // setup UI
  adventureUIManager = new adventureUI(renderer);
  // The adventureUI class has three major uses:
  // The adventureUIManager points to an instance with the full UI for the player
  // The narrarator->myScriptCaller points to an instance which might have some dialogue, so it needs some ui
  // Many objects have a pointer to an instance which is used to just run scripts, and thus needs no dialgue box
  // To init the UI which is wholey unqiue to the instance pointed to by the adventureUIManager, we must  wa
  adventureUIManager->initFullUI();


  if (canSwitchOffDevMode)
  {
    init_map_writing(renderer);
    // done once, because textboxes aren't cleared during clear_map()
    nodeInfoText = new textbox(renderer, "",  g_fontsize, 0, 0, WIN_WIDTH);
    g_config = "edit";
    nodeDebug = SDL_CreateTextureFromSurface(renderer, IMG_Load("engine/walkerYellow.bmp"));
  }

  // set bindings from file
  ifstream bindfile;
  bindfile.open("user/configs/" + g_config + ".cfg");
  string line;
  for (int i = 0; i < 14; i++)
  {
    getline(bindfile, line);
    bindings[i] = SDL_GetScancodeFromName(line.c_str());
    // D(bindings[i]);
  }
  // set vsync and g_fullscreen from config
  string valuestr;
  float value;

  // get vsync
  //  getline(bindfile, line);
  //  valuestr = line.substr(line.find(' '), line.length());
  //  value = stoi(valuestr);
  //  D(value);
  //  g_vsync = value;
  //  D(g_vsync);

  // get g_fullscreen
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stoi(valuestr);
  g_fullscreen = value;

  // get bg darkness
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stof(valuestr);
  g_background_darkness = value;

  // get music volume
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stof(valuestr);
  g_music_volume = value;

  // get sfx volume
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stof(valuestr);
  g_sfx_volume = value;

  // get standard textsize
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stof(valuestr);
  g_fontsize = value;

  // get mini textsize
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stof(valuestr);
  g_minifontsize = value;

  // transitionspeed
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stof(valuestr);
  g_transitionSpeed = value;

  // mapdetail
  //  0 -   - ultra low - no lighting, crappy settings for g_tilt_resolution
  //  1 -   -
  //  2 -
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stof(valuestr);
  g_graphicsquality = value;

  //adjustment of brightness
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stoi(valuestr);
  g_brightness = value;
  SDL_Surface* shadesurface = IMG_Load("engine/black-diffuse.bmp");
  g_shade = SDL_CreateTextureFromSurface(renderer, shadesurface);
  //SDL_SetTextureAlphaMod(g_shade, 255 - ( ( g_brightness/100.0 ) * 255));
  SDL_SetWindowBrightness(window, g_brightness/100.0 );
  SDL_SetTextureAlphaMod(g_shade, 0);
  SDL_FreeSurface(shadesurface);

  switch (g_graphicsquality)
  {
    case 0:
      g_TiltResolution = 16;
      g_platformResolution = 55;
      g_unlit = 1;
      break;

    case 1:
      g_TiltResolution = 4;
      g_platformResolution = 11;
      break;
    case 2:
      g_TiltResolution = 2;
      g_platformResolution = 11;
      break;
    case 3:
      g_TiltResolution = 1;
      g_platformResolution = 11;
      break;
  }

  bindfile.close();

  // apply vsync
  SDL_GL_SetSwapInterval(1);

  // hide mouse
  //

  // apply fullscreen
  if (g_fullscreen)
  {
    SDL_GetCurrentDisplayMode(0, &DM);
    SDL_SetWindowSize(window, DM.w, DM.h);
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
  }
  else
  {
    SDL_SetWindowFullscreen(window, 0);
  }

  // initialize box matrix z
  for (int i = 0; i < g_layers; i++)
  {
    vector<box *> v = {};
    g_boxs.push_back(v);
  }

  for (int i = 0; i < g_layers; i++)
  {
    vector<tri *> v = {};
    g_triangles.push_back(v);
  }

  for (int i = 0; i < g_layers; i++)
  {
    vector<ramp *> v = {};
    g_ramps.push_back(v);
  }

  for (int i = 0; i < g_numberOfInterestSets; i++)
  {
    vector<pointOfInterest *> v = {};
    g_setsOfInterest.push_back(v);
  }

  // init static resources
  g_bulletdestroySound = Mix_LoadWAV("static/sounds/step.wav");
  g_preloadedSounds.emplace_back(g_bulletdestroySound, "static/sounds/step.wav");

  g_cannonfireSound = Mix_LoadWAV("static/sounds/cannonfire.wav");
  g_preloadedSounds.emplace_back(g_cannonfireSound , "static/sounds/cannonfire.wav");

  g_playerdamage = Mix_LoadWAV("static/sounds/playerdamage.wav");
  g_preloadedSounds.emplace_back(g_playerdamage, "static/sounds/playerdamage.wav");

  g_enemydamage = Mix_LoadWAV("static/sounds/enemydamage.wav");
  g_preloadedSounds.emplace_back(g_enemydamage, "static/sounds/enemydamage.wav");

  g_npcdamage = Mix_LoadWAV("static/sounds/npcdamage.wav");
  g_preloadedSounds.emplace_back(g_npcdamage, "static/sounds/npcdamage.wav");

  g_s_playerdeath = Mix_LoadWAV("static/sounds/playerdeath.wav");
  g_preloadedSounds.emplace_back(g_s_playerdeath, "static/sounds/playerdeath.wav");

  g_land = Mix_LoadWAV("static/sounds/land.wav");
  g_preloadedSounds.emplace_back(g_land, "static/sounds/step2.wav");

  g_footstep_a = Mix_LoadWAV("static/sounds/protag-step-1.wav");
  g_preloadedSounds.emplace_back(g_footstep_a, "static/sounds/protag-step-1.wav");

  g_footstep_b = Mix_LoadWAV("static/sounds/protag-step-2.wav");
  g_preloadedSounds.emplace_back(g_footstep_b, "static/sounds/protag-step-2.wav");

  g_bonk = Mix_LoadWAV("static/sounds/bonk.wav");
  g_preloadedSounds.emplace_back(g_bonk, "static/sounds/bonk.wav");

  g_menu_open_sound = Mix_LoadWAV("static/sounds/open-menu.wav");
  g_preloadedSounds.emplace_back(g_menu_open_sound, "static/sounds/open-menu.wav");

  g_menu_close_sound = Mix_LoadWAV("static/sounds/close-menu.wav");
  g_preloadedSounds.emplace_back(g_menu_close_sound, "static/sounds/close-menu.wav");

  g_ui_voice = Mix_LoadWAV("static/sounds/voice-low.wav");
  g_preloadedSounds.emplace_back(g_ui_voice, "static/sounds/voice-low.wav");

  g_menu_manip_sound = Mix_LoadWAV("static/sounds/manip-menu.wav");
  g_preloadedSounds.emplace_back(g_menu_manip_sound, "static/sounds/manip-menu.wav");

  g_pelletCollectSound = Mix_LoadWAV("static/sounds/pellet.wav");
  g_preloadedSounds.emplace_back(g_pelletCollectSound, "static/sounds/pellet.wav");

  g_spiketrapSound = Mix_LoadWAV("static/sounds/spiketrap.wav");
  g_preloadedSounds.emplace_back(g_spiketrapSound, "static/sounds/spiketrap.wav");

  g_bladetrapSound = Mix_LoadWAV("static/sounds/bladetrap.wav");
  g_preloadedSounds.emplace_back(g_spiketrapSound, "static/sounds/bladetrap.wav");

  g_smarttrapSound = Mix_LoadWAV("static/sounds/smarttrap.wav");
  g_preloadedSounds.emplace_back(g_spiketrapSound, "static/sounds/smarttrap.wav");

  if(devMode) {
    g_dijkstraDebugRed = new ui(renderer, "engine/walkerRed.bmp", 0,0,32,32, 3);
    g_dijkstraDebugRed->persistent = 1;
    g_dijkstraDebugRed->worldspace = 1;
    g_dijkstraDebugBlue = new ui(renderer, "engine/walkerBlue.bmp", 0,0,32,32, 3);
    g_dijkstraDebugBlue->persistent = 1;
    g_dijkstraDebugBlue->worldspace = 1;
    g_dijkstraDebugYellow = new ui(renderer, "engine/walkerYellow.bmp", 0,0,32,32, 3);
    g_dijkstraDebugYellow->persistent = 1;
    g_dijkstraDebugYellow->worldspace = 1;
  }

  //init user keyboard
  //render each character of the alphabet to a texture
  TTF_Font* alphabetfont = 0;
  alphabetfont = TTF_OpenFont(g_font.c_str(), 60 * g_fontsize);
  SDL_Surface* textsurface = 0;
  SDL_Texture* texttexture = 0;
  g_alphabet_textures = &g_alphabetLower_textures;
  for (int i = 0; i < g_alphabet.size(); i++) {
    string letter = "";
    letter += g_alphabet_lower[i];
    bool special = 0;
    if(letter == ";") {
      //load custom enter graphic
      textsurface = IMG_Load("static/ui/menu_confirm.bmp");
      special = 1;
    } else if (letter == "<") {
      //load custom backspace graphic
      textsurface = IMG_Load("static/ui/menu_back.bmp");
      special = 1;
    } else if (letter == "^") {
      //load custom capslock graphic
      textsurface = IMG_Load("static/ui/menu_upper_empty.bmp");
      special = 1;
    } else {
      textsurface = TTF_RenderText_Blended_Wrapped(alphabetfont, letter.c_str(), g_textcolor, 70);
    }
    texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);

    int texW = 0;int texH = 0;
    SDL_QueryTexture(texttexture, NULL, NULL, &texW, &texH);
    if(!special) {
      texW *= 1.1; //gotta boosh out those letters
      g_alphabet_widths.push_back(texW);
    } else {
      g_alphabet_widths.push_back(50);
    }

    //SDL_SetTextureBlendMode(texttexture, SDL_BLENDMODE_ADD);
    g_alphabetLower_textures.push_back(texttexture);
    SDL_FreeSurface(textsurface);
  }

  for (int i = 0; i < g_alphabet.size(); i++) {
    string letter = "";
    letter += g_alphabet_upper[i];
    bool special = 0;
    if(letter == ";") {
      //load custom enter graphic
      textsurface = IMG_Load("static/ui/menu_confirm.bmp");
      special = 1;
    } else if (letter == "<") {
      //load custom backspace graphic
      textsurface = IMG_Load("static/ui/menu_back.bmp");
      special = 1;
    } else if (letter == "^") {
      //load custom capslock graphic
      textsurface = IMG_Load("static/ui/menu_upper.bmp");
      special = 1;
    } else {
      textsurface = TTF_RenderText_Blended_Wrapped(alphabetfont, letter.c_str(), g_textcolor, 70);
    }
    texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);

    int texW = 0;int texH = 0;
    SDL_QueryTexture(texttexture, NULL, NULL, &texW, &texH);

    if(!special) {
      texW *= 1.1; //gotta boosh out those letters
      g_alphabet_upper_widths.push_back(texW);
    } else {
      g_alphabet_upper_widths.push_back(50);
    }

    //SDL_SetTextureBlendMode(texttexture, SDL_BLENDMODE_ADD);
    g_alphabetUpper_textures.push_back(texttexture);
    SDL_FreeSurface(textsurface);
  }

  //fancy alphabet
  int fancyIndex = 0;
  for(char character : g_fancyAlphabetChars) { //not a char
    string letter = "";
    letter += character;

    // add support for special chars here
    textsurface = TTF_RenderText_Blended_Wrapped(alphabetfont, letter.c_str(), g_textcolor, 70);

    texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);

    int texW = 0;int texH = 0;
    SDL_QueryTexture(texttexture, NULL, NULL, &texW, &texH);

    float texWidth = texW;
    texWidth *= 0.2;
    
    std::pair<SDL_Texture*, float> imSecond(texttexture, texW);
    g_fancyAlphabet.insert( {fancyIndex, imSecond} );
    
    g_fancyCharLookup.insert({character, fancyIndex});
    
    fancyIndex++;

    SDL_FreeSurface(textsurface);
  }

  TTF_CloseFont(alphabetfont);
  g_fancybox = new fancybox();


  //init options menu
  g_settingsUI = new settingsUI();

  g_escapeUI = new escapeUI();
  
  { //load static textures
    string loadSTR = "levelsequence/icons/locked.bmp";
    SDL_Surface* loadMe = IMG_Load(loadSTR.c_str());
    g_locked_level_texture = SDL_CreateTextureFromSurface(renderer, loadMe);
    SDL_FreeSurface(loadMe);
  }

  //load levelSequence
  g_levelSequence = new levelSequence(g_levelSequenceName, renderer);

  //for dlc/custom content, add extra levels from any file that might be there
  for(const auto & entry : filesystem::directory_iterator("levelsequence/")) {
    if(entry.path() != "levelsequence/" + g_levelSequenceName + ".txt" && entry.path() != "levelsequence/icons") {
      string entryStr = entry.path().string();
      g_levelSequence->addLevels(entryStr);
    }
  }

  //This is used to call scripts triggered by pellet goals
  g_pelletGoalScriptCaller = new adventureUI(renderer, 1);
  g_pelletGoalScriptCaller->playersUI = 0;
  g_pelletGoalScriptCaller->useOwnScriptInsteadOfTalkersScript = 1;
  g_pelletGoalScriptCaller->talker = narrarator;

  //this is used for calling scripts from items in the backpack
  g_backpackScriptCaller = new adventureUI(renderer, 1);
  g_backpackScriptCaller->playersUI = 0;
  g_backpackScriptCaller->useOwnScriptInsteadOfTalkersScript = 1;
  g_pelletGoalScriptCaller->talker = narrarator;

  srand(time(NULL));
  if (devMode)
  {
    // g_transitionSpeed = 10000;
    
    loadSave();
     
    string filename = g_levelSequence->levelNodes[0]->mapfilename;

    load_map(renderer, filename,"a");
    vector<string> x = splitString(filename, '/');
    g_mapdir = x[1];
    
    protag->x = 100000;
    protag->y = 100000;
  }
  else
  {
    SDL_ShowCursor(0);
    loadSave();

    string filename = g_levelSequence->levelNodes[0]->mapfilename;

    load_map(renderer, filename,"a");
    vector<string> x = splitString(filename, '/');
    g_mapdir = x[1];
  }

  inventoryMarker = new ui(renderer, "static/ui/finger_selector_angled.bmp", 0, 0, 0.1, 0.1, 2);
  inventoryMarker->show = 0;
  inventoryMarker->persistent = 1;
  inventoryMarker->renderOverText = 1;
  inventoryMarker->heightFromWidthFactor = 1;
  inventoryMarker->height = 1;

  g_UiGlideSpeedY = 0.012 * WIN_WIDTH/WIN_HEIGHT;

  inventoryText = new textbox(renderer, "1", 40,  g_fontsize, 0, WIN_WIDTH * 0.2);
  inventoryText->dropshadow = 1;
  inventoryText->show = 0;
  inventoryText->align = 1;

  
  g_itemsines.push_back( sin(g_elapsed_accumulator / 300) * 10 + 30);
  g_itemsines.push_back( sin((g_elapsed_accumulator - 1400) / 300) * 10 + 30);
  g_itemsines.push_back( sin((g_elapsed_accumulator + 925) / 300) * 10 + 30);
  g_itemsines.push_back( sin((g_elapsed_accumulator + 500) / 300) * 10 + 30);
  g_itemsines.push_back( sin((g_elapsed_accumulator + 600) / 300) * 10 + 30);
  g_itemsines.push_back( sin((g_elapsed_accumulator + 630) / 300) * 10 + 30);
  g_itemsines.push_back( sin((g_elapsed_accumulator + 970) / 300) * 10 + 30);
  g_itemsines.push_back( sin((g_elapsed_accumulator + 1020) / 300) * 10 + 30);
 

  // This stuff is for the FoW mechanic
  // engine/resolution.bmp has resolution 1920 x 1200
  SDL_Surface *SurfaceA = IMG_Load("engine/resolution.bmp");

  TextureA = SDL_CreateTextureFromSurface(renderer, SurfaceA);
  TextureD = SDL_CreateTextureFromSurface(renderer, SurfaceA);

  SDL_FreeSurface(SurfaceA);

  SDL_Surface *blackbarSurface = IMG_Load("engine/black.bmp");
  blackbarTexture = SDL_CreateTextureFromSurface(renderer, blackbarSurface);

  SDL_FreeSurface(blackbarSurface);

  result = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 500, 500);
  result_c = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 500, 500);

  SDL_SetTextureBlendMode(result, SDL_BLENDMODE_MOD);
  SDL_SetTextureBlendMode(result_c, SDL_BLENDMODE_MOD);

  SDL_SetTextureBlendMode(TextureA, SDL_BLENDMODE_MOD);
  SDL_SetTextureBlendMode(TextureA, SDL_BLENDMODE_MOD);
  SDL_SetTextureBlendMode(TextureD, SDL_BLENDMODE_MOD);

  // SDL_SetTextureBlendMode(TextureB, SDL_BLENDMODE_NONE);

  // init fogslates
  
  //strange displacement in debugger vs standalone
  //in debugger g_fc[20][19] = g_fc[20][20] = -1414812757
  //but standalone = 0
  //?
  for(int i = 0; i < g_fogcookies.size(); i++) {
    for(int j = 0; j < g_fogcookies.size(); j++) {
      g_fc[i][j] = 0;
      g_sc[i][j] = 0;
      g_fogcookies[i][j] = 0;
    }
  }


  entity *s = nullptr;
//  s->dynamic = 0;
//  s->width = 0;

  for (size_t i = 0; i < 19; i++)
  {
    s = new entity(renderer, "engine/sp-fogslate");
    g_fogslates.push_back(s);
    s->height = 56;
    s->width = g_fogwidth * 64 + 50;
    s->curheight = s->height - 1;
    s->curwidth = s->width;
    s->xframes = 1;
    s->yframes = 19;
    s->animation = i;
    // s->persistentGeneral = 1;
    s->frameheight = 26;
    s->framewidth = 500;
    s->shadow->width = 0;
    s->dynamic = 0;
    s->sortingOffset = 130; // -35 then 130
  }

  for (size_t i = 0; i < 19; i++)
  {
    s = new entity(renderer, "engine/sp-fogslate");
    g_fogslatesA.push_back(s);
    s->z = 64;
    s->height = 56;
    s->width = g_fogwidth * 64 + 50;
    s->curheight = s->height - 1;
    s->curwidth = s->width;
    s->xframes = 1;
    s->yframes = 19;
    s->animation = i;
    // s->persistentGeneral = 1;
    s->frameheight = 26;
    s->framewidth = 500;
    s->shadow->width = 0;
    s->dynamic = 0;
    s->sortingOffset = 165; // -65 55
    //s->width = 0;
  }

  for (size_t i = 0; i < 19; i++)
  {
    s = new entity(renderer, "engine/sp-fogslate");
    g_fogslatesB.push_back(s);
    s->height = 56;
    s->width = g_fogwidth * 64 + 50;
    s->curheight = s->height - 1;
    s->curwidth = s->width;
    s->xframes = 1;
    s->yframes = 19;
    s->animation = i;
    // s->persistentGeneral = 1;
    s->frameheight = 26;
    s->framewidth = 500;
    s->shadow->width = 0;
    s->dynamic = 0;
    s->sortingOffset = 45000; // !!! might need to be bigger
    //s->width = 0;
  }

  SDL_DestroyTexture(s->texture);


  //this is used when spawning in entities
  smokeEffect = new effectIndex("puff", renderer);
  smokeEffect->persistent = 1;

  littleSmokeEffect = new effectIndex("steam", renderer);
  littleSmokeEffect->persistent = 1;

  blackSmokeEffect = new effectIndex("blackpowder", renderer);
  blackSmokeEffect->persistent = 1;

  sparksEffect = new effectIndex("sparks", renderer);
  sparksEffect->persistent = 1;

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderPresent(renderer);
  SDL_GL_SetSwapInterval(1);

  // textures for adding operation
  canvas = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 500, 500);
  //canvas_fc = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 500, 500); seems to be unused

  SDL_Surface *lightSurface = IMG_Load("engine/light.bmp");
  light = SDL_CreateTextureFromSurface(renderer, lightSurface);
  SDL_FreeSurface(lightSurface);

  SDL_Surface *lightAS = IMG_Load("engine/lighta.bmp");
  lighta = SDL_CreateTextureFromSurface(renderer, lightAS);
  SDL_FreeSurface(lightAS);

  SDL_Surface *lightBS = IMG_Load("engine/lightb.bmp");
  lightb = SDL_CreateTextureFromSurface(renderer, lightBS);
  SDL_FreeSurface(lightBS);

  SDL_Surface *lightCS = IMG_Load("engine/lightc.bmp");
  lightc = SDL_CreateTextureFromSurface(renderer, lightCS);
  SDL_FreeSurface(lightCS);

  SDL_Surface *lightDS = IMG_Load("engine/lightd.bmp");
  lightd = SDL_CreateTextureFromSurface(renderer, lightDS);
  SDL_FreeSurface(lightDS);

  SDL_Surface *lightASro = IMG_Load("engine/lightaro.bmp");
  lightaro = SDL_CreateTextureFromSurface(renderer, lightASro);
  SDL_FreeSurface(lightASro);

  SDL_Surface *lightBSro = IMG_Load("engine/lightbro.bmp");
  lightbro = SDL_CreateTextureFromSurface(renderer, lightBSro);
  SDL_FreeSurface(lightBSro);

  SDL_Surface *lightCSro = IMG_Load("engine/lightcro.bmp");
  lightcro = SDL_CreateTextureFromSurface(renderer, lightCSro);
  SDL_FreeSurface(lightCSro);

  SDL_Surface *lightDSro = IMG_Load("engine/lightdro.bmp");
  lightdro = SDL_CreateTextureFromSurface(renderer, lightDSro);
  SDL_FreeSurface(lightDSro);

  SDL_Surface *lightASri = IMG_Load("engine/lightari.bmp");
  lightari = SDL_CreateTextureFromSurface(renderer, lightASri);
  SDL_FreeSurface(lightASri);

  SDL_Surface *lightBSri = IMG_Load("engine/lightbri.bmp");
  lightbri = SDL_CreateTextureFromSurface(renderer, lightBSri);
  SDL_FreeSurface(lightBSri);

  SDL_Surface *lightCSri = IMG_Load("engine/lightcri.bmp");
  lightcri = SDL_CreateTextureFromSurface(renderer, lightCSri);
  SDL_FreeSurface(lightCSri);

  SDL_Surface *lightDSri = IMG_Load("engine/lightdri.bmp");
  lightdri = SDL_CreateTextureFromSurface(renderer, lightDSri);
  SDL_FreeSurface(lightDSri);

  g_loadingATM = 0;


  while (!quit)
  {
//    for(auto x : g_ai) {
//      if(x->name == "common/zombie") {
//        
//        if(x->target != nullptr && x->target->name != "common/fomm") {
//          //I found a bug where they agrod on the vent, but afterwards I checked and the vents have faction -1, so that shouldn't happen
//          M("Is this the bug Joseph found on Nov 23 2023?");
//          M("Entity common/zombie has target.");
//          D(x->target->name);
//          breakpoint();
//        }         
//
//        //D(x->target->name);
//        // print any debug info
//        //D(x->aggressiveness);
//        //M("status check");
//        if(x->agrod) {
//          //M("Zombie is agrod.");
//        } else {
//          //M("Zombie is not agrod.");
//        }
//        
//      }
//      if(x->name == "common/creep") {
//        if(x->agrod) {
//          //M("Creep is agrod.");
//        } else {
//          //M("Creep is not agrod.");
//        }
//      }
//    }

    // some event handling
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_WINDOWEVENT:
          switch (event.window.event)
          {
            case SDL_WINDOWEVENT_RESIZED:
              // we need to reload some (all?) textures
              for (auto x : g_mapObjects)
              {
                if (x->mask_fileaddress != "&")
                {
                  x->reloadTexture();
                }
              }

              // reassign textures for asset-sharers
              for (auto x : g_mapObjects)
              {
                if (x->mask_fileaddress != "&")
                {
                  x->reassignTexture();
                }
              }

              // the same must be done for masked tiles
              for (auto t : g_tiles)
              {
                if (t->mask_fileaddress != "&")
                {
                  t->reloadTexture();
                }
              }

              // reassign textures for any asset-sharers
              for (auto x : g_tiles)
              {
                x->reassignTexture();
              }
              break;
          }
          break;
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym)
          {
            case SDLK_TAB:
              g_holdingCTRL = 1;
              // protag->getItem(a, 1);
              break;
            case SDLK_LALT:
              g_holdingTAB = 1;
          }
          if(g_swallowAKey) {
            M("We should swallow this key");
            g_swallowedKey = event.key.keysym.scancode;
            g_swallowAKey = 0;
            g_swallowedAKeyThisFrame = 1;
          } else {
            g_swallowedAKeyThisFrame = 0;
          }

          break;
        case SDL_KEYUP:
          switch (event.key.keysym.sym)
          {
            case SDLK_TAB:
              g_holdingCTRL = 0;
              break;
            case SDLK_LALT:
              g_holdingTAB = 0;
              break;
          }
          break;
        case SDL_MOUSEWHEEL:
          if (g_holdingCTRL)
          {
            if (event.wheel.y > 0)
            {
              wallstart -= 64;
            }
            else if (event.wheel.y < 0)
            {
              wallstart += 64;
            }
            if (wallstart < 0)
            {
              wallstart = 0;
            }
            else
            {
              if (wallstart > 64 * g_layers)
              {
                wallstart = 64 * g_layers;
              }
              if (wallstart > wallheight - 64)
              {
                wallstart = wallheight - 64;
              }
            }
          }
          else
          {
            if (event.wheel.y > 0)
            {
              wallheight -= 64;
            }
            else if (event.wheel.y < 0)
            {
              wallheight += 64;
            }
            if (wallheight < wallstart + 64)
            {
              wallheight = wallstart + 64;
            }
            else
            {
              if (wallheight > 64 * g_layers)
              {
                wallheight = 64 * g_layers;
              }
            }
            break;
          }
        case SDL_MOUSEBUTTONDOWN:
          if (event.button.button == SDL_BUTTON_LEFT)
          {
            devinput[3] = 1;
          }
          if (event.button.button == SDL_BUTTON_MIDDLE)
          {
            devinput[10] = 1;
          }
          if (event.button.button == SDL_BUTTON_RIGHT)
          {
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

    // I(elapsed);

    // lock time
    elapsed = 16;

    // cooldowns
    halfsecondtimer += elapsed;
    use_cooldown -= elapsed / 1000;
    musicFadeTimer += elapsed;
    musicUpdateTimer += elapsed;
    g_blinkingMS += elapsed;
    g_jumpGaranteedAccelMs -= elapsed;
    g_boardingCooldownMs -= elapsed;
    g_usableWaitToCycleTime -= elapsed;
    if(g_protagIsWithinBoardable) { 
      g_msSinceBoarding += elapsed;
    
      if(g_msSinceBoarding < 500) {
        g_boardedEntity->curwidth = g_boardedEntity->width + 10 * abs(sin( ((float)g_msSinceBoarding)/50.0));
        g_boardedEntity->curheight = g_boardedEntity->height + 10 * abs(sin( ((float)g_msSinceBoarding)/50.0));
      } else {
        g_boardedEntity->sizeRestoreMs = 1000;
      }
    }

    // g_dash_cooldown -= elapsed;
    
    if(g_blinkingMS >= g_maxBlinkingMS) {
      g_blinkHidden = !g_blinkHidden;
      g_blinkingMS = g_blinkHidden? g_maxBlinkingMS * 0.9 : 0;
    }

    if (inPauseMenu)
    {
      // if we're paused, freeze gametime
      elapsed = 0;
    }

    // INPUT
    getInput(elapsed);



    //lerp protag to boarded ent smoothly
    if(g_protagIsWithinBoardable) {
      if(g_transferingByBoardable) {
        g_transferingByBoardableTime += elapsed;
        if(g_transferingByBoardableTime >= g_maxTransferingByBoardableTime) {
          g_transferingByBoardable = 0;
        }
        float px = g_formerBoardedEntity->getOriginX();
        float py = g_formerBoardedEntity->getOriginY();
        float tx = g_boardedEntity->getOriginX();
        float ty = g_boardedEntity->getOriginY();
        float completion = g_transferingByBoardableTime / g_maxTransferingByBoardableTime;
        protag->setOriginX( px * (1-completion) + tx *(completion) );
        protag->setOriginY( py * (1-completion) + ty *(completion) );

      } else {
        //this is just a visual effect
        float px = protag->getOriginX();
        float py = protag->getOriginY();
        float tx = g_boardedEntity->getOriginX();
        float ty = g_boardedEntity->getOriginY();
        protag->setOriginX( (px + tx)/2 );
        protag->setOriginY( (py + ty)/2);
        //protag->z = (protag->z + g_boardedEntity->z) /2;
      }

    }

    // spring
    if ((input[8] && !oldinput[8] && protag->grounded && protag_can_move) || (input[8] && storedJump && protag->grounded && protag_can_move && !g_selectingUsable))
    {
      protagMakesNoise();
      g_hasBeenHoldingJump = 1;
      g_afterspin_duration = 0;
      g_spinning_duration = 0;
      protag->zaccel = 180;
      g_protag_jumped_this_frame = 1;
      g_jumpGaranteedAccelMs = g_maxJumpGaranteedAccelMs;
      storedJump = 0;
      breakpoint();
      
      //if we're boarded within an entity, unboard
      if(g_protagIsWithinBoardable && !g_transferingByBoardable) {
        
        //if the protag was currently duping a behemoth and hopped out, re-agro that behemoth
        for(auto x : g_ai) {
          if(x->useAgro && x->lostHimSequence != 0) {
            x->agrod = 1;
            x->target = protag;
          }
        }

        smokeEffect->happen(protag->getOriginX(), protag->getOriginY(), protag->z, 0);
        g_protagIsWithinBoardable = 0;
        g_boardedEntity->semisolidwaittoenable = 1;
        g_boardedEntity->semisolid = 0;
        g_boardedEntity->sizeRestoreMs = 1000;
        g_boardedEntity->storedSemisolidValue = 1;
        protag->steeringAngle = wrapAngle(-M_PI/2);
        protag->animation = 4;
        protag->animation = 4;
        protag->flip = SDL_FLIP_NONE;
        protag->xvel = 0;
        protag->yvel = 200;
        g_boardedEntity = 0;
        protag->tangible = 1;    
        //protag->y += 45; //push us out of the boarded entity in a consistent way
        g_boardingCooldownMs = g_maxBoardingCooldownMs;
      }                          
    }                            
    else                         
    {                            
      g_protag_jumped_this_frame = 0;
      if (input[8] && !oldinput[8] && !protag->grounded)
      {
        storedJump = 1;
      }
    }

    // if we die don't worry about not being able to switch because we can't shoot yet
    if (protag->hp <= 0)
    {
      playSound(4, g_s_playerdeath, 0);
      protag->cooldown = 0;
    }

    // cycle right if the current character dies
    if ((input[9] && !oldinput[9]) || protag->hp <= 0)
    {
      // keep switching if we switch to a dead partymember
      int i = 0;

      if (party.size() > 1 && protag->cooldown <= 0)
      {
        do
        {
          M("Cycle party right");
          std::rotate(party.begin(), party.begin() + 1, party.end());
          protag->tangible = 0;
          protag->flashingMS = 0;
          party[0]->tangible = 1;
          party[0]->x = protag->getOriginX() - party[0]->bounds.x - party[0]->bounds.width / 2;
          party[0]->y = protag->getOriginY() - party[0]->bounds.y - party[0]->bounds.height / 2;
          party[0]->z = protag->z;
          party[0]->xvel = protag->xvel;
          party[0]->yvel = protag->yvel;
          party[0]->zvel = protag->zvel;

          party[0]->animation = protag->animation;
          party[0]->flip = protag->flip;
          protag->zvel = 0;
          protag->xvel = 0;
          protag->yvel = 0;
          protag->zaccel = 0;
          protag->xaccel = 0;
          protag->yaccel = 0;
          protag = party[0];
          protag->shadow->x = protag->x + protag->shadow->xoffset;
          protag->shadow->y = protag->y + protag->shadow->yoffset;
          g_focus = protag;
          protag->curheight = 0;
          protag->curwidth = 0;
          g_cameraShove = protag->hisweapon->attacks[0]->range / 2;
          // prevent infinite loop
          i++;
          if (i > 600)
          {
            M("Avoided infinite loop: no living partymembers yet no essential death. (Did the player's party contain at least one essential character?)");
            break;
            quit = 1;
          }
        } while (protag->hp <= 0);
      }
    }
    // party swap
    if (input[10] && !oldinput[10])
    {
      if (party.size() > 1 && protag->cooldown <= 0)
      {
        int i = 0;
        do
        {
          M("Cycle party left");
          std::rotate(party.begin(), party.begin() + party.size() - 1, party.end());
          protag->tangible = 0;
          protag->flashingMS = 0;
          party[0]->tangible = 1;
          party[0]->x = protag->getOriginX() - party[0]->bounds.x - party[0]->bounds.width / 2;
          party[0]->y = protag->getOriginY() - party[0]->bounds.y - party[0]->bounds.height / 2;
          party[0]->z = protag->z;
          party[0]->xvel = protag->xvel;
          party[0]->yvel = protag->yvel;
          party[0]->zvel = protag->zvel;

          party[0]->animation = protag->animation;
          party[0]->flip = protag->flip;
          protag->zvel = 0;
          protag->xvel = 0;
          protag->yvel = 0;
          protag->zaccel = 0;
          protag->xaccel = 0;
          protag->yaccel = 0;
          protag = party[0];
          protag->shadow->x = protag->x + protag->shadow->xoffset;
          protag->shadow->y = protag->y + protag->shadow->yoffset;
          g_focus = protag;
          protag->curheight = 0;
          protag->curwidth = 0;
          g_cameraShove = protag->hisweapon->attacks[0]->range / 2;
          i++;
          if (i > 600)
          {
            M("Avoided infinite loop: no living partymembers yet no essential death. (Did the player's party contain at least one essential character?)");
            break;
            quit = 1;
          }
        } while (protag->hp <= 0);
      }
    }

    // background
    // SDL_SetRenderTarget(renderer, TextureA);
    SDL_RenderClear(renderer);
    if (g_backgroundLoaded && g_useBackgrounds)
    { // if the level has a background and the user would like to see it
      SDL_RenderCopy(renderer, background, NULL, NULL);
    }

    for (auto n : g_entities)
    {
      n->cooldown -= elapsed;
    }

    // listeners
    for (long long unsigned int i = 0; i < g_listeners.size(); i++)
    {
      if (g_listeners[i]->update())
      {
        // !!! could need check that we aren't in dialogue or running a script
        adventureUIManager->blip = NULL;
        adventureUIManager->ownScript = g_listeners[i]->script;
        adventureUIManager->talker = narrarator;
        adventureUIManager->dialogue_index = -1;
        narrarator->sayings = g_listeners[i]->script;
        adventureUIManager->initDialogue();
        adventureUIManager->continueDialogue();
        g_listeners[i]->active = 0;
      }
    }

    // update camera
    SDL_GetWindowSize(window, &WIN_WIDTH, &WIN_HEIGHT);

    // !!! it might be better to not run this every frame
    if (old_WIN_WIDTH != WIN_WIDTH || g_update_zoom)
    {
      // user scaled window
      scalex = ((float)WIN_WIDTH / STANDARD_SCREENWIDTH) * g_defaultZoom * g_zoom_mod;
      scaley = scalex;
      if (scalex < min_scale)
      {
        scalex = min_scale;
      }
      if (scalex > max_scale)
      {
        scalex = max_scale;
      }
      SDL_RenderSetScale(renderer, scalex * g_zoom_mod, scalex * g_zoom_mod);
      SDL_GetWindowSize(window, &WIN_WIDTH, &WIN_HEIGHT);
      g_UiGlideSpeedY = 0.012 * WIN_WIDTH/WIN_HEIGHT;
    }

    old_WIN_WIDTH = WIN_WIDTH;

    WIN_WIDTH /= scalex;
    WIN_HEIGHT /= scaley;

    //animate dialogproceedarrow
    {
      adventureUIManager->c_dpiDesendMs += elapsed;
      if(adventureUIManager->c_dpiDesendMs > adventureUIManager->dpiDesendMs) {
        adventureUIManager->c_dpiDesendMs = 0;
        adventureUIManager->c_dpiAsending = !adventureUIManager->c_dpiAsending;
  
      }
      
      if(adventureUIManager->c_dpiAsending) {
        adventureUIManager->dialogProceedIndicator->y += adventureUIManager->dpiAsendSpeed;
      } else {
        adventureUIManager->dialogProceedIndicator->y -= adventureUIManager->dpiAsendSpeed;

      }
    }

    if (devMode)
    {

      g_camera.width = WIN_WIDTH / (scalex * g_zoom_mod * 0.2); // the 0.2 is arbitrary. it just makes sure we don't end the camera before the screen
      g_camera.height = WIN_HEIGHT / (scalex * g_zoom_mod * 0.2);
    }
    else
    {
      g_camera.width = WIN_WIDTH;
      g_camera.height = WIN_HEIGHT;
    }

    if (freecamera)
    {
      g_camera.update_movement(elapsed, camx, camy);
    }
    else
    {
      // lerp cameratargets
      g_cameraAimingOffsetY = g_cameraAimingOffsetY * g_cameraAimingOffsetLerpScale + g_cameraAimingOffsetYTarget * (1 - (g_cameraAimingOffsetLerpScale));
      g_cameraAimingOffsetX = g_cameraAimingOffsetX * g_cameraAimingOffsetLerpScale + g_cameraAimingOffsetXTarget * (1 - (g_cameraAimingOffsetLerpScale));
      float zoomoffsetx = ((float)WIN_WIDTH / 2) / g_zoom_mod;
      float zoomoffsety = ((float)WIN_HEIGHT / 2) / g_zoom_mod;
      // g_camera.zoom = 0.9;

      if(g_hog == 0) {
        g_camera.update_movement(elapsed, g_focus->getOriginX() - zoomoffsetx + (g_cameraAimingOffsetX * g_cameraShove), ((g_focus->getOriginY() - XtoZ * g_focus->z) - zoomoffsety - (g_cameraAimingOffsetY * g_cameraShove)));
      } else {
        int avgX = g_focus->getOriginX() + g_hog->getOriginX(); avgX *= 0.5;
        int avgY = g_focus->getOriginY() + g_hog->getOriginY(); avgY *= 0.5;
        g_camera.update_movement(elapsed, avgX - zoomoffsetx, ((avgY - XtoZ * g_focus->z) - zoomoffsety));
      }
    }

    adventureUIManager->thisUsableIcon->texture = adventureUIManager->noIconTexture;
    adventureUIManager->nextUsableIcon->texture = adventureUIManager->noIconTexture;
    adventureUIManager->prevUsableIcon->texture = adventureUIManager->noIconTexture;
    //update hotbar + graphics
    if(g_backpack.size() > 0) {
      adventureUIManager->thisUsableIcon->texture = g_backpack.at(g_backpackIndex)->texture;
      adventureUIManager->nextUsableIcon->texture = g_backpack.at(g_backpackIndex)->texture;
      adventureUIManager->prevUsableIcon->texture = g_backpack.at(g_backpackIndex)->texture;
    }

    if(adventureUIManager->shiftingMs > 0) {
      adventureUIManager->shiftingMs -= elapsed;
    } else {
      //reposition them
      int i = 1;
      for(auto x : adventureUIManager->hotbarTransitionIcons) {
        x->x = adventureUIManager->hotbarPositions[i].first;
        x->y = adventureUIManager->hotbarPositions[i].second;
        x->targetx = adventureUIManager->hotbarPositions[i].first;
        x->targety = adventureUIManager->hotbarPositions[i].second;
        
        if(g_backpack.size() > 0) {
          int index = g_backpackIndex - i - 1;
          index = index % g_backpack.size();
          x->texture = g_backpack.at(index)->texture;
        }
        i++;
      }

    }

    float percentSame = 0.5 * elapsed / 16;
    float percentDiff = 1 - percentSame;
    if(g_selectingUsable) {
      g_hotbarWidth = (g_hotbarWidth *percentSame) + (g_hotbarWidth_inventoryOpen*percentDiff);
      g_hotbarNextPrevOpacity = (g_hotbarNextPrevOpacity * percentSame) + (25500 * percentDiff);

    } else {
      g_hotbarWidth = (g_hotbarWidth *percentSame) + (g_hotbarWidth_inventoryClosed*percentDiff);
      g_hotbarNextPrevOpacity = (g_hotbarNextPrevOpacity * percentSame) + (0 * percentDiff);
    }
    adventureUIManager->hotbar->width = g_hotbarWidth;
    adventureUIManager->hotbar->height = 0.1/g_hotbarWidth; //maintain height despite using heightFromWidthFactor
    //next/prev icons should move with hotbar
    adventureUIManager->nextUsableIcon->x = g_hotbarX + (g_hotbarWidth - 0.1)/2;
    adventureUIManager->prevUsableIcon->x = g_hotbarX - (g_hotbarWidth - 0.1)/2;
   
    adventureUIManager->hotbarPositions[4].first = g_hotbarX - (g_hotbarWidth - 0.1)/2 + g_backpackHorizontalOffset;
    adventureUIManager->hotbarPositions[2].first = g_hotbarX + (g_hotbarWidth - 0.1)/2 + g_backpackHorizontalOffset;

    adventureUIManager->nextUsableIcon->opacity = g_hotbarNextPrevOpacity;
    adventureUIManager->prevUsableIcon->opacity = g_hotbarNextPrevOpacity;
    
    adventureUIManager->t1->opacity = g_hotbarNextPrevOpacity;
    adventureUIManager->t2->opacity = g_hotbarNextPrevOpacity;
    adventureUIManager->t3->opacity = 25500;
    adventureUIManager->t4->opacity = g_hotbarNextPrevOpacity;
    adventureUIManager->t5->opacity = g_hotbarNextPrevOpacity;

    adventureUIManager->thisUsableIcon->opacity = 25500;
    
    adventureUIManager->hotbar->x = 0.65 - g_hotbarWidth + g_backpackHorizontalOffset;
    adventureUIManager->hotbarPositions[3].first = 0.1 + (g_hotbarX - (g_hotbarWidth - 0.1)/2 + g_backpackHorizontalOffset);
    adventureUIManager->hotbarFocus->x = 0.1 + 0.005 + (g_hotbarX - (g_hotbarWidth - 0.1)/2 + g_backpackHorizontalOffset);
    adventureUIManager->cooldownIndicator->x = adventureUIManager->hotbarPositions[3].first;

    //adventureUIManager->thisUsableIcon->show = 0;

    if(g_backpack.size() > 1) {
      int nextUsableIndex = g_backpackIndex + 1; 
      
      if(nextUsableIndex >= g_backpack.size()) { nextUsableIndex = 0;}
      adventureUIManager->nextUsableIcon->texture = g_backpack.at(nextUsableIndex)->texture;
      
      int prevUsableIndex = g_backpackIndex - 1; 
      if(prevUsableIndex < 0) { prevUsableIndex = g_backpack.size()-1;}
      
      adventureUIManager->prevUsableIcon->texture = g_backpack.at(prevUsableIndex)->texture;
    }

    

    // update ui
    curTextWait += elapsed * text_speed_up;
    if (curTextWait >= textWait)
    {
      adventureUIManager->updateText();
      curTextWait = 0;
    }

    // old Fogofwar
    if (g_fogofwarEnabled && !devMode)
    {

      // this is the worst functional code I've written, with no exceptions

      int functionalX = g_focus->getOriginX();
      int functionalY = g_focus->getOriginY();

      // int functionalX = g_camera.x + WIN_WIDTH/2;
      // int functionalY = g_camera.y = WIN_HEIGHT/2;

      functionalX -= functionalX % 64;
      functionalX += 32;
      functionalY -= functionalY % 55;
      functionalY += 26;

      // for low spec, update only when the player moves across a space
      if (g_graphicsquality == 0)
      {
        // for low spec
        if (functionalX != g_lastFunctionalX || functionalY != g_lastFunctionalY || g_force_cookies_update)
        {

          int xdiff = functionalX - g_lastFunctionalX;
          int ydiff = functionalY - g_lastFunctionalY;
          if(g_force_cookies_update) {xdiff = 0; ydiff = 0;}

          // to make old tiles fade out

          bool flipper = 0;
          for (long long unsigned i = 0; i < g_fogcookies.size(); i++)
          {
            for (long long unsigned j = 0; j < g_fogcookies[0].size(); j++)
            {
              flipper = !flipper;
              int xpos = ((i - g_fogMiddleX) * 64) + functionalX;
              int ypos = ((j - g_fogMiddleY) * 55) + functionalY;
              //if (XYWorldDistance(functionalX, functionalY, xpos, ypos) > g_viewdist)
              if(g_fog_window[i][j])
              {
                // g_fogcookies[i][j] -= g_tile_fade_speed; if (g_fogcookies[i][j] < 0) g_fogcookies[i][j] = 0;
                g_fogcookies[i][j] = 0;

                // g_fc[i][j] -= g_tile_fade_speed; if (g_fc[i][j] < 0) g_fc[i][j] = 0;
                g_fc[i][j] = 0;

                // g_sc[i][j] -= g_tile_fade_speed; if (g_sc[i][j] < 0) g_sc[i][j] = 0;
                g_sc[i][j] = 0;
              }
              else if (
                  LineTrace(functionalX, functionalY, xpos, ypos, 0, 35, g_focus->stableLayer, 15, 1, 1) 
                  &&
                  !g_transferingByBoardable

                  )
              {
                g_fogcookies[i][j] = 255;
                g_fc[i][j] = 255;

                g_sc[i][j] = 255;
              }
              else
              {

                // g_fogcookies[i][j] -= g_tile_fade_speed; if (g_fogcookies[i][j] < 0) g_fogcookies[i][j] = 0;
                g_fogcookies[i][j] = 0;

                // g_fc[i][j] -= g_tile_fade_speed; if (g_fc[i][j] < 0) g_fc[i][j] = 0;
                g_fc[i][j] = 0;

                // g_sc[i][j] -= g_tile_fade_speed; if (g_sc[i][j] < 0) g_sc[i][j] = 0;
                g_sc[i][j] = 0;
              }
            }
          }
        }
      }
      else
      {
        if (1)
        {

          if (functionalX != g_lastFunctionalX)
          {
            xtileshift = functionalX - g_lastFunctionalX;
            xtileshift = xtileshift / abs(xtileshift);

            if (functionalY == g_lastFunctionalY)
            {
              ytileshift = 0;
            }
          }

          if (functionalY != g_lastFunctionalY)
          {

            ytileshift = functionalY - g_lastFunctionalY;

            ytileshift = ytileshift / abs(ytileshift);

            if (functionalX == g_lastFunctionalX)
            {
              xtileshift = 0;
            }
          }

          if (g_force_cookies_update)
          {
            g_lastFunctionalX = functionalX;
            g_lastFunctionalY = functionalY;
          }

          // to make old tiles fade out, when we move set the tiles opacity to their "old" tile
          if (functionalX != g_lastFunctionalX || functionalY != g_lastFunctionalY || g_force_cookies_update)
          {

            g_force_cookies_update = 0;

            // copy the vector
            auto fogcopy = g_fogcookies;
            auto fccopy = g_fc;
            auto sccopy = g_sc;

            for (long long unsigned i = 0; i < g_fogcookies.size(); i++)
            {
              for (long long unsigned j = 0; j < g_fogcookies.size(); j++)
              {
                // check if there was an "old" cookie
                if ((i + xtileshift >= 0 && i + xtileshift < g_fogcookies.size()) && (j + ytileshift >= 0 && j + ytileshift < g_fogcookies.size() - 2))
                {
                  g_fogcookies[i][j] = fogcopy[i + xtileshift][j + ytileshift];
                  g_fc[i][j] = fccopy[i + xtileshift][j + ytileshift];
                  g_sc[i][j] = sccopy[i + xtileshift][j + ytileshift];
                }
              }
            }
          }

          for (long long unsigned i = 0; i < g_fogcookies.size(); i++)
          {
            for (long long unsigned j = 0; j < g_fogcookies[0].size(); j++)
            {
              

              int xpos = ((i - g_fogMiddleX) * 64) + functionalX;
              int ypos = ((j - g_fogMiddleY) * 55) + functionalY;

              int xpos_fc = ((i - 10) * 64) + functionalX;
              int ypos_fc = ((j - 9) * 55) + functionalY;

              bool blocked = 1;
              if (g_fogIgnoresLOS)
              {
                blocked = LineTrace(xpos, ypos, xpos, ypos, 0, 6, g_focus->stableLayer + 1, 15, 1, 1);
              }
              else
              {
                blocked = LineTrace(functionalX, functionalY + 3, xpos, ypos, 0, 1, g_focus->stableLayer + 1, 15, 1, 1);
              }

              int yBoost = 0;  //you could debate this, but since the block closest to the bottom of the screen is hidden by the shadow-cookie above it, I'll give the player an extra block of vision towards the bottom of the screen
              if(ypos < functionalY) {yBoost = 55;}

              //if (!(XYWorldDistance(functionalX, functionalY + yBoost, xpos, ypos) > g_viewdist) && blocked)
              if(g_fog_window[i][j] && blocked && !g_transferingByBoardable)
              {

                g_fogcookies[i][j] += g_tile_fade_speed * (elapsed / 60);
                if (g_fogcookies[i][j] > 255)
                {
                  g_fogcookies[i][j] = 255;
                }
                // g_fogcookies[i][j] = 0;

                // if you want to increment g_fc, there is an additional rule
                //

                g_fc[i][j] += g_tile_fade_speed * (elapsed / 60);
                if (g_fc[i][j] > 255)
                {
                  g_fc[i][j] = 255;
                }

                // g_fc[i][j] = 0;

                g_sc[i][j] += g_tile_fade_speed * (elapsed / 60);
                if (g_sc[i][j] > 255)
                {
                  g_sc[i][j] = 255;
                }
                // g_sc[i][j] = 0;
              }
              else
              {

                g_fogcookies[i][j] -= g_tile_fade_speed * (elapsed / 60);
                if (g_fogcookies[i][j] < 0)
                {
                  g_fogcookies[i][j] = 0;
                }
                // g_fogcookies[i][j] = 0;

                g_fc[i][j] -= g_tile_fade_speed * (elapsed / 60);
                if (g_fc[i][j] < 0)
                {
                  g_fc[i][j] = 0;
                }

                // g_fc[i][j] = 0;

                g_sc[i][j] -= g_tile_fade_speed * (elapsed / 60);
                if (g_sc[i][j] < 0)
                {
                  g_sc[i][j] = 0;
                }
                // g_sc[i][j] = 0;
              }
            }
          }
        }
      }

      // save cookies that are just dark because they are inside of walls to g_savedcookies
      // AND if they tile infront is at 255

      for (long long unsigned i = 0; i < g_fogcookies.size(); i++)
      {
        for (long long unsigned j = 0; j < g_fogcookies[0].size(); j++)
        {
          int xpos = ((i - 10) * 64) + functionalX;
          int ypos = ((j - 9) * 55) + functionalY;
          // is this cookie in a wall? or behind a wall

          if (g_focus->stableLayer > g_layers)
          {
            break;
          }
          if (j + 1 < g_fogcookies.size() && g_fc[i][j + 1] > 0)
          {
            bool firsttrace = LineTrace(xpos, ypos, xpos, ypos, 0, 15, g_focus->stableLayer + 1, 2, 1, 1);
            bool secondtrace = LineTrace(xpos, ypos + 55, xpos, ypos + 55, 0, 15, g_focus->stableLayer + 1, 2, 1, 1);
            rect a = {xpos, ypos, 5, 5};

            // for large entities
            // if(firsttrace == -1) {
            //	g_fc[i][j] = 255;
            //	g_sc[i][j] = 255;
            // } else {
            if ((!firsttrace || !secondtrace))
            {
              // !!!
              // g_fc[i][j] += g_tile_fade_speed*2; if (g_fc[i][j] >255) {g_fc[i][j] = 255;}
              g_fc[i][j] = 255;
            }
            //}
          }
        }
      }

      // this is meant to prevent nasty clipping of shadows and large entities
      for (auto x : g_large_entities)
      {
        // if(XYWorldDistance(functionalX, functionalY, x->getOriginX(), x->getOriginY()) < g_viewdist) {
        for (long long unsigned i = 0; i < g_fogcookies.size(); i++)
        {
          for (long long unsigned j = 0; j < g_fogcookies[0].size(); j++)
          {
            int xpos = ((i - 10) * 64) + functionalX;
            int ypos = ((j - 9) * 55) + functionalY;
            rect a = {xpos, ypos, 1, 1};

            if (RectOverlap(a, x->getMovedBounds()))
            {
              if (j > 0)
              {
                g_fc[i][j - 1] = 255;
              }
              g_fc[i][j] = 255;
              // g_sc[i][j] = 255;
            }
          }
        }
        //}
      }

      // if a cookie intercepts a large entity, dont darken it
      // for(long long unsigned i = 0; i < g_fogcookies.size(); i++) {
      //	for(long long unsigned j = 0; j < g_fogcookies[0].size(); j++) {
      //		int xpos = ((i - 10) * 64) + functionalX;
      //		int ypos = ((j - 9) * 55) + functionalY;
      //		//is this cookie in a wall? or behind a wall

      //		bool firsttrace = LineTrace(xpos, ypos, xpos, ypos, 0, 15, 0, 2, 0);
      //		bool secondtrace = LineTrace(xpos, ypos + 55, xpos, ypos +55, 0, 15, 0, 2, 0);
      //		//for large entities
      //		if(firsttrace == -1) {
      //			g_fc[i][j] = 255;
      //			g_sc[i][j] = 255;
      //			cout << "why is this not triggering" << endl;
      //		}
      //
      //	}
      //}

      // if a cookie is light, and it is intersecting a triangle, not that in g_shc
      for (long long unsigned i = 0; i < g_fogcookies.size(); i++)
      {
        for (long long unsigned j = 0; j < g_fogcookies[0].size(); j++)
        {
          g_shc[i][j] = -1;
          int xpos = ((i - g_fogMiddleX) * 64) + functionalX;
          int ypos = ((j - g_fogMiddleY) * 55) + functionalY;
          rect r = {xpos - 10, ypos - 10, 20, 20};
          if (1)
          {
            for (auto t : g_triangles[g_focus->stableLayer + 1])
            {
              if (TriRectOverlap(t, r))
              {
                if (g_fogcookies[i][j])
                {
                  g_shc[i][j] = t->type;
                  g_fc[i][j] += g_tile_fade_speed * (elapsed / 60);
                  if (g_fc[i][j] > 255)
                  {
                    g_fc[i][j] = 255;
                  }
                  g_fc[i][j] = 255;
                  g_shc[i][j] += 4 * t->style;
                }

                // g_fc[i][j] += g_tile_fade_speed * (elapsed/60); if (g_fc[i][j] >255) {g_fc[i][j] = 255;}
                // g_sc[i][j] -= g_tile_fade_speed * (elapsed/60); if (g_sc[i][j] < 0) {g_sc[i][j] = 0;}
                break;
              }
            }
          }
        }
      }
      SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "3");

      g_lastFunctionalX = functionalX;
      g_lastFunctionalY = functionalY;

      // these are the corners and the center
      //  g_fogcookies[0][0] = 1;
      //  g_fogcookies[20][0] = 1;
      //  g_fogcookies[20][17] = 1;
      //  g_fogcookies[0][17] = 1;
      //  g_fogcookies[10][9] = 1;

      px = -(int)g_focus->getOriginX() % 64;

      // offset us to the protag's location
      // int yoffset =  ((g_focus->y- (g_focus->z + g_focus->zeight) * XtoZ)) * g_camera.zoom;
      // the zeight is constant at level 2  for now
      int yoffset = (g_focus->getOriginY()) * g_camera.zoom;

      // and then subtract half of the screen
      yoffset -= yoffset % 55;
      yoffset -= (g_fogheight * 55 + 12) / 2;
      yoffset -= g_camera.y;

      // we do this stuff to keep the offset on the grid
      // yoffset -= yoffset % 55;

      // px = 64 - px - 64;
      // py = 55 - py - 55;
      //  50 50
      
//      M("--\n");
//      for(int i = 0; i < g_fogcookies.size(); i++) {
//        for(int j = 0; j < g_fogcookies.size(); j++) {
//          Dnn(i); Snn(); Dnn(j); Snn(); D(g_sc[i][j]);
//        }
//      }
//      M("--\n");
//      M("--\n");
//      for(int i = 0; i < 19; i++) {
//        Dnn(i); Snn();
//        Dnn(g_fogslates[i]->animation); Snn();
//        D(g_fogslates[i]->texture);
//      }
//      M("--\n");


      addTextures(renderer, g_fc, canvas, light, 500, 500, 250, 250, 0); //g_fc is normal

      TextureC = IlluminateTexture(renderer, TextureD, canvas, result_c);

      // render graphics
      FoWrect = {px - 23, yoffset + 15, g_fogwidth * 64 + 50, g_fogheight * 55 + 18};
      // SDL_RenderCopy(renderer, TextureC, NULL, &FoWrect);

      for (auto x : g_fogslates)
      {
        x->texture = TextureC;
      }


      for (size_t i = 0; i < g_fogslates.size(); i++)
      {
        g_fogslates[i]->x = (int)g_focus->getOriginX() + px - 657;										   // 658
        g_fogslates[i]->y = (int)g_focus->getOriginY() - ((int)g_focus->getOriginY() % 55) + 55 * i - 453; // 449
        g_fogslates[i]->z = g_focus->stableLayer * 64;
      }

      // do it for z = 64
      FoWrect.y -= 64 * XtoZ;
      // SDL_RenderCopy(renderer, TextureC, NULL, &FoWrect);
      for (auto x : g_fogslatesA)
      {
        x->texture = TextureC;
      }

      for (size_t i = 0; i < g_fogslatesA.size(); i++)
      {
        g_fogslatesA[i]->x = (int)g_focus->getOriginX() + px - 658;											// 655
        g_fogslatesA[i]->y = (int)g_focus->getOriginY() - ((int)g_focus->getOriginY() % 55) + 55 * i - 453; // 449
        g_fogslatesA[i]->z = g_focus->stableLayer * 64 + 64;
      }

      addTextures(renderer, g_sc, canvas, light, 500, 500, 250, 250, 1); //g_sc is normal

      TextureB = IlluminateTexture(renderer, TextureA, canvas, result);

      for (auto x : g_fogslatesB)
      {
        x->texture = TextureB;
        x->z = g_focus->stableLayer * 64 + 128;
      }

      for (size_t i = 0; i < g_fogslates.size(); i++)
      {
        g_fogslatesB[i]->x = (int)g_focus->getOriginX() + px - 658;											// 655
        g_fogslatesB[i]->y = (int)g_focus->getOriginY() - ((int)g_focus->getOriginY() % 55) + 55 * i - 453; // 449
      }

      // render graphics
      FoWrect.y -= 67 * XtoZ;
      // SDL_RenderCopy(renderer, TextureB, NULL, &FoWrect);
    }

    // don't render triangles hidden behind fogofwar
    if (g_fogofwarEnabled && g_trifog_optimize)
    {
      // make a list of the triangular walls on the screen
      vector<tri *> onscreentris;

      SDL_FRect obj; // = {(floor((x -fcamera.x)* fcamera.zoom) , floor((y-fcamera.y - height - XtoZ * z) * fcamera.zoom), ceil(width * fcamera.zoom), ceil(height * fcamera.zoom))};
      // obj.x = (x -fcamera.x)* fcamera.zoom;
      // obj.y = (y-fcamera.y - height - XtoZ * z) * fcamera.zoom;
      // obj.w = width * fcamera.zoom;
      // obj.h = height * fcamera.zoom;

      SDL_FRect cam;
      cam.x = 0;
      cam.y = 0;
      cam.w = g_camera.width;
      cam.h = g_camera.height;

      // only checking for layer 0. come back and hide all triangles above hidden triangles
      for (auto t : g_triangles[0])
      {
        obj.x = t->x;
        obj.y = t->y;
        obj.w = t->width;
        obj.h = t->height;

        obj = transformRect(obj);

        // !!! save the result of this check to not redo it later for the same frame?
        if (RectOverlap(obj, cam))
        {
          onscreentris.push_back(t);
        }
      }

      // sort the cookies based on x and y
      sort(onscreentris.begin(), onscreentris.end(), trisort);

      // see if it works
      M("Debug information here:");
      for (auto t : onscreentris)
      {
        D(t->x);
        D(t->y);
        M("------");
      }
      // SDL_Delay(1000);
      // decide which cookie each one falls under.
      for (long long unsigned i = 0; i < g_fogcookies.size(); i++)
      {
        for (long long unsigned j = 0; j < g_fogcookies[0].size(); j++)
        {
        }
      }

      // if that cookie, and the adjacent cookies, are all turned off, don't render the traingle

      // check which of them are completely hidden behind fog
      for (long long unsigned i = 0; i < g_fogcookies.size(); i++)
      {
        for (long long unsigned j = 0; j < g_fogcookies[0].size(); j++)
        {
        }
      }
    }

    {
      if (g_objective != 0)
      {

        if (!g_objective->tangible)
        {
          g_objective = 0;
        }

        float ox = g_objective->getOriginX();
        float oy = g_objective->getOriginY();
        
        float distToObj = XYWorldDistanceSquared(ox, oy, protag->getOriginX(), protag->getOriginY());
        // update crosshair to current objective
        //
        
        float crossx = 0;
        float crossy = 0;

        // hide crosshair if we are close
        if(distToObj < pow(64*5.5,2))
        {
          crossx = 5;
          crossy = 5;
        } else {
          //crosshair should point to the object
          float angleToObj = atan2(ox - protag->getOriginX(), oy - protag->getOriginY());
          angleToObj += M_PI/2;
          float magnitude = 0.43;
          crossx = 0.5;
          crossy = 0.5;
          float w = WIN_WIDTH;
          float h = WIN_HEIGHT;

          //Since the camera is angled, a world block appears wider than it is tall
          //And so I want the reticles to travel around an elipse rather than a sphere
          //it's not perfectly simple to accomodate for this here, though
          //Let's do math to find the difference between the radius of a circle
          //and of an elipse
          
          float a = YtoX; //this ellipse has the same dimensional ratio as an image of a block in the world
          float b = 1;
          float ellipseRadius = (a * b) / ( pow( (pow(a,2) * pow(sin(angleToObj),2) + pow(b,2) * pow(cos(angleToObj),2)  ) , 0.5) );
          magnitude *=ellipseRadius;

          crossx += (-cos(angleToObj) * magnitude) * h/w;
          crossy += sin(angleToObj) * magnitude;
        }



        adventureUIManager->crosshair->x = crossx - adventureUIManager->crosshair->width / 2;
        adventureUIManager->crosshair->y = crossy - adventureUIManager->crosshair->height;
      }
    }


    { //behemoth ui
      if(g_behemoth0 != nullptr && g_behemoth0->tangible) {
        adventureUIManager->b0_element->show = 1;
        
        float ox = g_behemoth0->getOriginX();
        float oy = g_behemoth0->getOriginY();
        
        float distToObj = XYWorldDistanceSquared(ox, oy, protag->getOriginX(), protag->getOriginY());
        // update crosshair to current objective
        //
        
        float crossx = 0;
        float crossy = 0;

        // hide crosshair if we are close
        if(distToObj < pow(64*5.5,2))
        {
          crossx = 5;
          crossy = 5;
        } else {
          //crosshair should point to the object
          float angleToObj = atan2(ox - protag->getOriginX(), oy - protag->getOriginY());
          angleToObj += M_PI/2;
          float magnitude = 0.43;

          crossx = 0.5;
          crossy = 0.5;
          float w = WIN_WIDTH;
          float h = WIN_HEIGHT;
          
          //Since the camera is angled, a world block appears wider than it is tall
          //And so I want the reticles to travel around an elipse rather than a sphere
          //it's not perfectly simple to accomodate for this here, though
          //Let's do math to find the difference between the radius of a circle
          //and of an elipse
          
          float a = YtoX; //this ellipse has the same dimensional ratio as an image of a block in the world
          float b = 1;
          float ellipseRadius = (a * b) / ( pow( (pow(a,2) * pow(sin(angleToObj),2) + pow(b,2) * pow(cos(angleToObj),2)  ) , 0.5) );
          magnitude *=ellipseRadius;

          crossx += (-cos(angleToObj) * magnitude) * h/w;
          crossy += sin(angleToObj) * magnitude;
        }



        adventureUIManager->b0_element->x = crossx - adventureUIManager->crosshair->width / 2;
        adventureUIManager->b0_element->y = crossy - adventureUIManager->crosshair->height;

      } else {
        adventureUIManager->b0_element->show = 0;
      }
      if(g_behemoth1 != nullptr && g_behemoth1->tangible) {
        adventureUIManager->b1_element->show = 1;
        
        float ox = g_behemoth1->getOriginX();
        float oy = g_behemoth1->getOriginY();
        
        float distToObj = XYWorldDistanceSquared(ox, oy, protag->getOriginX(), protag->getOriginY());
        // update crosshair to current objective
        //
        
        float crossx = 0;
        float crossy = 0;

        // hide crosshair if we are close
        if(distToObj < pow(64*5.5,2))
        {
          crossx = 5;
          crossy = 5;
        } else {
          //crosshair should point to the object
          float angleToObj = atan2(ox - protag->getOriginX(), oy - protag->getOriginY());
          angleToObj += M_PI/2;
          float magnitude = 0.43;
          crossx = 0.5;
          crossy = 0.5;
          float w = WIN_WIDTH;
          float h = WIN_HEIGHT;

          //Since the camera is angled, a world block appears wider than it is tall
          //And so I want the reticles to travel around an elipse rather than a sphere
          //it's not perfectly simple to accomodate for this here, though
          //Let's do math to find the difference between the radius of a circle
          //and of an elipse
          
          float a = YtoX; //this ellipse has the same dimensional ratio as an image of a block in the world
          float b = 1;
          float ellipseRadius = (a * b) / ( pow( (pow(a,2) * pow(sin(angleToObj),2) + pow(b,2) * pow(cos(angleToObj),2)  ) , 0.5) );
          magnitude *=ellipseRadius;

          crossx += (-cos(angleToObj) * magnitude) * h/w;
          crossy += sin(angleToObj) * magnitude;
        }



        adventureUIManager->b1_element->x = crossx - adventureUIManager->crosshair->width / 2;
        adventureUIManager->b1_element->y = crossy - adventureUIManager->crosshair->height;

      } else {
        adventureUIManager->b1_element->show = 0;
      }


      if(g_behemoth2 != nullptr && g_behemoth2->tangible) {
        adventureUIManager->b2_element->show = 1;
        
        float ox = g_behemoth2->getOriginX();
        float oy = g_behemoth2->getOriginY();
        
        float distToObj = XYWorldDistanceSquared(ox, oy, protag->getOriginX(), protag->getOriginY());
        // update crosshair to current objective
        //
        
        float crossx = 0;
        float crossy = 0;

        // hide crosshair if we are close
        if(distToObj < pow(64*5.5,2))
        {
          crossx = 5;
          crossy = 5;
        } else {
          //crosshair should point to the object
          float angleToObj = atan2(ox - protag->getOriginX(), oy - protag->getOriginY());
          angleToObj += M_PI/2;
          float magnitude = 0.43;
          crossx = 0.5;
          crossy = 0.5;
          float w = WIN_WIDTH;
          float h = WIN_HEIGHT;

          //Since the camera is angled, a world block appears wider than it is tall
          //And so I want the reticles to travel around an elipse rather than a sphere
          //it's not perfectly simple to accomodate for this here, though
          //Let's do math to find the difference between the radius of a circle
          //and of an elipse
          
          float a = YtoX; //this ellipse has the same dimensional ratio as an image of a block in the world
          float b = 1;
          float ellipseRadius = (a * b) / ( pow( (pow(a,2) * pow(sin(angleToObj),2) + pow(b,2) * pow(cos(angleToObj),2)  ) , 0.5) );
          magnitude *=ellipseRadius;

          crossx += (-cos(angleToObj) * magnitude) * h/w;
          crossy += sin(angleToObj) * magnitude;
        }



        adventureUIManager->b2_element->x = crossx - adventureUIManager->crosshair->width / 2;
        adventureUIManager->b2_element->y = crossy - adventureUIManager->crosshair->height;

      } else {
        adventureUIManager->b2_element->show = 0;
      }


      if(g_behemoth3 != nullptr && g_behemoth3->tangible) {
        adventureUIManager->b3_element->show = 1;

        
        float ox = g_behemoth3->getOriginX();
        float oy = g_behemoth3->getOriginY();
        
        float distToObj = XYWorldDistanceSquared(ox, oy, protag->getOriginX(), protag->getOriginY());
        // update crosshair to current objective
        //
        
        float crossx = 0;
        float crossy = 0;

        // hide crosshair if we are close
        if(distToObj < pow(64*5.5,2))
        {
          crossx = 5;
          crossy = 5;
        } else {
          //crosshair should point to the object
          float angleToObj = atan2(ox - protag->getOriginX(), oy - protag->getOriginY());
          angleToObj += M_PI/2;
          float magnitude = 0.43;
          crossx = 0.5;
          crossy = 0.5;
          float w = WIN_WIDTH;
          float h = WIN_HEIGHT;

          //Since the camera is angled, a world block appears wider than it is tall
          //And so I want the reticles to travel around an elipse rather than a sphere
          //it's not perfectly simple to accomodate for this here, though
          //Let's do math to find the difference between the radius of a circle
          //and of an elipse
          
          float a = YtoX; //this ellipse has the same dimensional ratio as an image of a block in the world
          float b = 1;
          float ellipseRadius = (a * b) / ( pow( (pow(a,2) * pow(sin(angleToObj),2) + pow(b,2) * pow(cos(angleToObj),2)  ) , 0.5) );
          magnitude *=ellipseRadius;

          crossx += (-cos(angleToObj) * magnitude) * h/w;
          crossy += sin(angleToObj) * magnitude;
        }



        adventureUIManager->b3_element->x = crossx - adventureUIManager->crosshair->width / 2;
        adventureUIManager->b3_element->y = crossy - adventureUIManager->crosshair->height;

      } else {
        adventureUIManager->b3_element->show = 0;
      }

    }

    //check if protag is within hearing-range of any behemoths
    adventureUIManager->hearingDetectable->show = 0;
    for(auto x : g_behemoths) {
      float hearingRadiusSquared = pow(x->hearingRadius, 2);
      float distToProtagSquared = XYWorldDistanceSquared(x->getOriginX(), x->getOriginY(), protag->getOriginX(), protag->getOriginY());
//      D(distToProtagSquared);
//      D(hearingRadiusSquared);

      x->hearsPotentialTarget = 0;
      if(distToProtagSquared <= hearingRadiusSquared) {
        adventureUIManager->hearingDetectable->show = 1;
        g_protagIsInHearingRange = 1;
        x->aggressiveness += elapsed * x->aggressivenessNoiseGain;
        x->hearsPotentialTarget = 1;
        break;

      } else {
        g_protagIsInHearingRange = 0;
      }

    }

    //should we should the visionDetectable?
    adventureUIManager->seeingDetectable->show = 0;
    if(g_protagIsBeingDetectedBySight) {
      adventureUIManager->seeingDetectable->show = 1;
      adventureUIManager->hearingDetectable->show = 0;
    }

    //update cooldown indicator
    if(g_backpack.size() > 0){
      adventureUIManager->cooldownIndicator->show = 1;
      auto x = g_backpack.at(g_backpackIndex);
      float percentage = (float)x->cooldownMs / ((float)x->maxCooldownMs);
      percentage *= 48;
      bool useReady = 0;
      if(percentage <= 0) { useReady = 1;}
      int frame = 48 - round(percentage);
      adventureUIManager->cooldownIndicator->frame = frame;

      if(adventureUIManager->cooldownIndicator->frame >= adventureUIManager->cooldownIndicator->xframes) {
        adventureUIManager->cooldownIndicator->frame = adventureUIManager->cooldownIndicator->xframes - 1;

      } else {
        if(adventureUIManager->cooldownIndicator->frame < 0) {
          adventureUIManager->cooldownIndicator->frame = 0;
        }

      }
      if(useReady) {
        adventureUIManager->cooldownIndicator->frame = 24;
        adventureUIManager->cooldownIndicator->show = 0;
      }

      

    } else {
      adventureUIManager->cooldownIndicator->show = 0;

    }

    //!!! remove this before shipping
    for(auto u : g_navNodes) {
      u->costFromUsage = 0;
      u->highlighted = 0;
    }
    for(auto x: g_ai) {
      if(x->current != nullptr) {
        x->current->highlighted = 1;
      }
    }


    //do global nav calcs (shared intelligence for behemoths)
    if(protag != nullptr) {
     
      navCalcMs += elapsed;
      if(navCalcMs > maxNavCalcMs) {
        navCalcMs = 0;
        //precedeProtagNode = Get_Closest_Node(g_navNodes, protag->x, protag->y);


        //!!! remove this before shipping
        for(auto u : g_navNodes) {
          u->costFromUsage = 0;
        }



        for(auto x : g_entities) {
          if(x->aiIndex > 0 && x->customMovement == 0) {
            x->timeSinceLastDijkstra = -1; //force a dijkstra update
            //navNode* zombieNode = x->Get_Closest_Node(g_navNodes);
            //navNode* zombieNode = Get_Closest_Node(g_navNodes, x->getOriginX() + 15*x->xvel, x->getOriginY() + 15*x->yvel);
            //zombieNode->costFromUsage = 10000;
//            for(auto u : zombieNode->friends) {
//              u->costFromUsage = 10000;
//              for(auto y : u->friends) {
//                y->costFromUsage = 10000;
//              }
//            }
          }
        }

      }

    }

    // tiles
    for (long long unsigned int i = 0; i < g_tiles.size(); i++)
    {
      if (g_tiles[i]->z == 0)
      {
        g_tiles[i]->render(renderer, g_camera);
      }
    }

    for (long long unsigned int i = 0; i < g_tiles.size(); i++)
    {
      if (g_tiles[i]->z == 1)
      {
        g_tiles[i]->render(renderer, g_camera);
      }
    }

    // SDL_Rect FoWrect;

    // update particles
    for (auto x : g_particles)
    {
      x->update(elapsed, g_camera);
    }

    // delete old particles
    for (int i = 0; i < g_particles.size(); i++)
    {
      if (g_particles[i]->lifetime < 0)
      {
        if(g_particles[i]->type->disappearMethod == 0) {
          //shrink
          g_particles[i]->deltasizex = -1;
          g_particles[i]->deltasizey = -1;
        } else {
          //fade
          g_particles[i]->deltaAlpha = -60;
        }
      }
    }

    for (int i = 0; i < g_particles.size(); i++)
    {
      if(g_particles[i]->width <= 0 || g_particles[i]->curAlpha <= 0) {
        delete g_particles[i];
        i--;
      }
    }

    //emitters emit particles
    for(auto e : g_emitters) {

      //check interval ms
      if(e->currentIntervalMs <= 0) {
        e->type->happen(e->parent->getOriginX() + e->xoffset, e->parent->getOriginY() + e->yoffset, e->parent->z + e->zoffset, 0);
        e->currentIntervalMs = e->maxIntervalMs;
      }
      e->currentIntervalMs -= elapsed;

      
      if(e->timeToLiveMs != 0) {
        if(e->timeToLiveMs - elapsed <= 0) {
          delete e;
        } else {
          e->timeToLiveMs -= elapsed;
        }
      }

   

    }

    // sort
    sort_by_y(g_actors);
    for (long long unsigned int i = 0; i < g_actors.size(); i++)
    {
      g_actors[i]->render(renderer, g_camera);
    }

    for (long long unsigned int i = 0; i < g_tiles.size(); i++)
    {
      if (g_tiles[i]->z == 2)
      {
        g_tiles[i]->render(renderer, g_camera);
      }
    }

    // map editing
    if (devMode)
    {

      nodeInfoText->textcolor = {0, 0, 0};

      // draw nodes
      for (long long unsigned int i = 0; i < g_worldsounds.size(); i++)
      {
        SDL_Rect obj = {(int)((g_worldsounds[i]->x - g_camera.x - 20) * g_camera.zoom), (int)(((g_worldsounds[i]->y - g_camera.y - 20) * g_camera.zoom)), (int)((40 * g_camera.zoom)), (int)((40 * g_camera.zoom))};
        SDL_RenderCopy(renderer, worldsoundIcon->texture, NULL, &obj);

        SDL_Rect textrect = {(int)(obj.x), (int)(obj.y + 20), (int)(obj.w - 15), (int)(obj.h - 15)};

        //SDL_Surface *textsurface = TTF_RenderText_Blended_Wrapped(nodeInfoText->font, g_worldsounds[i]->name.c_str(), {15, 15, 15}, 1 * WIN_WIDTH);
//        SDL_Texture *texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);
//
//        SDL_RenderCopy(renderer, texttexture, NULL, &textrect);
//
//        SDL_FreeSurface(textsurface);
//        SDL_DestroyTexture(texttexture);

        nodeInfoText->x = obj.x;
        nodeInfoText->y = obj.y - 20;
        nodeInfoText->updateText(g_worldsounds[i]->name, -1, 15);
        nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      }


      //draw precede node(s)
      if(precedeProtagNode != nullptr) {
        SDL_Rect obj = { precedeProtagNode->x, precedeProtagNode->y, 40, 40};

        obj = transformRect(obj);
        SDL_RenderCopy(renderer, worldsoundIcon->texture, NULL, &obj);
      }

      for (long long unsigned int i = 0; i < g_musicNodes.size(); i++)
      {
        SDL_Rect obj = {(int)((g_musicNodes[i]->x - g_camera.x - 20) * g_camera.zoom), (int)(((g_musicNodes[i]->y - g_camera.y - 20) * g_camera.zoom)), (int)((40 * g_camera.zoom)), (int)((40 * g_camera.zoom))};
        SDL_RenderCopy(renderer, musicIcon->texture, NULL, &obj);

        SDL_Rect textrect = {(int)obj.x, (int)(obj.y + 20), (int)(obj.w - 15), (int)(obj.h - 15)};

        SDL_Surface *textsurface = TTF_RenderText_Blended_Wrapped(nodeInfoText->font, g_musicNodes[i]->name.c_str(), {15, 15, 15}, 1 * WIN_WIDTH);
        SDL_Texture *texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);

        SDL_RenderCopy(renderer, texttexture, NULL, &textrect);

        SDL_FreeSurface(textsurface);
        SDL_DestroyTexture(texttexture);
      }

      for (long long unsigned int i = 0; i < g_cueSounds.size(); i++)
      {
        SDL_Rect obj = {(int)((g_cueSounds[i]->x - g_camera.x - 20) * g_camera.zoom), (int)(((g_cueSounds[i]->y - g_camera.y - 20) * g_camera.zoom)), (int)((40 * g_camera.zoom)), (int)((40 * g_camera.zoom))};
        SDL_RenderCopy(renderer, cueIcon->texture, NULL, &obj);
        SDL_Rect textrect = {(int)obj.x, (int)(obj.y + 20), (int)(obj.w - 15), (int)(obj.h - 15)};

        SDL_Surface *textsurface = TTF_RenderText_Blended_Wrapped(nodeInfoText->font, g_cueSounds[i]->name.c_str(), {15, 15, 15}, 1 * WIN_WIDTH);
        SDL_Texture *texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);

        SDL_RenderCopy(renderer, texttexture, NULL, &textrect);

        SDL_FreeSurface(textsurface);
        SDL_DestroyTexture(texttexture);
      }

      for (long long unsigned int i = 0; i < g_waypoints.size(); i++)
      {
        if(!drawhitboxes) {break;}
        SDL_Rect obj = {(int)((g_waypoints[i]->x - g_camera.x - 20) * g_camera.zoom), (int)(((g_waypoints[i]->y - 20 - g_camera.y - g_waypoints[i]->z * XtoZ) * g_camera.zoom)), (int)((40 * g_camera.zoom)), (int)((40 * g_camera.zoom))};
        SDL_RenderCopy(renderer, waypointIcon->texture, NULL, &obj);
        SDL_Rect textrect = {(int)obj.x, (int)(obj.y + 20), (int)(obj.w - 15), (int)(obj.h - 15)};

        SDL_Surface *textsurface = TTF_RenderText_Blended_Wrapped(nodeInfoText->font, g_waypoints[i]->name.c_str(), {15, 15, 15}, 1 * WIN_WIDTH);
        SDL_Texture *texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);

        SDL_RenderCopy(renderer, texttexture, NULL, &textrect);

        SDL_FreeSurface(textsurface);
        SDL_DestroyTexture(texttexture);
      }

      for (auto x : g_setsOfInterest)
      {
        for (auto y : x)
        {
          SDL_Rect obj = {(int)((y->x - g_camera.x - 20) * g_camera.zoom), (int)((y->y - g_camera.y - 20) * g_camera.zoom), (int)((40 * g_camera.zoom)), (int)((40 * g_camera.zoom))};
          SDL_RenderCopy(renderer, poiIcon->texture, NULL, &obj);

          SDL_Rect textrect = {(int)obj.x, (int)(obj.y + 20), (int)(obj.w - 15), (int)(obj.h - 15)};

          SDL_Surface *textsurface = TTF_RenderText_Blended_Wrapped(nodeInfoText->font, to_string(y->index).c_str(), {15, 15, 15}, 1 * WIN_WIDTH);
          SDL_Texture *texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);

          SDL_RenderCopy(renderer, texttexture, NULL, &textrect);

          SDL_FreeSurface(textsurface);
          SDL_DestroyTexture(texttexture);
        }
      }

      // doors
      for (long long unsigned int i = 0; i < g_doors.size(); i++)
      {
        SDL_Rect obj = {(int)((g_doors[i]->x - g_camera.x) * g_camera.zoom), (int)(((g_doors[i]->y - g_camera.y - (g_doors[i]->zeight) * XtoZ) * g_camera.zoom)), (int)((g_doors[i]->width * g_camera.zoom)), (int)((g_doors[i]->height * g_camera.zoom))};
        SDL_RenderCopy(renderer, doorIcon->texture, NULL, &obj);
        // the wall
        SDL_Rect obj2 = {(int)((g_doors[i]->x - g_camera.x) * g_camera.zoom), (int)(((g_doors[i]->y - g_camera.y - (g_doors[i]->zeight) * XtoZ) * g_camera.zoom)), (int)((g_doors[i]->width * g_camera.zoom)), (int)(((g_doors[i]->zeight - g_doors[i]->z) * XtoZ * g_camera.zoom) + (g_doors[i]->height * g_camera.zoom))};
        SDL_RenderCopy(renderer, doorIcon->texture, NULL, &obj2);
        nodeInfoText->x = obj.x + 25;
        nodeInfoText->y = obj.y + 25;
        nodeInfoText->updateText(g_doors[i]->to_map + "->" + g_doors[i]->to_point, -1, 15);
        nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      }


      for (long long unsigned int i = 0; i < g_triggers.size(); i++)
      {
        SDL_Rect obj = {(int)((g_triggers[i]->x - g_camera.x) * g_camera.zoom), (int)(((g_triggers[i]->y - g_camera.y - (g_triggers[i]->zeight) * XtoZ) * g_camera.zoom)), (int)((g_triggers[i]->width * g_camera.zoom)), (int)((g_triggers[i]->height * g_camera.zoom))};
        SDL_RenderCopy(renderer, triggerIcon->texture, NULL, &obj);
        // the wall
        SDL_Rect obj2 = {(int)((g_triggers[i]->x - g_camera.x) * g_camera.zoom), (int)(((g_triggers[i]->y - g_camera.y - (g_triggers[i]->zeight) * XtoZ) * g_camera.zoom)), (int)((g_triggers[i]->width * g_camera.zoom)), (int)(((g_triggers[i]->zeight - g_triggers[i]->z) * XtoZ * g_camera.zoom) + (g_triggers[i]->height * g_camera.zoom))};
        SDL_RenderCopy(renderer, triggerIcon->texture, NULL, &obj2);

        nodeInfoText->x = obj.x + 25;
        nodeInfoText->y = obj.y + 25;
        nodeInfoText->updateText(g_triggers[i]->binding, -1, 15);
        nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      }

      // listeners
      for (long long unsigned int i = 0; i < g_listeners.size(); i++)
      {
        SDL_Rect obj = {(int)((g_listeners[i]->x - g_camera.x - 20) * g_camera.zoom), (int)((g_listeners[i]->y - g_camera.y - 20) * g_camera.zoom), (int)(40 * g_camera.zoom), (int)(40 * g_camera.zoom)};
        SDL_RenderCopy(renderer, listenerIcon->texture, NULL, &obj);
        nodeInfoText->x = obj.x;
        nodeInfoText->y = obj.y - 20;
        nodeInfoText->updateText(g_listeners[i]->listenList.size() + " of " + g_listeners[i]->entityName, -1, 15);
        nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      }

      write_map(protag);
      for (int i = 0; i < 50; i++)
      {
        devinput[i] = 0;
      }
    }

    if (g_fogofwarEnabled && !devMode)
    {
      // black bars
      SDL_Rect topbar = {px, FoWrect.y - 990, 1500, 1000};
      SDL_RenderCopy(renderer, blackbarTexture, NULL, &topbar);
      SDL_Rect botbar = {px, FoWrect.y + g_fogheight * 55 + 10, 2000, 1000};
      SDL_RenderCopy(renderer, blackbarTexture, NULL, &botbar);

      SDL_Rect leftbar = {px-800, FoWrect.y, 1000, 1500};
      //SDL_RenderCopy(renderer, blackbarTexture, NULL, &leftbar);
      SDL_Rect rightbar = {px + 1100, FoWrect.y, 1000, 1500};
      //SDL_RenderCopy(renderer, blackbarTexture, NULL, &rightbar);

    }

    // ui
    if (!inPauseMenu && g_showHUD)
    {
      // !!! segfaults on mapload sometimes
      
      SDL_Color useThisColor = g_healthtextcolor;
      if(protag->hp < 5) {
        useThisColor = g_healthtextlowcolor;
      }
      adventureUIManager->healthText->updateText(to_string(int(protag->hp)) + '/' + to_string(int(protag->maxhp)), -1, 0.9,  useThisColor);
      adventureUIManager->healthText->show = 1;

      //adventureUIManager->hungerText->updateText(to_string((int)((float)(min(g_foodpoints, g_maxVisibleFoodpoints) * 100) / (float)g_maxVisibleFoodpoints)) + '%', -1, 0.9);
      adventureUIManager->hungerText->show = 0;
   
      //animate the guts sometimes
      //heart shake
//      adventureUIManager->heartShakeIntervalMs -= elapsed;
//      if(adventureUIManager->heartShakeIntervalMs < 0) {
//        //make the heart shake back and forth briefly
//        adventureUIManager->heartShakeDurationMs = adventureUIManager->maxHeartShakeDurationMs;
//
//        adventureUIManager->heartShakeIntervalMs = adventureUIManager->maxHeartShakeIntervalMs + rand() % adventureUIManager->heartShakeIntervalRandomMs;
//      }
//
//      if(adventureUIManager->heartShakeDurationMs > 0) {
//        adventureUIManager->heartShakeDurationMs -= elapsed;
//
//        if(adventureUIManager->heartShakeDurationMs % 400 > 200) {
//          //move left
//          adventureUIManager->healthPicture->targetx = -0.04 - 0.005;
//
//        } else {
//          //move right
//          adventureUIManager->healthPicture->targetx = -0.04 + 0.005;
//
//        }
//
//      } else {
//        //move the heart back to its normal position
//        adventureUIManager->healthPicture->targetx = -0.04;
//
//      }

      //stomach shaking
//      adventureUIManager->stomachShakeIntervalMs -= elapsed;
//      if(adventureUIManager->stomachShakeIntervalMs < 0) {
//        //make the stomach shake back and forth briefly
//        adventureUIManager->stomachShakeDurationMs = adventureUIManager->maxstomachShakeDurationMs;
//
//        float hungerratio = ((float)(min(g_foodpoints, g_maxVisibleFoodpoints) / (float)g_maxVisibleFoodpoints));
//        D(hungerratio);
//
//        //stomach grumbles less when the protag is less hungry
//        adventureUIManager->stomachShakeIntervalMs = adventureUIManager->maxstomachShakeIntervalMs * hungerratio + rand() % adventureUIManager->stomachShakeIntervalRandomMs;
//      }

//      if(adventureUIManager->stomachShakeDurationMs > 0) {
//        adventureUIManager->stomachShakeDurationMs -= elapsed;
//
//        if(adventureUIManager->stomachShakeDurationMs % 400 > 200) {
//          //move left
//          adventureUIManager->hungerPicture->targetx = 0.8 - 0.005;
//
//        } else {
//          //move right
//          adventureUIManager->hungerPicture->targetx = 0.8 + 0.005;
//
//        }
//
//      } else {
//        //move the stomach back to its normal position
//        adventureUIManager->hungerPicture->targetx = 0.8;
//
//      }

//      //tongue swallowing
//      adventureUIManager->tungShakeIntervalMs -= elapsed;
//      if(adventureUIManager->tungShakeIntervalMs < 0) {
//        //make the tung shake back and forth briefly
//        adventureUIManager->tungShakeDurationMs = adventureUIManager->maxTungShakeDurationMs;
//
//        adventureUIManager->tungShakeIntervalMs = adventureUIManager->maxTungShakeIntervalMs + rand() % adventureUIManager->tungShakeIntervalRandomMs;
//      }
//
//      if(adventureUIManager->tungShakeDurationMs > 0) {
//        adventureUIManager->tungShakeDurationMs -= elapsed;
//
//        //move tung up
//        adventureUIManager->tastePicture->targety = 0.75 - 0.01;
//        adventureUIManager->tastePicture->targetx = 0 - 0.01;
//
//      } else {
//        //move the tung back to its normal position
//        adventureUIManager->tastePicture->targetx = 0;
//        adventureUIManager->tastePicture->targety = 0.75;
//
//      }

      //heart beating
      adventureUIManager->heartbeatDurationMs -= elapsed;
      if(adventureUIManager->heartbeatDurationMs > 200) {
          //expand
          adventureUIManager->healthPicture->targetwidth = 0.25;

      } else {
          //contract
          adventureUIManager->healthPicture->targetwidth = 0.25 - adventureUIManager->heartShrinkPercent;

      }
      if(adventureUIManager->heartbeatDurationMs < 0) {
        adventureUIManager->heartbeatDurationMs = (adventureUIManager->maxHeartbeatDurationMs - 300) * ((float)protag->hp / (float)protag->maxhp) + 300;
        float hpratio = ((float)protag->hp / (float)protag->maxhp);
        if(hpratio < 0.6) {
        adventureUIManager->healthPicture->widthGlideSpeed = 0.1 + min(0.2, 0.3 *((float)protag->hp / (float)protag->maxhp));
        } else {
          adventureUIManager->healthPicture->widthGlideSpeed = 0.1;

        }
      }


    }
    else
    {
      adventureUIManager->healthText->show = 0;
      adventureUIManager->hungerText->show = 0;
    }

    //get hungry
//    if(g_currentFoodpointsDecreaseIntervalMs < 0) {
//      g_currentFoodpointsDecreaseIntervalMs = g_foodpointsDecreaseIntervalMs;
//      g_foodpoints -= g_foodpointsDecreaseAmount;
//      if(g_foodpoints < 0) { g_foodpoints = 0; }
//    }
//    g_currentFoodpointsDecreaseIntervalMs -= elapsed;

    // move the healthbar properly to the protagonist
    //rect obj; // = {( , (((protag->y - ((protag->height))) - protag->z * XtoZ) - g_camera.y) * g_camera.zoom, (protag->width * g_camera.zoom), (protag->height * g_camera.zoom))};
//    obj.x = ((protag->x - g_camera.x) * g_camera.zoom);
//    obj.y = (((protag->y - ((floor(protag->height) * 0.9))) - protag->z * XtoZ) - g_camera.y) * g_camera.zoom;
//    obj.width = (protag->width * g_camera.zoom);
//    obj.height = (floor(protag->height) * g_camera.zoom);
//
//    protagHealthbarA->x = (((float)obj.x + obj.width / 2) / (float)WIN_WIDTH) - protagHealthbarA->width / 2.0;
//    protagHealthbarA->y = ((float)obj.y) / (float)WIN_HEIGHT;
//    protagHealthbarB->x = protagHealthbarA->x;
//    protagHealthbarB->y = protagHealthbarA->y;
//
//    protagHealthbarC->x = protagHealthbarA->x;
//    protagHealthbarC->y = protagHealthbarA->y;
//    protagHealthbarC->width = (protag->hp / protag->maxhp) * 0.05;
//    adventureUIManager->healthText->boxX = protagHealthbarA->x + protagHealthbarA->width / 2;
//    adventureUIManager->healthText->boxY = protagHealthbarA->y - 0.005;

    //bottom-most layer of ui
    for (long long unsigned int i = 0; i < g_ui.size(); i++)
    {
      if(g_ui[i]->layer0) {
        g_ui[i]->render(renderer, g_camera, elapsed);
      }
    }

    for (long long unsigned int i = 0; i < g_textboxes.size(); i++)
    {
      if(g_textboxes[i]->layer0) {
        g_textboxes[i]->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      }
    }

    for (long long unsigned int i = 0; i < g_ui.size(); i++)
    {
      if(!g_ui[i]->renderOverText && !g_ui[i]->layer0) {
        g_ui[i]->render(renderer, g_camera, elapsed);
      }
    }

    for (long long unsigned int i = 0; i < g_textboxes.size(); i++)
    {
      if(!g_textboxes[i]->layer0) {
        g_textboxes[i]->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      }
    }

    //some ui are rendered over text
    for (long long unsigned int i = 0; i < g_ui.size(); i++)
    {
      if(g_ui[i]->renderOverText) {
        g_ui[i]->render(renderer, g_camera, elapsed);
      }
    }

    //render fancybox
    //g_fancybox->render();

    // settings menu
    if (g_inSettingsMenu) 
    {
      //move reticle to the correct position
      if(!g_settingsUI->cursorIsOnBackButton) {
        g_settingsUI->handMarker->targety
          = g_settingsUI->optionTextboxes[g_settingsUI->positionOfCursor]->boxY
          + (g_settingsUI->handOffset);

        g_settingsUI->handMarker->targetx
          = g_settingsUI->markerHandX;

        g_settingsUI->fingerMarker->targety
          = g_settingsUI->optionTextboxes[g_settingsUI->positionOfCursor]->boxY
          + (g_settingsUI->fingerOffset);

        g_settingsUI->fingerMarker->targetx
          = g_settingsUI->markerFingerX;


      } else {
        float ww = WIN_WIDTH;
        float wh = WIN_HEIGHT;

        g_settingsUI->fingerMarker->targetx = g_settingsUI->bbNinePatch->x + (g_settingsUI->markerBBOffset);
        g_settingsUI->fingerMarker->targety = g_settingsUI->bbNinePatch->y + (g_settingsUI->markerBBOffsetY * (ww/wh));

        g_settingsUI->handMarker->targetx = g_settingsUI->bbNinePatch->x + (g_settingsUI->markerBBOffset);
        g_settingsUI->handMarker->targety = g_settingsUI->bbNinePatch->y + (g_settingsUI->markerBBOffsetY * (ww/wh));
      }

      if(g_firstFrameOfSettingsMenu) {
        g_firstFrameOfSettingsMenu = 0;
        g_settingsUI->handMarker->x = g_settingsUI->handMarker->targetx;
        g_settingsUI->handMarker->y = g_settingsUI->handMarker->targety;
        g_settingsUI->fingerMarker->x = g_settingsUI->fingerMarker->targetx;
        g_settingsUI->fingerMarker->y = g_settingsUI->fingerMarker->targety;

      }

    }

    //this is the menu for quitting or going back to the "overworld"
    if (g_inEscapeMenu) 
    {
      //move reticle to the correct position
      g_escapeUI->handMarker->targety
        = g_escapeUI->optionTextboxes[g_escapeUI->positionOfCursor]->boxY
        + (g_escapeUI->handOffset);

      g_escapeUI->handMarker->targetx
        = g_escapeUI->markerHandX;

      g_escapeUI->fingerMarker->targety
        = g_escapeUI->optionTextboxes[g_escapeUI->positionOfCursor]->boxY
        + (g_escapeUI->fingerOffset);

      float ww = WIN_WIDTH;
      float fwidth = g_escapeUI->optionTextboxes[g_escapeUI->positionOfCursor]->width;
      g_escapeUI->fingerMarker->targetx
        = g_escapeUI->optionTextboxes[g_escapeUI->positionOfCursor]->boxX + 
          fwidth / ww / 2;
      


      if(g_firstFrameOfSettingsMenu) {
        g_firstFrameOfSettingsMenu = 0;
        g_escapeUI->handMarker->x = g_escapeUI->handMarker->targetx;
        g_escapeUI->handMarker->y = g_escapeUI->handMarker->targety;
        g_escapeUI->fingerMarker->x = g_escapeUI->fingerMarker->targetx;
        g_escapeUI->fingerMarker->y = g_escapeUI->fingerMarker->targety;

      }

    }

    // draw pause screen
    if (inPauseMenu)
    {
      adventureUIManager->crosshair->x = 5;

      // iterate thru inventory and draw items on screen
      float defaultX = WIN_WIDTH * 0.05;
      float defaultY = WIN_HEIGHT * adventureUIManager->inventoryYStart;
      float x = defaultX;
      float y = defaultY;
      float maxX = WIN_WIDTH * 0.9;
      float maxY = WIN_HEIGHT * adventureUIManager->inventoryYEnd;
      float itemWidth = WIN_WIDTH * 0.07;
      float padding = WIN_WIDTH * 0.01;

      int i = 0;

      if (g_inventoryUiIsLevelSelect == 0) {
        if(g_inventoryUiIsKeyboard == 1) {
          //draw a letter in each box and append to a string


          for(int j = 0; j < g_alphabet.size(); j++) {
            if( i < itemsPerRow * inventoryScroll) {
              i++;
              continue;
            }

            SDL_Rect drect;
            if(g_alphabet == g_alphabet_lower) {
              drect = {(int)x + (0.02 * WIN_WIDTH) - (g_alphabet_widths[i] * itemWidth/230) , (int)y, (int)itemWidth * (g_alphabet_widths[i] / 60), (int)itemWidth}; 
            } else {
              drect = {(int)x + (0.02 * WIN_WIDTH) - (g_alphabet_widths[i] * itemWidth/230), (int)y, (int)itemWidth * (g_alphabet_upper_widths[i] / 60), (int)itemWidth}; 
            }
              
            // draw the ith letter of "alphabet" in drect
            if(1) {
              SDL_Rect shadowRect = drect;
              float booshAmount = g_textDropShadowDist  * (60 * g_fontsize);
              shadowRect.x += booshAmount;
              shadowRect.y += booshAmount;
              SDL_SetTextureColorMod(g_alphabet_textures->at(i), g_textDropShadowColor,g_textDropShadowColor,g_textDropShadowColor);
              SDL_RenderCopy(renderer, g_alphabet_textures->at(i), NULL, &shadowRect);
              SDL_SetTextureColorMod(g_alphabet_textures->at(i), 255,255,255);
            }
            SDL_RenderCopy(renderer, g_alphabet_textures->at(i), NULL, &drect);
  

            if (i == inventorySelection || g_firstFrameOfPauseMenu)
            {
            // this item should have the marker
            inventoryMarker->show = 1;
            float biggen = 0; // !!! resolutions : might have problems with diff resolutions
                                 
            if(g_firstFrameOfPauseMenu) {
              inventoryMarker->x = x / WIN_WIDTH;
              inventoryMarker->y = y / WIN_HEIGHT;
              inventoryMarker->x -= biggen;
              inventoryMarker->y -= biggen * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
              //now that it's a hand
              inventoryMarker->x += 0.015 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
              inventoryMarker->y += 0.03 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
              inventoryMarker->targetx = inventoryMarker->x;
              inventoryMarker->targety = inventoryMarker->y;
              g_firstFrameOfPauseMenu = 0;
            } else {
              inventoryMarker->targetx = x / WIN_WIDTH;
              inventoryMarker->targety = y / WIN_HEIGHT;
              inventoryMarker->targetx -= biggen;
              inventoryMarker->targety -= biggen * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
              //now that it's a hand
              inventoryMarker->targetx += 0.015 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
              inventoryMarker->targety += 0.03 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
            }

            inventoryMarker->width = itemWidth / WIN_WIDTH;
  
            inventoryMarker->width += biggen * 2;
            //inventoryMarker->height = inventoryMarker->width * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
            }
    
            x += itemWidth + padding;
            if (x > maxX)
            {
              x = defaultX;
              y += itemWidth + padding;
              if (y > maxY)
              {
                // we filled up the entire inventory, so lets leave
                break;
              }
            }
            i++;
  
          }

          //draw current input in the bottom box
          adventureUIManager->inputText->updateText(g_keyboardInput.c_str(), -1, 0.9);
          adventureUIManager->escText->updateText(adventureUIManager->keyboardPrompt, -1, 0.9);
          

          g_itemsInInventory = g_alphabet.size();


        } else {
          //populate boxes based on inventory
          for (auto it = mainProtag->inventory.rbegin(); it != mainProtag->inventory.rend(); ++it)
          {
    
            if (i < itemsPerRow * inventoryScroll)
            {
              // this item won't be rendered
              i++;
              continue;
            }
    
            SDL_Rect drect = {(int)x, (int)y, (int)itemWidth, (int)itemWidth};
            if (it->second > 0)
            {
              SDL_RenderCopy(renderer, it->first->texture, NULL, &drect);
            }
            // draw number
            if (it->second > 1)
            {
              inventoryText->show = 1;
              inventoryText->updateText(to_string(it->second), -1, 100);
              inventoryText->boxX = (x + (itemWidth * 0.8)) / WIN_WIDTH;
              inventoryText->boxY = (y + (itemWidth - inventoryText->boxHeight / 2) * 0.6) / WIN_HEIGHT;
              inventoryText->worldspace = 1;
              inventoryText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
            }
            else
            {
              inventoryText->show = 0;
            }
    
            if (i == inventorySelection || g_firstFrameOfPauseMenu)
            {
              // this item should have the marker
              inventoryMarker->show = 1;

              float biggen = 0.01; // !!! resolutions : might have problems with diff resolutions
              if(g_firstFrameOfPauseMenu) {
                inventoryMarker->x = x / WIN_WIDTH;
                inventoryMarker->y = y / WIN_HEIGHT;
                inventoryMarker->x -= biggen;
                inventoryMarker->y -= biggen * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
                //now that it's a hand
                inventoryMarker->x += 0.015 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
                inventoryMarker->y += 0.03 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
                inventoryMarker->targetx = inventoryMarker->x;
                inventoryMarker->targety = inventoryMarker->y;
                g_firstFrameOfPauseMenu = 0;
              } else {
                inventoryMarker->targetx = x / WIN_WIDTH;
                inventoryMarker->targety = y / WIN_HEIGHT;
                inventoryMarker->targetx -= biggen;
                inventoryMarker->targety -= biggen * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
                //now that it's a hand
                inventoryMarker->targetx += 0.015 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
                inventoryMarker->targety += 0.03 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
              }

              inventoryMarker->width = itemWidth / WIN_WIDTH;
              inventoryMarker->width += biggen * 2;
              inventoryMarker->height = inventoryMarker->width * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
            }
    
            x += itemWidth + padding;
            if (x > maxX)
            {
              x = defaultX;
              y += itemWidth + padding;
              if (y > maxY)
              {
                // we filled up the entire inventory, so lets leave
                break;
              }
            }
            i++;
          }
          g_itemsInInventory = mainProtag->inventory.size();
    
          if (mainProtag->inventory.size() > 0 && mainProtag->inventory.size() - 1 - inventorySelection < mainProtag->inventory.size())
          {
            string description = mainProtag->inventory[mainProtag->inventory.size() - 1 - inventorySelection].first->script[0];
            // first line is a comment so take off the //
            description = description.substr(2);
            adventureUIManager->escText->updateText(description, -1, 0.9);
          }
          else
          {
            adventureUIManager->escText->updateText("No items in inventory", -1, 0.9);
          }
        }
      } else {
          //populate the UI based on the loaded level sequence.
          for(int j = 0; j < g_levelSequence->levelNodes.size(); j++) {
            if( i < itemsPerRow * inventoryScroll) {
              i++;
              continue;
            }
            SDL_Rect drect = {(int)x, (int)y, (int)itemWidth, (int)itemWidth}; 
  
            //should we draw the locked graphic?
            if(g_levelSequence->levelNodes[j]->locked) {
              SDL_RenderCopy(renderer, g_locked_level_texture, NULL, &drect);
            } else {
              SDL_RenderCopy(renderer, g_levelSequence->levelNodes[j]->sprite, NULL, &drect);
            }
  
            if (i == inventorySelection)
            {
  
              if(g_levelSequence->levelNodes[i]->locked) {
                adventureUIManager->escText->updateText("???", -1, 0.9);
              } else {
                string dispText = g_levelSequence->levelNodes[i]->name;
                std::replace(dispText.begin(), dispText.end(),'_',' ');
                adventureUIManager->escText->updateText(g_levelSequence->levelNodes[i]->name, -1, 0.9);
  
              }
  
              // this item should have the marker
              inventoryMarker->show = 1;
              float biggen = 0.01; // !!! resolutions : might have problems with diff resolutions
                                   
              if(g_firstFrameOfPauseMenu) {
                inventoryMarker->x = x / WIN_WIDTH;
                inventoryMarker->y = y / WIN_HEIGHT;
                inventoryMarker->x -= biggen;
                inventoryMarker->y -= biggen * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
                //now that it's a hand
                inventoryMarker->x += 0.02 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
                inventoryMarker->y += 0.03 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
                inventoryMarker->targetx = inventoryMarker->x;
                inventoryMarker->targety = inventoryMarker->y;
                g_firstFrameOfPauseMenu = 0;
              } else {
                inventoryMarker->targetx = x / WIN_WIDTH;
                inventoryMarker->targety = y / WIN_HEIGHT;
                inventoryMarker->targetx -= biggen;
                inventoryMarker->targety -= biggen * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
                //now that it's a hand
                inventoryMarker->targetx += 0.02 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
                inventoryMarker->targety += 0.03 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
              }

              inventoryMarker->width = itemWidth / WIN_WIDTH;
    
              inventoryMarker->width += biggen * 2;
              inventoryMarker->height = inventoryMarker->width * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
            }
    
            x += itemWidth + padding;
            if (x > maxX)
            {
              x = defaultX;
              y += itemWidth + padding;
              if (y > maxY)
              {
                // we filled up the entire inventory, so lets leave
                break;
              }
            }
            i++;
  
          }
          g_itemsInInventory = g_levelSequence->levelNodes.size();
          
        }
  
        //re-render inventory reticle so it goes on top of the items/level icons
        inventoryMarker->render(renderer, g_camera, 0);
        inventoryMarker->show = 0;
      
    }
    else
    {
      inventoryMarker->show = 0;
      inventoryText->show = 0;
    }

    // sines for item bouncing
    g_elapsed_accumulator += elapsed;
    g_itemsines[0] = ( sin(g_elapsed_accumulator / 300) * 10 + 30);
    g_itemsines[1] = ( sin((g_elapsed_accumulator + (235 * 1) ) / 300) * 10 + 30);
    g_itemsines[2] = ( sin((g_elapsed_accumulator + (235 * 2) ) / 300) * 10 + 30);
    g_itemsines[3] = ( sin((g_elapsed_accumulator + (235 * 3) ) / 300) * 10 + 30);
    g_itemsines[4] = ( sin((g_elapsed_accumulator + (235 * 4) ) / 300) * 10 + 30);
    g_itemsines[5] = ( sin((g_elapsed_accumulator + (235 * 5) ) / 300) * 10 + 30);
    g_itemsines[6] = ( sin((g_elapsed_accumulator + (235 * 6) ) / 300) * 10 + 30);
    g_itemsines[7] = ( sin((g_elapsed_accumulator + (235 * 7) ) / 300) * 10 + 30);

    if (g_elapsed_accumulator > 1800 * M_PI)
    {
      g_elapsed_accumulator -= 1800* M_PI;
    }

    g_protagIsBeingDetectedBySmell = 0; //this will be set in the entity update loop
    g_protagIsBeingDetectedBySight = 0;

    // ENTITY MOVEMENT (ENTITY UPDATE)
    // dont update movement while transitioning
    if (1/*!transition*/)
    {
      for (long long unsigned int i = 0; i < g_entities.size(); i++)
      {
        if (g_entities[i]->isWorlditem || g_entities[i]->identity == 1)
        {
          // make it bounce
          int index = g_entities[i]->bounceindex;
          g_entities[i]->floatheight = g_itemsines[index];
        }
        door *taken = g_entities[i]->update(g_doors, elapsed);
        

        // added the !transition because if a player went into a map with a door located in the same place
        // as they are in the old map (before going to the waypoint) they would instantly take that door
        if (taken != nullptr && !transition)
        {
          // player took this door
          // clear level

          // we will now clear the map, so we will save the door's destination map as a string
          const string savemap = "maps/" + taken->to_map + ".map";
          const string dest_waypoint = taken->to_point;

          // render this frame

          //clear_map(g_camera);
          load_map(renderer, savemap, dest_waypoint);

          // clear_map() will also delete engine tiles, so let's re-load them (but only if the user is map-editing)
          if (canSwitchOffDevMode)
          {
            init_map_writing(renderer);
          }

          break;
        }
 
        if(g_entities[i]->usingTimeToLive) {

          if(g_entities[i]->timeToLiveMs < 0) {
            if(g_entities[i]->dontSave) {
              if(!g_entities[i]->asset_sharer) {
                g_entities[i]->tangible = 0;
                g_entities[i]->usingTimeToLive = 0;
              } else {
                delete g_entities[i];
              }

            } else {

              g_entities[i]->height = 0;
              g_entities[i]->width = 0;
  
              g_entities[i]->shrinking = 1;
  
              if(!g_entities[i]->wasPellet) { 
                g_entities[i]->dynamic = 0;
                g_entities[i]->xvel = 0;
                g_entities[i]->yvel = 0;
                g_entities[i]->missile = 0;
              }
  
  
              if(g_entities[i]->curheight < 1) {
                //remove this entity from it's parent's 
                //list of children
                if(g_entities[i]->isOrbital) {
                  g_entities[i]->parent->children.erase(remove(g_entities[i]->parent->children.begin(), g_entities[i]->parent->children.end(), g_entities[i]), g_entities[i]->parent->children.end());
                  g_entities[i]->isOrbital = 0;
                }
  
  
  
                if(!g_entities[i]->asset_sharer) {
                  g_entities[i]->tangible = 0;
                  g_entities[i]->usingTimeToLive = 0;
                } else {
                  delete g_entities[i];
                }
              }
            }
          } 

        }

      }
    }
    
    
    //did the protag collect a pellet?
    float protag_x = protag->getOriginX();
    float protag_y = protag->getOriginY();
    if(g_showPellets) {
      for(int i = 0; i < g_pellets.size(); i++) {
        entity* x = g_pellets[i];
  
        //well, this is shitty. Somehow I find myself in an unfortunate situation which I dont understand
        //Somehow, I can't get pellets to appear ontopof the lowest fogsheet yet behind fomm, if he's
        //infront.
        //I'm going to cheat and change their sorting offset based on distance to the protag, so they'll still be jank (e.g., other enemy is close to a cupcake), but whatever, it's better than nothin
        //and I really want pellets to stick out from fog
        int ydiff = protag_y - x->y;
        if(-ydiff > -85) {
          x->sortingOffset = 30;
        } else {
          x->sortingOffset = 160; //pellets that are close to the fog are artificially boosted in the draw order
        }
  
        if(ydiff < 85 && ydiff > 0) {
          x->sortingOffset = 30;
        }
        
        
        bool collected = 0;
        //try to collect
        if(devMode == 0) {
          bool m = CylinderOverlap(protag->getMovedBounds(), x->getMovedBounds());
          if(m) {
            //make tung image do swallow animation
            adventureUIManager->tungShakeIntervalMs = 500; //swallow 500ms after eating this
            adventureUIManager->tungShakeDurationMs = 0;
            
            
            playSound(4, g_pelletCollectSound, 0);
            x->usingTimeToLive = 1;
            x->timeToLiveMs = -1;
            x->shadow->size = 0;
            x->dynamic = 1;
            x->steeringAngle = wrapAngle(atan2(protag->getOriginX() - x->getOriginX(), protag->getOriginY() - x->getOriginY()) - M_PI/2);
            x->targetSteeringAngle = x->steeringAngle;
            x->forwardsVelocity = 14000;
  
            
            g_pellets.erase(remove(g_pellets.begin(), g_pellets.end(), x), g_pellets.end());
            x->identity = 0;
            i--;
            g_currentPelletsCollected++;
  
            collected = 1;
            
            //did we collect the objective?
            if(x == g_objective) {
              g_objective = nullptr;
            }
          }
        }
  
        if(collected) {
          for(auto x : g_pelletGoalScripts) {
  
            if(x.first <= g_currentPelletsCollected) {
              g_pelletGoalScripts.erase(remove(g_pelletGoalScripts.begin(), g_pelletGoalScripts.end(), x), g_pelletGoalScripts.end());
              M("Calling goalscript for " + to_string(x.first) + " pellets");
              //M("We'll call this script: " + x.second);
  
              //load and call this script (this code is taken from that for the "/script" script-command
              ifstream stream;
              string loadstr;
              string s = x.second;
          
              loadstr = "maps/" + g_map + "/scripts/" + s + ".txt";
              const char *plik = loadstr.c_str();
  
              stream.open(plik);
          
              if (!stream.is_open())
              {
                stream.open("scripts/" + s + ".txt");
              }
              string line;
          
              getline(stream, line);
          
              vector<string> nscript;
              while (getline(stream, line))
              {
                nscript.push_back(line);
              }
  
              parseScriptForLabels(nscript);
              
              M("About to interpret goalscript.");
  
              g_pelletGoalScriptCaller->blip = g_ui_voice;
              g_pelletGoalScriptCaller->ownScript = nscript;
              g_pelletGoalScriptCaller->dialogue_index = -1;
              g_pelletGoalScriptCaller->talker = g_pelletNarrarator;

              g_pelletGoalScriptCaller->continueDialogue();
  
              M("Finished interpreting goalscript.");
  
            }
  
          }
  
        }
      }
    }

    if(g_objectiveFadeModeOn) {
      if(abs(protag->xvel) > 2 || abs(protag->yvel) > 2) {
        g_objectiveOpacity -= elapsed * 50;
        g_objectiveFadeWaitMs = g_objectiveFadeMaxWaitMs;
      } else {
        g_objectiveFadeWaitMs -= elapsed;
        if(g_objectiveFadeWaitMs < 0) {
          g_objectiveOpacity += elapsed * 50;
        } else {
          g_objectiveOpacity -= elapsed * 50;
        }


      }
      g_objectiveOpacity = min(max(g_objectiveOpacity, 0), 25500);
      
    } else {
      g_objectiveOpacity = 25500;

    }
    
    adventureUIManager->crosshair->opacity = g_objectiveOpacity;

    
    if(g_usingPelletsAsObjective) {
      if(g_objective == nullptr) {
        M("Must find another pellet");

        //search for a nearby pellet
        entity* closest = nullptr;
        float closestDist = 0;
//        int px = protag->getOriginX();
//        int py = protag->getOriginY();
        int px = protag->x;
        int py = protag->y;
        int thisDist = 0;
        for(int i = 0; i < g_pellets.size(); i++) {
          thisDist = XYWorldDistanceSquared(px, py, g_pellets[i]->x, g_pellets[i]->y);
          if(thisDist < closestDist || closest == nullptr) {
            closest = g_pellets[i];
            closestDist = thisDist;
          }
        }
        g_objective = closest; //might be nullptr, that's fine

      }
    }

    string scorePrint = "";
    if(g_currentPelletsCollected < g_pelletsNeededToProgress || 1) {
      scorePrint = to_string(g_currentPelletsCollected) + "/" + to_string(g_pelletsNeededToProgress);
    } else {
      scorePrint = g_leaveStr;
    }

    adventureUIManager->scoreText->updateText(scorePrint, -1, 1);
    adventureUIManager->showScoreUI();

    //system clock display
    string systemTimePrint = "";
    time_t ttime = time(0);
    tm *local_time = localtime(&ttime);
    
    int useHour = local_time->tm_hour;
    if(useHour == 0) {useHour = 12;}
    string useMinString = to_string(local_time->tm_min);
    if(useMinString.size() == 1) { 
      useMinString = "0" + useMinString;
    }
    string useHourString = to_string(useHour%12);
    if(useHourString == "0") {useHourString = "12";}
    systemTimePrint+= useHourString + ":" + useMinString;
    
    if(local_time->tm_hour >=12){
      systemTimePrint += " PM";
    } else {
      systemTimePrint += " AM";
    }

    adventureUIManager->systemClock->updateText(systemTimePrint, -1, 1);


    //show dijkstra debugging sprites
//    if(devMode && g_dijkstraEntity != nullptr) {
//      if(g_dijkstraEntity->current != nullptr) {
//        g_dijkstraDebugRed->x = g_dijkstraEntity->current->x;
//        g_dijkstraDebugRed->y = g_dijkstraEntity->current->y;
//      }
//      if(g_dijkstraEntity->dest != nullptr) {
//        g_dijkstraDebugBlue->x = g_dijkstraEntity->dest->x;
//        g_dijkstraDebugBlue->y = g_dijkstraEntity->dest->y;
//      }
//      if(g_dijkstraEntity->Destination != nullptr) {
//        g_dijkstraDebugYellow->x = g_dijkstraEntity->Destination->x;
//        g_dijkstraDebugYellow->y = g_dijkstraEntity->Destination->y;
//      }
//    }


    // did the protag die?
    if (protag->hp <= 0 && protag->essential)
    {
      playSound(-1, g_deathsound, 0);

      if (!canSwitchOffDevMode)
      {
        clear_map(g_camera);
        SDL_Delay(5000);
        load_map(renderer, "maps/sp-death/sp-death.map", "a");
      }
      protag->hp = 0.1;
      // if(canSwitchOffDevMode) { init_map_writing(renderer);}
    }

    // late november 2021 - projectiles are now updated after entities are - that way
    // if a behemoth has trapped the player in a tight corridor, their hitbox will hit the player before being
    // destroyed in the wall
    // update projectiles
    for (auto n : g_projectiles)
    {
      n->update(elapsed);
    }

    // delete projectiles with expired lifetimes
    for (long long unsigned int i = 0; i < g_projectiles.size(); i++)
    {
      if (g_projectiles[i]->lifetime <= 0)
      {
        delete g_projectiles[i];
        i--;
      }
    }

    // triggers
    for (long long unsigned i = 0; i < g_triggers.size(); i++)
    {
      if (!g_triggers[i]->active)
      {
        continue;
      }
      rect trigger = {g_triggers[i]->x, g_triggers[i]->y, g_triggers[i]->width, g_triggers[i]->height};
      entity *checkHim = searchEntities(g_triggers[i]->targetEntity);
      if (checkHim == nullptr)
      {
        continue;
      }
      rect movedbounds = rect(checkHim->bounds.x + checkHim->x, checkHim->bounds.y + checkHim->y, checkHim->bounds.width, checkHim->bounds.height);
      if (RectOverlap(movedbounds, trigger) && (checkHim->z > g_triggers[i]->z && checkHim->z < g_triggers[i]->z + g_triggers[i]->zeight))
      {
        adventureUIManager->blip = g_ui_voice;
        adventureUIManager->ownScript = g_triggers[i]->script;
        adventureUIManager->talker = narrarator;
        adventureUIManager->dialogue_index = -1;
        narrarator->sayings = g_triggers[i]->script;
        adventureUIManager->continueDialogue();
        // we need to break here if we loaded a new map
        // definately definately revisit this if you are having problems
        // with loading maps and memorycorruption
        //!!!
        if (transition)
        {
          break;
        }

        g_triggers[i]->active = 0;
      }
    }

    // worldsounds
    for (long long unsigned int i = 0; i < g_worldsounds.size(); i++)
    {
      g_worldsounds[i]->update(elapsed);
    }

    // transition
    {
      if (transition)
      {

        g_forceEndDialogue = 0;
        // onframe things
        SDL_LockTexture(transitionTexture, NULL, &transitionPixelReference, &transitionPitch);

        memcpy(transitionPixelReference, transitionSurface->pixels, transitionSurface->pitch * transitionSurface->h);
        Uint32 format = SDL_PIXELFORMAT_ARGB8888;
        SDL_PixelFormat *mappingFormat = SDL_AllocFormat(format);
        Uint32 *pixels = (Uint32 *)transitionPixelReference;
        // int numPixels = transitionImageWidth * transitionImageHeight;
        Uint32 transparent = SDL_MapRGBA(mappingFormat, 0, 0, 0, 255);
        // Uint32 halftone = SDL_MapRGBA( mappingFormat, 50, 50, 50, 128);
        transitionDelta += g_transitionSpeed + 0.02 * transitionDelta;
        for (int x = 0; x < transitionImageWidth; x++)
        {
          for (int y = 0; y < transitionImageHeight; y++)
          {
            int dest = (y * transitionImageWidth) + x;

            if (pow(pow(transitionImageWidth / 2 - x, 2) + pow(transitionImageHeight + y, 2), 0.5) < transitionDelta)
            {
              pixels[dest] = 0;
            }
            else
            {
              pixels[dest] = transparent;
            }
          }
        }

        ticks = SDL_GetTicks();
        elapsed = ticks - lastticks;

        SDL_UnlockTexture(transitionTexture);
        SDL_RenderCopy(renderer, transitionTexture, NULL, NULL);

        if (transitionDelta > transitionImageHeight + pow(pow(transitionImageWidth / 2, 2) + pow(transitionImageHeight, 2), 0.5))
        {
          transition = 0;
        }
      }
      else
      {
        transitionDelta = transitionImageHeight;
      }
    }

    if(g_musicSilenceMs > 0) {
      g_musicSilenceMs -= elapsed;
      musicUpdateTimer = 500;
      g_currentMusicPlayingEntity = nullptr;
      g_closestMusicNode = nullptr;
    } else { 
      // update music
      if (musicUpdateTimer > 500)
      {
        musicUpdateTimer = 0;
  
        // check musicalentities
        entity *listenToMe = nullptr;
        for (auto x : g_musicalEntities)
        {
          if (XYWorldDistance(x->getOriginX(), x->getOriginY(), g_focus->getOriginX(), g_focus->getOriginY()) < x->musicRadius && x->agrod && x->target == protag)
          {
            // we should be playing his music
            // incorporate priority later
            listenToMe = x;
          }
        }
        if (listenToMe != nullptr)
        {
          g_closestMusicNode = nullptr;
          if (g_currentMusicPlayingEntity != listenToMe)
          {
            Mix_FadeOutMusic(200);
            // Mix_PlayMusic(listenToMe->theme, 0);
            Mix_VolumeMusic(g_music_volume * 128);
            entFadeFlag = 1;
            fadeFlag = 0;
            musicFadeTimer = 0;
            g_currentMusicPlayingEntity = listenToMe;
          }
        }
        else
        {
          bool hadEntPlayingMusic = 0;
          if (g_currentMusicPlayingEntity != nullptr)
          {
            // stop ent music
            // Mix_FadeOutMusic(1000);
            hadEntPlayingMusic = 1;
            g_currentMusicPlayingEntity = nullptr;
          }
          if (g_musicNodes.size() > 0 && !g_mute)
          {
            newClosest = protag->Get_Closest_Node(g_musicNodes);
            if (g_closestMusicNode == nullptr)
            {
              if (!hadEntPlayingMusic)
              {
                Mix_PlayMusic(newClosest->blip, -1);
                Mix_VolumeMusic(g_music_volume * 128);
                g_closestMusicNode = newClosest;
              }
              else
              {
                g_closestMusicNode = newClosest;
                // change music
                Mix_FadeOutMusic(1000);
                musicFadeTimer = 0;
                fadeFlag = 1;
                entFadeFlag = 0;
              }
            }
            else
            {
  
              // Segfaults, todo is initialize these musicNodes to have something
              if (newClosest->name != g_closestMusicNode->name)
              {
                // D(newClosest->name);
                // if(newClosest->name == "silence") {
                // Mix_FadeOutMusic(1000);
                //}
                g_closestMusicNode = newClosest;
                // change music
                Mix_FadeOutMusic(1000);
                musicFadeTimer = 0;
                fadeFlag = 1;
                entFadeFlag = 0;
              }
            }
          }
        }
        // check for any cues
        for (auto x : g_cueSounds)
        {
          if (x->played == 0 && Distance(x->x, x->y, protag->x + protag->width / 2, protag->y) < x->radius)
          {
            x->played = 1;
            playSound(-1, x->blip, 0);
          }
        }
      }
      if (fadeFlag && musicFadeTimer > 1000 && newClosest != 0)
      {
        fadeFlag = 0;
        Mix_HaltMusic();
        cout << "played some music " << newClosest->name << endl;
        Mix_FadeInMusic(newClosest->blip, -1, 1000);
      }
      if (entFadeFlag && musicFadeTimer > 200)
      {
        entFadeFlag = 0;
        Mix_HaltMusic();
        Mix_FadeInMusic(g_currentMusicPlayingEntity->theme, -1, 200);
      }
    }

    // wakeup manager if it is sleeping
    if (adventureUIManager->sleepflag)
    {
      adventureUIManager->continueDialogue();
    }

    //shade
    SDL_RenderCopy(renderer, g_shade, NULL, NULL);

    SDL_RenderPresent(renderer);
  }

  clear_map(g_camera);
  delete adventureUIManager;
  close_map_writing();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_FreeSurface(transitionSurface);
  SDL_DestroyTexture(transitionTexture);
  SDL_DestroyTexture(background);
  SDL_DestroyTexture(g_shade);
  SDL_DestroyTexture(g_shadowTexture);
  SDL_DestroyTexture(g_shadowTextureAlternate);
  IMG_Quit();
  Mix_CloseAudio();
  TTF_Quit();

  return 0;
}

int interact(float elapsed, entity *protag)
{
  SDL_Rect srect;
  switch (protag->animation)
  {

    case 0:
      srect.h = protag->bounds.height;
      srect.w = protag->bounds.width;

      srect.x = protag->getOriginX() - srect.w / 2;
      srect.y = protag->getOriginY() - srect.h / 2;

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

      srect.x = protag->getOriginX() - srect.w / 2;
      srect.y = protag->getOriginY() - srect.h / 2;

      srect.y -= 30;
      if (protag->flip == SDL_FLIP_NONE)
      {
        srect.x -= 30;
      }
      else
      {
        srect.x += 30;
      }

      srect = transformRect(srect);
      // if(drawhitboxes) {
      // 	SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
      // 	SDL_RenderFillRect(renderer, &srect);
      // 	SDL_RenderPresent(renderer);
      // 	SDL_Delay(500);
      // }
      break;
    case 2:
      srect.h = protag->bounds.height;
      srect.w = protag->bounds.width;

      srect.x = protag->getOriginX() - srect.w / 2;
      srect.y = protag->getOriginY() - srect.h / 2;

      if (protag->flip == SDL_FLIP_NONE)
      {
        srect.x -= 55;
      }
      else
      {
        srect.x += 55;
      }

      srect = transformRect(srect);
      // if(drawhitboxes) {
      // 	SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
      // 	SDL_RenderFillRect(renderer, &srect);
      // 	SDL_RenderPresent(renderer);
      // 	SDL_Delay(500);
      // }
      break;
    case 3:
      srect.h = protag->bounds.height;
      srect.w = protag->bounds.width;

      srect.x = protag->getOriginX() - srect.w / 2;
      srect.y = protag->getOriginY() - srect.h / 2;

      srect.y += 30;
      if (protag->flip == SDL_FLIP_NONE)
      {
        srect.x -= 30;
      }
      else
      {
        srect.x += 30;
      }

      srect = transformRect(srect);
      // if(drawhitboxes) {
      // 	SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
      // 	SDL_RenderFillRect(renderer, &srect);
      // 	SDL_RenderPresent(renderer);
      // 	SDL_Delay(500);
      // }
      break;
    case 4:
      srect.h = protag->bounds.height;
      srect.w = protag->bounds.width;

      srect.x = protag->getOriginX() - srect.w / 2;
      srect.y = protag->getOriginY() - srect.h / 2;

      srect.y += 55;

      srect = transformRect(srect);
      // if(drawhitboxes) {
      // 	SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
      // 	SDL_RenderFillRect(renderer, &srect);
      // 	SDL_RenderPresent(renderer);
      // 	SDL_Delay(500);
      // }
      break;
  }

  for (long long unsigned int i = 0; i < g_entities.size(); i++)
  {

    SDL_Rect hisrect = {(int)g_entities[i]->x + g_entities[i]->bounds.x, (int)g_entities[i]->y + g_entities[i]->bounds.y, (int)g_entities[i]->bounds.width, (int)g_entities[i]->bounds.height};
    hisrect = transformRect(hisrect);

    if (g_entities[i] != protag && RectOverlap(hisrect, srect))
    {
      if (g_entities[i]->isWorlditem)
      {
        // add item to inventory

        // if the item exists, dont make a new one
        indexItem *a = nullptr;
        for (auto x : g_indexItems)
        {
          // substr because worlditems have the name "ITEM-" + whatever their file is called
          if (g_entities[i]->name.substr(5) == x->name)
          {
            a = x;
          }
        }
        // no resource found, so lets just make one
        if (a == nullptr)
        {
          a = new indexItem(g_entities[i]->name.substr(5), 0);
        }

        mainProtag->getItem(a, 1);
        delete g_entities[i];
        return 0;
      }
      if (g_entities[i]->tangible && g_entities[i]->sayings.size() > 0)
      {
        if (g_entities[i]->animlimit != 0)
        {
          g_entities[i]->animate = 1;
        }
        // make ent look at player, if they have the frames

        if(g_entities[i]->turnToFacePlayer && g_entities[i]->yframes >= 7)
        {
          float xvector = (g_entities[i]->getOriginX()) - (protag->getOriginX());
          float yvector = (g_entities[i]->getOriginY()) - (protag->getOriginY());
          float angle = atan2(yvector, xvector);
          g_entities[i]->flip = SDL_FLIP_NONE;
          if(angle < -7 * M_PI / 8 || angle >= 7 * M_PI / 8) {
            g_entities[i]->animation = 6;
          } else if (angle < 7 * M_PI / 8 && angle >= 5 * M_PI / 8) {
            g_entities[i]->animation = 7;
          } else if (angle < 5 * M_PI / 8 && angle >= 3 * M_PI / 8) {
            g_entities[i]->animation = 0;
          } else if (angle < 3 * M_PI / 8 && angle >= M_PI / 8) {
            g_entities[i]->animation = 1;
          } else if (angle < M_PI / 8 && angle >= - M_PI / 8) {
            g_entities[i]->animation = 2;
          } else if (angle < - M_PI / 8 && angle >= - 3 * M_PI / 8) {
            g_entities[i]->animation = 3;
          } else if (angle < - 3 * M_PI / 8 && angle > - 5 * M_PI / 8) {
            g_entities[i]->animation = 4;
          } else if (angle < - 5 * M_PI / 8 && angle > - 7 * M_PI / 8) {
            g_entities[i]->animation = 5;
          }
        }
        else if (g_entities[i]->turnToFacePlayer && g_entities[i]->yframes >= 5)
        {

          int xdiff = (g_entities[i]->getOriginX()) - (protag->getOriginX());
          int ydiff = (g_entities[i]->getOriginY()) - (protag->getOriginY());
          int axdiff = (abs(xdiff) - abs(ydiff));
          if (axdiff > 0)
          {
            // xaxis is more important
            g_entities[i]->animation = 2;
            if (xdiff > 0)
            {
              g_entities[i]->flip = SDL_FLIP_NONE;
            }
            else
            {
              g_entities[i]->flip = SDL_FLIP_HORIZONTAL;
            }
          }
          else
          {
            // yaxis is more important
            g_entities[i]->flip = SDL_FLIP_NONE;
            if (ydiff > 0)
            {
              g_entities[i]->animation = 0;
            }
            else
            {
              g_entities[i]->animation = 4;
            }
          }
          if (abs(axdiff) < 45)
          {
            if (xdiff > 0)
            {
              g_entities[i]->flip = SDL_FLIP_NONE;
            }
            else
            {
              g_entities[i]->flip = SDL_FLIP_HORIZONTAL;
            }
            if (ydiff > 0)
            {
              g_entities[i]->animation = 1;
            }
            else
            {
              g_entities[i]->animation = 3;
            }
          }
        }

        protagMakesNoise();

        adventureUIManager->blip = g_entities[i]->voice;
        //adventureUIManager->sayings = &g_entities[i]->sayings;
        adventureUIManager->talker = g_entities[i];
        
        adventureUIManager->dialogue_index = -1;
        g_forceEndDialogue = 0;
        adventureUIManager->continueDialogue();
        // removing this in early july to fix problem moving after a script changes map
        // may cause unexpected problems
        // protag_is_talking = 1;
        M("continueDialogue() finished, returning from interact()");
        g_ignoreInput = 1;
        return 0;
      }
    }
    // no one to talk to, lets do x instead (heres where it goes)
    if (use_cooldown <= 0)
    {
      // if(inPauseMenu) {
      // 	inPauseMenu = 0;
      // 	adventureUIManager->hideInventoryUI();
      // } else {
      // 	inPauseMenu = 1;
      // 	adventureUIManager->showInventoryUI();
      // }
    }
  }

  // we didnt have anything to interact with- lets do a dash
  //  if(g_dash_cooldown < 0 && protag_can_move) {
  //  	M("dash");
  //  	//convert frame to angle
  //  	float angle = convertFrameToAngle(protag->frame, protag->flip);
  //  	protag->xvel += 500 * (1 - (protag->friction * 3)) * cos(angle);
  //  	protag->yvel += 500 * (1 - (protag->friction * 3)) * sin(angle);
  //  	protag->spinningMS = 700;
  //  	playSound(-1, g_spin_sound, 0);
  //  	g_dash_cooldown = g_max_dash_cooldown;
  //  }
  return 0;
}

void getInput(float &elapsed)
{
  for (int i = 0; i < 16; i++)
  {
    oldinput[i] = input[i];
  }

  for (int i = 0; i < 5; i++)
  {
    oldStaticInput[i] = staticInput[i];
  }

  SDL_PollEvent(&event);

  //don't take input the frame after a mapload to prevent
  //talking to an ent instantly
  //even tho the player didnt press anything
  if(g_ignoreInput) {
    g_ignoreInput = 0;
    return;
  }


  //for portability, use the input[] array to drive controls
  //some actions are not bound, e.g. navigation of the settings menu

  //menu up 
  if(keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_W])
  {
    staticInput[0] = 1;
  } else 
  {
    staticInput[0] = 0;
  }
  //menu down 
  if(keystate[SDL_SCANCODE_DOWN] || keystate[SDL_SCANCODE_S])
  {
    staticInput[1] = 1;
  } else 
  {
    staticInput[1] = 0;
  }
  //menu left
  if(keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_A])
  {
    staticInput[2] = 1;
  } else 
  {
    staticInput[2] = 0;
  }
  //menu down 
  if(keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_D])
  {
    staticInput[3] = 1;
  } else 
  {
    staticInput[3] = 0;
  }
  //menu confirm
  if(keystate[SDL_SCANCODE_Z] || keystate[SDL_SCANCODE_SPACE])
  {
    staticInput[4] = 1;
  } else 
  {
    staticInput[4] = 0;
  }


  if (keystate[bindings[9]])
  {
    input[9] = 1;
  }
  else
  {
    input[9] = 0;
  }

  if (keystate[SDL_SCANCODE_W])
  {
    camy -= 4;
  }
  if (keystate[SDL_SCANCODE_A])
  {
    camx -= 4;
  }
  if (keystate[SDL_SCANCODE_S])
  {
    camy += 4;
  }
  if (keystate[SDL_SCANCODE_D])
  {
    camx += 4;
  }
  if (keystate[SDL_SCANCODE_G] && !inputRefreshCanSwitchOffDevMode && canSwitchOffDevMode)
  {
    toggleDevmode();
  }
  if (keystate[SDL_SCANCODE_G])
  {
    inputRefreshCanSwitchOffDevMode = 1;
  }
  else
  {
    inputRefreshCanSwitchOffDevMode = 0;
  }

  protag_can_move = !protag_is_talking;
  if (protag_can_move)
  {
    protag->shooting = 0;
    protag->left = 0;
    protag->right = 0;
    protag->up = 0;
    protag->down = 0;
    g_cameraAimingOffsetXTarget = 0;
    g_cameraAimingOffsetYTarget = 0;

    if (keystate[bindings[4]] && !inPauseMenu && g_cur_diagonalHelpFrames > g_diagonalHelpFrames)
    {
      protag->shoot_up();
      g_cameraAimingOffsetYTarget = 1;
    }

    if (keystate[bindings[5]] && !inPauseMenu && g_cur_diagonalHelpFrames > g_diagonalHelpFrames)
    {
      protag->shoot_down();
      g_cameraAimingOffsetYTarget = -1;
    }

    if (keystate[bindings[6]] && !inPauseMenu && g_cur_diagonalHelpFrames > g_diagonalHelpFrames)
    {
      protag->shoot_left();
      g_cameraAimingOffsetXTarget = -1;
    }

    if (keystate[bindings[7]] && !inPauseMenu && g_cur_diagonalHelpFrames > g_diagonalHelpFrames)
    {
      protag->shoot_right();
      g_cameraAimingOffsetXTarget = 1;
    }

    // if we aren't pressing any shooting keys, reset g_cur_diagonalhelpframes
    if (!(keystate[bindings[4]] || keystate[bindings[5]] || keystate[bindings[6]] || keystate[bindings[7]]))
    {
      g_cur_diagonalHelpFrames = 0;
    }
    else
    {
      g_cur_diagonalHelpFrames++;
    }

    // normalize g_cameraAimingOffsetTargetVector
    float len = pow(pow(g_cameraAimingOffsetXTarget, 2) + pow(g_cameraAimingOffsetYTarget, 2), 0.5);
    if (!isnan(len) && len != 0)
    {
      g_cameraAimingOffsetXTarget /= len;
      g_cameraAimingOffsetYTarget /= len;
    }



    if (keystate[bindings[10]])
    {
      input[10] = 1;
    }
    else
    {
      input[10] = 0;
    }

    if (keystate[bindings[0]])
    {
      if (inPauseMenu && SoldUIUp <= 0)
      {
        //playSound(1, g_menu_manip_sound, 0);
        if(inventorySelection - itemsPerRow >= 0) {
          inventorySelection -= itemsPerRow;

        }
        SoldUIUp = (oldUIUp) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
      }
      else
      {
        if(protag_can_move && !g_selectingUsable) {
          protag->move_up();
        }
      }
      oldUIUp = 1;
    }
    else
    {
      oldUIUp = 0;
      SoldUIUp = 0;
    }
    SoldUIUp--;

    if (keystate[bindings[1]])
    {
      if (inPauseMenu && SoldUIDown <= 0)
      {
        //playSound(1, g_menu_manip_sound, 0);
        if(inventorySelection + itemsPerRow < g_itemsInInventory) {

          if (ceil((float)(inventorySelection + 1) / (float)itemsPerRow) < (g_itemsInInventory / g_inventoryRows))
          {
            inventorySelection += itemsPerRow;
          }
        }
        SoldUIDown = (oldUIDown) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
      }
      else
      {
        if(protag_can_move && !g_selectingUsable) {
          protag->move_down();
        }
      }
      oldUIDown = 1;
    }
    else
    {
      oldUIDown = 0;
      SoldUIDown = 0;
    }
    SoldUIDown--;

    if (keystate[bindings[2]])
    {
      if (inPauseMenu && SoldUILeft <= 0)
      {
        //playSound(1, g_menu_manip_sound, 0);
        if (inventorySelection > 0)
        {
          if (inventorySelection % itemsPerRow != 0)
          {
            inventorySelection--;
          }
        }
        SoldUILeft = (oldUILeft) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
      }
      else
      {
        if(protag_can_move && !g_selectingUsable) {
          protag->move_left();
        } else if(protag_can_move && g_selectingUsable && SoldUILeft <= 0 && !inPauseMenu && g_spinning_duration <= 0 && g_usableWaitToCycleTime < 0) {
          //select prev backpack item
          g_backpackIndex --;
          g_hotbarCycleDirection = 1;
          if(g_backpackIndex < 0) { g_backpackIndex = g_backpack.size()-1;}
          SoldUILeft = (oldUILeft) ? g_inputDelayRepeatFrames : g_inputDelayFrames;

          adventureUIManager->hotbarPositions[4].first = 0.35 + g_backpackHorizontalOffset;
          adventureUIManager->hotbarPositions[2].first = 0.55 + g_backpackHorizontalOffset;

          //move icons
          int i = 0; //shift previous
          for(auto x : adventureUIManager->hotbarTransitionIcons) {
            x->targetx = adventureUIManager->hotbarPositions[i].first;
            x->targety = adventureUIManager->hotbarPositions[i].second;
            i++;
          }
          i = 1;
          for(auto x : adventureUIManager->hotbarTransitionIcons) {
            x->x = adventureUIManager->hotbarPositions[i].first;
            x->y = adventureUIManager->hotbarPositions[i].second;
            if(g_backpack.size() > 0) {
              int index = g_backpackIndex - i - 0;
              index = index % g_backpack.size();
              x->texture = g_backpack.at(index)->texture;
            }
            i++;
          }
          adventureUIManager->shiftingMs = adventureUIManager->maxShiftingMs;
        }
      }
      oldUILeft = 1;
    }
    else
    {
      oldUILeft = 0;
      SoldUILeft = 0;
    }
    SoldUILeft--;

    if (keystate[bindings[3]])
    {
      if (inPauseMenu && SoldUIRight <= 0)
      {
        //playSound(1, g_menu_manip_sound, 0);
        if (inventorySelection <= g_itemsInInventory)
        {
          // dont want this to wrap around
          if (inventorySelection % itemsPerRow != itemsPerRow - 1)
          {
            inventorySelection++;
          }
        }
        SoldUIRight = (oldUIRight) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
      }


      else
      {
        if(protag_can_move && !g_selectingUsable) {
          protag->move_right();
        } else if(protag_can_move && g_selectingUsable && SoldUIRight <= 0 && !inPauseMenu && g_spinning_duration <= 0 && g_usableWaitToCycleTime < 0) {
          //select next backpack item
          g_backpackIndex ++;
          g_hotbarCycleDirection = 0;
          if(g_backpackIndex > g_backpack.size() - 1) { g_backpackIndex = 0;}
          SoldUIRight = (oldUIRight) ? g_inputDelayRepeatFrames : g_inputDelayFrames;

          adventureUIManager->hotbarPositions[4].first = 0.35 + g_backpackHorizontalOffset;
          adventureUIManager->hotbarPositions[2].first = 0.55 + g_backpackHorizontalOffset;

          //move icons
          int i = 2; //shift next
          for(auto x : adventureUIManager->hotbarTransitionIcons) {
            x->targetx = adventureUIManager->hotbarPositions[i].first;
            x->targety = adventureUIManager->hotbarPositions[i].second;
            i++;
          }
          i = 1;
          for(auto x : adventureUIManager->hotbarTransitionIcons) {
            x->x = adventureUIManager->hotbarPositions[i].first;
            x->y = adventureUIManager->hotbarPositions[i].second;
            if(g_backpack.size() > 0) {
              int index = g_backpackIndex - i - 2;
              index = index % g_backpack.size();
              x->texture = g_backpack.at(index)->texture;
              i++;
            }
          }
          adventureUIManager->shiftingMs = adventureUIManager->maxShiftingMs;
        }
      }
      oldUIRight = 1;
    }
    else
    {
      oldUIRight = 0;
      SoldUIRight = 0;
    }
    SoldUIRight--;

    // //fix inventory input
    // if(inventorySelection < 0) {
    // 	inventorySelection = 0;
    // }

    // check if the stuff is onscreen
    if (inventorySelection >= (g_inventoryRows * itemsPerRow) + (inventoryScroll * itemsPerRow))
    {
      inventoryScroll++;
    }
    else
    {
      if (inventorySelection < (inventoryScroll * itemsPerRow))
      {
        inventoryScroll--;
      }
    }

    // constrain inventorySelection based on itemsInInventory
    if (inventorySelection > g_itemsInInventory - 1)
    {
      // M(g_itemsInInventory - 1);
      inventorySelection = g_itemsInInventory - 1;
    }

    if (inventorySelection < 0)
    {
      inventorySelection = 0;
    }
   
    if(keystate[bindings[12]] && protag_can_move && !inPauseMenu) {
      //for short presses, advance inventory left rather than widening the hotbar
      g_currentHotbarSelectMs += elapsed;
      if(g_currentHotbarSelectMs >= g_hotbarLongSelectMs) {
        if(g_selectingUsable == 0) {
          g_usableWaitToCycleTime = g_maxUsableWaitToCycleTime;
        }
        g_selectingUsable = 1;
      }
    } else {
      if(g_currentHotbarSelectMs > 0 && g_selectingUsable == 0 && !inPauseMenu && g_spinning_duration <= 0) {
        //select next backpack item
        g_backpackIndex ++;
        if(g_backpackIndex > g_backpack.size() - 1) { g_backpackIndex = 0;}
      }
      g_selectingUsable = 0;
      g_currentHotbarSelectMs = 0;

      //end animation quickly
//      adventureUIManager->hotbarTransitionIcons[2]->x = adventureUIManager->hotbarPositions[3].first;
//      adventureUIManager->hotbarTransitionIcons[2]->y = adventureUIManager->hotbarPositions[3].second;
      adventureUIManager->hotbarTransitionIcons[2]->targetx = adventureUIManager->hotbarPositions[3].first;
      adventureUIManager->hotbarTransitionIcons[2]->targety = adventureUIManager->hotbarPositions[3].second;
     
      //this moves it to where it should be (other side), and then it glides to the middle
      //since the hotbar was moved to the right side of the screen it was adjusted 
//      if(adventureUIManager->shiftingMs < 150) {
//        if(g_hotbarCycleDirection) {
//          adventureUIManager->hotbarTransitionIcons[2]->x = adventureUIManager->hotbarTransitionIcons[3]->x;
//          adventureUIManager->hotbarTransitionIcons[2]->y = adventureUIManager->hotbarTransitionIcons[3]->y;
//        } else {
//          adventureUIManager->hotbarTransitionIcons[2]->x = adventureUIManager->hotbarTransitionIcons[1]->x;
//          adventureUIManager->hotbarTransitionIcons[2]->y = adventureUIManager->hotbarTransitionIcons[1]->y;
//        }
//      }
      if(adventureUIManager->shiftingMs < 150) {
        if(g_hotbarCycleDirection) {
          adventureUIManager->hotbarTransitionIcons[2]->x = adventureUIManager->smallBarStableX;
          //adventureUIManager->hotbarTransitionIcons[2]->y = adventureUIManager->hotbarTransitionIcons[3]->y;
        } else {
          adventureUIManager->hotbarTransitionIcons[2]->x = adventureUIManager->smallBarStableX;
          //adventureUIManager->hotbarTransitionIcons[2]->y = adventureUIManager->hotbarTransitionIcons[1]->y;
        }
      }

      if(g_backpack.size() > 0) {
        adventureUIManager->hotbarTransitionIcons[2]->texture = g_backpack.at(g_backpackIndex)->texture;
      }
    }


    if (keystate[bindings[12]] && protag_can_move)
    {


      //this used to be how we opened up the inventory menu which took up the whole screen
      //now we want to just open the backpack, but the player will be able to do this by
      //using the picnicbox, so I'm not deleting this code, just commenting it out
     
      //it will be exposed thru the scripting interpreter later
//      // pause menu
//      if (inPauseMenu)
//      {
//        //if this is the inventory screen, close it
//        if(g_inventoryUiIsLevelSelect == 0) {
//          playSound(-1, g_menu_close_sound, 0);
//          inPauseMenu = 0;
//          elapsed = 16;
//          adventureUIManager->hideInventoryUI();
//        }
//      }
//      else
//      {
//
//        playSound(-1, g_menu_open_sound, 0);
//        g_inventoryUiIsLevelSelect = 0;
//        inPauseMenu = 1;
//        g_firstFrameOfPauseMenu = 1;
//        adventureUIManager->escText->updateText("", -1, 0.9);
//        inventorySelection = 0;
//        adventureUIManager->positionInventory();
//        adventureUIManager->showInventoryUI();
//        
//
//
//      }
    }

  }
  else
  {
    //settings menu
    if(g_inSettingsMenu) {
      //menu up
      if (staticInput[0])
      {
        if(SoldUIUp <= 0)
        {
          
          if(!g_awaitSwallowedKey && !g_settingsUI->modifyingValue) {
            if(g_settingsUI->positionOfCursor == 0) {
            g_settingsUI->positionOfCursor = g_settingsUI->maxPositionOfCursor;
            } else {
              g_settingsUI->positionOfCursor--;
            }
          }
          SoldUIUp = (oldStaticInput[0]) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
        } else {
          SoldUIUp--;
        }
      } else {
        SoldUIUp = 0; 
      }

      //menu down
      if (staticInput[1]) {
        if(SoldUIDown <= 0 )
        {
          if(!g_awaitSwallowedKey && !g_settingsUI->modifyingValue) { 
            if(g_settingsUI->positionOfCursor == g_settingsUI->maxPositionOfCursor) {
              g_settingsUI->positionOfCursor = 0;
            } else {
              g_settingsUI->positionOfCursor++;
            }
          }
          SoldUIDown = (oldStaticInput[1]) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
        } else {
          SoldUIDown--;
        }
      } else {
        SoldUIDown = 0; 
      }

     
      //menu left
      if (staticInput[2])
      {
        if(SoldUILeft <= 0)
        {
          if(g_settingsUI->modifyingValue) {
            //modify the value left
            switch(g_settingsUI->positionOfCursor) {
              case 9: {
                g_music_volume -= g_settingsUI->deltaVolume;
                if(g_music_volume < g_settingsUI->minVolume) {
                  g_music_volume = g_settingsUI->minVolume;
                }
                string content = to_string((int)round(g_music_volume * 100)) + "%";
                g_settingsUI->valueTextboxes[9]->updateText(content, -1, 1);
                Mix_VolumeMusic(g_music_volume * 128);
                break;
              }
              case 10: {
                g_sfx_volume -= g_settingsUI->deltaVolume;
                if(g_sfx_volume < g_settingsUI->minVolume) {
                  g_sfx_volume = g_settingsUI->minVolume;
                }
                string content = to_string((int)round(g_sfx_volume * 100))+ "%";
                g_settingsUI->valueTextboxes[10]->updateText(content, -1, 1);
                break;
              }
              case 11: {
                g_graphicsquality -= g_settingsUI->deltaGraphics;
                if(g_graphicsquality < g_settingsUI->minGraphics) {
                  g_graphicsquality = g_settingsUI->minGraphics;
                }
                g_settingsUI->valueTextboxes[11]->updateText(g_graphicsStrings[g_graphicsquality], -1, 1);
                break;
              }
              default: {
                g_brightness -= g_settingsUI->deltaBrightness;
                if(g_brightness < g_settingsUI->minBrightness) {
                  g_brightness = g_settingsUI->minBrightness;
                }
                string content = to_string((int)round(g_brightness)) + "%";
                g_settingsUI->valueTextboxes[12]->updateText(content, -1, 1);
                //SDL_SetTextureAlphaMod(g_shade, 255 - ( ( g_brightness/100.0 ) * 255));
                SDL_SetWindowBrightness(window, g_brightness/100.0 );
                break;
              }
            }
          } else {
            if(g_settingsUI->cursorIsOnBackButton && !g_awaitSwallowedKey) {
              g_settingsUI->cursorIsOnBackButton = 0;
              g_settingsUI->positionOfCursor = 0;
            }
          }
          SoldUILeft = (oldStaticInput[2]) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
        } else {
          SoldUILeft --;
        }
      } else {
        SoldUILeft = 0;
      }
      
      //menu right
      if (staticInput[3])
      {
        if(SoldUIRight <= 0)
        {
          if(g_settingsUI->modifyingValue) {
            //modify the value right
            switch(g_settingsUI->positionOfCursor) {
              case 9: {
                g_music_volume += g_settingsUI->deltaVolume;
                if(g_music_volume > g_settingsUI->maxVolume) {
                  g_music_volume = g_settingsUI->maxVolume;
                }
                string content = to_string((int)round(g_music_volume * 100)) + "%";
                g_settingsUI->valueTextboxes[9]->updateText(content, -1, 1);
                Mix_VolumeMusic(g_music_volume * 128);
                break;
              }
              case 10: {
                g_sfx_volume += g_settingsUI->deltaVolume;
                if(g_music_volume > g_settingsUI->maxVolume) {
                  g_music_volume = g_settingsUI->maxVolume;
                }
                string content = to_string((int)round(g_sfx_volume * 100)) + "%";
                g_settingsUI->valueTextboxes[10]->updateText(content, -1, 1);
                break;
              }
              case 11: {
                g_graphicsquality += g_settingsUI->deltaGraphics;
                if(g_graphicsquality > g_settingsUI->maxGraphics) {
                  g_graphicsquality = g_settingsUI->maxGraphics;
                }
                g_settingsUI->valueTextboxes[11]->updateText(g_graphicsStrings[g_graphicsquality], -1, 1);
                break;
              }
              default: {
                g_brightness += g_settingsUI->deltaBrightness;
                if(g_brightness > g_settingsUI->maxBrightness) {
                  g_brightness = g_settingsUI->maxBrightness;
                }
                string content = to_string((int)round(g_brightness)) + "%";
                g_settingsUI->valueTextboxes[12]->updateText(content, -1, 1);
                //SDL_SetTextureAlphaMod(g_shade, 255 - ( ( g_brightness/100.0 ) * 255));
                SDL_SetWindowBrightness(window, g_brightness/100.0 );
                break;
              }
            }
          } else {
            if(!g_settingsUI->cursorIsOnBackButton && !g_awaitSwallowedKey) {
              g_settingsUI->cursorIsOnBackButton = 1;
            }
          }
          SoldUIRight = (oldStaticInput[3]) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
        } else {
          SoldUIRight --;
        }
      } else {
        SoldUIRight = 0;
      }

      //menu select
      if (staticInput[4] && !oldStaticInput[4] && !g_swallowedAKeyThisFrame)
      {
        if(g_settingsUI->cursorIsOnBackButton) {
          //save config to file
          {
            ofstream outfile("user/configs/" + g_config + ".cfg");
            for(int i = 0; i < 14; i++) {
              outfile << SDL_GetScancodeName(bindings[i]) << endl;
            }
            outfile << "Fullscreen " << g_fullscreen << endl;
            outfile << "BgDarkness " << g_background_darkness << endl;
            outfile << "MusicVolume " << g_music_volume << endl;
            outfile << "SFXVolume " << g_sfx_volume << endl;
            outfile << "Textsize " << g_fontsize << endl;
            outfile << "Minitextsize " << g_minifontsize << endl;
            outfile << "Transitionspeed " << g_transitionSpeed << endl;
            outfile << "Graphics0lowto3high " << g_graphicsquality << endl;
            outfile << "brightness0lowto100high " << g_brightness << endl;
            outfile << "This config was autowritten" << endl;
            outfile.close();

  
          }


          //continue the script which was running 
          g_inventoryUiIsLevelSelect = 0;
          g_inventoryUiIsKeyboard = 0;
          g_inSettingsMenu = 0;
          inPauseMenu = 0;
          g_settingsUI->hide();
          protag_is_talking = 0;

        } else {
          for(int i = 0; i < g_settingsUI->valueTextboxes.size(); i++) {
            //g_settingsUI->valueTextboxes[i]->blinking = 0;
          }


          if(g_settingsUI->positionOfCursor == 0) { //rebind Up
            g_pollForThisBinding = 0; 
            g_swallowAKey = 1;
            g_awaitSwallowedKey = 1;
            g_whichRebindValue = 0;
            g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText("_", -1, 1);
            //g_settingsUI->valueTextboxes[g_whichRebindValue]->blinking = 1;
            g_settingsUI->uiModifying();
          }
  
          if(g_settingsUI->positionOfCursor == 1) { //rebind Down
            g_pollForThisBinding = 1; 
            g_swallowAKey = 1;
            g_awaitSwallowedKey = 1;
            g_whichRebindValue = 1;
            g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText("_", -1, 1);
            //g_settingsUI->valueTextboxes[g_whichRebindValue]->blinking = 1;
            g_settingsUI->uiModifying();
          }
  
          if(g_settingsUI->positionOfCursor == 2) { //rebind Left
            g_pollForThisBinding = 2; 
            g_swallowAKey = 1;
            g_awaitSwallowedKey = 1;
            g_whichRebindValue = 2;
            g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText("_", -1, 1);
            //g_settingsUI->valueTextboxes[g_whichRebindValue]->blinking = 1;
            g_settingsUI->uiModifying();
          }
  
          if(g_settingsUI->positionOfCursor == 3) { //rebind Right
            g_pollForThisBinding = 3; 
            g_swallowAKey = 1;
            g_awaitSwallowedKey = 1;
            g_whichRebindValue = 3;
            g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText("_", -1, 1);
            //g_settingsUI->valueTextboxes[g_whichRebindValue]->blinking = 1;
            g_settingsUI->uiModifying();
          }
  
          if(g_settingsUI->positionOfCursor == 4) { //rebind Jump
            g_pollForThisBinding = 8; 
            g_swallowAKey = 1;
            g_awaitSwallowedKey = 1;
            g_whichRebindValue = 4;
            g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText("_", -1, 1);
            //g_settingsUI->valueTextboxes[g_whichRebindValue]->blinking = 1;
            g_settingsUI->uiModifying();
          }
  
          if(g_settingsUI->positionOfCursor == 5) { //rebind Interact
            g_pollForThisBinding = 11; 
            g_swallowAKey = 1;
            g_awaitSwallowedKey = 1;
            g_whichRebindValue = 5;
            g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText("_", -1, 1);
            //g_settingsUI->valueTextboxes[g_whichRebindValue]->blinking = 1;
            g_settingsUI->uiModifying();
          }
  
          if(g_settingsUI->positionOfCursor == 6) { //rebind Inventory
            g_pollForThisBinding = 12; 
            g_swallowAKey = 1;
            g_awaitSwallowedKey = 1;
            g_whichRebindValue = 6;
            g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText("_", -1, 1);
            //g_settingsUI->valueTextboxes[g_whichRebindValue]->blinking = 1;
            g_settingsUI->uiModifying();
          }
  
          if(g_settingsUI->positionOfCursor == 7) { //rebind Spin/use
            g_pollForThisBinding = 13; 
            g_swallowAKey = 1;
            g_awaitSwallowedKey = 1;
            g_whichRebindValue = 7;
            g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText("_", -1, 1);
            //g_settingsUI->valueTextboxes[g_whichRebindValue]->blinking = 1;
            g_settingsUI->uiModifying();
          }
  
          if(g_settingsUI->positionOfCursor == 8) { //fullscreen
            if(!g_fullscreen) {
              g_settingsUI->valueTextboxes[8]->updateText(g_affirmStr, -1, 0.3);
            } else {
              g_settingsUI->valueTextboxes[8]->updateText(g_negStr, -1, 0.3);
            }
            toggleFullscreen();
          }

          if(g_settingsUI->positionOfCursor == 9) { // Music volume
            g_settingsUI->modifyingValue = !g_settingsUI->modifyingValue;
            if(g_settingsUI->modifyingValue) {
              //g_settingsUI->valueTextboxes[9]->blinking = 1;
              g_settingsUI->uiModifying();
            } else {
              g_settingsUI->uiSelecting();
            }
          }

          if(g_settingsUI->positionOfCursor == 10) { // Sounds volume
            g_settingsUI->modifyingValue = !g_settingsUI->modifyingValue;
            if(g_settingsUI->modifyingValue) {
              //g_settingsUI->valueTextboxes[10]->blinking = 1;
              g_settingsUI->uiModifying();
            } else {
              g_settingsUI->uiSelecting();
            }
          }

          if(g_settingsUI->positionOfCursor == 11) { // Graphics Quality
            g_settingsUI->modifyingValue = !g_settingsUI->modifyingValue;
            if(g_settingsUI->modifyingValue) {
              //g_settingsUI->valueTextboxes[11]->blinking = 1;
              g_settingsUI->uiModifying();
            } else {
              g_settingsUI->uiSelecting();
            }
          }

          if(g_settingsUI->positionOfCursor == 12) { // Brightness
            g_settingsUI->modifyingValue = !g_settingsUI->modifyingValue;
            if(g_settingsUI->modifyingValue) {
              //g_settingsUI->valueTextboxes[12]->blinking = 1;
              g_settingsUI->uiModifying();
            } else {
              g_settingsUI->uiSelecting();
            }
          }
        }
      }

      //did we swallow a keypress?
      if(g_awaitSwallowedKey && !g_swallowAKey) {
        //there might be a problem with this logic
        bindings[g_pollForThisBinding] = g_swallowedKey;
        g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText(SDL_GetScancodeName(g_swallowedKey), -1, 1);
        //g_settingsUI->valueTextboxes[g_whichRebindValue]->blinking = 0;
        g_awaitSwallowedKey = 0;
        g_settingsUI->uiSelecting();
      }

      if(g_settingsUI->positionOfCursor > g_settingsUI->maxPositionOfCursor) {
        g_settingsUI->positionOfCursor = g_settingsUI->maxPositionOfCursor;
      }

      if(g_settingsUI->positionOfCursor < g_settingsUI->minPositionOfCursor) {
        g_settingsUI->positionOfCursor = g_settingsUI->minPositionOfCursor;
      } else {

      }
    }
    if(g_inEscapeMenu) {
      //menu up
      if (staticInput[0])
      {
        if(SoldUIUp <= 0)
        {
          
          if(!g_awaitSwallowedKey && !g_escapeUI->modifyingValue) {
            if(g_escapeUI->positionOfCursor == 0) {
            g_escapeUI->positionOfCursor = g_escapeUI->maxPositionOfCursor;
            } else {
              g_escapeUI->positionOfCursor--;
            }
          }
          SoldUIUp = (oldStaticInput[0]) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
        } else {
          SoldUIUp--;
        }
      } else {
        SoldUIUp = 0; 
      }

      //menu down
      if (staticInput[1]) {
        if(SoldUIDown <= 0 )
        {
          if(!g_awaitSwallowedKey && !g_escapeUI->modifyingValue) { 
            if(g_escapeUI->positionOfCursor == g_escapeUI->maxPositionOfCursor) {
              g_escapeUI->positionOfCursor = 0;
            } else {
              g_escapeUI->positionOfCursor++;
            }
          }
          SoldUIDown = (oldStaticInput[1]) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
        } else {
          SoldUIDown--;
        }
      } else {
        SoldUIDown = 0; 
      }

      //menu select
      if (staticInput[4] && !oldStaticInput[4] && !g_swallowedAKeyThisFrame)
      {
        g_inEscapeMenu = 0;
        //inPauseMenu = 0;
        protag_is_talking = 0;
  
        g_escapeUI->hide();

        if(g_escapeUI->positionOfCursor == 0) { //Back
          
        }
  
        if(g_escapeUI->positionOfCursor == 1) { //levelselect
          clear_map(g_camera);
          //load_map(renderer, "maps/base/g.map", "a");
          //instead of loading the base, open the level-select menu
          g_inventoryUiIsLevelSelect = 1;
          g_inventoryUiIsKeyboard = 0;
          inventorySelection = 0;
          inPauseMenu = 1;
          g_firstFrameOfPauseMenu = 1;
          old_z_value = 1;
          adventureUIManager->escText->updateText("", -1, 0.9);
          adventureUIManager->positionInventory();
          adventureUIManager->showInventoryUI();
          adventureUIManager->hideHUD();
        }
  
        if(g_escapeUI->positionOfCursor == 2) { //To desktop
          quit = 1;
        }

        if(g_escapeUI->positionOfCursor == 3) { //Settings
          g_settingsUI->show();
          g_inSettingsMenu = 1;
          g_firstFrameOfSettingsMenu = 1;
          g_settingsUI->positionOfCursor = 0;
          g_settingsUI->cursorIsOnBackButton = 0;
          protag_is_talking = 1;
        }
  
        
      }

      //did we swallow a keypress?
      if(g_awaitSwallowedKey && !g_swallowAKey) {
        //there might be a problem with this logic
        bindings[g_pollForThisBinding] = g_swallowedKey;
        g_awaitSwallowedKey = 0;
        g_escapeUI->uiSelecting();
      }

      if(g_escapeUI->positionOfCursor > g_escapeUI->maxPositionOfCursor) {
        g_escapeUI->positionOfCursor = g_escapeUI->maxPositionOfCursor;
      }

      if(g_escapeUI->positionOfCursor < g_escapeUI->minPositionOfCursor) {
        g_escapeUI->positionOfCursor = g_escapeUI->minPositionOfCursor;
      } else {

      }
    }

    // reset shooting offsets
    g_cameraAimingOffsetXTarget = 0;
    g_cameraAimingOffsetYTarget = 0;

    if (keystate[bindings[2]] && !left_ui_refresh)
    {
      if (adventureUIManager->askingQuestion)
      {
        if (adventureUIManager->response_index > 0)
        {
          adventureUIManager->response_index--;
        }
      }
      left_ui_refresh = 1;
    }
    else if (!keystate[bindings[2]])
    {
      left_ui_refresh = 0;
    }
    if (keystate[bindings[3]] && !right_ui_refresh)
    {
      if (adventureUIManager->askingQuestion)
      {
        adventureUIManager->response_index++;
        if (adventureUIManager->response_index > adventureUIManager->responses.size() - 1)
        {
          adventureUIManager->response_index--;
        }
      }
      right_ui_refresh = 1;
    }
    else if (!keystate[bindings[3]])
    {
      right_ui_refresh = 0;
    }
    protag->stop_hori();
    protag->stop_verti();
  }

  if (keystate[bindings[8]])
  {
    input[8] = 1;
  }
  else
  {
    input[8] = 0;
  }

  if (keystate[bindings[9]])
  {
    input[9] = 1;
  }
  else
  {
    input[9] = 0;
  }

  if(keystate[bindings[13]]) 
  {
    input[13] = 1;
  }
  else
  {
    input[13] = 0;
  }


  //spinning/using item
  if( g_backpack.size() > 0 && protag_can_move && !inPauseMenu) {
    if(g_backpack.at(g_backpackIndex)->specialAction == 1 ) { //do a spin

      if ( ((input[13] && !oldinput[13]) || (input[13] && storedSpin) ) && protag_can_move && (protag->grounded || g_currentSpinJumpHelpMs > 0 )
              && (g_spin_cooldown <= 0 && g_spinning_duration <= 0 + g_doubleSpinHelpMs && g_afterspin_duration <= 0 )
    
         )
      {
        g_backpack.at(g_backpackIndex)->cooldownMs = g_backpack.at(g_backpackIndex)->maxCooldownMs;
        protagMakesNoise();
        storedSpin = 0;
        //propel the protag in the direction of their velocity
        //at high speed, removing control from them
        g_spinning_duration = g_spinning_duration_max;
        g_spinning_xvel = protag->xvel * g_spinning_boost;
        g_spinning_yvel = protag->yvel * g_spinning_boost;
        g_spin_cooldown = g_spin_max_cooldown;
        g_spin_entity->frameInAnimation = 0;
      } else {
        if(input[13] && !oldinput[13] && !protag->grounded) {
          storedSpin = 1;
        }
    
      }
      g_currentSpinJumpHelpMs-= elapsed;
    
      if(protag->grounded) {
        g_currentSpinJumpHelpMs = g_spinJumpHelpMs;
      }
    
      if(g_spin_enabled && g_spinning_duration >= 0 && g_spinning_duration - elapsed < 0 && protag->grounded && !g_protag_jumped_this_frame) {
        g_afterspin_duration = g_afterspin_duration_max;
      }
    
      g_spin_entity->x = protag->x;
      g_spin_entity->y = protag->y;
      g_spin_entity->z = protag->z;
    
  
    } else if ( (input[13] && !oldinput[13]) && g_backpack.at(g_backpackIndex)->specialAction == 2 && g_backpack.at(g_backpackIndex)->cooldownMs <= 0) { //open inventory
      
      if (inPauseMenu)
      {
        //if this is the inventory screen, close it
        if(g_inventoryUiIsLevelSelect == 0) {
          playSound(-1, g_menu_close_sound, 0);
          inPauseMenu = 0;
          elapsed = 16;
          adventureUIManager->hideInventoryUI();
        }
      }
      else
      {
        protagMakesNoise();
        usable* thisUsable = g_backpack.at(g_backpackIndex); 
        thisUsable->cooldownMs = thisUsable->maxCooldownMs;
        playSound(-1, g_menu_open_sound, 0);
        g_inventoryUiIsLevelSelect = 0;
        inPauseMenu = 1;
        g_firstFrameOfPauseMenu = 1;
        adventureUIManager->escText->updateText("", -1, 0.9);
        inventorySelection = 0;
        adventureUIManager->positionInventory();
        adventureUIManager->showInventoryUI();
      }

    } else if (input[13] && !oldinput[13] && g_backpack.at(g_backpackIndex)->cooldownMs <= 0) { //there is an item to use, and it doesn't make the protag spin
      
      //is the player too hungry to use an item?
      if(g_foodpoints < g_starvingFoodpoints) {
        //make the stomach shake
        adventureUIManager->stomachShakeIntervalMs = 0;
        adventureUIManager->stomachShakeDurationMs = 0;

      } else {
        protagMakesNoise();
        
        usable* thisUsable = g_backpack.at(g_backpackIndex); 
        thisUsable->cooldownMs = thisUsable->maxCooldownMs;
  
        //make a scriptcaller
        adventureUI scripter(renderer, 1);
        scripter.playersUI = 0;
        scripter.useOwnScriptInsteadOfTalkersScript = 1;
        scripter.talker = g_backpackNarrarator;
        scripter.ownScript = thisUsable->script;
        scripter.dialogue_index = -1;
        scripter.continueDialogue();

      }

    }
  } else if (inPauseMenu && input[13] && !oldinput[13]) {
    //this button should take the player out of the inventory for safety
    //incase they somehow switch off 
    
    //if this is the inventory screen, close it
    if(g_inventoryUiIsLevelSelect == 0) {
      playSound(-1, g_menu_close_sound, 0);
      inPauseMenu = 0;
      elapsed = 16;
      adventureUIManager->hideInventoryUI();
    }
  }

  for(int i = 0; i < g_backpack.size(); i++) { g_backpack.at(i)->cooldownMs -= elapsed/2;}
  if(g_backpack.size() > 0) { g_backpack.at(g_backpackIndex)->cooldownMs -= elapsed/2; } //item selected cools down faster 

  g_spin_cooldown -= elapsed;
  g_spinning_duration -= elapsed;
  g_afterspin_duration -= elapsed;

  // mapeditor cancel button
//  if (keystate[SDL_SCANCODE_X])
//  {
//    devinput[4] = 1;
//  }

  int markeryvel = 0;
  // mapeditor cursor vertical movement for keyboards
  if (keystate[SDL_SCANCODE_KP_PLUS])
  {
    markeryvel = 1;
  }
  else
  {
    keyboard_marker_vertical_modifier_refresh = 1;
  }

  if (keystate[SDL_SCANCODE_KP_MINUS])
  {
    markeryvel = -1;
  }
  else
  {
    keyboard_marker_vertical_modifier_refresh_b = 1;
  }

  if (markeryvel != 0)
  {
    if (g_holdingCTRL)
    {
      if (markeryvel > 0 && keyboard_marker_vertical_modifier_refresh)
      {
        wallstart -= 64;
      }
      else if (markeryvel < 0 && keyboard_marker_vertical_modifier_refresh_b)
      {
        wallstart += 64;
      }
      if (wallstart < 0)
      {
        wallstart = 0;
      }
      else
      {
        if (wallstart > 64 * g_layers)
        {
          wallstart = 64 * g_layers;
        }
        if (wallstart > wallheight - 64)
        {
          wallstart = wallheight - 64;
        }
      }
    }
    else
    {
      if (markeryvel > 0 && keyboard_marker_vertical_modifier_refresh)
      {
        wallheight -= 64;
      }
      else if (markeryvel < 0 && keyboard_marker_vertical_modifier_refresh_b)
      {
        wallheight += 64;
      }
      if (wallheight < wallstart + 64)
      {
        wallheight = wallstart + 64;
      }
      else
      {
        if (wallheight > 64 * g_layers)
        {
          wallheight = 64 * g_layers;
        }
      }
    }
  }
  if (keystate[SDL_SCANCODE_KP_PLUS])
  {
    keyboard_marker_vertical_modifier_refresh = 0;
  }

  if (keystate[SDL_SCANCODE_KP_MINUS])
  {
    keyboard_marker_vertical_modifier_refresh_b = 0;
  }

  if (keystate[bindings[11]] && !old_z_value && !inPauseMenu && !g_inSettingsMenu && !g_inEscapeMenu)
  {
    if (protag_is_talking == 1)
    {
      if (!adventureUIManager->typing)
      {
        adventureUIManager->continueDialogue();
      }
    }
  }
  else if (keystate[bindings[11]] && !old_z_value && inPauseMenu && !g_firstFrameOfPauseMenu)
  {
    if(g_inventoryUiIsLevelSelect == 0) {

      if(g_inventoryUiIsKeyboard) {
        //append this character to the string

        //handle special keys like caps, backspace, and enter
        if(
            g_alphabet[inventorySelection] == '<' 
            || g_alphabet[inventorySelection] == '^' 
            || g_alphabet[inventorySelection] == ';'

            ) {

            //backspace
            if(g_alphabet[inventorySelection] == '<') {
              if(g_keyboardInput.size() > 0) {
                g_keyboardInput = g_keyboardInput.substr(0, g_keyboardInput.size()-1);
              }
            }

            //caps
            if(g_alphabet[inventorySelection] == '^') {
              if(g_alphabet == g_alphabet_lower) {
                g_alphabet = g_alphabet_upper;
                g_alphabet_textures = &g_alphabetUpper_textures;
              } else {
                g_alphabet = g_alphabet_lower;
                g_alphabet_textures = &g_alphabetLower_textures;
              }
            }

            if(g_alphabet[inventorySelection] == ';' && g_keyboardInput != "") {
              writeSaveFieldString(g_keyboardSaveToField, g_keyboardInput);

              //this will continue the script which was running, even if it just instantly terminates

              g_inventoryUiIsLevelSelect = 0;
              g_inventoryUiIsKeyboard = 0;
              inPauseMenu = 0;
              adventureUIManager->hideInventoryUI();
  

              adventureUIManager->dialogue_index++;
              adventureUIManager->continueDialogue();


            }


        } else {
        
          if(g_keyboardInput.size() < g_keyboardInputLength) {
            g_keyboardInput += g_alphabet[inventorySelection];
          }

          //take off caps (add sound if it was on)
          g_alphabet = g_alphabet_lower;
          g_alphabet_textures = &g_alphabetLower_textures;

        }

      } else {
      // select item in pausemenu
      // only if we arent running a script
      if (protag_can_move && adventureUIManager->sleepingMS <= 0 && mainProtag->inventory.size() > 0 && mainProtag->inventory[mainProtag->inventory.size() - 1 - inventorySelection].first->script.size() > 0 && !g_firstFrameOfPauseMenu)
      {
        // call the item's script
        // D(mainProtag->inventory[mainProtag->inventory.size()- 1 -inventorySelection].first->name);
        adventureUIManager->blip = g_ui_voice;
        adventureUIManager->ownScript = mainProtag->inventory[mainProtag->inventory.size() - 1 - inventorySelection].first->script;
        adventureUIManager->talker = protag;
        adventureUIManager->dialogue_index = -1;
        protag->sayings = mainProtag->inventory[mainProtag->inventory.size() - 1 - inventorySelection].first->script;
        adventureUIManager->continueDialogue();
        // if we changed maps/died/whatever, close the inventory
        if (transition)
        {
          inPauseMenu = 0;
          adventureUIManager->hideInventoryUI();
        }
        old_z_value = 1;
      }
      }
    } else {
      //if this level is unlocked, travel to its map
      if(g_levelSequence->levelNodes[inventorySelection]->locked == 0) {
  
        for (long long unsigned int i = 0; i < g_ui.size(); i++)
        {
          //g_ui[i]->render(renderer, g_camera);
        }
  
        clear_map(g_camera);
  
        inPauseMenu = 0;
  
        string mapName = g_levelSequence->levelNodes[inventorySelection]->mapfilename;
        vector<string> x = splitString(mapName, '/');
        g_mapdir = x[1];
  
        //since we are loading a new level, reset the pellets
        g_currentPelletsCollected = 0;

        load_map(renderer, mapName, g_levelSequence->levelNodes[inventorySelection]->waypointname);
        adventureUIManager->hideInventoryUI();
  
        if (canSwitchOffDevMode)
        {
          init_map_writing(renderer);
        }
        protag_is_talking = 0;
        protag_can_move = 1;
        adventureUIManager->showHUD();
      } else {
        //play an error noise?
  
      }
    }
  }

  dialogue_cooldown -= elapsed;


  if (keystate[bindings[11]] && !inPauseMenu && !transition)
  {
    if (protag_is_talking == 1)
    { // advance or speedup diaglogue
      text_speed_up = 50;
    }
    if (protag_is_talking == 0)
    {
      if (dialogue_cooldown < 0)
      {
        interact(elapsed, protag);
      }
    }
    old_z_value = 1;
  }
  else
  {

    // reset text_speed_up
    text_speed_up = 1;
    old_z_value = 0;
  }

  if(keystate[bindings[8]] && protag_is_talking) {
    adventureUIManager->skipText();
  }

  if (keystate[bindings[11]] && inPauseMenu)
  {
    old_z_value = 1;
  }
  else if (inPauseMenu)
  {
    old_z_value = 0;
  }

  if (protag_is_talking == 2)
  {
    protag_is_talking = 0;
    dialogue_cooldown = 500;
  }

  if (keystate[SDL_SCANCODE_LSHIFT] && devMode)
  {
    protag->xmaxspeed = 100;
    protag->turningSpeed = 1.4;
    //protag->ymaxspeed = 145;
  }
  if (keystate[SDL_SCANCODE_LCTRL] && devMode)
  {
    protag->xmaxspeed = 20;
    protag->turningSpeed = 16;
    //protag->ymaxspeed = 20;
  }
  if (keystate[SDL_SCANCODE_CAPSLOCK] && devMode)
  {
    protag->xmaxspeed = 750;
    protag->turningSpeed = 16;
    //protag->ymaxspeed = 750;
  }

  if (keystate[SDL_SCANCODE_SLASH] && devMode)
  {
  }
  // make another entity of the same type as the last spawned
  if (keystate[SDL_SCANCODE_U] && devMode)
  {
    devinput[1] = 1;
  }
  if (keystate[SDL_SCANCODE_V] && devMode)
  {
    devinput[2] = 1;
  }
//  if (keystate[SDL_SCANCODE_V] && devMode)
//  {
//    devinput[3] = 1;
//  }
  if (keystate[SDL_SCANCODE_B] && devMode)
  {
    // this is make-trigger
    devinput[0] = 1;
  }
  if (keystate[SDL_SCANCODE_N] && devMode)
  {
    devinput[5] = 1;
  }
  if (keystate[SDL_SCANCODE_M] && devMode)
  {
    devinput[6] = 1;
  }
  if (keystate[SDL_SCANCODE_KP_DIVIDE] && devMode)
  {
    // decrease gridsize
    devinput[7] = 1;
  }
  if (keystate[SDL_SCANCODE_KP_MULTIPLY] && devMode)
  {
    // increase gridsize
    devinput[8] = 1;
  }
  if (keystate[SDL_SCANCODE_O] && devMode)
  {
    // enable/disable collisions
    devinput[9] = 1;
    // mysteriously, this doesn't work anymore
    // might
  }
  if (keystate[SDL_SCANCODE_KP_5] && devMode)
  {
    // triangles
    devinput[10] = 1;
  }
  if (keystate[SDL_SCANCODE_KP_3] && devMode)
  {
    // debug hitboxes
    devinput[7] = 1;
  }
  if (keystate[SDL_SCANCODE_KP_8] && devMode)
  {
    devinput[22] = 1;
  }
  if (keystate[SDL_SCANCODE_KP_6] && devMode)
  {
    devinput[23] = 1;
  }
  if (keystate[SDL_SCANCODE_KP_2] && devMode)
  {
    devinput[24] = 1;
  }
  if (keystate[SDL_SCANCODE_KP_4] && devMode)
  {
    devinput[25] = 1;
  }

  if (keystate[SDL_SCANCODE_RETURN] && devMode)
  {
    // stop player first
    protag->stop_hori();
    protag->stop_verti();

    elapsed = 0;
    // pull up console
    devinput[11] = 1;
  }

  if (keystate[SDL_SCANCODE_COMMA] && devMode)
  {
    // make navnode box
    devinput[20] = 1;
  }

  // for testing particles
  if (keystate[SDL_SCANCODE_Y] && devMode)
  {
    devinput[37] = 1;
  }

  if (keystate[SDL_SCANCODE_PERIOD] && devMode)
  {
    // make navnode box
    devinput[21] = 1;
  }

  //make implied slope 
  if (keystate[SDL_SCANCODE_K] && devMode)
  {
    devinput[34] = 1;
  }

  //make triangular implied slope 
  if (keystate[SDL_SCANCODE_J] && devMode)
  {
    devinput[35] = 1;
  }
  if (keystate[SDL_SCANCODE_L] && devMode)
  {
    devinput[36] = 1;
  }

  if (keystate[SDL_SCANCODE_ESCAPE])
  {

    if (devMode)
    {
      quit = 1;
    }
    else
    {
      if(!protag_is_talking && !inPauseMenu) {
        //open escape menu
        g_escapeUI->show();
        g_inEscapeMenu = 1;
        g_firstFrameOfSettingsMenu = 1;
        g_escapeUI->positionOfCursor = 0;
        g_escapeUI->cursorIsOnBackButton = 0;
        protag_is_talking = 1;
      }
    }
  }
  if (devMode)
  {
    g_update_zoom = 0;
    if (keystate[SDL_SCANCODE_Q] && devMode && g_holdingCTRL)
    {
      g_update_zoom = 1;
      g_zoom_mod -= 0.001 * elapsed;

      if (g_zoom_mod < min_scale)
      {
        g_zoom_mod = min_scale;
      }
      if (g_zoom_mod > max_scale)
      {
        g_zoom_mod = max_scale;
      }
    }

    if (keystate[SDL_SCANCODE_E] && devMode && g_holdingCTRL)
    {
      g_update_zoom = 1;
      g_zoom_mod += 0.001 * elapsed;

      if (g_zoom_mod < min_scale)
      {
        g_zoom_mod = min_scale;
      }
      if (g_zoom_mod > max_scale)
      {
        g_zoom_mod = max_scale;
      }
    }
  }
  if (keystate[SDL_SCANCODE_BACKSPACE])
  {
    devinput[16] = 1;
  }

  if (keystate[bindings[0]] == keystate[bindings[1]])
  {
    protag->stop_verti();
  }

  if (keystate[bindings[2]] == keystate[bindings[3]])
  {
    protag->stop_hori();
  }

  if (keystate[SDL_SCANCODE_F] && fullscreen_refresh)
  {
    toggleFullscreen();
  }

  if (keystate[SDL_SCANCODE_F])
  {
    fullscreen_refresh = 0;
  }
  else
  {
    fullscreen_refresh = 1;
  }

  if (keystate[SDL_SCANCODE_1] && devMode)
  {
    devinput[16] = 1;
  }

  if (keystate[SDL_SCANCODE_2] && devMode)
  {
    devinput[17] = 1;
  }

  if (keystate[SDL_SCANCODE_3] && devMode)
  {
    devinput[18] = 1;
  }
  if (keystate[SDL_SCANCODE_3] && devMode)
  {
    devinput[18] = 1;
  }
  if (keystate[SDL_SCANCODE_I] && devMode)
  {
    devinput[19] = 1;
  }
}

void toggleFullscreen() {
  g_fullscreen = !g_fullscreen;
  if (g_fullscreen)
  {
    SDL_GetCurrentDisplayMode(0, &DM);

    SDL_GetWindowSize(window, &saved_WIN_WIDTH, &saved_WIN_HEIGHT);

    SDL_SetWindowSize(window, DM.w, DM.h);
    SDL_GetWindowSize(window, &WIN_WIDTH, &WIN_HEIGHT);
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);


    // we need to reload some (all?) textures
    for (auto x : g_mapObjects)
    {
      if (x->mask_fileaddress != "&")
      {
        x->reloadTexture();
      }
    }

    // reassign textures for asset-sharers
    for (auto x : g_mapObjects)
    {
      x->reassignTexture();
    }

    // the same must be done for masked tiles
    for (auto t : g_tiles)
    {
      if (t->mask_fileaddress != "&")
      {
        t->reloadTexture();
      }
    }

    // reassign textures for any asset-sharers
    for (auto x : g_tiles)
    {
      x->reassignTexture();
    }
  }
  else
  {

    SDL_SetWindowFullscreen(window, 0);

    // restore saved width/height
    SDL_SetWindowSize(window, saved_WIN_WIDTH, saved_WIN_HEIGHT);
    SDL_GetWindowSize(window, &WIN_WIDTH, &WIN_HEIGHT);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    // we need to reload some (all?) textures
    for (auto x : g_mapObjects)
    {
      if (x->mask_fileaddress != "&")
      {
        x->reloadTexture();
//        I("reloaded a texture of mask");
//        I(x->mask_fileaddress);
      }
    }

    // reassign textures for asset-sharers
    for (auto x : g_mapObjects)
    {
      x->reassignTexture();
    }

    // the same must be done for masked tiles
    for (auto t : g_tiles)
    {
      if (t->mask_fileaddress != "&")
      {
        t->reloadTexture();
      }
    }

    // reassign textures for any asset-sharers
    for (auto x : g_tiles)
    {
      x->reassignTexture();
    }
  }
}

void toggleDevmode() {
  if (canSwitchOffDevMode)
  {
    devMode = !devMode;
    if(g_camera.upperLimitX != g_camera.lowerLimitX) {
      if(devMode) {g_camera.enforceLimits = 0;} else {g_camera.enforceLimits = 1;}
    }
    g_zoom_mod = 1;
    g_update_zoom = 1;
    marker->x = -1000;
    markerz->x = -1000;
    if (g_fogofwarEnabled && !devMode)
    {
      for (auto x : g_fogslates)
      {
        x->tangible = 1;
      }
      for (auto x : g_fogslatesA)
      {
        x->tangible = 1;
      }
      for (auto x : g_fogslatesB)
      {
        x->tangible = 1;
      }
    }
    else
    {
      for (auto x : g_fogslates)
      {
        x->tangible = 0;
      }
      for (auto x : g_fogslatesA)
      {
        x->tangible = 0;
      }
      for (auto x : g_fogslatesB)
      {
        x->tangible = 0;
      }
    }
  }
  if (devMode)
  {
    if(drawhitboxes) {
      floortexDisplay->show = 1;
      captexDisplay->show = 1;
      walltexDisplay->show = 1;
    }
    boxsenabled = 0;
  }
  else
  {
    floortexDisplay->show = 0;
    captexDisplay->show = 0;
    walltexDisplay->show = 0;
    boxsenabled = 1;
    // float scalex = ((float)WIN_WIDTH / 1920) * g_defaultZoom;
    // float scaley = scalex;
    SDL_RenderSetScale(renderer, scalex * g_zoom_mod, scalex * g_zoom_mod);
  }
}

//when the protag jumps, uses an item, collects a pellet, or talks to someone
//nearby behemoths will agro on him
//but, nearby isn't defined by g_earshot but rather the behemoth's hearingRadius
void protagMakesNoise() {
  if(g_ninja || devMode) { return;}
  for(auto x : g_ai) {
    float distToProtag = XYWorldDistanceSquared(protag->getOriginX(), protag->getOriginY(), x->getOriginX(), x->getOriginY());
    float maxHearingDist = pow(x->hearingRadius,2);

    if(distToProtag <= maxHearingDist) {
      if(x->useAgro && x->targetFaction == protag->faction) {
        x->agrod = 1;
        x->target = protag;
      }
    }
  }

}
