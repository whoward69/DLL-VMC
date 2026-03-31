#pragma once

#include "CustomMods.h"
#include "CvInfos.h"
#include "CvEnums.h"
#include "CvGameCoreDLLPCH.h"

#ifdef MOD_GLOBAL_CORRUPTION

class CvCorruptionLevel : public CvBaseInfo {
public:
    bool CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility) override;

	int GetScoreLowerBound(int iMapWidth, int iMapHeight) const;
	int GetScoreLowerBoundBase() const;
	int GetScoreLowerBoundModFromMapSize(int iMapWidth, int iMapHeight) const;

	bool IsCapital() const;
	bool IsPuppet() const;

	BuildingClassTypes GetCityHallBuildingClass() const;
	BuildingClassTypes GetPublicSecurityBuildingClass() const;

private:
	int m_iScoreLowerBoundBase = 0;
	int m_iMapWidthModifier = 0;
	int m_iMapHeightModifier = 0;

	bool m_bIsCapital = false;
	bool m_bIsPuppet = false;

	BuildingClassTypes m_eCityHallBuildingClass = NO_BUILDINGCLASS;
	BuildingClassTypes m_ePublicSecurityBuildingClass = NO_BUILDINGCLASS;
};

class CvCorruptionLevelXMLEntries {
public:
	std::vector<CvCorruptionLevel*>& GetEntries();
	CvCorruptionLevel* GetEntry(CorruptionLevelTypes eLevel);

private:
	std::vector<CvCorruptionLevel*> m_vCorruptionLevelEntries;
};

#endif
