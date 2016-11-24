#pragma once

#include "Common.h"
#include <BWAPI.h>
#include "BuildingPlacer.h"

// Algorithm borrowed from
// http://satirist.org/ai/starcraft/blog/archives/151-LetaBots-wall-4-placing-buildings-to-form-the-wall.html on Nov 14 
// http://www.aaai.org/ocs/index.php/AIIDE/AIIDE14/paper/viewFile/8956/8927 looked at implementing
// https://arxiv.org/pdf/1306.4460.pdf
using namespace BWAPI;


// Boundingbox for wall region that is of size 10 by 10
struct BoundingBox{
    TilePosition start;
    TilePosition end;
    int map[17][17];
};

class WallManager {
public: 
    WallManager(BWAPI::TilePosition defensePoint, BWAPI::Region close, BWAPI::Region far);
    BoundingBox buildBoundingBox(BWAPI::TilePosition chokePoint);
    void findWall(int depth);
    bool WalledOff(int buildingType);
    bool properWall(int x, int y, int buildingNumber);
    void mapOutPlacement(int x, int y, int BuildingType, int BuildingType2);
    bool maxGap();
    BWAPI::TilePosition getSupplyDepot1();
    BWAPI::TilePosition getBarracks();
    BWAPI::TilePosition	getSupplyDepot2();

private:
    bool floodFillInit(int x, int y, int barracks=1) const;
    bool floodFill(int x, int y, int tileType, int xGoal, int yGoal, int barracks) const;   
    int findGoodYPos(int x, int y, int travelDirection) const;
    
    bool foundWall;
    BWAPI::TilePosition SupplyDepot1;
    BWAPI::TilePosition Barracks;
    BWAPI::TilePosition SupplyDepot2;
    bool walkable;
    std::vector<BWAPI::TilePosition> buildingPos;
    BWAPI::Region closeRegion;
    BWAPI::Region farRegion;
    std::array<int,3> buildings;
    std::array<std::array<int, 4>,2> buildingGaps;
    std::array<std::array<int, 2>, 2>buildingSize;
    BoundingBox box;   
};