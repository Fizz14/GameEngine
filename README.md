![demo1](https://user-images.githubusercontent.com/74583686/132134718-c85cc70c-3844-4f9f-9d4b-9a1310ff92c4.png)

# AdventureGameEngine
A WIP engine written in c++ to quickly develop top-down adventure games, with platforming, shooting, and puzzle elements.

The engine is 3d, meaning that the player can move in 3 dimensions to navigate a map or dodge bullets, but the rendering isn't, so no models, just sprites in 3d space.

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
    - (Upcoming) Cutscene-editor
  - Entity-system
    - For NPCs, enemies, or puzzle elements (such as a lever on a wall)
    - Entities can have customizable movement, combat, dialogue, and animation.
    - Dynamic entities move intelligently in 3d space thanks to Dijkstra's algorithm
    - Entities can be given a cost to spawn in the map, and maps can be given a budget with which entities will be spawned procedurally
  - Scripting-system
    - Triggers to call scripts when the player overlaps them
    - Interface to entities, combat, cutscenes, puzzles, more complicated mechanics... pretty much any event you would like to have happen in a game
    - Not c++, so it's not compiled.
  - Sound-system
    - Music-nodes placed in the level determine what song is played. The closest music node will play its song.
    - Cue-nodes are used to play sounds during cutscenes and if the player walks within a certain radius (e.g. a one-off strike of lightning)
    - Worldsound-nodes add ambience to a map, playing their sounds randomly at controlled intervals at a position (e.g. a croaking frog)
  - Save-game-system (to save player progress)
  - Appropriate attention to more technical features
    - Careful management/recycling of resources (e.g. textures) to keep loading-times down and performance up
    - User config for graphics and controls
    - Clever loading from directories so that a map's local directory is checked first for entity, weapon, texture, and other types of files to make development more convienient.
    - Countless failsafes to avoid crashing if the engine is misused

I'm creating a game alongside this engine. This guides my decision-making for the engine.
The ultimate goal, and what I see as the definition of this engine, is to be able to make whatever top-down adventure game you would like to without writing a line of c++.
I might make an exception for complicated combat-abilities, which might deserve their own c++ implementation rather than a script.
