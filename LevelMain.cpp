#include "PlayerInitialization.h"
#include "FightGroupMaker.h"
#include "DisasterHelper.h"
#include "HFL/Source/HFL.h"
#include "OP2Helper/OP2Helper.h"
#include "Outpost2DLL/Outpost2DLL.h"
#include <array>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <locale>
#include <stdexcept>
#include <memory>

Export void CreateDisaster();
void CreateMarkers();

// Required data exports  (Description, Map, TechTree, GameType, NumPlayers)
ExportLevelDetails("4P, RR, 'Yukon Trail'", "YukonTrail.map", "survtech.txt", MultiResourceRace, 5);

std::vector<bool> ccBuilt; // If each player has built their CC
std::vector<bool> moraleFree; // If each player's morale is free
bool allCCsBuilt;
const int disastersAndMoraleTimer = 20'000;

const MAP_RECT holdingRect(162 + X_, 1 + Y_, 220 + X_, 17 + Y_);

// Allows an enemy AI in a multiplayer scenario.
struct SDescBlockEx {
	int unk0;
	int unk1;
	int unk2;
	int unk3;
	int unk4;
	int unk5;
	int unk6;
	int unk7;
};

SCRIPT_API SDescBlockEx DescBlockEx = { 1, 0, 0, 0, 0, 0, 0, 0 };

struct ScriptGlobal
{
} scriptGlobal;
ExportSaveLoadData(scriptGlobal);

DisasterHelper disasterHelper;

// List of songs to play
SongIds PlayList[] = {
	SongIds::songStatic03,
	SongIds::songEden32, // Eden 5
	SongIds::songEP42,
	SongIds::songEP51,
	SongIds::songEP52,
	SongIds::songEP61,
	SongIds::songEP62,
	SongIds::songEP63
};

int HumanPlayerCount()
{
	return TethysGame::NoPlayers() - 1;
}

//Note: Scenario must have 5 or fewer human players to work.
PlayerColor GetAIColor(bool allowBlack = false)
{
	int totalColors = 6;
	if (allowBlack) {
		totalColors++;
	}

	std::vector<int> availableColors;
	for (int i = 0; i < totalColors; ++i)
	{
		availableColors.push_back(i);
	}

	for (int i = 0; i < TethysGame::NoPlayers() - 1; ++i)
	{
		availableColors.erase(
			std::remove(availableColors.begin(), availableColors.end(), ExtPlayer[i].GetColorNumber()), availableColors.end());
	}

	if (availableColors.size() == 0) {
		return PlayerColor::PlayerBlue;
	}

	int colorIndex = TethysGame::GetRand(availableColors.size());

	return (PlayerColor)availableColors[colorIndex];
}

// String content must be preserved for proper viewing of victory conditions
std::string commonMetalObjective;
std::string rareMetalObjective;
std::string noEnemiesObjective("No enemy units in rendevous area");

// AI player is last player
PlayerNum GetAIIndex()
{
	for (int i = 0; i < 7; ++i)
	{
		if (!Player[i].IsHuman()) {
			return static_cast<PlayerNum>(i);
		}
	}

	throw std::runtime_error("No AI player detected");
}

bool EnemyUnitInRendevousArea()
{
	Unit unit;
	InRectEnumerator rectEnumerator(holdingRect);

	while (rectEnumerator.GetNext(unit))
	{
		if (unit.OwnerID() == GetAIIndex()) {
			return true;
		}
	}

	return false;
}

//Trigger noEnemiesInRendevousTrigger;
//Trigger enemiesInRendevousVictoryTrigger;

//void CreateEnemiesInRendevousVictoryCondition()
//{
//	noEnemiesInRendevousTrigger = CreateTimeTrigger(true, false, 5, "NoResponseToTrigger");
//	enemiesInRendevousVictoryTrigger = CreateVictoryCondition(true, false, noEnemiesInRendevousTrigger, noEnemiesObjective.c_str());
//}

