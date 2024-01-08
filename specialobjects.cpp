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
#include "specialobjects.h"

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
    case 9:
    {
      //inventory chest

      break;
    }
    case 10:
    {
      //corkboard

      break;
    }
    case 11:
    {
      //lever
      
      //search for door
      for(auto x: g_entities) {
        if(x->identity == 12) {
          char b = x->name.at(x->name.size() - 1);
          char c = a->name.at(a->name.size() - 1);
          if(c == b) {
            a->spawnlist.push_back(x);
            M("Lever found door");
          }
        }

      }
      break;
    }
    case 12:
    {
      //door
      
      //search for lever
      for(auto x: g_entities) {
        if(x->identity == 11) {
          char c = x->name.at(x->name.size() - 1);
          char b = a->name.at(a->name.size() - 1);
          if(c == b) {
            x->spawnlist.push_back(a);
            M("Door found lever");
          }
        }

      }

      break;
    }
    case 13:
    {
      //braintrap
      a->spawnlist[0]->visible = 0;
      ribbon* zap = new ribbon();
      zap->texture = a->spawnlist[0]->texture;
      zap->sortingOffset = 500;
      a->actorlist.push_back(zap);

      break;
    }
    case 100:
    {
      //zombie
      a->spawnlist[0]->visible = 0;
      a->maxCooldownA = 300; //this is the time it takes to get from the start to the end of the bite animation
      a->aggressiveness = exponentialCurve(60, 10);
      a->aggressiveness += exponentialCurve(15, 3);
      a->poiIndex = 0;

      break;
    }
    case 101:
    {
      //disaster
      a->agrod = 1;
      a->target = protag;
      a->traveling = 0;
      a->spawnlist[0]->visible = 0;
      a->aggressiveness = exponentialCurve(60, 10);
      a->poiIndex = 1;
      break;
    }
    case 102:
    {
      //creep
      a->aggressiveness = exponentialCurve(60, 10);
      a->spawnlist[0]->visible = 0;
      a->activeState = 0;
      a->poiIndex = 2;
      break;

    }
    case 103:
    {
      //fnomunon
      a->traveling = 1;
      a->poiIndex = 3;
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
        playSoundAtPosition(7, g_bladetrapSound, 0, a->getOriginX(), a->getOriginY(), 0.5);
        float offset = 50;
        if(!a->flagA) {
          offset = -50;
        }
        float xoff = offset * cos(a->steeringAngle);
        float yoff = -offset * sin(a->steeringAngle);
        sparksEffect->happen(a->getOriginX() + xoff, a->getOriginY() + yoff, a->z + 25, 0);
        g_lastParticleCreated->sortingOffset = 50;
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
  switch(a->identity) {
    case 2: 
    {
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
            hurtProtag(1);
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
      break;
    } 
    case 3: 
    {
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
      break;
    } 
    case 4: 
    {
      //cannonball
      if(RectOverlap3d(a->getMovedBounds(), protag->getMovedBounds())) {
        a->timeToLiveMs = -1;
        hurtProtag(1);
      }
    } 
    case 5: 
    {
      //bladetrap
      a->cooldownA += elapsed;
      a->cooldownB += elapsed;
      if(CylinderOverlap(a->getMovedBounds(), protag->getMovedBounds()) && a->cooldownA > a->maxCooldownB)
      {
        a->cooldownA = 0;
        hurtProtag(1);
    
      }
      break;
    } 
    case 6: 
    {
      //smarttrap
    
      a->cooldownA += elapsed;
      a->cooldownB += elapsed;
      if(CylinderOverlap(a->getMovedBounds(), protag->getMovedBounds()) && a->cooldownA > a->maxCooldownA)
      {
        a->cooldownA = 0;
        hurtProtag(1);
    
    
      }
    
      if(a->readyForNextTravelInstruction) {
        //playSoundAtPosition(7, g_smarttrapSound, 0, getOriginX(), getOriginY(), 0.5);
      }
      break;
    } 
    case 7: 
    {
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
      break;
    } 
    case 8: 
    {
      a->cooldownB -= elapsed;
      //psychotrap
      if(CylinderOverlap(protag->getMovedBounds(), a->getMovedBounds())) {
        //show graphic
        for(auto entry : a->spawnlist) {
          entry->visible = 1;
        }
    
        if(CylinderOverlap(a->parent->getMovedBounds(), protag->getMovedBounds()) && a->cooldownB <= 0) {
          hurtProtag(1);
          a->cooldownB = 1300;
        }
    
      } else {
        //hide graphic
        for(auto entry : a->spawnlist) {
          entry->visible = 0;
        }
      }
      break;
    } 
    case 13:
    {
      //braintrap
      float dist = XYWorldDistanceSquared(a->getOriginX(), a->getOriginY(), protag->getOriginX(), protag->getOriginY());
      const float Dist = 36864;

      a->cooldownA -= elapsed;
      a->cooldownB -= elapsed;
      if(dist < Dist) {
        if(a->cooldownA < 0) {
          a->cooldownA = 3000;
          a->cooldownB = 100;
          hurtProtag(1);
          ribbon* zap = ((ribbon*)a->actorlist[0]);
      
          zap->x1 = protag->getOriginX();
          zap->x = zap->x1;
          zap->y1 = protag->getOriginY();
          zap->y = zap->y1;
          zap->z1 = protag->z + 100;
      
          zap->x2 = a->getOriginX();
          zap->y2 = a->getOriginY();
          zap->z2 = a->z + 160;
        }
      }

      if(a->cooldownB < 0) {
        a->actorlist[0]->visible = 0;
      } else {
        a->actorlist[0]->visible = 1;
      }
      



      break;
    }
    case 100: 
    {
      //zombie
      a->cooldownA += elapsed;
      if(a->flagA && a->cooldownA > a->maxCooldownA) {
        a->cooldownA = 0;
        a->flagA = 0;
        a->msPerFrame = 0;
        a->cooldownB = 300;
        a->flagB = 1;
      }
    
      if(a->flagB) {
        a->cooldownB -= elapsed;
        if(a->cooldownB < 0) {
          a->frameInAnimation = 0;
          a->flagB = 0;
        }
      }
    
      if(a->flagC) {
        a->cooldownC -= elapsed;
        if(a->cooldownC < 0) {
          a->flagC = 0;
          a->spawnlist[0]->visible = 1;
          a->spawnlist[0]->frameInAnimation = 0;
          a->spawnlist[0]->scriptedAnimation = 1;
          a->spawnlist[0]->msPerFrame = 70;
          a->spawnlist[0]->loopAnimation = 0;
          a->flagD = 1;
          a->cooldownD = 300;
        }
    
      }
    
      if(a->flagD) {
        a->cooldownD -= elapsed;
        if(a->cooldownD < 0) {
          a->spawnlist[0]->visible = 0;
          a->flagD = 0;
    
    
          { //create hitbox
            hitbox* h = new hitbox();
      
            float offset = 150;
            float yoff = -offset * sin(a->steeringAngle);
            float xoff = offset * cos(a->steeringAngle);
      
            h->x = a->getOriginX() + xoff - 96;
            h->y = a->getOriginY() + yoff - 96;
            h->z = a->z;
        
            h->bounds.x = 0;
            h->bounds.y = 0;
            h->bounds.z = 20;
            h->bounds.width = 192;
            h->bounds.height = 192;
            h->bounds.zeight = 128;
        
            h->activeMS = 10;
            h->sleepingMS = 0;
          }
        }
      }
    
    
      for(int i = 0; i < a->myAbilities.size(); i++) {
        if(a->myAbilities[i].ready) {
          a->myAbilities[i].ready = 0;
          a->myAbilities[i].cooldownMS = rng(a->myAbilities[i].lowerCooldownBound, a->myAbilities[i].upperCooldownBound);
          switch (i) {
          case 0:
            {
              { //only attack if it would hit
                
                float offset = 150;
                float yoff = -offset * sin(a->steeringAngle);
                float xoff = offset * cos(a->steeringAngle);
    
                rect prediction(a->getOriginX() + xoff -96, a->getOriginY() + yoff - 96, a->z + 20, 192, 192, 128);
    
                if(!CylinderOverlap(prediction, protag->getMovedBounds())) {
                  a->myAbilities[i].ready = 1;
                  break;
                }
                if(g_protagIsWithinBoardable) {
                  a->myAbilities[i].ready = 1;
                  break;
                }
              }
    
    
              a->cooldownA = 0;
              a->flagA = 1;
    
              a->cooldownC = 200;
              a->flagC = 1;
              
    
              a->frameInAnimation = 0;
              a->scriptedAnimation = 1;
              a->msPerFrame = 65;
              a->loopAnimation = 0;
              
    
              break;
            }
          case 1:
            {
              //updatestate
              if(a->lastState == a->activeState) { break;}
              if(a->activeState == 0) {
                //change to passive
                a->readyForNextTravelInstruction = 1;
                a->agrod = 0;
                a->target = nullptr;
                a->traveling = 1;
    
              } else {
                //change to active
                a->agrod = 1;
                a->target = protag;
                a->traveling = 0;
    
              }
              a->lastState = a->activeState;
    
              break;
            }
          case 2:
            {
              //update aggressiveness
              a->bonusSpeed = a->aggressiveness;
    
              break;
            }
          }
          
        }
      }
      break;
    } 
    case 101:
    {
      //disaster
      a->cooldownA -= elapsed;
      if(a->flagA) {
        a->frameInAnimation = 8;
        float angleToTarget = atan2(a->target->getOriginX() - a->getOriginX(), a->target->getOriginY() - a->getOriginY()) - M_PI/2;
        angleToTarget = wrapAngle(angleToTarget);
        a->steeringAngle = angleToTarget;
        a->targetSteeringAngle = angleToTarget;
      }
      if(a->flagA && a->cooldownA <= 0) {
        a->specialAngleOverride = 0;
        a->flagA = 0;
        a->msPerFrame = 0;
        a->scriptedAnimation = 0;
        a->spawnlist[0]->visible = 0;
        a->xmaxspeed = 2;
      }
    
      for(int i = 0; i < a->myAbilities.size(); i++) {
        if(a->myAbilities[i].ready) {
          a->myAbilities[i].ready = 0;
          a->myAbilities[i].cooldownMS = rng(a->myAbilities[i].lowerCooldownBound, a->myAbilities[i].upperCooldownBound);
          switch (i) {
          case 0:
            {
              {
                
                float offset = 50;
                float yoff = -offset * sin(a->steeringAngle);
                float xoff = offset * cos(a->steeringAngle);
    
//                rect prediction(a->getOriginX() + xoff -96, a->getOriginY() + yoff - 96, a->z + 20, 192, 192, 128);
//    
//                if(!CylinderOverlap(prediction, protag->getMovedBounds())) {
//                  a->myAbilities[i].ready  = 1;
//                  a->myAbilities[i].cooldownMS  = 10;
//                  break;
//                }

                if(g_protagIsWithinBoardable) {
                  //kick the player out of the boardable and destroy it
                  //puzzle potential?
                  entity* destroyMe = g_boardedEntity;

                  smokeEffect->happen(protag->getOriginX(), protag->getOriginY(), protag->z, 0);
                  g_protagIsWithinBoardable = 0;
                  protag->steeringAngle = wrapAngle(-M_PI/2);
                  protag->animation = 4;
                  protag->animation = 4;
                  protag->flip = SDL_FLIP_NONE;
                  protag->xvel = 0;
                  //protag->yvel = 200;
                  protag->tangible = 1;
                  g_boardingCooldownMs = g_maxBoardingCooldownMs;

                  destroyMe->tangible = 0;
                  destroyMe->x = -1000;
                  destroyMe->y = -1000;
                  //break;
                }
              }
      
      
              a->cooldownA = 1000;
              a->flagA = 1;
              
              //interesting!
//              a->forwardsPushAngle = a->steeringAngle;
//              a->forwardsPushVelocity = 400;
    
              a->frameInAnimation = 8;
              a->scriptedAnimation = 1;
              a->xmaxspeed = 0;
              a->bonusSpeed = 0;

              a->spawnlist[0]->visible = 1;
              a->spawnlist[0]->frameInAnimation = 0;
              a->spawnlist[0]->scriptedAnimation = 1;
              a->spawnlist[0]->msPerFrame = 30;
              a->spawnlist[0]->loopAnimation = 1;

              { //create hitbox
                hitbox* h = new hitbox();
          
                h->bounds.x = 0;
                h->bounds.y = 0;
                h->bounds.z = 20;
                h->bounds.width = 192;
                h->bounds.height = 192;
                h->bounds.zeight = 128;
            
                h->activeMS = 2000;
                h->sleepingMS = 0;
                h->parent = a->spawnlist[0];
              }
              
      
              break;
            }
          case 1:
            {
              //update aggressiveness
              if(!a->flagA) {
                a->bonusSpeed = a->aggressiveness/3;
              }
    
              break;
            }
          }
          
        }
      }
      break;


      break;
    }
    case 102:
    {
      //creep
      float dist = XYWorldDistanceSquared(a, protag);
      const float range = 262144; //8 blocks
      
      if(dist < range) {
        //protag->hisStatusComponent.slown.addStatus(1,0.4);
        protag->hisStatusComponent.disabled.addStatus(1,0.4);
      }


      a->bonusSpeed = a->aggressiveness;

      a->cooldownA -= elapsed;

      for(int i = 0; i < a->myAbilities.size(); i++) {
        if(a->myAbilities[i].ready) {
          a->myAbilities[i].ready = 0;
          a->myAbilities[i].cooldownMS = rng(a->myAbilities[i].lowerCooldownBound, a->myAbilities[i].upperCooldownBound);
          switch (i) {
            case 0:
              {
                float offset = 150;
                float yoff = -offset * sin(a->steeringAngle);
                float xoff = offset * cos(a->steeringAngle);
                
                { //only attack if it would hit
                  
      
                  rect prediction(a->getOriginX() + xoff -96, a->getOriginY() + yoff - 96, a->z + 20, 192, 192, 128);
      
                  if(!CylinderOverlap(prediction, protag->getMovedBounds())) {
                    a->myAbilities[i].ready = 1;
                    break;
                  }
                  if(g_protagIsWithinBoardable) {
                    a->myAbilities[i].ready = 1;
                    break;
                  }
                }

                { // only attack if she is active
                  if(a->activeState != 1) {
                    a->myAbilities[i].ready = 1;
                    break;
                  }
                }

                a->specialState = 1;
                a->spawnlist[0]->frameInAnimation = 0;
                a->spawnlist[0]->scriptedAnimation = 1;
                a->spawnlist[0]->msPerFrame = 60;
                a->spawnlist[0]->loopAnimation = 0;

                { //create hitbox
                  hitbox* h = new hitbox();
            
                  h->x = a->getOriginX() + xoff - 96;
                  h->y = a->getOriginY() + yoff - 96;
                  h->z = a->z;
              
                  h->bounds.x = 0;
                  h->bounds.y = 0;
                  h->bounds.z = 20;
                  h->bounds.width = 192;
                  h->bounds.height = 192;
                  h->bounds.zeight = 128;
              
                  h->activeMS = 10;
                  h->sleepingMS = 120;
                }

                break;
              }
          }
        }
      }


      switch(a->specialState) {
        case 0:
        {
          //walking
          a->cooldownA = 500;
          

          if(a->lastState == a->activeState) {break;}
          switch(a->activeState) {
            case 0:
            {
              M("switched to passive");
              //passive - roam
              a->readyForNextTravelInstruction = 1;
              a->agrod = 0;
              a->target = nullptr;
              a->traveling = 1;
              
              break;
            }
            case 1:
            {
              M("switched to active");
              a->agrod = 1;
              a->target = protag;
              a->traveling = 0;

            }

          }
          a->lastState = a->activeState;
          break;
        }
        case 1: 
        {
          //prepare for attack
          a->frameInAnimation = 7;
          a->scriptedAnimation = 1;
          a->specialAngleOverride = 1;
          a->spawnlist[0]->visible = 1;
          float angleToTarget = atan2(a->target->getOriginX() - a->getOriginX(), a->target->getOriginY() - a->getOriginY()) - M_PI/2;
          angleToTarget = wrapAngle(angleToTarget);
          //a->steeringAngle = angleToTarget;
          a->targetSteeringAngle = angleToTarget;
          a->hisStatusComponent.slown.addStatus(100,1);

          if(a->cooldownA < 0) {
            a->specialState = 2;
          }
          break;
        }
        case 2:
        {
          a->spawnlist[0]->visible = 0;
          a->scriptedAnimation = 0;
          a->specialState = 0;
          a->specialAngleOverride = 0;
        }
      }

    }
  }
}

