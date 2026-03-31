#include "CvUnitPurchaseCollections.h"
#include "CvGameCoreDLLPCH.h"
#include "CvGameCoreUtils.h"

class CvUnitPurchaseCollectionsPrivate
{
public:
	struct UnitPurchaseCollections
	{
        BuildingTypes eBuilding = NO_BUILDING;
		PolicyTypes ePolicy = NO_POLICY;
		BeliefTypes ePlayerBelief = NO_BELIEF;
		BeliefTypes eCityBelief = NO_BELIEF;
		bool bFoundersHolyCity = false;

        YieldTypes eYield = NO_YIELD;
		// for YIELD_GOLD, cost is a modifier
		// for YIELD_FAITH, cost is a actual value
        int iCost = -1;
	};
	std::multimap<UnitClassTypes, UnitPurchaseCollections> m_mCollectionsMap;
};

CvUnitPurchaseCollections::CvUnitPurchaseCollections()
{
	m_pPrivte = new CvUnitPurchaseCollectionsPrivate;
}

CvUnitPurchaseCollections::~CvUnitPurchaseCollections()
{
	delete m_pPrivte;
}

void CvUnitPurchaseCollections::Init()
{
	m_pPrivte->m_mCollectionsMap.clear();
    CvDatabaseUtility kUtility;
    {
		std::string strKey("Building_EnableUnitPurchase");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, 
				"select UnitClasses.ID as UnitClassID, Buildings.ID as BuildingID, Yields.ID as YieldID, Building_EnableUnitPurchase.CostModifier as CostModifier from Building_EnableUnitPurchase \
				inner join Yields on Yields.Type = YieldType \
				inner join UnitClasses on UnitClasses.Type = UnitClassType \
				inner join Buildings on Buildings.Type = BuildingType;");
		}
		while(pResults->Step())
		{
            UnitClassTypes eUnitClass = (UnitClassTypes)pResults->GetInt(0);
            CvUnitPurchaseCollectionsPrivate::UnitPurchaseCollections sCollections;
            sCollections.eBuilding = (BuildingTypes)pResults->GetInt(1);
            sCollections.eYield = (YieldTypes)pResults->GetInt(2);
            sCollections.iCost = pResults->GetInt(3);
            
			if (sCollections.eYield != YIELD_GOLD && sCollections.eYield != YIELD_FAITH) continue;
			if (sCollections.iCost <= 0) continue;
            m_pPrivte->m_mCollectionsMap.insert(std::make_pair(eUnitClass, sCollections));
		}
    }
	{
		std::string strKey("UnitClass_PurchaseCollections");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if (pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey,
				"select "
				"UnitClasses.ID as UnitClassID, "
				"UnitClass_PurchaseCollections.BuildingType as BuildingType, "
				"UnitClass_PurchaseCollections.PolicyType as PolicyType, "
				"UnitClass_PurchaseCollections.PlayerBeliefType as PlayerBeliefType, "
				"UnitClass_PurchaseCollections.CityBeliefType as CityBeliefType, "
				"UnitClass_PurchaseCollections.FoundersHolyCity as FoundersHolyCity, "
				"Yields.ID as YieldID, "
				"UnitClass_PurchaseCollections.CostModifier as CostModifier "
				"FROM UnitClass_PurchaseCollections "
				"inner join UnitClasses on UnitClasses.Type = UnitClassType "
				"inner join Yields on Yields.Type = YieldType; "
			);
		}

		while (pResults->Step())
		{
			UnitClassTypes eUnitClass = (UnitClassTypes)pResults->GetInt(0);

			CvUnitPurchaseCollectionsPrivate::UnitPurchaseCollections sCollections;

			const char* szBuilding   	= pResults->GetText(1);
			sCollections.eBuilding		= (BuildingTypes)GC.getInfoTypeForString(szBuilding);
			const char* szPolicy     	= pResults->GetText(2);
			sCollections.ePolicy		= (PolicyTypes)GC.getInfoTypeForString(szPolicy);
			const char* szPlayerBelief  = pResults->GetText(3);
			sCollections.ePlayerBelief	= (BeliefTypes)GC.getInfoTypeForString(szPlayerBelief);
			const char* szCityBelief 	= pResults->GetText(4);
			sCollections.eCityBelief	= (BeliefTypes)GC.getInfoTypeForString(szCityBelief);

			sCollections.bFoundersHolyCity = pResults->GetBool(5);
			sCollections.eYield = (YieldTypes)pResults->GetInt(6);
			sCollections.iCost  = pResults->GetInt(7);

			if (sCollections.eYield != YIELD_GOLD && sCollections.eYield != YIELD_FAITH) continue;
			if (sCollections.iCost <= 0) continue;
			m_pPrivte->m_mCollectionsMap.insert(std::make_pair(eUnitClass, sCollections));
		}
	}

}
int CvUnitPurchaseCollections::GetUnitClassPurchaseCost(CvCity *pCity, UnitClassTypes eUnitClass, YieldTypes eYield) const
{
	int iMinModifier = 0x7FFFFFFF;
	CvPlayerAI& pPlayer = GET_PLAYER(pCity->getOwner());

	const auto& collectionsMap = m_pPrivte->m_mCollectionsMap;
	auto range = collectionsMap.equal_range(eUnitClass);
	for (auto it = range.first; it != range.second; ++it)
	{
		const auto& collections = it->second;
		if (collections.eYield != eYield) continue;
		if (collections.eBuilding != NO_BUILDING && !pCity->HasBuilding(collections.eBuilding)) continue;
		if (collections.ePolicy != NO_POLICY && !(pPlayer.HasPolicy(collections.ePolicy) && !pPlayer.GetPlayerPolicies()->IsPolicyBlocked(collections.ePolicy))) continue;
		if (collections.ePlayerBelief != NO_BELIEF && !pPlayer.HasBelief(collections.ePlayerBelief)) continue;
		if (collections.bFoundersHolyCity)
		{
			ReligionTypes eReligion = pPlayer.GetReligions()->GetReligionCreatedByPlayer();
			if (eReligion == NO_RELIGION || !pCity->GetCityReligions()->IsHolyCityForReligion(eReligion)) continue;
		}
		if (collections.eCityBelief != NO_BELIEF && !pCity->HasBelief(collections.eCityBelief)) continue;
		
		iMinModifier = std::min(iMinModifier, collections.iCost);
	}
	return iMinModifier < 0x7FFFFFFF ? iMinModifier : -1;
}