void CreateVictoryConditions()
{
	const int commonMetalToWin = 20'000 * HumanPlayerCount(); 
	const int rareMetalToWin = 10'000 * HumanPlayerCount();
	
	std::ostringstream iss;
	iss.imbue(std::locale(""));
	iss << "Stage " << commonMetalToWin << " common metal in trucks at northern waypoint";
	commonMetalObjective = iss.str();

	iss.str( std::string() );
	iss.clear();
	iss << "Stage " << rareMetalToWin << " rare metal in trucks at northern waypoint";
	rareMetalObjective = iss.str();
	

	Trigger triggerCommon = CreateEscapeTrigger(true, false, PlayerAll, holdingRect.x1, holdingRect.y1, holdingRect.Width(), holdingRect.Height(), commonMetalToWin / 1000, map_id::mapCargoTruck, Truck_Cargo::truckCommonMetal, commonMetalToWin, "NoResponseToTrigger");
	CreateVictoryCondition(true, false, triggerCommon, commonMetalObjective.c_str());

	Trigger triggerRare = CreateEscapeTrigger(true, false, PlayerAll, holdingRect.x1, holdingRect.y1, holdingRect.Width(),holdingRect.Height(), rareMetalToWin / 1000, map_id::mapCargoTruck, Truck_Cargo::truckRareMetal, rareMetalToWin, "NoResponseToTrigger");
	CreateVictoryCondition(true, false, triggerRare, rareMetalObjective.c_str());

	//CreateEnemiesInRendevousVictoryCondition();
}

std::unique_ptr<FightGroupMaker> fightGroupMaker;
Trigger attackTrigger;
Trigger missileTrigger;

const std::array<MAP_RECT, 8> commonMineRects{
	MAP_RECT(28 + X_, 32 + Y_, 35 + X_ ,56 + Y_),
	MAP_RECT(37 + X_, 32 + Y_, 49 + X_ ,56 + Y_),

	MAP_RECT(28 + X_, 58 + Y_, 35 + X_ ,86 + Y_),
	MAP_RECT(37 + X_, 58 + Y_, 49 + X_ ,86 + Y_),

	MAP_RECT(51 + X_, 32 + Y_, 70 + X_ ,56 + Y_),
	MAP_RECT(72 + X_, 32 + Y_, 90 + X_ ,56 + Y_),

	MAP_RECT(51 + X_, 58 + Y_, 70 + X_ ,86 + Y_),
	MAP_RECT(72 + X_, 58 + Y_, 90 + X_ ,86 + Y_)
};

void CreateBeacon(const LOCATION& loc, BeaconTypes beaconType, Yield yield)
{
	TethysGame::CreateBeacon(map_id::mapMiningBeacon, loc.x, loc.y, beaconType, yield, Variant::Variant3);
}

