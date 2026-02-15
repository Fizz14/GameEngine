#include "combat.h"
#include "objects.h"
#include "main.h"
#include "utils.h"
#include <unordered_map>
#include <vector>
#include <regex>

void loadPalette(SDL_Renderer* renderer, const char* filePath, std::vector<Uint32>& palette) {
  // Load the image into a surface
  SDL_Surface* surface = IMG_Load(filePath);
  if (!surface) {
    std::cout << "Unable to load image! SDL_image Error: " << IMG_GetError() << std::endl;
  }

  SDL_PixelFormat* format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888); 
  for (int x = 0; x < 16; ++x) { 
    Uint32 pixel = ((Uint32*)surface->pixels)[x]; 
    Uint8 r, g, b, a; 
    SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a); 
    Uint32 mappedColor = SDL_MapRGBA(format, a, r, g, b); 
    palette.push_back(mappedColor); 
  }

  SDL_FreeSurface(surface);
}

dropInfo::dropInfo() {
}

void runCombatScript(vector<string> combatScript, int turn, combatant* c, string& targeting, vector<int>& patterns, float& damage) {
  int line = 0;

  while(line < combatScript.size()) {

    if(combatScript[line].substr(0,11) == "/addpattern") {
      //M("/addpattern");
      vector<string> x = splitString(combatScript[line], ' ');
      patterns.push_back(stoi(x[1]));
    }
    
    if(combatScript[line].substr(0,10) == "/targeting") {
      //M("/targeting");
      vector<string> x = splitString(combatScript[line], ' ');
      targeting = x[1];
    }

    if(combatScript[line].substr(0,7) == "/damage") {
      //M("/damage");
      vector<string> x = splitString(combatScript[line], ' ');
      damage = stoi(x[1]);
    }

    if(combatScript[line].at(0) == ':')
    {
      //unconditional jump
      //
      // :label
      // #
      // <label>
      //
      // ... will jump to that label
      //
      string s = combatScript[line];
      s.erase(0,1);
      string DIstr = "0";
      DIstr = s.substr(0, s.find(' '));
      s.erase(0, s.find(' ') + 1);
      int DI = 0;
      DI = stoi(DIstr);
      line = DI - 3;
    }
    if(combatScript[line][0] == '#') {
      return;
    }

    if(combatScript[line].substr(0,7) == "/print ") {
      // print from script
      string s = combatScript[line];
      vector<string> x = splitString(s, ' ');
      if(x.size() >= 2) {
        string printMe = "Print from Script: " + s.substr(7);
        M(printMe);
      }
    }

    if (regex_match(combatScript[line], regex("[[:digit:]]+\\-\\>\\[[[:digit:]]+\\]")))
    {
      // write selfdata 5->[4]
      string s = combatScript[line];
      int value = stoi(s.substr(0, s.find('-')));
      s.erase(0, s.find('-') +1);
      string blockstr = s.substr(s.find('['));
      blockstr.pop_back();
      blockstr.erase(0, 1);
      int block = stoi(blockstr);
      c->data[block] = value;
    }

    //write random number to selfdata
    // 0-1000->[4]
    if(regex_match (combatScript[line], regex("[[:digit:]]+\\-+[[:digit:]]+\\-\\>\\[[[:digit:]]+\\]"))) {
      string s = combatScript[line];
      int firstvalue = stoi( s.substr(0, s.find('-')) ); s.erase(0, s.find('-') + 1);
      int secondvalue = stoi( s.substr(0, s.find('-')) ); s.erase(0, s.find('-') + 1);
  
      string blockstr = s.substr(s.find('[')); 
      blockstr.pop_back(); blockstr.erase(0, 1);
      int block = stoi (blockstr);
  
      c->data[block] = rand() % (secondvalue - firstvalue + 1) + firstvalue;
    }

    if (regex_match(combatScript[line], regex("\\[[[:digit:]]+\\]")))
    {
      // read selfdata
      //
      // Make sure to use values in the order low to high
      //
      // [5]
      // *0:waszero
      // *1:wasone
      // #
      // <waszero>
      // /print it was zero
      // #
      // <wasone>
      // /print it was one
      // #
      //
      int j = 1;
      // parse which block of memory we are interested in
      string s = combatScript[line];
      s.erase(0, 1);
      string blockstr = s.substr(0, s.find(']'));
      int block = stoi(blockstr);
      string res = combatScript[line + j];

      while (res.find('*') != std::string::npos)
      {
  
        // parse option
        //  *15 29 -> if data is 15, go to line 29
        string s = combatScript[line + j];
        s.erase(0, 1);
        int condition = stoi(s.substr(0, s.find(':')));
        s.erase(0, s.find(':') + 1);
        int jump = stoi(s);
        if (c->data[block] <= condition)
        {
          line = jump-3;
          break;
        }
        j++;
        if(line + j < combatScript.size()) {
          res = combatScript[line + j]; //was causing heap allocation errors
        } else {
          break;
        }
      }


    }

    if(combatScript[line].substr(0,10) == "/idletext ") {
      //M("Idletext is being interpreted");
      // idletext if the enemy does not attack
      // only is printed if there are no attackpatterns
      // from the script
      // you just get one line
      string s = combatScript[line];
      string idleText = s.substr(9);
      combatUIManager->idleText = c->name + idleText;
    }


    if(combatScript[line].substr(0,17) == "/physicalbarrier ") {
      
      vector<string> x = splitString(combatScript[line], ' ');
      int magnitude = 0;
      int turns = 0;
      if(x.size() > 2) {
        magnitude = stoi(x[1]);
        turns = stoi(x[2]);
      }
      if(g_enemyCombatants.size() > 1) {
        combatUIManager->idleText = stringMultiInject(getLanguageData("CombatPhysicalBarrierTeam"), {c->name, getPossessivePronoun(c), to_string(magnitude)});
      } else {
        combatUIManager->idleText = stringMultiInject(getLanguageData("CombatPhysicalBarrier"), {c->name, to_string(magnitude)});

      }
      
      for(auto user :g_enemyCombatants) {
        bool alreadyHave = 0;
        int mag = 0;
        for(auto &x : user->statuses) {
          if(x.type == status::PHYSICALBARRIER) {
            alreadyHave = 1;
            if(x.magnitude <= magnitude) {
              x.turns = turns;
              x.magnitude = magnitude;
            }
            break;
          }
        }
  
        if(!alreadyHave) {
          statusEntry e;
          e.type = status::PHYSICALBARRIER;
          e.turns = turns;
          e.magnitude = magnitude;
          user->statuses.push_back(e);
        }
      }

    }
    

    line++;
  }

  return;
}

string getPossessivePronoun(combatant* c) {
  string pronoun = "";
  switch(c->gender) {
    case 0:
      return pronounTable[0];
      break;
    case 1:
      return pronounTable[1];
      break;
    case 2:
      return pronounTable[2];
      break;
    case 3:
      return pronounTable[3];
      break;
  }
  return "";
}

string getItemArticle(int itemnum) {
  string ret = getLanguageData("Article0") + " ";
  string vowels = getLanguageData("Vowels");
  string itemName = getLanguageData("I" + to_string(itemnum));
  if(vowels.find(itemName[0]) != string::npos) {
    ret = getLanguageData("Article1") + " ";
  }
  vector<int> itemsWithoutArticles = {8, 9, 13, 20, 22, 23, 34, 35, 38, 39, 46};
  if(std::find(itemsWithoutArticles.begin(), itemsWithoutArticles.end(), itemnum) != itemsWithoutArticles.end()) {
    ret = "";
  }
  return ret;
}

string getReflexivePronoun(combatant* c) {
  string pronoun = "";
  switch(c->gender) {
    case 0:
      return pronounTable[4];
      break;
    case 1:
      return pronounTable[5];
      break;
    case 2:
      return pronounTable[6];
      break;
    case 3:
      return pronounTable[7];
      break;
  }
  return pronoun;
}

string getSubjectivePronoun(combatant* c) {
  string pronoun = "";
  switch(c->gender) {
    case 0:
      return pronounTable[8];
      break;
    case 1:
      return pronounTable[9];
      break;
    case 2:
      return pronounTable[10];
      break;
    case 3:
      return pronounTable[11];
      break;
  }
  return pronoun;
}

string getObjectivePronoun(combatant* c) {
  string pronoun = "";
  switch(c->gender) {
    case 0:
      pronoun = "him";
      break;
    case 1:
      pronoun = "her";
      break;
    case 2:
      pronoun = "it";
      break;
    case 3:
      pronoun = "them";
      break;
  }
  return pronoun;
}

bground::bground() {};

bground::bground(SDL_Renderer* renderer, const char* configFilePath) {
  std::ifstream configFile(configFilePath);
  if (!configFile) {
    std::cerr << "Unable to open config file!" << std::endl;
    return;
  }

  std::string line;
  while (std::getline(configFile, line)) {
    std::istringstream iss(line);
    std::string key;
    if (std::getline(iss, key, ':')) {
      std::string value;
      if (std::getline(iss, value)) {
        if(key == "scene") scene = value;
        else if (key == "texture") texture = std::stoi(value);
        else if (key == "interleaved") interleaved = std::stoi(value);
        else if (key == "horizontalIntensity") horizontalIntensity = std::stof(value);
        else if (key == "horizontalPeriod") horizontalPeriod = std::stof(value);
        else if (key == "verticalIntensity") verticalIntensity = std::stof(value);
        else if (key == "verticalPeriod") verticalPeriod = std::stof(value);
        else if (key == "scrollXMagnitude") scrollXMagnitude = std::stof(value);
        else if (key == "scrollYMagnitude") scrollYMagnitude = std::stof(value);
        else if (key == "paletteFile") {
          std::string paletteFilePath = "resources/static/backgrounds/pallets/" + value + ".qoi";
          //loadPalette(renderer, paletteFilePath.c_str(), palette);
        }
        else if (key == "texture2") texture2 = std::stoi(value);
        else if (key == "interleaved2") interleaved2 = std::stoi(value);
        else if (key == "horizontalIntensity2") horizontalIntensity2 = std::stof(value);
        else if (key == "horizontalPeriod2") horizontalPeriod2 = std::stof(value);
        else if (key == "verticalIntensity2") verticalIntensity2 = std::stof(value);
        else if (key == "vertialPeriod2") vertialPeriod2 = std::stof(value);
        else if (key == "scrollXMagnitude2") scrollXMagnitude2 = std::stof(value);
        else if (key == "scrollYMagnitude2") scrollYMagnitude2 = std::stof(value);
        else if (key == "paletteFile2") {
          std::string paletteFilePath2 = "resources/static/backgrounds/pallets/" + value + ".qoi";
          //loadPalette(renderer, paletteFilePath2.c_str(), palette2);
        }
      }
    }
  }
}


// Warp effect function implementation
void applyWarpEffect(SDL_Texture* texture, SDL_Renderer* renderer, float time, bool interleaved, float horizontalWaveIntensity, float horizontalWavePeriod, float verticalWaveIntensity, float verticalWavePeriod, float scrollXMagnitude, float scrollYMagnitude) {
  int width, height;
  SDL_QueryTexture(texture, NULL, NULL, &width, &height);

  SDL_Texture* warpedTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
  SDL_SetRenderTarget(renderer, warpedTexture);
  SDL_SetTextureBlendMode(warpedTexture, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);

  int scrollX = static_cast<int>((time * scrollXMagnitude)) % width;
  int scrollY = static_cast<int>((time * scrollYMagnitude)) % height;

  for (int y = 0; y < height; ++y) {
    float scaleY = verticalWaveIntensity * sinf(((y) * 0.01f) + time * verticalWavePeriod);
    int srcY = (static_cast<int>((y + scrollY) - scaleY)) % height;
    if (srcY < 0) srcY += height;

    float offsetX = horizontalWaveIntensity * sinf((srcY + time * horizontalWavePeriod) * (2 * M_PI / width));
    if (interleaved && y % 2 == 0) {
      offsetX = -offsetX;
    }

    int srcX = (static_cast<int>(offsetX + scrollX)) % width;
    if (srcX < 0) srcX += width;

    SDL_Rect srcRect1 = {srcX, srcY, width - srcX, 1};
    SDL_Rect destRect1 = {0, y, width - srcX, 1};
    SDL_RenderCopy(renderer, texture, &srcRect1, &destRect1);

    if (srcX > 0) {
      SDL_Rect srcRect2 = {0, srcY, srcX, 1};
      SDL_Rect destRect2 = {width - srcX, y, srcX, 1};
      SDL_RenderCopy(renderer, texture, &srcRect2, &destRect2);
    }
  }

  SDL_SetRenderTarget(renderer, NULL);
  SDL_RenderCopy(renderer, warpedTexture, NULL, NULL);
  SDL_DestroyTexture(warpedTexture);
}

// Palette-cycling function
void cyclePalette(SDL_Surface* source, SDL_Surface* destination, std::vector<Uint32>& palette) {
  // Rotate the palette by one color
  Uint32 firstColor = palette[0];
  for (size_t i = 0; i < palette.size() - 1; ++i) {
    palette[i] = palette[i + 1];
  }
  palette.back() = firstColor;

  SDL_LockSurface(source);
  SDL_LockSurface(destination);
  Uint32* srcPixels = (Uint32*)source->pixels;
  Uint32* dstPixels = (Uint32*)destination->pixels;
  int width = source->w;
  int height = source->h;
  int paletteSize = palette.size();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      uint8_t red = (srcPixels[y * width + x] >> 16) & 0xFF;
      int index = (int)red/25;
      //std::cout << index << std::endl;
      int currentColorIndex = index % paletteSize;
      dstPixels[y * width + x] = palette[currentColorIndex];
    }
  }

  // Blur dstPixels
  std::vector<Uint32> tempPixels(width * height);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int r = 0, g = 0, b = 0, a = 0, count = 0;
      for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
          int ix = x + dx;
          int iy = y + dy;
          if (ix >= 0 && ix < width && iy >= 0 && iy < height) {
            Uint32 pixel = dstPixels[iy * width + ix];
            r += (pixel & 0x00FF0000) >> 16;
            g += (pixel & 0x0000FF00) >> 8;
            b += (pixel & 0x000000FF);
            a += (pixel & 0xFF000000) >> 24;
            count++;
          }
        }
      }
      Uint32 avgR = r / count;
      Uint32 avgG = g / count;
      Uint32 avgB = b / count;
      Uint32 avgA = a / count;
      tempPixels[y * width + x] = (avgA << 24) | (avgR << 16) | (avgG << 8) | avgB;
    }
  }

  // Copy the blurred pixels back to dstPixels
  std::copy(tempPixels.begin(), tempPixels.end(), dstPixels);


  SDL_UnlockSurface(source);
  SDL_UnlockSurface(destination);
}

void drawBackground() {
  combatUIManager->time += elapsed;

  // Cycle the palette every 0.5 seconds
  if (combatUIManager->time - combatUIManager->cycleTime >= 500) {
    //cyclePalette(combatUIManager->sb1, combatUIManager->db1, combatUIManager->loadedBackground.palette);
    combatUIManager->cycleTime = combatUIManager->time;
  }

  if(combatUIManager->tb1 != 0) {
    SDL_DestroyTexture(combatUIManager->tb1);
  }
  combatUIManager->tb1 = SDL_CreateTextureFromSurface(renderer, combatUIManager->db1);



  applyWarpEffect(combatUIManager->tb1, renderer, combatUIManager->time/10000.0f, combatUIManager->loadedBackground.interleaved, combatUIManager->loadedBackground.horizontalIntensity, combatUIManager->loadedBackground.horizontalPeriod, combatUIManager->loadedBackground.verticalIntensity, combatUIManager->loadedBackground.verticalPeriod, combatUIManager->loadedBackground.scrollXMagnitude, combatUIManager->loadedBackground.scrollYMagnitude);
}

void drawSimpleBackground() {
  SDL_RenderCopy(renderer, combatUIManager->scene, NULL, NULL);
}

//careful
combatant::combatant() {
}

combatant::combatant(string ffilename, int fxp) {
  string loadstr;
  loadstr = "resources/static/combatfiles/" + ffilename + ".cmb";
  istringstream file(loadTextAsString(loadstr));

  string temp;
  file >> temp;
  file >> temp;


  name = temp;
  if(name.back() == '\r') {
    name.pop_back();
  }
  
  name = getLanguageData(name.substr(1, name.size()-2));


  filename = ffilename;

  std::transform(name.begin(), name.end(), name.begin(), 
      [](unsigned char c) { if(c == '_') {int e = ' '; return e;} else {return int(c);}  } );

  file >> temp;
  file >> temp;
  gender = stoi(temp);

  file >> temp;
  file >> temp;


  string spritefilevar;
  spritefilevar = "resources/static/combatsprites/" + temp + ".qoi";
  const char* spritefile = spritefilevar.c_str();
  texture = loadTexture(renderer, spritefile);

  file >> temp;
  file >> temp;
  offset = stof(temp);

  file >> temp;
  file >> temp;
  myType = (type)stoi(temp);

  file >> temp;
  file >> l0Attack;

  file >> temp;
  file >> attackGain;
  attackGain -= l0Attack;
  attackGain /= 100; //attack at level 100

  file >> temp;
  file >> l0Defense;

  file >> temp;
  file >> defenseGain;
  defenseGain -= l0Defense;
  defenseGain /= 100; //attack at level 100

  file >> temp;
  file >> l0Strength;

  file >> temp;
  file >> strengthGain;
  strengthGain -= l0Strength;
  strengthGain /= 100; //attack at level 100

  file >> temp;
  file >> l0Critical;

  file >> temp;
  file >> criticalGain;
  criticalGain -= l0Critical;
  criticalGain /= 100; //attack at level 100

  file >> temp;
  file >> l0Skill;

  file >> temp;
  file >> skillGain;
  skillGain -= l0Skill;
  skillGain /= 100; //attack at level 100

  file >> temp;
  file >> l0Soul;

  file >> temp;
  file >> soulGain;
  soulGain -= l0Soul;
  soulGain /= 100; //attack at level 100

  file >> temp;
  file >> l0Mind;

  file >> temp;
  file >> mindGain;
  mindGain -= l0Mind;
  mindGain /= 100; //attack at level 100

  file >> temp;
  file >> l0Recovery;

  file >> temp;
  file >> recoveryGain;
  recoveryGain -= l0Recovery;
  recoveryGain /= 100; //attack at level 100

  file >> temp;
  file >> bossPoints; //each bossPoint increases resistance to stuns and blinds by 10%

  file >> temp;
  file >> deathText;

  if(deathText.back() == '\r') {
    deathText.pop_back();
  }

  deathText = getLanguageData(deathText.substr(1, deathText.size()-2));

//  for (char &ch : deathText) {
//    if(ch == '_') {
//      ch = ' ';
//    }
//  }

  file >> temp;
  file >> article;

  file >> temp;
  string parseMe = "";
  file >> parseMe;

  //percent, then index
  
  vector<string> x = splitString(parseMe, ':');
  if(x.size() > 2) {
    droppedItemPercent = stof(x[0]);
    droppedItemType = stoi(x[1]);
    droppedItemIndex = stoi(x[2]);
  }

  file >> temp;
  parseMe = "";
  file >> parseMe;

  x = splitString(parseMe, ':');
  if(x.size() > 1) {
    droppedEquipablePercent = stof(x[0]);
    droppedEquipableIndex = stoi(x[1]);
  }




  // now use combatscripts
  // to determine stuff like what attack to use, who the attack hits, what patterns are used, how much dmg,
  

//  file >> temp;
//  file >> temp; // Read the '{'
//  while (true) {
//    std::getline(file, temp);
//    temp.erase(std::remove(temp.begin(), temp.end(), '\r'), temp.end()); // Remove carriage return if present
//    if(temp.empty()) {continue;}
//    if (temp == "}") break;
//    vector<string> x = splitString(temp, ' ');
//    attackPatterns.push_back({});
//    for(auto y : x) {
//      attackPatterns[attackPatterns.size()-1].push_back(stoi(y));
//    }
//  }

  file >> temp;
  file >> temp; //read the '{'
  while (true) {
    std::getline(file, temp);
    temp.erase(std::remove(temp.begin(), temp.end(), '\r'), temp.end()); // Remove carriage return if present
    if(temp.empty()) {continue;}
    if (temp == "}") break;
    vector<string> x = splitString(temp, ':');
    pair<int, int> a; a.first = stoi(x[0]);
    a.second = stoi(x[1]);
    spiritTree.push_back(a);
  }


  // load combatscript

  loadstr = "resources/static/combatscripts/" + ffilename + ".txt";
  if(PHYSFS_exists(loadstr.c_str())) {
    //M("Loading the combatscript for " + ffilename);
    this->combatScript = loadText(loadstr);
    parseScriptForLabels(combatScript);
    parseScriptForDialogHooks(combatScript);

//    for(auto x : combatScript) {
//      D(x);
//    }
  }


  xp = fxp;
  level = xpToLevel(xp);

  int fw, fh;
  SDL_QueryTexture(texture, NULL, NULL, &fw, &fh);

  width = fw;
  height = fh; 

  width /= 1920.0;
  height /= 1920.0; 
  serial.target = -1;
  serial.action = turnAction::ATTACK;
  serial.actionIndex = -1;

}

combatant::~combatant() {
  SDL_DestroyTexture(texture);
}


itemInfo::itemInfo(string a, int b) {
  name = a;
  targeting = b;
}

itemInfo::itemInfo() {
  name = "";
  targeting = 0;
}

spiritInfo::spiritInfo(string a, int b, int c) {
  name = a;
  targeting = b;
  cost = c;
}

spiritInfo::spiritInfo() {
  name = "";
  targeting = 0;
  cost = 0;
}

unordered_map<int, std::unordered_map<int, itemInfo>> itemsTable;

std::unordered_map<int, spiritInfo> spiritTable;

vector<string> pronounTable;

void spawnBullets(int pattern, int& accumulator) {
  switch(pattern) {
    case 0:
      {
        int cooldown = 2000;
        if(accumulator >= cooldown) {
          accumulator = 0;
          for(int i = 0; i < 3; i++) {
            miniBullet* a = new miniBullet();
            a->texture = combatUIManager->bulletTexture;
            a->red = 0;
          }
        }
        break;
      }
    case 1:
      {
        int cooldown = 2000;
        if(accumulator >= cooldown) {
          accumulator = 0;
          for(int i = 0; i < 1; i++) {
            miniBullet* a = new miniBullet();
            a->angle = atan2(combatUIManager->dodgerY - a->y, combatUIManager->dodgerX - a->x);
            a->texture = combatUIManager->bulletTexture;
            a->homing = 1;
            a->blue = 0;
          }
        }
      }
    case 2:
      {
        int cooldown = 2000;
        if(accumulator >= cooldown) {
          accumulator = 0;
          for(int i = 0; i < 3; i++) {
            miniBullet* a = new miniBullet();
            a->x = SCREEN_WIDTH + SPAWN_MARGIN;
            a->y = rng(0, SCREEN_HEIGHT);
            a->angle = M_PI;
            a->texture = combatUIManager->bulletTexture;
            a->green = 0;
          }
        }
        break;
      }
    case 3:
      {
        int cooldown = 2000;
        if (accumulator >= cooldown) {
          accumulator = 0;
          int numBullets = 6; // Number of bullets in the sine wave
          float amplitude = 0.5f; // Amplitude of the sine wave
          float frequency = 0.1f; // Frequency of the sine wave

          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            a->x = -SPAWN_MARGIN; // Start from the left side, slightly off-screen
            a->y = -SPAWN_MARGIN; // Start from the bottom left corner

            // Calculate the angle using the sine function
            float offsetAngle = amplitude * sin(frequency * accumulator + i);
            a->angle = M_PI/4 + offsetAngle; // Sweep up and down

            a->texture = combatUIManager->bulletTexture;
            a->green = 128; // Set a different color for sine-pattern bullets
          }
        }
        break;
      }
    case 4:
      {
        // Stream bullets with sweeping angle pattern
        int cooldown = 600; // Short cooldown for continuous stream
        static float sweepTime = 0;
        if (accumulator >= cooldown) {
          accumulator = 0;
          float amplitude = 0.7f; // Amplitude of the sweep (radians)
          float frequency = 0.6f; // Frequency of the sweep (adjust as needed)
          float baseAngle = -M_PI / 2 + M_PI/4; // Base angle (straight up)

          miniBullet* a = new miniBullet();
          a->x = -SPAWN_MARGIN; // Start from the left side, slightly off-screen
          a->y = SCREEN_HEIGHT + SPAWN_MARGIN; // Start from the bottom left corner

          // Calculate the sweeping angle using the sine function
          float sweepAngle = baseAngle + amplitude * sin(frequency * sweepTime);
          a->angle = sweepAngle;

          a->texture = combatUIManager->bulletTexture;
          a->red = 128; // Set a different color for sine-pattern bullets
          a->blue = 128;
          sweepTime += 1; // Increment sweep time
        }
        break;
      }
    case 5:
      {
        // Shotgun blast pattern
        int cooldown = 1500; // Cooldown between each blast
        int numBullets = 3; // Number of bullets in the shotgun blast
        float spreadAngle = M_PI / 4; // Total spread angle (in radians)

        if (accumulator >= cooldown) {
          accumulator = 0;

          // Center point of the blast (e.g., from the bottom left corner)
          float startX = SCREEN_WIDTH + SPAWN_MARGIN;
          float startY = -SPAWN_MARGIN;

          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            a->x = startX;
            a->y = startY;
            a->velocity = frng(0.2, 0.7);
            a->w = frng(80, 120);
            a->h = a->w;

            // Calculate the angle for each bullet
            float angle = M_PI* (3.0/4.0) - spreadAngle / 2 + (spreadAngle / (numBullets - 1)) * i;
            angle += frng(-M_PI/6.0, M_PI/6.0);
            a->angle = angle;

            // Set the texture and color for the bullets
            a->texture = combatUIManager->bulletTexture;

            a->red = 255;
            a->blue = 128;
            a->green = 128;
          }
        }
        break;
      }
    case 6:
      {
        // Pattern using exploding bullets
        int cooldown = 2000; // Cooldown between each shot

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->x = SCREEN_WIDTH + SPAWN_MARGIN; // Spawn off-screen
          a->y = rng(0, SCREEN_HEIGHT); // Random y position
          a->angle = -M_PI; // Shoot left
          a->velocity = 0.3; // Set bullet velocity
          a->exploding = true; // Enable explosion feature
          a->numFragments = 6;
          a->explosionTimer = rng(1000, 3000); // Set explosion timer
          a->texture = combatUIManager->bulletTexture;
          a->red = 255;   // Set color for initial bullet
          a->green = 0;
          a->blue = 0;
        }
        break;
      }
    case 7:
      {
        // Pattern using exploding bullets
        int cooldown = 3000; // Cooldown between each shot

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->x = SCREEN_WIDTH + SPAWN_MARGIN; // Spawn off-screen
          a->y = rng(0, SCREEN_HEIGHT); // Random y position
          a->angle = -M_PI; // Shoot left
          a->velocity = 0.3; // Set bullet velocity
          a->exploding = 2; // Enable explosion feature
          a->numFragments = 3;
          a->fragSize = 0.75;
          a->explosionTimer = rng(1000, 3000); // Set explosion timer
          a->texture = combatUIManager->bulletTexture;
          a->red = 128;   // Set color for initial bullet
          a->green = 128;
          a->blue = 128;
          a->randomExplodeAngle = 1;
        }
        break;
      }
    case 8:
      {
        int cooldown = 800;

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->x = rng(0, SCREEN_WIDTH);
          a->y = -SPAWN_MARGIN;
          a->angle = M_PI/2;
          a->velocity = 0;
          a->acceleration = 0.003;
          a->texture = combatUIManager->bulletTexture;
          a->red = 0;
          a->green = 128;
          a->blue = 128;
        }
        break;
      }
    case 9:
      {
        int cooldown = 1200;

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->x = rng(0, SCREEN_WIDTH);
          a->y = -SPAWN_MARGIN;
          a->angle = M_PI/2;
          a->velocity = 2;
          a->acceleration = -0.002;
          a->texture = combatUIManager->bulletTexture;
          a->red = 128;
          a->green = 128;
          a->blue = 0;
        }
        break;
      }
    case 10:
      {
        int cooldown = 1200;

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->velocity = 1.6;
          a->acceleration = -0.0015;
          a->texture = combatUIManager->bulletTexture;
          a->red = 0;
          a->green = 128;
          a->blue = 128;
        }
        break;
      }
    case 11:
      {
        // Offscreen radial burst pattern
        int cooldown = 2500;
        if (accumulator >= cooldown) {
          accumulator = 0;
          int numBullets = 18; // Number of bullets in the radial burst
          float spawnX, spawnY;
          // Ensure bullets spawn completely offscreen
          if (rng(0, 1)) {
            spawnX = (rng(0, 1)) ? -SPAWN_MARGIN : SCREEN_WIDTH + SPAWN_MARGIN;
            spawnY = rng(-SPAWN_MARGIN, SCREEN_HEIGHT + SPAWN_MARGIN);
          } else {
            spawnX = rng(-SPAWN_MARGIN, SCREEN_WIDTH + SPAWN_MARGIN);
            spawnY = (rng(0, 1)) ? -SPAWN_MARGIN : SCREEN_HEIGHT + SPAWN_MARGIN;
          }

          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            a->x = spawnX;
            a->y = spawnY;
            a->angle = i * (2 * M_PI / numBullets); // Spread bullets evenly in a circle
            a->texture = combatUIManager->bulletTexture;
            a->red = 255;
            a->green = 255;
            a->blue = 0;
          }
        }
        break;
      }

    case 12:
      {
        // Offscreen radial burst pattern
        int cooldown = 2500;
        if (accumulator >= cooldown) {
          accumulator = 0;
          int numBullets = 12; // Number of bullets in the radial burst
          float spawnX, spawnY;
          // Ensure bullets spawn completely offscreen
          if (rng(0, 1)) {
            spawnX = (rng(0, 1)) ? -SPAWN_MARGIN : SCREEN_WIDTH + SPAWN_MARGIN;
            spawnY = rng(-SPAWN_MARGIN, SCREEN_HEIGHT + SPAWN_MARGIN);
          } else {
            spawnX = rng(-SPAWN_MARGIN, SCREEN_WIDTH + SPAWN_MARGIN);
            spawnY = (rng(0, 1)) ? -SPAWN_MARGIN : SCREEN_HEIGHT + SPAWN_MARGIN;
          }

          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            a->x = spawnX;
            a->y = spawnY;
            a->angle = i * (2 * M_PI / numBullets); // Spread bullets evenly in a circle
            a->texture = combatUIManager->bulletTexture;
            a->red = 255;
            a->green = 255;
            a->blue = 0;
          }
          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            a->x = spawnX;
            a->y = spawnY;
            a->velocity = 0.2;
            a->angle = i * (2 * M_PI / numBullets); // Spread bullets evenly in a circle
            a->angle += M_PI/numBullets;
            a->texture = combatUIManager->bulletTexture;
            a->red = 255;
            a->green = 255;
            a->blue = 0;
          }
        }
        break;
      }

    case 13:
      {
        // Wave pattern
        int cooldown = 1800;
        static float waveTime = 0;
        if (accumulator >= cooldown) {
          accumulator = 0;
          int numBullets = 5; // Number of bullets in the wave
          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            // Ensure bullets spawn completely offscreen
            a->x = SCREEN_WIDTH + SPAWN_MARGIN;
            a->y = (SCREEN_HEIGHT / numBullets) * i;
            a->angle = -M_PI; // Move left
                              // Calculate wave offset
            a->velocity = 0.3 + 0.4 * sin(waveTime + i * M_PI / numBullets);
            a->texture = combatUIManager->bulletTexture;
            a->red = 128;
            a->green = 0;
            a->blue = 255;
          }
          waveTime += 0.5;
        }
        break;
      }

    case 14:
      {
        // Zigzag pattern
        int cooldown = 1500;
        static bool direction = true; // Direction of the zigzag
        if (accumulator >= cooldown) {
          accumulator = 0;
          miniBullet* a = new miniBullet();
          // Ensure bullets spawn completely offscreen
          a->x = direction ? SCREEN_WIDTH + SPAWN_MARGIN : -SPAWN_MARGIN;
          a->y = rng(-SPAWN_MARGIN, SCREEN_HEIGHT + SPAWN_MARGIN);
          a->angle = direction ? -M_PI : 0; // Move left or right
          a->velocity = 0.5;
          a->texture = combatUIManager->bulletTexture;
          a->red = 255;
          a->green = 255;
          a->blue = 128;
          direction = !direction; // Toggle direction
        }
        break;
      }


    case 15:
      {
        // Random scatter pattern
        int cooldown = 2500;
        if (accumulator >= cooldown) {
          accumulator = 0;
          int numBullets = 20; // Number of bullets to scatter
          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            // Ensure bullets spawn completely offscreen
            if (rng(0, 1)) {
              a->x = (rng(0, 1)) ? -SPAWN_MARGIN : SCREEN_WIDTH + SPAWN_MARGIN;
              a->y = rng(-SPAWN_MARGIN, SCREEN_HEIGHT + SPAWN_MARGIN);
            } else {
              a->x = rng(-SPAWN_MARGIN, SCREEN_WIDTH + SPAWN_MARGIN);
              a->y = (rng(0, 1)) ? -SPAWN_MARGIN : SCREEN_HEIGHT + SPAWN_MARGIN;
            }
            a->angle = rng(0, 2 * M_PI); // Random direction
            a->velocity = frng(0.1, 0.5);
            a->texture = combatUIManager->bulletTexture;
            a->red = rng(0, 255);
            a->green = rng(0, 255);
            a->blue = rng(0, 255);
          }
        }
        break;
      }

    case 16:
      {
        // Converging pattern
        int cooldown = 3000;
        if (accumulator >= cooldown) {
          accumulator = 0;
          int numBullets = 5; // Number of bullets converging to a point
          float targetX = SCREEN_WIDTH / 2;
          float targetY = SCREEN_HEIGHT / 2;
          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            // Ensure bullets spawn completely offscreen
            if (rng(0, 1)) {
              a->x = (rng(0, 1)) ? -SPAWN_MARGIN : SCREEN_WIDTH + SPAWN_MARGIN;
              a->y = rng(-SPAWN_MARGIN, SCREEN_HEIGHT + SPAWN_MARGIN);
            } else {
              a->x = rng(-SPAWN_MARGIN, SCREEN_WIDTH + SPAWN_MARGIN);
              a->y = (rng(0, 1)) ? -SPAWN_MARGIN : SCREEN_HEIGHT + SPAWN_MARGIN;
            }
            a->angle = atan2(targetY - a->y, targetX - a->x); // Aim towards the center
            a->velocity = frng(0.2, 0.7);
            a->texture = combatUIManager->bulletTexture;
            a->red = 255;
            a->green = 0;
            a->blue = 255;
          }
        }
        break;
      }

    case 17:
      {
        // L-shaped pattern
        int cooldown = 1800;
        if (accumulator >= cooldown) {
          accumulator = 0;
          miniBullet* a = new miniBullet();
          // Ensure bullets spawn completely offscreen
          a->x = rng(-SPAWN_MARGIN, SCREEN_WIDTH + SPAWN_MARGIN);
          a->y = -SPAWN_MARGIN;
          a->angle = M_PI / 2; // Move downward
          a->velocity = 0.4;
          a->texture = combatUIManager->bulletTexture;
          a->red = 128;
          a->green = 255;
          a->blue = 0;
        }
        break;
      }

    case 18:
      {
        // Star pattern
        int cooldown = 9000;
        if (accumulator >= cooldown) {
          accumulator = 0;
          int numBullets = 6; // Number of bullets in the star pattern
          float centerX = SCREEN_WIDTH / 2;
          float centerY = SCREEN_HEIGHT / 2;
          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            // Ensure bullets spawn completely offscreen
            a->x = (centerX + cos(i * 2 * M_PI / numBullets) * (SCREEN_WIDTH / 2 + SPAWN_MARGIN));
            a->y = (centerY + sin(i * 2 * M_PI / numBullets) * (SCREEN_HEIGHT / 2 + SPAWN_MARGIN));
            //a->angle = atan2(centerY - a->y, centerX - a->x); // Aim towards the center
            a->velocity = 0.1;
            a->texture = combatUIManager->bulletTexture;
            a->red = 255;
            a->green = 128;
            a->blue = 128;
            a->exploding = 1;
            a->explosionTimer = 5300;
            a->spinSpeed = 0.001;
            a->spinAngle = i * 2 * M_PI / numBullets;
            a->radius = 512;
          }
        }
        break;
      }
    case 19:
      {
        // Sin wave pattern for the emitter's x position
        int cooldown = 800; // Time between each bullet
        static float time = 0; // Time variable for the sine wave
        float amplitude = SCREEN_WIDTH / 2; // Amplitude of the sine wave
        float frequency = 0.4f; // Frequency of the sine wave

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->x = SCREEN_WIDTH / 2 + amplitude * sin(frequency * time); // Sin wave x position
          a->y = SCREEN_HEIGHT + SPAWN_MARGIN; // Start from the bottom
          a->angle = -M_PI / 2; // Move upward
          a->velocity = 0.5;
          a->texture = combatUIManager->bulletTexture;
          a->red = 128;
          a->green = 255;
          a->blue = 128;

          // Increment time for the next bullet to create the sin wave effect
          time += 1;
        }
        break;
      }
    case 20:
      {
        // Spinning spiral pattern with bullets spawning offscreen
        int cooldown = 300;
        static float phase = 0;
        if (accumulator >= cooldown) {
          accumulator = 0;
          float centerX = SCREEN_WIDTH / 2;
          float centerY = SCREEN_HEIGHT / 2;
          int numBullets = 5;
          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            float angle = phase + i * (2 * M_PI / numBullets);
            float radius = SCREEN_WIDTH / 2 + SPAWN_MARGIN; // Spawn offscreen
            a->x = centerX + cos(angle) * radius;
            a->y = centerY + sin(angle) * radius;
            a->angle = angle + M_PI / 2;
            a->texture = combatUIManager->bulletTexture;
            a->red = 128;
            a->green = 128;
            a->blue = 255;
            a->spinSpeed = 0.001;
            a->spinAngle = angle;
            a->radius = radius;
            a->centerX = centerX;
            a->centerY = centerY;
          }
          phase += 0.1;
        }
        break;
      }
    case 21:
      {
        // Random wave pattern with bullets spawning offscreen
        int cooldown = 200; // Time between each bullet
        static float phase = 0; // Time variable for the sine wave
        float amplitude = SCREEN_WIDTH / 4; // Amplitude of the sine wave
        float frequency = 0.05f; // Frequency of the sine wave

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->x = rng(0, SCREEN_WIDTH); // Random x position along the bottom edge
          a->y = SCREEN_HEIGHT + SPAWN_MARGIN; // Start from the bottom offscreen
          a->angle = -M_PI / 2; // Move upward
          a->velocity = 0.5;
          a->texture = combatUIManager->bulletTexture;
          a->red = 128;
          a->green = 255;
          a->blue = 128;

          // Increment phase for the next bullet to create the wave effect
          phase += 1;
        }
        break;
      }
    case 22:
      {
        // Concentric circles pattern
        int cooldown = 2000;
        if (accumulator >= cooldown) {
          accumulator = 0;
          float centerX = SCREEN_WIDTH / 2;
          float centerY = SCREEN_HEIGHT / 2;
          int numCircles = 3;
          int bulletsPerCircle = 4;
          float initialRadius = 560;
          for (int j = 0; j < numCircles; j++) {
            for (int i = 0; i < bulletsPerCircle; i++) {
              miniBullet* a = new miniBullet();
              float angle = i * 2 * M_PI / bulletsPerCircle;
              float radius = initialRadius + j * 50;
              a->x = centerX + cos(angle) * radius;
              a->y = centerY + sin(angle) * radius;
              a->angle = angle + M_PI / 2;
              a->texture = combatUIManager->bulletTexture;
              a->numFragments = 0;
              a->red = 50;
              a->green = 190;
              a->blue = 180;
              a->spinSpeed = 0.0001 + j * 0.0002;
              a->spinAngle = angle;
              a->radius = radius;
              a->centerX = centerX;
              a->centerY = centerY;
              a->velocity = 0.05;
            }
          }
        }
        break;
      }
    case 23:
      {
        int cooldown = 100000;
        if(accumulator >= cooldown) {
          accumulator = 0;
          for(int i = 0; i < 3; i++) {
            miniBullet* a = new miniBullet();
            a->angle = atan2(512 - a->y, 512 - a->x);
            a->texture = combatUIManager->bulletTexture;
            a->blue = 0;
            a->green = 80;
            a->canBounce = 1;
            a->gravityAccelY = 0.02;

          }
        }
        break;
      }
    case 24:
      {
        int cooldown = 100000;
        if(accumulator >= cooldown) {
          accumulator = 0;
          for(int i = 0; i < 1; i++) {
            miniBullet* a = new miniBullet();
            a->texture = combatUIManager->bulletTexture;
            a->blue = 80;
            a->angle = atan2(512 - a->y, 512 - a->x);
            a->green = 0;
            a->canBounce = 1;
            a->velocity = 0.25;
            a->w = 250;
            a->h = 250;

          }
        }
        break;
      }
    case 25:
      {
        int cooldown = 3500;
        if(accumulator >= cooldown) {
          accumulator = 0;
          for(int i = 0; i < 1; i++) {
            miniBullet* a = new miniBullet();
            a->texture = combatUIManager->bulletTexture;
            a->red = 80;
            a->blue = 0;
            a->canBounce = 1;
            a->velocity = 0.25;
            a->w = 150;
            a->h = 150;
            a->exploding = 2;
            a->numFragments = 4;
            a->fragSize = 0.5;
            a->completelyRandomExplodeAngle =1;
            a->explosionTimer = rng(700, 2200);
          }
        }
        break;
      }
    case 26:
      {
        // simple come from the right pattern
        int cooldown = 5000; // Cooldown between each shot

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->x = SCREEN_WIDTH + SPAWN_MARGIN; // Spawn off-screen
          float rand = rng(0, SCREEN_HEIGHT);
          a->y = rand; // Random y position
          a->angle = -M_PI; // Shoot left
          a->velocity = 0.2; // Set bullet velocity
          a->texture = combatUIManager->bulletTexture;
          a->red = 128;   // Set color for initial bullet
          a->green = 128;
          a->blue = 128;
          a->randomExplodeAngle = 1;
        }
        break;
      }
    case 27:
      { 
        //slower version of pattern 10
        int cooldown = 1200;

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->velocity = 0.6;
          a->acceleration = -0.0015;
          a->texture = combatUIManager->bulletTexture;
          a->red = 0;
          a->green = 128;
          a->blue = 128;
        }
        break;
      }
    case 28:
      {
        // faster version of 26
        int cooldown = 2500; // Cooldown between each shot

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->x = SCREEN_WIDTH + SPAWN_MARGIN; // Spawn off-screen
          a->y = rng(0, SCREEN_HEIGHT); // Random y position
          a->angle = -M_PI; // Shoot left
          a->velocity = 0.2; // Set bullet velocity
          //a->exploding = 2; // Enable explosion feature
          //a->numFragments = 3;
          //a->fragSize = 0.75;
          //a->explosionTimer = rng(1000, 3000); // Set explosion timer
          a->texture = combatUIManager->bulletTexture;
          a->red = 128;   // Set color for initial bullet
          a->green = 128;
          a->blue = 128;
          a->randomExplodeAngle = 1;
        }
        break;
      }
    case 29:
      {
        // double come from right
        int cooldown = 1000; // Cooldown between each shot

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->x = SCREEN_WIDTH + SPAWN_MARGIN; // Spawn off-screen
          float rand = rng(0, SCREEN_HEIGHT);
          a->y = rand; // Random y position
          a->angle = -M_PI; // Shoot left
          a->velocity = 0.2; // Set bullet velocity
          a->texture = combatUIManager->bulletTexture;
          a->red = 128;   // Set color for initial bullet
          a->green = 128;
          a->blue = 128;
          a->randomExplodeAngle = 1;

          a = new miniBullet();
          a->x = SCREEN_WIDTH + SPAWN_MARGIN; // Spawn off-screen
          a->sleepMS = 1000;
          a->y = rand; // Random y position
          a->angle = -M_PI; // Shoot left
          a->velocity = 0.2; // Set bullet velocity
          a->texture = combatUIManager->bulletTexture;
          a->red = 128;   // Set color for initial bullet
          a->green = 128;
          a->blue = 128;
          a->randomExplodeAngle = 1;
        }
        break;
      }
    case 30:
      {
        // Concentric circles pattern
        int cooldown = 4000;
        if (accumulator >= cooldown) {
          accumulator = 0;
          float centerX = SCREEN_WIDTH / 2;
          float centerY = SCREEN_HEIGHT / 2;
          int numCircles = 3;
          int bulletsPerCircle = 4;
          float initialRadius = 560;
          for (int j = 0; j < numCircles; j++) {
            for (int i = 0; i < bulletsPerCircle; i++) {
              miniBullet* a = new miniBullet();
              float angle = i * 2 * M_PI / bulletsPerCircle;
              float radius = initialRadius + j * 50;
              a->x = centerX + cos(angle) * radius;
              a->y = centerY + sin(angle) * radius;
              a->angle = angle + M_PI / 2;
              a->texture = combatUIManager->bulletTexture;
              a->numFragments = 0;
              a->red = 50;
              a->green = 190;
              a->blue = 180;
              a->spinSpeed = 0.0001 + j * 0.0002;
              a->spinAngle = angle;
              a->radius = radius;
              a->centerX = centerX;
              a->centerY = centerY;
              a->velocity = 0.45;
            }
          }
        }
        break;
      }
  }
}

void initTables() {
  {
    pronounTable.push_back(getLanguageData("PossessivePronoun0"));
    pronounTable.push_back(getLanguageData("PossessivePronoun1"));
    pronounTable.push_back(getLanguageData("PossessivePronoun2"));
    pronounTable.push_back(getLanguageData("PossessivePronoun3"));

    pronounTable.push_back(getLanguageData("ReflexivePronoun0"));
    pronounTable.push_back(getLanguageData("ReflexivePronoun1"));
    pronounTable.push_back(getLanguageData("ReflexivePronoun2"));
    pronounTable.push_back(getLanguageData("ReflexivePronoun3"));

    pronounTable.push_back(getLanguageData("SubjectivePronoun0"));
    pronounTable.push_back(getLanguageData("SubjectivePronoun1"));
    pronounTable.push_back(getLanguageData("SubjectivePronoun2"));
    pronounTable.push_back(getLanguageData("SubjectivePronoun3"));
  }

  {
    // 0 -> enemy targeted
    // 1 -> ally targeted
    // 2 -> untargeted
    // 3 -> untargeted, usable outside of combat (Picnicbox)
    // 4 -> ally targeted, in-combat only (e.g. Rockyroad)

    string diskPrefix = getLanguageData("DiskPrefix");
    itemsTable[0][0] = itemInfo("Tofu", 1);
    for(int i = 0; i < 6; i++) {
      string name = diskPrefix + to_string(i) + " " + getLanguageData("S" + to_string(i));
      D(name);
      itemsTable[1][i] = itemInfo(name, 1);
    }
    itemsTable[2][0] = itemInfo("Pistol", 1);
    itemsTable[3][0] = itemInfo("Bomb", 0);

//    itemsTable[0] = itemInfo(getLanguageData("I0"), 1); //bandage
//    itemsTable[1] = itemInfo(getLanguageData("I1"), 2); // bomb
//    itemsTable[2] = itemInfo(getLanguageData("I2"), 2); // S.Bomb
//    itemsTable[3] = itemInfo(getLanguageData("I3"), 2); // U.Bomb
//    itemsTable[4] = itemInfo(getLanguageData("I4"), 0); // Grenade
//    itemsTable[5] = itemInfo(getLanguageData("I5"), 0); // S.Grenade
//    itemsTable[6] = itemInfo(getLanguageData("I6"), 0); // U.Grenade
//    itemsTable[7] = itemInfo(getLanguageData("I7"), 0); // Stickybomb
//    itemsTable[8] = itemInfo(getLanguageData("I8"), 1); // Tofu
//    itemsTable[9] = itemInfo(getLanguageData("I9"), 1); // Noodles
//    itemsTable[10] = itemInfo(getLanguageData("I10"), 1); // Avocado
//    itemsTable[11] = itemInfo(getLanguageData("I11"), 1); // Pretzel
//    itemsTable[12] = itemInfo(getLanguageData("I12"), 1); // Donut
//    itemsTable[13] = itemInfo(getLanguageData("I13"), 1); // Nutmilk
//    itemsTable[14] = itemInfo(getLanguageData("I14"), 1); // Sandwich
//    itemsTable[15] = itemInfo(getLanguageData("I15"), 1); // TVDinner
//    itemsTable[16] = itemInfo(getLanguageData("I16"), 1); // Burger
//    itemsTable[17] = itemInfo(getLanguageData("I17"), 1); // Grilledcheese
//    itemsTable[18] = itemInfo(getLanguageData("I18"), 3); // Picnicbox
//    itemsTable[19] = itemInfo(getLanguageData("I19"), 1); // Bagel
//    itemsTable[20] = itemInfo(getLanguageData("I20"), 1); // Pasta
//    itemsTable[21] = itemInfo(getLanguageData("I21"), 1); // Pickle
//    itemsTable[22] = itemInfo(getLanguageData("I22"), 1); // OrangeJuice
//    itemsTable[23] = itemInfo(getLanguageData("I23"), 1); // Rice
//    itemsTable[24] = itemInfo(getLanguageData("I24"), 1); // Apple
//    itemsTable[25] = itemInfo(getLanguageData("I25"), 1); // Toast
//    itemsTable[26] = itemInfo(getLanguageData("I26"), 1); // Cookie
//    itemsTable[27] = itemInfo(getLanguageData("I27"), 3); // DeluxePicnic
//    itemsTable[28] = itemInfo(getLanguageData("I28"), 3); // Weddingcake
//    itemsTable[29] = itemInfo(getLanguageData("I29"), 1); // Icecream
//    itemsTable[30] = itemInfo(getLanguageData("I30"), 0); // Snare
//    itemsTable[31] = itemInfo(getLanguageData("I31"), 0); // Electrosnare
//    itemsTable[32] = itemInfo(getLanguageData("I32"), 0); // Herbicide
//    itemsTable[33] = itemInfo(getLanguageData("I33"), 0); // HerbicideX
//    itemsTable[34] = itemInfo(getLanguageData("I34"), 0); // Bugspray
//    itemsTable[35] = itemInfo(getLanguageData("I35"), 0); // PremiumBugspray
//    itemsTable[36] = itemInfo(getLanguageData("I36"), 0); // Jammer
//    itemsTable[37] = itemInfo(getLanguageData("I37"), 0); // RetroJammer
//    itemsTable[38] = itemInfo(getLanguageData("I38"), 0); // Acid
//    itemsTable[39] = itemInfo(getLanguageData("I39"), 0); // IndustrialAcid
//    itemsTable[40] = itemInfo(getLanguageData("I40"), 0); // Strobe 
//    itemsTable[41] = itemInfo(getLanguageData("I41"), 0); // Discostrobe
//    itemsTable[42] = itemInfo(getLanguageData("I42"), 0); // Offering
//    itemsTable[43] = itemInfo(getLanguageData("I43"), 0); // ChaoticOffering
//    itemsTable[44] = itemInfo(getLanguageData("I44"), 0); // Spike
//    itemsTable[45] = itemInfo(getLanguageData("I45"), 0); // GoldenSpike
//    itemsTable[46] = itemInfo(getLanguageData("I46"), 0); // Chemicalwaste
//    itemsTable[47] = itemInfo(getLanguageData("I47"), 0); // Steeltrap
//    itemsTable[48] = itemInfo(getLanguageData("I48"), 4); // Rockyroad
//    itemsTable[49] = itemInfo(getLanguageData("I49"), 0); // Shrinkray
//    itemsTable[50] = itemInfo(getLanguageData("I50"), 1); // Healthium
//    itemsTable[51] = itemInfo(getLanguageData("I51"), 1); // Attackase
//    itemsTable[52] = itemInfo(getLanguageData("I52"), 1); // Defendine
//    itemsTable[53] = itemInfo(getLanguageData("I53"), 1); // Critium
//    itemsTable[54] = itemInfo(getLanguageData("I54"), 1); // Skillium
//    itemsTable[55] = itemInfo(getLanguageData("I55"), 1); // Soulium
//    itemsTable[56] = itemInfo(getLanguageData("I56"), 1); // Mindium
//    itemsTable[57] = itemInfo(getLanguageData("I57"), 1); // Recoverum
//    itemsTable[58] = itemInfo(getLanguageData("I58"), 1); // UnstableConcoction
//    itemsTable[59] = itemInfo(getLanguageData("I59"), 2); // Devbomb
   

  }

  {
    // 0 -> enemy targeted
    // 1 -> ally targeted
    // 2 -> untargeted
    // 3 -> ally targeted, in-combat only (e.g., bestow, so bestow can't be used in exploration mode)
    //name, targeting, cost
    spiritTable[0] = spiritInfo(getLanguageData("S0"), 0, 1); //Debug
    spiritTable[1] = spiritInfo(getLanguageData("S1"), 2, 1); //Harden
    spiritTable[2] = spiritInfo(getLanguageData("S2"), 0, 1); //Tackle
    spiritTable[3] = spiritInfo(getLanguageData("S3"), 1, 3); //Coffee
    spiritTable[4] = spiritInfo(getLanguageData("S4"), 2, 2); //Chant
    spiritTable[5] = spiritInfo(getLanguageData("S5"), 0, 1); //Inspect
    spiritTable[6] = spiritInfo(getLanguageData("S6"), 0, 2); //Taunt
    spiritTable[7] = spiritInfo(getLanguageData("S7"), 0, 2); //Slime
    spiritTable[8] = spiritInfo(getLanguageData("S8"), 2, 2); //Synchronize
    spiritTable[9] = spiritInfo(getLanguageData("S9"), 1, 6); //Optimize, all
    spiritTable[10] = spiritInfo(getLanguageData("S10"), 0, 4); //Ignite, Dafua
    spiritTable[11] = spiritInfo(getLanguageData("S11"), 0, 6); //Exploit, Neheten (dumb idea)
    spiritTable[12] = spiritInfo(getLanguageData("S12"), 3, 2); //Bestow, Fomm
    spiritTable[13] = spiritInfo(getLanguageData("S13"), 0, 5); //Curse, Blish
    spiritTable[14] = spiritInfo(getLanguageData("S14"), 0, 3); //Finish, Fomm
    spiritTable[15] = spiritInfo(getLanguageData("S15"), 0, 5); //Exhaust, Neheten
    spiritTable[16] = spiritInfo(getLanguageData("S16"), 0, 5); //Scary Face, Blish
    spiritTable[17] = spiritInfo(getLanguageData("S17"), 0, 5); //Combust, Dafua
  }
}

void initCombat() {

}

int xpToLevel(int xp) {
  int baseXP = 100;
  int level = 0;
  int totalXP = baseXP;

  while(xp >= totalXP) {
    level++;
    totalXP+= static_cast<int>(baseXP * pow(1.6, level - 1));
  }
  if(level > 100) {
    level = 100;
  }

  return level;
}

int levelToXp(int level) {
  if(level == 0) {
    return 0;
  }
  if(level > 100) {
    level = 100;
  }
  int baseXP = 100;
  int totalXP = baseXP;

  for (int i = 1; i < level; i++) {
    totalXP += static_cast<int>(baseXP * std::pow(1.4, i - 1));
  }

  return totalXP;
}

int useItem(int type, int index, int target, combatant* user) {

  switch(type) {
    case 0:
      {
        //use food
        break;
      }
    case 1:
      {
        //use disk
        
        vector<string> script = {"/teach " + to_string(index) + " " + to_string(target) , "#"};
        D(script[0]);

        adventureUIManager->ownScript = script;
        adventureUIManager->dialogue_index = -1;
        adventureUIManager->useOwnScriptInsteadOfTalkersScript = 1;
        adventureUIManager->sleepingMS = 0;
        protag_is_talking = 1;
        g_forceEndDialogue = 0;
        adventureUIManager->continueDialogue();

        //if they canceled out, don't remove the item

        break;
      }
    case 2:
      {
        //use equipment
        break;
      }
    case 3:
      {
        //use combat item (like a disposable bomb, or a healing potion)
        break;
      }
  }
  return 0; //this means the item was used up
}

  //old system, here for reference
//  switch(item) {
//    case 0:
//      {
//        //Bandage
//        int mag = 8.0f * frng(0.85,1.15) + 3 * (user->curSkill/100.0f);
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[0].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 1:
//      {
//        //Bomb
//        int mag = 25.0f * frng(0.85, 1.15) + (user->curSkill/100.0f)*50;
//        for(int i = 0; i < g_enemyCombatants.size(); i++) {
//          int thisMag = mag - g_enemyCombatants[i]->curDefense;
//          if(thisMag <0) {thisMag = 0;}
//          g_enemyCombatants[i]->health -= thisMag;
//          g_enemyCombatants[i]->damageTakenThisTurn += thisMag;
//          user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I1")});
//          combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//          combatUIManager->currentText = "";
//          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//          combatUIManager->dialogProceedIndicator->y = 0.25;
//          combatant* e = g_enemyCombatants[i];
//          if(e->health <= 0) {
//            string deathmessage = e->name + " " + e->deathText;
//            combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//            g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//            g_deadCombatants.push_back(e);
//            //delete e;
//            i--;
//          }
//        }
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 2:
//      {
//        //Super Bomb
//        int mag = 95.0f * frng(0.85, 1.15) + (user->curSkill/100.0f)*50;
//        for(int i = 0; i < g_enemyCombatants.size(); i++) {
//          int thisMag = mag - g_enemyCombatants[i]->curDefense;
//          if(thisMag <0) {thisMag = 0;}
//          g_enemyCombatants[i]->health -= thisMag;
//          g_enemyCombatants[i]->damageTakenThisTurn += thisMag;
//          user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I2")});
//          combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//          combatUIManager->currentText = "";
//          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//          combatUIManager->dialogProceedIndicator->y = 0.25;
//          combatant* e = g_enemyCombatants[i];
//          if(e->health <= 0) {
//            string deathmessage = e->name + " " + e->deathText;
//            combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//            g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//            g_deadCombatants.push_back(e);
//            //delete e;
//            i--;
//          }
//        }
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 3:
//      {
//        //Mega Bomb
//        int mag = 175.0f * frng(0.85, 1.15) + (user->curSkill/100.0f)*50;
//        for(int i = 0; i < g_enemyCombatants.size(); i++) {
//          int thisMag = mag - g_enemyCombatants[i]->curDefense;
//          if(thisMag <0) {thisMag = 0;}
//          g_enemyCombatants[i]->health -= thisMag;
//          g_enemyCombatants[i]->damageTakenThisTurn += thisMag;
//          user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I3")});
//          combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//          combatUIManager->currentText = "";
//          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//          combatUIManager->dialogProceedIndicator->y = 0.25;
//          combatant* e = g_enemyCombatants[i];
//          if(e->health <= 0) {
//            string deathmessage = e->name + " " + e->deathText;
//            combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//            g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//            g_deadCombatants.push_back(e);
//            //delete e;
//            i--;
//          }
//        }
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 4:
//      {
//        //Grenade
//        int mag = 35.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I4")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 5:
//      {
//        //S.Grenade
//        int mag = 70.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I5")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 6:
//      {
//        //U.Grenade
//        int mag = 150.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I6")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 7:
//      {
//        //Stickybomb
//        int mag = 20.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        mag = 0;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//        string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I7")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        statusEntry se;
//        se.type = status::STICKYBOMBED;
//        se.turns = 1;
//        se.magnitude = 1000;
//        e->statuses.push_back(se);
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 8:
//      {
//        // Tofu
//
//        int mag = 45.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 9:
//      {
//        // Noodles
//
//        int mag = 35.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 10:
//      {
//        // Avocado
//
//        int mag = 25.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 11:
//      {
//        // Pretzel
//
//        int mag = 10.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 12:
//      {
//        // Donut
//
//        int mag = 5.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        int smag = 5.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          g_partyCombatants[target]->sp += smag;
//          if(g_partyCombatants[target]->sp > g_partyCombatants[target]->curMind) {
//            g_partyCombatants[target]->sp = g_partyCombatants[target]->curMind;
//          }
//
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          int value2 = min(smag, g_partyCombatants[target]->curMind - g_partyCombatants[target]->sp);
//          if(user->name == g_partyCombatants[target]->name) {
//            message = stringMultiInject(getLanguageData("HealedForSelf"),{user->name, to_stringF(value)});
//          } else {
//            message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//          }
//          string spmessage = "";
//          if(user->name == g_partyCombatants[target]->name) {
//            spmessage = stringMultiInject(getLanguageData("RestoredSpForSelf"),{user->name, to_stringF(value2)});
//          } else {
//            spmessage = stringMultiInject(getLanguageData("RestoredSpFor"),{user->name, to_stringF(value2), g_partyCombatants[target]->name});
//          }
//          combatUIManager->queuedStrings.push_back(make_pair(spmessage,(combatant*)0));
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 13:
//      {
//        // Nutmilk
//
//        int mag = 12.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 14:
//      {
//        // Sandwich
//
//        int mag = 15.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 15:
//      {
//        // TVDinner
//
//        int mag = 18.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 16: 
//      {
//        // Burger
//
//        int mag = 35.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 17: 
//      {
//        // Grilledcheese
//
//        int mag = 35.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 18: 
//      {
//        // Picnicbox
//
//        int mag = 50.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        int smag = 10 * frng(0.85, 1.15) + 15 * (user->curSkill/100.0f);
//        string message = stringMultiInject(getLanguageData("PicnicboxText"),{user->name});
//        for(int i =0; i <g_partyCombatants.size(); i++) {
//          if(g_partyCombatants[i]->health > 0) {
//            g_partyCombatants[i]->health += mag;
//            g_partyCombatants[i]->sp += smag;
//            
//
//            if(g_partyCombatants[i]->health >= g_partyCombatants[i]->curStrength) {
//              g_partyCombatants[i]->health = floor(g_partyCombatants[i]->curStrength);
//            }
//
//            if(g_partyCombatants[i]->sp >= g_partyCombatants[i]->curMind) {
//              g_partyCombatants[i]->sp = floor(g_partyCombatants[i]->curMind);
//            }
//
//
//            int value = min(mag, g_partyCombatants[i]->curStrength - g_partyCombatants[i]->health);
//            int value2 = min(smag, g_partyCombatants[i]->curMind - g_partyCombatants[i]->sp);
//            string spmessage = stringMultiInject(getLanguageData("Regained"),{g_partyCombatants[i]->name, to_stringF(value), to_stringF(value2)});
//            combatUIManager->queuedStrings.push_back(make_pair(spmessage,(combatant*)0));
//          }
//        }
//
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 19: 
//      {
//        // Bagel
//
//        int mag = 15.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 20: 
//      {
//        // Pasta
//
//        int mag = 15.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 21: 
//      {
//        // Pickle
//
//        int mag = 8.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 22: 
//      {
//        // Orangejuice
//
//        int mag = 10.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 23: 
//      {
//        // Rice
//
//        int mag = 20.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 24: 
//      {
//        // Apple
//
//        int mag = 5.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 25: 
//      {
//        // Toast
//
//        int mag = 5.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 26: 
//      {
//        // Cookie
//
//        int mag = 5.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 27: 
//      {
//        // DeluxePicnicbox
//
//        int mag = 100.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        int smag = 20 * frng(0.85, 1.15) + 15 * (user->curSkill/100.0f);
//        string message = stringMultiInject(getLanguageData("PicnicboxText"),{user->name});
//        for(int i =0; i <g_partyCombatants.size(); i++) {
//          if(g_partyCombatants[i]->health > 0) {
//            g_partyCombatants[i]->health += mag;
//            g_partyCombatants[i]->sp += smag;
//            
//
//            if(g_partyCombatants[i]->health >= g_partyCombatants[i]->curStrength) {
//              g_partyCombatants[i]->health = floor(g_partyCombatants[i]->curStrength);
//            }
//
//            if(g_partyCombatants[i]->sp >= g_partyCombatants[i]->curMind) {
//              g_partyCombatants[i]->sp = floor(g_partyCombatants[i]->curMind);
//            }
//
//
//            int value = min(mag, g_partyCombatants[i]->curStrength - g_partyCombatants[i]->health);
//            int value2 = min(smag, g_partyCombatants[i]->curMind - g_partyCombatants[i]->sp);
//            string spmessage = stringMultiInject(getLanguageData("Regained"),{g_partyCombatants[i]->name, to_stringF(value), to_stringF(value2)});
//            combatUIManager->queuedStrings.push_back(make_pair(spmessage,(combatant*)0));
//          }
//        }
//
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 28: 
//      {
//        // Weddingcake
//
//        int mag = 150.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        int smag = 30 * frng(0.85, 1.15) + 15 * (user->curSkill/100.0f);
//        string message = stringMultiInject(getLanguageData("PicnicboxText"),{user->name});
//        for(int i =0; i <g_partyCombatants.size(); i++) {
//          if(g_partyCombatants[i]->health > 0) {
//            g_partyCombatants[i]->health += mag;
//            g_partyCombatants[i]->sp += smag;
//            
//
//            if(g_partyCombatants[i]->health >= g_partyCombatants[i]->curStrength) {
//              g_partyCombatants[i]->health = floor(g_partyCombatants[i]->curStrength);
//            }
//
//            if(g_partyCombatants[i]->sp >= g_partyCombatants[i]->curMind) {
//              g_partyCombatants[i]->sp = floor(g_partyCombatants[i]->curMind);
//            }
//
//
//            int value = min(mag, g_partyCombatants[i]->curStrength - g_partyCombatants[i]->health);
//            int value2 = min(smag, g_partyCombatants[i]->curMind - g_partyCombatants[i]->sp);
//            string spmessage = stringMultiInject(getLanguageData("Regained"),{g_partyCombatants[i]->name, to_stringF(value), to_stringF(value2)});
//            combatUIManager->queuedStrings.push_back(make_pair(spmessage,(combatant*)0));
//          }
//        }
//
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 29: 
//      {
//        // Icecream
//
//        int mag = 15.0f * frng(0.85, 1.15) + 35 * (user->curSkill/100.0f);
//        //string message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(mag)});
//        string message = "";
//        if(g_partyCombatants[target]->health > 0) {
//          g_partyCombatants[target]->health += mag;
//          int value = min(mag, g_partyCombatants[target]->curStrength - g_partyCombatants[target]->health);
//          message = stringMultiInject(getLanguageData("HealedFor"),{user->name, g_partyCombatants[target]->name, to_stringF(value)});
//        } else {
//          message = stringMultiInject(getLanguageData("TriedToUse"), {user->name, itemsTable[8].name});
//        }
//        combatUIManager->finalText = message;
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
//          g_partyCombatants[target]->health = floor(g_partyCombatants[target]->curStrength);
//        }
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        break;
//      }
//    case 30:
//      {
//        // Snare
//        int mag = 20.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        if(g_enemyCombatants[i]->myType == type::ANIMAL) {
//          mag *= 2;
//        }
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I30")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 31:
//      {
//        // Electrosnare
//        int mag = 100.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        if(g_enemyCombatants[i]->myType == type::ANIMAL) {
//          mag *= 2;
//        }
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I31")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 32:
//      {
//        // Herbicide
//        int mag = 20.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        if(g_enemyCombatants[i]->myType == type::PLANT) {
//          mag *= 2;
//        }
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I32")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 33:
//      {
//        // HerbicideX
//        int mag = 100.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        if(g_enemyCombatants[i]->myType == type::PLANT) {
//          mag *= 2;
//        }
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I33")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 34:
//      {
//        // Bugspray
//        int mag = 20.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        if(g_enemyCombatants[i]->myType == type::BUG) {
//          mag *= 2;
//        }
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I34")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 35:
//      {
//        // PremiumBugspray
//        int mag = 100.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        if(g_enemyCombatants[i]->myType == type::BUG) {
//          mag *= 2;
//        }
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I35")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 36:
//      {
//        // Jammer
//        int mag = 20.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        if(g_enemyCombatants[i]->myType == type::ROBOT) {
//          mag *= 2;
//        }
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I36")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 37:
//      {
//        // RetroJammer
//        int mag = 100.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        if(g_enemyCombatants[i]->myType == type::ROBOT) {
//          mag *= 2;
//        }
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I37")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 38:
//      {
//        // Acid
//        int mag = 20.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        if(g_enemyCombatants[i]->myType == type::ALIEN) {
//          mag *= 2;
//        }
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I38")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 39:
//      {
//        // IndustrialAcid
//        int mag = 100.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        if(g_enemyCombatants[i]->myType == type::ALIEN) {
//          mag *= 2;
//        }
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I39")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 40:
//      {
//        // Strobe
//        int mag = 20.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        if(g_enemyCombatants[i]->myType == type::UNDEAD) {
//          mag *= 2;
//        }
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I40")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 41:
//      {
//        // Discostrobe
//        int mag = 100.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        if(g_enemyCombatants[i]->myType == type::UNDEAD) {
//          mag *= 2;
//        }
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I41")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 42:
//      {
//        // Offering
//        int mag = 20.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        if(g_enemyCombatants[i]->myType == type::GHOST) {
//          mag *= 2;
//        }
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I42")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 43:
//      {
//        // ChaoticOffering
//        int mag = 100.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        if(g_enemyCombatants[i]->myType == type::GHOST) {
//          mag *= 2;
//        }
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I43")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 44:
//      {
//        // Spike
//        int mag = 20.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        if(g_enemyCombatants[i]->myType == type::DEMON) {
//          mag *= 2;
//        }
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I44")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 45:
//      {
//        // GoldenSpike
//        int mag = 100.0f * frng(0.85, 1.15) + (user->curSkill/100.0f) * 50;
//        int i = target;
//        mag -= g_enemyCombatants[i]->curDefense;
//        if(mag <0) { mag = 0;}
//        if(g_enemyCombatants[i]->myType == type::DEMON) {
//          mag *= 2;
//        }
//        g_enemyCombatants[i]->health -= mag;
//        g_enemyCombatants[i]->damageTakenThisTurn += mag;
//        user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I45")});
//        combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//        combatUIManager->currentText = "";
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//        if(e->health <= 0) {
//          string deathmessage = e->name + " " + e->deathText;
//          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//          g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//          g_deadCombatants.push_back(e);
//          //delete e;
//          i--;
//        }
//
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//    case 46:
//      {
//        // Chemicalwaste - blinds enemy
//        int i = target;
//        int bossPoints = g_enemyCombatants[target]->bossPoints;
//        bool succ = 0;
//        if(rng(0,100) > bossPoints * 10) {
//          succ = 1;
//        }
//        string message = "";
//        if(succ) {
//          message = stringMultiInject(getLanguageData("BlindedBy"), {g_enemyCombatants[i]->name, getLanguageData("I46") });
//          combatant* c = g_enemyCombatants[i];
//          bool alreadyHave = 0;
//          for(auto &x : c->statuses) {
//            if(x.type == status::BLINDED) {
//              alreadyHave = 1;
//              if(x.turns < 4) {
//                x.turns = 3;
//              }
//              float chanceToMiss = 0.5;
//              x.magnitude = chanceToMiss;
//              break;
//            }
//          }
//          if(!alreadyHave) {
//            statusEntry e;
//            e.type = status::BLINDED;
//            e.turns = 3;
//            float chanceToMiss = 0.5;
//            e.magnitude = chanceToMiss;
//            c->statuses.push_back(e);
//          }
//
//        } else {
//          message = stringMultiInject(getLanguageData("DidntWorkOn"), {getLanguageData("I45"), g_enemyCombatants[i]->name});
//        }
//
//        combatUIManager->currentText = "";
//        combatUIManager->finalText = message;
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//        combatant* e = g_enemyCombatants[i];
//
//        user->serial.target = 1;
//        break;
//      }
//    case 47:
//      {
//        //Steeltrap
//        int i = target;
//        string message = "";
//        combatant* c = g_enemyCombatants[i];
//        message = stringMultiInject(getLanguageData("SteeltrapUsage"), {user->name, c->name});
//        bool alreadyHave = 0;
//        for(auto &x : c->statuses) {
//          if(x.type == status::STEELTRAPPED) {
//            alreadyHave = 1;
//            if(x.turns < 4) {
//              x.turns = 3;
//            }
//            float chanceToMiss = 0.5;
//            x.magnitude = 5 + user->curSkill;
//            break;
//          }
//        }
//        if(!alreadyHave) {
//          statusEntry e;
//          e.type = status::STEELTRAPPED;
//          e.turns = 3;
//          e.magnitude = 5 + user->curSkill;
//          c->statuses.push_back(e);
//        }
//
//        combatUIManager->currentText = "";
//        combatUIManager->finalText = message;
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//
//        user->serial.target = 1;
//        break;
//      }
//    case 48:
//      {
//        //defenspill
//        combatant*c = g_partyCombatants[target];
//        int magnitude = 5 + 0.5 * user->curSkill;
//        string message = stringMultiInject(getLanguageData("DefenspillUsage"), {c->name, getPossessivePronoun(c), to_string(magnitude)});
//        bool alreadyHave = 0;
//
//        //defenspill can stack
//        if(!alreadyHave) {
//          statusEntry e;
//          e.type = status::DEFENSPILLED;
//          e.turns = 8;
//          e.magnitude = magnitude;
//          c->statuses.push_back(e);
//        }
//
//        combatUIManager->currentText = "";
//        combatUIManager->finalText = message;
//        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//        combatUIManager->dialogProceedIndicator->y = 0.25;
//
//        user->serial.target = 1;
//        break;
//
//
//        break;
//      }
//    case 49:
//      {
//        g_shrinkTurns = 4;
//        break;
//      }
//    case 100:
//      {
//        //Devbomb
//        int mag = 2500.0f * frng(0.85, 1.15) + (user->curSkill * 2);
//
//        if(canSwitchOffDevMode) {
//          if(g_combatInventory.size() < g_maxInventorySize) {
//            g_combatInventory.push_back(30);
//          }
//        }
//
//        for(int i = 0; i < g_enemyCombatants.size(); i++) {
//          int thisMag = mag - g_enemyCombatants[i]->curDefense;
//          if(thisMag <0) {thisMag = 0;}
//          g_enemyCombatants[i]->health -= thisMag;
//          g_enemyCombatants[i]->damageTakenThisTurn += thisMag;
//          user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
//          string message = stringMultiInject(getLanguageData("TookFrom"), {g_enemyCombatants[i]->name, to_stringF(mag), getLanguageData("I1")});
//          combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
//          combatUIManager->currentText = "";
//          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//          combatUIManager->dialogProceedIndicator->y = 0.25;
//          combatant* e = g_enemyCombatants[i];
//          if(e->health <= 0) {
//            string deathmessage = e->name + " " + e->deathText;
//            combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
//            g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
//            g_deadCombatants.push_back(e);
//            //delete e;
//            i--;
//          }
//        }
//        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
//        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
//        user->serial.target = 1;
//        break;
//      }
//  }

//note that statuses are procked after the enemy turn for the enemies and after the
//player turn for the protags.
//
// As such, if a protag's move is meant to decrease an enemy's defense, the move itself
// should have a line like enemy->curDefense = enemy->baseDefense * 0.8;
// and the status prock code should have a similar line, sine curDefense is reset before 
// applying statuses
//
void useSpiritMove(int spiritNumber, int target, combatant* user) {
  user->sp -= spiritTable[spiritNumber].cost;
  switch(spiritNumber) {
    case 0: //test move
      {
        float baseDmg = 10;
        if(g_enemyCombatants[target]->myType == type::DEMON) {
          baseDmg *= 2;
        }

        int mag = (baseDmg + (user->curSoul * 1) );
        mag*= frng(0.8, 1.2);

        if(mag < 0) {mag = 0;}
        g_enemyCombatants[target]->health -= mag;
        g_enemyCombatants[target]->damageTakenThisTurn += mag;
        user->dmgDealtOverFight += min(g_enemyCombatants[target]->health, (int)mag);

        string message = getLanguageData("DebugMoveText");
        message = stringMultiInject(message, {user->name, g_enemyCombatants[target]->name});
        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;
        combatant* e = g_enemyCombatants[target];
        if(e->health < 0) {
          string deathmessage = e->name + " " +  e->deathText;
          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
          g_enemyCombatants.erase(g_enemyCombatants.begin() + target);
          g_deadCombatants.push_back(e);
          //delete e;
        }
        break;
      }
    case 1: //Harden
      {
        //see if we already have the status
        bool alreadyHave = 0;
        int mag = 0;
        for(auto &x : user->statuses) {
          if(x.type == status::TOUGHENED) {
            alreadyHave = 1;
            if(x.turns < 6) {
              x.turns = 6;
              mag = x.magnitude;
            }
            break;
          }
        }

        if(!alreadyHave) {
          statusEntry e;
          e.type = status::TOUGHENED;
          e.turns = 6;
          e.magnitude = 1.1 + (0.5 * user->curSoul);
          mag = e.magnitude;
          user->statuses.push_back(e);
        }

        string message = getLanguageData("HardenMoveText");
        message = stringMultiInject(message, {user->name, to_string((int)mag)});
        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;

        break;
      }
    case 2: //Tackle
      {
        float baseDmg = 18;

        float mag = (baseDmg + (user->curSoul * 1.5) );
        mag*= frng(0.8, 1.2);

        int dmg = mag - g_enemyCombatants[target]->curDefense;
        if(dmg < 0) {dmg = 0;}

        g_enemyCombatants[target]->health -= dmg;
        g_enemyCombatants[target]->damageTakenThisTurn += dmg;
        user->dmgDealtOverFight += min(g_enemyCombatants[target]->health, (int)mag);
 
        int selfdmg = (mag * 0.25) - user->curDefense;
        if(selfdmg < 0) {selfdmg = 0;}
        if(selfdmg > user->health) {selfdmg = user->health;}
        user->health -= selfdmg;

        string message = getLanguageData("TackleMoveText");
        message = stringMultiInject(message, {user->name, g_enemyCombatants[target]->name, to_string(dmg), to_string(selfdmg)});

        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;
        combatant* e = g_enemyCombatants[target];
        if(e->health < 0) {
          string deathmessage = e->name + " " +  e->deathText;
          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
          g_enemyCombatants.erase(g_enemyCombatants.begin() + target);
          g_deadCombatants.push_back(e);
          //delete e;
        }
        break;
      }
    case 3: //Coffee/Tea
      {
        float baseHeal = 2.5;
        float mag = baseHeal + (user->curSoul * 0.5);

        mag*= frng(0.8, 1.2);
        int heal = mag;
        if(heal < 0) {heal = 0;}
        combatant* healedOne = g_partyCombatants[target];
        if(heal + healedOne->health > healedOne->curStrength) {
          heal = healedOne->curStrength - healedOne->health;
        }
        bool healedOneAlive = 0;
        string message = "";
        if(healedOne->health > 0) {
          healedOneAlive = 1;
        }
        if(healedOneAlive) {
          healedOne->health += heal;
          if(healedOne->health > healedOne->curStrength) {
            healedOne->health = floor(healedOne->curStrength);
          }
          message = stringMultiInject(getLanguageData("CoffeeMoveSuccessText"), {healedOne->name, to_string(heal)});
        } else {
          message = stringMultiInject(getLanguageData("CoffeeMoveFailText"), {healedOne->name});
        }


        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;
        break;
      }
    case 4: //Chant
      {
        //see if we already have the status
        bool alreadyHave = 0;
        for(auto &x : user->statuses) {
          if(x.type == status::CHANTED) {
            alreadyHave = 1;
            break;
          }
        }

        if(!alreadyHave) {
          statusEntry e;
          e.type = status::CHANTED;
          e.turns = 1;
          e.magnitude = 100;
          user->statuses.push_back(e);
        }

        string message = stringMultiInject(getLanguageData("ChantMoveText"), {user->name});
        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;

        break;
      }
    case 5: //Inspect
      {
        combatant* e = g_enemyCombatants[target];
        string message = getLanguageData("InspectMoveText0");
        message = stringMultiInject(message, {e->name, to_stringF(e->baseStrength), to_stringF(e->baseAttack), to_stringF(e->baseDefense)});
        string message2 = getLanguageData("InspectMoveText1");
        message2 = stringMultiInject(message2, {getSubjectivePronoun(e), to_string(e->health), to_stringF(e->curStrength), to_stringF(e->curDefense)});
        string message3 = getLanguageData("InspectMoveText2");
        message3 = stringMultiInject(message3, {e->name, getLanguageData("CombatType"+ to_string((int)e->myType)) });



        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;

        combatUIManager->queuedStrings.push_back(make_pair(message2,(combatant*)0));
        combatUIManager->queuedStrings.push_back(make_pair(message3,(combatant*)0));
        break;
      }
    case 6: //Taunt
      {

        //see if they already have the status
        bool alreadyHave = 0;
        combatant* e = g_enemyCombatants[target];
        for(auto &x : e->statuses) {
          if(x.type == status::TAUNTED) {
            alreadyHave = 1;
            if(x.turns < 6) {
              x.turns = 6;
            }
            break;
          }
        }

        if(!alreadyHave) {
          statusEntry se;
          se.type = status::TAUNTED;
          se.turns = 6;
          se.magnitude = 40 + (user->curSoul * 1);
          se.datastr = user->filename;
          e->statuses.push_back(se);
        }

        //string message = user->name + " taunts " + e->name + ".";
        string message = getLanguageData("TauntMoveText");
        message = stringMultiInject(message, {user->name, e->name});


        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;

        break;
      }
    case 7: //Slime
      {
        bool alreadyHave = 0;
        combatant* e = g_enemyCombatants[target];
        int dmg = 0;
        for(auto &x : e->statuses) {
          if(x.type == status::SLIMED) {
            alreadyHave = 1;
            x.magnitude *= (1.3 + (user->curSoul / 25));
            if(x.magnitude > user->curSoul * 4.8) {
              x.magnitude = 50;
            }
            dmg = x.magnitude * frng(0.8, 1.2);
            break;
          }
        }


        if(!alreadyHave) {
          statusEntry se;
          se.type = status::SLIMED;
          se.turns = 1;
          se.magnitude = 8;
          se.datastr = user->filename;
          e->statuses.push_back(se);
          dmg = se.magnitude * frng(0.8, 1.2);
        }

        g_enemyCombatants[target]->health -= dmg;
        g_enemyCombatants[target]->damageTakenThisTurn += dmg;
        user->dmgDealtOverFight += min(g_enemyCombatants[target]->health, (int)dmg);

        //string message = user->name + " slimes " + e->name + " for " + to_string(dmg) + " damage.";
        string message = stringMultiInject(getLanguageData("Slime"), {user->name, e->name, to_string(dmg)});
        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;

        break;
      }
    case 8:
      {
        vector<string> msgs = {};
        for(unsigned int i = 0; i < g_partyCombatants.size(); i++) {
          //see if they already have the status
          bool alreadyHave = 0;
          combatant*e = g_partyCombatants[i];
          for(auto &x : e->statuses) {
            if(x.type == status::SYNCHRONIZED) {
              alreadyHave = 1;
              if(x.turns < 4) {
                x.turns = 4;
              }
              break;
            }
          }
  
          if(!alreadyHave) {
            statusEntry se;
            se.type = status::SYNCHRONIZED;
            se.turns = 4;
            se.magnitude = 0 + (user->curSoul * 0.2);
            se.datastr = user->filename;
            e->statuses.push_back(se);
          }

          string message = e->name + " is in sync with " + getPossessivePronoun(e) + " friends.";
          msgs.push_back(message);
        }

        combatUIManager->finalText = msgs.at(0);
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;

        for(int i = 1; i < msgs.size(); i++) {
          combatUIManager->queuedStrings.push_back(make_pair(msgs[i], (combatant*)0));
        }

        break;
      }
    case 9:
      {
        //Optimize
        
        //revisit this when characters can have debuffs (Exhaust)
      }
    case 10:
      {
        //Ignite
        combatant* e = g_enemyCombatants[target];
        if(e->damageTakenThisTurn > 1) {
          float baseDmg = 12;
  
          float mag = (baseDmg + (user->curSoul * 1.3) );
          mag*= frng(0.8, 1.2);
  
          int dmg = mag - g_enemyCombatants[target]->curDefense;
          if(dmg < 0) {dmg = 0;}
  
          g_enemyCombatants[target]->health -= dmg;
          g_enemyCombatants[target]->damageTakenThisTurn += dmg;
          user->dmgDealtOverFight += min(g_enemyCombatants[target]->health, (int)mag);
   
          string message = getLanguageData("IgniteMove1");
          message = stringMultiInject(message, {user->name, g_enemyCombatants[target]->name, to_string(dmg)});
  
          combatUIManager->finalText = message;
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          combatant* e = g_enemyCombatants[target];
          if(e->health < 0) {
            string deathmessage = e->name + " " +  e->deathText;
            combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
            g_enemyCombatants.erase(g_enemyCombatants.begin() + target);
            g_deadCombatants.push_back(e);
          }
          
        } else {

          bool alreadyHave = 0;
          combatant* e = g_enemyCombatants[target];
          int dmg = 0;
          for(auto &x : e->statuses) {
            if(x.type == status::IGNITED) {
              alreadyHave = 1;
              x.turns = 4;
              break;
            }
          }
  
          if(!alreadyHave) {
            statusEntry se;
            se.type = status::IGNITED;
            se.turns = 4;
            se.magnitude = 5 + user->curSoul * 1.2;
            e->statuses.push_back(se);
            M("Pushed back ignited status");
          }
          string message = getLanguageData("IgniteMove2");
          message = stringMultiInject(message, {user->name, g_enemyCombatants[target]->name});

          combatUIManager->finalText = message;
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
  
        }
        break;
      }
    case 11:
      {
        //punish
        float baseDmg = 6 + 0.4 * user->curSoul;

        combatant* e = g_enemyCombatants[target];
        bool punish = 0;
        
        for(auto x : e->statuses) {
          if(x.type == status::IGNITED) {
            punish = 1;
            baseDmg = 10 + 1.3 * user->curSoul;
          }
        }


        float mag = baseDmg;
        mag*= frng(0.8, 1.2);

        int dmg = mag - g_enemyCombatants[target]->curDefense;
        if(dmg < 0) {dmg = 0;}

        g_enemyCombatants[target]->health -= dmg;
        g_enemyCombatants[target]->damageTakenThisTurn += dmg;
        user->dmgDealtOverFight += min(g_enemyCombatants[target]->health, (int)mag);
 

        string message = getLanguageData("PunishMove");
        message = stringMultiInject(message, {user->name, g_enemyCombatants[target]->name, to_string(dmg)});

        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;
        if(e->health < 0) {
          string deathmessage = e->name + " " +  e->deathText;
          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,e));
          g_enemyCombatants.erase(g_enemyCombatants.begin() + target);
          g_deadCombatants.push_back(e);
          //delete e;
        }
        break;
      }
    case 12:
      {
        //Bestow
        
        //give a teammate an attack boost based on user's defense
        
        //see if we already have the status
        bool alreadyHave = 0;
        combatant* c = g_partyCombatants[target];
        float mag = 0;
        for(auto &x : c->statuses) {
          if(x.type == status::BESTOWED) {
            alreadyHave = 1;
            if(x.turns < 3) {
              x.turns = 3;
            }
            float bonusDefense = user->curDefense - user->baseDefense;
            x.magnitude = 0.5 * user->curSoul + 1.5 * bonusDefense;
            x.magnitude *= frng(0.8,1.2);
            mag = x.magnitude;
            break;
          }
        }

        if(!alreadyHave) {
          statusEntry e;
          e.type = status::BESTOWED;
          e.turns = 3;
          float bonusDefense = user->curDefense - user->baseDefense;
          e.magnitude = 0.5 * user->curSoul + 1.5 * bonusDefense;
          e.magnitude *= frng(0.8,1.2);
          mag = e.magnitude;
          c->statuses.push_back(e);
        }

        string message = getLanguageData("BestowMove");
        c->curAttack += mag;
        message = stringMultiInject(message, {user->name, c->name, to_string((int)mag)});
        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;

        break;
      }

  }
}

//given a combatant, and a statusEntry with duration, type, and magnitude, apply it to them
bool applyStatus(combatant* c, statusEntry* e) {
  switch(e->type) {
    case status::NONE:
      {
        break;
      }
    case status::TOUGHENED:
      {
        M("Apply toughened Status");
        if(e->turns <= 0) {
          //the status wore off
          combatUIManager->finalText = stringMultiInject(getLanguageData("StatusWornOff"), {spiritTable[1].name, c->name});

          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          g_submode = submode::TEXT_STATUS_P;
          return 1;

        } else {
          //the status is applied
          c->curDefense = c->baseDefense * e->magnitude;
          e->turns--;
          //curStatusIndex++;
        }
        break;
      }
    case status::CHANTED:
      {
        if(e->turns <= 0) {
          combatUIManager->finalText = stringMultiInject(getLanguageData("StatusWornOff"), {spiritTable[4].name, c->name});
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          g_submode = submode::TEXT_STATUS_P;
          return 1;
        }
        c->curCritical = 100;
        //curStatusIndex++;
        break;
      }
    case status::TAUNTED:
      {
        if(e->turns <= 0) {
          combatUIManager->finalText = c->name + "'s is no longer Taunted.";
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          g_submode = submode::TEXT_STATUS_E;
          return 1;
        } else {
          e->turns--;
        }
        break;
      }

    case status::SLIMED:
      {
        //complete
        break;
      }
    case status::SYNCHRONIZED:
      {
        if(e->turns <= 0) {
          //combatUIManager->finalText = c->name + " is out-of-sync.";
          //combatUIManager->finalText = stringMultiInject(getLanguageData("SynchronizeOverText"), {c->name});
          combatUIManager->finalText = stringMultiInject(getLanguageData("StatusWornOff"), {spiritTable[8].name, c->name});
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          g_submode = submode::TEXT_STATUS_P;
          return 1;
        }
        c->curSoul *= e->magnitude;
        e->turns--;

        break;
      }
    case status::STICKYBOMBED:
      {
        if(e->turns == 1) {
          int damage = (frng(0.8,1.2) * e->magnitude) - c->curDefense;
          if(damage <0) {damage =0;}
          c->health -= damage;
          c->damageTakenThisTurn += damage;
  
          //combatUIManager->finalText = c->name + " is out-of-sync.";
          combatUIManager->finalText = stringMultiInject(getLanguageData("TookFrom"), {c->name, to_stringF(e->magnitude), getLanguageData("I7")});

          if(c->health <= 0) {
            string dm = c->name + " " + c->deathText;
            combatUIManager->queuedStrings.push_back(make_pair(dm,c));
            g_deadCombatants.push_back(c);
            int index = 0;
            bool del = 0;
            for(index = 0; index < g_enemyCombatants.size(); index++) {
              if(g_enemyCombatants.at(index) == c) {
                curCombatantIndex--;
                curStatusIndex = 0;
                M("Decrementing curCombatantIndex because an enemy died from a status");
                D(curCombatantIndex);
                g_enemyCombatants.erase(g_enemyCombatants.begin() + index);
                del = 1;
                break;
              }
            }
            if(del == 0) E("Couldn't delete enemy from g_enemyCombatants");
            //!!!joseph try ending an encounter with a stickybomb
          }




          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          g_submode = submode::TEXT_STATUS_E;
        }

        e->turns--;
        break;
      }

    case status::IGNITED:
      {
        int damage = (frng(0.8,1.2) * e->magnitude) - c->curDefense;
        if(damage <0) {damage =0;}
        c->health -= damage;
        c->damageTakenThisTurn += damage;
  
        combatUIManager->finalText = stringMultiInject(getLanguageData("IgniteStatus"), {c->name, to_string(damage)});
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;
        g_submode = submode::TEXT_STATUS_E;

        e->turns--;
        break;
      }
    case status::BESTOWED:
      {
        if(e->turns <= 0) {
          //wore off

          combatUIManager->finalText = stringMultiInject(getLanguageData("StatusWornOff"), {spiritTable[12].name, c->name});
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          g_submode = submode::TEXT_STATUS_P;
          return 1;
        }
        c->curAttack += e->magnitude;
        e->turns--;
        break;
      }
    case status::PHYSICALBARRIER:
      {
        if(e->turns <= 0) {
          //wore off
          combatUIManager->finalText = stringMultiInject(getLanguageData("PhysicalBarrierWornOff"), {c->name});
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          if(g_submode == submode::STATUS_P) {
            g_submode = submode::TEXT_STATUS_P;
          } else {
            g_submode = submode::TEXT_STATUS_E;
          }
          return 1;
        }
        c->physicalDefense = e->magnitude;
        e->turns --;
        break;
      }
    case status::BLINDED:
      {
        if(e->turns <= 0) {
          //wore off
          combatUIManager->finalText = stringMultiInject(getLanguageData("StatusWornOff"), { getLanguageData("BlindedStatus"), c->name});
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          if(g_submode == submode::STATUS_P) {
            g_submode = submode::TEXT_STATUS_P;
          } else {
            g_submode = submode::TEXT_STATUS_E;
          }
          return 1;
        }
        e->turns --;
        break;
      }
    case status::DEFENSPILLED:
      {
        if(e->turns <= 0) {
          //wore off
          combatUIManager->finalText = stringMultiInject(getLanguageData("StatusWornOff"), { getLanguageData("DefenspillStatus"), c->name});
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          if(g_submode == submode::STATUS_P) {
            g_submode = submode::TEXT_STATUS_P;
          } else {
            g_submode = submode::TEXT_STATUS_E;
          }
          return 1;
        }
        c->curDefense += e->magnitude;
        e->turns --;
        break;
      }



  }
  curStatusIndex++;
  return 0;
}

void combatUI::calculateXP() {
  xpToGrant = 0;
  //leveling up is boring. the player should progress with
  //items, which can be different every time
  //and vary in rewardfullness
  /*
  for(auto x : g_deadCombatants) {
    int dmg = x->baseAttack;
    int def = x->baseDefense;
    int het = x->baseStrength;
    int sum = dmg + def + het;
    xpToGrant += sum;
  }
  */
}

combatUI::combatUI(SDL_Renderer* renderer) {
  initTables();

  directionalPreposition = getLanguageData("DirectionalPreposition");

  options[0] = getLanguageData("CombatOption1");
  options[1] = getLanguageData("CombatOption2");
  options[2] = getLanguageData("CombatOption3");
  options[3] = getLanguageData("CombatOption4");
  options[4] = getLanguageData("CombatOption5");
  options[5] = getLanguageData("CombatOption6");

  for(int i = 0; i < 4; i++) {
    ui* u = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0, 0.65, 1, 0.35, 0);
    u->patchwidth = 213;
    u->patchscale = 0.4;
    u->is9patch = true;
    u->persistent = true;
    u->show = 0;
    u->layer0 = 1;
    partyHealthBoxes.push_back(u);
  }

  for(int i = 0; i < 4; i++) {
    textbox* t = new textbox(renderer, "", 1, 0, 0, 0.9);
    t->boxWidth = 0;
    t->width = 0.95;
    t->boxHeight = 0;
    t->boxX = -1;
    t->boxY = -1;
    t->align = 2;
    t->dropshadow = 1;
    t->show = 1;
    t->layer0 = 1;
    partyNameTextboxes.push_back(t);
  }


//  for(int i = 0; i < 4; i++) {
//    textbox* t = new textbox(renderer, getLanguageData("PartyHP").c_str(), 0, 0, 0, 0.9);
//    t->boxWidth = 0;
//    t->width = 0.95;
//    t->boxHeight = 0;
//    t->boxX = -1;
//    t->boxY = -1;
//    t->align = 0;
//    t->dropshadow = 1;
//    t->show = 1;
//    t->layer0 = 1;
//    partyHealthDescripterTextboxes.push_back(t);
//  }

//  for(int i = 0; i < 4; i++) {
//    textbox* t = new textbox(renderer, "", 1, 0, 0, 0.9);
//    t->boxWidth = 0;
//    t->width = 0.95;
//    t->boxHeight = 0;
//    t->boxX = -1;
//    t->boxY = -1;
//    t->align = 1;
//    t->dropshadow = 1;
//    t->show = 1;
//    t->layer0 = 1;
//    partyHealthTextboxes.push_back(t);
//  }


//  for(int i = 0; i < 4; i++) {
//    textbox* t = new textbox(renderer, getLanguageData("PartySP").c_str(), 0, 0, 0, 0.9);
//    t->boxWidth = 0;
//    t->width = 0.95;
//    t->boxHeight = 0;
//    t->boxX = -1;
//    t->boxY = -1;
//    t->align = 0;
//    t->dropshadow = 1;
//    t->show = 1;
//    t->layer0 = 1;
//    partyManaDescripterTextboxes.push_back(t);
//  }

//  for(int i = 0; i < 4; i++) {
//    textbox* t = new textbox(renderer, "", 1, 0, 0, 0.9);
//    t->boxWidth = 0;
//    t->width = 0.95;
//    t->boxHeight = 0;
//    t->boxX = -1;
//    t->boxY = -1;
//    t->align = 1;
//    t->dropshadow = 1;
//    t->show = 1;
//    t->layer0 = 1;
//    partyManaTextboxes.push_back(t);
//  }

//  partyMiniText = new textbox(renderer, "", 0, 0, 0, 0.9);
//  partyMiniText->boxWidth = 0;
//  partyMiniText->width = 0.95;
//  partyMiniText->boxHeight = 0;
//  partyMiniText->boxX = 0.2;
//  partyMiniText->boxY = 1-0.1;
//  partyMiniText->align = 1;
//  partyMiniText->dropshadow = 1;
//  partyMiniText->show = 1;

  mainPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0, 0.65, 1, 0.35, 0);
  mainPanel->patchwidth = 213;
  mainPanel->patchscale = 0.4;
  mainPanel->is9patch = true;
  mainPanel->persistent = true;
  mainPanel->y = 0;


  dialogProceedIndicator = new ui(renderer, "resources/engine/dialog_proceed.qoi", 0.92, 0.88, 0.05, 1, 0);
  dialogProceedIndicator->heightFromWidthFactor = 1;
  dialogProceedIndicator->persistent = true;
  dialogProceedIndicator->priority = 8;
  dialogProceedIndicator->dropshadow = 1;
  dialogProceedIndicator->y =  0.25;

  mainText = new textbox(renderer, "", 2, 0, 0, 0.9);
  mainText->boxWidth = 0.9;
  mainText->width = 0.9;
  mainText->boxHeight = 0.25;
  mainText->boxX = 0.05;
  mainText->boxY = 0.05;
  mainText->dropshadow = 1;

  optionsPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.08, 0.05, 0.48, 0.225, 0);
  optionsPanel->patchwidth = 213;
  optionsPanel->patchscale = 0.4;
  optionsPanel->is9patch = true;
  optionsPanel->persistent = true;

  optionsText = new textbox(renderer, "", 1, 0, 0, 0.9);
  optionsText->boxWidth = 0.9;
  optionsText->width = 0.9;
  optionsText->boxHeight = 0.25;
  optionsText->boxX = 0.1;
  optionsText->boxY = 0.05;
  optionsText->dropshadow = 1;

  optionsMiniText = new textbox(renderer, "", 0, 0, 0, 0.9);
  optionsMiniText->boxWidth = 0;
  optionsMiniText->width = 0.95;
  optionsMiniText->boxHeight = 0;
  optionsMiniText->boxX = 0.127;
  optionsMiniText->boxY = 0.07;
  optionsMiniText->dropshadow = 1;
  optionsMiniText->show = 1;

  menuPicker = new ui(renderer, "resources/static/ui/menu_picker.qoi", 0.92, 0.88, 0.03, 1, 0);
  menuPicker->heightFromWidthFactor = 1;
  menuPicker->persistent = true;
  menuPicker->priority = 8;
  menuPicker->dropshadow = 1;
  menuPicker->y =  0.25;

  targetPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.4, 0.18, 0.3, 0.14, 0);
  targetPanel->patchwidth = 213;
  targetPanel->patchscale = 0.4;
  targetPanel->is9patch = true;
  targetPanel->persistent = true;

  targetText = new textbox(renderer, "", 1, 0, 0, 0.9);
  targetText->boxWidth = 0.3;
  targetText->boxHeight = 0.12;
  targetText->boxX = 0.55;
  targetText->boxY = 0.22;
  targetText->align = 2;
  targetText->dropshadow = 1;

  inventoryPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.4, 0.05, 0.46, 0.58, 0);
  inventoryPanel->patchwidth = 213;
  inventoryPanel->patchscale = 0.4;
  inventoryPanel->is9patch = true;
  inventoryPanel->persistent = true;

  inventoryText = new textbox(renderer, "", 1, 0, 0, 0.9);
  inventoryText->boxWidth = 0.3;
  inventoryText->boxHeight = 0.12;
  inventoryText->boxX = 0.45;
  inventoryText->boxY = 0.22;
  inventoryText->dropshadow = 1;

  spiritPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.41, 0.05, 0.34, 0.38, 0);
  spiritPanel->patchwidth = 213;
  spiritPanel->patchscale = 0.4;
  spiritPanel->is9patch = true;
  spiritPanel->persistent = true;

  spiritText = new textbox(renderer, "", 1, 0, 0, 0.9);
  spiritText->boxWidth = 0.3;
  spiritText->boxHeight = 0.12;
  spiritText->boxX = 0.45;
  spiritText->boxY = 0.22;
  spiritText->dropshadow = 1;

  forgetPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.2, 0.25, 1- 0.35*2, 0.42, 0);
  forgetPanel->patchwidth = 213;
  forgetPanel->patchscale = 0.4;
  forgetPanel->is9patch = true;
  forgetPanel->persistent = true;

  forgetText = new textbox(renderer, "", 1, 0, 0, 0.9);
  forgetText->boxWidth = 0.3;
  forgetText->boxHeight = 0.12;
  forgetText->boxX = 0.45;
  forgetText->boxY = 0.22;
  forgetText->dropshadow = 1;

  forgetPicker = new ui(renderer, "resources/static/ui/menu_picker.qoi", 0.65, 0.88, 0.032, 1, 0);
  forgetPicker->heightFromWidthFactor = 1;
  forgetPicker->persistent = true;
  forgetPicker->priority = 8;
  forgetPicker->dropshadow = 1;
  forgetPicker->y =  0.25;

  forgetInfoPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.5, 0.25, 0.5, 0.42, 0);
  forgetInfoPanel->patchwidth = 213;
  forgetInfoPanel->patchscale = 0.4;
  forgetInfoPanel->is9patch = true;
  forgetInfoPanel->persistent = true;

  forgetInfoText = new textbox(renderer, "", 0, 0, 0, 0.2);
  forgetInfoText->boxWidth = 0.1;
  forgetInfoText->boxHeight = 0.6;
  forgetInfoText->boxX = 0.53;
  forgetInfoText->boxY = 0.28;
  forgetInfoText->dropshadow = 1;


  //these are like forgetInfo, but provide info about the spiritmove
  //when the player is in the overworld looking at their moves
  spiritInfoPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.5, 0.25, 0.5, 0.42, 0);
  spiritInfoPanel->patchwidth = 213;
  spiritInfoPanel->patchscale = 0.4;
  spiritInfoPanel->is9patch = true;
  spiritInfoPanel->persistent = true;

  spiritInfoText = new textbox(renderer, "", 0, 0, 0, 0.2);
  spiritInfoText->boxWidth = 0.1;
  spiritInfoText->boxHeight = 0.6;
  spiritInfoText->boxX = 0.53;
  spiritInfoText->boxY = 0.28;
  spiritInfoText->dropshadow = 1;

  yes = new textbox(renderer, "Yes", 2, 0, 0, 0.9);
  yes->boxX = 0.50 - 0.07;
  yes->boxY = 0.2;
  yes->boxWidth = 0.01;
  yes->boxHeight = 0.35;
  yes->dropshadow = 1;
  yes->align = 2;

  no = new textbox(renderer, "No", 2, 0, 0, 0.9);
  no->boxX = 0.50 + 0.07;
  no->boxY = 0.2;
  no->boxWidth = 0;
  no->boxHeight = 0.35;
  no->dropshadow = 1;
  no->align = 2;

  confirmPicker = new ui(renderer, "resources/static/ui/menu_picker.qoi", 0.92, 0.88, 0.04, 1, 0);
  confirmPicker->heightFromWidthFactor = 1;
  confirmPicker->persistent = true;
  confirmPicker->priority = 8;
  confirmPicker->dropshadow = 1;
  confirmPicker->y =  0.25;

  aspect = 16.0f / 10.0f;
  dodgePanelFullWidth = 0.25;
  dodgePanelFullHeight = 0.25 * aspect;
  dodgePanelFullX = 0.5 - dodgePanelFullWidth/2;
  dodgePanelFullY = 0.5 - dodgePanelFullHeight/2;

  dodgePanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.5- dodgePanelFullX, dodgePanelFullY, dodgePanelFullWidth, dodgePanelFullHeight, 0);
  dodgePanel->patchwidth = 213;
  dodgePanel->patchscale = 0.4;
  dodgePanel->is9patch = true;
  dodgePanel->persistent = true;

  useOrDiscardPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.45, 0.2, 0.2, 0.273, 0);
  useOrDiscardPanel->patchwidth = 213;
  useOrDiscardPanel->patchscale = 0.4;
  useOrDiscardPanel->is9patch = true;
  useOrDiscardPanel->persistent = true;
  useOrDiscardPanel->renderOverText1 = 1;

  useOrDiscardUseText = new textbox(renderer, "", 1, 0, 0, 0.9);
  useOrDiscardUseText->boxWidth = 0.9;
  useOrDiscardUseText->width = 0.9;
  useOrDiscardUseText->boxHeight = 0.25;
  useOrDiscardUseText->boxX = 0.05;
  useOrDiscardUseText->boxY = 0.05;
  useOrDiscardUseText->dropshadow = 1;
  useOrDiscardUseText->updateText(getLanguageData("UDuseText"), -1, 15);
  useOrDiscardUseText->layer2 = 1;


  useOrDiscardDiscardText = new textbox(renderer, "", 1, 0, 0, 0.9);
  useOrDiscardDiscardText->boxWidth = 0.9;
  useOrDiscardDiscardText->width = 0.9;
  useOrDiscardDiscardText->boxHeight = 0.25;
  useOrDiscardDiscardText->boxX = 0.05;
  useOrDiscardDiscardText->boxY = 0.05;
  useOrDiscardDiscardText->dropshadow = 1;
  useOrDiscardDiscardText->updateText(getLanguageData("UDdiscardText"), -1, 15);
  useOrDiscardDiscardText->layer2 = 1;


  udInfoText = new textbox(renderer, "", 1, 0, 0, 0.9);
  udInfoText->boxWidth = 0.9;
  udInfoText->width = 0.9;
  udInfoText->boxHeight = 0.25;
  udInfoText->boxX = 0.05;
  udInfoText->boxY = 0.05;
  udInfoText->dropshadow = 1;
  udInfoText->updateText(getLanguageData("UDinfoText"), -1, 15);
  udInfoText->layer2 = 1;

  useOrDiscardMenuPicker = new ui(renderer, "resources/static/ui/menu_picker.qoi", 0.92, 0.88, 0.03, 1, 0);
  useOrDiscardMenuPicker->heightFromWidthFactor = 1;
  useOrDiscardMenuPicker->persistent = true;
  useOrDiscardMenuPicker->priority = 8;
  useOrDiscardMenuPicker->dropshadow = 1;
  useOrDiscardMenuPicker->y =  0.25;
  useOrDiscardMenuPicker->renderOverText1 = 1;
  
  const char* file = "resources/static/sprites/minigame/fomm-heart.qoi";

  fommDodgerTex = loadTexture(renderer, file);
  
  const char* filen = "resources/static/sprites/minigame/neheten-heart.qoi";
  nehetenDodgerTex = loadTexture(renderer, filen);

  const char* fileb = "resources/static/sprites/minigame/blish-heart.qoi";
  blishDodgerTex = loadTexture(renderer, fileb);


  const char* filed = "resources/static/sprites/minigame/dafua-heart.qoi";
  dafuaDodgerTex = loadTexture(renderer, filed);
  dodgerTexture = fommDodgerTex;

  file = "resources/static/sprites/minigame/bullet.qoi";
  bulletTexture = loadTexture(renderer, file);

  rendertarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 1024, 1024);

  db1 = SDL_CreateRGBSurface(0, 512, 512, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

  file = "resources/static/sprites/minigame/heart.qoi";
  heartTexture = loadTexture(renderer, file);

  file = "resources/static/sprites/minigame/heart-empty.qoi";
  heartEmptyTexture = loadTexture(renderer, file);

  file = "resources/static/sprites/minigame/star.qoi";
  starTexture = loadTexture(renderer, file);

  file = "resources/static/sprites/minigame/star-empty.qoi";
  starEmptyTexture = loadTexture(renderer, file);
}

combatUI::~combatUI() {
  for(int i = 0; i < 4; i++) {
    delete partyHealthBoxes[i];
    delete partyNameTextboxes[i];
//    delete partyHealthDescripterTextboxes[i];
//    delete partyHealthTextboxes[i];
//    delete partyManaDescripterTextboxes[i];
//    delete partyManaTextboxes[i];
  }

  delete mainPanel;
  delete dialogProceedIndicator;
  delete mainText;
  delete optionsPanel;
  delete optionsText;
  delete optionsMiniText;
  delete menuPicker;
  delete targetPanel;
  delete targetText;
  delete inventoryPanel;
  delete inventoryText;
  delete spiritPanel;
  delete spiritText;
  delete forgetPanel;
  delete forgetText;
  delete forgetPicker;
  delete forgetInfoPanel;
  delete forgetInfoText;
  delete spiritInfoPanel;
  delete spiritInfoText;
  delete yes;
  delete no;
  delete confirmPicker;
  delete dodgePanel;
  delete useOrDiscardPanel;
  delete useOrDiscardUseText;
  delete useOrDiscardDiscardText;
  delete udInfoText;
  delete useOrDiscardMenuPicker;
  SDL_DestroyTexture(fommDodgerTex);
  SDL_DestroyTexture(nehetenDodgerTex);
  SDL_DestroyTexture(blishDodgerTex);
  SDL_DestroyTexture(dafuaDodgerTex);
  SDL_DestroyTexture(bulletTexture);

 
  SDL_DestroyTexture(rendertarget);
  SDL_FreeSurface(db1);

}

void drawOptionsPanel() {
  combatUIManager->optionsPanel->render(renderer, g_camera, elapsed);

  combatUIManager->optionsMiniText->updateText(g_partyCombatants[curCombatantIndex]->name, -1, 0.85, g_textcolor, g_font);
  combatUIManager->optionsMiniText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
  const int rows = 2;
  const int columns = 3;
  const float width = 0.16;
  const float height = 0.07;
  const float initialX = 0.125;
  const float initialY = 0.1;
  int index = 0;
  for(int i = 0; i < columns; i++) {
    for(int j = 0; j < rows; j++) {
      combatUIManager->optionsText->boxX = initialX + (i * width);
      combatUIManager->optionsText->boxY = initialY + (j * height);
      combatUIManager->optionsText->updateText(combatUIManager->options[index], -1, 0.85, g_textcolor, g_font);
      if(index == combatUIManager->currentOption) {
        combatUIManager->menuPicker->x = initialX + (i * width) - 0.027;
        combatUIManager->menuPicker->y = initialY + (j * height) + 0.007;
      }

      combatUIManager->optionsText->render(renderer, WIN_WIDTH, WIN_HEIGHT);


      index++;
    }
  }

  combatUIManager->menuPicker->render(renderer, g_camera, elapsed);
}

void combatUI::hideAll() {
  for(int i = 0; i < 4; i++) {
    partyHealthBoxes[i]->show = 0;
//    partyHealthDescripterTextboxes[i]->show = 0;
//    partyHealthTextboxes[i]->show = 0;
//    partyManaDescripterTextboxes[i]->show = 0;
//    partyManaTextboxes[i]->show = 0;
    partyNameTextboxes[i]->show = 0;
  }
//  partyHealthBox->show = 0;
//  partyText->show = 0;
//  partyMiniText->show = 0;
  mainPanel->show = 0;
  dialogProceedIndicator->show = 0;
  mainText->show = 0;
  optionsPanel->show = 0;
  optionsText->show = 0;
  optionsMiniText->show = 0;
  menuPicker->show = 0;
  targetPanel->show = 0;
  targetText->show = 0;
  inventoryPanel->show = 0;
  inventoryText->show = 0;
  spiritPanel->show = 0;
  spiritText->show = 0;
  dodgePanel->show = 0;
  forgetPanel->show = 0;
  forgetText->show = 0;
  forgetInfoText->show = 0;
  forgetPicker->show = 0;
  forgetInfoPanel->show = 0;
  yes->show = 0;
  no->show = 0;
  confirmPicker->show = 0;
  spiritInfoPanel->show = 0;
  spiritInfoText->show = 0;
  useOrDiscardPanel->show = 0;
  useOrDiscardUseText->show = 0;
  useOrDiscardDiscardText->show = 0;
  useOrDiscardMenuPicker->show = 0;
  udInfoText->show = 0;
}

void getCombatInput() {
  for (int i = 0; i < 16; i++)
  {
    oldinput[i] = input[i];
  }

  SDL_PollEvent(&event);

  if (keystate[SDL_SCANCODE_F] && fullscreen_refresh)
  {
    toggleFullscreen();
  }

  if (keystate[SDL_SCANCODE_G] && !inputRefreshCanSwitchOffDevMode && canSwitchOffDevMode)
  {
    toggleDevmode();
  }

  //up
  if (keystate[bindings[0]])
  {
    input[0] = 1;
  }
  else
  {
    input[0] = 0;
  }

  //down
  if (keystate[bindings[1]])
  {
    input[1] = 1;
  }
  else
  {
    input[1] = 0;
  }

  //left
  if (keystate[bindings[2]])
  {
    input[2] = 1;
  }
  else
  {
    input[2] = 0;
  }

  //right
  if (keystate[bindings[3]])
  {
    input[3] = 1;
  }
  else
  {
    input[3] = 0;
  }

  //item button
  if (keystate[bindings[10]])
  {
    input[10] = 1;
  }
  else
  {
    input[10] = 0;
  }

  //interact button
  if (keystate[bindings[11]])
  {
    input[11] = 1;
  }
  else
  {
    input[11] = 0;
  }

  if (keystate[bindings[9]])
  {
    input[9] = 1;
  }
  else
  {
    input[9] = 0;
  }

  //jump button
  if (keystate[bindings[8]])
  {
    input[8] = 1;
  }
  else
  {
    input[8] = 0;
  }

}

void renderSpiritPanel()
{
  const int rows = 4;
  const int columns = 1;
  const float width = 0.16;
  const float height = 0.07;
  float initialX = 0.45;
  float initialY = 0.1;
  int index = 0;
  combatUIManager->spiritPanel->x = 0.41;
  combatUIManager->spiritPanel->y = 0.05;

  if(g_amState != amState::CLOSED) {
    //move spirit panel over because the user is in the overworld, and so they can see the descriptions
    initialX = 0.18;
    initialY = 0.3;
    combatUIManager->spiritPanel->x = 0.14;
    combatUIManager->spiritPanel->y = 0.25;
    combatUIManager->spiritPanel->show = 1;
    combatUIManager->spiritInfoText->show = 1;
    combatUIManager->spiritText->show = 1;


  }
  combatUIManager->spiritPanel->render(renderer, g_camera, elapsed);

  combatUIManager->menuPicker->x = 10;
  if(g_partyCombatants[curCombatantIndex]->spiritMoves.size() > 0) {
    for(int i = 0; i < columns; i++) {
      for(int j = 0; j < rows; j++) {
        combatUIManager->spiritText->boxX = initialX + (i * width);
        combatUIManager->spiritText->boxY = initialY + (j * height);
        string spiritName = "";
        spiritName = spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[index]].name;

        combatUIManager->spiritText->align = 0;
        combatUIManager->spiritText->updateText(spiritName, -1, 0.85, g_textcolor, g_font);
        if(index == combatUIManager->currentInventoryOption) {
          combatUIManager->menuPicker->x = initialX + (i * width) - 0.027;
          combatUIManager->menuPicker->y = initialY + (j * height) + 0.007;
        }
        combatUIManager->spiritText->render(renderer, WIN_WIDTH, WIN_HEIGHT);

        //render cost
        if(index < g_partyCombatants[curCombatantIndex]->spiritMoves.size() && spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[index]].cost > 0)
        {
          combatUIManager->spiritText->align = 1;
          combatUIManager->spiritText->boxX += 0.27;
          combatUIManager->spiritText->boxWidth = 0;
          combatUIManager->spiritText->updateText(to_string(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[index]].cost), -1, 0.85, g_textcolor, g_font);
          combatUIManager->spiritText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
        }



        index++;
      }
    }
    combatUIManager->menuPicker->render(renderer, g_camera, elapsed);
  }
}

void renderInventoryPanel() 
{
  const int rows = 7;
  const int columns = 2;
  const float width = 0.22;
  const float height = 0.07;
  const float initialX = 0.44;
  const float initialY = 0.1;
  int index = 0;
  if(g_items.size() > 0) {
    for(int i = 0; i < columns; i++) {
      for(int j = 0; j < rows; j++) {
        combatUIManager->inventoryText->boxX = initialX + (i * width);
        combatUIManager->inventoryText->boxY = initialY + (j * height);

        int itemType = 0;
        int itemIndex = 0;
        if(index < g_items.size() && index >= 0) {
          itemType = g_items[index].type;
          itemIndex = g_items[index].index;
        }

        string itemName = "";
        if(itemType >=0 && itemType <= 3 && itemIndex >= 0 && itemIndex < itemsTable[itemType].size()) {
          itemName = itemsTable.at(itemType).at(itemIndex).name;
        } else {
          E("Bad itemIndex " + itemIndex);
          abort();
        }
        combatUIManager->inventoryText->updateText(itemName, -1, 0.85, g_textcolor, g_font);
        if(combatUIManager->inventoryText->width > 0.185 * WIN_WIDTH) {
          combatUIManager->inventoryText->width = 0.185 * WIN_WIDTH;
        }
        if(index == combatUIManager->currentInventoryOption) {
          combatUIManager->menuPicker->x = initialX + (i * width) - 0.028;
          combatUIManager->menuPicker->y = initialY + (j * height) + 0.007;
        }
        if(index < (int)g_items.size()) {
          combatUIManager->inventoryText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
        }

        index++;
      }
    }
    combatUIManager->inventoryText->boxX = 10;
    combatUIManager->menuPicker->render(renderer, g_camera, elapsed);
  }
}


void drawCombatants() {
  int count = g_enemyCombatants.size();

  // Total padding on both sides
  int padding = 0.05 * WIN_WIDTH;

  // Calculate total available width for spacing
  int availableWidth = WIN_WIDTH - (2 * padding);

  // Calculate gap between combatants' centers
  int gap = availableWidth / (count + 1);

  // Starting x position for the first combatant's center
  int xCenter = padding + gap;

  for (int i = 0; i < count; ++i) {
    combatant* combatant = g_enemyCombatants[i];

    // Set renderQuad if it hasn't been initialized
    if (combatant->renderQuad.x == -1) {

      // Calculate actual width and height
      int actual_width = static_cast<int>(WIN_WIDTH * combatant->width);
      int actual_height = static_cast<int>(WIN_WIDTH * combatant->height);

      // Calculate x and y position for this combatant
      int x = xCenter - (actual_width / 2);
      int y = WIN_HEIGHT *(0.43 + combatant->offset); // Center vertically

      //to make them seem equidistant from the camera, move ones closer to the
      //edges of the screen down a bit (to go on the guidecircle)
      int distFromCenter = 0.00000003* pow(abs(xCenter - WIN_WIDTH/2), 3);
      y += distFromCenter;
      actual_width += distFromCenter *2;
      actual_height += distFromCenter *2;


      SDL_Rect renderQuad = { x, y, actual_width, actual_height };
      combatant->renderQuad = renderQuad;
    }

    // Apply targeting color mod if needed
    if (g_submode == submode::TARGETING && i == combatUIManager->currentTarget) {
      SDL_SetTextureColorMod(combatant->texture, combatUIManager->targetingColorMod, combatUIManager->targetingColorMod, combatUIManager->targetingColorMod);
    } else {
      SDL_SetTextureColorMod(combatant->texture, 255, 255, 255);
    }

    // Render the combatant
    SDL_RenderCopy(renderer, combatant->texture, nullptr, &combatant->renderQuad);

    // Update xCenter for the next combatant
    xCenter += gap;
  }

  for(auto x : g_deadCombatants) {
    SDL_SetTextureAlphaMod(x->texture, x->opacity);
    if(x->disappearing && x->opacity-1 > 0) {
      x->opacity-=5;
    }
    SDL_RenderCopy(renderer, x->texture, nullptr, &x->renderQuad);
  }

  SDL_SetTextureAlphaMod(g_shade, g_dungeonDarkEffect);

  if(g_gamemode != gamemode::EXPLORATION) {
    SDL_RenderCopy(renderer, g_shade, NULL, NULL);
  }

  //count = g_partyCombatants.size();
  count = 4; //now I just want to use fomm's health
  gap = 0.1;

  for (int i = 0; i < count; i++) {
    SDL_Color textcolor {155, 115, 115};
    combatant* combatant = g_partyCombatants[i];
    float bonusY = 0;
    if(g_submode == submode::MAIN ||
        g_submode == submode::TARGETING ||
        g_submode == submode::ITEMCHOOSE ||
        g_submode == submode::ALLYTARGETING ||
        g_submode == submode::SPIRITCHOOSE ||
        g_submode == submode::SPWARNING ||
        g_submode == submode::RUNWARNING ||
        g_amState == amState::SPIRIT ||
        g_amState == amState::STARGETING ||
        g_amState == amState::SPIRITSELECT
        ) {
      if(i == curCombatantIndex){
        bonusY = -0.05;
      }
    }

    combatUIManager->partyHealthBoxes[i]->bonusY = bonusY;
    combatUIManager->partyNameTextboxes[i]->bonusY = bonusY;
//    combatUIManager->partyHealthDescripterTextboxes[i]->bonusY = bonusY;
//    combatUIManager->partyManaDescripterTextboxes[i]->bonusY = bonusY;
//    combatUIManager->partyHealthTextboxes[i]->bonusY = bonusY;
//    combatUIManager->partyManaTextboxes[i]->bonusY = bonusY;

    //combatUIManager->partyText->textcolor = { 155, 115, 115};
    if(combatant->health <= 0) {
      textcolor = g_healthtextlowcolor;
    }

    if(g_submode == submode::ALLYTARGETING
        || g_amState == amState::STARGETING
        || g_amState == amState::ITARGETING) {
      if(i == combatUIManager->currentTarget) {
        if(combatant->health <= 0) {
          textcolor = g_healthtextlowcolor;
          textcolor.r -= 45;
          textcolor.g -= 45;
          textcolor.b -= 45;
        } else {
          textcolor = { 108, 80, 80};
        }
      } else {
        if(combatant->health <= 0) {
          textcolor = g_healthtextlowcolor;
        } else {
          textcolor = { 155, 115, 115};
        }
      }
    }



    combatUIManager->partyNameTextboxes[i]->textcolormod = textcolor;
//    combatUIManager->partyHealthDescripterTextboxes[i]->textcolormod = textcolor;
//    combatUIManager->partyHealthTextboxes[i]->textcolormod = textcolor;
//    combatUIManager->partyManaDescripterTextboxes[i]->textcolormod = textcolor;
//    combatUIManager->partyManaTextboxes[i]->textcolormod = textcolor;


    if(combatUIManager->partyNameTextboxes[0]->boxX == -1) {
      for(int i = 0; i < g_partyCombatants.size(); i++) {

        //update these values so we know to not re-render the text textures
        combatUIManager->pHpRValues[i] = g_partyCombatants[i]->health;
        combatUIManager->pMhpRValues[i] = g_partyCombatants[i]->curStrength;
        combatUIManager->pSpRValues[i] = g_partyCombatants[i]->sp;
        combatUIManager->pMspRValues[i] = g_partyCombatants[i]->curMind;
        
        // Convert percentage-based width and height to actual pixel values
        float actual_width = 0.3 *  WIN_HEIGHT / WIN_WIDTH;
        float actual_height = 0.3;
    
        // Calculate X position to arrange horizontally
        float total_width = (count * actual_width) + ((count - 1) * gap);
        float x = (1 - total_width) / 2 + i * (actual_width + gap);
        float y = (1 - actual_height) / 2; // Centering vertically
    
        //combatUIManager->partyHealthBox->x = x;
        combatUIManager->partyHealthBoxes[i]->x = x;
        //combatUIManager->partyHealthBox->y = 0.7 + bonusY;
        combatUIManager->partyHealthBoxes[i]->y = 0.7;
    //    combatUIManager->partyHealthBox->width = actual_width;
        combatUIManager->partyHealthBoxes[i]->width = actual_width;
    //    combatUIManager->partyHealthBox->height = actual_height;
        combatUIManager->partyHealthBoxes[i]->height = actual_height;
    
        //combatUIManager->partyHealthBox->render(renderer, g_camera, elapsed);
    
    
    
        //combatUIManager->partyText->boxX = x + 0.02;
        combatUIManager->partyNameTextboxes[i]->boxX = x + 0.02 + 0.075;
    //    combatUIManager->partyText->boxY = 0.7 + 0.02 + bonusY;
        combatUIManager->partyNameTextboxes[i]->boxY = 0.7 + 0.02;
    //    combatUIManager->partyText->boxWidth = actual_width;
        combatUIManager->partyNameTextboxes[i]->boxWidth = actual_width;
    //    combatUIManager->partyText->boxHeight = actual_height;
        combatUIManager->partyNameTextboxes[i]->boxHeight = actual_height;
    //    combatUIManager->partyText->updateText(combatant->name, -1, 34, combatUIManager->partyText->textcolor);
        combatUIManager->partyNameTextboxes[i]->updateText(g_partyCombatants[i]->name, -1, 34,  g_whitetextcolor);
    //    combatUIManager->partyText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
//        combatUIManager->partyHealthDescripterTextboxes[i]->boxX = combatUIManager->partyNameTextboxes[i]->boxX - 0.075;
//        combatUIManager->partyHealthDescripterTextboxes[i]->boxY = combatUIManager->partyNameTextboxes[i]->boxY + 0.07 + 0.01;
//        combatUIManager->partyHealthDescripterTextboxes[i]->boxHeight = actual_height;
//        combatUIManager->partyHealthDescripterTextboxes[i]->boxWidth = actual_width;
//        combatUIManager->partyHealthDescripterTextboxes[i]->updateText(getLanguageData("PartyHP"), -1, 34, g_whitetextcolor);

//        combatUIManager->partyManaDescripterTextboxes[i]->boxX = combatUIManager->partyNameTextboxes[i]->boxX - 0.075;
//        combatUIManager->partyManaDescripterTextboxes[i]->boxY = combatUIManager->partyNameTextboxes[i]->boxY + 0.14 + 0.01;
//        combatUIManager->partyManaDescripterTextboxes[i]->boxHeight = actual_height;
//        combatUIManager->partyManaDescripterTextboxes[i]->boxWidth = actual_width;
//        combatUIManager->partyManaDescripterTextboxes[i]->updateText(getLanguageData("PartySP"), -1, 34,  g_whitetextcolor);

//        combatUIManager->partyHealthTextboxes[i]->boxX = combatUIManager->partyNameTextboxes[i]->boxX + 0.15 - 0.075;
//        combatUIManager->partyHealthTextboxes[i]->boxY = combatUIManager->partyNameTextboxes[i]->boxY + 0.07;
//        combatUIManager->partyHealthTextboxes[i]->boxHeight = actual_height;
//        combatUIManager->partyHealthTextboxes[i]->boxWidth = actual_width;
//        combatUIManager->partyHealthTextboxes[i]->updateText(to_string(g_partyCombatants[i]->health) + '/' + to_stringF(floor(g_partyCombatants[i]->curStrength)), -1, 34,  g_whitetextcolor);

//        combatUIManager->partyManaTextboxes[i]->boxX = combatUIManager->partyNameTextboxes[i]->boxX + 0.15 - 0.075;
//        combatUIManager->partyManaTextboxes[i]->boxY = combatUIManager->partyNameTextboxes[i]->boxY + 0.14;
//        combatUIManager->partyManaTextboxes[i]->boxHeight = actual_height;
//        combatUIManager->partyManaTextboxes[i]->boxWidth = actual_width;
//        combatUIManager->partyManaTextboxes[i]->updateText(to_string(g_partyCombatants[i]->sp) + '/' + to_stringF(floor(g_partyCombatants[i]->curMind)), -1, 34,  g_whitetextcolor);

      }
    
    //    combatUIManager->partyText->boxY += 0.07;
    //    combatUIManager->partyText->updateText(to_string(combatant->health), -1, 34, combatUIManager->partyText->textcolor);
    //    combatUIManager->partyText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    //    combatUIManager->partyText->boxY += 0.07;
    //    combatUIManager->partyText->updateText(to_string(combatant->sp), -1, 34, combatUIManager->partyText->textcolor);
    //    combatUIManager->partyText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    //
    //    combatUIManager->partyMiniText->show = 1;
    //    combatUIManager->partyMiniText->boxX = x + 0.15 + 0.02;
    //    combatUIManager->partyMiniText->boxY = 0.7 + 0.02 + bonusY + 0.073;
    //    combatUIManager->partyMiniText->boxWidth = actual_width;
    //    combatUIManager->partyMiniText->boxHeight = actual_height;
    //
    //    combatUIManager->partyMiniText->updateText(getLanguageData("PartyHP"), -1, 1, combatUIManager->partyText->textcolor);
    //    combatUIManager->partyMiniText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    //    combatUIManager->partyMiniText->boxY += 0.025;
    //    combatUIManager->partyMiniText->updateText('/' + to_stringF(floor(combatant->curStrength)), -1, 1, combatUIManager->partyText->textcolor);
    //    combatUIManager->partyMiniText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    //    combatUIManager->partyMiniText->boxY += 0.045;
    //    combatUIManager->partyMiniText->updateText(getLanguageData("PartySP"), -1, 1, combatUIManager->partyText->textcolor);
    //    combatUIManager->partyMiniText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    //    combatUIManager->partyMiniText->boxY += 0.025;
    //    combatUIManager->partyMiniText->updateText('/' + to_stringF(floor(combatant->curMind)), -1, 1, combatUIManager->partyText->textcolor);
    //    combatUIManager->partyMiniText->render(renderer, WIN_WIDTH, WIN_HEIGHT);


    }

  }

  for(int i = 0; i < g_partyCombatants.size(); i++) {

    if(combatUIManager->pHpRValues[i] != g_partyCombatants[i]->health
        || combatUIManager->pMhpRValues[i] != g_partyCombatants[i]->curStrength) {
        //combatUIManager->partyHealthTextboxes[i]->updateText(to_string(g_partyCombatants[i]->health) + '/' + to_stringF(floor(g_partyCombatants[i]->curStrength)), -1, 34,  g_whitetextcolor);
           
    }

    if(combatUIManager->pSpRValues[i] != g_partyCombatants[i]->sp  
        || combatUIManager->pMspRValues[i] != g_partyCombatants[i]->curMind) {
//        combatUIManager->partyManaTextboxes[i]->updateText(to_string(g_partyCombatants[i]->sp) + '/' + to_stringF(floor(g_partyCombatants[i]->curMind)), -1, 34,  g_whitetextcolor);
    }

   



    combatUIManager->partyNameTextboxes[i]->show = 1;
    combatUIManager->partyHealthBoxes[i]->show = 1;
    combatUIManager->partyHealthBoxes[i]->render(renderer, g_camera, elapsed);
    combatUIManager->partyNameTextboxes[i]->render(renderer, WIN_WIDTH, WIN_HEIGHT);

    float originalX = combatUIManager->partyHealthBoxes[i]->x * WIN_WIDTH + 31.61;
    float x = originalX;

    float originalY = (combatUIManager->partyHealthBoxes[i]->y + combatUIManager->partyHealthBoxes[i]->bonusY) * WIN_HEIGHT + 80;
    float y = originalY;

    //render the character's health with empty and full heart containers
    for(int j = 0; j < g_partyCombatants[i]->curStrength; j+=2) {
      SDL_Rect dst = {x, y, 32, 32};
      SDL_Rect shdst = {x + 2, y + 2, 32, 32};

      SDL_SetTextureColorMod(combatUIManager->heartEmptyTexture, 128, 128, 128);
      SDL_RenderCopy(renderer, combatUIManager->heartEmptyTexture, NULL, &shdst);
      SDL_SetTextureColorMod(combatUIManager->heartEmptyTexture, 255, 255, 255);
      SDL_RenderCopy(renderer, combatUIManager->heartEmptyTexture, NULL, &dst);

      if(g_partyCombatants[i]->health >= j + 2) {
        SDL_SetTextureColorMod(combatUIManager->heartTexture, 128, 128, 128);
        SDL_RenderCopy(renderer, combatUIManager->heartTexture, NULL, &shdst);
        SDL_SetTextureColorMod(combatUIManager->heartTexture, 255, 255, 255);
        SDL_RenderCopy(renderer, combatUIManager->heartTexture, NULL, &dst);
      } else if(g_partyCombatants[i]->health >= j + 1){
        //draw a half-heart
        SDL_Rect src = {0,0,32,64};
        SDL_Rect hdst = {x, y, 16, 32};
        SDL_RenderCopy(renderer, combatUIManager->heartTexture, &src, &hdst);
      }



      x += 32;
      float limit =(combatUIManager->partyHealthBoxes[i]->x + combatUIManager->partyHealthBoxes[i]->width)* WIN_WIDTH - 31.61;

      if(x > limit) {
        x = originalX;
        y += 32;
      }
    }


    y = originalY + 64 + 16;
    x = originalX;

    //render the character's mana
    for(int j = 0; j < g_partyCombatants[i]->curMind; j+=2) {
      SDL_Rect dst = {x, y, 32, 32};
      SDL_Rect shdst = {x + 2, y + 2, 32, 32};

      SDL_SetTextureColorMod(combatUIManager->starEmptyTexture, 128, 128, 128);
      SDL_RenderCopy(renderer, combatUIManager->starEmptyTexture, NULL, &shdst);
      SDL_SetTextureColorMod(combatUIManager->starEmptyTexture, 255, 255, 255);
      SDL_RenderCopy(renderer, combatUIManager->starEmptyTexture, NULL, &dst);

      if(g_partyCombatants[i]->sp >= j + 2) {
        SDL_SetTextureColorMod(combatUIManager->starTexture, 128, 128, 128);
        SDL_RenderCopy(renderer, combatUIManager->starTexture, NULL, &shdst);
        SDL_SetTextureColorMod(combatUIManager->starTexture, 255, 255, 255);
        SDL_RenderCopy(renderer, combatUIManager->starTexture, NULL, &dst);
      } else if(g_partyCombatants[i]->sp >= j + 1){
        //draw a half-star
        SDL_Rect src = {0,0,32,64};
        SDL_Rect hdst = {x, y, 16, 32};
        SDL_RenderCopy(renderer, combatUIManager->starTexture, &src, &hdst);
      }


      x += 32;
      float limit =(combatUIManager->partyHealthBoxes[i]->x + combatUIManager->partyHealthBoxes[i]->width)* WIN_WIDTH - 31.61;

      if(x > limit) {
        x = originalX;
        y += 32;
      }

    }



//    D(combatUIManager->partyHealthBoxes[i]->width/7 * WIN_WIDTH);
//    D(combatUIManager->partyHealthBoxes[i]->width/8 * WIN_WIDTH);


    combatUIManager->partyNameTextboxes[i]->show = 0;
    combatUIManager->partyHealthBoxes[i]->show = 0;


    //combatUIManager->partyHealthDescripterTextboxes[i]->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    //combatUIManager->partyManaDescripterTextboxes[i]->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    //combatUIManager->partyHealthTextboxes[i]->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    //combatUIManager->partyManaTextboxes[i]->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    
  }


}

void CombatLoop() {
  getCombatInput();

  SDL_RenderClear(renderer);

  updateWindowResolution();

  if(combatUIManager->loadedBackground.scene[0] == '>') {
    drawBackground();
  } else {
    drawSimpleBackground();
  }

  BM("Before drawCombatants()");
  drawCombatants();
  BM("drawCombatants()");

  SDL_RenderCopy(renderer, g_shade, NULL, NULL);


  switch (g_submode) {
    case submode::BEFORE:
      {
        for(int i = 0; i < 4; i ++) {
          combatUIManager->dodgingThisTurn[i] = 0;
        }

        for(auto &x : g_partyCombatants) {
          x->dmgDealtOverFight = 0;
          x->dmgTakenOverFight = 0;
        }

        g_autoFight = 0;

        //clear statuses
        for(auto x : g_partyCombatants) {
          x->statuses.clear();
          x->curStrength = x->baseStrength;
          x->curMind = x->baseMind;
          x->curAttack = x->baseAttack;
          x->curDefense = x->baseDefense;
          x->curSoul = x->baseSoul;
          x->curSkill = x->baseSkill;
          x->curCritical = x->baseCritical;
          x->curRecovery = x->baseSoul;
          x->physicalDefense = 0;
          x->spiritDefense = 0;
        }

        //copy g_enemyCombatants to g_enemyCombatantsForDropping
        //so items can be dropped based on who was fought
        g_dropInfos.clear();
        for(auto x : g_enemyCombatants) {

          dropInfo d;
          d.dropPercent = x->droppedItemPercent;
          d.dropIndex = x->droppedItemIndex;
          d.dropType = x->droppedItemType;
          d.name = x->name;
          d.eDropPercent = x->droppedEquipablePercent;
          d.eDropIndex = x->droppedEquipableIndex;
          g_dropInfos.push_back(d);
        }


        g_forceEndDialogue = 0;
        g_submode = submode::INWIPE;
      }
    case submode::INWIPE:
      {

       for(int i = 0; i < g_partyCombatants.size(); i++) {
//         combatUIManager->pHpRValues[i] = -1;
//         combatUIManager->pMhpRValues[i] = -1;
//         combatUIManager->pSpRValues[i] = -1;
//         combatUIManager->pMspRValues[i] = -1;

         combatUIManager->partyNameTextboxes[i]->show = 1;
         combatUIManager->partyHealthBoxes[i]->show = 1;
         //combatUIManager->partyHealthDescripterTextboxes[i]->show = 1;
//         combatUIManager->partyManaDescripterTextboxes[i]->show = 1;
//         combatUIManager->partyHealthTextboxes[i]->show = 1;
//         combatUIManager->partyManaTextboxes[i]->show = 1;
       }


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

//        ticks = SDL_GetTicks();
//        elapsed = ticks - lastticks;

        SDL_UnlockTexture(transitionTexture);
        SDL_RenderCopy(renderer, transitionTexture, NULL, NULL);

        if (transitionDelta > transitionImageHeight + pow(pow(transitionImageWidth / 2, 2) + pow(transitionImageHeight, 2), 0.5))
        {
          g_submode = submode::TEXT;
        }
        break;
      }
    case submode::OUTWIPE:
      {
        resetTrivialData();
        //M("OUTWIPE");

        for(auto x : g_partyCombatants) {
          writeSaveField(x->filename + "-dealt", x->dmgDealtOverFight);
          writeSaveField(x->filename + "-taken", x->dmgTakenOverFight);
        }

        {
          //SDL_GL_SetSwapInterval(0);
          bool cont = false;
          float ticks = 0;
          float lastticks = 0;
          float transitionElapsed = 5;
          float mframes = 60;
          float transitionMinFrametime = 5;
          transitionMinFrametime = 1/mframes * 1000;


          SDL_Surface* transitionSurface = loadSurface("resources/engine/transition.qoi");

          int imageWidth = transitionSurface->w;
          int imageHeight = transitionSurface->h;

          SDL_Texture* transitionTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, transitionSurface->w, transitionSurface->h );
          SDL_SetTextureBlendMode(transitionTexture, SDL_BLENDMODE_BLEND);


          void* pixelReference;
          int pitch;

          float offset = imageHeight;

          SDL_Texture* frame = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, WIN_WIDTH, WIN_HEIGHT);
          SDL_SetRenderTarget(renderer, frame);

          if(combatUIManager->loadedBackground.scene[0] == '>') {
            drawBackground();
          } else {
            drawSimpleBackground();
          }
          drawCombatants();

          SDL_RenderCopy(renderer, g_shade, NULL, NULL);

          SDL_SetRenderTarget(renderer, NULL);
          SDL_RenderClear(renderer);

          while (!cont) {

            //onframe things
            SDL_LockTexture(transitionTexture, NULL, &pixelReference, &pitch);

            memcpy( pixelReference, transitionSurface->pixels, transitionSurface->pitch * transitionSurface->h);
            Uint32 format = SDL_PIXELFORMAT_ARGB8888;
            SDL_PixelFormat* mappingFormat = SDL_AllocFormat( format );
            Uint32* pixels = (Uint32*)pixelReference;
            Uint32 transparent = SDL_MapRGBA( mappingFormat, 0, 0, 0, 255);

            offset += g_transitionSpeed + 0.02 * offset;

            for(int x = 0;  x < imageWidth; x++) {
              for(int y = 0; y < imageHeight; y++) {


                int dest = (y * imageWidth) + x;
                //int src =  (y * imageWidth) + x;

                if(pow(pow(imageWidth/2 - x,2) + pow(imageHeight + y,2),0.5) < offset) {
                  pixels[dest] = transparent;
                } else {
                  pixels[dest] = 0;
                }

              }
            }





            ticks = SDL_GetTicks();
            transitionElapsed = ticks - lastticks;
            //lock framerate
            if(transitionElapsed < transitionMinFrametime) {
              SDL_Delay(transitionMinFrametime - transitionElapsed);
              ticks = SDL_GetTicks();
              transitionElapsed = ticks - lastticks;
            }
            lastticks = ticks;

            SDL_RenderClear(renderer);
            //render last frame
            //SDL_RenderCopy(renderer, frame, NULL, NULL);
            if(combatUIManager->loadedBackground.scene[0] == '>') {
              drawBackground();
            } else {
              drawSimpleBackground();
            }

            drawCombatants();

            SDL_RenderCopy(renderer, g_shade, NULL, NULL);

            SDL_UnlockTexture(transitionTexture);
            SDL_RenderCopy(renderer, transitionTexture, NULL, NULL);
            SDL_RenderPresent(renderer);

            if(offset > imageHeight + pow(pow(imageWidth/2,2) + pow(imageHeight,2),0.5)) {
              cont = 1;
            }
          }
          SDL_FreeSurface(transitionSurface);
          SDL_DestroyTexture(transitionTexture);
          SDL_DestroyTexture(frame);
          SDL_GL_SetSwapInterval(1);
        }

        for(auto x : g_combatWorldEnts) {
          x->opacity_delta = -3;
          x->semisolid = 0;
          x->agrod = 0;
        }
        g_combatWorldEnts.clear();
        g_gamemode = gamemode::EXPLORATION;
        protag_can_move = 1;
        protag->dynamic = 1;
        transition = 1;
        transitionDelta = transitionImageHeight;
        combatUIManager->hideAll();
        if(g_combatEntryType == 0) {
          //continue script
          adventureUIManager->dialogue_index++;
          adventureUIManager->continueDialogue();
        } else {
          //wasn't a scripted fight
        }

        //if the player ran away, we should delete these
        for(int i = 0; i < g_enemyCombatants.size(); i++) {
          delete g_enemyCombatants[i];
        }
        g_enemyCombatants.clear();
        for(int i = 0; i < g_deadCombatants.size(); i++) {
          delete g_deadCombatants[i];
        }
        g_deadCombatants.clear();

        writeSave();

        break;
      }
    case submode::OUTWIPEL:
      {
        {
          //SDL_GL_SetSwapInterval(0);
          bool cont = false;
          float ticks = 0;
          float lastticks = 0;
          float transitionElapsed = 5;
          float mframes = 60;
          float transitionMinFrametime = 5;
          transitionMinFrametime = 1/mframes * 1000;


          SDL_Surface* transitionSurface = loadSurface("resources/engine/transition.qoi");

          int imageWidth = transitionSurface->w;
          int imageHeight = transitionSurface->h;

          SDL_Texture* transitionTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, transitionSurface->w, transitionSurface->h );
          SDL_SetTextureBlendMode(transitionTexture, SDL_BLENDMODE_BLEND);


          void* pixelReference;
          int pitch;

          float offset = imageHeight;

          SDL_Texture* frame = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, WIN_WIDTH, WIN_HEIGHT);
          SDL_SetRenderTarget(renderer, frame);

          if(combatUIManager->loadedBackground.scene[0] == '>') {
            drawBackground();
          } else {
            drawSimpleBackground();
          }
          drawCombatants();

          SDL_RenderCopy(renderer, g_shade, NULL, NULL);

          SDL_SetRenderTarget(renderer, NULL);
          SDL_RenderClear(renderer);

          while (!cont) {

            //onframe things
            SDL_LockTexture(transitionTexture, NULL, &pixelReference, &pitch);

            memcpy( pixelReference, transitionSurface->pixels, transitionSurface->pitch * transitionSurface->h);
            Uint32 format = SDL_PIXELFORMAT_ARGB8888;
            SDL_PixelFormat* mappingFormat = SDL_AllocFormat( format );
            Uint32* pixels = (Uint32*)pixelReference;
            Uint32 transparent = SDL_MapRGBA( mappingFormat, 0, 0, 0, 255);

            offset += g_transitionSpeed + 0.02 * offset;

            for(int x = 0;  x < imageWidth; x++) {
              for(int y = 0; y < imageHeight; y++) {


                int dest = (y * imageWidth) + x;
                //int src =  (y * imageWidth) + x;

                if(pow(pow(imageWidth/2 - x,2) + pow(imageHeight + y,2),0.5) < offset) {
                  pixels[dest] = transparent;
                } else {
                  pixels[dest] = 0;
                }

              }
            }





            ticks = SDL_GetTicks();
            transitionElapsed = ticks - lastticks;
            //lock framerate
            if(transitionElapsed < transitionMinFrametime) {
              SDL_Delay(transitionMinFrametime - transitionElapsed);
              ticks = SDL_GetTicks();
              transitionElapsed = ticks - lastticks;
            }
            lastticks = ticks;

            SDL_RenderClear(renderer);
            //render last frame
            //SDL_RenderCopy(renderer, frame, NULL, NULL);
            if(combatUIManager->loadedBackground.scene[0] == '>') {
              drawBackground();
            } else {
              drawSimpleBackground();
            }

            drawCombatants();

            SDL_RenderCopy(renderer, g_shade, NULL, NULL);

            SDL_UnlockTexture(transitionTexture);
            SDL_RenderCopy(renderer, transitionTexture, NULL, NULL);
            SDL_RenderPresent(renderer);

            if(offset > imageHeight + pow(pow(imageWidth/2,2) + pow(imageHeight,2),0.5)) {
              cont = 1;
            }
          }
          SDL_FreeSurface(transitionSurface);
          SDL_DestroyTexture(transitionTexture);
          SDL_DestroyTexture(frame);
          SDL_GL_SetSwapInterval(1);
        }

        adventureUIManager->executingScript = 0;

        adventureUIManager->mobilize = 0;
        adventureUIManager->hideTalkingUI();
        protag_is_talking = 2;

        g_gamemode = gamemode::LOSS;
        g_lossSub = lossSub::INWIPE;
        transitionDelta = transitionImageHeight;
        //lossUIManager->redness = 255;
        transition = 1;
        transitionDelta = transitionImageHeight;
        combatUIManager->hideAll();

        //if the player ran away, we should delete these
        for(int i = 0; i < g_enemyCombatants.size(); i++) {
          delete g_enemyCombatants[i];
        }
        g_enemyCombatants.clear();
        for(int i = 0; i < g_deadCombatants.size(); i++) {
          delete g_deadCombatants[i];
        }
        g_deadCombatants.clear();

        break;
      }
    case submode::TEXT:
      {
        curCombatantIndex = 0;
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              g_submode = submode::MAIN;
              combatUIManager->currentOption = 0;
              while(g_partyCombatants[curCombatantIndex]->health <= 0 && curCombatantIndex+1 < g_partyCombatants.size()) {
                curCombatantIndex ++;
              }
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::MAIN:
      {
        if(input[8]) {
          g_autoFight = 0;
        }

        if(g_autoFight) {

          while(curCombatantIndex < g_partyCombatants.size()) {
            g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ATTACK;
            g_partyCombatants[curCombatantIndex]->serial.actionIndex = 0;
            g_partyCombatants[curCombatantIndex]->serial.target = 0;
            curCombatantIndex++;

          }
          g_submode = submode::EXECUTE_P;
          combatUIManager->executePIndex = 0;
          curCombatantIndex = 0;

          break;
        }


        if(input[0] && !oldinput[0]) {
          if(combatUIManager->currentOption == 1 ||
              combatUIManager->currentOption == 3 ||
              combatUIManager->currentOption == 5) {
            combatUIManager->currentOption --;
          }
        }

        if(input[1] && !oldinput[1]) {
          if(combatUIManager->currentOption == 0 ||
              combatUIManager->currentOption == 2 ||
              combatUIManager->currentOption == 4) {
            combatUIManager->currentOption ++;
          }
        }

        if(input[2] && !oldinput[2]) {
          if(combatUIManager->currentOption == 2 ||
              combatUIManager->currentOption == 3 ||
              combatUIManager->currentOption == 4 ||
              combatUIManager->currentOption == 5) {
            combatUIManager->currentOption -= 2;
          }
        }

        if(input[3] && !oldinput[3]) {
          if(combatUIManager->currentOption == 0 ||
              combatUIManager->currentOption == 1 ||
              combatUIManager->currentOption == 2 ||
              combatUIManager->currentOption == 3) {
            combatUIManager->currentOption += 2;
          }
        }

        if(input[11] && !oldinput[11]) {
          switch(combatUIManager->currentOption) {
            case 0: 
              {
                //attack
                g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ATTACK;
                g_partyCombatants[curCombatantIndex]->serial.actionIndex = 0;

                //now, choose a target
                g_submode = submode::TARGETING;
                combatUIManager->currentTarget = 0;

                break;
              }
            case 1:
              {
                //Spirit move
                g_partyCombatants[curCombatantIndex]->serial.action = turnAction::SPIRITMOVE;

                //now, choose a move
                g_submode = submode::SPIRITCHOOSE;
                combatUIManager->currentInventoryOption = 0;

                break;
              }
            case 2:
              {
                //Bag
                g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ITEM;

                combatUIManager->currentInventoryOption = 0;

                //now, choose a target
                g_submode = submode::ITEMCHOOSE;
                combatUIManager->currentInventoryOption = 0;

                break;
              }
            case 3:
              {
                //Defend
                g_partyCombatants[curCombatantIndex]->serial.action = turnAction::DEFEND;
                g_partyCombatants[curCombatantIndex]->serial.actionIndex = 0;

                g_submode = submode::CONTINUE;
                break;
              }
            case 4:
              {
                //Run
                if(adventureUIManager->executingScript) {
                  //can't run from this fight
                  combatUIManager->finalText = getLanguageData("CombatCantRun");
                  combatUIManager->currentText = "";
                  combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
                  combatUIManager->dialogProceedIndicator->y = 0.25;
                  g_submode = submode::RUNWARNING;

                } else {
                  g_partyCombatants[curCombatantIndex]->serial.action = turnAction::FLEE;
                  g_submode = submode::CONTINUE;
                  break;



                }

                break;
              }
            case 5:
              {
                //Autofight
                g_autoFight = 1;

                while(curCombatantIndex < g_partyCombatants.size()) {
                  g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ATTACK;
                  g_partyCombatants[curCombatantIndex]->serial.actionIndex = 0;
                  g_partyCombatants[curCombatantIndex]->serial.target = 0;
                  curCombatantIndex++;

                }
                g_submode = submode::EXECUTE_P;
                combatUIManager->executePIndex = 0;
                curCombatantIndex = 0;

                break;
              }

          }

        }

        if(input[8] && !oldinput[8]) {
          if(curCombatantIndex > 0) {
            int newCombatantIndex = curCombatantIndex - 1;
            while(newCombatantIndex >= 0 && g_partyCombatants[newCombatantIndex]->health <= 0) {
              newCombatantIndex--;
            }
            if(newCombatantIndex >= 0) {
              curCombatantIndex = newCombatantIndex;
              g_submode = submode::MAIN;
              combatUIManager->currentOption = 0;
            }
          }
        }


        combatUIManager->optionsPanel->show = 1;
        combatUIManager->menuPicker->show = 1;
        combatUIManager->optionsText->show = 1;
        combatUIManager->optionsMiniText->show = 1;

        drawOptionsPanel();
        break;
      }
    case submode::TARGETING: 
      {
        combatUIManager->mainPanel->show = 0;
        combatUIManager->dialogProceedIndicator->show = 0;
        combatUIManager->mainText->show = 0;

        combatUIManager->optionsPanel->show = 1;
        combatUIManager->menuPicker->show = 1;
        combatUIManager->optionsText->show = 1;

        combatUIManager->targetPanel->show = 1;
        combatUIManager->targetText->show = 1;

        combatUIManager->tcm_accumulator += 0.1;
        if(combatUIManager->tcm_accumulator > M_PI * 2) {
          combatUIManager->tcm_accumulator -= M_PI * 2;
        }

        //combatUIManager->targetingColorMod = (sin(combatUIManager->tcm_accumulator) + 1) * 128;
        combatUIManager->targetingColorMod = 128;


        if(input[2] && !oldinput[2]) {
          if(combatUIManager->currentTarget > 0) {
            combatUIManager->currentTarget --;
          }
        }

        if(input[3] && !oldinput[3]) {
          if(combatUIManager->currentTarget < g_enemyCombatants.size() - 1) {
            combatUIManager->currentTarget ++;
          }
        }

        if(combatUIManager->currentTarget < 0) { combatUIManager->currentTarget = 0; }
        if(combatUIManager->currentTarget >= g_enemyCombatants.size()) { combatUIManager->currentTarget = g_enemyCombatants.size() - 1; }


        if(input[11] && !oldinput[11]) {
          g_partyCombatants[curCombatantIndex]->serial.target = combatUIManager->currentTarget;
          g_submode = submode::CONTINUE;
        }

        if(input[8] && !oldinput[8]) {
          g_submode = submode::MAIN;
          combatUIManager->currentOption = 0;
        }


        drawOptionsPanel();

        //combatUIManager->targetText->updateText("To " + g_enemyCombatants.at(combatUIManager->currentTarget)->name, -1, 34);
        combatUIManager->targetText->updateText(combatUIManager->directionalPreposition + " " + g_enemyCombatants.at(combatUIManager->currentTarget)->name, -1, 34);

        combatUIManager->targetPanel->render(renderer, g_camera, elapsed);
        combatUIManager->targetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);



        break;
      }
    case submode::CONTINUE:
      {

        drawOptionsPanel();
        if(curCombatantIndex == g_partyCombatants.size() - 1) {
          g_submode = submode::EXECUTE_P;
          combatUIManager->executePIndex = 0;
          curCombatantIndex = 0;
        } else {
          g_submode = submode::MAIN;
          combatUIManager->currentOption = 0;
          curCombatantIndex++;
          while(g_partyCombatants[curCombatantIndex]->health <= 0) {
            curCombatantIndex++;
            if(curCombatantIndex >= g_partyCombatants.size()) {

              for(auto x : g_enemyCombatants) {
                x->damageTakenThisTurn = 0;
              }

              g_submode = submode::EXECUTE_P;
              combatUIManager->executePIndex = 0;
              curCombatantIndex = 0;
            }
          }
        }


        break;
      }
    case submode::EXECUTE_P:
      {

        while(combatUIManager->executePIndex+1 <= g_partyCombatants.size() && g_partyCombatants[combatUIManager->executePIndex]->health <= 0) {
          combatUIManager->executePIndex++;
        }
        combatant* c = g_partyCombatants[combatUIManager->executePIndex];
        if(combatUIManager->executePIndex == g_partyCombatants.size() - 1 && g_partyCombatants[combatUIManager->executePIndex]->health <= 0) {
          //reset all stats
          for(auto x : g_partyCombatants) {
            x->curStrength = x->baseStrength;
            x->curMind = x->baseMind;
            x->curAttack = x->baseAttack;
            x->curDefense = x->baseDefense;
            x->curSoul = x->baseSoul;
            x->curSkill = x->baseSkill;
            x->curCritical = x->baseCritical;
            x->curRecovery = x->baseSoul;
            x->physicalDefense = 0;
            x->spiritDefense = 0;
          }
          g_submode = submode::STATUS_P;
          break;
        }
        if(combatUIManager->executePIndex >= g_partyCombatants.size()) {
          //reset all stats
          for(auto x : g_partyCombatants) {
            x->curStrength = x->baseStrength;
            x->curMind = x->baseMind;
            x->curAttack = x->baseAttack;
            x->curDefense = x->baseDefense;
            x->curSoul = x->baseSoul;
            x->curSkill = x->baseSkill;
            x->curCritical = x->baseCritical;
            x->curRecovery = x->baseSoul;
            x->physicalDefense = 0;
            x->spiritDefense = 0;
          }
          g_submode = submode::STATUS_P;
          break;
        }
        if(c->serial.action == turnAction::ATTACK) {
          if(g_enemyCombatants.size() == 0) {
            g_submode = submode::FINAL;
            break;
          }
          while(c->serial.target >= g_enemyCombatants.size()) {
            c->serial.target-= 1;
          }
          combatant* e = g_enemyCombatants[c->serial.target];

          //do we crit?
          bool crit = 0;
          if(frng(0, 100) < c->curCritical) {
            crit = 1;
          }
          
          //chant wears off
          for(auto &x : c->statuses) {
            if(x.type == status::CHANTED) {
              x.turns = 0;
            }
          }


          int damage = c->curAttack - e->curDefense - e->physicalDefense;
          damage *= frng(0.85,1.15);
          if(crit) { damage *= 3;}
          if(damage < 0) {damage = 0;}
          int dmgToReport = damage;
          if(e->health < dmgToReport) {
            dmgToReport = e->health;
          }
          c->dmgDealtOverFight += dmgToReport;

          e->health -= damage;
          e->damageTakenThisTurn += damage;
          string message;
          if(crit) {
            //message = c->name + " crits " + e->name + " for " + to_string(damage) + ".";
            message = getLanguageData("CombatProtagCrit");
            message = stringMultiInject(message, {c->name, e->name, to_string(damage)});
          } else {
            //message = c->name + " deals " + to_string(damage) + " to " + e->name + ".";
            message = getLanguageData("CombatProtagAttack");
            message = stringMultiInject(message, {c->name, e->name, to_string(damage)});
          }


          if(e->health < 0) {
            string deathmessage = e->name + " " +  e->deathText;
            combatUIManager->queuedStrings.push_back(make_pair(deathmessage, e));
            g_enemyCombatants.erase(g_enemyCombatants.begin() + c->serial.target);
            g_deadCombatants.push_back(e);
            //delete e;
          }

          combatUIManager->finalText = message;
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          g_submode = submode::TEXT_P;
        } else if(c->serial.action == turnAction::ITEM) {
          if(g_enemyCombatants.size() == 0) {
            g_submode = submode::FINAL;
            break;
          }
          while(c->serial.target >= g_partyCombatants.size()) {
            c->serial.target-= 1;
            if(c->serial.target <0) {break;}
          }
          combatant* com = g_partyCombatants[combatUIManager->executePIndex];
          int type = com->serial.actionIndex;
          int index = com->serial.secondActionIndex;
          int b = com->serial.target; //which ally/enemy
          com->itemIndexToUse = -1;

          useItem(type, index, b, com);

          g_submode = submode::TEXT_P;

        } else if(c->serial.action == turnAction::SPIRITMOVE) {
          if(g_enemyCombatants.size() == 0) {
            g_submode = submode::FINAL;
            break;
          }

          if(spiritTable[c->serial.actionIndex].targeting == 0) {
            while(c->serial.target >= (int)g_enemyCombatants.size()) {
              c->serial.target-= 1;
              M("Stuck in loop which confused me earlier");
            }
          }

          combatant* com = g_partyCombatants[combatUIManager->executePIndex];
          int whichSpiritAbility = com->serial.actionIndex; //which spirit ability
          int target = com->serial.target;
          useSpiritMove(whichSpiritAbility, target, com);
          g_submode = submode::TEXT_P;
        } else if(c->serial.action == turnAction::DEFEND) {
          if(g_enemyCombatants.size() == 0) {
            g_submode = submode::FINAL;
            break;
          }
          string text = getLanguageData("CombatProtagShrinks");
          text = stringMultiInject(text, {g_partyCombatants[combatUIManager->executePIndex]->name});
          combatUIManager->finalText = text;

          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dodgingThisTurn[combatUIManager->executePIndex] = 1;
//          for(auto x :combatUIManager->dodgingThisTurn) {
//            D(x);
//          }
          g_submode = submode::TEXT_P;
        } else if(c->serial.action == turnAction::FLEE) {
          int levelDifference = 0;
          int highestTeamateLevel = 0;
          for(auto x : g_partyCombatants) {
            if(x->level > highestTeamateLevel) {
              highestTeamateLevel = x->level;
            }
          }
          int highestEnemyLevel = 0;
          for(auto x : g_enemyCombatants) {
            if(x->level > highestEnemyLevel) {
              highestEnemyLevel = x->level;
            }
          }
          levelDifference = highestTeamateLevel - highestEnemyLevel;
          int random = rng(0,10);
          if(random + levelDifference >= 7) {
            //successful fleeing
            combatUIManager->finalText = g_partyCombatants[combatUIManager->executePIndex]->name + getLanguageData("CombatRunAttempt");
            combatUIManager->queuedStrings.push_back(make_pair(getLanguageData("CombatRunSuccess"), (combatant*)0));
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            combatUIManager->dialogProceedIndicator->y = 0.25;
            g_submode = submode::RUNSUCCESSTEXT;

          } else {
            //failed fleeing
            combatUIManager->finalText = g_partyCombatants[combatUIManager->executePIndex]->name + getLanguageData("CombatRunAttempt");
            combatUIManager->queuedStrings.push_back(make_pair(getLanguageData("CombatRunFail"), (combatant*)0));
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            combatUIManager->dialogProceedIndicator->y = 0.25;
            g_submode = submode::RUNFAILTEXT;

          }
        }

        break;
      }
    case submode::TEXT_P: 
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {
          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }
          curTextWait = 0;
        }

        if( combatUIManager->finalText == combatUIManager->currentText && input[11] && !oldinput[11]) {
          //advance dialog
          if(combatUIManager->queuedStrings.size() > 0) {
            combatUIManager->dialogProceedIndicator->y = 0.25;
            combatUIManager->currentText = "";
            combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;

            combatant* whoDied = combatUIManager->queuedStrings.at(0).second;
            if(whoDied != 0) {
              int index = 0;
              while(index < g_deadCombatants.size()) {
                if(g_deadCombatants.at(index) == whoDied) {
                  whoDied->disappearing = 1;
                  break;
                }
                index++;
              }
            }

            combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
          } else {

            //make sure to handle death from selfdamage properly
            int deadPartyMembers = 0;
            for(auto x : g_partyCombatants) {
              if(x->health <= 0) {
                deadPartyMembers++;
              }
            }

            if(g_partyCombatants[combatUIManager->executePIndex]->health <= 0) {

              combatUIManager->finalText = g_partyCombatants[combatUIManager->executePIndex]->name + " passed out!";
              combatUIManager->currentText = "";
              combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            }

            if(deadPartyMembers == g_partyCombatants.size()) {
              string message = "All party members are knocked-out!";
              combatUIManager->queuedStrings.clear();
              combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
              g_submode = submode::ALLDEADTEXT;
              break;

            }

            if(g_partyCombatants[combatUIManager->executePIndex]->health <= 0) {
              g_submode = submode::MEMBERDEADTEXT_P;
              break;
            }
            

            if(combatUIManager->executePIndex + 1 ==  g_partyCombatants.size()) {
              //reset all stats
              for(auto x : g_partyCombatants) {
                x->curStrength = x->baseStrength;
                x->curMind = x->baseMind;
                x->curAttack = x->baseAttack;
                x->curDefense = x->baseDefense;
                x->curSoul = x->baseSoul;
                x->curSkill = x->baseSkill;
                x->curCritical = x->baseCritical;
                x->curRecovery = x->baseSoul;
                x->physicalDefense = 0;
                x->spiritDefense = 0;
              }
              g_submode = submode::STATUS_P;
            } else {
              combatUIManager->executePIndex++;
              g_submode = submode::EXECUTE_P;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }




        break;
      }
    case submode::EXECUTE_E:
      {
        combatUIManager->dodgerAngle = frng(0,2*M_PI);
        //it's possible that the protags died during or right after their turn
        //it can crash in this switch case if you aren't careful handling
        //status damage or self damage

        if(combatUIManager->executeEIndex >= g_enemyCombatants.size()) {
          //fight over
          g_submode = submode::FINAL;
          break;
        }
        combatant* c = g_enemyCombatants[combatUIManager->executeEIndex];

        //temp
        c->serial.action = turnAction::ATTACK;
        //this should connect to specialcombatants.cpp
        //by an identity field

        if(c->serial.action == turnAction::ATTACK) {

          //if the enemy attacker is blind, we should display a different message
          float blindedChance = 0; // (0-1)
          for(auto &x : c->statuses) {
            if(x.type == status::BLINDED) {
              blindedChance = x.magnitude;
              break;
            }
          }
          float random = frng(0,1);
          if(random > (1-blindedChance)) {
            //the enemy failed to attack
            combatUIManager->finalText = stringMultiInject(getLanguageData("EnemyBlindedText"), {c->name, getPossessivePronoun(c)});
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            combatUIManager->dialogProceedIndicator->y = 0.25;
            g_submode = submode::TEXT_ENEMY_BLINDED;
            break;
          }

          //        combatant* e = g_partyCombatants[rng(0, g_partyCombatants.size() - 1)];
          //        int damage = c->baseAttack + (c->attackGain * c->level) - (e->baseDefense + (e->defenseGain * e->level));
          //        damage *= frng(0.70,1.30);
          //        e->health -= damage;
          //        string message = c->name + " deals " + to_string(damage) + " to " + e->name + "!";
          //        g_submode = submode::TEXT_E;
          //        combatUIManager->finalText = message;
          //        combatUIManager->currentText = "";
          //        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          //        combatUIManager->dialogProceedIndicator->y = 0.25;
          //        combatUIManager->mainPanel->show = 0;
          //        combatUIManager->dialogProceedIndicator->show = 0;
          //        combatUIManager->mainText->show = 0;

          vector<combatant*> validCombatants = {};
          for(int i = 0; i < g_partyCombatants.size(); i++) {
            if(g_partyCombatants[i]->health > 0) {
              validCombatants.push_back(g_partyCombatants[i]);
            }
          }

          if(validCombatants.size() <=0) { abort();}

          //int dodgingIndex = rng(0, validCombatants.size() - 1);
          //execute script to determine:
          // who to attack (/target -> dodging index)
          // what patterns to use (/addpattern -> combatUIManager->curPatterns)
          // base dmg per level (/damage ->
          //

          int dodgingIndex = 0;
          string targeting = "";
          vector<int> patterns = {};
          runCombatScript(c->combatScript, combatUIManager->turnCounter, c, targeting, patterns, combatUIManager->specificMultiplier);


          //use targeting to set dodgingIndex
          if(targeting == "lowest") {
            int least = 1000000;
            for(int i = 0; i < validCombatants.size(); i++) {
              if(validCombatants[i]->health < least) {
                dodgingIndex = i;
                least = validCombatants[i]->health;
              }
            }
          } else if(targeting == "highest") {
            int highest = -1;
            for(int i = 0; i < validCombatants.size(); i++) {
              if(validCombatants[i]->health > highest) {
                dodgingIndex = i;
                highest = validCombatants[i]->health;
              }
            }
            
          } else if(targeting == "random") {
            dodgingIndex = rng(0, validCombatants.size() - 1);
          } else if(targeting == "absolute0") {
            //Fomm
            dodgingIndex = 0;
          } else if(targeting == "absolute1") {
            //Neheten
            dodgingIndex = 1;
          } else if(targeting == "absolute2") {
            //Blish
            dodgingIndex = 2;
          } else if(targeting == "absolute3") {
            //Dafua
            dodgingIndex = 3;
          }

          combatUIManager->curPatterns.clear();
          for(auto x : patterns) {
            combatUIManager->curPatterns.push_back(x);
          }





          //check for taunt
          bool breakflag = 0;
          for(auto &x : c->statuses) {
            if(x.type == status::TAUNTED) {
              for(int i = 0; i < validCombatants.size(); i++) {
                if(validCombatants[i]->filename == x.datastr) {
                  dodgingIndex = i;
                  breakflag = 1;
                  break;
                }
              }
            }
            if(breakflag) break;
          }

          combatant* e = validCombatants[dodgingIndex];
          int adjustedDIndex = 0;
          for(auto x : g_partyCombatants) {
            if(x == e) {
              break;
            }
            adjustedDIndex++;
          }
          combatUIManager->partyDodgingCombatant = e;
          //combatUIManager->partyDodgingCombatant = protag->hisCombatant;

          if(combatUIManager->partyDodgingCombatant->filename == "common/fomm") {combatUIManager->dodgerTexture = combatUIManager->fommDodgerTex;}
  
          if(combatUIManager->partyDodgingCombatant->filename == "common/neheten") {combatUIManager->dodgerTexture = combatUIManager->nehetenDodgerTex;}
  
          if(combatUIManager->partyDodgingCombatant->filename == "common/blish") {combatUIManager->dodgerTexture = combatUIManager->blishDodgerTex;}
  
          if(combatUIManager->partyDodgingCombatant->filename == "common/dafua") {combatUIManager->dodgerTexture = combatUIManager->dafuaDodgerTex;}

          int damage = (c->curAttack* combatUIManager->specificMultiplier) - e->curDefense - e->physicalDefense;
          damage *= frng(0.85,1.15);
          bool skipDodgingPhase = 0;
          if(damage < 0) {
            damage = 0;
          }
          combatUIManager->damageFromEachHit = damage;
          combatUIManager->dodgePanel->x = combatUIManager->dodgePanelSmallX;
          combatUIManager->dodgePanel->y = combatUIManager->dodgePanelSmallY;
          combatUIManager->dodgePanel->width = combatUIManager->dodgePanelSmallWidth;
          combatUIManager->dodgePanel->height = combatUIManager->dodgePanelSmallHeight;
          combatUIManager->dodgePanel->show = 1;
          combatUIManager->incrementDodgeTimer = 0;
          combatUIManager->dodgeTimer = 0;
          combatUIManager->damageTakenFromDodgingPhase = 0;
          combatUIManager->invincibleMs = 0;

          //g_submode = submode::DODGING;
          //string message = c->name + " attacks " + e->name + " for " + to_string(damage) + " damage.";
          string message = getLanguageData("CombatEnemyAttack");
          message = stringMultiInject(message, {c->name, e->name, to_string(damage)});

//          D(adjustedDIndex);
//          for(auto x :combatUIManager->dodgingThisTurn) {
//            D(x);
//          }

          if(combatUIManager->dodgingThisTurn[adjustedDIndex] == 1 || g_shrinkTurns > 0) {
            //M("Should shrink");
            combatUIManager->shrink = 1;
            g_shrinkTurns --;
          } else {
            //M("Shouldn't shrink");
            combatUIManager->shrink = 0;
          }

          combatUIManager->finalText = message;
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;

          if(combatUIManager->curPatterns.size() == 0) {
            combatUIManager->finalText = combatUIManager->idleText;
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            combatUIManager->dialogProceedIndicator->y = 0.25;

            //check for steeltrap
            bool st = 0;
            int damage = 0;
            for(auto &x : c->statuses) {
              if(x.type == status::STEELTRAPPED) {
                st = 1;
                damage = x.magnitude;
                break;
              }
            }
            if(st) {
              damage *= frng(0.8, 1.2);
              damage *= 1000;
              combatUIManager->finalText = stringMultiInject(getLanguageData("SteeltrapProc"), {c->name, to_stringF(damage)});
              if(damage < 0) { damage = 0;}
              c->health -= damage;
              c->damageTakenThisTurn += damage; //maybe remove this
              if(c->health <= 0) {
                string deathmessage = c->name + " " + c->deathText;
                combatUIManager->queuedStrings.push_back(make_pair(deathmessage,c));
                g_enemyCombatants.erase(g_enemyCombatants.begin() + combatUIManager->executeEIndex);
                g_deadCombatants.push_back(c);
                combatUIManager->executeEIndex--;

//                blah blah bug here
//                joseph don't forget to fix death from stickybomb status
//                also the items check for death if health < 0 not <=

              }



            }

            g_submode = submode::TEXT_IDLE;
            break;



          } else {
//            if(skipDodgingPhase) {
//              //The enemy did a puny attack
//              string message = getLanguageData("CombatEnemyAttack");
//
//              combatUIManager->finalText = message;
//              combatUIManager->currentText = "";
//              combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
//              combatUIManager->dialogProceedIndicator->y = 0.25;
//              g_submode = submode::TEXT_PUNY;
//            }

            g_submode = submode::TEXT_E;
          }
        }

        break;
      }
    case submode::TEXT_E:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          } 
          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText && input[11] && !oldinput[11]) {
          //advance dialog
          if(combatUIManager->queuedStrings.size() > 0) {
            combatUIManager->dialogProceedIndicator->y = 0.25;
            combatUIManager->currentText = "";
            combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
            combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
          } else {

            if(combatUIManager->damageFromEachHit <1) {
              if(combatUIManager->executeEIndex + 1 ==  g_enemyCombatants.size()) {
                combatUIManager->mainPanel->show = 0;
                combatUIManager->mainText->show = 0;
                combatUIManager->dialogProceedIndicator->show = 0;
                curCombatantIndex = 0;
                curStatusIndex = 0;
    
                //reset all stats
                for(auto x : g_enemyCombatants) {
                  x->curStrength = x->baseStrength;
                  x->curMind = x->baseMind;
                  x->curAttack = x->baseAttack;
                  x->curDefense = x->baseDefense;
                  x->curSoul = x->baseSoul;
                  x->curSkill = x->baseSkill;
                  x->curCritical = x->baseCritical;
                  x->curRecovery = x->baseSoul;
                  x->physicalDefense = 0;
                  x->spiritDefense = 0;
                }
    
                g_submode = submode::STATUS_E;
                break;
              } else {
                combatUIManager->executeEIndex++;
                g_submode = submode::EXECUTE_E;
                break;
              }
            }

            g_submode = submode::DODGING;
            for(int i = 0; i < g_partyCombatants.size(); i++) {
              if(combatUIManager->partyDodgingCombatant == g_partyCombatants[i]) {


              }
            }
            combatUIManager->accuA = 1000000;
            combatUIManager->accuB = 1000000;
            combatUIManager->accuC = 1000000;
            combatUIManager->dodgerX = 512;
            combatUIManager->dodgerY = 512;
            combatant* e = g_enemyCombatants[combatUIManager->executeEIndex];
  
  //          if(e->attackPatterns.size() <0) {
  //            E("Add attack patterns for " + e->name);
  //            abort();
  //          }
            //combatUIManager->curPatterns = e->attackPatterns[rng(0, e->attackPatterns.size()-1)];
  
  //          M("Spawning bullets for");
  //
  //          for(auto x : combatUIManager->curPatterns) {
  //            cout << x << " ";
  //          }
  //          cout << endl;
  
            for(int i = 0; i < g_miniEnts.size(); i++) {
              delete g_miniEnts[i];
              i--;
            }
            g_miniBullets.clear();
            
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::STATUS_E:
      {
        int breakout = 0;
        //got a crash here after idle text state
        
        //while(curCombatantIndex <0) { curCombatantIndex++;}
//        if(g_enemyCombatants.size() == 0) {
//          M("Latest new check");
//          g_submode = submode::FINAL;
//          break;
//        }
        
        //the "curCombatantIndex < 0" is for when 
        //an enemy dies from a status, and curCombatantIndex
        //is decremented, which might go to <0
        while(curCombatantIndex< 0 || curStatusIndex >= (int)g_enemyCombatants[curCombatantIndex]->statuses.size()) {
          curStatusIndex = 0;
          curCombatantIndex++;
          if(curCombatantIndex == (int)g_enemyCombatants.size()) {
            curCombatantIndex = 0;
            for(int i = 0; i < 4; i ++) {
              combatUIManager->dodgingThisTurn[i] = 0;
            }
            while(g_partyCombatants[curCombatantIndex]->health <= 0 && curCombatantIndex+1 < (int)g_partyCombatants.size()) {
              curCombatantIndex ++; //used for choosing which protag picks action in submode::MAIN
            }
            combatUIManager->currentOption = 0;
            combatUIManager->turnCounter++;
            g_submode = submode::MAIN;
            breakout = 1;
            break;

          }
        }
        if(breakout) {break;}

        combatant* c = g_enemyCombatants[curCombatantIndex];
        if(applyStatus(c, &c->statuses[curStatusIndex])) {
          c->statuses.erase(c->statuses.begin() + curStatusIndex);
          curStatusIndex--;
        }

        break;
      }
    case submode::TEXT_STATUS_E:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;

              combatant* whoDied = combatUIManager->queuedStrings.at(0).second;
              if(whoDied != 0) {
                int index = 0;
                while(index < g_deadCombatants.size()) {
                  if(g_deadCombatants.at(index) == whoDied) {
                    whoDied->disappearing = 1;
                    break;
                  }
                  index++;
                }
              }
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              M("Did the last enemy die of a status?");
              //this is used when the last enemy dies from a status
              if(g_enemyCombatants.size() == 0) {
                M("Yes they did");
                g_submode = submode::FINAL;
                break;
              }

              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              curStatusIndex++;

              //reset all stats
              for(auto x : g_enemyCombatants) {
                x->curStrength = x->baseStrength;
                x->curMind = x->baseMind;
                x->curAttack = x->baseAttack;
                x->curDefense = x->baseDefense;
                x->curSoul = x->baseSoul;
                x->curSkill = x->baseSkill;
                x->curCritical = x->baseCritical;
                x->curRecovery = x->baseSoul;
                x->physicalDefense = 0;
                x->spiritDefense = 0;
              }

              g_submode = submode::STATUS_E;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::FINAL:
      {
        //calculate XP based on total stats of defeated enemies
        //award xp grant xp award exp grant exp give xp give exp

        //return held items to inventory (combatant.itemToUse)
        for(auto x : g_partyCombatants) {
          if(x->itemIndexToUse != -1) {
            if(g_items.size() < g_maxInventorySize) {
              itemData td;
              td.type = x->itemIndexToUse;
              td.index = x->itemIndexToUse;
              g_items.push_back(td);
              x->itemIndexToUse = -1;
            }

          }
        }

        //apply recovery
        for(auto x : g_partyCombatants) {
          if(x->health > 0) {
            x->health += (x->curRecovery/100.0f) * x->baseStrength;
            x->sp += (x->curRecovery/100.0f) * x->baseMind;
            if(x->health > floor(x->baseStrength)) {
              x->health = floor(x->baseStrength);
            }
            x->curStrength = x->baseStrength;
            if(x->sp > floor(x->baseMind)) {
              x->sp = floor(x->baseMind);
            }
            x->curMind = x->baseMind;
          }
        }

        combatUIManager->calculateXP();
        //combatUIManager->xpToGrant = 1000;
        curCombatantIndex = 0;

        g_submode = submode::CHARAXP;

        //      combatUIManager->currentText = "";
        //      combatUIManager->finalText = "Fomm has won the battle!";
        //      g_submode = submode::FINALTEXT;
        break;
      }
    case submode::CHARAXP:
      {
        while(curCombatantIndex < g_partyCombatants.size() && g_partyCombatants[curCombatantIndex]->health <= 0) {
          curCombatantIndex++;
        }
        if(curCombatantIndex >= g_partyCombatants.size()) {
          //        combatUIManager->currentText = "";
          //        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          //        combatUIManager->finalText = "Fomm has won the battle!";
          //        g_submode = submode::FINALTEXT;
          //        break;
          //g_submode = submode::OUTWIPE;
          g_submode = submode::DROPITEMS;
          break;
        }
        combatant* x = g_partyCombatants[curCombatantIndex];
        x->level = xpToLevel(x->xp);
        combatUIManager->oldLevel = x->level;
        x->xp += combatUIManager->xpToGrant * frng(0.8, 1.2);
        combatUIManager->newLevel= xpToLevel(x->xp);
        combatUIManager->thisLevel = combatUIManager->oldLevel+1;
        //g_submode = submode::LEVELUP;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " gains " + to_string(combatUIManager->xpToGrant) + " xp.";
        g_submode = submode::XPTEXT;


        break;
      }
    case submode::XPTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              g_submode = submode::LEVELUP;
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::LEVELUP:
      {
        if(combatUIManager->thisLevel > combatUIManager->newLevel) {
          curCombatantIndex++;
          g_submode = submode::CHARAXP;
          break;
        }

        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        string line = "";
        line = stringMultiInject(getLanguageData("CombatLevelUp0"), {g_partyCombatants[curCombatantIndex]->name, to_string(combatUIManager->thisLevel)});
        combatUIManager->finalText = line;


        g_partyCombatants[curCombatantIndex]-> strIncrease = g_partyCombatants[curCombatantIndex]->strengthGain * frng(0.8, 1.2);
        g_partyCombatants[curCombatantIndex]->baseStrength += g_partyCombatants[curCombatantIndex]->strIncrease;
        g_partyCombatants[curCombatantIndex]->health += g_partyCombatants[curCombatantIndex]->strIncrease;
        string line2 = stringMultiInject(getLanguageData("CombatLevelUp1"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseStrength), to_stringF(g_partyCombatants[curCombatantIndex]->strIncrease)});
        //combatUIManager->queuedStrings.push_back(make_pair(line,0));
        //combatUIManager->finalText += "\n" + line2;

        g_partyCombatants[curCombatantIndex]->mindIncrease = g_partyCombatants[curCombatantIndex]->mindGain * frng(0.8, 1.2);
        g_partyCombatants[curCombatantIndex]->baseMind += g_partyCombatants[curCombatantIndex]->mindIncrease;
        g_partyCombatants[curCombatantIndex]->sp += g_partyCombatants[curCombatantIndex]->mindIncrease;
        line = stringMultiInject(getLanguageData("CombatLevelUp2"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseMind), to_stringF(g_partyCombatants[curCombatantIndex]->mindIncrease)});
        combatUIManager->queuedStrings.push_back(make_pair(line2 + "\n" + line,(combatant*)0));
        //combatUIManager->queuedStrings.push_back(make_pair(line,0));

        g_partyCombatants[curCombatantIndex]->attackIncrease = g_partyCombatants[curCombatantIndex]->attackGain * frng(0.8, 1.2);

        g_partyCombatants[curCombatantIndex]->baseAttack += g_partyCombatants[curCombatantIndex]->attackIncrease;
        line2 = stringMultiInject(getLanguageData("CombatLevelUp3"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseAttack), to_stringF(g_partyCombatants[curCombatantIndex]->attackIncrease)});

        g_partyCombatants[curCombatantIndex]->defenseIncrease = g_partyCombatants[curCombatantIndex]->defenseGain * frng(0.8, 1.2);
        g_partyCombatants[curCombatantIndex]->baseDefense += g_partyCombatants[curCombatantIndex]->defenseIncrease;
        line = stringMultiInject(getLanguageData("CombatLevelUp4"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseDefense), to_stringF(g_partyCombatants[curCombatantIndex]->defenseIncrease)});
        combatUIManager->queuedStrings.push_back(make_pair(line2 + "\n" + line,(combatant*)0));

        g_partyCombatants[curCombatantIndex]->soulIncrease = g_partyCombatants[curCombatantIndex]->soulGain * frng(0.8, 1.2);
        g_partyCombatants[curCombatantIndex]->baseSoul += g_partyCombatants[curCombatantIndex]->soulIncrease;
        line2 = stringMultiInject(getLanguageData("CombatLevelUp5"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseSoul), to_stringF(g_partyCombatants[curCombatantIndex]->soulIncrease)});

        g_partyCombatants[curCombatantIndex]->criticalIncrease = g_partyCombatants[curCombatantIndex]->criticalGain * frng(0.8, 1.2);

        g_partyCombatants[curCombatantIndex]->baseCritical += g_partyCombatants[curCombatantIndex]->criticalIncrease;

        line = stringMultiInject(getLanguageData("CombatLevelUp6"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseCritical), to_stringF(g_partyCombatants[curCombatantIndex]->criticalIncrease)});
        combatUIManager->queuedStrings.push_back(make_pair(line2 + "\n" + line,(combatant*)0));


        g_partyCombatants[curCombatantIndex]->skillIncrease = g_partyCombatants[curCombatantIndex]->skillGain * frng(0.8, 1.2);
        g_partyCombatants[curCombatantIndex]->baseSkill += g_partyCombatants[curCombatantIndex]->skillIncrease;
        line = stringMultiInject(getLanguageData("CombatLevelUp7"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseSkill), to_stringF(g_partyCombatants[curCombatantIndex]->skillIncrease)});


        g_partyCombatants[curCombatantIndex]->recoveryIncrease = g_partyCombatants[curCombatantIndex]->recoveryGain * frng(0.8, 1.2);
        g_partyCombatants[curCombatantIndex]->baseRecovery += g_partyCombatants[curCombatantIndex]->recoveryIncrease;
        line2 = stringMultiInject(getLanguageData("CombatLevelUp8"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseRecovery), to_stringF(g_partyCombatants[curCombatantIndex]->recoveryIncrease)});


        combatUIManager->queuedStrings.push_back(make_pair(line + "\n" + line2,(combatant*)0));


        g_partyCombatants[curCombatantIndex]->level = combatUIManager->thisLevel;

        g_submode = submode::LEVELTEXT;



        break;
      }
    case submode::FINALTEXT:
      {
        for(int i = 0; i < g_enemyCombatants.size(); i++) {
          delete g_enemyCombatants[i];
        }
        g_enemyCombatants.clear();
        for(int i = 0; i < g_deadCombatants.size(); i++) {
          delete g_deadCombatants[i];
        }
        g_deadCombatants.clear();
        curCombatantIndex = 0;
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              g_submode = submode::OUTWIPE;
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::LEVELTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              //check and see if they can learn a spiritmove
              bool canLearnMove = 0;
              for(auto x : g_partyCombatants[curCombatantIndex]->spiritTree) {
                int a = x.first;
                int b = x.second;
                if(a == combatUIManager->thisLevel) {
                  canLearnMove = 1;
                  combatUIManager->moveToLearn = x.second;
                  break;
                }
              }

              if(canLearnMove) {
                if(g_partyCombatants[curCombatantIndex]->spiritMoves.size() < 4) {
                  //just learn it 
                  combatUIManager->currentText = "";
                  combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
                  //combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " learned " + spiritTable[combatUIManager->moveToLearn].name + ".";
                  combatUIManager->finalText = stringMultiInject(getLanguageData("LearnedMoveSuccess"), {g_partyCombatants[curCombatantIndex]->name, spiritTable[combatUIManager->moveToLearn].name});
                  g_partyCombatants[curCombatantIndex]->spiritMoves.push_back(combatUIManager->moveToLearn);
                  g_submode = submode::LEARNEDTEXT;
                  break;
                } else {
                  combatUIManager->currentText = "";
                  combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
                  //combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " can learn " + spiritTable[combatUIManager->moveToLearn].name + ", but would need to forget another move.";
                  combatUIManager->finalText = stringMultiInject(getLanguageData("LearnedMovePossible"), {g_partyCombatants[curCombatantIndex]->name, spiritTable[combatUIManager->moveToLearn].name});

                  //combatUIManager->queuedStrings.push_back(make_pair("Choose a move for " + g_partyCombatants[curCombatantIndex]->name + " to do without.",0));
                  combatUIManager->queuedStrings.push_back(make_pair(stringMultiInject(getLanguageData("LearnedMovePossible2"), {g_partyCombatants[curCombatantIndex]->name}),(combatant*)0));

                  g_submode = submode::LEARNTEXT;
                  break;
                }
              } else {
                //no move to learn, go to the next levelup
                combatUIManager->thisLevel++;
                g_submode = submode::LEVELUP;
                break;
              }
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::LEARNEDTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              //no move to learn, go to the next levelup
              combatUIManager->thisLevel++;
              g_submode = submode::LEVELUP;

              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::LEARNTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              g_submode = submode::FORGET;
              combatUIManager->forgetPanel->show = 1;
              combatUIManager->forgetInfoPanel->show = 1;
              combatUIManager->forgetText->show = 1;
              combatUIManager->forgetInfoText->show = 1;
              combatUIManager->forgetOption = 0;
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::FORGET:
      {
        if(input[0] && !oldinput[0]) {
          if(combatUIManager->forgetOption > 0) {
            combatUIManager->forgetOption -= 1;
          }
        }

        if(input[1] && !oldinput[1]) {
          if(combatUIManager->forgetOption < 4) {
            combatUIManager->forgetOption += 1;
          }
        }
        if(input[11] && !oldinput[11]) {
          if(combatUIManager->forgetOption < 4) {
            //combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " will forget " + spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->forgetOption]].name + " and learn " + spiritTable[combatUIManager->moveToLearn].name + ".";
            combatUIManager->finalText = stringMultiInject(getLanguageData("ForgetMoveConfirm"), {g_partyCombatants[curCombatantIndex]->name, spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->forgetOption]].name, spiritTable[combatUIManager->moveToLearn].name});
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            g_submode = submode::FORGETTEXT;
            combatUIManager->forgetPanel->show = 0;
            combatUIManager->forgetInfoPanel->show = 0;
            combatUIManager->forgetText->show = 0;
            combatUIManager->forgetInfoText->show = 0;
            combatUIManager->forgetPicker->show = 0;
            break;
          } else {
            //combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " will not learn " + spiritTable[combatUIManager->moveToLearn].name + ".";
            combatUIManager->finalText = stringMultiInject(getLanguageData("WontLearnMoveConfirm"), {g_partyCombatants[curCombatantIndex]->name, spiritTable[combatUIManager->moveToLearn].name});
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            g_submode = submode::FORGETTEXT;
            combatUIManager->forgetPanel->show = 0;
            combatUIManager->forgetInfoPanel->show = 0;
            combatUIManager->forgetText->show = 0;
            combatUIManager->forgetInfoText->show = 0;
            combatUIManager->forgetPicker->show = 0;
            break;

          }
          break;
        }

        break;
      }
    case submode::FORGETTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if((input[11] && !oldinput[11]) || 1) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              g_submode = submode::FORGETCONFIRM;
              combatUIManager->confirmOption = 0;
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::FORGETCONFIRM:
      {
        combatUIManager->dialogProceedIndicator->show = 0;

        if(input[2] && !oldinput[2]) {
          combatUIManager->confirmOption = 0;
        }
        if(input[3] && !oldinput[3]) {
          combatUIManager->confirmOption = 1;
        }

        if(input[11] && !oldinput[11]) {
          if(combatUIManager->confirmOption == 0) {
            g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->forgetOption] = combatUIManager->moveToLearn;
            combatUIManager->thisLevel++;
            g_submode = submode::LEVELUP;
          } else {
            //combatUIManager->mainText->updateText("Choose a move for " + g_partyCombatants[curCombatantIndex]->name + " to do without.", -1, 0.85, g_textcolor, g_font);
            combatUIManager->mainText->updateText(stringMultiInject(getLanguageData("LearnedMovePossible2"), {g_partyCombatants[curCombatantIndex]->name }), -1, 0.85, g_textcolor, g_font);

            g_submode = submode::FORGET;
            combatUIManager->forgetPanel->show = 1;
            combatUIManager->forgetInfoPanel->show = 1;
            combatUIManager->forgetText->show = 1;
            combatUIManager->forgetInfoText->show = 1;
            combatUIManager->forgetOption = 0;
          }
        }

        break;
      }
    case submode::ITEMCHOOSE: 
      {

        if(g_partyCombatants[curCombatantIndex]->itemIndexToUse != -1) {
          if(g_items.size() < g_maxInventorySize) {
            itemData td;
            td.type = g_partyCombatants[curCombatantIndex]->itemTypeToUse;
            td.index = g_partyCombatants[curCombatantIndex]->itemIndexToUse;
            g_items.push_back(td);
          }
          g_partyCombatants[curCombatantIndex]->itemIndexToUse = -1;
        }
        combatUIManager->mainPanel->show = 0;
        combatUIManager->dialogProceedIndicator->show = 0;
        combatUIManager->mainText->show = 0;

        combatUIManager->optionsPanel->show = 1;
        combatUIManager->menuPicker->show = 1;
        combatUIManager->optionsText->show = 1;

        combatUIManager->targetPanel->show = 0;
        combatUIManager->targetText->show = 0;

        combatUIManager->inventoryPanel->show = 1;
        combatUIManager->inventoryText->show = 1;

        drawOptionsPanel();

        combatUIManager->inventoryPanel->render(renderer, g_camera, elapsed);

        if(input[0] && !oldinput[0]) {
          if(combatUIManager->currentInventoryOption != 0 &&
              combatUIManager->currentInventoryOption != 7) {
            combatUIManager->currentInventoryOption --;
          }
        }

        if(input[1] && !oldinput[1]) {
          if(combatUIManager->currentInventoryOption != 6 &&
              combatUIManager->currentInventoryOption != 13) {
            if(combatUIManager->currentInventoryOption + 1 < g_items.size()) {
              combatUIManager->currentInventoryOption ++;
            }
          }
        }

        if(input[2] && !oldinput[2]) {
          if(combatUIManager->currentInventoryOption >= 7) {
            combatUIManager->currentInventoryOption -= 7;
          }
        }

        if(input[3] && !oldinput[3]) {
          if(combatUIManager->currentInventoryOption <= 6) {
            if(combatUIManager->currentInventoryOption + 7 < g_items.size()) {
              combatUIManager->currentInventoryOption += 7;
            }
          }
        }


        if(input[8] && !oldinput[8]) {
          g_submode = submode::MAIN;
          //combatUIManager->currentOption = 0;
          combatUIManager->inventoryPanel->show = 0;
          combatUIManager->inventoryText->show = 0;
        }

        if(input[11] && !oldinput[11] && g_items.size() > 0) {
          g_partyCombatants[curCombatantIndex]->itemTypeToUse = g_items[combatUIManager->currentInventoryOption].type;
          g_partyCombatants[curCombatantIndex]->itemIndexToUse = g_items[combatUIManager->currentInventoryOption].index;
          itemData td = g_items[combatUIManager->currentInventoryOption];
          switch(itemsTable[td.type][td.index].targeting) {
            case 0:
              //enemy
              g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ITEM;
              g_partyCombatants[curCombatantIndex]->serial.actionIndex = g_items[combatUIManager->currentInventoryOption].type;
              g_partyCombatants[curCombatantIndex]->serial.secondActionIndex = g_items[combatUIManager->currentInventoryOption].index;

              g_submode = submode::TARGETING;
              combatUIManager->currentTarget = 0;
              break;
            case 1:
              //teamate
              g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ITEM;
              g_partyCombatants[curCombatantIndex]->serial.actionIndex = g_items[combatUIManager->currentInventoryOption].type;
              g_partyCombatants[curCombatantIndex]->serial.secondActionIndex = g_items[combatUIManager->currentInventoryOption].index;
              g_submode = submode::ALLYTARGETING;
              combatUIManager->currentTarget = 0;
              break;
            case 2:
              //none
              g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ITEM;
              g_partyCombatants[curCombatantIndex]->serial.actionIndex = g_items[combatUIManager->currentInventoryOption].type;
              g_partyCombatants[curCombatantIndex]->serial.secondActionIndex = g_items[combatUIManager->currentInventoryOption].index;
              g_submode = submode::CONTINUE;
              break;
            case 3:
              //to be used on allies, untargeted, e.g., picnicbox
              //must be usable in overworld
              g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ITEM;
              g_partyCombatants[curCombatantIndex]->serial.actionIndex = g_items[combatUIManager->currentInventoryOption].type;
              g_partyCombatants[curCombatantIndex]->serial.secondActionIndex = g_items[combatUIManager->currentInventoryOption].index;
              g_submode = submode::CONTINUE;
              break;
            case 4:
              //ally targeted
              g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ITEM;
              g_partyCombatants[curCombatantIndex]->serial.actionIndex = g_items[combatUIManager->currentInventoryOption].type;
              g_partyCombatants[curCombatantIndex]->serial.secondActionIndex = g_items[combatUIManager->currentInventoryOption].index;
              g_submode = submode::ALLYTARGETING;
              combatUIManager->currentTarget = 0;
              break;
          }
          g_items.erase(g_items.begin() + combatUIManager->currentInventoryOption);

        }

        renderInventoryPanel();

        break;
      }
    case submode::ALLYTARGETING: 
      {
        combatUIManager->mainPanel->show = 0;
        combatUIManager->dialogProceedIndicator->show = 0;
        combatUIManager->mainText->show = 0;

        combatUIManager->optionsPanel->show = 1;
        combatUIManager->menuPicker->show = 1;
        combatUIManager->optionsText->show = 1;

        combatUIManager->targetPanel->show = 1;
        combatUIManager->targetText->show = 1;

        combatUIManager->tcm_accumulator += 0.1;
        if(combatUIManager->tcm_accumulator > M_PI * 2) {
          combatUIManager->tcm_accumulator -= M_PI * 2;
        }

        if(input[2] && !oldinput[2]) {
          if(combatUIManager->currentTarget > 0) {
            combatUIManager->currentTarget --;
          }
        }

        if(input[3] && !oldinput[3]) {
          if(combatUIManager->currentTarget < g_partyCombatants.size() - 1) {
            combatUIManager->currentTarget ++;
          }
        }

        if(combatUIManager->currentTarget < 0) { combatUIManager->currentTarget = 0; }
        if(combatUIManager->currentTarget >= g_partyCombatants.size()) { combatUIManager->currentTarget = g_partyCombatants.size() - 1; }


        if(input[11] && !oldinput[11]) {
          g_partyCombatants[curCombatantIndex]->serial.target = combatUIManager->currentTarget;
          g_submode = submode::CONTINUE;
        }

        if(input[8] && !oldinput[8]) {
          g_submode = submode::MAIN;
          combatUIManager->currentOption = 0;
        }


        drawOptionsPanel();

        combatUIManager->targetText->updateText("To " + g_partyCombatants.at(combatUIManager->currentTarget)->name, -1, 34);

        combatUIManager->targetPanel->render(renderer, g_camera, elapsed);
        combatUIManager->targetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);



        break;
      }
    case submode::SPIRITCHOOSE:
      {
        combatUIManager->mainPanel->show = 0;
        combatUIManager->dialogProceedIndicator->show = 0;
        combatUIManager->mainText->show = 0;

        combatUIManager->optionsPanel->show = 1;
        combatUIManager->menuPicker->show = 1;
        combatUIManager->optionsText->show = 1;

        combatUIManager->targetPanel->show = 0;
        combatUIManager->targetText->show = 0;

        combatUIManager->spiritPanel->show = 1;
        combatUIManager->spiritText->show = 1;

        drawOptionsPanel();


        if(input[0] && !oldinput[0]) {
          if(combatUIManager->currentInventoryOption > 0) {
            combatUIManager->currentInventoryOption --;
          }
        }

        if(input[1] && !oldinput[1]) {
          if(combatUIManager->currentInventoryOption + 1 < g_partyCombatants[curCombatantIndex]->spiritMoves.size()) {
            combatUIManager->currentInventoryOption ++;
          }
        }

        if(input[8] && !oldinput[8]) {
          g_submode = submode::MAIN;
          //combatUIManager->currentOption = 0;
          combatUIManager->spiritPanel->show = 0;
          combatUIManager->spiritText->show = 0;
        }

        if(input[11] && !oldinput[11] && g_partyCombatants[curCombatantIndex]->spiritMoves.size() > 0) {
          //does he have enough sp?
          int cost = spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption]].cost;
          int currentSp = g_partyCombatants[curCombatantIndex]->sp;
          string name = spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption]].name;

          if(currentSp < cost) {
            //combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " doesn't have enough SP for " + name + ".";
            combatUIManager->finalText = stringMultiInject(getLanguageData("CombatSPWarning"), {g_partyCombatants[curCombatantIndex]->name, name});
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            combatUIManager->dialogProceedIndicator->y = 0.25;
            g_submode = submode::SPWARNING;

          } else {
            switch(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption]].targeting) {
              case 0:
                {
                  //enemy
                  g_partyCombatants[curCombatantIndex]->serial.action = turnAction::SPIRITMOVE;
                  int spiritNumber = g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption];
                  g_partyCombatants[curCombatantIndex]->serial.actionIndex = spiritNumber;

                  g_submode = submode::TARGETING;
                  combatUIManager->currentTarget = 0;
                  break;
                }
              case 1:
                {
                  //teamate
                  g_partyCombatants[curCombatantIndex]->serial.action = turnAction::SPIRITMOVE;
                  int spiritMove = g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption];
                  int spiritNumber = spiritMove;
                  g_partyCombatants[curCombatantIndex]->serial.actionIndex = spiritNumber;

                  g_submode = submode::ALLYTARGETING;
                  break;
                }
              case 2:
                {
                  //none
                  g_partyCombatants[curCombatantIndex]->serial.action = turnAction::SPIRITMOVE;
                  g_partyCombatants[curCombatantIndex]->serial.target = -1;
                  int spiritNumber = g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption];
                  g_partyCombatants[curCombatantIndex]->serial.actionIndex = spiritNumber;
                  g_submode = submode::CONTINUE;
                  break;
                }
              case 3:
                {
                  //teamate
                  g_partyCombatants[curCombatantIndex]->serial.action = turnAction::SPIRITMOVE;
                  int spiritMove = g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption];
                  int spiritNumber = spiritMove;
                  g_partyCombatants[curCombatantIndex]->serial.actionIndex = spiritNumber;

                  g_submode = submode::ALLYTARGETING;
                  break;
                }
            }
          }

        }

        renderSpiritPanel();

        break;
      }
    case submode::SPWARNING: 
      {
        //curCombatantIndex = 0;
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              g_submode = submode::SPIRITCHOOSE;
              combatUIManager->currentOption = 0;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::DODGING:
      {
        combatUIManager->dialogProceedIndicator->show = 0;
        float rate = 2;
        if(combatUIManager->dodgePanel->x > combatUIManager->dodgePanelFullX) {
          combatUIManager->dodgePanel->x -= 0.01 * rate;
        }
        if(combatUIManager->dodgePanel->x <= combatUIManager->dodgePanelFullX) {
          combatUIManager->dodgePanel->x = combatUIManager->dodgePanelFullX;
          combatUIManager->incrementDodgeTimer = 1;
        }

        if(combatUIManager->dodgePanel->y > combatUIManager->dodgePanelFullY) {
          combatUIManager->dodgePanel->y -= 0.01 * rate * (16.0f / 10.0f);;
        }
        if(combatUIManager->dodgePanel->y < combatUIManager->dodgePanelFullY) {
          combatUIManager->dodgePanel->y = combatUIManager->dodgePanelFullY;
        }

        if(combatUIManager->dodgePanel->width < combatUIManager->dodgePanelFullWidth) {
          combatUIManager->dodgePanel->width += 0.02 * rate;
        }
        if(combatUIManager->dodgePanel->width > combatUIManager->dodgePanelFullWidth) {
          combatUIManager->dodgePanel->width = combatUIManager->dodgePanelFullWidth;
        }

        if(combatUIManager->dodgePanel->height < combatUIManager->dodgePanelFullHeight) {
          combatUIManager->dodgePanel->height += 0.02 * rate * (16.0f / 10.0f);
        }
        if(combatUIManager->dodgePanel->height > combatUIManager->dodgePanelFullHeight) {
          combatUIManager->dodgePanel->height = combatUIManager->dodgePanelFullHeight;
        }

        if(combatUIManager->incrementDodgeTimer) {
          combatUIManager->dodgeTimer += elapsed;
          bool movingUp = input[0];
          bool movingDown = input[1];
          bool movingLeft = input[2];
          bool movingRight = input[3];

          // Normalize speed for diagonal movement
          float speed = combatUIManager->dodgerSpeed;

          if (movingUp) {
            if(movingLeft) {
              combatUIManager->dodgerY -= speed * 0.707106;
              combatUIManager->dodgerX -= speed * 0.707106;
            } else if(movingRight) {
              combatUIManager->dodgerY -= speed * 0.707106;
              combatUIManager->dodgerX += speed * 0.707106;
            } else {
              combatUIManager->dodgerY -= speed;
            }
          } else if(movingDown) {
            if(movingLeft) {
              combatUIManager->dodgerY += speed * 0.707106;
              combatUIManager->dodgerX -= speed * 0.707106;
            } else if(movingRight) {
              combatUIManager->dodgerY += speed * 0.707106;
              combatUIManager->dodgerX += speed * 0.707106;
            } else {
              combatUIManager->dodgerY += speed;
            }
          } else if(movingLeft) {
            combatUIManager->dodgerX -= speed;
          } else if(movingRight) {
            combatUIManager->dodgerX += speed;
          }

          float margin = combatUIManager->dodgerWidth/2;
          if(combatUIManager->dodgerX < 0 + margin) {
            combatUIManager->dodgerX = 0 + margin;
          }
          if(combatUIManager->dodgerY < 0 + margin) {
            combatUIManager->dodgerY = margin;
          }
          if(combatUIManager->dodgerX > 1024 - margin) {
            combatUIManager->dodgerX = 1024 - margin;
          }
          if(combatUIManager->dodgerY > 1024 - margin) {
            combatUIManager->dodgerY = 1024 - margin;
          }
        }
        if(combatUIManager->partyDodgingCombatant->health <= 0 || (devMode && input[8])) {
          //end early
          combatUIManager->dodgeTimer = combatUIManager->maxDodgeTimer + 1;
        }


        if(combatUIManager->dodgeTimer > combatUIManager->maxDodgeTimer) {
          // delete all miniEnts
          int size = g_miniEnts.size();
          for(int i = 0; i < size; i++) {
            delete g_miniEnts[0];
          }

          if(combatUIManager->executeEIndex + 1 ==  g_enemyCombatants.size()) {

            int deadPartyMembers = 0;
            for(auto x : g_partyCombatants) {
              if(x->health <= 0) {
                deadPartyMembers++;
              }
            }

            if(combatUIManager->partyDodgingCombatant->health <= 0) {
              combatUIManager->finalText = combatUIManager->partyDodgingCombatant->name + " passed out!";
              combatUIManager->currentText = "";
              combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            }


            if(deadPartyMembers == g_partyCombatants.size()) {
              //string message = "All party members are knocked-out!";
              string message = getLanguageData("CombatPartyDead");
              combatUIManager->queuedStrings.clear();
              combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
              g_submode = submode::ALLDEADTEXT;
              break;

            }

            if(combatUIManager->partyDodgingCombatant->health <= 0) {
              g_submode = submode::MEMBERDEADTEXT;
              break;
            }

            combatUIManager->mainPanel->show = 0;
            combatUIManager->mainText->show = 0;
            combatUIManager->dialogProceedIndicator->show = 0;
            curCombatantIndex = 0;
            curStatusIndex = 0;

            //reset all stats
            for(auto x : g_enemyCombatants) {
              x->curStrength = x->baseStrength;
              x->curMind = x->baseMind;
              x->curAttack = x->baseAttack;
              x->curDefense = x->baseDefense;
              x->curSoul = x->baseSoul;
              x->curSkill = x->baseSkill;
              x->curCritical = x->baseCritical;
              x->curRecovery = x->baseSoul;
              x->physicalDefense = 0;
              x->spiritDefense = 0;
            }

            g_submode = submode::STATUS_E;

          } else {
            //if the character died, report on it with MEMBERDEADTEXT. If all characters are dead, report on it with ALLDEADTEXT

            int deadPartyMembers = 0;
            for(auto x : g_partyCombatants) {
              if(x->health <= 0) {
                deadPartyMembers++;
              }
            }

            if(combatUIManager->partyDodgingCombatant->health <= 0) {

              combatUIManager->finalText = combatUIManager->partyDodgingCombatant->name + " passed out!";
              combatUIManager->currentText = "";
              combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            }

            if(deadPartyMembers == g_partyCombatants.size()) {
              //string message = "All party members are knocked-out!";
              string message = getLanguageData("CombatPartyDead");
              combatUIManager->queuedStrings.clear();
              combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
              g_submode = submode::ALLDEADTEXT;
              break;

            }

            if(combatUIManager->partyDodgingCombatant->health <= 0) {
              g_submode = submode::MEMBERDEADTEXT;
              break;
            }


            if(combatUIManager->executeEIndex + 1 ==  g_enemyCombatants.size()) {


            } else {
              combatUIManager->executeEIndex++;
              g_submode = submode::EXECUTE_E;
            }
          }

        }
        break;
      }
    case submode::TEXT_IDLE:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;

              combatant* whoDied = combatUIManager->queuedStrings.at(0).second;
              if(whoDied != 0) {
                int index = 0;
                while(index < g_deadCombatants.size()) {
                  if(g_deadCombatants.at(index) == whoDied) {
                    whoDied->disappearing = 1;
                    break;
                  }
                  index++;
                }
              }

              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
//              combatUIManager->mainPanel->show = 0;
//              combatUIManager->mainText->show = 0;
//              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              //curStatusIndex++;

//              combatUIManager->executeEIndex++;
//              M("IDLETEXT TO EXECUTE_E");
//              g_submode = submode::EXECUTE_E;


              //this is used when the last enemy dies from a steeltrap
              if(g_enemyCombatants.size() == 0) {
                g_submode = submode::FINAL;
                break;
              }

              if(combatUIManager->executeEIndex + 1 ==  g_enemyCombatants.size()) {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              curCombatantIndex = 0;
              combatUIManager->executeEIndex = 0;
              g_submode = submode::STATUS_E;
              curStatusIndex = 0;

  
  
              } else {
                combatUIManager->executeEIndex++;
                g_submode = submode::EXECUTE_E;
              }
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::TEXT_ENEMY_BLINDED:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
//              combatUIManager->mainPanel->show = 0;
//              combatUIManager->mainText->show = 0;
//              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              //curStatusIndex++;

//              combatUIManager->executeEIndex++;
//              M("IDLETEXT TO EXECUTE_E");
//              g_submode = submode::EXECUTE_E;

              if(combatUIManager->executeEIndex + 1 ==  g_enemyCombatants.size()) {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              curCombatantIndex = 0;
              combatUIManager->executeEIndex = 0;
              g_submode = submode::STATUS_E;
              curStatusIndex = 0;

  
  
              } else {
                combatUIManager->executeEIndex++;
                g_submode = submode::EXECUTE_E;
              }
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::STATUS_P:
      {
        int breakout = 0;
        while(curStatusIndex >= g_partyCombatants[curCombatantIndex]->statuses.size()) {
          curStatusIndex = 0;
          curCombatantIndex++;
          if(curCombatantIndex == g_partyCombatants.size()) {
            curCombatantIndex = 0;
//            for(int i = 0; i < 4; i ++) {
//              combatUIManager->dodgingThisTurn[i] = 0;
//            }
            while(g_partyCombatants[curCombatantIndex]->health <= 0 && curCombatantIndex+1 < g_partyCombatants.size()) {
              curCombatantIndex ++;
            }
            g_submode = submode::EXECUTE_E;
            combatUIManager->executeEIndex = 0;
            breakout = 1;
            break;
          }
        }
        if(breakout) {break;}

        combatant* c = g_partyCombatants[curCombatantIndex];
        if(applyStatus(c, &c->statuses[curStatusIndex])) {
          c->statuses.erase(c->statuses.begin() + curStatusIndex);
          curStatusIndex--;
        }
        break;
      }
    case submode::TEXT_STATUS_P:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              curStatusIndex++;

              g_submode = submode::STATUS_P;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::RUNWARNING: 
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              g_submode = submode::MAIN;
              combatUIManager->currentOption = 0;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::RUNSUCCESSTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }

        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              g_submode = submode::OUTWIPE;
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::RUNFAILTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }

        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              if(combatUIManager->executePIndex + 1 ==  g_partyCombatants.size()) {
                g_submode = submode::EXECUTE_E;
                combatUIManager->executeEIndex = 0;
              } else {
                combatUIManager->executePIndex++;
                g_submode = submode::EXECUTE_P;
              }
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::MEMBERDEADTEXT:
      {

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }

        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              if(combatUIManager->executeEIndex + 1 ==  g_enemyCombatants.size()) {
                combatUIManager->mainPanel->show = 0;
                combatUIManager->mainText->show = 0;
                combatUIManager->dialogProceedIndicator->show = 0;
                curCombatantIndex = 0;
                curStatusIndex = 0;
                //reset all stats
                for(auto x : g_enemyCombatants) {
                  x->curStrength = x->baseStrength;
                  x->curMind = x->baseMind;
                  x->curAttack = x->baseAttack;
                  x->curDefense = x->baseDefense;
                  x->curSoul = x->baseSoul;
                  x->curSkill = x->baseSkill;
                  x->curCritical = x->baseCritical;
                  x->curRecovery = x->baseSoul;
                  x->physicalDefense = 0;
                  x->spiritDefense = 0;
                }
                g_submode = submode::STATUS_E;
                //g_submode = submode::MAIN;
                break;
              } else {
                combatUIManager->executeEIndex++;
                g_submode = submode::EXECUTE_E;
                break;
              }
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::MEMBERDEADTEXT_P:
      {

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }

        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              if(combatUIManager->executePIndex + 1 ==  g_partyCombatants.size()) {
                //reset all stats
                for(auto x : g_partyCombatants) {
                  x->curStrength = x->baseStrength;
                  x->curMind = x->baseMind;
                  x->curAttack = x->baseAttack;
                  x->curDefense = x->baseDefense;
                  x->curSoul = x->baseSoul;
                  x->curSkill = x->baseSkill;
                  x->curCritical = x->baseCritical;
                  x->curRecovery = x->baseSoul;
                  x->physicalDefense = 0;
                  x->spiritDefense = 0;
                }
                g_submode = submode::STATUS_P;
              } else {
                combatUIManager->executePIndex++;
                g_submode = submode::EXECUTE_P;
              }
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::ALLDEADTEXT:
      {

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }

        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {


              //g_gamemode = gamemode::LOSS;
              g_lossSub = lossSub::INWIPE;
              transitionDelta = transitionImageHeight;
              g_submode = submode::OUTWIPEL;//dont run the code to draw the minients after the switch statement
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::DROPITEMS:
      {
        vector<string> messages; messages.clear();
        for(auto x : g_dropInfos) {
          if(frng(0,100) <= x.dropPercent && g_items.size() < g_maxInventorySize) {
            messages.push_back(stringMultiInject(getLanguageData("FoundText"), {itemsTable[x.dropType][x.dropIndex].name}));
            //messages.push_back("Fomm found a " + itemsTable[x.dropIndex].name + " which the enemy left behind.");
            itemData td;
            td.index = x.dropIndex;
            td.type = x.dropType;
            g_items.push_back(td);
          }
        }

        if(messages.size() > 0) {
          combatUIManager->currentText = "";
          combatUIManager->finalText = messages[0];
        }
        for(int i = 1; i < messages.size(); i++) {
          combatUIManager->queuedStrings.push_back(make_pair(messages.at(i), (combatant*)0));
        }

        if(g_items.size() < g_maxInventorySize) {

        }

        if(messages.size() == 0) {
          g_submode = submode::OUTWIPE;
          break;
        }
        
        g_submode = submode::DROPITEMTEXT;
        break;
      }
    case submode::DROPITEMTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {

              //change this
              g_submode = submode::OUTWIPE;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
  }

  BM("After submode handling");


  combatUIManager->mainPanel->render(renderer, g_camera, elapsed);
  combatUIManager->mainText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
  combatUIManager->dialogProceedIndicator->render(renderer, g_camera, elapsed);


  if(g_submode == submode::DODGING) {
    combatUIManager->dodgePanel->render(renderer, g_camera, elapsed);
    SDL_SetRenderTarget(renderer, combatUIManager->rendertarget);
    SDL_SetRenderDrawColor(renderer, 6, 7, 6, 255);
    SDL_RenderClear(renderer);

    combatUIManager->accuA += elapsed;
    combatUIManager->accuB += elapsed;
    combatUIManager->accuC += elapsed;

    if(combatUIManager->curPatterns.size() > 0) {
      spawnBullets(combatUIManager->curPatterns[0], combatUIManager->accuA);
    }

    if(combatUIManager->curPatterns.size() > 1) {
      spawnBullets(combatUIManager->curPatterns[1], combatUIManager->accuB);
    }

    if(combatUIManager->curPatterns.size() > 2) {
      spawnBullets(combatUIManager->curPatterns[2], combatUIManager->accuC);
    }

    {
      for(auto x : g_miniEnts) {
        x->update(elapsed);
      }
      for(int i = 0; i < g_miniBullets.size(); i++) {
        g_miniBullets[i]->bulletUpdate(elapsed);
      }
      for(int i = 0; i < g_miniEnts.size(); i++) {
        if(g_miniEnts[i]->x < -SPAWN_MARGIN || g_miniEnts[i]->x > SCREEN_WIDTH + SPAWN_MARGIN ||
            g_miniEnts[i]->y < -SPAWN_MARGIN || g_miniEnts[i]->y > SCREEN_HEIGHT + SPAWN_MARGIN) {
          delete g_miniEnts[i];
          i--;
          continue;
        }
        if(g_miniEnts[i]->exploded) {
          delete g_miniEnts[i];
          i--;
          continue;
        }
      }
      for(auto x : g_miniEnts) {
        x->render(1);
      }
      for(auto x : g_miniEnts) {
        x->render(0);
      }
      if(combatUIManager->shrink) {
        combatUIManager->dodgerWidth = 50;
        combatUIManager->dodgerHeight = 50;
      } else {
        combatUIManager->dodgerWidth = 100;
        combatUIManager->dodgerHeight = 100;
      }
      if(combatUIManager->invincibleMs <= 0) {
        for(auto x : g_miniBullets) {
          if(Distance(combatUIManager->dodgerX, combatUIManager->dodgerY, x->x, x->y) < (combatUIManager->dodgerWidth + x->w)/2) {
            combatUIManager->partyDodgingCombatant->health -= combatUIManager->damageFromEachHit;
            int dmgToReport = combatUIManager->damageFromEachHit;
            if(dmgToReport > combatUIManager->partyDodgingCombatant->health) {
              dmgToReport = combatUIManager->partyDodgingCombatant->health;
            }
            combatUIManager->partyDodgingCombatant->dmgTakenOverFight += dmgToReport;
            if(combatUIManager->partyDodgingCombatant->health < 0) {combatUIManager->partyDodgingCombatant->health = 0;}

            combatUIManager->damageTakenFromDodgingPhase += combatUIManager->damageFromEachHit;
            combatUIManager->invincibleMs = combatUIManager->maxInvincibleMs;
            break;
          }
        }
      }
      SDL_Rect drect;
      drect.x = combatUIManager->dodgerX - combatUIManager->dodgerWidth/2;
      drect.y = combatUIManager->dodgerY - combatUIManager->dodgerHeight/2;
      drect.w = combatUIManager->dodgerWidth;
      drect.h = combatUIManager->dodgerHeight;

      if(combatUIManager->invincibleMs > 0) {
        if(combatUIManager->blinkMs > 50) {
          combatUIManager->drawDodger = !combatUIManager->drawDodger;
          combatUIManager->blinkMs = 0;
        }
      } else {
        combatUIManager->drawDodger = 1;
      }

      if(combatUIManager->drawDodger) {



        SDL_SetTextureColorMod(combatUIManager->dodgerTexture, 255*0.7, 255*0.7, 255*0.7);
        //SDL_RenderCopy(renderer, combatUIManager->dodgerTexture, NULL, &drect);
        SDL_Point center = {drect.w/2,drect.h/2};
        SDL_RenderCopyEx(renderer, combatUIManager->dodgerTexture, NULL, &drect, 0, &center, SDL_FLIP_NONE);
        //combatUIManager->dodgerAngle += combatUIManager->dodgerAngleDelta * (double)elapsed / 16.0;

            
        SDL_SetTextureColorMod(combatUIManager->dodgerTexture, 255, 255, 255);
        drect.x += 8;
        drect.y += 8;
        drect.w -= 16;
        drect.h -= 16;
        center = {drect.w/2,drect.h/2};
        //SDL_RenderCopy(renderer, combatUIManager->dodgerTexture, NULL, &drect);
        SDL_RenderCopyEx(renderer, combatUIManager->dodgerTexture, NULL, &drect, combatUIManager->dodgerAngle, &center, SDL_FLIP_NONE);
      }


      combatUIManager->invincibleMs -= elapsed;
      combatUIManager->blinkMs += elapsed;
    }







    SDL_SetRenderTarget(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_Rect dstrect;
    float padding = 0.04;
    dstrect.x = (combatUIManager->dodgePanel->x + padding/2) * WIN_WIDTH;
    dstrect.y = (combatUIManager->dodgePanel->y + (padding*combatUIManager->aspect)/2) * WIN_HEIGHT;
    dstrect.w = (combatUIManager->dodgePanel->width - padding) * WIN_WIDTH;
    dstrect.h = (combatUIManager->dodgePanel->height - (padding*combatUIManager->aspect))* WIN_HEIGHT;
    SDL_RenderCopy(renderer, combatUIManager->rendertarget, NULL, &dstrect);
  }

  if(g_submode== submode::FORGET) {
    combatUIManager->dialogProceedIndicator->show = 0;
    combatUIManager->forgetPanel->render(renderer, g_camera, elapsed);
    float y = 0.275;
    float yDelta = 0.075;
    float x = 0.25;

    float px = 0.23;
    float pxoffset = -0.009; //slightly negative
    float pyoffset = 0.006;

    combatUIManager->forgetText->updateText(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[0]].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    combatUIManager->forgetPicker->show = 1;

    if(combatUIManager->forgetOption == 0) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    y+= yDelta;

    combatUIManager->forgetText->updateText(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[1]].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);

    if(combatUIManager->forgetOption == 1) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    y+= yDelta;

    combatUIManager->forgetText->updateText(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[2]].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    if(combatUIManager->forgetOption == 2) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    y+= yDelta;

    combatUIManager->forgetText->updateText(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[3]].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    if(combatUIManager->forgetOption == 3) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    y+= yDelta;

    combatUIManager->forgetText->updateText(spiritTable[combatUIManager->moveToLearn].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    if(combatUIManager->forgetOption == 4) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    combatUIManager->forgetInfoPanel->render(renderer, g_camera, elapsed);
    
    if(combatUIManager->forgetOption <= 3) {
      string info = getLanguageData("SI" + to_string(g_partyCombatants[g_whoLearnsMove]->spiritMoves[combatUIManager->forgetOption]));
      while(replaceString(info, "\\n", "\n")){}
      string name = getLanguageData("S" + to_string(g_partyCombatants[g_whoLearnsMove]->spiritMoves[combatUIManager->forgetOption]));
      string final = name + "\n" + info;

      combatUIManager->forgetInfoText->updateText(final, -1, 0.43, g_textcolor, g_font);
    } else {
      string info = getLanguageData("SI" + to_string(combatUIManager->moveToLearn));
      while(replaceString(info, "\\n", "\n")){}
      string name = getLanguageData("S" + to_string(combatUIManager->moveToLearn));
      string final = name + "\n" + info;

      combatUIManager->forgetInfoText->updateText(final, -1, 0.43, g_textcolor, g_font );
    }
    combatUIManager->forgetInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);



  }

  if(g_submode == submode::FORGETCONFIRM) {
    combatUIManager->yes->show = 1;
    combatUIManager->yes->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    combatUIManager->no->show = 1;
    combatUIManager->no->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    combatUIManager->confirmPicker->show = 1;

    if(combatUIManager->confirmOption == 0) {
      combatUIManager->confirmPicker->x = combatUIManager->yes->boxX - 0.08;
      combatUIManager->confirmPicker->y = combatUIManager->yes->boxY + 0.01;
    } else {
      combatUIManager->confirmPicker->x = combatUIManager->no->boxX - 0.07;
      combatUIManager->confirmPicker->y = combatUIManager->no->boxY + 0.01;

    }
    combatUIManager->confirmPicker->render(renderer, g_camera, elapsed);

  }



  SDL_RenderPresent(renderer);
  BM("After present");
}

//this is for gaining xp out of combat (including learning/forgetting spiritmoves
// gainxp()
void explorationLevelupLoop() {
  getCombatInput();

  //SDL_RenderClear(renderer);

  //updateWindowResolution();

//  if(combatUIManager->loadedBackground.scene[0] == '>') {
//    drawBackground();
//  } else {
//    drawSimpleBackground();
//  }

  //drawCombatants();

  //SDL_RenderCopy(renderer, g_shade, NULL, NULL);

  switch (g_submode) {
    case submode::BEFORE:
      {
        for(int i = 0; i < 4; i ++) {
          combatUIManager->dodgingThisTurn[i] = 0;
        }

        for(auto &x : g_partyCombatants) {
          x->dmgDealtOverFight = 0;
          x->dmgTakenOverFight = 0;
        }

        g_autoFight = 0;

        //clear statuses
        for(auto x : g_partyCombatants) {
          x->statuses.clear();
          x->curStrength = x->baseStrength;
          x->curMind = x->baseMind;
          x->curAttack = x->baseAttack;
          x->curDefense = x->baseDefense;
          x->curSoul = x->baseSoul;
          x->curSkill = x->baseSkill;
          x->curCritical = x->baseCritical;
          x->curRecovery = x->baseSoul;
          x->physicalDefense = 0;
          x->spiritDefense = 0;
        }

        g_forceEndDialogue = 0;
        g_submode = submode::INWIPE;
      }
    case submode::INWIPE:
      {

         for(int i = 0; i < g_partyCombatants.size(); i++) {
//           combatUIManager->pHpRValues[i] = -1;
//           combatUIManager->pMhpRValues[i] = -1;
//           combatUIManager->pSpRValues[i] = -1;
//           combatUIManager->pMspRValues[i] = -1;
  
           combatUIManager->partyNameTextboxes[i]->show = 1;
           //combatUIManager->partyHealthBoxes[i]->show = 1;
           //combatUIManager->partyHealthDescripterTextboxes[i]->show = 1;
           //combatUIManager->partyManaDescripterTextboxes[i]->show = 1;
           //combatUIManager->partyHealthTextboxes[i]->show = 1;
           //combatUIManager->partyManaTextboxes[i]->show = 1;
         }
  
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
          g_submode = submode::TEXT;
        }
        break;
      }
    case submode::OUTWIPE:
      {
        resetTrivialData();
        //M("OUTWIPE");

        for(auto x : g_partyCombatants) {
          writeSaveField(x->filename + "-dealt", x->dmgDealtOverFight);
          writeSaveField(x->filename + "-taken", x->dmgTakenOverFight);
        }

        {
          //SDL_GL_SetSwapInterval(0);
          bool cont = false;
          float ticks = 0;
          float lastticks = 0;
          float transitionElapsed = 5;
          float mframes = 60;
          float transitionMinFrametime = 5;
          transitionMinFrametime = 1/mframes * 1000;


          SDL_Surface* transitionSurface = loadSurface("resources/engine/transition.qoi");

          int imageWidth = transitionSurface->w;
          int imageHeight = transitionSurface->h;

          SDL_Texture* transitionTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, transitionSurface->w, transitionSurface->h );
          SDL_SetTextureBlendMode(transitionTexture, SDL_BLENDMODE_BLEND);


          void* pixelReference;
          int pitch;

          float offset = imageHeight;

          SDL_Texture* frame = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, WIN_WIDTH, WIN_HEIGHT);
          SDL_SetRenderTarget(renderer, frame);

          if(combatUIManager->loadedBackground.scene[0] == '>') {
            drawBackground();
          } else {
            drawSimpleBackground();
          }
          drawCombatants();

          //SDL_RenderCopy(renderer, g_shade, NULL, NULL);

          SDL_SetRenderTarget(renderer, NULL);
          SDL_RenderClear(renderer);

          while (!cont) {

            //onframe things
            SDL_LockTexture(transitionTexture, NULL, &pixelReference, &pitch);

            memcpy( pixelReference, transitionSurface->pixels, transitionSurface->pitch * transitionSurface->h);
            Uint32 format = SDL_PIXELFORMAT_ARGB8888;
            SDL_PixelFormat* mappingFormat = SDL_AllocFormat( format );
            Uint32* pixels = (Uint32*)pixelReference;
            Uint32 transparent = SDL_MapRGBA( mappingFormat, 0, 0, 0, 255);

            offset += g_transitionSpeed + 0.02 * offset;

            for(int x = 0;  x < imageWidth; x++) {
              for(int y = 0; y < imageHeight; y++) {


                int dest = (y * imageWidth) + x;
                //int src =  (y * imageWidth) + x;

                if(pow(pow(imageWidth/2 - x,2) + pow(imageHeight + y,2),0.5) < offset) {
                  pixels[dest] = transparent;
                } else {
                  pixels[dest] = 0;
                }

              }
            }





            ticks = SDL_GetTicks();
            transitionElapsed = ticks - lastticks;
            //lock framerate
            if(transitionElapsed < transitionMinFrametime) {
              SDL_Delay(transitionMinFrametime - transitionElapsed);
              ticks = SDL_GetTicks();
              transitionElapsed = ticks - lastticks;
            }
            lastticks = ticks;

            SDL_RenderClear(renderer);
            //render last frame
            //SDL_RenderCopy(renderer, frame, NULL, NULL);
            if(combatUIManager->loadedBackground.scene[0] == '>') {
              drawBackground();
            } else {
              drawSimpleBackground();
            }

            drawCombatants();

            //SDL_RenderCopy(renderer, g_shade, NULL, NULL);

            SDL_UnlockTexture(transitionTexture);
            SDL_RenderCopy(renderer, transitionTexture, NULL, NULL);
            SDL_RenderPresent(renderer);

            if(offset > imageHeight + pow(pow(imageWidth/2,2) + pow(imageHeight,2),0.5)) {
              cont = 1;
            }
          }
          SDL_FreeSurface(transitionSurface);
          SDL_DestroyTexture(transitionTexture);
          SDL_DestroyTexture(frame);
          SDL_GL_SetSwapInterval(1);
        }

        for(auto x : g_combatWorldEnts) {
          x->opacity_delta = -3;
          x->semisolid = 0;
          x->agrod = 0;
        }
        g_combatWorldEnts.clear();
        g_gamemode = gamemode::EXPLORATION;
        protag_can_move = 1;
        protag->dynamic = 1;
        transition = 1;
        transitionDelta = transitionImageHeight;
        combatUIManager->hideAll();
        if(g_combatEntryType == 0) {
          //continue script
          adventureUIManager->dialogue_index++;
          adventureUIManager->continueDialogue();
        } else {
          //wasn't a scripted fight
        }

        //if the player ran away, we should delete these
        for(int i = 0; i < g_enemyCombatants.size(); i++) {
          delete g_enemyCombatants[i];
        }
        g_enemyCombatants.clear();
        for(int i = 0; i < g_deadCombatants.size(); i++) {
          delete g_deadCombatants[i];
        }
        g_deadCombatants.clear();

        writeSave();

        break;
      }
    case submode::OUTWIPEL:
      {
        {
          //SDL_GL_SetSwapInterval(0);
          bool cont = false;
          float ticks = 0;
          float lastticks = 0;
          float transitionElapsed = 5;
          float mframes = 60;
          float transitionMinFrametime = 5;
          transitionMinFrametime = 1/mframes * 1000;


          SDL_Surface* transitionSurface = loadSurface("resources/engine/transition.qoi");

          int imageWidth = transitionSurface->w;
          int imageHeight = transitionSurface->h;

          SDL_Texture* transitionTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, transitionSurface->w, transitionSurface->h );
          SDL_SetTextureBlendMode(transitionTexture, SDL_BLENDMODE_BLEND);


          void* pixelReference;
          int pitch;

          float offset = imageHeight;

          SDL_Texture* frame = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, WIN_WIDTH, WIN_HEIGHT);
          SDL_SetRenderTarget(renderer, frame);

          if(combatUIManager->loadedBackground.scene[0] == '>') {
            drawBackground();
          } else {
            drawSimpleBackground();
          }
          drawCombatants();

          //SDL_RenderCopy(renderer, g_shade, NULL, NULL);

          SDL_SetRenderTarget(renderer, NULL);
          SDL_RenderClear(renderer);

          while (!cont) {

            //onframe things
            SDL_LockTexture(transitionTexture, NULL, &pixelReference, &pitch);

            memcpy( pixelReference, transitionSurface->pixels, transitionSurface->pitch * transitionSurface->h);
            Uint32 format = SDL_PIXELFORMAT_ARGB8888;
            SDL_PixelFormat* mappingFormat = SDL_AllocFormat( format );
            Uint32* pixels = (Uint32*)pixelReference;
            Uint32 transparent = SDL_MapRGBA( mappingFormat, 0, 0, 0, 255);

            offset += g_transitionSpeed + 0.02 * offset;

            for(int x = 0;  x < imageWidth; x++) {
              for(int y = 0; y < imageHeight; y++) {


                int dest = (y * imageWidth) + x;
                //int src =  (y * imageWidth) + x;

                if(pow(pow(imageWidth/2 - x,2) + pow(imageHeight + y,2),0.5) < offset) {
                  pixels[dest] = transparent;
                } else {
                  pixels[dest] = 0;
                }

              }
            }





            ticks = SDL_GetTicks();
            transitionElapsed = ticks - lastticks;
            //lock framerate
            if(transitionElapsed < transitionMinFrametime) {
              SDL_Delay(transitionMinFrametime - transitionElapsed);
              ticks = SDL_GetTicks();
              transitionElapsed = ticks - lastticks;
            }
            lastticks = ticks;

            SDL_RenderClear(renderer);
            //render last frame
            //SDL_RenderCopy(renderer, frame, NULL, NULL);
            if(combatUIManager->loadedBackground.scene[0] == '>') {
              drawBackground();
            } else {
              drawSimpleBackground();
            }

            drawCombatants();

            //SDL_RenderCopy(renderer, g_shade, NULL, NULL);

            SDL_UnlockTexture(transitionTexture);
            SDL_RenderCopy(renderer, transitionTexture, NULL, NULL);
            SDL_RenderPresent(renderer);

            if(offset > imageHeight + pow(pow(imageWidth/2,2) + pow(imageHeight,2),0.5)) {
              cont = 1;
            }
          }
          SDL_FreeSurface(transitionSurface);
          SDL_DestroyTexture(transitionTexture);
          SDL_DestroyTexture(frame);
          SDL_GL_SetSwapInterval(1);
        }

        adventureUIManager->executingScript = 0;

        adventureUIManager->mobilize = 0;
        adventureUIManager->hideTalkingUI();
        protag_is_talking = 2;

        g_gamemode = gamemode::LOSS;
        g_lossSub = lossSub::INWIPE;
        transitionDelta = transitionImageHeight;
        //lossUIManager->redness = 255;
        transition = 1;
        transitionDelta = transitionImageHeight;
        combatUIManager->hideAll();

        //if the player ran away, we should delete these
        for(int i = 0; i < g_enemyCombatants.size(); i++) {
          delete g_enemyCombatants[i];
        }
        g_enemyCombatants.clear();
        for(int i = 0; i < g_deadCombatants.size(); i++) {
          delete g_deadCombatants[i];
        }
        g_deadCombatants.clear();

        break;
      }
    case submode::TEXT:
      {
        curCombatantIndex = 0;
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              g_submode = submode::MAIN;
              combatUIManager->currentOption = 0;
              while(g_partyCombatants[curCombatantIndex]->health <= 0 && curCombatantIndex+1 < g_partyCombatants.size()) {
                curCombatantIndex ++;
              }
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::MAIN:
      {
        if(input[8]) {
          g_autoFight = 0;
        }

        if(g_autoFight) {

          while(curCombatantIndex < g_partyCombatants.size()) {
            g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ATTACK;
            g_partyCombatants[curCombatantIndex]->serial.actionIndex = 0;
            g_partyCombatants[curCombatantIndex]->serial.target = 0;
            curCombatantIndex++;

          }
          g_submode = submode::EXECUTE_P;
          combatUIManager->executePIndex = 0;
          curCombatantIndex = 0;

          break;
        }


        if(input[0] && !oldinput[0]) {
          if(combatUIManager->currentOption == 1 ||
              combatUIManager->currentOption == 3 ||
              combatUIManager->currentOption == 5) {
            combatUIManager->currentOption --;
          }
        }

        if(input[1] && !oldinput[1]) {
          if(combatUIManager->currentOption == 0 ||
              combatUIManager->currentOption == 2 ||
              combatUIManager->currentOption == 4) {
            combatUIManager->currentOption ++;
          }
        }

        if(input[2] && !oldinput[2]) {
          if(combatUIManager->currentOption == 2 ||
              combatUIManager->currentOption == 3 ||
              combatUIManager->currentOption == 4 ||
              combatUIManager->currentOption == 5) {
            combatUIManager->currentOption -= 2;
          }
        }

        if(input[3] && !oldinput[3]) {
          if(combatUIManager->currentOption == 0 ||
              combatUIManager->currentOption == 1 ||
              combatUIManager->currentOption == 2 ||
              combatUIManager->currentOption == 3) {
            combatUIManager->currentOption += 2;
          }
        }

        if(input[11] && !oldinput[11]) {
          switch(combatUIManager->currentOption) {
            case 0: 
              {
                //attack
                g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ATTACK;
                g_partyCombatants[curCombatantIndex]->serial.actionIndex = 0;

                //now, choose a target
                g_submode = submode::TARGETING;
                combatUIManager->currentTarget = 0;

                break;
              }
            case 1:
              {
                //Spirit move
                g_partyCombatants[curCombatantIndex]->serial.action = turnAction::SPIRITMOVE;

                //now, choose a move
                g_submode = submode::SPIRITCHOOSE;
                combatUIManager->currentInventoryOption = 0;

                break;
              }
            case 2:
              {
                //Bag
                g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ITEM;

                combatUIManager->currentInventoryOption = 0;

                //now, choose a target
                g_submode = submode::ITEMCHOOSE;
                combatUIManager->currentInventoryOption = 0;

                break;
              }
            case 3:
              {
                //Defend
                g_partyCombatants[curCombatantIndex]->serial.action = turnAction::DEFEND;
                g_partyCombatants[curCombatantIndex]->serial.actionIndex = 0;

                g_submode = submode::CONTINUE;
                break;
              }
            case 4:
              {
                //Run
                if(adventureUIManager->executingScript) {
                  //can't run from this fight
                  combatUIManager->finalText = getLanguageData("CombatCantRun");
                  combatUIManager->currentText = "";
                  combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
                  combatUIManager->dialogProceedIndicator->y = 0.25;
                  g_submode = submode::RUNWARNING;

                } else {
                  g_partyCombatants[curCombatantIndex]->serial.action = turnAction::FLEE;
                  g_submode = submode::CONTINUE;
                  break;



                }

                break;
              }
            case 5:
              {
                //Autofight
                g_autoFight = 1;

                while(curCombatantIndex < g_partyCombatants.size()) {
                  g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ATTACK;
                  g_partyCombatants[curCombatantIndex]->serial.actionIndex = 0;
                  g_partyCombatants[curCombatantIndex]->serial.target = 0;
                  curCombatantIndex++;

                }
                g_submode = submode::EXECUTE_P;
                combatUIManager->executePIndex = 0;
                curCombatantIndex = 0;

                break;
              }

          }

        }

        if(input[8] && !oldinput[8]) {
          if(curCombatantIndex > 0) {
            int newCombatantIndex = curCombatantIndex - 1;
            while(newCombatantIndex >= 0 && g_partyCombatants[newCombatantIndex]->health <= 0) {
              newCombatantIndex--;
            }
            if(newCombatantIndex >= 0) {
              curCombatantIndex = newCombatantIndex;
              g_submode = submode::MAIN;
              combatUIManager->currentOption = 0;
            }
          }
        }


        combatUIManager->optionsPanel->show = 1;
        combatUIManager->menuPicker->show = 1;
        combatUIManager->optionsText->show = 1;
        combatUIManager->optionsMiniText->show = 1;

        drawOptionsPanel();
        break;
      }
    case submode::TARGETING: 
      {
        combatUIManager->mainPanel->show = 0;
        combatUIManager->dialogProceedIndicator->show = 0;
        combatUIManager->mainText->show = 0;

        combatUIManager->optionsPanel->show = 1;
        combatUIManager->menuPicker->show = 1;
        combatUIManager->optionsText->show = 1;

        combatUIManager->targetPanel->show = 1;
        combatUIManager->targetText->show = 1;

        combatUIManager->tcm_accumulator += 0.1;
        if(combatUIManager->tcm_accumulator > M_PI * 2) {
          combatUIManager->tcm_accumulator -= M_PI * 2;
        }

        //combatUIManager->targetingColorMod = (sin(combatUIManager->tcm_accumulator) + 1) * 128;
        combatUIManager->targetingColorMod = 128;


        if(input[2] && !oldinput[2]) {
          if(combatUIManager->currentTarget > 0) {
            combatUIManager->currentTarget --;
          }
        }

        if(input[3] && !oldinput[3]) {
          if(combatUIManager->currentTarget < g_enemyCombatants.size() - 1) {
            combatUIManager->currentTarget ++;
          }
        }

        if(combatUIManager->currentTarget < 0) { combatUIManager->currentTarget = 0; }
        if(combatUIManager->currentTarget >= g_enemyCombatants.size()) { combatUIManager->currentTarget = g_enemyCombatants.size() - 1; }


        if(input[11] && !oldinput[11]) {
          g_partyCombatants[curCombatantIndex]->serial.target = combatUIManager->currentTarget;
          g_submode = submode::CONTINUE;
        }

        if(input[8] && !oldinput[8]) {
          g_submode = submode::MAIN;
          combatUIManager->currentOption = 0;
        }


        drawOptionsPanel();

        //combatUIManager->targetText->updateText("To " + g_enemyCombatants.at(combatUIManager->currentTarget)->name, -1, 34);
        combatUIManager->targetText->updateText(combatUIManager->directionalPreposition + " " + g_enemyCombatants.at(combatUIManager->currentTarget)->name, -1, 34);

        combatUIManager->targetPanel->render(renderer, g_camera, elapsed);
        combatUIManager->targetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);



        break;
      }
    case submode::CONTINUE:
      {

        drawOptionsPanel();
        if(curCombatantIndex == g_partyCombatants.size() - 1) {
          g_submode = submode::EXECUTE_P;
          combatUIManager->executePIndex = 0;
          curCombatantIndex = 0;
        } else {
          g_submode = submode::MAIN;
          combatUIManager->currentOption = 0;
          curCombatantIndex++;
          while(g_partyCombatants[curCombatantIndex]->health <= 0) {
            curCombatantIndex++;
            if(curCombatantIndex >= g_partyCombatants.size()) {
              g_submode = submode::EXECUTE_P;
              combatUIManager->executePIndex = 0;
              curCombatantIndex = 0;
            }
          }
        }


        break;
      }
    case submode::EXECUTE_P:
      {

        while(combatUIManager->executePIndex+1 <= g_partyCombatants.size() && g_partyCombatants[combatUIManager->executePIndex]->health <= 0) {
          combatUIManager->executePIndex++;
        }
        combatant* c = g_partyCombatants[combatUIManager->executePIndex];
        if(combatUIManager->executePIndex == g_partyCombatants.size() - 1 && g_partyCombatants[combatUIManager->executePIndex]->health <= 0) {
          //reset all stats
          for(auto x : g_partyCombatants) {
            x->curStrength = x->baseStrength;
            x->curMind = x->baseMind;
            x->curAttack = x->baseAttack;
            x->curDefense = x->baseDefense;
            x->curSoul = x->baseSoul;
            x->curSkill = x->baseSkill;
            x->curCritical = x->baseCritical;
            x->curRecovery = x->baseSoul;
            x->physicalDefense = 0;
            x->spiritDefense = 0;
          }
          g_submode = submode::STATUS_P;
          break;
        }
        if(combatUIManager->executePIndex >= g_partyCombatants.size()) {
          //reset all stats
          for(auto x : g_partyCombatants) {
            x->curStrength = x->baseStrength;
            x->curMind = x->baseMind;
            x->curAttack = x->baseAttack;
            x->curDefense = x->baseDefense;
            x->curSoul = x->baseSoul;
            x->curSkill = x->baseSkill;
            x->curCritical = x->baseCritical;
            x->curRecovery = x->baseSoul;
            x->physicalDefense = 0;
            x->spiritDefense = 0;
          }
          g_submode = submode::STATUS_P;
          break;
        }
        if(c->serial.action == turnAction::ATTACK) {
          if(g_enemyCombatants.size() == 0) {
            g_submode = submode::FINAL;
            break;
          }
          while(c->serial.target >= g_enemyCombatants.size()) {
            c->serial.target-= 1;
          }
          combatant* e = g_enemyCombatants[c->serial.target];

          //do we crit?
          bool crit = 0;
          if(frng(0, 100) < c->curCritical) {
            crit = 1;
          }
          
          //chant wears off
          for(auto &x : c->statuses) {
            if(x.type == status::CHANTED) {
              x.turns = 0;
            }
          }


          int damage = c->curAttack - e->curDefense - e->physicalDefense;
          damage *= frng(0.80,1.20);
          if(crit) { damage *= 3;}
          if(damage < 0) {damage = 0;}
          int dmgToReport = damage;
          if(e->health < dmgToReport) {
            dmgToReport = e->health;
          }
          c->dmgDealtOverFight += dmgToReport;

          e->health -= damage;
          e->damageTakenThisTurn += damage;
          string message;
          if(crit) {
            //message = c->name + " crits " + e->name + " for " + to_string(damage) + ".";
            message = getLanguageData("CombatProtagCrit");
            message = stringMultiInject(message, {c->name, e->name, to_string(damage)});
          } else {
            //message = c->name + " deals " + to_string(damage) + " to " + e->name + ".";
            message = getLanguageData("CombatProtagAttack");
            message = stringMultiInject(message, {c->name, e->name, to_string(damage)});
          }


          if(e->health < 0) {
            string deathmessage = e->name + " " +  e->deathText;
            combatUIManager->queuedStrings.push_back(make_pair(deathmessage, e));
            g_enemyCombatants.erase(g_enemyCombatants.begin() + c->serial.target);
            g_deadCombatants.push_back(e);
            //delete e;
          }

          combatUIManager->finalText = message;
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          g_submode = submode::TEXT_P;
        } else if(c->serial.action == turnAction::ITEM) {
          if(g_enemyCombatants.size() == 0) {
            g_submode = submode::FINAL;
            break;
          }
          while(c->serial.target >= g_enemyCombatants.size()) {
            c->serial.target-= 1;
            if(c->serial.target <0) {break;}
          }
          combatant* com = g_partyCombatants[combatUIManager->executePIndex];
          int a = com->serial.actionIndex; //which item
          int b = com->serial.target; //which ally/enemy
          itemData td;
          td.type = com->itemTypeToUse;
          td.index = com->itemIndexToUse;
          com->itemIndexToUse = -1;

          useItem(td.type, td.index, b, com);

          g_submode = submode::TEXT_P;

        } else if(c->serial.action == turnAction::SPIRITMOVE) {
          if(g_enemyCombatants.size() == 0) {
            g_submode = submode::FINAL;
            break;
          }

          if(spiritTable[c->serial.actionIndex].targeting == 0) {
            while(c->serial.target >= (int)g_enemyCombatants.size()) {
              c->serial.target-= 1;
              M("Stuck in loop which confused me earlier");
            }
          }

          combatant* com = g_partyCombatants[combatUIManager->executePIndex];
          int whichSpiritAbility = com->serial.actionIndex; //which spirit ability
          int target = com->serial.target;
          useSpiritMove(whichSpiritAbility, target, com);
          g_submode = submode::TEXT_P;
        } else if(c->serial.action == turnAction::DEFEND) {
          if(g_enemyCombatants.size() == 0) {
            g_submode = submode::FINAL;
            break;
          }
          string text = getLanguageData("CombatProtagShrinks");
          text = stringMultiInject(text, {g_partyCombatants[combatUIManager->executePIndex]->name});
          combatUIManager->finalText = text;

          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dodgingThisTurn[combatUIManager->executePIndex] = 1;
//          for(auto x :combatUIManager->dodgingThisTurn) {
//          }
          g_submode = submode::TEXT_P;
        } else if(c->serial.action == turnAction::FLEE) {
          int levelDifference = 0;
          int highestTeamateLevel = 0;
          for(auto x : g_partyCombatants) {
            if(x->level > highestTeamateLevel) {
              highestTeamateLevel = x->level;
            }
          }
          int highestEnemyLevel = 0;
          for(auto x : g_enemyCombatants) {
            if(x->level > highestEnemyLevel) {
              highestEnemyLevel = x->level;
            }
          }
          levelDifference = highestTeamateLevel - highestEnemyLevel;
          int random = rng(0,10);
          if(random + levelDifference >= 7) {
            //successful fleeing
            combatUIManager->finalText = g_partyCombatants[combatUIManager->executePIndex]->name + getLanguageData("CombatRunAttempt");
            combatUIManager->queuedStrings.push_back(make_pair(getLanguageData("CombatRunSuccess"), (combatant*)0));
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            combatUIManager->dialogProceedIndicator->y = 0.25;
            g_submode = submode::RUNSUCCESSTEXT;

          } else {
            //failed fleeing
            combatUIManager->finalText = g_partyCombatants[combatUIManager->executePIndex]->name + getLanguageData("CombatRunAttempt");
            combatUIManager->queuedStrings.push_back(make_pair(getLanguageData("CombatRunFail"), (combatant*)0));
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            combatUIManager->dialogProceedIndicator->y = 0.25;
            g_submode = submode::RUNFAILTEXT;

          }
        }

        break;
      }
    case submode::TEXT_P: 
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {
          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }
          curTextWait = 0;
        }

        if( combatUIManager->finalText == combatUIManager->currentText && input[11] && !oldinput[11]) {
          //advance dialog
          if(combatUIManager->queuedStrings.size() > 0) {
            combatUIManager->dialogProceedIndicator->y = 0.25;
            combatUIManager->currentText = "";
            combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;

            combatant* whoDied = combatUIManager->queuedStrings.at(0).second;
            if(whoDied != 0) {
              int index = 0;
              while(index < g_deadCombatants.size()) {
                if(g_deadCombatants.at(index) == whoDied) {
                  whoDied->disappearing = 1;
                  break;
                }
                index++;
              }
            }
            combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
          } else {

            //make sure to handle death from selfdamage properly
            int deadPartyMembers = 0;
            for(auto x : g_partyCombatants) {
              if(x->health <= 0) {
                deadPartyMembers++;
              }
            }

            if(g_partyCombatants[combatUIManager->executePIndex]->health <= 0) {

              combatUIManager->finalText = g_partyCombatants[combatUIManager->executePIndex]->name + " passed out!";
              combatUIManager->currentText = "";
              combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            }

            if(deadPartyMembers == g_partyCombatants.size()) {
              string message = "All party members are knocked-out!";
              combatUIManager->queuedStrings.clear();
              combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
              g_submode = submode::ALLDEADTEXT;
              break;

            }

            if(g_partyCombatants[combatUIManager->executePIndex]->health <= 0) {
              g_submode = submode::MEMBERDEADTEXT_P;
              break;
            }
            

            if(combatUIManager->executePIndex + 1 ==  g_partyCombatants.size()) {
              //reset all stats
              for(auto x : g_partyCombatants) {
                x->curStrength = x->baseStrength;
                x->curMind = x->baseMind;
                x->curAttack = x->baseAttack;
                x->curDefense = x->baseDefense;
                x->curSoul = x->baseSoul;
                x->curSkill = x->baseSkill;
                x->curCritical = x->baseCritical;
                x->curRecovery = x->baseSoul;
                x->physicalDefense = 0;
                x->spiritDefense = 0;
              }
              g_submode = submode::STATUS_P;
            } else {
              combatUIManager->executePIndex++;
              g_submode = submode::EXECUTE_P;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }




        break;
      }
    case submode::EXECUTE_E:
      {
        //it's possible that the protags died during or right after their turn
        //it can crash in this switch case if you aren't careful handling
        //status damage or self damage

        if(combatUIManager->executeEIndex >= g_enemyCombatants.size()) {
          //fight over
          g_submode = submode::FINAL;
          break;
        }
        combatant* c = g_enemyCombatants[combatUIManager->executeEIndex];

        //temp
        c->serial.action = turnAction::ATTACK;
        //this should connect to specialcombatants.cpp
        //by an identity field

        if(c->serial.action == turnAction::ATTACK) {



          //        combatant* e = g_partyCombatants[rng(0, g_partyCombatants.size() - 1)];
          //        int damage = c->baseAttack + (c->attackGain * c->level) - (e->baseDefense + (e->defenseGain * e->level));
          //        damage *= frng(0.70,1.30);
          //        e->health -= damage;
          //        string message = c->name + " deals " + to_string(damage) + " to " + e->name + "!";
          //        g_submode = submode::TEXT_E;
          //        combatUIManager->finalText = message;
          //        combatUIManager->currentText = "";
          //        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          //        combatUIManager->dialogProceedIndicator->y = 0.25;
          //        combatUIManager->mainPanel->show = 0;
          //        combatUIManager->dialogProceedIndicator->show = 0;
          //        combatUIManager->mainText->show = 0;

          vector<combatant*> validCombatants = {};
          for(int i = 0; i < g_partyCombatants.size(); i++) {
            if(g_partyCombatants[i]->health > 0) {
              validCombatants.push_back(g_partyCombatants[i]);
            }
          }

          if(validCombatants.size() <=0) { abort();}

          //int dodgingIndex = rng(0, validCombatants.size() - 1);
          //execute script to determine:
          // who to attack (/target -> dodging index)
          // what patterns to use (/addpattern -> combatUIManager->curPatterns)
          // base dmg per level (/damage ->
          //

          int dodgingIndex = 0;
          string targeting = "";
          vector<int> patterns = {};
          runCombatScript(c->combatScript, combatUIManager->turnCounter, c, targeting, patterns, combatUIManager->specificMultiplier);


          //use targeting to set dodgingIndex
          if(targeting == "lowest") {
            int least = 1000000;
            for(int i = 0; i < validCombatants.size(); i++) {
              if(validCombatants[i]->health < least) {
                dodgingIndex = i;
                least = validCombatants[i]->health;
              }
            }
          } else if(targeting == "highest") {
            int highest = -1;
            for(int i = 0; i < validCombatants.size(); i++) {
              if(validCombatants[i]->health > highest) {
                dodgingIndex = i;
                highest = validCombatants[i]->health;
              }
            }
            
          } else if(targeting == "random") {
            dodgingIndex = rng(0, validCombatants.size() - 1);
          } else if(targeting == "absolute0") {
            //Fomm
            dodgingIndex = 0;
          } else if(targeting == "absolute1") {
            //Neheten
            dodgingIndex = 1;
          } else if(targeting == "absolute2") {
            //Blish
            dodgingIndex = 2;
          } else if(targeting == "absolute3") {
            //Dafua
            dodgingIndex = 3;
          }

          combatUIManager->curPatterns.clear();
          for(auto x : patterns) {
            combatUIManager->curPatterns.push_back(x);
          }








          //check for taunt
          bool breakflag = 0;
          for(auto &x : c->statuses) {
            if(x.type == status::TAUNTED) {
              for(int i = 0; i < validCombatants.size(); i++) {
                if(validCombatants[i]->filename == x.datastr) {
                  dodgingIndex = i;
                  breakflag = 1;
                  break;
                }
              }
            }
            if(breakflag) break;
          }

          combatant* e = validCombatants[dodgingIndex];
          int adjustedDIndex = 0;
          for(auto x : g_partyCombatants) {
            if(x == e) {
              break;
            }
            adjustedDIndex++;
          }
          //combatUIManager->partyDodgingCombatant = e;
          combatUIManager->partyDodgingCombatant = protag->hisCombatant;
          int damage = (c->curAttack* combatUIManager->specificMultiplier) - e->curDefense;
          damage *= frng(0.85,1.15);
          if(damage < 0) {damage = 0;}
          combatUIManager->damageFromEachHit = damage;
          combatUIManager->dodgePanel->x = combatUIManager->dodgePanelSmallX;
          combatUIManager->dodgePanel->y = combatUIManager->dodgePanelSmallY;
          combatUIManager->dodgePanel->width = combatUIManager->dodgePanelSmallWidth;
          combatUIManager->dodgePanel->height = combatUIManager->dodgePanelSmallHeight;
          combatUIManager->dodgePanel->show = 1;
          combatUIManager->incrementDodgeTimer = 0;
          combatUIManager->dodgeTimer = 0;
          combatUIManager->damageTakenFromDodgingPhase = 0;
          combatUIManager->invincibleMs = 0;

          //g_submode = submode::DODGING;
          //string message = c->name + " attacks " + e->name + " for " + to_string(damage) + " damage.";
          string message = getLanguageData("CombatEnemyAttack");
          message = stringMultiInject(message, {c->name, e->name, to_string(damage)});

//          D(adjustedDIndex);
//          for(auto x :combatUIManager->dodgingThisTurn) {
//            D(x);
//          }

          if(combatUIManager->dodgingThisTurn[adjustedDIndex] == 1) {
            //M("Should shrink");
            combatUIManager->shrink = 1;
          } else {
            //M("Shouldn't shrink");
            combatUIManager->shrink = 0;
          }

          combatUIManager->finalText = message;
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;

          if(combatUIManager->curPatterns.size() == 0) {
            combatUIManager->finalText = combatUIManager->idleText;
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            combatUIManager->dialogProceedIndicator->y = 0.25;
            g_submode = submode::TEXT_IDLE;
            break;



          } else {
            g_submode = submode::TEXT_E;
          }


        }

        break;
      }
    case submode::TEXT_E:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          } 
          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText && input[11] && !oldinput[11]) {
          //advance dialog
          if(combatUIManager->queuedStrings.size() > 0) {
            combatUIManager->dialogProceedIndicator->y = 0.25;
            combatUIManager->currentText = "";
            combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
            combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
          } else {

            g_submode = submode::DODGING;
            combatUIManager->accuA = 1000000;
            combatUIManager->accuB = 1000000;
            combatUIManager->accuC = 1000000;
            combatUIManager->dodgerX = 512;
            combatUIManager->dodgerY = 512;
            combatant* e = g_enemyCombatants[combatUIManager->executeEIndex];
  
  //          if(e->attackPatterns.size() <0) {
  //            E("Add attack patterns for " + e->name);
  //            abort();
  //          }
            //combatUIManager->curPatterns = e->attackPatterns[rng(0, e->attackPatterns.size()-1)];
  
  //          M("Spawning bullets for");
  //
  //          for(auto x : combatUIManager->curPatterns) {
  //            cout << x << " ";
  //          }
  //          cout << endl;
  
            for(int i = 0; i < g_miniEnts.size(); i++) {
              delete g_miniEnts[i];
              i--;
            }
            g_miniBullets.clear();
            
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::STATUS_E:
      {
        int breakout = 0;
        //got a crash here after idle text state
        while(curStatusIndex >= (int)g_enemyCombatants[curCombatantIndex]->statuses.size()) {
          curStatusIndex = 0;
          curCombatantIndex++;
          if(curCombatantIndex == (int)g_enemyCombatants.size()) {
            curCombatantIndex = 0;
            for(int i = 0; i < 4; i ++) {
              combatUIManager->dodgingThisTurn[i] = 0;
            }
            while(g_partyCombatants[curCombatantIndex]->health <= 0 && curCombatantIndex+1 < (int)g_partyCombatants.size()) {
              curCombatantIndex ++; //used for choosing which protag picks action in submode::MAIN
            }
            combatUIManager->currentOption = 0;
            combatUIManager->turnCounter++;
            g_submode = submode::MAIN;
            breakout = 1;
            break;

          }
        }
        if(breakout) {break;}

        combatant* c = g_enemyCombatants[curCombatantIndex];
        if(applyStatus(c, &c->statuses[curStatusIndex])) {
          c->statuses.erase(c->statuses.begin() + curStatusIndex);
          curStatusIndex--;
        }

        break;
      }
    case submode::TEXT_STATUS_E:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {

              //this is used when the last enemy dies from a status
              if(g_enemyCombatants.size() == 0) {
                g_submode = submode::FINAL;
                break;
              }

              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              curStatusIndex++;

              //reset all stats
              for(auto x : g_enemyCombatants) {
                x->curStrength = x->baseStrength;
                x->curMind = x->baseMind;
                x->curAttack = x->baseAttack;
                x->curDefense = x->baseDefense;
                x->curSoul = x->baseSoul;
                x->curSkill = x->baseSkill;
                x->curCritical = x->baseCritical;
                x->curRecovery = x->baseSoul;
                x->physicalDefense = 0;
                x->spiritDefense = 0;
              }

              g_submode = submode::STATUS_E;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::FINAL:
      {
        //calculate XP based on total stats of defeated enemies
        //award xp grant xp award exp grant exp give xp give exp

        //return held items to inventory (combatant.itemToUse)
        for(auto x : g_partyCombatants) {
          if(x->itemIndexToUse != -1) {
            if(g_items.size() < g_maxInventorySize) {
              itemData td;
              td.type = x->itemTypeToUse;
              td.index = x->itemIndexToUse;

              g_items.push_back(td);
              x->itemIndexToUse = -1;
            }

          }
        }

        //apply recovery
        for(auto &x : g_partyCombatants) {
          if(x->health > 0) {
            x->health += (x->curRecovery/100.0f) * x->baseStrength;
            x->sp += (x->curRecovery/100.0f) * x->baseMind;
            if(x->health > x->baseStrength) {
              x->health = floor(x->baseStrength);
            }
            if(x->sp > x->baseMind) {
              x->sp = floor(x->baseMind);
            }
          }
        }

        combatUIManager->calculateXP();
        //D(combatUIManager->xpToGrant);
        //combatUIManager->xpToGrant = 1000;
        curCombatantIndex = 0;

        g_submode = submode::CHARAXP;

        //      combatUIManager->currentText = "";
        //      combatUIManager->finalText = "Fomm has won the battle!";
        //      g_submode = submode::FINALTEXT;
        break;
      }
    case submode::CHARAXP:
      {
        if(curCombatantIndex >= g_partyCombatants.size()) {
          //        combatUIManager->currentText = "";
          //        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          //        combatUIManager->finalText = "Fomm has won the battle!";
          //        g_submode = submode::FINALTEXT;
          //        break;
          //g_submode = submode::OUTWIPE;
          //M("ENding here? C");
          g_gainingXPInExplorationMode = 0;
          combatUIManager->hideAll();
    
          adventureUIManager->dialogue_index++;
          adventureUIManager->continueDialogue();
          return;
          break;
        }
        combatant* x = g_partyCombatants[curCombatantIndex];
        x->level = xpToLevel(x->xp);
        combatUIManager->oldLevel = x->level;
        x->xp += combatUIManager->xpToGrant * frng(0.95, 1.05);
        combatUIManager->newLevel= xpToLevel(x->xp);
        combatUIManager->thisLevel = combatUIManager->oldLevel+1;
        //g_submode = submode::LEVELUP;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " gains " + to_string(combatUIManager->xpToGrant) + " xp.";
        g_submode = submode::XPTEXT;


        break;
      }
    case submode::XPTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              g_submode = submode::LEVELUP;
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::LEVELUP:
      {
        if(combatUIManager->thisLevel > combatUIManager->newLevel) {
          curCombatantIndex++;
          g_submode = submode::CHARAXP;
          break;
        }

        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        string line = "";
        line = stringMultiInject(getLanguageData("CombatLevelUp0"), {g_partyCombatants[curCombatantIndex]->name, to_string(combatUIManager->thisLevel)});
        combatUIManager->finalText = line;


        g_partyCombatants[curCombatantIndex]-> strIncrease = g_partyCombatants[curCombatantIndex]->strengthGain * frng(0.8, 1.2);
        g_partyCombatants[curCombatantIndex]->baseStrength += g_partyCombatants[curCombatantIndex]->strIncrease;
        g_partyCombatants[curCombatantIndex]->health += g_partyCombatants[curCombatantIndex]->strIncrease;
        string line2 = stringMultiInject(getLanguageData("CombatLevelUp1"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseStrength), to_stringF(g_partyCombatants[curCombatantIndex]->strIncrease)});
        //combatUIManager->queuedStrings.push_back(make_pair(line,0));
        //combatUIManager->finalText += "\n" + line2;

        g_partyCombatants[curCombatantIndex]->mindIncrease = g_partyCombatants[curCombatantIndex]->mindGain * frng(0.8, 1.2);
        g_partyCombatants[curCombatantIndex]->baseMind += g_partyCombatants[curCombatantIndex]->mindIncrease;
        g_partyCombatants[curCombatantIndex]->sp += g_partyCombatants[curCombatantIndex]->mindIncrease;
        line = stringMultiInject(getLanguageData("CombatLevelUp2"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseMind), to_stringF(g_partyCombatants[curCombatantIndex]->mindIncrease)});
        combatUIManager->queuedStrings.push_back(make_pair(line2 + "\n" + line,(combatant*)0));
        //combatUIManager->queuedStrings.push_back(make_pair(line,0));

        g_partyCombatants[curCombatantIndex]->attackIncrease = g_partyCombatants[curCombatantIndex]->attackGain * frng(0.8, 1.2);

        g_partyCombatants[curCombatantIndex]->baseAttack += g_partyCombatants[curCombatantIndex]->attackIncrease;
        line2 = stringMultiInject(getLanguageData("CombatLevelUp3"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseAttack), to_stringF(g_partyCombatants[curCombatantIndex]->attackIncrease)});

        g_partyCombatants[curCombatantIndex]->defenseIncrease = g_partyCombatants[curCombatantIndex]->defenseGain * frng(0.8, 1.2);
        g_partyCombatants[curCombatantIndex]->baseDefense += g_partyCombatants[curCombatantIndex]->defenseIncrease;
        line = stringMultiInject(getLanguageData("CombatLevelUp4"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseDefense), to_stringF(g_partyCombatants[curCombatantIndex]->defenseIncrease)});
        combatUIManager->queuedStrings.push_back(make_pair(line2 + "\n" + line,(combatant*)0));

        g_partyCombatants[curCombatantIndex]->soulIncrease = g_partyCombatants[curCombatantIndex]->soulGain * frng(0.8, 1.2);
        g_partyCombatants[curCombatantIndex]->baseSoul += g_partyCombatants[curCombatantIndex]->soulIncrease;
        line2 = stringMultiInject(getLanguageData("CombatLevelUp5"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseSoul), to_stringF(g_partyCombatants[curCombatantIndex]->soulIncrease)});

        g_partyCombatants[curCombatantIndex]->criticalIncrease = g_partyCombatants[curCombatantIndex]->criticalGain * frng(0.8, 1.2);

        g_partyCombatants[curCombatantIndex]->baseCritical += g_partyCombatants[curCombatantIndex]->criticalIncrease;

        line = stringMultiInject(getLanguageData("CombatLevelUp6"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseCritical), to_stringF(g_partyCombatants[curCombatantIndex]->criticalIncrease)});
        combatUIManager->queuedStrings.push_back(make_pair(line2 + "\n" + line,(combatant*)0));


        g_partyCombatants[curCombatantIndex]->skillIncrease = g_partyCombatants[curCombatantIndex]->skillGain * frng(0.8, 1.2);
        g_partyCombatants[curCombatantIndex]->baseSkill += g_partyCombatants[curCombatantIndex]->skillIncrease;
        line = stringMultiInject(getLanguageData("CombatLevelUp7"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseSkill), to_stringF(g_partyCombatants[curCombatantIndex]->skillIncrease)});


        g_partyCombatants[curCombatantIndex]->recoveryIncrease = g_partyCombatants[curCombatantIndex]->recoveryGain * frng(0.8, 1.2);
        g_partyCombatants[curCombatantIndex]->baseRecovery += g_partyCombatants[curCombatantIndex]->recoveryIncrease;
        line2 = stringMultiInject(getLanguageData("CombatLevelUp8"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseRecovery), to_stringF(g_partyCombatants[curCombatantIndex]->recoveryIncrease)});


        combatUIManager->queuedStrings.push_back(make_pair(line + "\n" + line2,(combatant*)0));


        g_partyCombatants[curCombatantIndex]->level = combatUIManager->thisLevel;

        g_submode = submode::LEVELTEXT;



        break;
      }
    case submode::FINALTEXT:
      {
        for(int i = 0; i < g_enemyCombatants.size(); i++) {
          delete g_enemyCombatants[i];
        }
        g_enemyCombatants.clear();
        for(int i = 0; i < g_deadCombatants.size(); i++) {
          delete g_deadCombatants[i];
        }
        g_deadCombatants.clear();
        curCombatantIndex = 0;
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              g_submode = submode::OUTWIPE;
              //M("Ending here? B");
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::LEVELTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              //check and see if they can learn a spiritmove
              bool canLearnMove = 0;
              for(auto x : g_partyCombatants[curCombatantIndex]->spiritTree) {
                int a = x.first;
                int b = x.second;
                if(a == combatUIManager->thisLevel) {
                  canLearnMove = 1;
                  combatUIManager->moveToLearn = x.second;
                  break;
                }
              }

              if(canLearnMove) {
                if(g_partyCombatants[curCombatantIndex]->spiritMoves.size() < 4) {
                  //just learn it 
                  combatUIManager->currentText = "";
                  combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
                  //combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " learned " + spiritTable[combatUIManager->moveToLearn].name + ".";
                  combatUIManager->finalText = stringMultiInject(getLanguageData("LearnedMoveSuccess"), {g_partyCombatants[curCombatantIndex]->name, spiritTable[combatUIManager->moveToLearn].name});
                  g_partyCombatants[curCombatantIndex]->spiritMoves.push_back(combatUIManager->moveToLearn);
                  g_submode = submode::LEARNEDTEXT;
                  break;
                } else {
                  combatUIManager->currentText = "";
                  combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
                  //combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " can learn " + spiritTable[combatUIManager->moveToLearn].name + ", but would need to forget another move.";
                  combatUIManager->finalText = stringMultiInject(getLanguageData("LearnedMovePossible"), {g_partyCombatants[curCombatantIndex]->name, spiritTable[combatUIManager->moveToLearn].name});

                  //combatUIManager->queuedStrings.push_back(make_pair("Choose a move for " + g_partyCombatants[curCombatantIndex]->name + " to do without.",0));
                  combatUIManager->queuedStrings.push_back(make_pair(stringMultiInject(getLanguageData("LearnedMovePossible2"), {g_partyCombatants[curCombatantIndex]->name}),(combatant*)0));

                  g_submode = submode::LEARNTEXT;
                  break;
                }
              } else {
                //no move to learn, go to the next levelup
                combatUIManager->thisLevel++;
                g_submode = submode::LEVELUP;
                break;
              }
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::LEARNEDTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              //no move to learn, go to the next levelup
              combatUIManager->thisLevel++;
              g_submode = submode::LEVELUP;

              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::LEARNTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              g_submode = submode::FORGET;
              combatUIManager->forgetPanel->show = 1;
              combatUIManager->forgetInfoPanel->show = 1;
              combatUIManager->forgetText->show = 1;
              combatUIManager->forgetInfoText->show = 1;
              combatUIManager->forgetOption = 0;
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::FORGET:
      {
        if(input[0] && !oldinput[0]) {
          if(combatUIManager->forgetOption > 0) {
            combatUIManager->forgetOption -= 1;
          }
        }

        if(input[1] && !oldinput[1]) {
          if(combatUIManager->forgetOption < 4) {
            combatUIManager->forgetOption += 1;
          }
        }
        if(input[11] && !oldinput[11]) {
          if(combatUIManager->forgetOption < 4) {
            //combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " will forget " + spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->forgetOption]].name + " and learn " + spiritTable[combatUIManager->moveToLearn].name + ".";
            combatUIManager->finalText = stringMultiInject(getLanguageData("ForgetMoveConfirm"), {g_partyCombatants[curCombatantIndex]->name, spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->forgetOption]].name, spiritTable[combatUIManager->moveToLearn].name});
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            g_submode = submode::FORGETTEXT;
            combatUIManager->forgetPanel->show = 0;
            combatUIManager->forgetInfoPanel->show = 0;
            combatUIManager->forgetText->show = 0;
            combatUIManager->forgetInfoText->show = 0;
            combatUIManager->forgetPicker->show = 0;
            break;
          } else {
            //combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " will not learn " + spiritTable[combatUIManager->moveToLearn].name + ".";
            combatUIManager->finalText = stringMultiInject(getLanguageData("WontLearnMoveConfirm"), {g_partyCombatants[curCombatantIndex]->name, spiritTable[combatUIManager->moveToLearn].name});
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            g_submode = submode::FORGETTEXT;
            combatUIManager->forgetPanel->show = 0;
            combatUIManager->forgetInfoPanel->show = 0;
            combatUIManager->forgetText->show = 0;
            combatUIManager->forgetInfoText->show = 0;
            combatUIManager->forgetPicker->show = 0;
            break;

          }
          break;
        }

        break;
      }
    case submode::FORGETTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if((input[11] && !oldinput[11]) || 1) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              g_submode = submode::FORGETCONFIRM;
              combatUIManager->confirmOption = 0;
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::FORGETCONFIRM:
      {
        combatUIManager->dialogProceedIndicator->show = 0;

        if(input[2] && !oldinput[2]) {
          combatUIManager->confirmOption = 0;
        }
        if(input[3] && !oldinput[3]) {
          combatUIManager->confirmOption = 1;
        }

        if(input[11] && !oldinput[11]) {
          if(combatUIManager->confirmOption == 0) {
            g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->forgetOption] = combatUIManager->moveToLearn;
            combatUIManager->thisLevel++;
            g_submode = submode::LEVELUP;
          } else {
            //combatUIManager->mainText->updateText("Choose a move for " + g_partyCombatants[curCombatantIndex]->name + " to do without.", -1, 0.85, g_textcolor, g_font);
            combatUIManager->mainText->updateText(stringMultiInject(getLanguageData("LearnedMovePossible2"), {g_partyCombatants[curCombatantIndex]->name }), -1, 0.85, g_textcolor, g_font);

            g_submode = submode::FORGET;
            combatUIManager->forgetPanel->show = 1;
            combatUIManager->forgetInfoPanel->show = 1;
            combatUIManager->forgetText->show = 1;
            combatUIManager->forgetInfoText->show = 1;
            combatUIManager->forgetOption = 0;
          }
        }

        break;
      }
    case submode::ITEMCHOOSE: 
      {

        if(g_partyCombatants[curCombatantIndex]->itemIndexToUse != -1) {
          if(g_items.size() < g_maxInventorySize) {
            itemData td;
            td.type = g_partyCombatants[curCombatantIndex]->itemTypeToUse;
            td.index = g_partyCombatants[curCombatantIndex]->itemIndexToUse;
            g_items.push_back(td);
          }
          g_partyCombatants[curCombatantIndex]->itemIndexToUse = -1;
        }
        combatUIManager->mainPanel->show = 0;
        combatUIManager->dialogProceedIndicator->show = 0;
        combatUIManager->mainText->show = 0;

        combatUIManager->optionsPanel->show = 1;
        combatUIManager->menuPicker->show = 1;
        combatUIManager->optionsText->show = 1;

        combatUIManager->targetPanel->show = 0;
        combatUIManager->targetText->show = 0;

        combatUIManager->inventoryPanel->show = 1;
        combatUIManager->inventoryText->show = 1;

        drawOptionsPanel();

        combatUIManager->inventoryPanel->render(renderer, g_camera, elapsed);

        if(input[0] && !oldinput[0]) {
          if(combatUIManager->currentInventoryOption != 0 &&
              combatUIManager->currentInventoryOption != 7) {
            combatUIManager->currentInventoryOption --;
          }
        }

        if(input[1] && !oldinput[1]) {
          if(combatUIManager->currentInventoryOption != 6 &&
              combatUIManager->currentInventoryOption != 13) {
            if(combatUIManager->currentInventoryOption + 1 < g_items.size()) {
              combatUIManager->currentInventoryOption ++;
            }
          }
        }

        if(input[2] && !oldinput[2]) {
          if(combatUIManager->currentInventoryOption >= 7) {
            combatUIManager->currentInventoryOption -= 7;
          }
        }

        if(input[3] && !oldinput[3]) {
          if(combatUIManager->currentInventoryOption <= 6) {
            if(combatUIManager->currentInventoryOption + 7 < g_items.size()) {
              combatUIManager->currentInventoryOption += 7;
            }
          }
        }


        if(input[8] && !oldinput[8]) {
          g_submode = submode::MAIN;
          //combatUIManager->currentOption = 0;
          combatUIManager->inventoryPanel->show = 0;
          combatUIManager->inventoryText->show = 0;
        }

        if(input[11] && !oldinput[11] && g_items.size() > 0) {
          g_partyCombatants[curCombatantIndex]->itemTypeToUse = g_items[combatUIManager->currentInventoryOption].type;
          g_partyCombatants[curCombatantIndex]->itemIndexToUse = g_items[combatUIManager->currentInventoryOption].index;
          itemData td;
          td = g_items[combatUIManager->currentInventoryOption];
          switch(itemsTable[td.type][td.index].targeting) {
            case 0:
              //enemy
              g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ITEM;
              g_partyCombatants[curCombatantIndex]->serial.actionIndex = g_items[combatUIManager->currentInventoryOption].type;
              g_partyCombatants[curCombatantIndex]->serial.secondActionIndex = g_items[combatUIManager->currentInventoryOption].index;

              g_submode = submode::TARGETING;
              combatUIManager->currentTarget = 0;
              break;
            case 1:
              //teamate
              g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ITEM;
              g_partyCombatants[curCombatantIndex]->serial.actionIndex = g_items[combatUIManager->currentInventoryOption].type;
              g_partyCombatants[curCombatantIndex]->serial.secondActionIndex = g_items[combatUIManager->currentInventoryOption].index;
              g_submode = submode::ALLYTARGETING;
              combatUIManager->currentTarget = 0;
              break;
            case 2:
              //none
              g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ITEM;
              g_partyCombatants[curCombatantIndex]->serial.actionIndex = g_items[combatUIManager->currentInventoryOption].type;
              g_partyCombatants[curCombatantIndex]->serial.secondActionIndex = g_items[combatUIManager->currentInventoryOption].index;
              g_submode = submode::CONTINUE;
              break;
          }
          g_items.erase(g_items.begin() + combatUIManager->currentInventoryOption);

        }

        renderInventoryPanel();

        break;
      }
    case submode::ALLYTARGETING: 
      {
        combatUIManager->mainPanel->show = 0;
        combatUIManager->dialogProceedIndicator->show = 0;
        combatUIManager->mainText->show = 0;

        combatUIManager->optionsPanel->show = 1;
        combatUIManager->menuPicker->show = 1;
        combatUIManager->optionsText->show = 1;

        combatUIManager->targetPanel->show = 1;
        combatUIManager->targetText->show = 1;

        combatUIManager->tcm_accumulator += 0.1;
        if(combatUIManager->tcm_accumulator > M_PI * 2) {
          combatUIManager->tcm_accumulator -= M_PI * 2;
        }

        if(input[2] && !oldinput[2]) {
          if(combatUIManager->currentTarget > 0) {
            combatUIManager->currentTarget --;
          }
        }

        if(input[3] && !oldinput[3]) {
          if(combatUIManager->currentTarget < g_partyCombatants.size() - 1) {
            combatUIManager->currentTarget ++;
          }
        }

        if(combatUIManager->currentTarget < 0) { combatUIManager->currentTarget = 0; }
        if(combatUIManager->currentTarget >= g_partyCombatants.size()) { combatUIManager->currentTarget = g_partyCombatants.size() - 1; }


        if(input[11] && !oldinput[11]) {
          g_partyCombatants[curCombatantIndex]->serial.target = combatUIManager->currentTarget;
          g_submode = submode::CONTINUE;
        }

        if(input[8] && !oldinput[8]) {
          g_submode = submode::MAIN;
          combatUIManager->currentOption = 0;
        }


        drawOptionsPanel();

        combatUIManager->targetText->updateText("To " + g_partyCombatants.at(combatUIManager->currentTarget)->name, -1, 34);

        combatUIManager->targetPanel->render(renderer, g_camera, elapsed);
        combatUIManager->targetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);



        break;
      }
    case submode::SPIRITCHOOSE:
      {
        combatUIManager->mainPanel->show = 0;
        combatUIManager->dialogProceedIndicator->show = 0;
        combatUIManager->mainText->show = 0;

        combatUIManager->optionsPanel->show = 1;
        combatUIManager->menuPicker->show = 1;
        combatUIManager->optionsText->show = 1;

        combatUIManager->targetPanel->show = 0;
        combatUIManager->targetText->show = 0;

        combatUIManager->spiritPanel->show = 1;
        combatUIManager->spiritText->show = 1;

        drawOptionsPanel();


        if(input[0] && !oldinput[0]) {
          if(combatUIManager->currentInventoryOption > 0) {
            combatUIManager->currentInventoryOption --;
          }
        }

        if(input[1] && !oldinput[1]) {
          if(combatUIManager->currentInventoryOption + 1 < g_partyCombatants[curCombatantIndex]->spiritMoves.size()) {
            combatUIManager->currentInventoryOption ++;
          }
        }

        if(input[8] && !oldinput[8]) {
          g_submode = submode::MAIN;
          //combatUIManager->currentOption = 0;
          combatUIManager->spiritPanel->show = 0;
          combatUIManager->spiritText->show = 0;
        }

        if(input[11] && !oldinput[11] && g_partyCombatants[curCombatantIndex]->spiritMoves.size() > 0) {
          //does he have enough sp?
          int cost = spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption]].cost;
          int currentSp = g_partyCombatants[curCombatantIndex]->sp;
          string name = spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption]].name;

          if(currentSp < cost) {
            //combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " doesn't have enough SP for " + name + ".";
            combatUIManager->finalText = stringMultiInject(getLanguageData("CombatSPWarning"), {g_partyCombatants[curCombatantIndex]->name, name});
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            combatUIManager->dialogProceedIndicator->y = 0.25;
            g_submode = submode::SPWARNING;

          } else {
            switch(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption]].targeting) {
              case 0:
                {
                  //enemy
                  g_partyCombatants[curCombatantIndex]->serial.action = turnAction::SPIRITMOVE;
                  int spiritNumber = g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption];
                  g_partyCombatants[curCombatantIndex]->serial.actionIndex = spiritNumber;

                  g_submode = submode::TARGETING;
                  break;
                }
              case 1:
                {
                  //teamate
                  g_partyCombatants[curCombatantIndex]->serial.action = turnAction::SPIRITMOVE;
                  int spiritMove = g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption];
                  int spiritNumber = spiritMove;
                  g_partyCombatants[curCombatantIndex]->serial.actionIndex = spiritNumber;

                  g_submode = submode::ALLYTARGETING;
                  break;
                }
              case 2:
                {
                  //none
                  g_partyCombatants[curCombatantIndex]->serial.action = turnAction::SPIRITMOVE;
                  g_partyCombatants[curCombatantIndex]->serial.target = -1;
                  int spiritNumber = g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption];
                  g_partyCombatants[curCombatantIndex]->serial.actionIndex = spiritNumber;
                  g_submode = submode::CONTINUE;
                  break;
                }
            }
          }

        }

        renderSpiritPanel();

        break;
      }
    case submode::SPWARNING: 
      {
        //curCombatantIndex = 0;
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              g_submode = submode::SPIRITCHOOSE;
              combatUIManager->currentOption = 0;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::DODGING:
      {
        combatUIManager->dialogProceedIndicator->show = 0;
        float rate = 2;
        if(combatUIManager->dodgePanel->x > combatUIManager->dodgePanelFullX) {
          combatUIManager->dodgePanel->x -= 0.01 * rate;
        }
        if(combatUIManager->dodgePanel->x <= combatUIManager->dodgePanelFullX) {
          combatUIManager->dodgePanel->x = combatUIManager->dodgePanelFullX;
          combatUIManager->incrementDodgeTimer = 1;
        }

        if(combatUIManager->dodgePanel->y > combatUIManager->dodgePanelFullY) {
          combatUIManager->dodgePanel->y -= 0.01 * rate * (16.0f / 10.0f);;
        }
        if(combatUIManager->dodgePanel->y < combatUIManager->dodgePanelFullY) {
          combatUIManager->dodgePanel->y = combatUIManager->dodgePanelFullY;
        }

        if(combatUIManager->dodgePanel->width < combatUIManager->dodgePanelFullWidth) {
          combatUIManager->dodgePanel->width += 0.02 * rate;
        }
        if(combatUIManager->dodgePanel->width > combatUIManager->dodgePanelFullWidth) {
          combatUIManager->dodgePanel->width = combatUIManager->dodgePanelFullWidth;
        }

        if(combatUIManager->dodgePanel->height < combatUIManager->dodgePanelFullHeight) {
          combatUIManager->dodgePanel->height += 0.02 * rate * (16.0f / 10.0f);
        }
        if(combatUIManager->dodgePanel->height > combatUIManager->dodgePanelFullHeight) {
          combatUIManager->dodgePanel->height = combatUIManager->dodgePanelFullHeight;
        }

        if(combatUIManager->incrementDodgeTimer) {
          combatUIManager->dodgeTimer += elapsed;
          bool movingUp = input[0];
          bool movingDown = input[1];
          bool movingLeft = input[2];
          bool movingRight = input[3];

          // Determine if diagonal movement is happening
          bool diagonalMovement = (movingUp || movingDown) && (movingLeft || movingRight);

          // Normalize speed for diagonal movement
          float speed = diagonalMovement ? combatUIManager->dodgerSpeed / sqrt(2) : combatUIManager->dodgerSpeed;

          if (movingUp) {
            combatUIManager->dodgerY -= speed;
          }
          if (movingDown) {
            combatUIManager->dodgerY += speed;
          }
          if (movingLeft) {
            combatUIManager->dodgerX -= speed;
          }
          if (movingRight) {
            combatUIManager->dodgerX += speed;
          }

          float margin = combatUIManager->dodgerWidth/2;
          if(combatUIManager->dodgerX < 0 + margin) {
            combatUIManager->dodgerX = 0 + margin;
          }
          if(combatUIManager->dodgerY < 0 + margin) {
            combatUIManager->dodgerY = margin;
          }
          if(combatUIManager->dodgerX > 1024 - margin) {
            combatUIManager->dodgerX = 1024 - margin;
          }
          if(combatUIManager->dodgerY > 1024 - margin) {
            combatUIManager->dodgerY = 1024 - margin;
          }
        }
        if(combatUIManager->partyDodgingCombatant->health <= 0 || (devMode && input[8])) {
          //end early
          combatUIManager->dodgeTimer = combatUIManager->maxDodgeTimer + 1;
        }

        if(combatUIManager->dodgeTimer > combatUIManager->maxDodgeTimer) {
          // delete all miniEnts
          int size = g_miniEnts.size();
          for(int i = 0; i < size; i++) {
            delete g_miniEnts[0];
          }

          if(combatUIManager->executeEIndex + 1 ==  g_enemyCombatants.size()) {

            int deadPartyMembers = 0;
            for(auto x : g_partyCombatants) {
              if(x->health <= 0) {
                deadPartyMembers++;
              }
            }

            if(combatUIManager->partyDodgingCombatant->health <= 0) {
              combatUIManager->finalText = combatUIManager->partyDodgingCombatant->name + " passed out!";
              combatUIManager->currentText = "";
              combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            }


            if(deadPartyMembers == g_partyCombatants.size()) {
              //string message = "All party members are knocked-out!";
              string message = getLanguageData("CombatPartyDead");
              combatUIManager->queuedStrings.clear();
              combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
              g_submode = submode::ALLDEADTEXT;
              break;

            }

            if(combatUIManager->partyDodgingCombatant->health <= 0) {
              g_submode = submode::MEMBERDEADTEXT;
              break;
            }

            combatUIManager->mainPanel->show = 0;
            combatUIManager->mainText->show = 0;
            combatUIManager->dialogProceedIndicator->show = 0;
            curCombatantIndex = 0;
            curStatusIndex = 0;

            //reset all stats
            for(auto x : g_enemyCombatants) {
              x->curStrength = x->baseStrength;
              x->curMind = x->baseMind;
              x->curAttack = x->baseAttack;
              x->curDefense = x->baseDefense;
              x->curSoul = x->baseSoul;
              x->curSkill = x->baseSkill;
              x->curCritical = x->baseCritical;
              x->curRecovery = x->baseSoul;
              x->physicalDefense = 0;
              x->spiritDefense = 0;
            }

            g_submode = submode::STATUS_E;

          } else {
            //if the character died, report on it with MEMBERDEADTEXT. If all characters are dead, report on it with ALLDEADTEXT

            int deadPartyMembers = 0;
            for(auto x : g_partyCombatants) {
              if(x->health <= 0) {
                deadPartyMembers++;
              }
            }

            if(combatUIManager->partyDodgingCombatant->health <= 0) {

              combatUIManager->finalText = combatUIManager->partyDodgingCombatant->name + " passed out!";
              combatUIManager->currentText = "";
              combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            }

            if(deadPartyMembers == g_partyCombatants.size()) {
              //string message = "All party members are knocked-out!";
              string message = getLanguageData("CombatPartyDead");
              combatUIManager->queuedStrings.clear();
              combatUIManager->queuedStrings.push_back(make_pair(message,(combatant*)0));
              g_submode = submode::ALLDEADTEXT;
              break;

            }

            if(combatUIManager->partyDodgingCombatant->health <= 0) {
              g_submode = submode::MEMBERDEADTEXT;
              break;
            }


            if(combatUIManager->executeEIndex + 1 ==  g_enemyCombatants.size()) {


            } else {
              combatUIManager->executeEIndex++;
              g_submode = submode::EXECUTE_E;
            }
          }

        }
        break;
      }
    case submode::TEXT_IDLE:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
//              combatUIManager->mainPanel->show = 0;
//              combatUIManager->mainText->show = 0;
//              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              //curStatusIndex++;

//              combatUIManager->executeEIndex++;
//              M("IDLETEXT TO EXECUTE_E");
//              g_submode = submode::EXECUTE_E;

              if(combatUIManager->executeEIndex + 1 ==  g_enemyCombatants.size()) {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              curCombatantIndex = 0;
              combatUIManager->executeEIndex = 0;
              g_submode = submode::STATUS_E;
              curStatusIndex = 0;

  
  
              } else {
                combatUIManager->executeEIndex++;
                g_submode = submode::EXECUTE_E;
              }
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::STATUS_P:
      {
        int breakout = 0;
        while(curStatusIndex >= g_partyCombatants[curCombatantIndex]->statuses.size()) {
          curStatusIndex = 0;
          curCombatantIndex++;
          if(curCombatantIndex == g_partyCombatants.size()) {
            curCombatantIndex = 0;
//            for(int i = 0; i < 4; i ++) {
//              combatUIManager->dodgingThisTurn[i] = 0;
//            }
            while(g_partyCombatants[curCombatantIndex]->health <= 0 && curCombatantIndex+1 < g_partyCombatants.size()) {
              curCombatantIndex ++;
            }
            g_submode = submode::EXECUTE_E;
            combatUIManager->executeEIndex = 0;
            breakout = 1;
            break;
          }
        }
        if(breakout) {break;}

        combatant* c = g_partyCombatants[curCombatantIndex];
        if(applyStatus(c, &c->statuses[curStatusIndex])) {
          c->statuses.erase(c->statuses.begin() + curStatusIndex);
          curStatusIndex--;
        }
        break;
      }
    case submode::TEXT_STATUS_P:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              curStatusIndex++;

              g_submode = submode::STATUS_P;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::RUNWARNING: 
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              g_submode = submode::MAIN;
              combatUIManager->currentOption = 0;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::RUNSUCCESSTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }

        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              g_submode = submode::OUTWIPE;
              //M("Ending here? A");
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::RUNFAILTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }

        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              if(combatUIManager->executePIndex + 1 ==  g_partyCombatants.size()) {
                g_submode = submode::EXECUTE_E;
                combatUIManager->executeEIndex = 0;
              } else {
                combatUIManager->executePIndex++;
                g_submode = submode::EXECUTE_P;
              }
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::MEMBERDEADTEXT:
      {

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }

        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              if(combatUIManager->executeEIndex + 1 ==  g_enemyCombatants.size()) {
                combatUIManager->mainPanel->show = 0;
                combatUIManager->mainText->show = 0;
                combatUIManager->dialogProceedIndicator->show = 0;
                curCombatantIndex = 0;
                curStatusIndex = 0;
                //reset all stats
                for(auto x : g_enemyCombatants) {
                  x->curStrength = x->baseStrength;
                  x->curMind = x->baseMind;
                  x->curAttack = x->baseAttack;
                  x->curDefense = x->baseDefense;
                  x->curSoul = x->baseSoul;
                  x->curSkill = x->baseSkill;
                  x->curCritical = x->baseCritical;
                  x->curRecovery = x->baseSoul;
                  x->physicalDefense = 0;
                  x->spiritDefense = 0;
                }
                g_submode = submode::STATUS_E;
                //g_submode = submode::MAIN;
                break;
              } else {
                combatUIManager->executeEIndex++;
                g_submode = submode::EXECUTE_E;
                break;
              }
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::MEMBERDEADTEXT_P:
      {

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }

        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              if(combatUIManager->executePIndex + 1 ==  g_partyCombatants.size()) {
                //reset all stats
                for(auto x : g_partyCombatants) {
                  x->curStrength = x->baseStrength;
                  x->curMind = x->baseMind;
                  x->curAttack = x->baseAttack;
                  x->curDefense = x->baseDefense;
                  x->curSoul = x->baseSoul;
                  x->curSkill = x->baseSkill;
                  x->curCritical = x->baseCritical;
                  x->curRecovery = x->baseSoul;
                  x->physicalDefense = 0;
                  x->spiritDefense = 0;
                }
                g_submode = submode::STATUS_P;
              } else {
                combatUIManager->executePIndex++;
                g_submode = submode::EXECUTE_P;
              }
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::ALLDEADTEXT:
      {

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }

        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {


              //g_gamemode = gamemode::LOSS;
              g_lossSub = lossSub::INWIPE;
              transitionDelta = transitionImageHeight;
              g_submode = submode::OUTWIPEL;//dont run the code to draw the minients after the switch statement
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
  }


  combatUIManager->mainPanel->render(renderer, g_camera, elapsed);
  combatUIManager->mainText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
  combatUIManager->dialogProceedIndicator->render(renderer, g_camera, elapsed);


  if(g_submode == submode::DODGING) {
    combatUIManager->dodgePanel->render(renderer, g_camera, elapsed);
    SDL_SetRenderTarget(renderer, combatUIManager->rendertarget);
    SDL_SetRenderDrawColor(renderer, 6, 7, 6, 255);
    SDL_RenderClear(renderer);

    combatUIManager->accuA += elapsed;
    combatUIManager->accuB += elapsed;
    combatUIManager->accuC += elapsed;

    if(combatUIManager->curPatterns.size() > 0) {
      spawnBullets(combatUIManager->curPatterns[0], combatUIManager->accuA);
    }

    if(combatUIManager->curPatterns.size() > 1) {
      spawnBullets(combatUIManager->curPatterns[1], combatUIManager->accuB);
    }

    if(combatUIManager->curPatterns.size() > 2) {
      spawnBullets(combatUIManager->curPatterns[2], combatUIManager->accuC);
    }

    {
      for(auto x : g_miniEnts) {
        x->update(elapsed);
      }
      for(int i = 0; i < g_miniBullets.size(); i++) {
        g_miniBullets[i]->bulletUpdate(elapsed);
      }
      for(int i = 0; i < g_miniEnts.size(); i++) {
        if(g_miniEnts[i]->x < -SPAWN_MARGIN || g_miniEnts[i]->x > SCREEN_WIDTH + SPAWN_MARGIN ||
            g_miniEnts[i]->y < -SPAWN_MARGIN || g_miniEnts[i]->y > SCREEN_HEIGHT + SPAWN_MARGIN) {
          delete g_miniEnts[i];
          i--;
          continue;
        }
        if(g_miniEnts[i]->exploded) {
          delete g_miniEnts[i];
          i--;
          continue;
        }
      }
      for(auto x : g_miniEnts) {
        x->render(1);
      }
      for(auto x : g_miniEnts) {
        x->render(0);
      }

      if(combatUIManager->shrink) {
        combatUIManager->dodgerWidth = 50;
        combatUIManager->dodgerHeight = 50;
      } else {
        combatUIManager->dodgerWidth = 100;
        combatUIManager->dodgerHeight = 100;
      }
      if(combatUIManager->invincibleMs <= 0) {
        for(auto x : g_miniBullets) {
          if(Distance(combatUIManager->dodgerX, combatUIManager->dodgerY, x->x, x->y) < (combatUIManager->dodgerWidth + x->w)/2) {
            combatUIManager->partyDodgingCombatant->health -= combatUIManager->damageFromEachHit;
            int dmgToReport = combatUIManager->damageFromEachHit;
            if(dmgToReport > combatUIManager->partyDodgingCombatant->health) {
              dmgToReport = combatUIManager->partyDodgingCombatant->health;
            }
            combatUIManager->partyDodgingCombatant->dmgTakenOverFight += dmgToReport;
            if(combatUIManager->partyDodgingCombatant->health < 0) {combatUIManager->partyDodgingCombatant->health = 0;}

            combatUIManager->damageTakenFromDodgingPhase += combatUIManager->damageFromEachHit;
            combatUIManager->invincibleMs = combatUIManager->maxInvincibleMs;
            break;
          }
        }
      }
      SDL_Rect drect;
      drect.x = combatUIManager->dodgerX - combatUIManager->dodgerWidth/2;
      drect.y = combatUIManager->dodgerY - combatUIManager->dodgerHeight/2;
      drect.w = combatUIManager->dodgerWidth;
      drect.h = combatUIManager->dodgerHeight;

      if(combatUIManager->invincibleMs > 0) {
        if(combatUIManager->blinkMs > 50) {
          combatUIManager->drawDodger = !combatUIManager->drawDodger;
          combatUIManager->blinkMs = 0;
        }
      } else {
        combatUIManager->drawDodger = 1;
      }

      if(combatUIManager->drawDodger) {
        SDL_SetTextureColorMod(combatUIManager->dodgerTexture, 255*0.7, 255*0.7, 255*0.7);
        SDL_RenderCopy(renderer, combatUIManager->dodgerTexture, NULL, &drect);
        SDL_SetTextureColorMod(combatUIManager->dodgerTexture, 255, 255, 255);
        drect.x += 10;
        drect.y += 10;
        drect.w -= 20;
        drect.h -= 20;
        SDL_RenderCopy(renderer, combatUIManager->dodgerTexture, NULL, &drect);
      }


      combatUIManager->invincibleMs -= elapsed;
      combatUIManager->blinkMs += elapsed;
    }







    SDL_SetRenderTarget(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_Rect dstrect;
    float padding = 0.04;
    dstrect.x = (combatUIManager->dodgePanel->x + padding/2) * WIN_WIDTH;
    dstrect.y = (combatUIManager->dodgePanel->y + (padding*combatUIManager->aspect)/2) * WIN_HEIGHT;
    dstrect.w = (combatUIManager->dodgePanel->width - padding) * WIN_WIDTH;
    dstrect.h = (combatUIManager->dodgePanel->height - (padding*combatUIManager->aspect))* WIN_HEIGHT;
    SDL_RenderCopy(renderer, combatUIManager->rendertarget, NULL, &dstrect);
  }

  if(g_submode== submode::FORGET) {
    combatUIManager->dialogProceedIndicator->show = 0;
    combatUIManager->forgetPanel->render(renderer, g_camera, elapsed);
    float y = 0.275;
    float yDelta = 0.075;
    float x = 0.25;

    float px = 0.23;
    float pxoffset = -0.009; //slightly negative
    float pyoffset = 0.006;

    combatUIManager->forgetText->updateText(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[0]].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    combatUIManager->forgetPicker->show = 1;

    if(combatUIManager->forgetOption == 0) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    y+= yDelta;

    combatUIManager->forgetText->updateText(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[1]].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);

    if(combatUIManager->forgetOption == 1) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    y+= yDelta;

    combatUIManager->forgetText->updateText(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[2]].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    if(combatUIManager->forgetOption == 2) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    y+= yDelta;

    combatUIManager->forgetText->updateText(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[3]].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    if(combatUIManager->forgetOption == 3) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    y+= yDelta;

    combatUIManager->forgetText->updateText(spiritTable[combatUIManager->moveToLearn].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    if(combatUIManager->forgetOption == 4) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    combatUIManager->forgetInfoPanel->render(renderer, g_camera, elapsed);
    
    if(combatUIManager->forgetOption <= 3) {
      string info = getLanguageData("SI" + to_string(g_partyCombatants[g_whoLearnsMove]->spiritMoves[combatUIManager->forgetOption]));
      while(replaceString(info, "\\n", "\n")){}
      string name = getLanguageData("S" + to_string(g_partyCombatants[g_whoLearnsMove]->spiritMoves[combatUIManager->forgetOption]));
      string final = name + "\n" + info;

      combatUIManager->forgetInfoText->updateText(final, -1, 0.43, g_textcolor, g_font);
    } else {
      string info = getLanguageData("SI" + to_string(combatUIManager->moveToLearn));
      while(replaceString(info, "\\n", "\n")){}
      string name = getLanguageData("S" + to_string(combatUIManager->moveToLearn));
      string final = name + "\n" + info;

      combatUIManager->forgetInfoText->updateText(final, -1, 0.43, g_textcolor, g_font );
    }
    combatUIManager->forgetInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);



  }

  if(g_submode == submode::FORGETCONFIRM) {
    combatUIManager->yes->show = 1;
    combatUIManager->yes->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    combatUIManager->no->show = 1;
    combatUIManager->no->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    combatUIManager->confirmPicker->show = 1;

    if(combatUIManager->confirmOption == 0) {
      combatUIManager->confirmPicker->x = combatUIManager->yes->boxX - 0.08;
      combatUIManager->confirmPicker->y = combatUIManager->yes->boxY + 0.01;
    } else {
      combatUIManager->confirmPicker->x = combatUIManager->no->boxX - 0.07;
      combatUIManager->confirmPicker->y = combatUIManager->no->boxY + 0.01;

    }
    combatUIManager->confirmPicker->render(renderer, g_camera, elapsed);

  }

  SDL_RenderPresent(renderer);

}

//this is for learning moves in exploration gamemode
void learnMoveLoop() {
  getCombatInput();

  //drawCombatants();
  switch (g_submode) {
    case submode::LEVELUP:
      {
        if(g_partyCombatants[g_whoLearnsMove]->spiritMoves.size() < 4) {
          //just learn it
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          combatUIManager->finalText = stringMultiInject(getLanguageData("LearnedMoveSuccess"), {g_partyCombatants[g_whoLearnsMove]->name, spiritTable[combatUIManager->moveToLearn].name});
          g_partyCombatants[g_whoLearnsMove]->spiritMoves.push_back(combatUIManager->moveToLearn);
          g_items.erase(g_items.begin() + combatUIManager->currentInventoryOption);

          g_submode = submode::LEARNEDTEXT;
          break;
        } else {
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->finalText = stringMultiInject(getLanguageData("LearnedMovePossible"), {g_partyCombatants[g_whoLearnsMove]->name, spiritTable[combatUIManager->moveToLearn].name});
          combatUIManager->queuedStrings.push_back(make_pair(stringMultiInject(getLanguageData("LearnedMovePossible2"), {g_partyCombatants[g_whoLearnsMove]->name}),(combatant*)0));
          g_submode = submode::LEARNTEXT;
          break;
        }

        break;
      }
    case submode::LEVELTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {

              if(g_partyCombatants[g_whoLearnsMove]->spiritMoves.size() < 4) {
                //just learn it 
                combatUIManager->currentText = "";
                combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
                combatUIManager->finalText = stringMultiInject(getLanguageData("LearnedMoveSuccess"), {g_partyCombatants[g_whoLearnsMove]->name, spiritTable[combatUIManager->moveToLearn].name});
                g_partyCombatants[g_whoLearnsMove]->spiritMoves.push_back(combatUIManager->moveToLearn);
                g_items.erase(g_items.begin() + combatUIManager->currentInventoryOption);

                g_submode = submode::LEARNEDTEXT;
                break;



              } else {
                combatUIManager->currentText = "";
                combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
                combatUIManager->finalText = stringMultiInject(getLanguageData("LearnedMovePossible"), {g_partyCombatants[g_whoLearnsMove]->name, spiritTable[combatUIManager->moveToLearn].name});
                combatUIManager->queuedStrings.push_back(make_pair(stringMultiInject(getLanguageData("LearnedMovePossible2"), {g_partyCombatants[g_whoLearnsMove]->name}),(combatant*)0));
                g_submode = submode::LEARNTEXT;
                break;
              }
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::LEARNEDTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {


                //go back to the dialog system in exploration mode
                g_learningMove = 0;
                combatUIManager->hideAll();

                //this is because the player now learns from disks in the item menu
                combatUIManager->inventoryPanel->show = 1;
                combatUIManager->inventoryText->show = 1;
                combatUIManager->currentInventoryOption = 0;
                combatUIManager->menuPicker->show = 1;
                combatUIManager->menuPicker->x = 10;
    
                adventureUIManager->dialogue_index++;
                adventureUIManager->continueDialogue();
                return;

              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;
          }
        }

        break;
      }
    case submode::LEARNTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              g_submode = submode::FORGET;

              combatUIManager->forgetPanel->show = 1;
              combatUIManager->forgetInfoPanel->show = 1;
              combatUIManager->forgetText->show = 1;
              combatUIManager->forgetInfoText->show = 1;
              combatUIManager->forgetOption = 0;
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::FORGET:
      {
        if(input[0] && !oldinput[0]) {
          if(combatUIManager->forgetOption > 0) {
            combatUIManager->forgetOption -= 1;
          }
        }

        if(input[1] && !oldinput[1]) {
          if(combatUIManager->forgetOption < 4) {
            combatUIManager->forgetOption += 1;
          }
        }
        if(input[11] && !oldinput[11]) {
          if(combatUIManager->forgetOption < 4) {
            combatUIManager->finalText = stringMultiInject(getLanguageData("ForgetMoveConfirm"), {g_partyCombatants[g_whoLearnsMove]->name, spiritTable[g_partyCombatants[g_whoLearnsMove]->spiritMoves[combatUIManager->forgetOption]].name, spiritTable[combatUIManager->moveToLearn].name});
            //combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " will forget " + spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->forgetOption]].name + " and learn " + spiritTable[combatUIManager->moveToLearn].name + ".";
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            g_submode = submode::FORGETTEXT;

            combatUIManager->forgetPanel->show = 0;
            combatUIManager->forgetInfoPanel->show = 0;
            combatUIManager->forgetText->show = 0;
            combatUIManager->forgetInfoText->show = 0;
            combatUIManager->forgetPicker->show = 0;

            break;
          } else {
            combatUIManager->finalText = stringMultiInject(getLanguageData("WontLearnMoveConfirm"), {g_partyCombatants[g_whoLearnsMove]->name, spiritTable[combatUIManager->moveToLearn].name});
            //combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " will not learn " + spiritTable[combatUIManager->moveToLearn].name + ".";
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            g_submode = submode::FORGETTEXT;

            combatUIManager->forgetPanel->show = 0;
            combatUIManager->forgetInfoPanel->show = 0;
            combatUIManager->forgetText->show = 0;
            combatUIManager->forgetInfoText->show = 0;
            combatUIManager->forgetPicker->show = 0;

            break;

          }
          break;
        }

        break;
      }
    case submode::FORGETTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if((input[11] && !oldinput[11]) || 1) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              g_submode = submode::FORGETCONFIRM;
              combatUIManager->confirmOption = 0;
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::FORGETCONFIRM:
      {
        combatUIManager->dialogProceedIndicator->show = 0;

        if(input[2] && !oldinput[2]) {
          combatUIManager->confirmOption = 0;
        }
        if(input[3] && !oldinput[3]) {
          combatUIManager->confirmOption = 1;
        }

        if(input[11] && !oldinput[11]) {
          if(combatUIManager->confirmOption == 0) {
            if(combatUIManager->forgetOption == 4) {
              //the user chose to not learn a move
              
              g_learningMove = 0;
              combatUIManager->hideAll();

              //this is because the player now learns from disks in the item menu
              combatUIManager->inventoryPanel->show = 1;
              combatUIManager->inventoryText->show = 1;
              combatUIManager->currentInventoryOption = 0;
              combatUIManager->menuPicker->show = 1;
              combatUIManager->menuPicker->x = 10;
    
              adventureUIManager->dialogue_index++;
              adventureUIManager->continueDialogue();
              return;
            }
            g_partyCombatants[g_whoLearnsMove]->spiritMoves[combatUIManager->forgetOption] = combatUIManager->moveToLearn;
            g_items.erase(g_items.begin() + combatUIManager->currentInventoryOption);

            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            combatUIManager->finalText = stringMultiInject(getLanguageData("LearnedMoveSuccess"), {g_partyCombatants[g_whoLearnsMove]->name, spiritTable[combatUIManager->moveToLearn].name});

            g_submode = submode::LEARNEDTEXT;

            break;
            
          } else {
            combatUIManager->mainText->updateText(stringMultiInject(getLanguageData("LearnedMovePossible2"), {g_partyCombatants[g_whoLearnsMove]->name}), -1, 0.85, g_textcolor, g_font);
            g_submode = submode::FORGET;

            combatUIManager->forgetPanel->show = 1;
            combatUIManager->forgetInfoPanel->show = 1;
            combatUIManager->forgetText->show = 1;
            combatUIManager->forgetInfoText->show = 1;
            combatUIManager->forgetPicker->show = 1;
            combatUIManager->forgetOption = 0;
          }
        }

        break;
      }
  }


  combatUIManager->mainPanel->render(renderer, g_camera, elapsed);
  combatUIManager->mainText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
  combatUIManager->dialogProceedIndicator->render(renderer, g_camera, elapsed);

  if(g_submode== submode::FORGET) {
    combatUIManager->dialogProceedIndicator->show = 0;
    combatUIManager->forgetPanel->render(renderer, g_camera, elapsed);
    float y = 0.275;
    float yDelta = 0.075;
    float x = 0.25;

    float px = 0.23;
    float pxoffset = -0.009; //slightly negative
    float pyoffset = 0.006;

    combatUIManager->forgetText->updateText(spiritTable[g_partyCombatants[g_whoLearnsMove]->spiritMoves[0]].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    combatUIManager->forgetPicker->show = 1;

    if(combatUIManager->forgetOption == 0) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    y+= yDelta;

    combatUIManager->forgetText->updateText(spiritTable[g_partyCombatants[g_whoLearnsMove]->spiritMoves[1]].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);

    if(combatUIManager->forgetOption == 1) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    y+= yDelta;

    combatUIManager->forgetText->updateText(spiritTable[g_partyCombatants[g_whoLearnsMove]->spiritMoves[2]].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    if(combatUIManager->forgetOption == 2) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    y+= yDelta;

    combatUIManager->forgetText->updateText(spiritTable[g_partyCombatants[g_whoLearnsMove]->spiritMoves[3]].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    if(combatUIManager->forgetOption == 3) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    y+= yDelta;

    combatUIManager->forgetText->updateText(spiritTable[combatUIManager->moveToLearn].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    if(combatUIManager->forgetOption == 4) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    combatUIManager->forgetInfoPanel->render(renderer, g_camera, elapsed);
    
    if(combatUIManager->forgetOption <= 3) {
      string info = getLanguageData("SI" + to_string(g_partyCombatants[g_whoLearnsMove]->spiritMoves[combatUIManager->forgetOption]));
      while(replaceString(info, "\\n", "\n")){}
      string name = getLanguageData("S" + to_string(g_partyCombatants[g_whoLearnsMove]->spiritMoves[combatUIManager->forgetOption]));
      string final = name + "\n" + info;

      combatUIManager->forgetInfoText->updateText(final, -1, 0.43, g_textcolor, g_font);
    } else {
      string info = getLanguageData("SI" + to_string(combatUIManager->moveToLearn));
      while(replaceString(info, "\\n", "\n")){}
      string name = getLanguageData("S" + to_string(combatUIManager->moveToLearn));
      string final = name + "\n" + info;

      combatUIManager->forgetInfoText->updateText(final, -1, 0.43, g_textcolor, g_font );
    }
    combatUIManager->forgetInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);




  }

  if(g_submode == submode::FORGETCONFIRM) {
    combatUIManager->yes->show = 1;
    combatUIManager->yes->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    combatUIManager->no->show = 1;
    combatUIManager->no->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    combatUIManager->confirmPicker->show = 1;

    if(combatUIManager->confirmOption == 0) {
      combatUIManager->confirmPicker->x = combatUIManager->yes->boxX - 0.08;
      combatUIManager->confirmPicker->y = combatUIManager->yes->boxY + 0.01;
    } else {
      combatUIManager->confirmPicker->x = combatUIManager->no->boxX - 0.07;
      combatUIManager->confirmPicker->y = combatUIManager->no->boxY + 0.01;

    }
    combatUIManager->confirmPicker->render(renderer, g_camera, elapsed);

  }

  SDL_RenderPresent(renderer);
}

miniEnt::miniEnt() {
  g_miniEnts.push_back(this);
}

miniEnt::~miniEnt() {
  g_miniEnts.erase(remove(g_miniEnts.begin(), g_miniEnts.end(), this), g_miniEnts.end());
}

void miniEnt::update(float elapsed) {

  if(sleepMS > 0) {
    sleepMS -=elapsed;
    return;
  }

  // Calculate new position based on velocity
  float deltaX = velocity * cos(angle) * elapsed;
  float deltaY = velocity * sin(angle) * elapsed;
  x += deltaX;
  y += deltaY;

  gravityVX += gravityAccelX * elapsed;
  x += gravityVX;
  gravityVY += gravityAccelY * elapsed;
  y += gravityVY;

  // If spinSpeed is non-zero, apply the spinning motion
  if (spinSpeed != 0) {
    spinAngle += spinSpeed * elapsed;

    // Move the bullet along the perpendicular vector
    x = centerX + radius * cos(spinAngle);
    y = centerY + radius * sin(spinAngle);

    radius -= velocity * elapsed;
  }

  if(!isInPlayArea && x -w/2>= 0 && x + w/2 <= SCREEN_WIDTH && y -h/2 >= 0 && y +h/2 <= SCREEN_WIDTH) {
    isInPlayArea = 1;
  }
  if (isInPlayArea && canBounce) {
    if (x < w / 2 || x > SCREEN_WIDTH - w / 2) {
      angle = M_PI - angle; // Reflect horizontally
      if (x < w / 2) { 
        x = w / 2;
        gravityVX = -gravityVX;

      }
      if (x > SCREEN_WIDTH - w / 2) {
        x = SCREEN_WIDTH - w / 2;
        gravityVX = -gravityVX;
      }
    }
    if (y < w / 2 || y > SCREEN_HEIGHT - w / 2) {
      angle = -angle; // Reflect vertically
      if (y < w / 2)  {
        y = w / 2;
        gravityVY = -gravityVY;
      }
      if (y > SCREEN_HEIGHT - w / 2) {
        y = SCREEN_HEIGHT - w / 2;
        gravityVY = -gravityVY;
      }
    }
  }
}

void miniEnt::render(int shadow) {
  SDL_Rect drect = {
    (int)x - w/2,
    (int)y - h/2,
    (int)w,
    (int)h
  };
//  red = 255;
//  blue = 255;
//  green = 255;
  red = 155;
  blue = 115;
  green = 115;
  if(shadow) {
    SDL_SetTextureColorMod(texture, red*0.7, blue*0.7, green*0.7);
    SDL_RenderCopy(renderer, texture, NULL, &drect);
  } else {
    drect.x += 7;
    drect.y += 7;
    drect.w -= 14;
    drect.h -= 14;
    SDL_SetTextureColorMod(texture, red, blue, green);
    SDL_RenderCopy(renderer, texture, NULL, &drect);
  }
}

miniBullet::miniBullet(float f_angle, float f_velocity) {
  angle = f_angle;
  velocity = f_velocity;
  x = 512.0f + SPAWN_MARGIN * cos(angle);
  y = 512.0f + SPAWN_MARGIN * sin(angle);
  g_miniBullets.push_back(this);
}

miniBullet::miniBullet() {
  float spawnX, spawnY;
  float targetX, targetY;
  targetX = rng(0, SCREEN_WIDTH);
  targetY = rng(0, SCREEN_HEIGHT);
  int side = rand() % 4;
  switch(side) {
    case 0:
      spawnX = -SPAWN_MARGIN;
      spawnY = rng(0, SCREEN_HEIGHT);
      break;
    case 1:
      spawnX = SCREEN_WIDTH + SPAWN_MARGIN;
      spawnY = rng(0, SCREEN_HEIGHT);
      break;
    case 2:
      spawnX = rng(0, SCREEN_WIDTH);
      spawnY = -SPAWN_MARGIN;
      break;
    case 3:
      spawnX = rng(0, SCREEN_WIDTH);
      spawnY = SCREEN_HEIGHT + SPAWN_MARGIN;
      break;
  }
  x = spawnX;
  y = spawnY;
  w = 100;
  h = 100;
  angle = atan2(targetY - spawnY, targetX - spawnX);
  velocity = 0.3;
  g_miniBullets.push_back(this);
}

miniBullet::~miniBullet() {
  g_miniBullets.erase(remove(g_miniBullets.begin(), g_miniBullets.end(), this), g_miniBullets.end());
}

void miniBullet::explode(int numFragments, int exploding, float fragSize) {
  // Create smaller bullets upon explosion

  float baseAngle = 0;
  if(randomExplodeAngle) {
    baseAngle = frng(0, M_PI * 2);
  }
  for (int i = 0; i < numFragments; i++) {
    miniBullet* fragment = new miniBullet();
    fragment->x = x;
    fragment->y = y;
    fragment->angle = 2 * M_PI / numFragments * i;
    fragment->angle += baseAngle;
    if(completelyRandomExplodeAngle) {
      fragment->angle = frng(0, 2*M_PI);
      fragment->completelyRandomExplodeAngle = 1;
    }
    fragment->canBounce = canBounce;
    fragment->velocity = 0.3; // Set velocity of fragments
    fragment->texture = texture;
    fragment->red = this->red;
    fragment->green = this->green;
    fragment->blue = this->blue;
    fragment->texture = this->texture;
    fragment->exploding = exploding;
    fragment->explosionTimer = 1000;
    fragment->w = this->w * fragSize;
    fragment->h = fragment->w;
    fragment->numFragments = numFragments;
    fragment->fragSize = fragSize;
    fragment->randomExplodeAngle = randomExplodeAngle;
    if(exploding == 0) {
      fragment->canBounce = 0;
    }
  }
}

void miniBullet::bulletUpdate(float elapsed) {
  if(homing) {
    float dx = combatUIManager->dodgerX - x;
    float dy = combatUIManager->dodgerY - y;
    float targetAngle = atan2(dy, dx);
    float angleDifference = targetAngle - angle;
    if(angleDifference > M_PI) angleDifference -= 2 * M_PI;
    if(angleDifference < -M_PI) angleDifference += 2 * M_PI;
    angle += angleDifference * 0.001f * elapsed;
  }
  if (exploding) {
    explosionTimer -= elapsed;
    if (explosionTimer <= 0 && exploded == 0) {
      explode(numFragments, exploding - 1, fragSize);
      exploded = 1;
    }
  }
}
