#include "WallManager.h"

using namespace UAlbertaBot;

WallManager::WallManager(){}

WallManager::WallManager(BWAPI::TilePosition defensePoint)//, BWAPI::Region close, BWAPI::Region farSide)
{
    count = 0;
    // We just started, we can't have found a wall yet
    foundWall = false;
    // This might be vistigial code
    //closeRegion = close;
    //farRegion = farSide;

    // Generate a bounding box around the point we want to defend
    box = buildBoundingBox(defensePoint);
    buildings[0] = 2;
    buildings[1] = 3;
    buildings[2] = 3;
    // Dimensions of a Barracks
    buildingSize[0][0] = 4; // Width
    buildingSize[0][1] = 3; // Height

    // Dimensions of a SupplyDepot
    buildingSize[1][0] = 3; // Width
    buildingSize[1][1] = 2; // Height

    // Dimensions of a SupplyDepot
    buildingSize[2][0] = 3; // Width
    buildingSize[2][1] = 2; // Height
}

BoundingBox WallManager::buildBoundingBox(BWAPI::TilePosition chokePoint){
    BoundingBox box;
    // a 8 by 8 tile array including choke tile
    int startTileX = chokePoint.x - 8;
    int startTileY = chokePoint.y - 8;
    int endTileX = chokePoint.x + 8;
    int endTileY = chokePoint.y + 8;

    if (startTileX < 0){
        startTileX = 0;
    }

    if (startTileY < 0){
        startTileY = 0;
    }

    int width = BWAPI::Broodwar->mapWidth();
    int height = BWAPI::Broodwar->mapHeight();
    if (endTileX >= width){
        endTileX = width - 1;
    }

    if (endTileY >= height){
        endTileY = height - 1;
    }

    box.start = BWAPI::TilePosition(startTileX, startTileY);
    box.end = BWAPI::TilePosition(endTileX, endTileY);

    std::vector<int> row;
    std::vector<int> row2;
    for (int i = 0; i < 17; ++i){
        
        for (int j = 0; j < 17; ++j){
            box.map[j][i] = BWAPI::Broodwar->isBuildable(i + startTileX, j + startTileY);
            BWAPI::WalkPosition pos = BWAPI::WalkPosition(BWAPI::TilePosition(i + startTileX, j + startTileY));

            // Let's see if for every buildable tile we can walk to it
            // the problem with this is it's not high enough resolution
            bool topLeft = BWAPI::Broodwar->isWalkable(pos.x, pos.y);
            bool topRight = BWAPI::Broodwar->isWalkable(pos.x + 1, pos.y);
            bool bottomLeft = BWAPI::Broodwar->isWalkable(pos.x, pos.y + 1);
            bool bottomRight = BWAPI::Broodwar->isWalkable(pos.x + 1, pos.y + 1);
            walkable[j][i] = (topLeft * topLeft * bottomRight * bottomLeft);
        }
    }
    return box;
}

void WallManager::findWall(int depth){
    //Debug file
    std::ofstream debug;
    debug.open("debug.txt", std::ios_base::app);

    // Problem, need to find close and far regions

    // If we've found the wall we can stop the search
    if (foundWall){
        return;
     }
    // If we've placed all the buildings, do they meet our requirements
    if (depth == buildings.size()){
        // Print out the state of the wall manager
        // Is it walled off?
        bool canWalk = floodFillInit(0, 0);
        count++;

        if (!canWalk){
            debug << *this;

            //lift barracks somehow, can we still pass through?
            bool walkable = floodFillInit(0, 0, 2);
            if (canWalk){
                // If it's not blocked off without baracks we're good to go.
                foundWall = true;
              
                BWAPI::Broodwar->printf("Size of buildingPos %d", buildingPos.size());
                Barracks = buildingPos[0];
                SupplyDepot1 = buildingPos[1];
                SupplyDepot2 = buildingPos[2];
            }
        }
        return;
    } else {
        int building = buildings[depth];
        for (int x = 0; x < int(box.map.size()) ; ++x){
            for (int y = 0; y < int(box.map.size()); ++y){
                // See if we can place the building down in such a way as to build part of a wall
                if (properWall(x, y, building, depth)){
                    // Define tile locations of buildings
                    mapOutPlacement(x, y, depth, building);
                    // This is a good place to put the next building
                    buildingPos.push_back(BWAPI::TilePosition(x + box.start.x, y + box.start.y));
                    findWall(depth + 1);
                    // Maybe not so good of a place
                    buildingPos.pop_back();
                    // Let's unmap it's layout
                    // 0 for blocked, 1 for free, >1 for type of building
                    mapOutPlacement(x, y, depth, 1);
                }
            }
        }

    }
}
// Most likely an A* algorithm, possibly floodfill
// bool WallManager::WalledOff(int buildingType){
//     return false;
// }