Export int InitProc()
{
	HFLInit();
	CreateMarkers();

	PlayerNum aiIndex = GetAIIndex();
	Player[aiIndex].GoAI();
	Player[aiIndex].SetColorNumber(GetAIColor());
	Player[aiIndex].GoPlymouth();

	fightGroupMaker = std::make_unique<FightGroupMaker>(GetAIIndex(), holdingRect);
	ccBuilt.resize(HumanPlayerCount(), false);
	
	if (TethysGame::UsesMorale()) {
		moraleFree.resize(HumanPlayerCount(), false);
	}
	else {
		moraleFree.resize(HumanPlayerCount(), true);
	}

	TethysGame::SetMusicPlayList(8, 2, PlayList);

	TethysGame::ForceMoraleGood(-1);

	TethysGame::SetDaylightEverywhere(TethysGame::UsesDayNight() == 0);
	TethysGame::SetDaylightMoves(1);
	GameMap::SetInitialLightLevel(TethysGame::GetRand(128));

	for (int i = 0; i < HumanPlayerCount(); ++i)
	{
		for (int j = i + 1; j < HumanPlayerCount(); ++j) {
			Player[i].AllyWith(j);
			Player[j].AllyWith(i);
		}
	}

	InitializePlayers(HumanPlayerCount(), GetAIIndex());

	CreateBeacon(MAP_RECT(26 + X_, 41 + Y_, 40 + X_, 49 + Y_).RandPt(), BeaconTypes::OreTypeCommon, Yield::Bar3);

	CreateBeacon(MAP_RECT(124 + X_, 72 + Y_, 133 + X_, 82 + Y_).RandPt(), BeaconTypes::OreTypeCommon, Yield::Bar2);
	CreateBeacon(MAP_RECT(47 + X_, 154 + Y_, 75 + X_, 165 + Y_).RandPt(), BeaconTypes::OreTypeCommon, Yield::Bar2);
	CreateBeacon(MAP_RECT(56 + X_, 183 + Y_, 70 + X_, 198 + Y_).RandPt(), BeaconTypes::OreTypeCommon, Yield::Bar2);
	CreateBeacon(MAP_RECT(191 + X_, 175 + Y_, 224 + X_, 199 + Y_).RandPt(), BeaconTypes::OreTypeCommon, Yield::Bar2);
	CreateBeacon(MAP_RECT(209 + X_, 221 + Y_, 224 + X_, 234 + Y_).RandPt(), BeaconTypes::OreTypeCommon, Yield::Bar2);
	CreateBeacon(MAP_RECT(127 + X_, 90 + Y_, 135 + X_, 99 + Y_).RandPt(), BeaconTypes::OreTypeCommon, Yield::Bar2);
	CreateBeacon(MAP_RECT(104 + X_, 194 + Y_, 120 + X_, 203 + Y_).RandPt(), BeaconTypes::OreTypeCommon, Yield::Bar2);

	CreateBeacon(MAP_RECT(241 + X_, 236 + Y_, 251 + X_, 243 + Y_).RandPt(), BeaconTypes::OreTypeCommon, Yield::Bar1);
	CreateBeacon(MAP_RECT(212 + X_, 52 + Y_, 231 + X_, 65 + Y_).RandPt(), BeaconTypes::OreTypeCommon, Yield::Bar1);
	CreateBeacon(MAP_RECT(238 + X_, 70 + Y_, 252 + X_, 80 + Y_).RandPt(), BeaconTypes::OreTypeCommon, Yield::Bar1);
	CreateBeacon(MAP_RECT(194 + X_, 75 + Y_, 205 + X_, 82 + Y_).RandPt(), BeaconTypes::OreTypeCommon, Yield::Bar1);
	CreateBeacon(MAP_RECT(145 + X_, 149 + Y_, 176 + X_, 175 + Y_).RandPt(), BeaconTypes::OreTypeCommon, Yield::Bar1);
	CreateBeacon(MAP_RECT(100 + X_, 157 + Y_, 118 + X_, 167 + Y_).RandPt(), BeaconTypes::OreTypeCommon, Yield::Bar1);
	CreateBeacon(MAP_RECT(133 + X_, 22 + Y_, 148 + X_, 37 + Y_).RandPt(), BeaconTypes::OreTypeCommon, Yield::Bar1);

	CreateBeacon(MAP_RECT(215 + X_, 248 + Y_, 227 + X_, 254 + Y_).RandPt(), BeaconTypes::OreTypeRare, Yield::Bar3);
	CreateBeacon(MAP_RECT(212 + X_, 52 + Y_, 231 + X_, 65 + Y_).RandPt(), BeaconTypes::OreTypeRare, Yield::Bar3);
	CreateBeacon(MAP_RECT(33 + X_, 84 + Y_, 51 + X_, 90 + Y_).RandPt(), BeaconTypes::OreTypeRare, Yield::Bar3);

	CreateBeacon(MAP_RECT(199 + X_, 247 + Y_, 209 + X_, 251 + Y_).RandPt(), BeaconTypes::OreTypeRare, Yield::Bar2);
	CreateBeacon(MAP_RECT(101 + X_, 84 + Y_, 110 + X_, 90 + Y_).RandPt(), BeaconTypes::OreTypeRare, Yield::Bar2);
	CreateBeacon(MAP_RECT(129 + X_, 204 + Y_, 148 + X_, 212 + Y_).RandPt(), BeaconTypes::OreTypeRare, Yield::Bar2);
	CreateBeacon(MAP_RECT(241 + X_, 3 + Y_, 252 + X_, 9 + Y_).RandPt(), BeaconTypes::OreTypeRare, Yield::Bar2);
	CreateBeacon(MAP_RECT(241 + X_, 3 + Y_, 252 + X_, 9 + Y_).RandPt(), BeaconTypes::OreTypeRare, Yield::Bar2);

	CreateBeacon(MAP_RECT(78 + X_, 184 + Y_, 252 + X_, 100 + Y_).RandPt(), BeaconTypes::OreTypeRare, Yield::Bar1);
	CreateBeacon(MAP_RECT(199 + X_, 96 + Y_, 217 + X_, 106 + Y_).RandPt(), BeaconTypes::OreTypeRare, Yield::Bar1);
	
	TethysGame::CreateBeacon(map_id::mapFumarole, 229 + X_, 162 + Y_, -1, -1, -1);
	TethysGame::CreateBeacon(map_id::mapFumarole, 239 + X_, 162 + Y_, -1, -1, -1);
	TethysGame::CreateBeacon(map_id::mapFumarole, 231 + X_, 22 + Y_, -1, -1, -1);
	TethysGame::CreateBeacon(map_id::mapFumarole, 245 + X_, 6 + Y_, -1, -1, -1);
	TethysGame::CreateBeacon(map_id::mapFumarole, 73 + X_, 18 + Y_, -1, -1, -1);
	TethysGame::CreateBeacon(map_id::mapFumarole, 52 + X_, 239 + Y_, -1, -1, -1);
	TethysGame::CreateBeacon(map_id::mapFumarole, 83 + X_, 193 + Y_, -1, -1, -1);

	TethysGame::CreateBeacon(map_id::mapMagmaVent, 167 + X_, 140 + Y_, -1, -1, -1);
	TethysGame::CreateBeacon(map_id::mapMagmaVent, 7 + X_, 10 + Y_, -1, -1, -1);
	TethysGame::CreateBeacon(map_id::mapMagmaVent, 98 + X_, 153 + Y_, -1, -1, -1);
	

	CreateVictoryConditions();

	CreateTimeTrigger(true, true, 16'000, 17'000, "CreateScoutPatrol");
	
	return true;
}

