#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <math.h>
#include <stdio.h>
#include <string>
#include <limits>
#include <stdlib.h>

#include "globals.h"
#include "objects.h"

void specialObjectsInit(entity* a) {
  switch(a->identity) {
    case 1:
    {
      //pellet
      g_pellets.push_back(a);
      a->bounceindex = rand() % 8;
      a->wasPellet = 1;
      a->CalcDynamicForOneFrame = 1;

      break;
    }
    case 2: 
    {
      //spiketrap
      for(auto entry:a->spawnlist) {
        entry->visible = 0;
      }
      a->maxCooldownA = 1300;
      a->maxCooldownB = 1300;
      break;
    }
    case 3:
    {
      //cannon
      for(auto entry:a->spawnlist) {
        entry->visible = 0;
      }
      a->maxCooldownA = 1300;
      break;
    }
    case 6:
    {
      //smarttrap
      a->isAI = 1;
      a->poiIndex = 6;
      a->readyForNextTravelInstruction = 1;
      break;
    }
    case 8:
    {
      //psychotrap
      a->parent->visible = 0;
      for(auto entry : a->spawnlist) {
        entry->visible = 1;
        entry->msPerFrame = 2;
        entry->loopAnimation = 1;
        entry->scriptedAnimation = 1;
      }
      break;
    }
    case 100:
    {
      //zombie
      a->spawnlist[0]->visible = 0;
      break;
    }
  }
}

void specialObjectsBump(entity* a, bool xcollide, bool ycollide) {
  switch(a->identity) {
    case 5:
    {
      //bladetrap
      if(xcollide || ycollide) {
        if(a->flagB == a->flagA) {
          playSoundAtPosition(7, g_bladetrapSound, 0, a->getOriginX(), a->getOriginY(), 0.5);
          float offset = 50;
          if(!a->flagA) {
            offset = -50;
          }
          float xoff = offset * cos(a->steeringAngle);
          float yoff = -offset * sin(a->steeringAngle);
          sparksEffect->happen(a->getOriginX() + xoff, a->getOriginY() + yoff, a->z + 25, 0);
          g_lastParticleCreated->sortingOffset = 50;
        }
        if(a->cooldownA > a->maxCooldownA) {
          a->cooldownA = 0;
          a->flagA = !a->flagA;
        }

      }
      break;
    }
  }
  
}

