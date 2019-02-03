#pragma once

#include "OP2Helper/OP2Helper.h"
#include "Outpost2DLL/Outpost2DLL.h"
#include <vector>

enum class TurretLevel
{
	Initial, //microwave, sticky, laser
	Intermediate, //RPG, EMP, Railgun
	Advanced //ESG, Cloud, Thor
};

enum class ChassisType
{
	Lynx,
	Panther,
	Tiger
};

class AITechManager {
public:
	AITechManager(PlayerNum aiPlayerIndex);

	void RaiseTech();
	bool EmpMissileAvailable() const;
	TurretLevel CurrentTurretLevel() const;
	ChassisType GetChassisType() const;
private:
	struct TechOption
	{
		TechID prerequisite;
		std::vector<TechID> gainedTech;
		bool raiseTurretLevel;
		bool raiseChassisLevel;
	};

	static const std::vector<TechOption> allTech;
	
	const PlayerNum aiPlayerIndex;
	std::vector<TechOption> unusedTech;
	TurretLevel turretLevel;
	ChassisType chassisType;

	std::vector<TechOption> GetAvailableTech();
};