void CreateMarkers()
{
	
	int midX, midY;
	midX = (holdingRect.x2 - holdingRect.x1) / 2;
	midY = (holdingRect.y2 - holdingRect.y1) / 2;
	Unit marker;

	TethysGame::PlaceMarker(marker, holdingRect.x1, holdingRect.y1, MarkerTypes::Circle);
	TethysGame::PlaceMarker(marker, holdingRect.x2, holdingRect.y1, MarkerTypes::Circle);
	TethysGame::PlaceMarker(marker, holdingRect.x1, holdingRect.y2, MarkerTypes::Circle);
	TethysGame::PlaceMarker(marker, holdingRect.x2, holdingRect.y2, MarkerTypes::Circle);
	TethysGame::PlaceMarker(marker, holdingRect.x1 + midX, holdingRect.y1, MarkerTypes::Circle);
	TethysGame::PlaceMarker(marker, holdingRect.x1 + midX, holdingRect.y2, MarkerTypes::Circle);
	TethysGame::PlaceMarker(marker, holdingRect.x1, holdingRect.y1 + midY, MarkerTypes::Circle);
	TethysGame::PlaceMarker(marker, holdingRect.x2, holdingRect.y1 + midY, MarkerTypes::Circle);
}

void FreeMorale(int playerIndex) 
{
	Unit unit;
	moraleFree[playerIndex] = true;
	TethysGame::FreeMoraleLevel(playerIndex);
	TethysGame::AddMessage(unit, "Morale is waivering, Good Luck Commander!", playerIndex, SoundID::sndMessage2);
}

