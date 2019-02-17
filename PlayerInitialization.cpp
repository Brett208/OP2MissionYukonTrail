#include "PlayerInitialization.h"
#include "OP2Helper/OP2Helper.h"
#include <array>
#include <vector>

void InitializePlayer(const PlayerNum playerNumber, const LOCATION& startLoc);
void CreateInitialVech(Unit& unit, map_id unitType, const LOCATION& loc, PlayerNum playerNumber, map_id cargo);

// TODO: Randomize starting location
const std::array<LOCATION, 4> playerStartLocs{
	LOCATION(165 + X_, 1 + Y_),
	LOCATION(170 + X_, 1 + Y_),
	LOCATION(175 + X_, 1 + Y_),
	LOCATION(180 + X_, 1 + Y_)};

const ResourceSet resourceSet {
	{
		// Kids, Adults, Scientist, Food, Common, Rare
		{ 27, 18, 22, 4000, 0, 0 },		// Easy
		{ 24, 15, 20, 3500, 0, 0 },		// Medium
		{ 21, 13, 19, 3250, 0, 0 },		// Hard
	}
};

void CreateWinVechs(PlayerNum playerNumber, PlayerNum aiPlayerIndex);

void InitializePlayers(int humanPlayerCount, PlayerNum aiPlayerIndex)
{
	std::vector<LOCATION> initStartLocs(playerStartLocs.size());
	std::copy(playerStartLocs.begin(), playerStartLocs.end(), initStartLocs.begin());

#if _DEBUG
	//CreateWinVechs(PlayerNum::Player0, aiPlayerIndex);
#endif

	for (int i = 0; i < humanPlayerCount; ++i)
	{
		InitPlayerResources(i, resourceSet);
		
		const int locationIndex = TethysGame::GetRand(initStartLocs.size());
		InitializePlayer(static_cast<PlayerNum>(i), initStartLocs[locationIndex]);
		initStartLocs.erase(initStartLocs.begin() + locationIndex);
		
	}
}

void InitializePlayer(const PlayerNum playerNumber, const LOCATION& startLoc)
{
	Player[playerNumber].CenterViewOn(startLoc.x, startLoc.y);

	LOCATION currentLoc = startLoc;
	Unit unit;

	CreateInitialVech(unit, map_id::mapEarthworker, currentLoc, playerNumber, map_id::mapNone);
	currentLoc.x++;
	CreateInitialVech(unit, map_id::mapConVec, currentLoc, playerNumber, map_id::mapStructureFactory);
	currentLoc.x++;
	CreateInitialVech(unit, map_id::mapConVec, currentLoc, playerNumber, map_id::mapCommonOreSmelter);
	currentLoc.x++;
	CreateInitialVech(unit, map_id::mapConVec, currentLoc, playerNumber, map_id::mapAgridome);

	currentLoc.x = startLoc.x;
	currentLoc.y++;

	CreateInitialVech(unit, map_id::mapConVec, currentLoc, playerNumber, map_id::mapTokamak);
	currentLoc.x++;
	CreateInitialVech(unit, map_id::mapConVec, currentLoc, playerNumber, map_id::mapCommandCenter);
	currentLoc.x++;
	CreateInitialVech(unit, map_id::mapConVec, currentLoc, playerNumber, map_id::mapBasicLab);
	currentLoc.x++;
	CreateInitialVech(unit, map_id::mapRoboMiner, currentLoc, playerNumber, map_id::mapNone);

	currentLoc.x = startLoc.x;
	currentLoc.y++;

	CreateInitialVech(unit, map_id::mapEvacuationTransport, currentLoc, playerNumber, map_id::mapNone);
	currentLoc.x++;
	CreateInitialVech(unit, map_id::mapEvacuationTransport, currentLoc, playerNumber, map_id::mapNone);
	currentLoc.x++;
	CreateInitialVech(unit, map_id::mapCargoTruck, currentLoc, playerNumber, map_id::mapNone);
	unit.SetTruckCargo(Truck_Cargo::truckCommonMetal, 1000);
	currentLoc.x++;
	CreateInitialVech(unit, map_id::mapCargoTruck, currentLoc, playerNumber, map_id::mapNone);
	unit.SetTruckCargo(Truck_Cargo::truckCommonMetal, 1000);
	
	currentLoc.x = startLoc.x;
	currentLoc.y++;

	CreateInitialVech(unit, map_id::mapCargoTruck, currentLoc, playerNumber, map_id::mapNone);
	unit.SetTruckCargo(Truck_Cargo::truckFood, 1000);
	currentLoc.x++;
	CreateInitialVech(unit, map_id::mapCargoTruck, currentLoc, playerNumber, map_id::mapNone);
	unit.SetTruckCargo(Truck_Cargo::truckFood, 1000);
	currentLoc.x++;
	CreateInitialVech(unit, map_id::mapRoboSurveyor, currentLoc, playerNumber, map_id::mapNone);

	map_id cargo = map_id::mapMicrowave;
	if (Player[playerNumber].IsEden()) {
		cargo = map_id::mapLaser;
	}

	for (int i = 0; i < TethysGame::InitialUnits(); ++i)
	{
		currentLoc.x = startLoc.x + i % 4;
		currentLoc.y = startLoc.y + 5 + i / 4;

		CreateInitialVech(unit, map_id::mapLynx, currentLoc, playerNumber, cargo);
	}
}

