#include "CvIndependentPromotionInfo.h"
#include "CvGameCoreDLLPCH.h"
#include "CvGameCoreUtils.h"
void CvIndependentPromotionInfo::Init()
{
    CvDatabaseUtility kUtility;
    {
		m_vAllyCityStateCombatModifiers.clear();
		std::string strKey("AllyCityStateCombatModifier");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select ID from UnitPromotions where AllyCityStateCombatModifier != 0");
		}
		while(pResults->Step())
		{
			const int iPromotionID = pResults->GetInt(0);
            m_vAllyCityStateCombatModifiers.push_back((PromotionTypes)iPromotionID);
		}
	}
	{
		m_vHappinessCombatModifiers.clear();
        std::string strKey("ExtraHappinessCombatModifier");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select ID from UnitPromotions where ExtraHappinessCombatModifier != 0");
		}
		while(pResults->Step())
		{
			const int iPromotionID = pResults->GetInt(0);
            m_vHappinessCombatModifiers.push_back((PromotionTypes)iPromotionID);
		}
    }
    {
		m_vResourceCombatModifiers.clear();
		std::string strKey("ExtraResourceCombatModifier");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select ID from UnitPromotions where ExtraResourceCombatModifier != 0 and ExtraResourceType != ''");
		}
		while(pResults->Step())
		{
			const int iPromotionID = pResults->GetInt(0);
            m_vResourceCombatModifiers.push_back((PromotionTypes)iPromotionID);
		}
	}
    {
		m_vNearbyUnitPromotionBonuses.clear();
        std::string strKey("NearbyUnitPromotionBonus");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select ID from UnitPromotions where NearbyUnitPromotionBonus != 0 and CombatBonusFromNearbyUnitPromotion != ''");
		}
		while(pResults->Step())
		{
			const int iPromotionID = pResults->GetInt(0);
            m_vNearbyUnitPromotionBonuses.push_back((PromotionTypes)iPromotionID);
		}
    }
}
#define CollectIndependentPromotion(vTotalPromotions) \
	if(vTotalPromotions.size() < 1) return 0; \
	std::vector<PromotionTypes> vPromotionsUnitHas; \
	for(auto& ePromotion : vTotalPromotions) if(pUnit.isHasPromotion(ePromotion)) vPromotionsUnitHas.push_back(ePromotion); \
	if(vPromotionsUnitHas.size() < 1) return 0;
int CvIndependentPromotionInfo::GetAllyCityStateCombatModifier(const CvUnit& pUnit) const
{
	CollectIndependentPromotion(m_vAllyCityStateCombatModifiers);
	int iModifier = 0;
	int iNumAllyCityState = GET_PLAYER(pUnit.getOwner()).GetNumCSAllies();
	if(iNumAllyCityState <= 0) return 0;
	for(auto& ePromotion : vPromotionsUnitHas)
	{
		CvPromotionEntry* pPromotion = GC.getPromotionInfo(ePromotion);
		int iModifierMax = pPromotion->GetAllyCityStateCombatModifierMax();
		if(iModifierMax <= 0) iModifierMax = 0x7FFFFFFF;
		iModifier += std::min(iNumAllyCityState * pPromotion->GetAllyCityStateCombatModifier(), iModifierMax);
	}
    return iModifier;
}
int CvIndependentPromotionInfo::GetHappinessCombatModifier(const CvUnit& pUnit) const
{
	CollectIndependentPromotion(m_vHappinessCombatModifiers);
	int iModifier = 0;
	int iHappiness = GET_PLAYER(pUnit.getOwner()).GetExcessHappiness();
	if(iHappiness <= 0) return 0;
	for(auto& ePromotion : vPromotionsUnitHas)
	{
		CvPromotionEntry* pPromotion = GC.getPromotionInfo(ePromotion);
		int iModifierMax = pPromotion->GetExtraHappinessCombatModifierMax();
		if(iModifierMax <= 0) iModifierMax = 0x7FFFFFFF;
		iModifier += std::min(iHappiness * pPromotion->GetExtraHappinessCombatModifier(), iModifierMax);
	}
    return iModifier;
}
int CvIndependentPromotionInfo::GetResourceCombatModifier(const CvUnit& pUnit) const
{
	CollectIndependentPromotion(m_vResourceCombatModifiers);
	int iModifier = 0;
	CvPlayerAI& pPlayer = GET_PLAYER(pUnit.getOwner());
	for(auto& ePromotion : vPromotionsUnitHas)
	{
		CvPromotionEntry* pPromotion = GC.getPromotionInfo(ePromotion);
		int iModifierMax = pPromotion->GetExtraResourceCombatModifierMax();
		if(iModifierMax <= 0) iModifierMax = 0x7FFFFFFF;
		iModifier += std::min(pPlayer.getNumResourceTotal(pPromotion->GetExtraResourceType()) * pPromotion->GetExtraResourceCombatModifier(), iModifierMax);
	}
    return iModifier;
}
int CvIndependentPromotionInfo::GetNearbyUnitPromotionBonus(const CvUnit& pUnit) const
{
	CollectIndependentPromotion(m_vNearbyUnitPromotionBonuses);
	int iModifier = 0;
	for(auto& ePromotion : vPromotionsUnitHas)
	{
		CvPromotionEntry* pPromotion = GC.getPromotionInfo(ePromotion);
		PromotionTypes eTargetPromotion = pPromotion->GetCombatBonusFromNearbyUnitPromotion();
		int iUnitPromotionRange = pPromotion->GetNearbyUnitPromotionBonusRange();
		int iUnitPromotionModifier = pPromotion->GetNearbyUnitPromotionBonus();
		int iModifierMax = pPromotion->GetNearbyUnitPromotionBonusMax();
		if(iModifierMax <= 0) iModifierMax = 0x7FFFFFFF;
		int iThisModifier = 0;
		// Look around this Unit to see if there's a nearby Unit Promotion that will give us the modifier
		for (int iX = -iUnitPromotionRange; iX <= iUnitPromotionRange; iX++)
		{
			for (int iY = -iUnitPromotionRange; iY <= iUnitPromotionRange; iY++)
			{
				CvPlot* pLoopPlot = plotXYWithRangeCheck(pUnit.getX(), pUnit.getY(), iX, iY, iUnitPromotionRange);
				if (pLoopPlot == nullptr || pLoopPlot->getNumUnits() == 0) continue;
				for (int iK = 0; iK < pLoopPlot->getNumUnits(); iK++)
				{
					CvUnit* pLoopUnit = pLoopPlot->getUnitByIndex(iK);
					if (pLoopUnit == nullptr) continue;
					if (!pLoopUnit->isHasPromotion(eTargetPromotion) || pLoopUnit == &pUnit) continue;
					if (GET_PLAYER(pLoopUnit->getOwner()).getTeam() != GET_PLAYER(pUnit.getOwner()).getTeam()) continue;

					iThisModifier += iUnitPromotionModifier;
					if(iThisModifier >= iModifierMax) goto THIS_LOOP_END;
				}
			}
		}
THIS_LOOP_END:
		iModifier += std::min(iThisModifier, iModifierMax);
	}
    return iModifier;
}