void CheckMorale()
{
	for (int i = 0; i < HumanPlayerCount(); ++i) {
		if (moraleFree[i]) {
			continue;
		}

		if (TethysGame::Time() > disastersAndMoraleTimer) {
			FreeMorale(i);
		}

		PlayerBuildingEnum playerBuildingEnum = PlayerBuildingEnum(i, map_id::mapStandardLab);

		Unit unit;
		while (playerBuildingEnum.GetNext(unit))
		{
			if (unit.GetType() == map_id::mapStandardLab)
			{
				FreeMorale(i);
				break;
			}
		}
	}
}

void AddSafeRect(Unit unit, int playerIndex)
{
	ccBuilt[playerIndex] = true;

	disasterHelper.AddSafeRect(MAP_RECT(
		unit.Location().x - 15, unit.Location().y - 15,
		unit.Location().x + 15, unit.Location().y + 15));
}

void CheckccBuilt() 
{
	for (int i = 0; i < HumanPlayerCount(); ++i) {
		if (ccBuilt[i]) {
			continue;
		}

		if (TethysGame::Time() > disastersAndMoraleTimer) {
			ccBuilt[i] = true;
			continue;
		}

		PlayerBuildingEnum playerBuildingEnum = PlayerBuildingEnum(i, map_id::mapCommandCenter);

		Unit unit;
		while (playerBuildingEnum.GetNext(unit))
		{
			if (unit.GetType() == map_id::mapCommandCenter)
			{
				AddSafeRect(unit, i);
				break;
			}
		}
	}

}

//void CheckEnemiesInRendevousArea()
//{
//	if (EnemyUnitInRendevousArea()) {
//		if (noEnemiesInRendevousTrigger.HasFired(Player0)) {
//			noEnemiesInRendevousTrigger.Destroy();
//			enemiesInRendevousVictoryTrigger.Destroy();
//
//			CreateEnemiesInRendevousVictoryCondition();
//		}
//	}
//	else {
//		if (!noEnemiesInRendevousTrigger.IsEnabled()) {
//			noEnemiesInRendevousTrigger.Enable();
//		}
//	}
//}

void CheckReadyForDisasters()
{
	for (auto isBuilt : ccBuilt) {
		if (!isBuilt) {
			return;
		}
	}

	allCCsBuilt = true;

	if (TethysGame::CanHaveDisasters())
	{
		Trigger DisasterTimeTrig = CreateTimeTrigger(true, false, 2000, 4500, "CreateDisaster"); //Set time in ticks (marks / 100)
	}
}

Export void AIProc() 
{
	//CheckEnemiesInRendevousArea();
	CheckccBuilt();
	CheckMorale();
	fightGroupMaker->UpdateFightGroups();

	if (!allCCsBuilt) {
		CheckReadyForDisasters();
	}
	
}

Export void NoResponseToTrigger() {}	//Optional function export, supposed to be empty

Export void CreateDisaster()
{
	if (!disasterHelper.MapPropertiesSet())
	{
		disasterHelper.SetMapProperties(256, 256, false); //MapWidth, MapHeight, Does map wrap East/West
	}

	disasterHelper.CreateRandomDisaster();
}

Export void CreateScoutPatrol()
{
	fightGroupMaker->CreatePatrolGroups();
	//attackTrigger = CreateTimeTrigger(true, false, 70, 110, "CreateAttack");
	attackTrigger = CreateTimeTrigger(true, false, 7500, 10000, "CreateAttack");

	const int playerCount = HumanPlayerCount();
	if (playerCount == 2) {
		missileTrigger = CreateTimeTrigger(true, false, 6000, 9000, "LaunchEmpMissile");
	}
	else if (playerCount == 3) {
		missileTrigger = CreateTimeTrigger(true, false, 5000, 8000, "LaunchEmpMissile");
	}
	else {
		missileTrigger = CreateTimeTrigger(true, false, 4000, 7000, "LaunchEmpMissile");
	}
}

Export void CreateAttack()
{
	fightGroupMaker->CreateAttack();
}

Export void LaunchEmpMissile()
{
	fightGroupMaker->LaunchEmpMissile();
}