void CreateInitialVech(Unit& unit, map_id unitType, const LOCATION& loc, PlayerNum playerNumber, map_id cargo)
{
	const UnitDirection rotation = UnitDirection::South;
	TethysGame::CreateUnit(unit, unitType, loc, playerNumber, cargo, rotation);
	unit.DoSetLights(true);

	LOCATION moveLoc(unit.Location().x, unit.Location().y + 5);
	unit.DoMove(moveLoc);
}

void CreateWinVechs(PlayerNum playerNumber, PlayerNum aiPlayerIndex)
{
	Unit unit;
	LOCATION enemyLoc(200 + X_, 5 + Y_);
	LOCATION CCLoc(150 + X_, 100 + Y_);
	LOCATION CCLoc2(165 + X_, 100 + Y_);
	LOCATION smelterLoc(160 + X_, 125 + Y_);
	LOCATION mineLoc(120 + X_, 150 + Y_);
	LOCATION empLoc(152 + X_, 100 + Y_);
	LOCATION rpgLoc(155 + X_, 100 + Y_);
	
	for (int i = 0; i < 40; ++i)
	{
		LOCATION loc(176 + X_, 25 + Y_);
		CreateInitialVech(unit, map_id::mapCargoTruck,loc, playerNumber, map_id::mapNone);
		unit.SetTruckCargo(Truck_Cargo::truckCommonMetal, 1000);
	}
	for (int i = 0; i < 20; ++i)
	{
		LOCATION loc(160 + X_, 25 + Y_);
		CreateInitialVech(unit, map_id::mapCargoTruck, loc, playerNumber, map_id::mapNone);
		unit.SetTruckCargo(Truck_Cargo::truckRareMetal, 1000);
	}

	for(int i = 0;i < 5; i++)
		CreateInitialVech(unit, map_id::mapLynx, empLoc, playerNumber, map_id::mapEMP);
	
	for (int i = 0; i < 5; i++)
		CreateInitialVech(unit, map_id::mapLynx, rpgLoc, playerNumber, map_id::mapRPG);

	CreateInitialVech(unit, map_id::mapScout, enemyLoc, aiPlayerIndex, map_id::mapNone);

	TethysGame::CreateUnit(unit, map_id::mapCommandCenter, CCLoc, playerNumber, map_id::mapNone, East);
	TethysGame::CreateUnit(unit, map_id::mapCommandCenter, CCLoc2, Player1, map_id::mapNone, East);

	TethysGame::CreateUnit(unit, map_id::mapCommonOreSmelter, smelterLoc, playerNumber, map_id::mapNone, East);
	TethysGame::CreateUnit(unit, map_id::mapRareOreSmelter, mineLoc, Player1, map_id::mapNone, East);
	
}
