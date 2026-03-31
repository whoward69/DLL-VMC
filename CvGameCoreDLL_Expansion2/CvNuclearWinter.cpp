#include "CvNuclearWinter.h"
#include "CvGameCoreDLLPCH.h"

#ifdef MOD_NUCLEAR_WINTER_FOR_SP

bool CvNuclearWinterLevel::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if (!CvBaseInfo::CacheResults(kResults, kUtility))
	{
		return false;
	}
	m_iTriggerThreshold = kResults.GetInt("TriggerThreshold");

	m_aiYieldRateTimes100.clear();
	m_aiYieldRateTimes100.resize(NUM_YIELD_TYPES);
	m_aiYieldModifierTimes100.clear();
	m_aiYieldModifierTimes100.resize(NUM_YIELD_TYPES);
	for(int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		m_aiYieldRateTimes100[iI] = 0;
		m_aiYieldModifierTimes100[iI] = 0;
	}
	
	const char* szNuclearWinterType = GetType();
	{
		std::string strKey("NuclearWinterLevel_GlobalYield");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Yields.ID as YieldID, Yield from NuclearWinterLevel_GlobalYield inner join Yields on Yields.Type = YieldType where NuclearWinterLevelType = ?");
		}

		pResults->Bind(1, szNuclearWinterType);

		while(pResults->Step())
		{
			const int YieldID = pResults->GetInt(0);
			const int Yield = pResults->GetInt(1);
			m_aiYieldRateTimes100[YieldID] = Yield;
		}
	}
	{
		std::string strKey("NuclearWinterLevel_GlobalYieldModifier");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Yields.ID as YieldID, Yield from NuclearWinterLevel_GlobalYieldModifier inner join Yields on Yields.Type = YieldType where NuclearWinterLevelType = ?");
		}

		pResults->Bind(1, szNuclearWinterType);

		while(pResults->Step())
		{
			const int YieldID = pResults->GetInt(0);
			const int Yield = pResults->GetInt(1);
			m_aiYieldModifierTimes100[YieldID] = Yield;
		}
	}
}

int CvNuclearWinterLevel::GetTriggerThreshold()
{
	return m_iTriggerThreshold;
}

int CvNuclearWinterLevel::GetYieldTimes100(int iYieldIndex) const
{
	return m_aiYieldRateTimes100[iYieldIndex];
}
int CvNuclearWinterLevel::GetYieldModifierTimes100(int iYieldIndex) const
{
	return m_aiYieldModifierTimes100[iYieldIndex];
}

std::vector<CvNuclearWinterLevel*>& CvNuclearWinterLevelXMLEntries::GetEntries()
{
	return m_vNuclearWinterLevelEntries;
}

CvNuclearWinterLevel* CvNuclearWinterLevelXMLEntries::GetEntry(NuclearWinterLevelTypes eLevel)
{
	int index = static_cast<int>(eLevel);
	if (index < 0 || index >= m_vNuclearWinterLevelEntries.size())
	{
		return nullptr;
	}

	return m_vNuclearWinterLevelEntries[index];
}

#endif
