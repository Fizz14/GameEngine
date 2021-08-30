# AdventureGameEngine
An engine written in c++ to quickly develop top-down adventure games, with platforming, shooting, and puzzle elements.

The engine is 3d, meaning that the player can move in 3 dimensions to navigate a map or dodge bullets, but the rendering isn't, so no models, just sprites in 3d space.

Features:
  - Quick and easy block-based map-editor
    - Launch the game in developermode to access the mapeditor
    - Blocks and entities can be placed in the level
      -easy enough for my family and friends to be able to make 3d maps easily.
    - A few clicks to add a suitable navigation-mesh for AI
    - Simple 2d baked lighting
    - Triggers and Event-listeners can be placed to drive scripts
    - Doors and waypoints to "stitch together" the game.
      - Every door is linked to a waypoint in another map
     - A few ways to incorporate sound into the level
    - (Upcoming) Cutscene-editor
  - Entity-system
    - For NPCs, enemies, or puzzle elements (such as a lever on a wall)
    - Entities can have customizable movement, combat, dialogue, and animation.
      - Enemy agression can be tweaked to attack on sight, or when attacked by another entity (e.g. player)
    - Dynamic entities move intelligently in 3d space thanks to Dijkstra's algorithm
  - Scripting-system
    - Triggers to call scripts when the player overlaps them
    - Interface to entities, combat, cutscenes, puzzles, more complicated mechanics... pretty much any event you would like to have happen
    - Not c++, so it's not compiled.
  - Sound-system
    - Music-nodes placed in the level determine what song is played. The closest music node will play its song.
