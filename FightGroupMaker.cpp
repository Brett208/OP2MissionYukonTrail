#include "FightGroupMaker.h"
#include "OP2Helper/OP2Helper.h"
#include "HFL\Source\HFL.h"
#include <array>
#include <algorithm>
#include <stdexcept>

const std::map<TurretLevel,std::vector<FightGroupMaker::TurretRatio>> FightGroupMaker::PlymouthTurrets
{
	{
		TurretLevel::Initial,
		std::vector<TurretRatio> { 
			TurretRatio {map_id::mapMicrowave, 70 }, 
			TurretRatio { map_id::mapStickyfoam, 100 }}
	},
	{
		TurretLevel::Intermediate,
		std::vector<TurretRatio> {
			TurretRatio { map_id::mapStickyfoam, 20 },
			TurretRatio { map_id::mapRPG, 70 },
			TurretRatio { map_id::mapEMP, 100 }}
	},
	{
		TurretLevel::Advanced,
		std::vector<TurretRatio> {
			TurretRatio { map_id::mapStickyfoam, 15 },
			TurretRatio { map_id::mapRPG, 40 },
			TurretRatio { map_id::mapEMP, 70 },
			TurretRatio { map_id::mapESG, 100 }}
	}
};

const std::map<TurretLevel, std::vector<FightGroupMaker::TurretRatio>> FightGroupMaker::EdenTurrets
{
	{
		TurretLevel::Initial,
		std::vector<TurretRatio> {
			TurretRatio { map_id::mapLaser, 100 }}
	},
	{
		TurretLevel::Intermediate,
		std::vector<TurretRatio> {
			TurretRatio { map_id::mapRailGun, 70 },
			TurretRatio { map_id::mapEMP, 100 }}
	},
	{
		TurretLevel::Advanced,
		std::vector<TurretRatio> {
			TurretRatio { map_id::mapEMP, 30 },
			TurretRatio { map_id::mapAcidCloud, 50 },
			TurretRatio { map_id::mapThorsHammer, 100 }}
	}
};

const std::array<FightGroupMaker::StartLoc, 8> FightGroupMaker::startLocs{
	StartLoc { LOCATION(26 + X_, 1 + Y_), UnitDirection::South},
	StartLoc { LOCATION(113 + X_, 1 + Y_), UnitDirection::South},
	StartLoc { LOCATION(1 + X_, 221 + Y_), UnitDirection::East},
	StartLoc { LOCATION(1 + X_, 46 + Y_), UnitDirection::East},
	StartLoc { LOCATION(1 + X_, 148 + Y_), UnitDirection::East},
	StartLoc { LOCATION(95 + X_, 255 + Y_), UnitDirection::North},
	StartLoc { LOCATION(20 + X_, 255 + Y_), UnitDirection::North},
	StartLoc { LOCATION(256 + X_, 138 + Y_), UnitDirection::West}
};

FightGroupMaker::FightGroupMaker(PlayerNum aiPlayerNum, MAP_RECT holdingRect) :
	aiPlayerNum(aiPlayerNum),
	aiTechManager(aiPlayerNum),
	attackGroupSize(6),
	holdingRect(holdingRect) {}

void FightGroupMaker::UpdateFightGroups()
{
	CheckTargets();
	CheckHoldingZone();
}

void FightGroupMaker::CheckTargets()
{
	for (int i = fightGroupsWithTarget.size(); i-- > 0; ) {
		if (!fightGroupsWithTarget[i].building.IsLive()) {
			FightGroupTarget fightGroupTarget = fightGroupsWithTarget[i];
			fightGroupsWithTarget.erase(fightGroupsWithTarget.begin() + i);
			TaskFightGroup(fightGroupTarget.fightGroup);
		}
	}
}

void FightGroupMaker::CheckHoldingZone()
{
	bool attacked = false;
	for (std::size_t i = 0; i < fightGroupsHoldingRect.size(); ++i)
	{
		if (fightGroupsHoldingRect[i].HasBeenAttacked())
		{
			attacked = true;
			break;
		}
	}

	if (attacked) {
		for (FightGroup& fightGroup : fightGroupsHoldingRect) {
			fightGroup.DoAttackEnemy();
		}
		fightGroupsHoldingRect.clear();
	}
}

void FightGroupMaker::CreatePatrolGroups()
{
	for (std::size_t i = 0; i < HumanPlayerCount(); ++i) {
		CreatePatrolGroup();
	}
}

