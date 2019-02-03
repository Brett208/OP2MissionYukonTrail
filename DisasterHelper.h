#pragma once

// Disaster Helper Version: 1.0

#include "OP2Helper\OP2Helper.h"
#include "Outpost2DLL\Outpost2DLL.h"
#include <climits>
#include <cmath>
#include <vector>

// DisasterHelper simplifies adding random disasters to a map. The default
// settings should be ideal for a basic Last One Standing Map. Many features are 
// customizable such as ratio of occuring disasters, disaster duration, and disaster strength.
// USAGE: Pass the function CreateRandomDisaster to an Outpost 2 time trigger. You will have
//        to wrap CreateRandomDisaster in an Export function since you cannot use the 
//        TYPEDEF Export on functions within C++ classes. Set the time interval on the 
//        trigger for the frequency you want disasters to appear. When a disaster is called
//        for, it will not occur until about 10-15 marks later to allow time for disaster
//        warnings/watches in game if they are researched by the player. Inidiviual functions
//        exist to call for Vortex, Meteor, Storm, or Earthquake using the DisasterHelper's
//        settings if desired over a random disaster.
// MAP LOCATIONS: Ensure all map coordinates passed into DisasterHelper are pre-corrected 
//                for the proper offset. East-West wrap map (x - 1, y - 1). Non wrap (x + 31, y - 1).
// WARNINGS: DisasterHelper can produce warning messages by using the Outpost 2 built in
//           multiplayer chat pane. For these warnings to appear, the code must be compiled
//           in debug mode. For final production code, you should compile in Release mode
//           which will keep the warnings from occuring.
class DisasterHelper
{
public:
	//Tile width and tile height of map. 
	//DisasterHelper uses the setting wrapsEastWest to set proper map coordinate offsets.
	void SetMapProperties(int mapWidth, int mapHeight, bool wrapsEastWest);

	void CreateRandomDisaster();
	void CreateStorm();

	//Medium and Large meteors are created outside SafeAreas if at least one SafeArea is set.
	void CreateMeteor();

	//Earthquakes are created outside of SafeAreas if at least one SafeArea is set.
	void CreateEarthquake();

	//Vortices are created inside VortexCorridors. If no corridors are set, no vortex will be created.
	void CreateVortex();

	//If percent values do not sum to 100, no changes will be made. 
	//Set percent values as a whole number (ie 10 = 10 percent).
	//For including Vortexes, recommended starting percentages are: noDisasterPercent(15),
	//  meteorPercent(35), earthquakePercent(20), stormPercent(20), vortexPercent(10)
	void SetDisasterPercents(int noDisasterPercent, int meteorPercent,
		int earthquakePercent, int stormPercent, int vortexPercent);

	//If percent values do not sum to 100, no changes will be made. 
	//Set percent values as a whole number (ie 10 = 10 percent).
	void SetMeteorSizePercents(int smallMeteorPercent, int mediumMeteorPercent, int largeMeteorPercent);

	//Min earthquake strength is 0.
	void SetEarthquakeStrength(int minStrength, int maxStrength)
	{
		if (minStrength > maxStrength || minStrength < 0)
		{
			return;
		}

		this->minQuakeStrength = minStrength;
		this->maxQuakeStrength = maxStrength;
	}

	//Duration is represented by number of marks
	void SetVortexDuration(int minDuration, int maxDuration)
	{
		if (minDuration > maxDuration || minDuration < 0)
		{
			return;
		}

		this->minVortexDuration = minDuration;
		this->maxVortexDuration = maxDuration;
	}

	//Duration is represented by number of marks
	void SetStormDuration(int minDuration, int maxDuration)
	{
		if (minDuration > maxDuration || minDuration < 0)
		{
			return;
		}

		this->minStormDuration = minDuration;
		this->maxStormDuration = maxDuration;
	}

	//Sets a timer value in marks where base safeZones expire, allowing more dangerous disasters to hit bases. 
	//To make SafeZones never expire, set timeInMarks to the const TimerNeverExpires.
	//To disable SafeZones without clearing them from DisasterHelper, set timeInMarks <= 0
	void SetSafeZoneExpirationTimer(int timeInMarks)
	{
		this->safeZoneTimer = timeInMarks;
	}

	void ClearSafeRects()
	{
		safeRects.clear();
	}

	//Add a MAP_RECT where no earthquakes or large meteors will be created.
	//This is designed to protect starting locations from powerful disasters.
	void AddSafeRect(const MAP_RECT& safeRect)
	{
		safeRects.push_back(safeRect);
	}

	//Add a c style array of MAP_RECTs where no earthquakes or large meteors will be created.
	//This is designed to protect starting locations from powerful disasters.
	void AddSafeRects(int arraySize, MAP_RECT safeRects[])
	{
		for (int i = 0; i < arraySize; i++)
		{
			this->safeRects.push_back(safeRects[i]);
		}
	}

	template<typename MAP_RECT, size_t N>
	void AddSafeRects(const std::array<MAP_RECT, N>& safeRects)
	{
		this->safeRects.insert(this->safeRects.end(), safeRects.begin(), safeRects.end());
	}

	void AddSafeRects(const std::vector<MAP_RECT>& safeRects)
	{
		this->safeRects.insert(this->safeRects.end(), safeRects.begin(), safeRects.end());
	}

	void ClearVortexCorridors()
	{
		vortexRects.clear();
	}

	//corridorWeight represents how often the corridor is chosen randomly.
	//IE: adding one corridor with a weight of 1 and one corridor with a weight of 2
	//    means the second vortex corridor will be used twice as often.
	//Vortex corridors will produce vortices within Safe Areas if they overlap on the map.
	//Small vortex corridors will force the vortex to not travel very far, but stay mostly stationary.
	//Min vortex corridor width is 5 and height is 5.
	void AddVortexCorridor(const MAP_RECT& mapRect, int corridorWeight);

	// Attempts to return a LOCATION that exists outside any declared safe areas. 
	// If unable to find a valid LOCATION, reports a warning in debug mode and returns (0,0).
	LOCATION GetRandLocOutsideSafeAreas();

	bool MapPropertiesSet()
	{
		return mapWidth != 0;
	}

	static const int TimerNeverExpires = INT_MAX;
	static const int TimerDefaultValue = 250; //In Marks

private:
	int xOffset = 31;
	int yOffset = -1;

	int mapWidth;
	int mapHeight;

	int noDisasterPercent = 15;
	int meteorPercent = 40;
	int earthquakePercent = 22;
	int stormPercent = 23;
	int vortexPercent = 0;

	int smallMeteorPercent = 60;
	int mediumMeteorPercent = 25;
	int largeMeteorPercent = 15;

	int minQuakeStrength = 0;
	int maxQuakeStrength = 3;

	int minVortexDuration = 5;
	int maxVortexDuration = 20;

	int minStormDuration = 10;
	int maxStormDuration = 55;

	int safeZoneTimer = TimerDefaultValue;

	std::vector<MAP_RECT> safeRects;
	std::vector<MAP_RECT> vortexRects;

	LOCATION GetRandMapLoc();
	double DistanceBetweenPoints(LOCATION loc1, LOCATION loc2);	
	bool DisasterHelper::IsLocInSafeArea(LOCATION& loc);
	LOCATION DisasterHelper::FindVortexEndLoc(const MAP_RECT& vortexCorridor,
		const LOCATION& startLoc, double minPercentHypotenuseTravel);

	void SendDebugMessage(char* message);
};