bool WallManager::properWall(int x, int y, int buildingNumber, int depth){
    bool neighbour = false;
    Building here;
    BWAPI::TilePosition currentTile = BWAPI::TilePosition(x + box.start.x, y + box.start.y);
    //Don't know how to get that information
    if (depth == 0){
        UnitType unit = MetaType(BWAPI::UnitTypes::Terran_Barracks).getUnitType();
        Building here = Building(unit, currentTile);
    }
    else{
        UnitType unit = MetaType(BWAPI::UnitTypes::Terran_Supply_Depot).getUnitType();
        Building here = Building(unit, currentTile);
    }
    if (!BuildingPlacer::Instance().canBuildHere(currentTile, here)){
        return neighbour;
    }
    if (box.map[y][x] != 1){
        return neighbour;
    }

    int width = buildingSize[depth][0];
    int height = buildingSize[depth][1];
    if (x + width > 16 || y + width > 16){
        return neighbour;
    }
    // We are building Barracks first, so if it's buildable we're good to go
    // Don't have a solid access for this
    if (buildingNumber == 2){
        neighbour = true;
    }
    // If it's not a Barracks, it should be beside another unit
    // x and y being all pairwise combinations of x and y;
    int dx[] = {1, 1, 1, 0, 0, -1, -1,-1};
    int dy[] = { 1, -1, 0, 1, -1, 1, -1, 0 };
    
    
    //For all spots around x and y, is there a building
    for (int i = x; i < x + width ; ++i){
        for (int j = y; j < y+height; ++j){
            for (int k = 0; k < 8; ++k){

                int deltaX = i + dx[k];
                int deltaY = j + dy[k];
                //check to see if there are anybuildings near outer tiles
                // as well, if they are make sure the gap isn't too big
                
                // If they are are the edge, continue
                if (deltaX < 0 || deltaY < 0){
                    continue;
                }
                if (deltaX >= box.map.size() || deltaY >= box.map.size()){
                    continue;
                }

                // Prevent overlapping buildings
                if (box.map[j][i] != 1){
                    return false;
                }
                if (box.map[deltaY][deltaX] >= 2){
                    // if there is a neighbour make sure it doesn't violate the max gap principle
                    // neighbour = maxGap()
                    // Eventually need to check for maxgap but for now we're happy that it has a nei
                    neighbour = true;
                }


                // Idea for how to use maxgap 
                // if(neighbour){
                //     if(maxGap()){
                //         neighbour = false;
                //     } else {
                //         return neighbour;
                //     }
                // }
            }
        }
    }

    return neighbour;
}

bool WallManager::maxGap(){
    return false;
}
BWAPI::TilePosition  WallManager::getSupplyDepot1(){
    return SupplyDepot1;
}
BWAPI::TilePosition  WallManager::getBarracks(){
    return Barracks;
}
BWAPI::TilePosition	 WallManager::getSupplyDepot2(){
    return SupplyDepot2;
}

void WallManager::mapOutPlacement(int x, int y, int buildingType, int fillNumber){
    int width = buildingSize[buildingType][0];
    int height = buildingSize[buildingType][1];
    assert(y + height < 17);
    assert(x + width < 17);
    for(int i=x; i< x + width ; ++i){
        for(int j=y; j < y + height ; ++j){
            box.map[j][i] = fillNumber;
        }
    }
}

