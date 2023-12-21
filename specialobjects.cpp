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

void specialObjectsUpdate(entity* a, float elapsed) {
  if(a->identity == 2) {
    //spiketrap
    if(a->spikeState == 0) {
      if(RectOverlap3d(a->getMovedBounds(), protag->getMovedBounds())) {
        a->spikeState = 1;
        a->spikeWaitMS = 0;
      }
    }
    if(a->spikeState == 1) {
      //wait a second, and then extend
      a->spikeWaitMS += elapsed;
      if(a->spikeWaitMS > a->maxSpikeWaitMS) {
        a->spikeActiveMS = 0;
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
        a->spikeState = 2;
        a->spikedPlayer = 0;
        a->spikeWaitMS = 0;
      }

    }
    if(a->spikeState == 2) {
      if(!a->spikedPlayer) {
        rect changeMe = a->getMovedBounds();
        changeMe.zeight = 32;
        if(RectOverlap3d(changeMe, protag->getMovedBounds())) {
          protag->hp -= a->maxhp; //blegh
          protag->flashingMS = g_flashtime;
          playSound(2, g_playerdamage, 0);
          a->spikedPlayer = 1;
        }
      }

      a->spikeWaitMS += elapsed;
      if(a->spikeWaitMS > a->maxSpikeActiveMS) {
        a->spikeActiveMS = 0;
        //hide spikes
        for(auto entry : a->spawnlist) {
          entry->frameInAnimation = 1;
          entry->loopAnimation = 0;
          entry->msPerFrame = 50;
          entry->scriptedAnimation = 1;
          entry->reverseAnimation = 1;
        }
        a->spikeState = 3;
        a->spikeWaitMS = 0;
      }

    }
    if(a->spikeState == 3) {
      a->spikeWaitMS += elapsed;
      if(a->spikeWaitMS > 125) {
        for(auto entry : a->spawnlist) {
          entry->visible = 0;
        }
        a->spikeState = 0;
        a->spikeWaitMS = 0;
      }

    }
     
  } else if (a->identity == 3) {
    //cannon
    a->spikeWaitMS += elapsed;
    if(a->spikeWaitMS > a->maxSpikeWaitMS) {
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
      a->spikeWaitMS = 0;
       
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
      protag->hp -= a->maxhp;
      protag->flashingMS = g_flashtime;
      playSound(2, g_playerdamage, 0);
    }
  } else if(a->identity == 5) {
    //bladetrap
    a->spikeWaitMS += elapsed;
    a->spikeActiveMS+=elapsed;
    if(CylinderOverlap(a->getMovedBounds(), protag->getMovedBounds()) && a->spikeWaitMS > a->maxSpikeWaitMS)
    {
      a->spikeWaitMS = 0;
      protag->hp -= a->maxhp;
      protag->flashingMS = g_flashtime;
      playSound(2, g_playerdamage, 0);

    }
  } else if(a->identity == 6) {
    //smarttrap

    a->spikeWaitMS += elapsed;
    a->spikeActiveMS += elapsed;
    if(CylinderOverlap(a->getMovedBounds(), protag->getMovedBounds()) && a->spikeWaitMS > a->maxSpikeWaitMS)
    {
      a->spikeWaitMS = 0;
      protag->hp -= a->maxhp;
      protag->flashingMS = g_flashtime;
      playSound(2, g_playerdamage, 0);


    }

    if(a->readyForNextTravelInstruction) {
      //playSoundAtPosition(7, g_smarttrapSound, 0, getOriginX(), getOriginY(), 0.5);
    }
  } else if(a->identity == 7) {
    //facetrap
    float dist = XYWorldDistanceSquared(a->getOriginX(), a->getOriginY(), protag->getOriginX(), protag->getOriginY());
    const float maxDist = 16384;
    const float minDist = -2000;
    if(dist > maxDist) {
      a->opacity = 0;

    } else {
      a->opacity = 255.0 * (1 -((dist + minDist) / (maxDist +minDist)));
      if(a->opacity > 255) {a->opacity = 255;}
      //SDL_SetTextureAlphaMod(texture, a-opacity);
    }
  } else if(a->identity == 8) {
    a->spikeActiveMS -= elapsed;
    //psychotrap
    if(CylinderOverlap(protag->getMovedBounds(), a->getMovedBounds())) {
      //show graphic
      for(auto entry : a->spawnlist) {
        entry->visible = 1;
      }

      if(CylinderOverlap(a->parent->getMovedBounds(), protag->getMovedBounds()) && a->spikeActiveMS <= 0) {
        protag->hp -= a->maxhp;
        protag->flashingMS = g_flashtime;
        playSound(2, g_playerdamage, 0);
        a->spikeActiveMS = a->maxSpikeActiveMS;
      }

    } else {
      //hide graphic
      for(auto entry : a->spawnlist) {
        entry->visible = 0;
      }
    }
  } else if(a->identity == 100) {
    //zombie
    for(int i = 0; i < a->myAbilities.size(); i++) {
      if(a->myAbilities[i].ready) {
        //M("Used " + a->myAbilities[i].name);
        //D(a->myAbilities[i].cooldownMS);
        a->myAbilities[i].ready = 0;
        a->myAbilities[i].cooldownMS = rng(a->myAbilities[i].lowerCooldownBound, a->myAbilities[i].upperCooldownBound);
        switch i:
        case 0:
          //attack

          break;
        case 1;
          //updatestate

          break;
        case 2;
          //update aggressiveness

          break;
        
      }
    }

  }
}

