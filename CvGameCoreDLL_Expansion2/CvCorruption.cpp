#include "CvCorruption.h"
#include "CvCity.h"
#include "CvMap.h"
#include "CvGameCoreDLLPCH.h"

#ifdef MOD_GLOBAL_CORRUPTION

bool CvCorruptionLevel::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if (!CvBaseInfo::CacheResults(kResults, kUtility))
	{
		return false;
	}

	m_iScoreLowerBoundBase = kResults.GetInt("ScoreLowerBoundBase");
	m_iMapWidthModifier = kResults.GetInt("MapWidthModifier");
	m_iMapHeightModifier = kResults.GetInt("MapHeightModifier");

	m_bIsPuppet = kResults.GetBool("IsPuppet");
	m_bIsCapital = kResults.GetBool("IsCapital");

	m_eCityHallBuildingClass = static_cast<BuildingClassTypes>(GC.getInfoTypeForString(kResults.GetText("CityHallBuildingClass")));
	m_ePublicSecurityBuildingClass = static_cast<BuildingClassTypes>(GC.getInfoTypeForString(kResults.GetText("PublicSecurityBuildingClass")));
}

int CvCorruptionLevel::GetScoreLowerBound(int iMapWidth, int iMapHeight) const
{
	return m_iScoreLowerBoundBase + iMapWidth * m_iMapWidthModifier / 100 + iMapHeight * m_iMapHeightModifier / 100;
}

int CvCorruptionLevel::GetScoreLowerBoundBase() const
{
	return m_iScoreLowerBoundBase;
}

int CvCorruptionLevel::GetScoreLowerBoundModFromMapSize(int iMapWidth, int iMapHeight) const
{
	return iMapWidth * m_iMapWidthModifier / 100 + iMapHeight * m_iMapHeightModifier / 100;
}

bool CvCorruptionLevel::IsCapital() const
{
	return m_bIsCapital;
}

bool CvCorruptionLevel::IsPuppet() const
{
	return m_bIsPuppet;
}

BuildingClassTypes CvCorruptionLevel::GetCityHallBuildingClass() const
{
	return m_eCityHallBuildingClass;
}

BuildingClassTypes CvCorruptionLevel::GetPublicSecurityBuildingClass() const
{
	return m_ePublicSecurityBuildingClass;
}

std::vector<CvCorruptionLevel*>& CvCorruptionLevelXMLEntries::GetEntries()
{
	return m_vCorruptionLevelEntries;
}

CvCorruptionLevel* CvCorruptionLevelXMLEntries::GetEntry(CorruptionLevelTypes eLevel)
{
	int index = static_cast<int>(eLevel);
	if (index < 0 || index >= m_vCorruptionLevelEntries.size())
	{
		return nullptr;
	}

	return m_vCorruptionLevelEntries[index];
}

#endif
