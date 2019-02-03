#include "DisasterHelper.h"

void DisasterHelper::SetMapProperties(int mapWidth, int mapHeight, bool wrapsEastWest)
{
	if (mapWidth <= 0)
	{
		SendDebugMessage("Map width cannot be set to 0 or negative.");
		return;
	}

	if (mapHeight <= 0)
	{
		SendDebugMessage("Map height cannot be set to 0 or negative.");
		return;
	}

	this->mapWidth = mapWidth;
	this->mapHeight = mapHeight;

	if (wrapsEastWest)
	{
		xOffset = -1;
		yOffset = -1;
	}
}

void DisasterHelper::SetDisasterPercents(int noDisasterPercent, int meteorPercent, 
	int earthquakePercent, int stormPercent, int vortexPercent)
{
	if (noDisasterPercent + meteorPercent + earthquakePercent + stormPercent + vortexPercent != 100)
	{
		SendDebugMessage("SetDisasterPercents: The sum of percents must = 100.");
		return;
	}

	if (noDisasterPercent < 0 || meteorPercent < 0 || earthquakePercent < 0 || stormPercent < 0 || vortexPercent < 0)
	{
		SendDebugMessage("SetDisasterPercents: No value may be < 0.");
		return;
	}

	this->noDisasterPercent = noDisasterPercent;
	this->meteorPercent = meteorPercent;
	this->earthquakePercent = earthquakePercent;
	this->stormPercent = stormPercent;
	this->vortexPercent = vortexPercent;
}

void DisasterHelper::SetMeteorSizePercents(int smallMeteorPercent, int mediumMeteorPercent, int largeMeteorPercent)
{
	if (smallMeteorPercent + mediumMeteorPercent + largeMeteorPercent != 100)
	{
		SendDebugMessage("SetMeteorSizePercents: The sum of percents must = 100.");
		return;
	}

	if (smallMeteorPercent < 0 || mediumMeteorPercent < 0 || largeMeteorPercent < 0)
	{
		SendDebugMessage("SetMeteorSizePercents No value may be < 0.");
		return;
	}

	this->smallMeteorPercent = smallMeteorPercent;
	this->mediumMeteorPercent = mediumMeteorPercent;
	this->largeMeteorPercent = largeMeteorPercent;
}

void DisasterHelper::AddVortexCorridor(const MAP_RECT& mapRect, int corridorWeight)
{
	if (mapRect.Width() < 5 || mapRect.Height() < 5)
	{
		SendDebugMessage("Vortex corridor width & height must be > 5.");
		return;
	}

	if (corridorWeight <= 0)
	{
		SendDebugMessage("Vortex corridor weight must be > 0.");
		return;
	}

	for (int i = 0; i < corridorWeight; i++)
	{
		vortexRects.push_back(mapRect);
	}
}

LOCATION DisasterHelper::GetRandMapLoc()
{
	if (!MapPropertiesSet())
	{
		SendDebugMessage("DisasterHelper map properties are not set.");
		return LOCATION(0, 0);
	}

	return LOCATION(TethysGame::GetRand(mapWidth) + xOffset, TethysGame::GetRand(mapHeight) + yOffset);
}

bool DisasterHelper::IsLocInSafeArea(LOCATION& loc)
{
	if (TethysGame::Time() / 100 >= safeZoneTimer)
	{
		return false;
	}

	for (auto& mapRect : safeRects)
	{
		if (mapRect.Check(loc))
		{
			return true;
		}
	}

	return false;
}

LOCATION DisasterHelper::GetRandLocOutsideSafeAreas()
{
	if (!MapPropertiesSet())
	{
		SendDebugMessage("DisasterHelper map properties are not set.");
		return LOCATION(0, 0);
	}

	LOCATION loc;
	bool LocInSafeArea = true;
	int numberOfLocsChecked = 0;
	while (LocInSafeArea)
	{
		loc = GetRandMapLoc();

		if (!IsLocInSafeArea(loc))
		{
			break;
		}

		numberOfLocsChecked++;

		if (numberOfLocsChecked > 40)
		{
			SendDebugMessage("Unable to find a LOCATION outside of safe areas.");
			return LOCATION(0, 0);
		}
	}

	return loc;
}

double DisasterHelper::DistanceBetweenPoints(LOCATION loc1, LOCATION loc2)
{
	return std::sqrt(std::pow(loc2.x - loc1.x, 2) + std::pow(loc2.y - loc1.y, 2));
}

