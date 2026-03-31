#include "CvGameCoreDLLPCH.h"
#include "CvPromotionCollectionClasses.h"

#ifdef MOD_PROMOTION_COLLECTIONS

#define DEBUG_CvPromotionCollectionEntry

bool CvPromotionCollectionEntry::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
    if (!CvBaseInfo::CacheResults(kResults, kUtility))
        return false;

#ifdef DEBUG_CvPromotionCollectionEntry
	CUSTOMLOG("CvPromotionCollectionEntry::CacheResults start!");
#endif
	const char* szThisType = CvBaseInfo::GetType();
	const size_t lenThisType = strlen(szThisType);

// ------------------------------------------------------------------------
	{
		std::string strKey = "PromotionCollections - Entries";
		Database::Results* pResults = kUtility.GetResults(strKey);
		if (pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select *, t2.ID as PromotionID from PromotionCollections_Entries t1 "
            "left join UnitPromotions t2 on t1.PromotionType = t2.Type where t1.CollectionType = ? order by t1.PromotionIndex;");
		}

		pResults->Bind(1, szThisType, lenThisType, false);

		while (pResults->Step())
		{
            PromotionEntry entry;
            entry.m_ePromotionType = (PromotionTypes) pResults->GetInt("PromotionID");
            entry.m_iIndex = pResults->GetInt("PromotionIndex");
            entry.m_kTriggerInfo.m_bMeleeAttack = pResults->GetBool("TriggerMeleeAttack");
            entry.m_kTriggerInfo.m_bRangedAttack = pResults->GetBool("TriggerRangedAttack");
            entry.m_kTriggerInfo.m_bMeleeDefense = pResults->GetBool("TriggerMeleeDefense");
            entry.m_kTriggerInfo.m_bRangedDefense = pResults->GetBool("TriggerRangedDefense");
            entry.m_kTriggerInfo.m_iHPFixed = pResults->GetInt("TriggerHPFixed");
            entry.m_kTriggerInfo.m_iHPPercent = pResults->GetInt("TriggerHPPercent");
            entry.m_kTriggerInfo.m_bLuaCheck = pResults->GetBool("TriggerLuaCheck");
            entry.m_kTriggerInfo.m_bLuaHook = pResults->GetBool("TriggerLuaHook");

            m_vPromotions.push_back(entry);
		}
		pResults->Reset();
	}

    {
		std::string strKey = "PromotionCollections - AddEnemyPromotionPools";
		Database::Results* pResults = kUtility.GetResults(strKey);
		if (pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select PromotionCollections.ID as OtherID from PromotionCollections_AddEnemyPromotions "
                "left join PromotionCollections on PromotionCollections_AddEnemyPromotions.OtherCollectionType = PromotionCollections.Type where CollectionType = ?;");
		}

		pResults->Bind(1, szThisType, lenThisType, false);

		while (pResults->Step())
		{
			m_vAddEnemyPromotionPools.push_back((PromotionCollectionsTypes)pResults->GetInt("OtherID"));
		}
		pResults->Reset();
	}

	m_bStackingFightBack = kResults.GetBool("StackingFightBack");
	m_bStopAttacker = kResults.GetBool("StopAttacker");
// ------------------------------------------------------------------------

#ifdef DEBUG_CvPromotionCollectionEntry
	CUSTOMLOG("CvPromotionCollectionEntry::CacheResults end!");
#endif

	return true;
}

#endif