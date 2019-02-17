#include "AITechManager.h"

const std::vector<AITechManager::TechOption> AITechManager::allTech
{
	TechOption{techMobileWeaponsPlatform, std::vector<TechID>{techLargeScaleOpticalResonators, techMicrowaveTurret, techDissipatingAdhesives}, false, false},
	
	TechOption{techLargeScaleOpticalResonators, std::vector<TechID>{techIndependentTurretPowerSystems}, false, false},
	TechOption{techLargeScaleOpticalResonators, std::vector<TechID>{techScoutClassDriveTrainRefit, techHeatDissipationSystemsPlymouth}, false, false},

	TechOption{techIndependentTurretPowerSystems, std::vector<TechID>{techAdvancedCombatChassis}, false, true},

	TechOption{techAdvancedCombatChassis, std::vector<TechID>{techAdvancedArmoringSystems}, false, false},
	//TechOption{techAdvancedCombatChassis, std::vector<TechID>{techReinforcedPantherConstruction}},
	//TechOption{techAdvancedCombatChassis, std::vector<TechID>{techPatherDriveTrainRefit}},
	TechOption{techAdvancedCombatChassis, std::vector<TechID>{techElectromagneticPulsing, techDirectionalMagneticFields, techRocketPropulsion}, true, false},
	
	TechOption{techElectromagneticPulsing, std::vector<TechID>{techRocketAtmosphericReEntrySystem}, false, false},
	TechOption{techElectromagneticPulsing, std::vector<TechID>{techDualTurretWeaponsSystems}, false, true},
	TechOption{techDualTurretWeaponsSystems, std::vector<TechID>{techTigerSpeedModification}, false, false},

	TechOption{techDirectionalMagneticFields, std::vector<TechID>{techIncreasedCapacitanceCircuitry, techHighPoweredExplosives}, false, false},

	TechOption{techIncreasedCapacitanceCircuitry, std::vector<TechID>{techMultipleMineProjectileSystem, techArtificialLightning, techAcidWeaponry}, true, false},
	TechOption{techArtificialLightning, std::vector<TechID>{techHeatDissipationSystemsEden, techReducedFoamEvaporation}, false, false},
	TechOption{techMultipleMineProjectileSystem, std::vector<TechID>{techExtendedRangeProjectileLauncherPlymouth, techExtendedRangeProjectileLauncherEden}, false, false},
	TechOption{techExtendedRangeProjectileLauncherEden, std::vector<TechID>{techGrenadeLoadingMechanismEden, techGrenadeLoadingMechanismPlymouth}, false, false},
};

AITechManager::AITechManager(PlayerNum aiPlayerIndex) :
	aiPlayerIndex(aiPlayerIndex),
	unusedTech(allTech),
	turretLevel(TurretLevel::Initial),
	chassisType(ChassisType::Lynx)
{
	Player[aiPlayerIndex].MarkResearchComplete(TechID::techMobileWeaponsPlatform);
}

void AITechManager::RaiseTech()
{
	const std::vector<TechOption> availableTech = GetAvailableTech();

	if (availableTech.empty()) {
		return;
	}

	const int techIndex = TethysGame::GetRand(availableTech.size());
	const TechOption techOption = availableTech[techIndex];
	unusedTech.erase(unusedTech.begin() + techIndex);

	if (techOption.raiseTurretLevel) {
		turretLevel = static_cast<TurretLevel>(static_cast<int>(turretLevel) + 1);
	}

	if (techOption.raiseChassisLevel) {
		chassisType = static_cast<ChassisType>(static_cast<int>(chassisType) + 1);
	}

	for (const auto& techID : techOption.gainedTech) {
		Player[aiPlayerIndex].MarkResearchComplete(techID);
	}

}

std::vector<AITechManager::TechOption> AITechManager::GetAvailableTech()
{
	std::vector<TechOption> availableTech;

	for (const auto& techOption : unusedTech)
	{
		if (Player[aiPlayerIndex].HasTechnology(techOption.prerequisite)) {
			availableTech.push_back(techOption);
		}
	}

	return availableTech;
}

TurretLevel AITechManager::CurrentTurretLevel() const {
	return turretLevel;
}

ChassisType AITechManager::GetChassisType() const
{
	return chassisType;
}

bool AITechManager::EmpMissileAvailable() const
{
	return Player[aiPlayerIndex].HasTechnology(TechID::techRocketAtmosphericReEntrySystem);
}
