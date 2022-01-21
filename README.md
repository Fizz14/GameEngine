# GameEngine
A WIP engine written in c++ to quickly develop top-down adventure games, with platforming, fighting, and puzzle elements.

![demo1](https://user-images.githubusercontent.com/74583686/132134718-c85cc70c-3844-4f9f-9d4b-9a1310ff92c4.png)
![demo2](https://user-images.githubusercontent.com/74583686/132134918-490ed6be-9cc4-4616-a9e4-f266af0922f2.png)
![demo3](https://user-images.githubusercontent.com/74583686/132135089-804e84e4-64bf-40ad-96dd-efffe1ac00de.png)
![demo4](https://user-images.githubusercontent.com/74583686/132135462-55b34f87-039b-45d0-8f85-717f1a008576.gif)



Features:
  - Quick and easy block-based map-editor
    - Blocks and entities can be placed in the level
      -easy enough for my family and friends to be able to make 3d maps easily.
    - A few clicks to add a suitable navigation-mesh for AI
    - Simple 2d baked lighting
    - Triggers and Event-listeners can be placed to drive scripts
    - Exits and entrances for the player to travel between maps.
     - A few ways to incorporate sound into the level
     - Heightmaps can be applied to floors to simulate geometry in the z axis
    - Points-of-interest for AI movement, e.g. patrolling behavior
  - Entity-system
    - For NPCs, enemies, or puzzle elements (such as a lever on a wall)
    - Entities can have customizable movement, combat, dialogue, and animation.
    - Dynamic entities move intelligently in 3d space
    - Entities can be given a cost to spawn in the map, and maps can be given a budget with which entities will be spawned procedurally
    - Entities have simple configurable AI and perform scripted abilities based on their distance from their target during combat
    - Entities can be animated or parented to other entities for better visuals
    - Entities belong to a faction and will target members of enemy factions if they detect them. 
  - Scripting-system
    - Triggers to call scripts when the player overlaps them
    - Interface to entities, combat, cutscenes, puzzles, more complicated mechanics... pretty much any event you would like to have happen in a game
    - Not c++, so it's not compiled.
  - Sound-system
    - Music-nodes placed in the level determine what song is played. The closest music node will play its song.
    - Cue-nodes are used to play sounds during cutscenes and if the player walks within a certain radius (e.g. a one-off strike of lightning)
    - Worldsound-nodes add ambience to a map, playing their sounds randomly at controlled intervals at a position (e.g. a croaking frog)
    - Entities can be set to have their own music which plays in range of the player.
  - Save-game-system (to save player progress)
  - Appropriate attention to more technical features
    - Careful management/recycling of resources (e.g. textures) to keep loading-times down and performance up
    - User config for graphics and controls
    - Clever loading from directories so that a map's local directory is checked first for entity, weapon, texture, and other types of files to make development more convienient.
    - Countless failsafes to avoid crashing if the engine is misused