void DisasterHelper::CreateRandomDisaster()
{
	int randNum = TethysGame::GetRand(100);

	if (randNum < noDisasterPercent)
	{
		// No Disaster this cycle
		return;
	}
	else if (randNum < noDisasterPercent + meteorPercent)
	{
		CreateMeteor();
	}
	else if (randNum < noDisasterPercent + meteorPercent + earthquakePercent)
	{
		CreateEarthquake();
	}
	else if (randNum < noDisasterPercent + meteorPercent + earthquakePercent + stormPercent)
	{
		CreateStorm();
	}
	else if (randNum < noDisasterPercent + meteorPercent + earthquakePercent + stormPercent + vortexPercent)
	{
		CreateVortex();
	}
}

void DisasterHelper::CreateEarthquake()
{
	if (!MapPropertiesSet())
	{
		SendDebugMessage("DisasterHelper map properties are not set.");
		return;
	}

	LOCATION loc = GetRandLocOutsideSafeAreas();
	int quakeStrength = minQuakeStrength + TethysGame::GetRand(maxQuakeStrength - minQuakeStrength);
	TethysGame::SetEarthquake(loc.x, loc.y, quakeStrength);
}

//minPercentHypotenuseTravel is the minimum percentage of the MAP_RECT's hypotenuse's size that the line must travel.
LOCATION DisasterHelper::FindVortexEndLoc(const MAP_RECT& mapRect, const LOCATION& startLoc, double minPercentHypotenuseTravel)
{
	double minVortexTravelDistance = DistanceBetweenPoints(
		LOCATION(mapRect.x1, mapRect.y1),
		LOCATION(mapRect.x2, mapRect.y2)) * (minPercentHypotenuseTravel / 100);

	LOCATION endLoc = mapRect.RandPt();

	while (DistanceBetweenPoints(startLoc, endLoc) < minVortexTravelDistance)
	{
		endLoc = mapRect.RandPt();
	}

	return endLoc;
}

void DisasterHelper::CreateVortex()
{
	if (vortexRects.size() == 0)
	{
		SendDebugMessage("No vortex corridors provided.");
		return;
	}

	MAP_RECT vortexRect = vortexRects[TethysGame::GetRand(vortexRects.size())];

	LOCATION vortexStartLoc = vortexRect.RandPt();
	LOCATION vortexEndLoc = FindVortexEndLoc(vortexRect, vortexStartLoc, 33);

	TethysGame::SetTornado(
		vortexStartLoc.x,
		vortexStartLoc.y,
		minVortexDuration + TethysGame::GetRand(maxVortexDuration - minVortexDuration),
		vortexEndLoc.x,
		vortexEndLoc.x,
		false);
}

void DisasterHelper::CreateStorm()
{
	if (!MapPropertiesSet())
	{
		SendDebugMessage("DisasterHelper map properties are not set.");
		return;
	}

	//NOTE: Electrical storms will conintue beyond their end point if duration permits.
	LOCATION startLoc = GetRandMapLoc();
	LOCATION endLoc = GetRandMapLoc();

	int stormDuration = minStormDuration + TethysGame::GetRand(maxStormDuration - minStormDuration);

	TethysGame::SetLightning(startLoc.x, startLoc.y, stormDuration, endLoc.x, endLoc.y);
}

void DisasterHelper::CreateMeteor()
{
	if (!MapPropertiesSet())
	{
		SendDebugMessage("DisasterHelper map properties are not set.");
		return;
	}

	LOCATION disasterLoc = GetRandMapLoc();

	int randNumber = TethysGame::GetRand(100);
	int meteorSize = 0;

	if (randNumber <= smallMeteorPercent)
	{
		meteorSize = 0;
	}
	else if (randNumber <= smallMeteorPercent + mediumMeteorPercent)
	{
		meteorSize = 1;
		disasterLoc = GetRandLocOutsideSafeAreas();
	}
	else if (randNumber <= smallMeteorPercent + mediumMeteorPercent + largeMeteorPercent)
	{
		meteorSize = 2;
		disasterLoc = GetRandLocOutsideSafeAreas();
	}

	TethysGame::SetMeteor(disasterLoc.x, disasterLoc.y, meteorSize);
}

void DisasterHelper::SendDebugMessage(char* message)
{
#if _DEBUG
	Unit unit;
	TethysGame::AddMessage(unit, message, PlayerNum::Player0, SoundID::sndBeep8);
#endif
}
