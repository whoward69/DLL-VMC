#pragma once

#include "CustomMods.h"
#include "CvInfos.h"
#include "CvEnums.h"
#include "CvGameCoreDLLPCH.h"

#ifdef MOD_NUCLEAR_WINTER_FOR_SP

class CvNuclearWinterLevel : public CvBaseInfo
{
public:
    bool CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility) override;

	int GetTriggerThreshold();
	int GetYieldTimes100(int iYieldIndex) const;
    int GetYieldModifierTimes100(int iYieldIndex) const;

private:
	int m_iTriggerThreshold = 0;
	std::vector<int> m_aiYieldRateTimes100;
	std::vector<int> m_aiYieldModifierTimes100;
};

class CvNuclearWinterLevelXMLEntries
{
public:
	std::vector<CvNuclearWinterLevel*>& GetEntries();
	CvNuclearWinterLevel* GetEntry(NuclearWinterLevelTypes eLevel);

private:
	std::vector<CvNuclearWinterLevel*> m_vNuclearWinterLevelEntries;
};

#endif