// Adapt these two helper function to implement floodfill
bool WallManager::floodFillInit(int x, int y, int barracks) const{
    int width = BWAPI::Broodwar->mapWidth();
    int height = BWAPI::Broodwar->mapHeight();
    //walked = std::array< std::array<int,17>, 17>;
    memset(&walked, 0, 17*17);
    // Generate key for map
    if (x < 0 || x >= width){
        // Bounds check for x
        return false;
    }
    else if (y < 0 || y >= height){
        // Bounds check for y
        return false;
    } else {
    
        int yGoal;
        for (size_t i = x; i < walkable.size(); ++i){
            yGoal = findGoodYPos(i, y, 1);
            if (yGoal>-1){
                break;
            }
        }
        y = yGoal;
        int xGoal = 16;
        for (size_t i = 16; i > walkable.size(); --i){
            yGoal = findGoodYPos(i, y, 1);
            if (yGoal>-1){
                xGoal = i;
                break;
            }
        }
        if (yGoal == -1){
            return false;
        }
        int tileType = 1;
        return floodFill(x, y, tileType, xGoal, yGoal, barracks);
    }
}

bool WallManager::floodFill(const int x, const int y, int tileNumber, int xGoal, int yGoal, int barracks) const{
    int width = BWAPI::Broodwar->mapWidth();
    int height = BWAPI::Broodwar->mapHeight();
    walked[y][x] = 0;
    if (x < 0 || x >= width){
        // Bounds check for x
        return false;
    } else if (y < 0 || y >= height){
        // Bounds check for y
        return false;
    /*} else if (!canFit(size, x, y)){
        return false;*/
    //} else if(x < box.start.x){
    //    // reached min x
    //    return false; 
    //} else if(y < box.start.y){
    //    // reached min y
    //    return false; 
    } else if(y > 16){
        return false;
    } else if(x > 16){
        return false;
    }


    else if (walkable[y][x] != 1  || box.map[y][x] > barracks){
        return false;
    }
    walked[y][x] = 1;
    if(x == xGoal && y == yGoal){
        BWAPI::Broodwar->printf("Found it");
        return true;
    } else{
        // North
        //floodFill(size, x, y - 1, group, tile);
        
        // South 
        if (floodFill(x, y+1, tileNumber, xGoal, yGoal, barracks)){
            return true;
        }
        //East
        if (floodFill(x+1, y, tileNumber, xGoal, yGoal, barracks)){
            return true;
        }
        // West
        // floodFill(size, x - 1, y, group, tile);
    }
    return false;
}

int WallManager::findGoodYPos(int x, int y, int travelDirection) const{
    int max = walkable.size();
    int yPos;
    for (int i = 0; i < max; ++i){
        yPos = y + travelDirection*i;
        if (walkable[yPos][x] == 1 && box.map[yPos][i] == 1){
            return yPos;
        }
    }
    return -1;
}

// For debugging the wall manager
std::ostream& operator<<(std::ostream & out, const WallManager & wallmanager){
    static int iteration = 0;
    out << "WallManager state: " << std::endl;
    out << "Iteration : " << iteration++ << std::endl;
    out << "Found Wall? :" << wallmanager.foundWall << std::endl;

    out << "Building Sizes: " << std::endl;
    int buildingCount = 0;
    for (const auto building : wallmanager.buildingSize){
        out << "Building: " << buildingCount++ << " ";
        out << building[0] << " by " << building[1] << std::endl;
    }

    std::stringstream buildings;
    std::stringstream walkable;
    std::stringstream walked;
    out << "Bounding Box: " << std::endl;
    BoundingBox box = wallmanager.box;
    for (size_t i = 0; i < 17; ++i){
        for (size_t j = 0; j < 17; ++j){
            buildings << box.map[i][j] << " ";
            walkable << wallmanager.walkable[i][j] << " ";
            walked << wallmanager.walked[i][j] << " ";
        }

        out << buildings.rdbuf() << "\t" << walkable.rdbuf() << "\t" << walked.rdbuf() << std::endl;
        buildings.clear();
        walkable.clear();
        walked.clear();
    }

    /*out << "Is Walkable" << std::endl;
    for (size_t i = 0; i < wallmanager.walkable.size(); ++i){
        for (size_t j = 0; j < wallmanager.walkable[i].size(); ++j){
            out << wallmanager.walkable[i][j] << " ";
        }
        out << std::endl;
    }

    out << "Walked" << std::endl;
    for (size_t i = 0; i < wallmanager.walked.size(); ++i){
        for (size_t j = 0; j < wallmanager.walked[i].size(); ++j){
            out << wallmanager.walked[i][j] << " ";
        }
        out << std::endl;
    }*/
    out << "------------" << std::endl << std::endl;
    return out;
}