void FightGroupMaker::CreatePatrolGroup()
{
#if _DEBUG
	//AttackGroupDebug();
	//TestGuardingHoldingZone();
	//return;
#endif

	FightGroup patrolGroup = CreateFightGroup(Player[aiPlayerNum]);

	const StartLoc& startLoc = startLocs[TethysGame::GetRand(startLocs.size())];

	Unit unit;
	TethysGame::CreateUnit(unit, map_id::mapLynx, startLoc.loc, aiPlayerNum, map_id::mapLaser, startLoc.rotation);
	unit.DoSetLights(true);
	patrolGroup.TakeUnit(unit);

	LOCATION patrolLoc = GetPatrolLoc(startLoc.rotation);

	LOCATION patrolPoints[4];
	patrolPoints[0] = patrolLoc;
	patrolPoints[1] = startLocs[TethysGame::GetRand(startLocs.size())].loc;
	patrolPoints[2] = startLoc.loc;
	patrolPoints[3] = LOCATION(-1, -1);

	PatrolRoute patrolRoute;
	patrolRoute.waypoints = patrolPoints;
	patrolGroup.DoPatrolOnly();
	patrolGroup.SetPatrolMode(patrolRoute);
}

LOCATION FightGroupMaker::GetPatrolLoc(UnitDirection rotation)
{
	switch (rotation)
	{
	case West: //unit moves West
		return LOCATION(0 + X_, 0 + Y_);

	case North: //unit moves North
		return LOCATION(187 + X_, 5 + Y_);

	case East: //unit moves East
		return LOCATION(243 + X_, 87 + Y_);

	default: 
		return LOCATION(238 + X_, 208 + Y_);
	}
}

void FightGroupMaker::CreateAttack() 
{
	aiTechManager.RaiseTech();
	std::vector<StartLoc> attackLocs(startLocs.size());
	std::copy(startLocs.begin(), startLocs.end(), attackLocs.begin());
		
	map_id chassisID;
	if (aiTechManager.GetChassisType() != ChassisType::Lynx && TethysGame::GetRand(2) == 0) {
		chassisID = GetChassisID(ChassisType::Lynx);
	}
	else {
		chassisID = GetChassisID(aiTechManager.GetChassisType());
	}

	for (std::size_t i = 0; i < HumanPlayerCount(); ++i)
	{
		const int locationIndex = TethysGame::GetRand(attackLocs.size());
		CreateAttackGroup(attackLocs[locationIndex], chassisID);
		attackLocs.erase(attackLocs.begin() + locationIndex);

		chassisID = GetChassisID(aiTechManager.GetChassisType());
	}

	FightGroupMaker::attackGroupSize += 2;

}

void FightGroupMaker::CreateAttackGroup(const StartLoc& startLoc, map_id chassisID)
{
	FightGroup fightGroup = CreateFightGroup(Player[aiPlayerNum]);
	const ColonyType colonyType = SelectColony();
	
	LOCATION location = startLoc.loc;

	Unit unit;
	for (std::size_t i = 0; i < attackGroupSize; ++i)
	{
		if (startLoc.rotation == South || startLoc.rotation == North) {
			location.x = startLoc.loc.x + i % 4;
			location.y = startLoc.loc.y + i / 4;
		}
		else if(startLoc.rotation == East) {
			location.x = startLoc.loc.x + i / 4;
			location.y = startLoc.loc.y + i % 4;
		}
		else {
			location.x = startLoc.loc.x - i / 4;
			location.y = startLoc.loc.y + i % 4;
		}

		TethysGame::CreateUnit(unit, chassisID, location, aiPlayerNum, SelectTurret(colonyType), startLoc.rotation);
		unit.DoSetLights(true);

		fightGroup.TakeUnit(unit);
	}

	TaskFightGroup(fightGroup);
}

map_id FightGroupMaker::GetChassisID(ChassisType chassisType) 
{
	switch (chassisType)
	{
	case ChassisType::Lynx:
		return map_id::mapLynx;

	case ChassisType::Panther:
		return map_id::mapPanther;
		
	default:
		return map_id::mapTiger;
	}
}

map_id FightGroupMaker::SelectTurret(FightGroupMaker::ColonyType colonyType)
{
	std::vector<TurretRatio> turretRatios;

	if (colonyType == ColonyType::Eden) {
		turretRatios = EdenTurrets.at(aiTechManager.CurrentTurretLevel());
	}
	else {
		turretRatios = PlymouthTurrets.at(aiTechManager.CurrentTurretLevel());
	}
	
	unsigned int percent = TethysGame::GetRand(100);

	for (const auto& turretRatio : turretRatios) {
		if (percent < turretRatio.maxPercent) {
			return turretRatio.turret;
		}
	}

	throw std::runtime_error("Turret type not provided.");
}

FightGroupMaker::ColonyType FightGroupMaker::SelectColony()
{
	return static_cast<ColonyType>(TethysGame::GetRand(2));
}

const std::vector<map_id> miningBuildings {
	map_id::mapCommonOreSmelter, map_id::mapRareOreSmelter,
	map_id::mapCommonOreMine, map_id::mapRareOreMine};