void specialObjectsUpdate(entity* a, float elapsed) {
  if(a->identity == 2) {
    //spiketrap
    if(a->flagA == 0) {
      if(RectOverlap3d(a->getMovedBounds(), protag->getMovedBounds())) {
        a->flagA = 1;
        a->cooldownA = 0;
      }
    }
    if(a->flagA == 1) {
      //wait a second, and then extend
      a->cooldownA += elapsed;
      if(a->cooldownA > a->maxCooldownA) {
        a->cooldownB = 0;
        //show spikes
        for(auto entry : a->spawnlist) {
          entry->frameInAnimation = 0;
          entry->loopAnimation = 0;
          entry->msPerFrame = 50;
          entry->scriptedAnimation = 1;
          entry->reverseAnimation = 0;
          entry->visible = 1;
        }
        playSound(5, g_spiketrapSound, 0);
        a->flagA = 2;
        a->flagB = 0;
        a->cooldownA = 0;
      }

    }
    if(a->flagA == 2) {
      if(!a->flagB) {
        rect changeMe = a->getMovedBounds();
        changeMe.zeight = 32;
        if(RectOverlap3d(changeMe, protag->getMovedBounds())) {
          protag->hp -= a->maxhp;
          protag->flashingMS = g_flashtime;
          playSound(2, g_playerdamage, 0);
          a->flagB = 1;
        }
      }

      a->cooldownA += elapsed;
      if(a->cooldownA > a->maxCooldownB) {
        a->cooldownB = 0;
        //hide spikes
        for(auto entry : a->spawnlist) {
          entry->frameInAnimation = 1;
          entry->loopAnimation = 0;
          entry->msPerFrame = 50;
          entry->scriptedAnimation = 1;
          entry->reverseAnimation = 1;
        }
        a->flagA = 3;
        a->cooldownA = 0;
      }

    }
    if(a->flagA == 3) {
      a->cooldownA += elapsed;
      if(a->cooldownA > 125) {
        for(auto entry : a->spawnlist) {
          entry->visible = 0;
        }
        a->flagA = 0;
        a->cooldownA = 0;
      }

    }
     
  } else if (a->identity == 3) {
    //cannon
    a->cooldownA += elapsed;
    if(a->cooldownA > a->maxCooldownA) {
      entity *copy = new entity(renderer, a->spawnlist[0]);
      copy->z = a->z + 10;
      copy->dontSave = 1;
      copy->usingTimeToLive = 1;
      copy->timeToLiveMs = 3000;
      copy->steeringAngle = a->steeringAngle;
      copy->targetSteeringAngle = a->steeringAngle;
      copy->missile = 1;
      copy->visible = 1;
      copy->fragileMovement = 1;
      copy->msPerFrame = 70;
      copy->loopAnimation = 1;
      copy->useGravity = 0;
      copy->identity = 4;
      a->cooldownA = 0;
       
      float offset = 50;
      float yoff = -offset * sin(a->steeringAngle);
      float xoff = offset * cos(a->steeringAngle);

      copy->setOriginX(a->getOriginX() + xoff);
      copy->setOriginY(a->getOriginY() + yoff);
      
  
      blackSmokeEffect->happen(a->getOriginX() + xoff, a->getOriginY() + yoff, a->z + 40, a->steeringAngle);
      playSoundAtPosition(6, g_cannonfireSound, 0, a->getOriginX(), a->getOriginY(), 0.6);
    }

  } else if(a->identity == 4) {
    //cannonball
    if(RectOverlap3d(a->getMovedBounds(), protag->getMovedBounds())) {
      a->timeToLiveMs = -1;
      protag->hp -= a->hp;
      protag->flashingMS = g_flashtime;
      playSound(2, g_playerdamage, 0);
    }
  } else if(a->identity == 5) {
    //bladetrap
    a->cooldownA += elapsed;
    a->cooldownB += elapsed;
    if(CylinderOverlap(a->getMovedBounds(), protag->getMovedBounds()) && a->cooldownA > a->maxCooldownB)
    {
      a->cooldownA = 0;
      protag->hp -= a->hp;
      protag->flashingMS = g_flashtime;
      playSound(2, g_playerdamage, 0);

    }
//    if(flagA) {
//      forwardsVelocity = xagil;
//    } else {
//      forwardsVelocity = -xagil;
//    }
//    if(devMode) {
//      forwardsVelocity = 0;
//    }

    
  } else if(a->identity == 6) {
    //smarttrap

    a->cooldownA += elapsed;
    a->cooldownB += elapsed;
    if(CylinderOverlap(a->getMovedBounds(), protag->getMovedBounds()) && a->cooldownA > a->maxCooldownA)
    {
      a->cooldownA = 0;
      protag->hp -= a->hp;
      protag->flashingMS = g_flashtime;
      playSound(2, g_playerdamage, 0);


    }

    if(a->readyForNextTravelInstruction) {
      //playSoundAtPosition(7, g_smarttrapSound, 0, getOriginX(), getOriginY(), 0.5);
    }
  } else if(a->identity == 7) {
    //facetrap
    float dist = XYWorldDistanceSquared(a->getOriginX(), a->getOriginY(), protag->getOriginX(), protag->getOriginY());
    const float Dist = 16384;
    const float minDist = -2000;
    if(dist > Dist) {
      a->opacity = 0;

    } else {
      a->opacity = 255.0 * (1 -((dist + minDist) / (Dist +minDist)));
      if(a->opacity > 255) {a->opacity = 255;}
      //SDL_SetTextureAlphaMod(texture, a-opacity);
    }
  } else if(a->identity == 8) {
    a->cooldownB -= elapsed;
    //psychotrap
    if(CylinderOverlap(protag->getMovedBounds(), a->getMovedBounds())) {
      //show graphic
      for(auto entry : a->spawnlist) {
        entry->visible = 1;
      }

      if(CylinderOverlap(a->parent->getMovedBounds(), protag->getMovedBounds()) && a->cooldownB <= 0) {
        protag->hp -= a->hp;
        protag->flashingMS = g_flashtime;
        playSound(2, g_playerdamage, 0);
        a->cooldownB = 1300;
      }

    } else {
      //hide graphic
      for(auto entry : a->spawnlist) {
        entry->visible = 0;
      }
    }
  } else if(a->identity == 100) {
    //zombie
    
    a->cooldownA += elapsed;
    if(a->cooldownA > a->maxCooldownA) {
      a->cooldownA = 0;
      a->flagA = 0;
    }

    if(a->flagA) {
      a->spawnlist[0]->visible = 1;
    } else {
      a->spawnlist[0]->visible = 0;
    }


    for(int i = 0; i < a->myAbilities.size(); i++) {
      if(a->myAbilities[i].ready) {
        a->myAbilities[i].ready = 0;
        a->myAbilities[i].cooldownMS = rng(a->myAbilities[i].lowerCooldownBound, a->myAbilities[i].upperCooldownBound);
        switch (i) {
        case 0:
          {
            //attack
            hitbox* h = new hitbox();
  
            float offset = 50;
            float yoff = -offset * sin(a->steeringAngle);
            float xoff = offset * cos(a->steeringAngle);
  
            h->x = a->getOriginX() + xoff - 64;
            h->y = a->getOriginY() + yoff - 64;
            h->z = a->z;
  
            h->bounds.x = 0;
            h->bounds.y = 0;
            h->bounds.z = 20;
            h->bounds.width = 128;
            h->bounds.height = 128;
            h->bounds.zeight = 128;
  
            h->activeMS = 2000;
            h->sleepingMS = 100;

            a->cooldownA = 0;
            a->flagA = 1;
  
            break;
          }
        case 1:
          {
            //updatestate
            break;
          }
        case 2:
          {
            //update aggressiveness
  
            break;
          }
        }
        
      }
    }

  }
}

