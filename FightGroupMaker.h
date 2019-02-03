#pragma once

#include "AITechManager.h"
#include "Outpost2DLL/Outpost2DLL.h"
#include <vector>
#include <cstddef>
#include <map>

class FightGroupMaker {
public:
	FightGroupMaker(PlayerNum aiPlayerNum, MAP_RECT holdingRect);

	void CreatePatrolGroups();
	void CreateAttack();
	void LaunchEmpMissile();
	void UpdateFightGroups();

private:
	enum class ColonyType
	{
		Eden = 0,
		Plymouth
	};

	struct TurretRatio 
	{
		map_id turret;
		unsigned int maxPercent;
	};

	struct StartLoc {
		LOCATION loc;
		UnitDirection rotation;
	};

	struct FightGroupTarget
	{
		FightGroup fightGroup;
		Unit building;
	};

	static const std::map<TurretLevel, std::vector<TurretRatio>> PlymouthTurrets;
	static const std::map<TurretLevel, std::vector<TurretRatio>> EdenTurrets;
	static const std::array<FightGroupMaker::StartLoc, 8> startLocs;
	
	
	const PlayerNum aiPlayerNum;
	AITechManager aiTechManager;
	map_id GetChassisID(ChassisType chassisType);
	std::size_t attackGroupSize;
	std::vector<FightGroupTarget> fightGroupsWithTarget;
	std::vector<FightGroup> fightGroupsHoldingRect;
	MAP_RECT holdingRect;

	inline std::size_t HumanPlayerCount() const
	{
		return aiPlayerNum;
	}

	LOCATION GetPatrolLoc(UnitDirection rotation);
	void CreatePatrolGroup();
	void CreateAttackGroup(const StartLoc& startLoc, map_id chassisID);
	void TaskFightGroup(FightGroup& fightGroup);
	void AttackBuilding(FightGroup& fightGroup, const std::vector<map_id>& buildingTypes);
	void AttackHoldingZone(FightGroup& fightGroup);
	void CheckTargets();
	void CheckHoldingZone();
	map_id SelectTurret(ColonyType colonyType);
	ColonyType SelectColony();
	void GetHumanBuildings(std::vector<Unit>& buildingsOut, map_id buildingType);
	void GetHumanBuildings(std::vector<Unit>& buildingsOut, const std::vector<map_id>& buildingTypes);
	void AttackGroupDebug();
	void FightGroupMaker::TestGuardingHoldingZone();
};




