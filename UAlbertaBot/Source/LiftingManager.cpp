#include "LiftingManager.h"

using namespace UAlbertaBot;

LiftingManager & LiftingManager::Instance()
{
	static LiftingManager instance;
	return instance;
}

void LiftingManager::update(){
	//check if any wall buildings need to be lifted


}