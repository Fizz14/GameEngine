#include <iostream>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdlib.h>
#include "globals.h"
#include "objects.h"
#include "map_editor.h"
#include "lightcookietesting.h"

using namespace std;

void getInput(float &elapsed);

int WinMain()
{

  //devMode = 0; canSwitchOffDevMode = 0;
  devMode = 1; canSwitchOffDevMode = 1;

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
  // narrarator->name = "sp-narrarator";

  // entity* fomm;
  // cout << "about to make fomm" << endl;
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
  ui *protagHealthbarA = new ui(renderer, "static/ui/healthbarA.bmp", 0, 0, 0.05, 0.02, -3);
  protagHealthbarA->persistent = 1;
  ui *protagHealthbarB = new ui(renderer, "static/ui/healthbarB.bmp", 0, 0, 0.05, 0.02, -2);
  protagHealthbarB->persistent = 1;
  protagHealthbarB->shrinkPixels = 1;

  ui *protagHealthbarC = new ui(renderer, "static/ui/healthbarC.bmp", 0, 0, 0.05, 0.02, -1);
  protagHealthbarC->persistent = 1;
  protagHealthbarC->shrinkPixels = 1;

  protagHealthbarA->show = g_showHealthbar;
  protagHealthbarB->show = g_showHealthbar;
  protagHealthbarC->show = g_showHealthbar;


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
  g_font = "engine/Monda-Bold.ttf";

  // setup UI
  adventureUIManager = new adventureUI(renderer);

  if (canSwitchOffDevMode)
  {
    init_map_writing(renderer);
    // done once, because textboxes aren't cleared during clear_map()
    nodeInfoText = new textbox(renderer, "", g_fontsize * WIN_WIDTH, 0, 0, WIN_WIDTH);
    config = "edit";
    nodeDebug = SDL_CreateTextureFromSurface(renderer, IMG_Load("engine/walker.bmp"));
  }
  // set bindings from file
  ifstream bindfile;
  bindfile.open("user/configs/" + config + ".cfg");
  string line;
  for (int i = 0; i < 13; i++)
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
  cout << g_music_volume << endl;

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
  SDL_SetTextureAlphaMod(g_shade, 255 - ( ( g_brightness/100.0 ) * 255));
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
  g_playerdamage = Mix_LoadWAV("static/sounds/playerdamage.wav");
  g_enemydamage = Mix_LoadWAV("static/sounds/enemydamage.wav");
  g_npcdamage = Mix_LoadWAV("static/sounds/npcdamage.wav");
  g_s_playerdeath = Mix_LoadWAV("static/sounds/playerdeath.wav");
  g_land = Mix_LoadWAV("static/sounds/step2.wav");
  g_footstep_a = Mix_LoadWAV("static/sounds/protag-step-1.wav");
  g_footstep_b = Mix_LoadWAV("static/sounds/protag-step-2.wav");
  g_bonk = Mix_LoadWAV("static/sounds/bonk.wav");
  g_menu_open_sound = Mix_LoadWAV("static/sounds/open-menu.wav");
  g_menu_close_sound = Mix_LoadWAV("static/sounds/close-menu.wav");
  g_ui_voice = Mix_LoadWAV("static/sounds/voice-normal.wav");
  g_menu_manip_sound = Mix_LoadWAV("static/sounds/manip-menu.wav");

  srand(time(NULL));
  if (devMode)
  {
    // g_transitionSpeed = 10000;
    //!!!
    loadSave();
    cout << "finished loadSave()" << endl;
    // empty map or default map for map editing, perhaps a tutorial even
    load_map(renderer, "maps/g/g.map","a");
    //cameraToReset.resetCamera(); //somehow, reseting the cam is needed for lightcookies (changed default values to zero)
    //init_map_writing(renderer);
    g_map = "g";
    g_mapdir = "g";
    protag->x = 100000;
    protag->y = 100000;
  }
  else
  {
    SDL_ShowCursor(0);
    loadSave();
    g_mapdir = "first";
    load_map(renderer, g_first_map, "a");
  }

  ui *inventoryMarker = new ui(renderer, "static/ui/non_selector.bmp", 0, 0, 0.15, 0.15, 2);
  inventoryMarker->show = 0;
  inventoryMarker->persistent = 1;

  textbox *inventoryText = new textbox(renderer, "1", 40, WIN_WIDTH * 0.8, 0, WIN_WIDTH * 0.2);
  inventoryText->show = 0;
  inventoryText->align = 1;

  // This stuff is for the FoW mechanic

  // engine/resolution.bmp has resolution 1920 x 1200
  SDL_Surface *SurfaceA = IMG_Load("engine/resolution.bmp");
  // SDL_Surface* SurfaceB = IMG_Load("engine/b.bmp");

  TextureA = SDL_CreateTextureFromSurface(renderer, SurfaceA);
  TextureD = SDL_CreateTextureFromSurface(renderer, SurfaceA);

  // SDL_Texture* TextureA = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIN_WIDTH, WIN_HEIGHT);
  // TextureB = SDL_CreateTextureFromSurface(renderer, SurfaceB);

  SDL_FreeSurface(SurfaceA);
  // SDL_FreeSurface(SurfaceB);

  SDL_Surface *blackbarSurface = IMG_Load("engine/black.bmp");
  blackbarTexture = SDL_CreateTextureFromSurface(renderer, blackbarSurface);

  SDL_FreeSurface(blackbarSurface);

  // SDL_Texture* TextureC;

  result = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 500, 500);
  result_c = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 500, 500);

  SDL_SetTextureBlendMode(result, SDL_BLENDMODE_MOD);
  SDL_SetTextureBlendMode(result_c, SDL_BLENDMODE_MOD);

  SDL_SetTextureBlendMode(TextureA, SDL_BLENDMODE_MOD);
  SDL_SetTextureBlendMode(TextureA, SDL_BLENDMODE_MOD);
  SDL_SetTextureBlendMode(TextureD, SDL_BLENDMODE_MOD);

  // SDL_SetTextureBlendMode(TextureB, SDL_BLENDMODE_NONE);

  // init fogslates

  entity *s;
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
    s->sortingOffset = 130; // -35
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
  }

  SDL_DestroyTexture(s->texture);

  // test new particle system

  smokeEffect = new effectIndex("default", renderer);

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


  for (auto x : g_fogslates)
  {
    x->texture = TextureC;
  }

  g_loadingATM = 0;

  while (!quit)
  {

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
          }
          break;
        case SDL_KEYUP:
          switch (event.key.keysym.sym)
          {
            case SDLK_TAB:
              g_holdingCTRL = 0;
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
    // g_dash_cooldown -= elapsed;

    if (inPauseMenu)
    {
      // if we're paused, freeze gametime
      elapsed = 0;
    }

    // INPUT
    getInput(elapsed);

    // spring
    if ((input[8] && !oldinput[8] && protag->grounded && protag_can_move) || (input[8] && storedJump && protag->grounded && protag_can_move))
    {
      protag->zaccel = 180;
      storedJump = 0;
      breakpoint();
    }
    else
    {
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
        adventureUIManager->blip = NULL;
        adventureUIManager->sayings = &g_listeners[i]->script;
        adventureUIManager->talker = narrarator;
        narrarator->dialogue_index = -1;
        narrarator->sayings = g_listeners[i]->script;
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
    }

    old_WIN_WIDTH = WIN_WIDTH;

    WIN_WIDTH /= scalex;
    WIN_HEIGHT /= scaley;
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

          // to make old tiles fade out

          bool flipper = 0;
          for (long long unsigned i = 0; i < g_fogcookies.size(); i++)
          {
            for (long long unsigned j = 0; j < g_fogcookies[0].size(); j++)
            {
              flipper = !flipper;
              int xpos = ((i - g_fogMiddleX) * 64) + functionalX;
              int ypos = ((j - g_fogMiddleY) * 55) + functionalY;
              if (XYWorldDistance(functionalX, functionalY, xpos, ypos) > g_viewdist)
              {
                // g_fogcookies[i][j] -= g_tile_fade_speed; if (g_fogcookies[i][j] < 0) g_fogcookies[i][j] = 0;
                g_fogcookies[i][j] = 0;

                // g_fc[i][j] -= g_tile_fade_speed; if (g_fc[i][j] < 0) g_fc[i][j] = 0;
                g_fc[i][j] = 0;

                // g_sc[i][j] -= g_tile_fade_speed; if (g_sc[i][j] < 0) g_sc[i][j] = 0;
                g_sc[i][j] = 0;
              }
              else if (LineTrace(functionalX, functionalY, xpos, ypos, 0, 35, g_focus->stableLayer, 15, 1))
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
                blocked = LineTrace(xpos, ypos, xpos, ypos, 0, 6, g_focus->stableLayer + 1, 15, 1);
              }
              else
              {
                blocked = LineTrace(functionalX, functionalY + 3, xpos, ypos, 0, 1, g_focus->stableLayer + 1, 15, 1);
              }
              if (!(XYWorldDistance(functionalX, functionalY, xpos, ypos) > g_viewdist) && blocked)
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
            bool firsttrace = LineTrace(xpos, ypos, xpos, ypos, 0, 15, g_focus->stableLayer + 1, 2, 1);
            bool secondtrace = LineTrace(xpos, ypos + 55, xpos, ypos + 55, 0, 15, g_focus->stableLayer + 1, 2, 1);
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

        // update crosshair to current objective
        rect objectiverect = {g_objective->getOriginX(), g_objective->getOriginY() - (XtoZ * ((g_objective->bounds.zeight / 2) + g_objective->z)), 1, 1};
        objectiverect = transformRect(objectiverect);
        // is the x offscreen? let's adjust it somewhat
        const float margin = 0.1;

        float crossx = (float)objectiverect.x / (float)WIN_WIDTH;
        float crossy = (float)objectiverect.y / (float)WIN_HEIGHT;

        // make vector from the middle of the screen to the position of the obj
        float vx = crossx - 0.5;
        float vy = crossy - 0.5;

        float vectorlen = pow(pow(vx, 2) + pow(vy, 2), 0.5);
        if (vectorlen * 2.2 > 1)
        {
          vx /= vectorlen * 2.2;
          vy /= vectorlen * 2.2;
          // vy /= WIN_WIDTH/ WIN_HEIGHT;
        }
        crossx = vx + 0.5;
        crossy = vy + 0.5;

        // hide crosshair if we are close
        if (vectorlen < 0.17)
        {
          crossx = 5;
          crossy = 5;
        }

        adventureUIManager->crosshair->x = crossx - adventureUIManager->crosshair->width / 2;

        adventureUIManager->crosshair->y = crossy - adventureUIManager->crosshair->height / 2;
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
        delete g_particles[i];
        i--;
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

        SDL_Surface *textsurface = TTF_RenderText_Blended_Wrapped(nodeInfoText->font, g_worldsounds[i]->name.c_str(), {15, 15, 15}, 1 * WIN_WIDTH);
        SDL_Texture *texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);

        SDL_RenderCopy(renderer, texttexture, NULL, &textrect);

        SDL_FreeSurface(textsurface);
        SDL_DestroyTexture(texttexture);

        nodeInfoText->x = obj.x;
        nodeInfoText->y = obj.y - 20;
        nodeInfoText->updateText(g_worldsounds[i]->name, 15, 15);
        nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
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
        nodeInfoText->updateText(g_doors[i]->to_map + "->" + g_doors[i]->to_point, 15, 15);
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
        nodeInfoText->updateText(g_triggers[i]->binding, 15, 15);
        nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      }

      // listeners
      for (long long unsigned int i = 0; i < g_listeners.size(); i++)
      {
        SDL_Rect obj = {(int)((g_listeners[i]->x - g_camera.x - 20) * g_camera.zoom), (int)((g_listeners[i]->y - g_camera.y - 20) * g_camera.zoom), (int)(40 * g_camera.zoom), (int)(40 * g_camera.zoom)};
        SDL_RenderCopy(renderer, listenerIcon->texture, NULL, &obj);
        nodeInfoText->x = obj.x;
        nodeInfoText->y = obj.y - 20;
        nodeInfoText->updateText(g_listeners[i]->listenList.size() + " of " + g_listeners[i]->entityName, 15, 15);
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
      SDL_Rect topbar = {px, FoWrect.y - 1000, 1500, 1000};
      SDL_RenderCopy(renderer, blackbarTexture, NULL, &topbar);
      SDL_Rect botbar = {px, FoWrect.y + g_fogheight * 55 + 10, 2000, 1000};
      SDL_RenderCopy(renderer, blackbarTexture, NULL, &botbar);

      SDL_Rect leftbar = {px-800, FoWrect.y, 1000, 1500};
      SDL_RenderCopy(renderer, blackbarTexture, NULL, &leftbar);
      SDL_Rect rightbar = {px + 1100, FoWrect.y, 1000, 1500};
      SDL_RenderCopy(renderer, blackbarTexture, NULL, &rightbar);

    }

    // ui
    if (!inPauseMenu && g_showHUD)
    {
      // !!! segfaults on mapload sometimes
      adventureUIManager->healthText->updateText(to_string(int(protag->hp)) + '/' + to_string(int(protag->maxhp)), WIN_WIDTH * g_minifontsize, 0.9);
      adventureUIManager->healthText->show = g_showHealthbar;
    }
    else
    {
      adventureUIManager->healthText->show = 0;
    }

    // move the healthbar properly to the protagonist
    rect obj; // = {( , (((protag->y - ((protag->height))) - protag->z * XtoZ) - g_camera.y) * g_camera.zoom, (protag->width * g_camera.zoom), (protag->height * g_camera.zoom))};
    obj.x = ((protag->x - g_camera.x) * g_camera.zoom);
    obj.y = (((protag->y - ((floor(protag->height) * 0.9))) - protag->z * XtoZ) - g_camera.y) * g_camera.zoom;
    obj.width = (protag->width * g_camera.zoom);
    obj.height = (floor(protag->height) * g_camera.zoom);

    protagHealthbarA->x = (((float)obj.x + obj.width / 2) / (float)WIN_WIDTH) - protagHealthbarA->width / 2.0;
    protagHealthbarA->y = ((float)obj.y) / (float)WIN_HEIGHT;
    protagHealthbarB->x = protagHealthbarA->x;
    protagHealthbarB->y = protagHealthbarA->y;

    protagHealthbarC->x = protagHealthbarA->x;
    protagHealthbarC->y = protagHealthbarA->y;
    protagHealthbarC->width = (protag->hp / protag->maxhp) * 0.05;
    adventureUIManager->healthText->boxX = protagHealthbarA->x + protagHealthbarA->width / 2;
    adventureUIManager->healthText->boxY = protagHealthbarA->y - 0.005;

    for (long long unsigned int i = 0; i < g_ui.size(); i++)
    {
      g_ui[i]->render(renderer, g_camera);
    }
    for (long long unsigned int i = 0; i < g_textboxes.size(); i++)
    {
      g_textboxes[i]->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    }

    // draw pause screen
    if (inPauseMenu)
    {
      adventureUIManager->crosshair->x = 5;

      // iterate thru inventory and draw items on screen
      float defaultX = WIN_WIDTH * 0.05;
      float defaultY = WIN_WIDTH * 0.05;
      float x = defaultX;
      float y = defaultY;
      float maxX = WIN_WIDTH * 0.9;
      float maxY = WIN_HEIGHT * 0.60;
      float itemWidth = WIN_WIDTH * 0.07;
      float padding = WIN_WIDTH * 0.01;

      int i = 0;
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
          inventoryText->updateText(to_string(it->second), 35, 100);
          inventoryText->boxX = (x + (itemWidth * 0.8)) / WIN_WIDTH;
          inventoryText->boxY = (y + (itemWidth - inventoryText->boxHeight / 2) * 0.6) / WIN_HEIGHT;
          inventoryText->worldspace = 1;
          inventoryText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
        }
        else
        {
          inventoryText->show = 0;
        }

        if (i == inventorySelection)
        {
          // this item should have the marker
          inventoryMarker->show = 1;
          inventoryMarker->x = x / WIN_WIDTH;
          inventoryMarker->y = y / WIN_HEIGHT;
          inventoryMarker->width = itemWidth / WIN_WIDTH;

          float biggen = 0.01; // !!! resolutions : might have problems with diff resolutions
          inventoryMarker->x -= biggen;
          inventoryMarker->y -= biggen * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
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
        adventureUIManager->escText->updateText(description, WIN_WIDTH * g_fontsize, 0.9);
      }
      else
      {
        adventureUIManager->escText->updateText("", WIN_WIDTH * g_fontsize, 0.9);
      }
    }
    else
    {
      inventoryMarker->show = 0;
      inventoryText->show = 0;
    }

    // sines for item bouncing
    g_elapsed_accumulator += elapsed;
    g_itemsinea = sin(g_elapsed_accumulator / 300) * 10 + 30;
    g_itemsineb = sin((g_elapsed_accumulator - 1400) / 300) * 10 + 30;
    g_itemsinec = sin((g_elapsed_accumulator + 925) / 300) * 10 + 30;

    if (g_elapsed_accumulator > 1800)
    {
      g_elapsed_accumulator -= 1800;
    }

    // ENTITY MOVEMENT (ENTITY UPDATE)
    // dont update movement while transitioning
    if (!transition)
    {
      for (long long unsigned int i = 0; i < g_entities.size(); i++)
      {
        if (g_entities[i]->isWorlditem)
        {
          // make it bounce
          int index = g_entities[i]->bounceindex;
          if (index == 0)
          {
            g_entities[i]->floatheight = g_itemsinea;
          }
          else if (index == 1)
          {
            g_entities[i]->floatheight = g_itemsineb;
          }
          else
          {
            g_entities[i]->floatheight = g_itemsinec;
          }
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

          clear_map(g_camera);
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
            //change manner of death for ttl here (e.g. shrink, fade)
            //ttl ents will just be deleted so that they don't accumulate
            //if you wish for cheap textures, spawn another ent with that tex
            //which doesn't get deleted until the map is deloaded
            if(!g_entities[i]->asset_sharer) {
              g_entities[i]->tangible = 0;
              g_entities[i]->usingTimeToLive = 0;
            } else {
              delete g_entities[i];
            }
          } /*else if(g_entities[i]->timeToLiveMs < 500)  {
            //shrink
            g_entities[i]->width = 0;
            g_entities[i]->height = 0;
            g_entities[i]->animspeed = 0.001;
          } */

        }

      }
    }

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
        adventureUIManager->sayings = &g_triggers[i]->script;
        adventureUIManager->talker = narrarator;
        narrarator->dialogue_index = -1;
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
          //!!! this is for a debuggingsession for windows, take it out soon
          for (int y = 0; y < transitionImageHeight; y++)
          {
            int dest = (y * transitionImageWidth) + x;
            // int src =  (y * transitionImageWidth) + x;

            if (pow(pow(transitionImageWidth / 2 - x, 2) + pow(transitionImageHeight + y, 2), 0.5) < transitionDelta)
            {
              pixels[dest] = 0;
            }
            else
            {
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
        closestMusicNode = nullptr;
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
          if (closestMusicNode == nullptr)
          {
            if (!hadEntPlayingMusic)
            {
              Mix_PlayMusic(newClosest->blip, -1);
              Mix_VolumeMusic(g_music_volume * 128);
              closestMusicNode = newClosest;
            }
            else
            {
              closestMusicNode = newClosest;
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
            if (newClosest->name != closestMusicNode->name)
            {
              // D(newClosest->name);
              // if(newClosest->name == "silence") {
              // Mix_FadeOutMusic(1000);
              //}
              closestMusicNode = newClosest;
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
    if (fadeFlag && musicFadeTimer > 1000 /*&& newClosest != 0*/)
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
  // M("interact()");
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

        adventureUIManager->blip = g_entities[i]->voice;
        adventureUIManager->sayings = &g_entities[i]->sayings;
        adventureUIManager->talker = g_entities[i];
        
        adventureUIManager->talker->dialogue_index = -1;
        g_forceEndDialogue = 0;
        adventureUIManager->continueDialogue();
        // removing this in early july to fix problem moving after a script changes map
        // may cause unexpected problems
        // protag_is_talking = 1;
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

  SDL_PollEvent(&event);

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
      floortexDisplay->show = 1;
      captexDisplay->show = 1;
      walltexDisplay->show = 1;
    }
    else
    {
      floortexDisplay->show = 0;
      captexDisplay->show = 0;
      walltexDisplay->show = 0;
      // float scalex = ((float)WIN_WIDTH / 1920) * g_defaultZoom;
      // float scaley = scalex;
      SDL_RenderSetScale(renderer, scalex * g_zoom_mod, scalex * g_zoom_mod);
    }
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

    if (keystate[bindings[9]])
    {
      input[9] = 1;
    }
    else
    {
      input[9] = 0;
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
        playSound(-1, g_menu_manip_sound, 0);
        // if(inventorySelection - itemsPerRow >= 0) {
        inventorySelection -= itemsPerRow;

        //}
        SoldUIUp = (oldUIUp) ? 6 : 30;
      }
      else
      {
        protag->move_up();
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
        playSound(-1, g_menu_manip_sound, 0);
        // if(inventorySelection + itemsPerRow < g_itemsInInventory) {

        if (ceil((float)(inventorySelection + 1) / (float)itemsPerRow) < (g_itemsInInventory / g_inventoryRows))
        {
          inventorySelection += itemsPerRow;
        }
        //}
        SoldUIDown = (oldUIDown) ? 6 : 30;
      }
      else
      {
        protag->move_down();
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
        playSound(-1, g_menu_manip_sound, 0);
        if (inventorySelection > 0)
        {
          if (inventorySelection % itemsPerRow != 0)
          {
            inventorySelection--;
          }
        }
        SoldUILeft = (oldUILeft) ? 6 : 30;
      }
      else
      {
        protag->move_left();
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
        playSound(-1, g_menu_manip_sound, 0);
        if (inventorySelection <= g_itemsInInventory)
        {
          // dont want this to wrap around
          if (inventorySelection % itemsPerRow != itemsPerRow - 1)
          {
            inventorySelection++;
          }
        }
        SoldUIRight = (oldUIRight) ? 6 : 30;
      }
      else
      {
        protag->move_right();
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

    if (keystate[bindings[12]] && !old_pause_value && protag_can_move)
    {
      // pause menu
      if (inPauseMenu)
      {
        playSound(-1, g_menu_close_sound, 0);
        inPauseMenu = 0;
        elapsed = 16;
        adventureUIManager->hideInventoryUI();
      }
      else
      {
        playSound(-1, g_menu_open_sound, 0);
        inPauseMenu = 1;
        inventorySelection = 0;
        adventureUIManager->showInventoryUI();
      }
    }

    if (keystate[bindings[12]])
    {
      old_pause_value = 1;
    }
    else
    {
      old_pause_value = 0;
    }
  }
  else
  {
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

  // mapeditor cancel button
  if (keystate[SDL_SCANCODE_X])
  {
    devinput[4] = 1;
  }

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

  if (keystate[bindings[11]] && !old_z_value && !inPauseMenu)
  {
    if (protag_is_talking == 1)
    {
      if (!adventureUIManager->typing)
      {
        adventureUIManager->continueDialogue();
      }
    }
  }
  else if (keystate[bindings[11]] && !old_z_value && inPauseMenu)
  {
    // select item in pausemenu
    // only if we arent running a script
    D(mainProtag->inventory.size());
    if (protag_can_move && adventureUIManager->sleepingMS <= 0 && mainProtag->inventory.size() > 0 && mainProtag->inventory[mainProtag->inventory.size() - 1 - inventorySelection].first->script.size() > 0)
    {
      // call the item's script
      // D(mainProtag->inventory[mainProtag->inventory.size()- 1 -inventorySelection].first->name);
      adventureUIManager->blip = g_ui_voice;
      adventureUIManager->sayings = &mainProtag->inventory[mainProtag->inventory.size() - 1 - inventorySelection].first->script;
      adventureUIManager->talker = protag;
      protag->dialogue_index = -1;
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
  // D(mainProtag->inventory[mainProtag->inventory.size() - 1 -inventorySelection].first->name);
  dialogue_cooldown -= elapsed;
  if (keystate[bindings[11]] && !inPauseMenu)
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
    //protag->ymaxspeed = 145;
  }
  if (keystate[SDL_SCANCODE_LCTRL] && devMode)
  {
    protag->xmaxspeed = 20;
    //protag->ymaxspeed = 20;
  }
  if (keystate[SDL_SCANCODE_CAPSLOCK] && devMode)
  {
    protag->xmaxspeed = 750;
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
  if (keystate[SDL_SCANCODE_C] && devMode)
  {
    devinput[2] = 1;
  }
  if (keystate[SDL_SCANCODE_V] && devMode)
  {
    devinput[3] = 1;
  }
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
    smokeEffect->happen(g_focus->getOriginX(), g_focus->getOriginY(), g_focus->z);
  }

  if (keystate[SDL_SCANCODE_PERIOD] && devMode)
  {
    // make navnode box
    devinput[21] = 1;
  }

  if (keystate[SDL_SCANCODE_ESCAPE])
  {
    if (devMode)
    {
      quit = 1;
    }
    else
    {
      quit = 1;
      // if(inPauseMenu) {
      // 	inPauseMenu = 0;
      // 	adventureUIManager->hideInventoryUI();
      // 	clear_map(g_camera);
      // 	load_map(renderer, "maps/sp-title/sp-title.map", "a");
      // }
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
          I("reloaded a texture of mask");
          I(x->mask_fileaddress);
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