void FightGroupMaker::TaskFightGroup(FightGroup& fightGroup)
{
	int choice = TethysGame::GetRand(4);

	switch (choice)
	{
	case 0: // General Attack
		fightGroup.DoAttackEnemy();
		return;
	case 1: // Attack Command Center
		AttackBuilding(fightGroup, std::vector<map_id>{ map_id::mapCommandCenter });
		return;
	case 2: // Attack Smelter
		AttackBuilding(fightGroup, miningBuildings);
		return;
	case 3: // Attack Cargo Truck Holding Zone
		AttackHoldingZone(fightGroup);
		return;
	}
}

void FightGroupMaker::AttackBuilding(FightGroup& fightGroup, const std::vector<map_id>& buildingTypes)
{
	std::vector<Unit> buildings;
	GetHumanBuildings(buildings, buildingTypes);

	if (buildings.empty())
	{
		fightGroup.DoAttackEnemy();
		return;
	}

	Unit building = buildings[TethysGame::GetRand(buildings.size())];

	fightGroup.SetRect(MAP_RECT(
		building.Location().x - 5, building.Location().y - 5,
		building.Location().x + 5, building.Location().y + 5));

	fightGroupsWithTarget.push_back(FightGroupTarget { fightGroup, building } );
}

void FightGroupMaker::GetHumanBuildings(std::vector<Unit>& buildingsOut, const std::vector<map_id>& buildingTypes)
{
	for (map_id buildingType : buildingTypes) {
		GetHumanBuildings(buildingsOut, buildingType);
	}
}

void FightGroupMaker::GetHumanBuildings(std::vector<Unit>& buildingsOut, map_id buildingType)
{
	Unit building;
	for (std::size_t i = 0; i < HumanPlayerCount(); ++i)
	{
		PlayerBuildingEnum playerBuildingEnum(i, buildingType);

		while (playerBuildingEnum.GetNext(building))
		{
			buildingsOut.push_back(building);
		}
	}
}

void FightGroupMaker::AttackHoldingZone(FightGroup& fightGroup)
{
	LOCATION patrolPoints[3];
	patrolPoints[0] = holdingRect.RandPt();
	patrolPoints[1] = holdingRect.RandPt();
	patrolPoints[2] = LOCATION(-1, -1);

	PatrolRoute patrolRoute;
	patrolRoute.waypoints = patrolPoints;
	fightGroup.SetPatrolMode(patrolRoute);

	fightGroupsHoldingRect.push_back(fightGroup);
}

void FightGroupMaker::LaunchEmpMissile()
{
	if (!aiTechManager.EmpMissileAvailable()) {
		return;
	}

	std::vector<Unit> buildings;
	GetHumanBuildings(buildings, miningBuildings);

	if (buildings.empty()) {
		return;
	}

	Unit& target = buildings[TethysGame::GetRand(buildings.size())];
	TethysGame::SetEMPMissile(0, 0, 0, target.Location().x, target.Location().y);
}

void FightGroupMaker::TestGuardingHoldingZone()
{
	FightGroup fightGroup = CreateFightGroup(Player[aiPlayerNum]);

	Unit unit;
	LOCATION loc(120 + X_, 50 + Y_);

	for (int i = 0; i < 6; i++)
	{
		TethysGame::CreateUnit(unit, mapLynx, loc, aiPlayerNum, mapLaser, East);
		unit.DoSetLights(true);
		fightGroup.TakeUnit(unit);
	}

	AttackHoldingZone(fightGroup);
}

void FightGroupMaker::AttackGroupDebug()
{
	FightGroup fightGroup = CreateFightGroup(Player[aiPlayerNum]);
	FightGroup fightGroupHoldingZone = CreateFightGroup(Player[aiPlayerNum]);
	FightGroup fightGroupMiningBuildings = CreateFightGroup(Player[aiPlayerNum]);

	Unit unit;
	LOCATION loc(20 + X_, 30 + Y_);

	for (int i = 0; i < 6; i++)
	{
		TethysGame::CreateUnit(unit, mapLynx, loc, aiPlayerNum, mapLaser, East);
		unit.DoSetLights(true);
		fightGroup.TakeUnit(unit);
	}

	fightGroup.SetAttackType(mapCommandCenter);
	fightGroup.DoAttackEnemy();

	for (int i = 0; i < 8; i++)
	{
		TethysGame::CreateUnit(unit, mapLynx, loc, aiPlayerNum, mapMicrowave, East);
		unit.DoSetLights(true);
		fightGroupHoldingZone.TakeUnit(unit);
	}

	fightGroupHoldingZone.SetRect(MAP_RECT (162 + X_, 1 + Y_, 190 + X_, 7 + Y_));
	
	for (int i = 0; i < 3; i++)
	{
		TethysGame::CreateUnit(unit, mapLynx, loc, aiPlayerNum, mapRPG, East);
		unit.DoSetLights(true);
		fightGroupMiningBuildings.TakeUnit(unit);
	}

	AttackBuilding(fightGroupMiningBuildings, miningBuildings);
}