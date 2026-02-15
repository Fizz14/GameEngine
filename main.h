#ifndef main_h
#define main_h

void updateWindowResolution();

void setBarColor();

void TitleLoop();

void explorationRender();

void ExplorationLoop();

void renderGPiece(int px, int py, int pindex, SDL_Texture* floortex, SDL_Texture walltex, int opacity);

void CombatLoop();

void toggleDevmode();

void toggleFullscreen();

void resetUnremarkableData();

bool segmentsInSamePlace(edgeInfo seg1, edgeInfo seg2, float tolerance);

void sortEdges(std::vector<edgeInfo>& edges, float px, float py); 

void drawUI();

#endif