void specialObjectsInteract(entity* a) {
  switch(a->identity) {
    case 9:
    {
      //inventory chest
      g_inventoryUiIsLevelSelect = 0;
      g_inventoryUiIsLoadout = 1;
      g_inventoryUiIsKeyboard = 0;
      inventorySelection = 0;
      inPauseMenu = 1;
      g_firstFrameOfPauseMenu = 1;
      old_z_value = 1;
      adventureUIManager->escText->updateText("", -1, 0.9);
      adventureUIManager->positionInventory();
      adventureUIManager->showInventoryUI();
      //adventureUIManager->hideHUD();
      break;
    }
    case 10:
    {
      //corkboard
      clear_map(g_camera);
      g_inventoryUiIsLevelSelect = 1;
      g_inventoryUiIsKeyboard = 0;
      g_inventoryUiIsLoadout = 0;
      inventorySelection = 0;
      inPauseMenu = 1;
      g_firstFrameOfPauseMenu = 1;
      old_z_value = 1;
      adventureUIManager->escText->updateText("", -1, 0.9);
      adventureUIManager->positionInventory();
      adventureUIManager->showInventoryUI();
      adventureUIManager->hideHUD();
      break;
    }
    case 11:
    {
      //lever
      if(a->flagA == 0) {
        //open door
        M("open");
        a->spawnlist[0]->banished = 1;
        a->spawnlist[0]->zaccel = 220;
        a->spawnlist[0]->shadow->enabled = 0;
        a->flagA = 1;
        a->frameInAnimation = 1;
        a->spawnlist[0]->navblock = 0;
      } else {
        //close door
        //need to add check for entities
        int good = 1;
        for(auto x : g_entities) {
          if(!x->dynamic || !x->tangible) {continue;}
          if(RectOverlap(a->spawnlist[0]->getMovedBounds(), x->getMovedBounds())) {
            good = 0;
          }
        }
        if(!good) {break;}
        
        M("close");
        a->spawnlist[0]->banished = 0;
        a->spawnlist[0]->dynamic = 1;
        a->spawnlist[0]->opacity = 255;
        a->spawnlist[0]->shadow->enabled = 1;
        a->spawnlist[0]->navblock = 1;
        for(auto x : a->spawnlist[0]->overlappedNodes) {
          x->enabled = 0;
        }

        a->flagA = 0;
        a->frameInAnimation = 0;
      }
      break;
    }
  }
}

void usableItemCode(usable* a) {
  //1 - spin
  //2 - inventory
  //3 - mechanism
  switch(a->specialAction) {
    case 3:
      {
        //radio
        protag->forwardsPushVelocity = 1700;
        protag->forwardsPushAngle = protag->steeringAngle;
        littleSmokeEffect->happen(protag->getOriginX(), protag->getOriginY(), protag->z, 0);

        break;
      }
  }
}


float exponentialCurve(int max, int exponent) {
  float x = rng(0, 100);
  float b = pow(x, exponent);
  return (b * max)/(pow(100,exponent));
}
