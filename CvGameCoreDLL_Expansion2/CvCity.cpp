/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */

#include "CustomMods.h"
#include "CvCorruption.h"
#include "CvGameCoreDLLPCH.h"
#include "CvGlobals.h"
#include "CvCity.h"
#include "CvArea.h"
#include "CvMap.h"
#include "CvPlot.h"
#include "CvTeam.h"
#include "CvGameCoreUtils.h"
#include "CvInternalGameCoreUtils.h"
#include "CvPlayerAI.h"
#include "CvUnit.h"
#include "CvInfos.h"
#include "CvRandom.h"
#include "CvImprovementClasses.h"
#include "CvCitySpecializationAI.h"
#include "CvEconomicAI.h"
#include "CvMilitaryAI.h"
#include "CvNotifications.h"
#include "CvUnitCombat.h"
#include "CvTypes.h"

// interfaces used
#include "CvEnumSerialization.h"
#include "CvDiplomacyAI.h"
#include "CvWonderProductionAI.h"

#include "CvDllCity.h"
#include "CvDllCombatInfo.h"
#include "CvDllPlot.h"
#include "CvDllUnit.h"
#include "CvGameQueries.h"

#include "CvInfosSerializationHelper.h"
#include "cvStopWatch.h"
#include "CvCityManager.h"

// include after all other headers
#include "LintFree.h"
#include "FunctionsRef.h"
#include "NetworkMessageUtil.h"

#include "CvLuaCity.h"


OBJECT_VALIDATE_DEFINITION(CvCity)

namespace
{
// debugging
YieldTypes s_lastYieldUsedToUpdateRateFromTerrain;
int        s_changeYieldFromTerreain;
}

//	--------------------------------------------------------------------------------
namespace FSerialization
{
std::set<CvCity*> citiesToCheck;
void SyncCities()
{
	if(GC.getGame().isNetworkMultiPlayer())
	{
		PlayerTypes authoritativePlayer = GC.getGame().getActivePlayer();

		std::set<CvCity*>::const_iterator i;
		for(i = citiesToCheck.begin(); i != citiesToCheck.end(); ++i)
		{
			const CvCity* city = *i;

			if(city)
			{
				const CvPlayer& player = GET_PLAYER(city->getOwner());
				if(city->getOwner() == authoritativePlayer || (gDLL->IsHost() && !player.isHuman() && player.isAlive()))
				{
					const FAutoArchive& archive = city->getSyncArchive();
					if(archive.hasDeltas())
					{
						FMemoryStream memoryStream;
						std::vector<std::pair<std::string, std::string> > callStacks;
						archive.saveDelta(memoryStream, callStacks);
						gDLL->sendCitySyncCheck(city->getOwner(), city->GetID(), memoryStream, callStacks);
					}
				}
			}
		}
	}
}

//	--------------------------------------------------------------------------------
// clears ALL deltas for ALL units
void ClearCityDeltas()
{
	std::set<CvCity*>::iterator i;
	for(i = citiesToCheck.begin(); i != citiesToCheck.end(); ++i)
	{
		CvCity* city = *i;

		if(city)
		{
			FAutoArchive& archive = city->getSyncArchive();
			archive.clearDelta();
		}
	}
}
}

//helper function for managing extra yields
template <typename T>
bool ModifierUpdateInsertRemove(vector<pair<T, int>>& container, T key, int value, bool modifyExisting)
{
	for (typename vector<pair<T, int>>::iterator it = container.begin(); it != container.end(); ++it)
	{
		if (it->first == key)
		{
			if (modifyExisting && value != 0)
			{
				it->second += value;
				if (it->second == 0)
					container.erase(it);
				return true; //update was made
			}

			if (!modifyExisting && value != it->second)
			{
				it->second = value;
				if (it->second == 0)
					container.erase(it);
				return true; //update was made
			}

			return false; //no change
		}
	}

	//if we're here we don't have an entry yet
	if (value != 0)
	{
		container.push_back(make_pair(key, value));
		return true; //update was made
	}

	return false;
}

template <typename T>
int ModifierLookup(const vector<pair<T, int>>& container, T key)
{
	for (typename vector<pair<T, int>>::const_iterator it = container.begin(); it != container.end(); ++it)
		if (it->first == key)
			return it->second;

	return 0;
}

void CvCity::ExtractToArg(BasicArguments* arg) {
	arg->set_argtype("CvCity");
	arg->set_identifier1(getOwner());
	arg->set_identifier2(GetID());
}

void CvCity::PushToLua(lua_State* L, BasicArguments* arg) {
	CvLuaCity::PushLtwt(L, Provide(PlayerTypes(arg->identifier1()), arg->identifier2()));
}

void CvCity::RegistInstanceFunctions() {
	REGIST_INSTANCE_FUNCTION(CvCity::setDamage);
}

void CvCity::RegistStaticFunctions() {
	REGIST_STATIC_FUNCTION(CvCity::Provide);
	REGIST_STATIC_FUNCTION(CvCity::PushToLua);
}

CvCity* CvCity::Provide(PlayerTypes player, int cityID) {
	if (player < 0 || player >= MAX_PLAYERS) throw NetworkMessageNullPointerExceptopn("CvPlayer", player);
	auto rtn = GET_PLAYER(player).getCity(cityID);
	if (!rtn) throw NetworkMessageNullPointerExceptopn("CvCity", player, cityID);
	return rtn;
}



//	--------------------------------------------------------------------------------
// Public Functions...
CvCity::CvCity() :
	m_syncArchive(*this)
	, m_strNameIAmNotSupposedToBeUsedAnyMoreBecauseThisShouldNotBeCheckedAndWeNeedToPreserveSaveGameCompatibility("CvCity::m_strName", m_syncArchive, "")
	, m_eOwner("CvCity::m_eOwner", m_syncArchive, NO_PLAYER)
	, m_iX("CvCity::m_iX", m_syncArchive)
	, m_iY("CvCity::m_iY", m_syncArchive)
	, m_iID("CvCity::m_iID", m_syncArchive)
	, m_iRallyX("CvCity::m_iRallyX", m_syncArchive)
	, m_iRallyY("CvCity::m_iRallyY", m_syncArchive)
	, m_iGameTurnFounded("CvCity::m_iGameTurnFounded", m_syncArchive)
	, m_iGameTurnAcquired("CvCity::m_iGameTurnAcquired", m_syncArchive)
	, m_iGameTurnLastExpanded("CvCity::m_iGameTurnLastExpanded", m_syncArchive)

#if defined(MOD_ROG_CORE)
	, m_iExtraDamageHealPercent(0)
	, m_iExtraDamageHeal("CvCity::m_iExtraDamageHeal", m_syncArchive)
	, m_iBombardRange("CvCity::m_iBombardRange", m_syncArchive)
	, m_iBombardIndirect(0)
	, m_iCityBuildingRangeStrikeModifier("CvCity::m_iCityBuildingRangeStrikeModifier", m_syncArchive)

	, m_iResetDamageValue("CvCity::m_iResetDamageValue", m_syncArchive)
	, m_iReduceDamageValue("CvCity::m_iReduceDamageValue", m_syncArchive)


	, m_iWaterTileDamage("CvCity::m_iWaterTileDamage", m_syncArchive)
	, m_iWaterTileMovementReduce("CvCity::m_iWaterTileMovementReduce", m_syncArchive)
	, m_iWaterTileTurnDamage("CvCity::m_iWaterTileTurnDamage", m_syncArchive)
	, m_iLandTileDamage("CvCity::m_iLandTileDamage", m_syncArchive)
	, m_iLandTileMovementReduce("CvCity::m_iLandTileMovementReduce", m_syncArchive)
	, m_iLandTileTurnDamage("CvCity::m_iLandTileTurnDamage", m_syncArchive)
#endif

	, m_aiNumProjects()
	, m_iNumAttacks("CvCity::m_iNumAttacks", m_syncArchive)
	, m_iAttacksMade("CvCity::m_iAttacksMade", m_syncArchive)
	, m_iAddsFreshWater(0)
	, m_iExtraAttackOnKill("CvCity::m_iExtraAttackOnKill", m_syncArchive)
	, m_iForbiddenForeignSpyCount(0)
	, m_iAllowSpaceshipLaunchCount(0)
#if defined(MOD_ROG_CORE)
	, m_aiNumTimesAttackedThisTurn()
	, m_aiYieldPerAlly()
	, m_aiYieldPerFriend()
	, m_aiBaseYieldRateFromCSAlliance()
	, m_aiBaseYieldRateFromCSFriendship()
	, m_aiYieldPerEspionageSpy()
	, m_aiBaseYieldRateFromEspionageSpy()
	, m_aiYieldFromConstruction()
	, m_aiYieldFromUnitProduction()
	, m_aiYieldFromBirth()
	, m_aiYieldFromBorderGrowth()
	, m_aiYieldFromPillage()
	, m_aiResourceQuantityFromPOP()
#endif

	, m_iPopulation("CvCity::m_iPopulation", m_syncArchive)
#if defined(MOD_GLOBAL_CITY_AUTOMATON_WORKERS)
	, m_iAutomatons(0)
#endif
	, m_iHighestPopulation("CvCity::m_iHighestPopulation", m_syncArchive)
	, m_iExtraHitPoints(0)
	, m_iNumGreatPeople("CvCity::m_iNumGreatPeople", m_syncArchive)
	, m_iBaseGreatPeopleRate("CvCity::m_iBaseGreatPeopleRate", m_syncArchive)
	, m_iGreatPeopleRateModifier("CvCity::m_iGreatPeopleRateModifier", m_syncArchive)
	, m_iJONSCultureStored("CvCity::m_iJONSCultureStored", m_syncArchive, true)
	, m_iJONSCultureLevel("CvCity::m_iJONSCultureLevel", m_syncArchive)
	, m_iJONSCulturePerTurnFromBuildings("CvCity::m_iJONSCulturePerTurnFromBuildings", m_syncArchive)
	, m_iJONSCulturePerTurnFromPolicies("CvCity::m_iJONSCulturePerTurnFromPolicies", m_syncArchive)
	, m_iJONSCulturePerTurnFromSpecialists("CvCity::m_iJONSCulturePerTurnFromSpecialists", m_syncArchive)
	, m_iCultureRateModifier("CvCity::m_iCultureRateModifier", m_syncArchive)
	, m_iNumWorldWonders("CvCity::m_iNumWorldWonders", m_syncArchive)
	, m_iNumTeamWonders("CvCity::m_iNumTeamWonders", m_syncArchive)
	, m_iNumNationalWonders("CvCity::m_iNumNationalWonders", m_syncArchive)
	, m_iWonderProductionModifier("CvCity::m_iWonderProductionModifier", m_syncArchive)
	, m_iCapturePlunderModifier("CvCity::m_iCapturePlunderModifier", m_syncArchive)
	, m_iPlotCultureCostModifier("CvCity::m_iPlotCultureCostModifier", m_syncArchive)
	, m_iPlotBuyCostModifier(0)
	, m_iUnitMaxExperienceLocal(0)
	, m_iTradeRouteRiverBonusModifier(0)
	, m_iSecondCapitalsExtraScore(0)
	, m_iFoodKeptFromPollution(0)
	, m_iNumAllowsFoodTradeRoutes(0)
	, m_iNumAllowsProductionTradeRoutes(0)
#if defined(MOD_BUILDINGS_CITY_WORKING)
	, m_iCityWorkingChange(0)
#endif
#if defined(MOD_BUILDINGS_CITY_AUTOMATON_WORKERS)
	, m_iCityAutomatonWorkersChange(0)
#endif

	, m_iNukeInterceptionChance(0)

	
	, m_iMaintenance("CvCity::m_iMaintenance", m_syncArchive)
	, m_iHealRate("CvCity::m_iHealRate", m_syncArchive)
	, m_iNoOccupiedUnhappinessCount("CvCity::m_iNoOccupiedUnhappinessCount", m_syncArchive)
	, m_iFood("CvCity::m_iFood", m_syncArchive)
	, m_iFoodKept("CvCity::m_iFoodKept", m_syncArchive)
	, m_iMaxFoodKeptPercent("CvCity::m_iMaxFoodKeptPercent", m_syncArchive)
	, m_iOverflowProduction("CvCity::m_iOverflowProduction", m_syncArchive)
	, m_iFeatureProduction("CvCity::m_iFeatureProduction", m_syncArchive)
	, m_iMilitaryProductionModifier("CvCity::m_iMilitaryProductionModifier", m_syncArchive)
	, m_iSpaceProductionModifier("CvCity::m_iSpaceProductionModifier", m_syncArchive)
	, m_iFreeExperience("CvCity::m_iFreeExperience", m_syncArchive)
	, m_iNumCanAirlift(0)
	, m_iCurrAirlift("CvCity::m_iCurrAirlift", m_syncArchive) // unused
	, m_iMaxAirUnits("CvCity::m_iMaxAirUnits", m_syncArchive)
	, m_iAirModifier("CvCity::m_iAirModifier", m_syncArchive) // unused
	, m_iNukeModifier("CvCity::m_iNukeModifier", m_syncArchive)
	, m_iCultureUpdateTimer("CvCity::m_iCultureUpdateTimer", m_syncArchive)	// unused
	, m_iCitySizeBoost("CvCity::m_iCitySizeBoost", m_syncArchive)
	, m_iSpecialistFreeExperience("CvCity::m_iSpecialistFreeExperience", m_syncArchive)
	, m_iStrengthValue("CvCity::m_iStrengthValue", m_syncArchive, true)
	, m_iDamage("CvCity::m_iDamage", m_syncArchive)
	, m_iThreatValue("CvCity::m_iThreatValue", m_syncArchive, true)
	, m_iGarrisonedUnit("CvCity::m_iGarrisonedUnit", m_syncArchive)   // unused
	, m_iResourceDemanded("CvCity::m_iResourceDemanded", m_syncArchive)
	, m_iWeLoveTheKingDayCounter("CvCity::m_iWeLoveTheKingDayCounter", m_syncArchive)
	, m_iLastTurnGarrisonAssigned("CvCity::m_iLastTurnGarrisonAssigned", m_syncArchive)
	, m_iThingsProduced("CvCity::m_iThingsProduced", m_syncArchive)
	, m_iDemandResourceCounter("CvCity::m_iDemandResourceCounter", m_syncArchive, true)
	, m_iResistanceTurns("CvCity::m_iResistanceTurns", m_syncArchive)
	, m_iRazingTurns("CvCity::m_iRazingTurns", m_syncArchive)
	, m_iCountExtraLuxuries("CvCity::m_iCountExtraLuxuries", m_syncArchive)
	, m_iCheapestPlotInfluence("CvCity::m_iCheapestPlotInfluence", m_syncArchive)
	, m_iEspionageModifier(0)
#if defined(MOD_RELIGION_CONVERSION_MODIFIERS)
	, m_iConversionModifier(0)
#endif
	, m_iTradeRouteRecipientBonus(0)
	, m_iTradeRouteTargetBonus(0)
	, m_unitBeingBuiltForOperation()
	, m_bNeverLost("CvCity::m_bNeverLost", m_syncArchive)
	, m_bDrafted("CvCity::m_bDrafted", m_syncArchive)
	, m_bAirliftTargeted("CvCity::m_bAirliftTargeted", m_syncArchive)   // unused
	, m_bProductionAutomated("CvCity::m_bProductionAutomated", m_syncArchive)
	, m_bLayoutDirty("CvCity::m_bLayoutDirty", m_syncArchive)
	, m_bMadeAttack("CvCity::m_bMadeAttack", m_syncArchive)
	, m_bOccupied("CvCity::m_bOccupied", m_syncArchive)
	, m_bPuppet("CvCity::m_bPuppet", m_syncArchive)
	, m_bEverCapital("CvCity::m_bEverCapital", m_syncArchive)
	, m_bIndustrialRouteToCapital("CvCity::m_bIndustrialRouteToCapital", m_syncArchive)
	, m_bFeatureSurrounded("CvCity::m_bFeatureSurrounded", m_syncArchive)
	, m_ePreviousOwner("CvCity::m_ePreviousOwner", m_syncArchive)
	, m_eOriginalOwner("CvCity::m_eOriginalOwner", m_syncArchive)
	, m_ePlayersReligion("CvCity::m_ePlayersReligion", m_syncArchive)
	, m_aiSeaPlotYield("CvCity::m_aiSeaPlotYield", m_syncArchive)
	, m_aiRiverPlotYield("CvCity::m_aiRiverPlotYield", m_syncArchive)
	, m_aiLakePlotYield("CvCity::m_aiLakePlotYield", m_syncArchive)
	, m_aiSeaResourceYield("CvCity::m_aiSeaResourceYield", m_syncArchive)
	, m_aiBaseYieldRateFromTerrain("CvCity::m_aiBaseYieldRateFromTerrain", m_syncArchive, true)
	, m_aiBaseYieldRateFromBuildings("CvCity::m_aiBaseYieldRateFromBuildings", m_syncArchive)
	, m_aiBaseYieldRateFromBuildingsPolicies("CvCity::m_aiBaseYieldRateFromBuildingsPolicies", m_syncArchive)
	, m_aiBaseYieldRateFromSpecialists("CvCity::m_aiBaseYieldRateFromSpecialists", m_syncArchive)
	, m_aiBaseYieldRateFromProjects("CvCity::m_aiBaseYieldRateFromProjects", m_syncArchive)
	, m_aiBaseYieldRateFromMisc("CvCity::m_aiBaseYieldRateFromMisc", m_syncArchive)
	, m_aiYieldRateModifier("CvCity::m_aiYieldRateModifier", m_syncArchive)
	, m_aiYieldRateMultiplier("CvCity::m_aiYieldRateMultiplier", m_syncArchive)
	, m_aiYieldPerPop("CvCity::m_aiYieldPerPop", m_syncArchive)
	, m_aiYieldPerPopInEmpire("CvCity::m_aiYieldPerPopInEmpire", m_syncArchive)
	, m_aiPowerYieldRateModifier("CvCity::m_aiPowerYieldRateModifier", m_syncArchive)
	, m_aiFeatureYieldRateModifier("CvCity::m_aiFeatureYieldRateModifier", m_syncArchive)
	, m_aiTerrainYieldRateModifier("CvCity::m_aiTerrainYieldRateModifier", m_syncArchive)
	, m_aiImprovementYieldRateModifier("CvCity::m_aiImprovementYieldRateModifier", m_syncArchive)
	, m_aiSpecialistYieldRateModifier("CvCity::m_aiSpecialistYieldRateModifier", m_syncArchive)
	, m_aiResourceYieldRateModifier("CvCity::m_aiResourceYieldRateModifier", m_syncArchive)
	, m_aiExtraSpecialistYield("CvCity::m_aiExtraSpecialistYield", m_syncArchive)
	, m_aiProductionToYieldModifier("CvCity::m_aiProductionToYieldModifier", m_syncArchive)
	, m_aiDomainFreeExperience("CvCity::m_aiDomainFreeExperience", m_syncArchive)
	, m_aiDomainFreeExperiencesPerPop("CvCity::m_aiDomainFreeExperiencesPerPop", m_syncArchive)
	, m_aiDomainFreeExperiencesPerPopGlobal("CvCity::m_aiDomainFreeExperiencesPerPopGlobal", m_syncArchive)
	, m_aiDomainFreeExperiencesPerTurn("CvCity::m_aiDomainFreeExperiencesPerTurn", m_syncArchive)
	, m_aiDomainEnemyCombatModifier("CvCity::m_aiDomainEnemyCombatModifier", m_syncArchive)
	, m_aiDomainFriendsCombatModifierLocal("CvCity::m_aiDomainFriendsCombatModifierLocal", m_syncArchive)
	, m_aiDomainProductionModifier("CvCity::m_aiDomainProductionModifier", m_syncArchive)
	, m_abEverOwned("CvCity::m_abEverOwned", m_syncArchive)
	, m_abRevealed("CvCity::m_abRevealed", m_syncArchive, true)
	, m_strScriptData("CvCity::m_strScriptData", m_syncArchive)
	, m_paiNoResource("CvCity::m_paiNoResource", m_syncArchive)
	, m_paiFreeResource("CvCity::m_paiFreeResource", m_syncArchive)
	, m_paiNumResourcesLocal("CvCity::m_paiNumResourcesLocal", m_syncArchive)
	, m_paiProjectProduction("CvCity::m_paiProjectProduction", m_syncArchive)
	, m_paiSpecialistProduction("CvCity::m_paiSpecialistProduction", m_syncArchive)
	, m_paiUnitProduction("CvCity::m_paiUnitProduction", m_syncArchive)
	, m_paiUnitProductionTime("CvCity::m_paiUnitProductionTime", m_syncArchive)
	, m_paiSpecialistCount("CvCity::m_paiSpecialistCount", m_syncArchive)
	, m_paiMaxSpecialistCount("CvCity::m_paiMaxSpecialistCount", m_syncArchive)
	, m_paiForceSpecialistCount("CvCity::m_paiForceSpecialistCount", m_syncArchive)
	, m_paiFreeSpecialistCount("CvCity::m_paiFreeSpecialistCount", m_syncArchive)
	, m_paiImprovementFreeSpecialists("CvCity::m_paiImprovementFreeSpecialists", m_syncArchive)
	, m_paiUnitCombatFreeExperience("CvCity::m_paiUnitCombatFreeExperience", m_syncArchive)
	, m_paiUnitCombatProductionModifier("CvCity::m_paiUnitCombatProductionModifier", m_syncArchive)
	, m_paiFreePromotionCount("CvCity::m_paiFreePromotionCount", m_syncArchive)
	, m_viTradeRouteDomainRangeModifier("CvCity::m_viTradeRouteDomainRangeModifier", m_syncArchive)
	, m_viTradeRouteDomainGoldBonus("CvCity::m_viTradeRouteDomainGoldBonus", m_syncArchive)

	, m_iBaseHappinessFromBuildings(0)
	, m_iUnmoddedHappinessFromBuildings(0)
	, m_bRouteToCapitalConnectedLastTurn(false)
	, m_bRouteToCapitalConnectedThisTurn(false)
	, m_strName("")
	, m_orderQueue()
	, m_aaiBuildingSpecialistUpgradeProgresses(0)
	, m_aiYieldFromInternalTR()
	, m_aiYieldFromProcessModifier("CvCity::m_aiYieldFromProcessModifier", m_syncArchive)

	, m_pCityBuildings(FNEW(CvCityBuildings, c_eCiv5GameplayDLL, 0))
	, m_pCityStrategyAI(FNEW(CvCityStrategyAI, c_eCiv5GameplayDLL, 0))
	, m_pCityCitizens(FNEW(CvCityCitizens, c_eCiv5GameplayDLL, 0))
	, m_pCityReligions(FNEW(CvCityReligions, c_eCiv5GameplayDLL, 0))
	, m_pEmphases(FNEW(CvCityEmphases, c_eCiv5GameplayDLL, 0))
	, m_pCityEspionage(FNEW(CvCityEspionage, c_eCiv5GameplayDLL, 0))
	, m_pCityCulture(FNEW(CvCityCulture, c_eCiv5GameplayDLL, 0))
	, m_bombardCheckTurn(0)
	, m_iPopulationRank(0)
	, m_bPopulationRankValid(false)
	, m_aiBaseYieldRank("CvCity::m_aiBaseYieldRank", m_syncArchive)
	, m_abBaseYieldRankValid("CvCity::m_abBaseYieldRankValid", m_syncArchive)
	, m_aiYieldRank("CvCity::m_aiYieldRank", m_syncArchive)
	, m_abYieldRankValid("CvCity::m_abYieldRankValid", m_syncArchive)
	, m_paiHurryModifier("CvCity::m_paiHurryModifier", m_syncArchive)
	, m_bOwedCultureBuilding(false)

	, m_yieldChanges(NUM_YIELD_TYPES)
#if defined(MOD_BUILDING_IMPROVEMENT_RESOURCES)
	   , m_ppiResourceFromImprovement()
#endif
#if defined(MOD_BUGFIX_FREE_FOOD_BUILDING)
	    , m_bOwedFoodBuilding(false)
#endif
		, m_paiNumTerrainWorked()
		, m_paiNumFeaturelessTerrainWorked()
		, m_paiNumImprovementWorked()
		, m_paiNumFeatureWorked()
		, m_iNumNearbyMountains()
		, m_iAdditionalFood()
		, m_iBaseTourism()
		, m_iBaseTourismBeforeModifiers()

#if defined(MOD_API_UNIFIED_YIELDS_MORE)
		, m_bIsColony()
		, m_iOrganizedCrime()
		, m_iResistanceCounter()
		, m_iPlagueCounter()
		, m_iPlagueTurns()
		, m_iPlagueType()
		, m_iLoyaltyCounter()
		, m_iDisloyaltyCounter()
		, m_iLoyaltyStateType()
		, m_aiYieldModifierFromHealth()
		, m_aiYieldModifierFromCrime()
		, m_aiYieldFromHappiness()
		, m_aiYieldFromHealth()
		, m_aiYieldFromCrime()
		, m_aiSpecialistRateModifier()
		, m_aiStaticCityYield()
#endif
	, m_iLastTurnWorkerDisbanded(0)
	, m_iDefendedAgainstSpreadUntilTurn(0)
	, m_paiNumBuildingClasses()
{
	OBJECT_ALLOCATED
	FSerialization::citiesToCheck.insert(this);

	reset(0, NO_PLAYER, 0, 0, true);
}

//	--------------------------------------------------------------------------------
CvCity::~CvCity()
{
	CvCityManager::OnCityDestroyed(this);
	FSerialization::citiesToCheck.erase(this);

	uninit();

	delete m_pCityBuildings;
	delete m_pCityStrategyAI;
	delete m_pCityCitizens;
	delete m_pCityReligions;
	delete m_pEmphases;
	delete m_pCityEspionage;
	delete m_pCityCulture;

	OBJECT_DESTROYED
}
//	--------------------------------------------------------------------------------
#if defined(MOD_API_EXTENSIONS)
void CvCity::init(int iID, PlayerTypes eOwner, int iX, int iY, bool bBumpUnits, bool bInitialFounding, ReligionTypes eInitialReligion, const char* szName)
#else
void CvCity::init(int iID, PlayerTypes eOwner, int iX, int iY, bool bBumpUnits, bool bInitialFounding)
#endif
{
	VALIDATE_OBJECT
	//CvPlot* pAdjacentPlot;
	CvPlot* pPlot;
	BuildingTypes eLoopBuilding;
	int iI;

	pPlot = GC.getMap().plot(iX, iY);

	//--------------------------------
	// Init saved data
	reset(iID, eOwner, pPlot->getX(), pPlot->getY());

	CvPlayerAI& owningPlayer = GET_PLAYER(getOwner());

	//--------------------------------
	// Init non-saved data

	//--------------------------------
	// Init other game data
	CvString strNewCityName = owningPlayer.getNewCityName();
	setName(strNewCityName.c_str());

#if !defined(NO_ACHIEVEMENTS)
	if(strcmp(strNewCityName.c_str(), "TXT_KEY_CITY_NAME_LLANFAIRPWLLGWYNGYLL") == 0)
	{
		gDLL->UnlockAchievement(ACHIEVEMENT_XP1_34);
	}
#endif

	// Plot Ownership
	setEverOwned(getOwner(), true);

	pPlot->setOwner(getOwner(), m_iID, bBumpUnits);
	// Clear the improvement before the city attaches itself to the plot, else the improvement does not
	// remove the resource allocation from the current owner.  This would result in double resource points because
	// the plot has already had setOwner called on it (above), giving the player the resource points.
	pPlot->setImprovementType(NO_IMPROVEMENT);
#if defined(MOD_BUGFIX_MINOR)
#if defined(MOD_EVENTS_TILE_IMPROVEMENTS)
	pPlot->SetImprovementPillaged(false, false);
	pPlot->SetRoutePillaged(false, false);
#else
	pPlot->SetImprovementPillaged(false);
	pPlot->SetRoutePillaged(false);
#endif
#endif
	pPlot->setPlotCity(this);
	pPlot->SetCityPurchaseID(m_iID);

	int iRange = 1;
#if defined(MOD_CONFIG_GAME_IN_XML)
	iRange = GD_INT_GET(CITY_STARTING_RINGS);
#endif
	for(int iDX = -iRange; iDX <= iRange; iDX++)
	{
		for(int iDY = -iRange; iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXYWithRangeCheck(getX(), getY(), iDX, iDY, iRange);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->getOwner() == NO_PLAYER)
				{
					pLoopPlot->setOwner(getOwner(), m_iID, bBumpUnits);
				}
				if (pLoopPlot->getOwner() == getOwner())
				{
					pLoopPlot->SetCityPurchaseID(m_iID);
				}
			}
		}
	}

	// this is a list of plot that are owned by the player
	owningPlayer.UpdatePlots();

#if defined(MOD_GLOBAL_CITY_FOREST_BONUS)
	static BuildTypes eBuildRemoveForest = (BuildTypes)GC.getInfoTypeForString("BUILD_REMOVE_FOREST");
	bool bClearedForest = false;
	FeatureTypes eFeature = pPlot->getFeatureType();
#endif

#if defined(MOD_GLOBAL_CITY_JUNGLE_BONUS)
	static BuildTypes eBuildRemoveJungle = (BuildTypes)GC.getInfoTypeForString("BUILD_REMOVE_JUNGLE");
	bool bClearedJungle = false;
#if !defined(MOD_GLOBAL_CITY_FOREST_BONUS)
	FeatureTypes eFeature = pPlot->getFeatureType();
#endif
#endif

	//SCRIPT call ' bool citiesDestroyFeatures(iX, iY);'
	if(pPlot->getFeatureType() != NO_FEATURE)
	{
#if defined(MOD_GLOBAL_CITY_FOREST_BONUS)
		// Only for major civs building on a forest
		if(MOD_GLOBAL_CITY_FOREST_BONUS && eBuildRemoveForest != -1 && !owningPlayer.isMinorCiv() && eFeature == FEATURE_FOREST)
		{
			// Don't do this for the AI capitals - it's just too much of an initial boost!
			if (owningPlayer.isHuman() || owningPlayer.getCapitalCity() != NULL) {
				TechTypes iRequiredTech = (TechTypes) gCustomMods.getOption("GLOBAL_CITY_FOREST_BONUS_TECH", -1);
				bClearedForest = (iRequiredTech == -1 || GET_TEAM(owningPlayer.getTeam()).GetTeamTechs()->HasTech(iRequiredTech));
			}
		}
#endif

#if defined(MOD_GLOBAL_CITY_JUNGLE_BONUS)
		// Only for major civs building on a jungle
		if(MOD_GLOBAL_CITY_JUNGLE_BONUS && eBuildRemoveJungle != -1 && !owningPlayer.isMinorCiv() && eFeature == FEATURE_JUNGLE)
		{
			// Don't do this for the AI capitals - it's just too much of an initial boost!
			if (owningPlayer.isHuman() || owningPlayer.getCapitalCity() != NULL) {
				TechTypes iRequiredTech = (TechTypes) gCustomMods.getOption("GLOBAL_CITY_JUNGLE_BONUS_TECH", -1);
				bClearedJungle = (iRequiredTech == -1 || GET_TEAM(owningPlayer.getTeam()).GetTeamTechs()->HasTech(iRequiredTech));
			}
		}
#endif
					
		pPlot->setFeatureType(NO_FEATURE);
	}

	// wipe out dig sites
	ResourceTypes eArtifactResourceType = static_cast<ResourceTypes>(GC.getARTIFACT_RESOURCE());
	ResourceTypes eHiddenArtifactResourceType = static_cast<ResourceTypes>(GC.getHIDDEN_ARTIFACT_RESOURCE());
	if (pPlot->getResourceType() == eArtifactResourceType || pPlot->getResourceType() == eHiddenArtifactResourceType)
	{
		pPlot->setResourceType(NO_RESOURCE, 0);
		pPlot->ClearArchaeologicalRecord();
	}

	setupGraphical();

	pPlot->updateCityRoute();

	for(iI = 0; iI < MAX_TEAMS; iI++)
	{
		if(GET_TEAM((TeamTypes)iI).isAlive())
		{
			if(pPlot->isVisible(((TeamTypes)iI)))
			{
				setRevealed(((TeamTypes)iI), true);
			}
		}
	}

	int iNumBuildingInfos = GC.getNumBuildingInfos();
	for(iI = 0; iI < iNumBuildingInfos; iI++)
	{
		if(owningPlayer.isBuildingFree((BuildingTypes)iI))
		{
			if(isValidBuildingLocation((BuildingTypes)iI))
			{
				m_pCityBuildings->SetNumFreeBuilding(((BuildingTypes)iI), 1);
			}
		}
	}

	area()->changeCitiesPerPlayer(getOwner(), 1);

	GET_TEAM(getTeam()).changeNumCities(1);

	GC.getGame().changeNumCities(1);
	// Tell the city manager now as well.
	CvCityManager::OnCityCreated(this);

	int iGameTurn = GC.getGame().getGameTurn();
	setGameTurnFounded(iGameTurn);
	setGameTurnAcquired(iGameTurn);
	setGameTurnLastExpanded(iGameTurn);

#if defined(MOD_GLOBAL_CITY_WORKING)
	GC.getMap().updateWorkingCity(pPlot,getWorkPlotDistance()*2);
#else	
	GC.getMap().updateWorkingCity(pPlot,NUM_CITY_RINGS*2);
#endif
	GetCityCitizens()->DoFoundCity();

	// Default starting population
	changePopulation(GC.getINITIAL_CITY_POPULATION() + GC.getGame().getStartEraInfo().getFreePopulation());
	// Free population from things (e.g. Policies)
	changePopulation(GET_PLAYER(getOwner()).GetNewCityExtraPopulation());

#if defined(MOD_API_EXTENSIONS)
	// We do this here as changePopulation() sends a game event we may have caught to do funky renaming things
	if (szName) {
		setName(szName);
	}
#endif




	// Free food from things (e.g. Policies)
	int iFreeFood = growthThreshold() * GET_PLAYER(getOwner()).GetFreeFoodBox();
	changeFoodTimes100(iFreeFood);
	
	if (bInitialFounding)
	{
		owningPlayer.setFoundedFirstCity(true);
		owningPlayer.ChangeNumCitiesFounded(1);

		// Free resources under city?
		for(int i = 0; i < GC.getNumResourceInfos(); i++)
		{
			ResourceTypes eResource = (ResourceTypes)i;
			FreeResourceXCities freeResource = owningPlayer.GetPlayerTraits()->GetFreeResourceXCities(eResource);

			if(freeResource.m_iResourceQuantity > 0)
			{
				if(owningPlayer.GetNumCitiesFounded() <= freeResource.m_iNumCities)
				{
					plot()->setResourceType(NO_RESOURCE, 0);
					plot()->setResourceType(eResource, freeResource.m_iResourceQuantity);
				}
			}
		}

		owningPlayer.GetPlayerTraits()->AddUniqueLuxuries(this);
		if(owningPlayer.isMinorCiv())
		{
			owningPlayer.GetMinorCivAI()->DoAddStartingResources(plot());
		}
	}

	// make sure that all the team members get the city connection update
	for(int i = 0; i < MAX_PLAYERS; i++)
	{
		PlayerTypes ePlayer = (PlayerTypes)i;
		if(GET_PLAYER(ePlayer).getTeam() == owningPlayer.getTeam())
		{
			GET_PLAYER(ePlayer).GetCityConnections()->Update();
		}
	}
	owningPlayer.DoUpdateHappiness();

	// Policy changes
	PolicyTypes ePolicy;
	for(int iPoliciesLoop = 0; iPoliciesLoop < GC.getNumPolicyInfos(); iPoliciesLoop++)
	{
		ePolicy = (PolicyTypes) iPoliciesLoop;

		if(owningPlayer.GetPlayerPolicies()->HasPolicy(ePolicy) && !owningPlayer.GetPlayerPolicies()->IsPolicyBlocked(ePolicy))
		{
			// Free Culture-per-turn in every City from Policies
			ChangeJONSCulturePerTurnFromPolicies(GC.getPolicyInfo(ePolicy)->GetCulturePerCity());
		}
	}

	// Add Resource Quantity to total
	if(plot()->getResourceType() != NO_RESOURCE)
	{
		if(GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes) GC.getResourceInfo(plot()->getResourceType())->getTechCityTrade()))
		{
			owningPlayer.changeNumResourceTotal(plot()->getResourceType(), plot()->getNumResourceForPlayer(getOwner()));
		}
	}

	CvPlot* pLoopPlot;

	// We may need to link Resources to this City if it's constructed within previous borders and the Resources were too far away for another City to link to
#if defined(MOD_GLOBAL_CITY_WORKING)
	for(int iJ = 0; iJ < GetNumWorkablePlots(); iJ++)
#else
	for(int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
#endif
	{
		pLoopPlot = plotCity(getX(), getY(), iJ);

		if(pLoopPlot != NULL)
		{
			if(pLoopPlot->getOwner() == getOwner())
			{
				if(pLoopPlot->getResourceType() != NO_RESOURCE)
				{
					// Is this Resource as of yet unlinked?
					if(pLoopPlot->GetResourceLinkedCity() == NULL)
					{
						pLoopPlot->DoFindCityToLinkResourceTo();
					}
				}
			}
		}
	}

	PlayerTypes ePlayer;

	// Update Proximity between this Player and all others
	for(int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
	{
		ePlayer = (PlayerTypes) iPlayerLoop;

		if(ePlayer != getOwner())
		{
			if(GET_PLAYER(ePlayer).isAlive())
			{
				// Players do NOT have to know one another in order to calculate proximity.  Having this info available (even whey they haven't met) can be useful
				owningPlayer.DoUpdateProximityToPlayer(ePlayer);
				GET_PLAYER(ePlayer).DoUpdateProximityToPlayer(getOwner());
			}
		}
	}

	// Free Buildings in the first City
	CvCivilizationInfo& thisCiv = getCivilizationInfo();
	if(GC.getGame().isFinalInitialized())
	{
		if(owningPlayer.getNumCities() == 1)
		{
#if defined(MOD_EVENTS_CITY_CAPITAL)
			int eCapitalBuilding = owningPlayer.GetCivBuilding((BuildingClassTypes)GC.getCAPITAL_BUILDINGCLASS());
#endif
			
			for(iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
			{
				CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo((BuildingClassTypes)iI);
				if(!pkBuildingClassInfo)
				{
					continue;
				}

				if(thisCiv.isCivilizationFreeBuildingClass(iI))
				{
					eLoopBuilding = owningPlayer.GetCivBuilding((BuildingClassTypes)iI);

					if(eLoopBuilding != NO_BUILDING)
					{
						m_pCityBuildings->SetNumRealBuilding(eLoopBuilding, true);

#if defined(MOD_EVENTS_CITY_CAPITAL)
						if (iI == eCapitalBuilding && MOD_EVENTS_CITY_CAPITAL) {
							GAMEEVENTINVOKE_HOOK(GAMEEVENT_CapitalChanged, getOwner(), GetID(), -1);
						}
#endif
					}
				}
			}

			if(!isHuman())
			{
				changeOverflowProduction(GC.getINITIAL_AI_CITY_PRODUCTION());
#if defined(ACHIEVEMENT_HACKS)
			} else {
				CvAchievementUnlocker::UnlockFromDatabase();
#endif
			}
		}
	}

	// How long before this City picks a Resource to demand?
	DoSeedResourceDemandedCountdown();

	// Update City Strength
	updateStrengthValue();

	// Update Unit Maintenance for the player
	CvPlayer& kPlayer = GET_PLAYER(getOwner());
	kPlayer.UpdateUnitProductionMaintenanceMod();

	// Spread a pantheon here if one is active
	CvPlayerReligions* pReligions = kPlayer.GetReligions();
	if(pReligions->HasCreatedPantheon() && !pReligions->HasCreatedReligion())
	{
		GetCityReligions()->AddReligiousPressure(FOLLOWER_CHANGE_PANTHEON_FOUNDED, RELIGION_PANTHEON, GC.getRELIGION_ATHEISM_PRESSURE_PER_POP() * getPopulation() * 2);
	}
	
#if defined(MOD_API_EXTENSIONS)
	if (bInitialFounding) {
		if (eInitialReligion != NO_RELIGION) {
			// Spread an initial religion here if one was given
			GetCityReligions()->AdoptReligionFully(eInitialReligion);
		}

#if defined(MOD_RELIGION_LOCAL_RELIGIONS)
		else if (MOD_RELIGION_LOCAL_RELIGIONS) {
			// Spread a local religion here if one is active
			if(pReligions->HasCreatedReligion() && GC.getReligionInfo(pReligions->GetReligionCreatedByPlayer())->IsLocalReligion()) {
				GetCityReligions()->AdoptReligionFully(pReligions->GetReligionCreatedByPlayer());
			}
		}
#endif
	}
#endif

	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		YieldTypes eYield = (YieldTypes)iI;
		if (eYield == NO_YIELD)
			continue;

		UpdateCityYields(eYield);

		for (int iK = 0; iK < NUM_YIELD_TYPES; iK++)
		{
			YieldTypes eYield2 = (YieldTypes)iK;

			if (eYield == eYield2)
				continue;

			int iGlobalConversionYield = owningPlayer.getYieldFromYieldGlobal(eYield, eYield2);
			if (iGlobalConversionYield > 0)
			{
				ChangeBuildingYieldFromYield(eYield, eYield2, iGlobalConversionYield);
			}
		}

		for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
		{
			SpecialistTypes eSpecialist = (SpecialistTypes)iI;
			CvSpecialistInfo* pSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
			if (pSpecialistInfo)
			{
				int iGlobalConversionYield = owningPlayer.GetYieldModifierFromSpecialistGlobal(eSpecialist, eYield);
				if (iGlobalConversionYield > 0)
				{
					changeYieldModifierFromSpecialist(eSpecialist, eYield, iGlobalConversionYield);
				}
			}
		}
	}

	// A new City might change our victory progress
	GET_TEAM(getTeam()).DoTestSmallAwards();

	DLLUI->setDirty(NationalBorders_DIRTY_BIT, true);

	// Garrisoned?
	if (plot()->getNumUnits() > 0)
	{
		bool bGarrisonFreeMaintenance = kPlayer.IsGarrisonFreeMaintenance();
		for(int iUnitLoop = 0; iUnitLoop < plot()->getNumUnits(); iUnitLoop++)
		{
			CvUnit* iUnit = pPlot->getUnitByIndex(iUnitLoop);
			if(iUnit->GetBaseCombatStrength(true/*bIgnoreEmbarked*/) > 0 && iUnit->getDomainType() == DOMAIN_LAND)
			{
				if(bGarrisonFreeMaintenance)
				{
					kPlayer.changeExtraUnitCost(iUnit->getUnitInfo().GetExtraMaintenanceCost());
				}

				if (getOwner() == iUnit->getOwner())
				{
					int iCulture = kPlayer.GetPlayerPolicies()->GetNumericModifier(POLICYMOD_CULTURE_FROM_GARRISON);
					if (iCulture != 0) ChangeJONSCulturePerTurnFromPolicies(iCulture);

					int iNoOccupiedUnhappiness = kPlayer.getPolicyModifiers(POLICYMOD_NO_OCCUPIED_UNHAPPINESS_GARRISONED_CITY);
					if (iNoOccupiedUnhappiness > 0) ChangeNoOccupiedUnhappinessCount(iNoOccupiedUnhappiness);
				}
			}
		}
	}

#if defined(MOD_GLOBAL_CITY_FOREST_BONUS)
	if (bClearedForest) {
		int iProduction;

		// Base value
		if (GET_PLAYER(getOwner()).GetAllFeatureProduction() > 0) {
			iProduction = GET_PLAYER(getOwner()).GetAllFeatureProduction();
		} else {
			iProduction = GC.getBuildInfo(eBuildRemoveForest)->getFeatureProduction(FEATURE_FOREST);
		}

		iProduction *= std::max(0, (GET_PLAYER(getOwner()).getFeatureProductionModifier() + 100));
		iProduction /= 100;

		iProduction *= GC.getGame().getGameSpeedInfo().getFeatureProductionPercent();
		iProduction /= 100;
		iProduction *= (100+GetCuttingBonusModifier());
		iProduction /= 100;

		if (iProduction > 0) {
			// Make the production higher than a "ring-1 chop"
			iProduction *= gCustomMods.getOption("GLOBAL_CITY_FOREST_BONUS_PERCENT", 125);
			iProduction /= 100;

			changeFeatureProduction(iProduction);
			CUSTOMLOG("Founding of %s on a forest created %d initial production", getName().GetCString(), iProduction);

			if (getOwner() == GC.getGame().getActivePlayer()) {
				CvString strBuffer = GetLocalizedText("TXT_KEY_MISC_CLEARING_FEATURE_RESOURCE", GC.getFeatureInfo(eFeature)->GetTextKey(), iProduction, getNameKey());
				GC.GetEngineUserInterface()->AddCityMessage(0, GetIDInfo(), getOwner(), false, GC.getEVENT_MESSAGE_TIME(), strBuffer);
			}
			DoCuttingExtraInstantYield(iProduction);
		}
	}
#endif

#if defined(MOD_GLOBAL_CITY_JUNGLE_BONUS)
	if (bClearedJungle) {
		int iProduction;

		// Base value
		if (GET_PLAYER(getOwner()).GetAllFeatureProduction() > 0) {
			iProduction = GET_PLAYER(getOwner()).GetAllFeatureProduction();
		} else {
			iProduction = GC.getBuildInfo(eBuildRemoveJungle)->getFeatureProduction(FEATURE_JUNGLE);
		}

		iProduction *= std::max(0, (GET_PLAYER(getOwner()).getFeatureProductionModifier() + 100));
		iProduction /= 100;

		iProduction *= GC.getGame().getGameSpeedInfo().getFeatureProductionPercent();
		iProduction /= 100;
		iProduction *= (100+GetCuttingBonusModifier());
		iProduction /= 100;

		if (iProduction > 0) {
			// Make the production higher than a "ring-1 chop"
			iProduction *= gCustomMods.getOption("GLOBAL_CITY_JUNGLE_BONUS_PERCENT", 125);
			iProduction /= 100;

			changeFeatureProduction(iProduction);
			CUSTOMLOG("Founding of %s on a jungle created %d initial production", getName().GetCString(), iProduction);

			if (getOwner() == GC.getGame().getActivePlayer()) {
				CvString strBuffer = GetLocalizedText("TXT_KEY_MISC_CLEARING_FEATURE_RESOURCE", GC.getFeatureInfo(eFeature)->GetTextKey(), iProduction, getNameKey());
				GC.GetEngineUserInterface()->AddCityMessage(0, GetIDInfo(), getOwner(), false, GC.getEVENT_MESSAGE_TIME(), strBuffer);
			}
			DoCuttingExtraInstantYield(iProduction);
		}
	}
#endif

#ifdef MOD_GLOBAL_CITY_SCALES
	if (MOD_GLOBAL_CITY_SCALES)
		UpdateScaleBuildings();
#endif

#ifdef MOD_GLOBAL_CORRUPTION
	if (MOD_GLOBAL_CORRUPTION)
		UpdateCorruption();
#endif

	AI_init();

	if (GC.getGame().getGameTurn() == 0)
	{
		chooseProduction();
	}
}

//	--------------------------------------------------------------------------------
void CvCity::uninit()
{
	VALIDATE_OBJECT

	if(m_aaiBuildingSpecialistUpgradeProgresses)
	{
		for(int i=0; i < GC.getNumBuildingInfos(); i++)
		{
			SAFE_DELETE_ARRAY(m_aaiBuildingSpecialistUpgradeProgresses[i]);
		}
	}
	SAFE_DELETE_ARRAY(m_aaiBuildingSpecialistUpgradeProgresses);


	m_pCityBuildings->Uninit();
	m_pCityStrategyAI->Uninit();
	m_pCityCitizens->Uninit();
	m_pCityReligions->Uninit();
	m_pEmphases->Uninit();
	m_pCityEspionage->Uninit();

	m_orderQueue.clear();

	m_yieldChanges.clear();
#if defined(MOD_BUILDING_IMPROVEMENT_RESOURCES) 
	m_ppiResourceFromImprovement.clear();
#endif
	m_ppiYieldModifierFromFeature.clear();
	m_ppiYieldModifierFromTerrain.clear();
	m_ppiYieldModifierFromImprovement.clear();
	m_ppiYieldModifierFromSpecialist.clear();
	m_ppiYieldModifierFromResource.clear();
	m_paiHurryModifier.clear();
}

//	--------------------------------------------------------------------------------
// FUNCTION: reset()
// Initializes data members that are serialized.
void CvCity::reset(int iID, PlayerTypes eOwner, int iX, int iY, bool bConstructorCall)
{
	VALIDATE_OBJECT
	m_syncArchive.reset();

	int iI;

	//--------------------------------
	// Uninit class
	uninit();

	m_iID = iID;
	m_iX = iX;
	m_iY = iY;
	m_iRallyX = INVALID_PLOT_COORD;
	m_iRallyY = INVALID_PLOT_COORD;
	m_iGameTurnFounded = 0;
	m_iGameTurnAcquired = 0;
	m_iPopulation = 0;
#if defined(MOD_GLOBAL_CITY_AUTOMATON_WORKERS)
	m_iAutomatons = 0;
#endif

	m_iNumNearbyMountains = 0;
	m_iAdditionalFood = 0;
	m_iBaseTourism = 0;
	m_iBaseTourismBeforeModifiers = 0;
	m_aiNumProjects.resize(GC.getNumProjectInfos());
#if defined(MOD_VOLCANO_BREAK)
	m_iImmueVolcanoDamage = 0;
#endif
	m_iAddsFreshWater = 0;
	m_iExtraAttackOnKill = 0;
	m_iForbiddenForeignSpyCount = 0;
	m_iAllowSpaceshipLaunchCount = 0;
#if defined(MOD_ROG_CORE)
	m_aiNumTimesAttackedThisTurn.resize(REALLY_MAX_PLAYERS);
	m_aiSpecialistRateModifier.resize(GC.getNumSpecialistInfos());
	m_iExtraDamageHealPercent = 0;
	m_iExtraDamageHeal = 0;
	m_iBombardRange = 0;
	m_iBombardIndirect = 0;
	m_iCityBuildingRangeStrikeModifier = 0;

	m_iResetDamageValue = 0;
	m_iReduceDamageValue = 0;


	m_iWaterTileDamage = 0;
	m_iWaterTileMovementReduce = 0;
	m_iWaterTileTurnDamage = 0;
	m_iLandTileDamage = 0;
	m_iLandTileMovementReduce = 0;
	m_iLandTileTurnDamage = 0;
#endif

	m_iNumAttacks = 1;
	m_iAttacksMade = 0;


	m_iHighestPopulation = 0;
	m_iExtraHitPoints = 0;
	m_iNumGreatPeople = 0;
	m_iBaseGreatPeopleRate = 0;
	m_iGreatPeopleRateModifier = 0;
	m_iJONSCultureStored = 0;
	m_iJONSCultureLevel = 0;
	m_iJONSCulturePerTurnFromBuildings = 0;
	m_iJONSCulturePerTurnFromPolicies = 0;
	m_iJONSCulturePerTurnFromSpecialists = 0;
	m_iCultureRateModifier = 0;
	m_iNumWorldWonders = 0;
	m_iNumTeamWonders = 0;
	m_iNumNationalWonders = 0;
	m_iWonderProductionModifier = 0;
	m_iCapturePlunderModifier = 0;
	m_iPlotCultureCostModifier = 0;
	m_iPlotBuyCostModifier = 0;
	m_iUnitMaxExperienceLocal = 0;
	m_iTradeRouteRiverBonusModifier = 0;
	m_iSecondCapitalsExtraScore = 0;
	m_iFoodKeptFromPollution = 0;
	m_iNumAllowsFoodTradeRoutes = 0;
	m_iNumAllowsProductionTradeRoutes = 0;
#if defined(MOD_BUILDINGS_CITY_WORKING)
	m_iCityWorkingChange = 0;
#endif
#if defined(MOD_BUILDINGS_CITY_AUTOMATON_WORKERS)
	m_iCityAutomatonWorkersChange = 0;
#endif

	m_iNukeInterceptionChance = 0;

	m_iMaintenance = 0;
	m_iHealRate = 0;
	m_iEspionageModifier = 0;
#if defined(MOD_RELIGION_CONVERSION_MODIFIERS)
	m_iConversionModifier = 0;
#endif
	m_iNoOccupiedUnhappinessCount = 0;
	m_iFood = 0;
	m_iFoodKept = 0;
	m_iMaxFoodKeptPercent = 0;
	m_iOverflowProduction = 0;
	m_iFeatureProduction = 0;
	m_iMilitaryProductionModifier = 0;
	m_iSpaceProductionModifier = 0;
	m_iFreeExperience = 0;
	m_iNumCanAirlift = 0;
	m_iCurrAirlift = 0; // unused
	m_iMaxAirUnits = GC.getBASE_CITY_AIR_STACKING();
	m_iAirModifier = 0; // unused
	m_iNukeModifier = 0;
	m_iTradeRouteRecipientBonus = 0;
	m_iTradeRouteTargetBonus = 0;
	m_iCultureUpdateTimer = 0;
	m_iCitySizeBoost = 0;
	m_iSpecialistFreeExperience = 0;
	m_iStrengthValue = 0;
	m_iDamage = 0;
	m_iThreatValue = 0;
	m_iGarrisonedUnit = -1;    // unused
	m_iResourceDemanded = -1;
	m_iWeLoveTheKingDayCounter = 0;
	m_iLastTurnGarrisonAssigned = -1;
	m_iThingsProduced = 0;
	m_iDemandResourceCounter = 0;
	m_iResistanceTurns = 0;
	m_iRazingTurns = 0;
	m_iCountExtraLuxuries = 0;
	m_iCheapestPlotInfluence = 0;
	m_unitBeingBuiltForOperation.Invalidate();

	m_bNeverLost = true;
	m_bDrafted = false;
	m_bAirliftTargeted = false;   // unused
	m_bProductionAutomated = false;
	m_bLayoutDirty = false;
	m_bMadeAttack = false;
	m_bOccupied = false;
	m_bPuppet = false;
	m_bIgnoreCityForHappiness = false;
	m_bEverCapital = false;
	m_bIndustrialRouteToCapital = false;
	m_bFeatureSurrounded = false;
	m_bOwedCultureBuilding = false;
#if defined(MOD_BUGFIX_FREE_FOOD_BUILDING)
	m_bOwedFoodBuilding = false;
#endif

	m_eOwner = eOwner;
	m_ePreviousOwner = NO_PLAYER;
	m_eOriginalOwner = eOwner;
	m_ePlayersReligion = NO_PLAYER;


	m_aiSeaPlotYield.resize(NUM_YIELD_TYPES);
	m_aiRiverPlotYield.resize(NUM_YIELD_TYPES);
	m_aiSeaResourceYield.resize(NUM_YIELD_TYPES);
	m_aiLakePlotYield.resize(NUM_YIELD_TYPES);
	m_aiBaseYieldRateFromTerrain.resize(NUM_YIELD_TYPES);
	m_aiBaseYieldRateFromBuildings.resize(NUM_YIELD_TYPES);
	m_aiBaseYieldRateFromBuildingsPolicies.resize(NUM_YIELD_TYPES);
	m_aiBaseYieldRateFromSpecialists.resize(NUM_YIELD_TYPES);
	m_aiBaseYieldRateFromProjects.resize(NUM_YIELD_TYPES);
	m_aiBaseYieldRateFromMisc.resize(NUM_YIELD_TYPES);
	m_aiBaseYieldRateFromReligion.resize(NUM_YIELD_TYPES);
	m_aiYieldPerPop.resize(NUM_YIELD_TYPES);
	m_aiYieldPerPopInEmpire.resize(NUM_YIELD_TYPES);

	m_aiYieldFromInternalTR.resize(NUM_YIELD_TYPES);
	m_aiYieldFromProcessModifier.resize(NUM_YIELD_TYPES);


#if defined(MOD_ROG_CORE)

	for (iI = 0; iI < REALLY_MAX_PLAYERS; iI++)
	{
		m_aiNumTimesAttackedThisTurn[iI] = 0;
	}

	m_aiYieldPerAlly.resize(NUM_YIELD_TYPES);
	m_aiYieldPerFriend.resize(NUM_YIELD_TYPES);
	m_aiBaseYieldRateFromCSFriendship.resize(NUM_YIELD_TYPES);
	m_aiBaseYieldRateFromCSAlliance.resize(NUM_YIELD_TYPES);
	m_aiYieldPerEspionageSpy.resize(NUM_YIELD_TYPES);
	m_aiBaseYieldRateFromEspionageSpy.resize(NUM_YIELD_TYPES);
	m_aiYieldFromConstruction.resize(NUM_YIELD_TYPES);
	m_aiYieldFromUnitProduction.resize(NUM_YIELD_TYPES);
	m_aiYieldFromBirth.resize(NUM_YIELD_TYPES);
	m_aiYieldFromBorderGrowth.resize(NUM_YIELD_TYPES);
	m_aiYieldFromPillage.resize(NUM_YIELD_TYPES);
#endif

	m_aiYieldPerReligion.resize(NUM_YIELD_TYPES);
	m_aiYieldPerEra.resize(NUM_YIELD_TYPES);
	m_aiYieldModifierPerEra.resize(NUM_YIELD_TYPES);
	m_aiCityStateTradeRouteYieldModifier.resize(NUM_YIELD_TYPES);
	m_aiYieldRateModifier.resize(NUM_YIELD_TYPES);
	m_aiYieldRateMultiplier.resize(NUM_YIELD_TYPES);
	m_aiPowerYieldRateModifier.resize(NUM_YIELD_TYPES);
	m_aiFeatureYieldRateModifier.resize(NUM_YIELD_TYPES);
	m_aiTerrainYieldRateModifier.resize(NUM_YIELD_TYPES);
	m_aiImprovementYieldRateModifier.resize(NUM_YIELD_TYPES);
	m_aiSpecialistYieldRateModifier.resize(NUM_YIELD_TYPES);
	m_aiResourceYieldRateModifier.resize(NUM_YIELD_TYPES);
	m_aiExtraSpecialistYield.resize(NUM_YIELD_TYPES);
	m_aiProductionToYieldModifier.resize(NUM_YIELD_TYPES);
	for(iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		m_aiSeaPlotYield.setAt(iI, 0);
		m_aiRiverPlotYield.setAt(iI, 0);
		m_aiLakePlotYield.setAt(iI, 0);
		m_aiSeaResourceYield.setAt(iI, 0);
		m_aiBaseYieldRateFromTerrain.setAt(iI, 0);
		m_aiBaseYieldRateFromBuildings.setAt(iI, 0);
		m_aiBaseYieldRateFromBuildingsPolicies.setAt(iI, 0);
		m_aiBaseYieldRateFromSpecialists.setAt(iI, 0);
		m_aiBaseYieldRateFromProjects.setAt(iI, 0);
		m_aiBaseYieldRateFromMisc.setAt(iI, 0);
		m_aiBaseYieldRateFromReligion[iI] = 0;
		m_aiYieldPerPop.setAt(iI, 0);
		m_aiYieldPerPopInEmpire.setAt(iI, 0);
		m_aiYieldPerReligion[iI] = 0;
		m_aiYieldPerEra[iI] = 0;
		m_aiYieldModifierPerEra[iI] = 0;
		m_aiCityStateTradeRouteYieldModifier[iI] = 0;
		m_aiYieldRateModifier.setAt(iI, 0);
		m_aiYieldRateMultiplier.setAt(iI, 0);
		m_aiPowerYieldRateModifier.setAt(iI, 0);
		m_aiFeatureYieldRateModifier.setAt(iI, 0);
		m_aiTerrainYieldRateModifier.setAt(iI, 0);
		m_aiImprovementYieldRateModifier.setAt(iI, 0);
		m_aiSpecialistYieldRateModifier.setAt(iI, 0);
		m_aiResourceYieldRateModifier.setAt(iI, 0);
		m_aiExtraSpecialistYield.setAt(iI, 0);
		m_aiProductionToYieldModifier.setAt(iI, 0);
#if defined(MOD_BALANCE_CORE)
		m_aiYieldPerAlly[iI] = 0;
		m_aiYieldPerFriend[iI] = 0;
		m_aiBaseYieldRateFromCSFriendship[iI] = 0;
		m_aiBaseYieldRateFromCSAlliance[iI] = 0;
		m_aiYieldPerEspionageSpy[iI] = 0;
		m_aiBaseYieldRateFromEspionageSpy[iI] = 0;
		m_aiYieldFromConstruction[iI] = 0;	
		m_aiYieldFromUnitProduction[iI] = 0;	
		m_aiYieldFromBirth[iI] = 0;
		m_aiYieldFromBorderGrowth[iI] = 0;
		m_aiYieldFromPillage[iI] = 0;
#endif
	}

	m_aiDomainFreeExperience.resize(NUM_DOMAIN_TYPES);
	m_aiDomainProductionModifier.resize(NUM_DOMAIN_TYPES);
	m_aiDomainFreeExperiencesPerPop.resize(NUM_DOMAIN_TYPES);
	m_aiDomainFreeExperiencesPerPopGlobal.resize(NUM_DOMAIN_TYPES);
	m_aiDomainFreeExperiencesPerTurn.resize(NUM_DOMAIN_TYPES);
	m_aiDomainEnemyCombatModifier.resize(NUM_DOMAIN_TYPES);
	m_aiDomainFriendsCombatModifierLocal.resize(NUM_DOMAIN_TYPES);
	for(iI = 0; iI < NUM_DOMAIN_TYPES; iI++)
	{
		m_aiDomainFreeExperience.setAt(iI, 0);
		m_aiDomainProductionModifier.setAt(iI, 0);
		m_aiDomainFreeExperiencesPerPop.setAt(iI, 0);
		m_aiDomainFreeExperiencesPerPopGlobal.setAt(iI, 0);
		m_aiDomainFreeExperiencesPerTurn.setAt(iI, 0);
		m_aiDomainEnemyCombatModifier.setAt(iI, 0);
		m_aiDomainFriendsCombatModifierLocal.setAt(iI, 0);
	}


	for (iI = 0; iI < GC.getNumProjectInfos(); iI++)
	{
		m_aiNumProjects[iI] = 0;
	}


	m_abEverOwned.resize(REALLY_MAX_PLAYERS);
	for(iI = 0; iI < REALLY_MAX_PLAYERS; iI++)
	{
		m_abEverOwned.setAt(iI, false);
	}

	m_abRevealed.resize(REALLY_MAX_TEAMS);
	for(iI = 0; iI < REALLY_MAX_TEAMS; iI++)
	{
		m_abRevealed.setAt(iI, false);
	}

	m_strName = "";
	m_strNameIAmNotSupposedToBeUsedAnyMoreBecauseThisShouldNotBeCheckedAndWeNeedToPreserveSaveGameCompatibility = "";
	m_strScriptData = "";

	m_bPopulationRankValid = false;
	m_iPopulationRank = -1;

	m_abBaseYieldRankValid.resize(NUM_YIELD_TYPES);
	m_abYieldRankValid.resize(NUM_YIELD_TYPES);
	m_aiBaseYieldRank.resize(NUM_YIELD_TYPES);
	m_aiYieldRank.resize(NUM_YIELD_TYPES);
#if defined(MOD_API_UNIFIED_YIELDS_MORE)
	m_aiYieldModifierFromHealth.resize(NUM_YIELD_TYPES);
	m_aiYieldModifierFromCrime.resize(NUM_YIELD_TYPES);
	m_aiYieldFromHappiness.resize(NUM_YIELD_TYPES);
	m_aiYieldFromHealth.resize(NUM_YIELD_TYPES);
	m_aiYieldFromCrime.resize(NUM_YIELD_TYPES);
	m_aiStaticCityYield.resize(NUM_YIELD_TYPES);
#endif
	for(iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		m_abBaseYieldRankValid.setAt(iI, false);
		m_abYieldRankValid.setAt(iI, false);
		m_aiBaseYieldRank.setAt(iI, -1);
		m_aiYieldRank.setAt(iI, -1);
#if defined(MOD_API_UNIFIED_YIELDS_MORE)
		m_aiYieldModifierFromHealth[iI] = 0;
		m_aiYieldModifierFromCrime[iI] = 0;
		m_aiYieldFromHappiness[iI] = 0;
		m_aiYieldFromHealth[iI] = 0;
		m_aiYieldFromCrime[iI] = 0;
		m_aiStaticCityYield[iI] = 0;
#endif
	}

#if defined(MOD_API_UNIFIED_YIELDS_MORE)
	m_bIsColony = false;
	m_iOrganizedCrime = -1;
	m_iResistanceCounter = 0;
	m_iPlagueCounter = 0;
	m_iPlagueTurns = -1;
	m_iPlagueType = -1;
	m_iLoyaltyCounter = 0;
	m_iDisloyaltyCounter = 0;
	m_iLoyaltyStateType = 0;
#endif

	if(!bConstructorCall)
	{
		int iNumResources = GC.getNumResourceInfos();
		CvAssertMsg((0 < iNumResources),  "GC.getNumResourceInfos() is not greater than zero but an array is being allocated in CvCity::reset");
		m_paiNoResource.clear();
		m_paiNoResource.resize(iNumResources);
		m_paiFreeResource.clear();
		m_paiFreeResource.resize(iNumResources);
		m_paiNumResourcesLocal.clear();
		m_paiNumResourcesLocal.resize(iNumResources);

#if defined(MOD_ROG_CORE)
		m_aiResourceQuantityFromPOP.clear();
		m_aiResourceQuantityFromPOP.resize(iNumResources);
#endif
		m_aiYieldFromInternalTR[iI] = 0;
		m_aiYieldFromProcessModifier.setAt(iI, 0);


		for(iI = 0; iI < iNumResources; iI++)
		{
			m_paiNoResource.setAt(iI, 0);
			m_paiFreeResource.setAt(iI, 0);
			m_paiNumResourcesLocal.setAt(iI, 0);
#if defined(MOD_ROG_CORE)
			m_aiResourceQuantityFromPOP[iI] = 0;
#endif
		}


		int iNumTerrainInfos = GC.getNumTerrainInfos();
		int iNumImprovementInfos = GC.getNumImprovementInfos();
		int iNumFeatureInfos = GC.getNumFeatureInfos();
		int iNumResourceInfos = GC.getNumResourceInfos();

		m_paiNumTerrainWorked.clear();
		m_paiNumFeaturelessTerrainWorked.clear();
		m_paiNumFeaturelessTerrainWorked.resize(iNumTerrainInfos);
		m_paiNumTerrainWorked.resize(iNumTerrainInfos);
		for (iI = 0; iI < iNumTerrainInfos; iI++)
		{
			m_paiNumTerrainWorked[iI] = 0;
			m_paiNumFeaturelessTerrainWorked[iI] = 0;
		}

		m_paiNumImprovementWorked.clear();
		m_paiNumImprovementWorked.resize(iNumImprovementInfos);
		for (iI = 0; iI < iNumImprovementInfos; iI++)
		{
			m_paiNumImprovementWorked[iI] = 0;
		}

		m_paiNumFeatureWorked.clear();
		m_paiNumFeatureWorked.resize(iNumFeatureInfos);
		for (iI = 0; iI < iNumFeatureInfos; iI++)
		{
			m_paiNumFeatureWorked[iI] = 0;
		}


		int iNumProjectInfos = GC.getNumProjectInfos();
		m_paiProjectProduction.clear();
		m_paiProjectProduction.resize(iNumProjectInfos);
		for(iI = 0; iI < iNumProjectInfos; iI++)
		{
			m_paiProjectProduction.setAt(iI, 0);
		}

		int iNumSpecialistInfos = GC.getNumSpecialistInfos();
		m_paiSpecialistProduction.clear();
		m_paiSpecialistProduction.resize(iNumSpecialistInfos);
		for(iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
		{
			m_paiSpecialistProduction.setAt(iI, 0);
#if defined(MOD_ROG_CORE)
			m_aiSpecialistRateModifier[iI] = 0;
#endif
		}

		m_pCityBuildings->Init(GC.GetGameBuildings(), this);

		int iNumUnitInfos = GC.getNumUnitInfos();
		CvAssertMsg((0 < iNumUnitInfos),  "GC.getNumUnitInfos() is not greater than zero but an array is being allocated in CvCity::reset");
		m_paiUnitProduction.clear();
		m_paiUnitProduction.resize(iNumUnitInfos);
		m_paiUnitProductionTime.clear();
		m_paiUnitProductionTime.resize(iNumUnitInfos);
		for(iI = 0; iI < iNumUnitInfos; iI++)
		{
			m_paiUnitProduction.setAt(iI, 0);
			m_paiUnitProductionTime.setAt(iI, 0);
		}

		CvAssertMsg((0 < iNumSpecialistInfos),  "GC.getNumSpecialistInfos() is not greater than zero but an array is being allocated in CvCity::reset");
		m_paiSpecialistCount.clear();
		m_paiSpecialistCount.resize(iNumSpecialistInfos);
		m_paiMaxSpecialistCount.clear();
		m_paiMaxSpecialistCount.resize(iNumSpecialistInfos);
		m_paiForceSpecialistCount.clear();
		m_paiForceSpecialistCount.resize(iNumSpecialistInfos);
		m_paiFreeSpecialistCount.clear();
		m_paiFreeSpecialistCount.resize(iNumSpecialistInfos);

		for(iI = 0; iI < iNumSpecialistInfos; iI++)
		{
			m_paiSpecialistCount.setAt(iI, 0);
			m_paiMaxSpecialistCount.setAt(iI, 0);
			m_paiForceSpecialistCount.setAt(iI, 0);
			m_paiFreeSpecialistCount.setAt(iI, 0);
		}

		CvAssertMsg((0 < iNumImprovementInfos),  "GC.getNumImprovementInfos() is not greater than zero but an array is being allocated in CvCity::reset");
		m_paiImprovementFreeSpecialists.clear();
		m_paiImprovementFreeSpecialists.resize(iNumImprovementInfos);
		for(iI = 0; iI < iNumImprovementInfos; iI++)
		{
			m_paiImprovementFreeSpecialists.setAt(iI, 0);
		}

		int iNumUnitCombatClassInfos = GC.getNumUnitCombatClassInfos();
		CvAssertMsg((0 < iNumUnitCombatClassInfos),  "GC.getNumUnitCombatClassInfos() is not greater than zero but an array is being allocated in CvCity::reset");
		m_paiUnitCombatFreeExperience.clear();
		m_paiUnitCombatFreeExperience.resize(iNumUnitCombatClassInfos);
		m_paiUnitCombatProductionModifier.clear();
		m_paiUnitCombatProductionModifier.resize(iNumUnitCombatClassInfos);
		for(iI = 0; iI < iNumUnitCombatClassInfos; iI++)
		{
			m_paiUnitCombatFreeExperience.setAt(iI, 0);
			m_paiUnitCombatProductionModifier.setAt(iI, 0);
		}

		int iNumPromotionInfos = GC.getNumPromotionInfos();
		CvAssertMsg((0 < iNumPromotionInfos),  "GC.getNumPromotionInfos() is not greater than zero but an array is being allocated in CvCity::reset");
		m_paiFreePromotionCount.clear();
		m_paiFreePromotionCount.resize(iNumPromotionInfos);

		m_viTradeRouteDomainRangeModifier.clear();
		m_viTradeRouteDomainRangeModifier.resize(NUM_DOMAIN_TYPES, 0);
		m_viTradeRouteDomainGoldBonus.clear();
		m_viTradeRouteDomainGoldBonus.resize(NUM_DOMAIN_TYPES, 0);

		for(iI = 0; iI < iNumPromotionInfos; iI++)
		{
			m_paiFreePromotionCount.setAt(iI, 0);
		}

		int iJ;

		int iNumBuildingInfos = GC.getNumBuildingInfos();
		int iMAX_SPECIALISTS_FROM_BUILDING = GC.getMAX_SPECIALISTS_FROM_BUILDING();
		CvAssertMsg(m_aaiBuildingSpecialistUpgradeProgresses==NULL, "about to leak memory, CvCity::m_aaiBuildingSpecialistUpgradeProgresses");
		m_aaiBuildingSpecialistUpgradeProgresses = FNEW(int*[iNumBuildingInfos], c_eCiv5GameplayDLL, 0);
		for(iI = 0; iI < iNumBuildingInfos; iI++)
		{
			m_aaiBuildingSpecialistUpgradeProgresses[iI] = FNEW(int[iMAX_SPECIALISTS_FROM_BUILDING], c_eCiv5GameplayDLL, 0);
			for(iJ = 0; iJ < iMAX_SPECIALISTS_FROM_BUILDING; iJ++)
			{
				m_aaiBuildingSpecialistUpgradeProgresses[iI][iJ] = -1;
			}
		}

	}

#if defined(MOD_BUILDING_IMPROVEMENT_RESOURCES)
	m_ppiResourceFromImprovement.clear();
#endif

	m_yieldChanges = vector<SCityExtraYields>(NUM_YIELD_TYPES);

	if(!bConstructorCall)
	{
		// Set up AI and hook it up to the flavor manager
		m_pCityStrategyAI->Init(GC.GetGameAICityStrategies(), this, true);
		if(m_eOwner != NO_PLAYER)
		{
			GET_PLAYER(getOwner()).GetFlavorManager()->AddFlavorRecipient(m_pCityStrategyAI);
			m_pCityStrategyAI->SetDefaultSpecialization(GET_PLAYER(getOwner()).GetCitySpecializationAI()->GetNextSpecializationDesired());
		}

		m_pCityCitizens->Init(this);
		m_pCityReligions->Init(this);
		m_pEmphases->Init(GC.GetGameEmphases(), this);
		m_pCityEspionage->Init(this);
		m_pCityCulture->Init(this);

		AI_reset();

#if defined(MOD_GLOBAL_CITY_AUTOMATON_WORKERS)
		if (m_eOwner != NO_PLAYER) {
			setAutomatons(GET_TEAM(GET_PLAYER(getOwner()).getTeam()).GetCityAutomatonWorkersChange());
			setAutomatons(GET_PLAYER(getOwner()).GetCityAutomatonWorkersChange());
		}
#endif
	}

#ifdef MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD
	if (MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD)
	{
		//m_bHasYieldFromOtherYield = false;
		//for (size_t i = 0; i < NUM_YIELD_TYPES; i++)
		//{
		//	for (size_t j = 0; j < NUM_YIELD_TYPES; j++)
		//	{
		//		m_ppiYieldFromOtherYield[i][j][YieldFromYield::IN_VALUE] = 0;
		//		m_ppiYieldFromOtherYield[i][j][YieldFromYield::OUT_VALUE] = 0;
		//	}
		//}
	}
#endif

#if defined(MOD_NUCLEAR_WINTER_FOR_SP)
	m_iNumNoNuclearWinterLocal = 0;
#endif
#if defined(MOD_TROOPS_AND_CROPS_FOR_SP)
	m_iCityEnableCrops = 0;
	m_iCityEnableArmee = 0;
#endif
#if defined(MOD_INTERNATIONAL_IMMIGRATION_FOR_SP)
	m_bCanDoImmigration = true;
	m_iNumAllScaleImmigrantIn = 0;
#endif
#ifdef MOD_GLOBAL_CITY_SCALES
	m_eCityScale = NO_CITY_SCALE;
#endif
	Firaxis::Array< int, NUM_YIELD_TYPES > yield;
	for(unsigned int j = 0; j < NUM_YIELD_TYPES; ++j)
	{
		yield[j] = 0;
	}

	m_ppiYieldModifierFromFeature.clear();
	m_ppiYieldModifierFromFeature.resize(GC.getNumFeatureInfos());
	for(unsigned int i = 0; i < m_ppiYieldModifierFromFeature.size(); ++i)
	{
		m_ppiYieldModifierFromFeature[i] = yield;
	}

	m_ppiYieldModifierFromTerrain.clear();
	m_ppiYieldModifierFromTerrain.resize(GC.getNumTerrainInfos());
	for (unsigned int i = 0; i < m_ppiYieldModifierFromTerrain.size(); ++i)
	{
		m_ppiYieldModifierFromTerrain[i] = yield;
	}

	m_ppiYieldModifierFromImprovement.clear();
	m_ppiYieldModifierFromImprovement.resize(GC.getNumImprovementInfos());
	for(unsigned int i = 0; i < m_ppiYieldModifierFromImprovement.size(); ++i)
	{
		m_ppiYieldModifierFromImprovement[i] = yield;
	}

	m_ppiYieldModifierFromSpecialist.clear();
	m_ppiYieldModifierFromSpecialist.resize(GC.getNumSpecialistInfos());
	for (unsigned int i = 0; i < m_ppiYieldModifierFromSpecialist.size(); ++i)
	{
		m_ppiYieldModifierFromSpecialist[i] = yield;
	}

	m_ppiYieldModifierFromResource.clear();
	m_ppiYieldModifierFromResource.resize(GC.getNumResourceInfos());
	for(unsigned int i = 0; i < m_ppiYieldModifierFromResource.size(); ++i)
	{
		m_ppiYieldModifierFromResource[i] = yield;
	}
	m_paiHurryModifier.clear();
	m_paiHurryModifier.resize(GC.getNumHurryInfos(), 0);

	for (int i = 0; i < NUM_YIELD_TYPES; ++i)
	{
		m_aTradeRouteFromTheCityYields[i] = 0;
	}
	for (int i = 0; i < NUM_YIELD_TYPES; ++i)
	{
		m_aTradeRouteFromTheCityYieldsPerEra[i] = 0;
	}
	m_iLastTurnWorkerDisbanded = 0;
	m_iDefendedAgainstSpreadUntilTurn = 0;
	m_paiNumBuildingClasses.clear();
	m_paiNumBuildingClasses.resize(GC.getNumBuildingClassInfos());
	for (int i = 0; i < m_paiNumBuildingClasses.size(); ++i)
	{
		m_paiNumBuildingClasses[i] = 0;
	}
}


//////////////////////////////////////
// graphical only setup
//////////////////////////////////////
void CvCity::setupGraphical()
{
	VALIDATE_OBJECT
	if(!GC.IsGraphicsInitialized())
	{
		return;
	}

	CvPlayer& player = GET_PLAYER(getOwner());
	EraTypes eCurrentEra =(EraTypes) player.GetCurrentEra();

	auto_ptr<ICvCity1> pkDllCity(new CvDllCity(this));
	gDLL->GameplayCityCreated(pkDllCity.get(), eCurrentEra);
	gDLL->GameplayCitySetDamage(pkDllCity.get(), getDamage(), 0);

	// setup the wonders
	setupWonderGraphics();

	// setup any special buildings
	setupBuildingGraphics();

	// setup the spaceship
	setupSpaceshipGraphics();

	setLayoutDirty(true);
}

//	--------------------------------------------------------------------------------
void CvCity::setupWonderGraphics()
{
	VALIDATE_OBJECT
	PlayerTypes ePlayerID = getOwner();
	for(int eBuildingType = 0; eBuildingType < GC.getNumBuildingInfos(); eBuildingType++)
	{
		const BuildingTypes eBuilding = static_cast<BuildingTypes>(eBuildingType);
		CvBuildingEntry* buildingInfo = GC.getBuildingInfo(eBuilding);

		//Only work with valid buildings.
		if(buildingInfo == NULL)
			continue;

		// if this building exists
		int iExists = m_pCityBuildings->GetNumRealBuilding(eBuilding);
		int iPreferredPosition = buildingInfo->GetPreferredDisplayPosition();
		if(iPreferredPosition > 0)
		{
			if(iExists > 0)
			{
				// display the wonder
				auto_ptr<ICvPlot1> pDllPlot(new CvDllPlot(plot()));
				gDLL->GameplayWonderCreated(ePlayerID, pDllPlot.get(), eBuilding, 1);
			}
			else
			{
				if (isWorldWonderClass(buildingInfo->GetBuildingClassInfo()))
				{
					bool bShowHalfBuilt = false;
					// Are we are constructing it?
					if (eBuilding == getProductionBuilding())
					{
						bShowHalfBuilt = true;
					}
					else
					{
						// Is it part of an international project?
						LeagueProjectTypes eThisBuildingProject = NO_LEAGUE_PROJECT;
						for (int i = 0; i < GC.getNumLeagueProjectInfos(); i++)
						{
							LeagueProjectTypes eProject = (LeagueProjectTypes)i;
							CvLeagueProjectEntry* pProjectInfo = GC.getLeagueProjectInfo(eProject);
							if (pProjectInfo != NULL && pProjectInfo->GetRewardTier3() != NO_LEAGUE_PROJECT_REWARD) // Only check top reward tier
							{
								CvLeagueProjectRewardEntry* pRewardInfo = GC.getLeagueProjectRewardInfo(pProjectInfo->GetRewardTier3());
								if (pRewardInfo != NULL && pRewardInfo->GetBuilding() == eBuilding)
								{
									eThisBuildingProject = eProject;
									break;
								}
							}
						}
						if (eThisBuildingProject != NO_LEAGUE_PROJECT)
						{
							// Have we contributed anything to it?
							if (GC.getGame().GetGameLeagues()->GetNumActiveLeagues() > 0)
							{
								CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
								if (pLeague != NULL)
								{
									if (pLeague->IsProjectActive(eThisBuildingProject) && pLeague->GetMemberContribution(ePlayerID, eThisBuildingProject) > 0)
									{
										// Only show the graphic in the capital, since that is where the wonder would go
										if (isCapital())
										{
											bShowHalfBuilt = true;
										}
									}
								}
							}
						}

						
					}

					if (bShowHalfBuilt)
					{
						auto_ptr<ICvPlot1> pDllPlot(new CvDllPlot(plot()));
						gDLL->GameplayWonderCreated(ePlayerID, pDllPlot.get(), eBuilding, 0);
					}
				}
			}
		}
	}
}

//	--------------------------------------------------------------------------------
void CvCity::setupBuildingGraphics()
{
	VALIDATE_OBJECT
	for(int eBuildingType = 0; eBuildingType < GC.getNumBuildingInfos(); eBuildingType++)
	{
		const BuildingTypes eBuilding = static_cast<BuildingTypes>(eBuildingType);
		CvBuildingEntry* buildingInfo = GC.getBuildingInfo(eBuilding);

		if(buildingInfo)
		{
			int iExists = m_pCityBuildings->GetNumBuilding(eBuilding);
			if(iExists > 0 && buildingInfo->IsCityWall())
			{
				auto_ptr<ICvPlot1> pDllPlot(new CvDllPlot(plot()));
				gDLL->GameplayWallCreated(pDllPlot.get());
			}
		}
	}
}

//	--------------------------------------------------------------------------------
void CvCity::setupSpaceshipGraphics()
{
	VALIDATE_OBJECT

	CvTeam& thisTeam = GET_TEAM(getTeam());

	ProjectTypes ApolloProgram = (ProjectTypes) GC.getSPACE_RACE_TRIGGER_PROJECT();

	int spaceshipState = 0;

	if(isCapital() && thisTeam.getProjectCount((ProjectTypes)ApolloProgram) == 1)
	{
		ProjectTypes capsuleID = (ProjectTypes) GC.getSPACESHIP_CAPSULE();
		ProjectTypes boosterID = (ProjectTypes) GC.getSPACESHIP_BOOSTER();
		ProjectTypes stasisID = (ProjectTypes) GC.getSPACESHIP_STASIS();
		ProjectTypes engineID = (ProjectTypes) GC.getSPACESHIP_ENGINE();

		enum eSpaceshipState
		{
		    eUnderConstruction	= 0x0000,
		    eFrame				= 0x0001,
		    eCapsule			= 0x0002,
		    eStasis_Chamber		= 0x0004,
		    eEngine				= 0x0008,
		    eBooster1			= 0x0010,
		    eBooster2			= 0x0020,
		    eBooster3			= 0x0040,
		    eConstructed		= 0x0080,
		};

		auto_ptr<ICvPlot1> pDllPlot(new CvDllPlot(plot()));
		gDLL->GameplaySpaceshipRemoved(pDllPlot.get());
		gDLL->GameplaySpaceshipCreated(pDllPlot.get(), eUnderConstruction + eFrame);

		spaceshipState = eFrame;

		if((thisTeam.getProjectCount((ProjectTypes)capsuleID)) == 1)
		{
			spaceshipState += eCapsule;
		}

		if((thisTeam.getProjectCount((ProjectTypes)stasisID)) == 1)
		{
			spaceshipState += eStasis_Chamber;
		}

		if((thisTeam.getProjectCount((ProjectTypes)engineID)) == 1)
		{
			spaceshipState += eEngine;
		}

		if((thisTeam.getProjectCount((ProjectTypes)boosterID)) >= 1)
		{
			spaceshipState += eBooster1;
		}

		if((thisTeam.getProjectCount((ProjectTypes)boosterID)) >= 2)
		{
			spaceshipState += eBooster2;
		}

		if((thisTeam.getProjectCount((ProjectTypes)boosterID)) == 3)
		{
			spaceshipState += eBooster3;
		}

		gDLL->GameplaySpaceshipEdited(pDllPlot.get(), spaceshipState);
	}
}

//	--------------------------------------------------------------------------------
#if defined(MOD_GLOBAL_VENICE_KEEPS_RESOURCES) || defined(MOD_GLOBAL_CS_MARRIAGE_KEEPS_RESOURCES)
void CvCity::PreKill(bool bKeepResources)
#else
void CvCity::PreKill()
#endif
{
	VALIDATE_OBJECT

	PlayerTypes eOwner;
	if(isCitySelected())
	{
		DLLUI->clearSelectedCities();
	}

#if defined(MOD_GLOBAL_CITY_AUTOMATON_WORKERS)
	setAutomatons(0);
#endif
	setPopulation(0);

	CvPlot* pPlot = plot();

#if defined(MOD_BUGFIX_MINOR)
	GC.getGame().GetGameTrade()->ClearAllCityTradeRoutes(pPlot, true);
#else
	GC.getGame().GetGameTrade()->ClearAllCityTradeRoutes(pPlot);
#endif

	// Update resources linked to this city
#if defined(MOD_GLOBAL_CITY_WORKING)
	for(int iI = 0; iI < GetNumWorkablePlots(); iI++)
#else
	for(int iI = 0; iI < NUM_CITY_PLOTS; iI++)
#endif
	{
		CvPlot* pLoopPlot;
		pLoopPlot = GetCityCitizens()->GetCityPlotFromIndex(iI);

		if(pLoopPlot != NULL)
		{
			if(pLoopPlot->getWorkingCityOverride() == this)
			{
				pLoopPlot->setWorkingCityOverride(NULL);
			}

			// Unlink Resources from this City
			if(pLoopPlot->getOwner() == getOwner())
			{
				if(pLoopPlot->getResourceType() != NO_RESOURCE)
				{
					if(pLoopPlot->GetResourceLinkedCity() == this)
					{
						pLoopPlot->SetResourceLinkedCity(NULL);
						pLoopPlot->DoFindCityToLinkResourceTo(this);
					}
				}
			}
		}
	}

	// If this city was built on a Resource, remove its Quantity from total
	if(pPlot->getResourceType() != NO_RESOURCE)
	{
		if(GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes) GC.getResourceInfo(pPlot->getResourceType())->getTechCityTrade()))
		{
			GET_PLAYER(getOwner()).changeNumResourceTotal(pPlot->getResourceType(), -pPlot->getNumResourceForPlayer(getOwner()));
		}
	}

#if defined(MOD_GLOBAL_VENICE_KEEPS_RESOURCES) || defined(MOD_GLOBAL_CS_MARRIAGE_KEEPS_RESOURCES)
	plot()->removeMinorResources(bKeepResources);
#else
	if(GET_PLAYER(getOwner()).isMinorCiv())
	{
		GET_PLAYER(getOwner()).GetMinorCivAI()->DoRemoveStartingResources(plot());
	}
#endif

	for(int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		CvBuildingEntry* pkBuilding = GC.getBuildingInfo((BuildingTypes)iI);
		if(pkBuilding)
		{
			m_pCityBuildings->SetNumRealBuilding(((BuildingTypes)iI), 0);
			m_pCityBuildings->SetNumFreeBuilding(((BuildingTypes)iI), 0);
		}
	}

	clearOrderQueue();

	// Killing a city while in combat is not something we really expect to happen.
	// It is *mostly* safe for it to happen, but combat systems must be able to gracefully handle the disapperance of a city.
	CvAssertMsg_Debug(!isFighting(), "isFighting did not return false as expected");

	clearCombat();

	// Could also be non-garrisoned units here that we need to show
	CvUnit* pLoopUnit;
	for(int iUnitLoop = 0; iUnitLoop < pPlot->getNumUnits(); iUnitLoop++)
	{
		pLoopUnit = pPlot->getUnitByIndex(iUnitLoop);

		// Only show units that belong to this city's owner - that way we don't show units on EVERY city capture (since the old city is deleted in this case)
		if(getOwner() == pLoopUnit->getOwner())
		{
			auto_ptr<ICvUnit1> pDllUnit(new CvDllUnit(pLoopUnit));
			gDLL->GameplayUnitVisibility(pDllUnit.get(), !pLoopUnit->isInvisible(GC.getGame().getActiveTeam(),true) /*bVisible*/);
		}
	}

	for(int iPlotLoop = 0; iPlotLoop < GC.getMap().numPlots(); iPlotLoop++)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndexUnchecked(iPlotLoop);
		if(NULL != pLoopPlot && pLoopPlot->GetCityPurchaseOwner() == getOwner() && pLoopPlot->GetCityPurchaseID() == GetID())
		{
			pLoopPlot->ClearCityPurchaseInfo();
			pLoopPlot->setOwner(NO_PLAYER, NO_PLAYER, /*bCheckUnits*/ true, /*bUpdateResources*/ true);
		}
	}

	pPlot->setPlotCity(NULL);

	area()->changeCitiesPerPlayer(getOwner(), -1);

	GET_TEAM(getTeam()).changeNumCities(-1);

	GC.getGame().changeNumCities(-1);

	CvAssertMsg(getNumGreatPeople() == 0, "getNumGreatPeople is expected to be 0");
	CvAssertMsg(!isProduction(), "isProduction is expected to be false");

	eOwner = getOwner();

	GET_PLAYER(getOwner()).GetFlavorManager()->RemoveFlavorRecipient(m_pCityStrategyAI);

	if(m_unitBeingBuiltForOperation.IsValid())
	{
		GET_PLAYER(getOwner()).CityUncommitToBuildUnitForOperationSlot(m_unitBeingBuiltForOperation);
		m_unitBeingBuiltForOperation.Invalidate();
	}
}

//	--------------------------------------------------------------------------------
void CvCity::PostKill(bool bCapital, CvPlot* pPlot, PlayerTypes eOwner)
{
	VALIDATE_OBJECT

	CvPlayer& owningPlayer = GET_PLAYER(eOwner);

	// Recompute Happiness
	owningPlayer.DoUpdateHappiness();

	// Update Unit Maintenance for the player
	owningPlayer.UpdateUnitProductionMaintenanceMod();

	// Update Proximity between this Player and all others
	PlayerTypes ePlayer;
	for(int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
	{
		ePlayer = (PlayerTypes) iPlayerLoop;

		if(ePlayer != eOwner)
		{
			if(GET_PLAYER(ePlayer).isAlive())
			{
				owningPlayer.DoUpdateProximityToPlayer(ePlayer);
				GET_PLAYER(ePlayer).DoUpdateProximityToPlayer(eOwner);
			}
		}
	}

#if defined(MOD_GLOBAL_CITY_WORKING)
	GC.getMap().updateWorkingCity(pPlot,getWorkPlotDistance()*2);
#else	
	GC.getMap().updateWorkingCity(pPlot,NUM_CITY_RINGS*2);
#endif

	if(bCapital)
	{
#if defined(MOD_GLOBAL_NO_CONQUERED_SPACESHIPS)
		owningPlayer.disassembleSpaceship(pPlot);
#endif
		owningPlayer.findNewCapital();
		owningPlayer.SetHasLostCapital(true, getOwner());
		GET_TEAM(owningPlayer.getTeam()).resetVictoryProgress();
	}

	pPlot->setImprovementType((ImprovementTypes)(GC.getRUINS_IMPROVEMENT()));

	if(eOwner == GC.getGame().getActivePlayer())
	{
		DLLUI->setDirty(SelectionButtons_DIRTY_BIT, true);
	}

	DLLUI->setDirty(NationalBorders_DIRTY_BIT, true);

	if(GC.getGame().getActivePlayer() == eOwner)
	{
		CvMap& theMap = GC.getMap();
		theMap.updateDeferredFog();
	}

}

//	--------------------------------------------------------------------------------
#if defined(MOD_GLOBAL_VENICE_KEEPS_RESOURCES) || defined(MOD_GLOBAL_CS_MARRIAGE_KEEPS_RESOURCES)
void CvCity::kill(bool bKeepResources)
#else
void CvCity::kill()
#endif
{
	VALIDATE_OBJECT
	CvPlot* pPlot = plot();
	PlayerTypes eOwner = getOwner();
	bool bCapital = isCapital();

	IDInfo* pUnitNode;
	CvUnit* pLoopUnit;
	pUnitNode = pPlot->headUnitNode();

	FFastSmallFixedList<IDInfo, 25, true, c_eCiv5GameplayDLL > oldUnits;

	while(pUnitNode != NULL)
	{
		oldUnits.insertAtEnd(pUnitNode);
		pUnitNode = pPlot->nextUnitNode((IDInfo*)pUnitNode);
	}

	pUnitNode = oldUnits.head();

	bool bGarrisonFreeMaintenance = GET_PLAYER(eOwner).IsGarrisonFreeMaintenance();

	while(pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(*pUnitNode);
		pUnitNode = oldUnits.next(pUnitNode);

		if(pLoopUnit)
		{
			if(bGarrisonFreeMaintenance && pLoopUnit->GetBaseCombatStrength(true/*bIgnoreEmbarked*/) > 0 && pLoopUnit->getDomainType() == DOMAIN_LAND)
			{
				GET_PLAYER(eOwner).changeExtraUnitCost(pLoopUnit->getUnitInfo().GetExtraMaintenanceCost());
			}
			if(pLoopUnit->IsImmobile())
			{
				pLoopUnit->kill(false, eOwner);
			}
		}
	}

#if defined(MOD_GLOBAL_VENICE_KEEPS_RESOURCES) || defined(MOD_GLOBAL_CS_MARRIAGE_KEEPS_RESOURCES)
	PreKill(bKeepResources);
#else
	PreKill();
#endif

	// get spies out of city
	CvCityEspionage* pCityEspionage = GetCityEspionage();
	if(pCityEspionage)
	{
		for(int i = 0; i < MAX_MAJOR_CIVS; i++)
		{
			int iAssignedSpy = pCityEspionage->m_aiSpyAssignment[i];
			// if there is a spy in the city
			if(iAssignedSpy != -1)
			{
				GET_PLAYER((PlayerTypes)i).GetEspionage()->ExtractSpyFromCity(iAssignedSpy);
			}
		}
	}

	// Delete the city's information here!!!
	CvGameTrade* pkGameTrade = GC.getGame().GetGameTrade();
	if(pkGameTrade)
	{
#if defined(MOD_BUGFIX_MINOR)
		pkGameTrade->ClearAllCityTradeRoutes(plot(), true);
#else
		pkGameTrade->ClearAllCityTradeRoutes(plot());
#endif
	}
	GET_PLAYER(getOwner()).deleteCity(m_iID);
	GET_PLAYER(eOwner).GetCityConnections()->Update();

	// clean up
	PostKill(bCapital, pPlot, eOwner);
}

//	--------------------------------------------------------------------------------
CvPlayer* CvCity::GetPlayer()
{
	VALIDATE_OBJECT
	return &GET_PLAYER(getOwner());
}

//	--------------------------------------------------------------------------------
void CvCity::doTurn()
{
	AI_PERF_FORMAT("City-AI-perf.csv", ("CvCity::doTurn, Turn %03d, %s, %s,", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );

	VALIDATE_OBJECT
	CvPlot* pLoopPlot;
	int iI;

#if defined(MOD_API_UNIFIED_YIELDS_MORE)
	if (getDamage() > 0 && !HasPlague())
#else
	if (getDamage() > 0 )
#endif
	{
		CvAssertMsg(m_iDamage <= GetMaxHitPoints(), "Somehow a city has more damage than hit points. Please show this to a gameplay programmer immediately.");

		int iHitsHealed = GC.getCITY_HIT_POINTS_HEALED_PER_TURN();
		if(isCapital() && !GET_PLAYER(getOwner()).isMinorCiv())
		{
			iHitsHealed++;
		}
		int iBuildingDefense = m_pCityBuildings->GetBuildingDefense();
		int iBuildingDefenseMod = 100 + m_pCityBuildings->GetBuildingDefenseMod() + GET_PLAYER(m_eOwner).getCityDefenseModifierGlobal();
		iBuildingDefense *= iBuildingDefenseMod;
		iBuildingDefense /= 100;
		iHitsHealed += iBuildingDefense / 500;


		//cities heal much faster if process defense

#if defined(MOD_ROG_CORE)
		for (int iPlayerLoop = 0; iPlayerLoop < MAX_MAJOR_CIVS; iPlayerLoop++)
		{
			ChangeNumTimesAttackedThisTurn((PlayerTypes)iPlayerLoop, (-1 * GetNumTimesAttackedThisTurn((PlayerTypes)iPlayerLoop)));
		}

		if (getProductionProcess() != NO_PROCESS)
		{
			CvProcessInfo* pkProcessInfo = GC.getProcessInfo(getProductionProcess());
			if (pkProcessInfo && pkProcessInfo->getDefenseValue() != 0)
			{
				int iPile = getYieldRate(YIELD_PRODUCTION, false) * pkProcessInfo->getDefenseValue();
				iHitsHealed += iPile / 100;
			}
		}

		iHitsHealed += getExtraDamageHeal();
#endif
		iHitsHealed += getExtraDamageHealPercent() * GetMaxHitPoints() / 100;


		changeDamage(-iHitsHealed);
	}
	if(getDamage() < 0)
	{
		setDamage(0);
	}


	if (GetPlagueTurns() > 0)
	{
		int extraRange = 2;
		for (int iDX = -extraRange; iDX <= extraRange; iDX++)
		{
			for (int iDY = -extraRange; iDY <= extraRange; iDY++)
			{
				CvPlot* pTargetPlot = plotXYWithRangeCheck(getX(), getY(), iDX, iDY, 1);
				if (pTargetPlot != NULL)
				{
					for (int iUnitLoop = 0; iUnitLoop < pTargetPlot->getNumUnits(); iUnitLoop++)
					{
						CvUnit* loopUnit = pTargetPlot->getUnitByIndex(iUnitLoop);
						if (loopUnit != NULL && !loopUnit->isPlagueImmune())
						{
							loopUnit->changeDamage(33);
						}
					}
				}
			}
		}
		ChangePlagueTurns(-1);
	}

	
	setDrafted(false);
	setMadeAttack(false);
	GetCityBuildings()->SetSoldBuildingThisTurn(false);

	DoUpdateFeatureSurrounded();

	GetCityStrategyAI()->DoTurn();

	GetCityCitizens()->DoTurn();

	AI_doTurn();

	bool bRazed = DoRazingTurn();

	if(!bRazed)
	{
		DoResistanceTurn();
		DoReligionFounderChange();

		bool bAllowNoProduction = !doCheckProduction();

		doGrowth();

		DoUpdateIndustrialRouteToCapital();

		doProduction(bAllowNoProduction);

		doDecay();

#if defined(MOD_INTERNATIONAL_IMMIGRATION_FOR_SP)
		SetCanDoImmigration(true);
#endif


		{
			AI_PERF_FORMAT_NESTED("City-AI-perf.csv", ("doImprovement, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );
#if defined(MOD_GLOBAL_CITY_WORKING)
			for(iI = 0; iI < GetNumWorkablePlots(); iI++)
#else
			for(iI = 0; iI < NUM_CITY_PLOTS; iI++)
#endif
			{
				pLoopPlot = GetCityCitizens()->GetCityPlotFromIndex(iI);

				if(pLoopPlot != NULL)
				{
					if(pLoopPlot->getWorkingCity() == this)
					{
						if(pLoopPlot->isBeingWorked())
						{
							pLoopPlot->doImprovement();
						}
					}
				}
			}
		}

		// Following function also looks at WLTKD stuff
		DoTestResourceDemanded();

		// Culture accumulation
		if(getJONSCulturePerTurn() > 0)
		{
			ChangeJONSCultureStored(getJONSCulturePerTurn());
		}

#ifdef MOD_GLOBAL_UNLIMITED_ONE_TURN_CULTURE
		if (MOD_GLOBAL_UNLIMITED_ONE_TURN_CULTURE)
		{
			while (GetJONSCultureStored() >= GetJONSCultureThreshold())
			{
				DoJONSCultureLevelIncrease();
			}
		}
		else
		{
			// Enough Culture to acquire a new Plot?
			if (GetJONSCultureStored() >= GetJONSCultureThreshold())
			{
				DoJONSCultureLevelIncrease();
			}
		}
#else
		// Enough Culture to acquire a new Plot?
		if(GetJONSCultureStored() >= GetJONSCultureThreshold())
		{
			DoJONSCultureLevelIncrease();
		}
#endif
		// Resource Demanded Counter
		if(GetResourceDemandedCountdown() > 0)
		{
			ChangeResourceDemandedCountdown(-1);

			if(GetResourceDemandedCountdown() == 0)
			{
				// Pick a Resource to demand
				DoPickResourceDemanded();
			}
		}

		updateStrengthValue();

		DoNearbyEnemy();

#if !defined(NO_ACHIEVEMENTS)
		//Check for Achievements
		if(isHuman() && !GC.getGame().isGameMultiPlayer() && GET_PLAYER(GC.getGame().getActivePlayer()).isLocalPlayer())
		{
			if(getJONSCulturePerTurn()>=100)
			{
				gDLL->UnlockAchievement(ACHIEVEMENT_CITY_100CULTURE);
			}
			if(getYieldRate(YIELD_GOLD, false)>=100)
			{
				gDLL->UnlockAchievement(ACHIEVEMENT_CITY_100GOLD);
			}
			if(getYieldRate(YIELD_SCIENCE, false)>=100)
			{
				gDLL->UnlockAchievement(ACHIEVEMENT_CITY_100SCIENCE);
			}
		}
#endif

		// sending notifications on when routes are connected to the capital
		if(!isCapital())
		{
			CvNotifications* pNotifications = GET_PLAYER(m_eOwner).GetNotifications();
			if(pNotifications)
			{
				CvCity* pPlayerCapital = GET_PLAYER(m_eOwner).getCapitalCity();
				CvAssertMsg(pPlayerCapital, "No capital city?");

				if(m_bRouteToCapitalConnectedLastTurn != m_bRouteToCapitalConnectedThisTurn && pPlayerCapital)
				{
					Localization::String strMessage;
					Localization::String strSummary;

					if(m_bRouteToCapitalConnectedThisTurn)  // connected this turn
					{
						strMessage = Localization::Lookup("TXT_KEY_NOTIFICATION_TRADE_ROUTE_ESTABLISHED");
						strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_TRADE_ROUTE_ESTABLISHED");
						strMessage << getNameKey();
						strMessage << pPlayerCapital->getNameKey();
						pNotifications->Add(NOTIFICATION_TRADE_ROUTE, strMessage.toUTF8(), strSummary.toUTF8(), -1, -1, -1);
					}
					else // lost connection this turn
					{
						strMessage = Localization::Lookup("TXT_KEY_NOTIFICATION_TRADE_ROUTE_BROKEN");
						strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_TRADE_ROUTE_BROKEN");
						strMessage << getNameKey();
						strMessage << pPlayerCapital->getNameKey();
						pNotifications->Add(NOTIFICATION_TRADE_ROUTE_BROKEN, strMessage.toUTF8(), strSummary.toUTF8(), -1, -1, -1);
					}
				}
			}

			m_bRouteToCapitalConnectedLastTurn = m_bRouteToCapitalConnectedThisTurn;
		}

		for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			YieldTypes eYield = (YieldTypes)iI;
			if (eYield == NO_YIELD)
				continue;

			UpdateCityYields(eYield);
		}

		// XXX
#ifdef _DEBUG
		{
			CvPlot* pPlot;
			int iCount;

			for(iI = 0; iI < NUM_YIELD_TYPES; iI++)
			{
				CvAssert(getBaseYieldRate((YieldTypes)iI) >= 0);
				CvAssert(getYieldRate((YieldTypes)iI, false) >= 0);

				iCount = 0;

#if defined(MOD_GLOBAL_CITY_WORKING)
				for(int iJ = 0; iJ < GetNumWorkablePlots(); iJ++)
#else
				for(int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
#endif
				{
					pPlot = GetCityCitizens()->GetCityPlotFromIndex(iJ);

					if(pPlot != NULL)
					{
						if(GetCityCitizens()->IsWorkingPlot(pPlot))
						{
							iCount += pPlot->getYield((YieldTypes)iI);
						}
					}
				}

				for(int iJ = 0; iJ < GC.getNumSpecialistInfos(); iJ++)
				{
					iCount += (GET_PLAYER(getOwner()).specialistYield(((SpecialistTypes)iJ), ((YieldTypes)iI)) * (GetCityCitizens()->GetSpecialistCount((SpecialistTypes)iJ)));
				}

				for(int iJ = 0; iJ < GC.getNumBuildingInfos(); iJ++)
				{
					const BuildingTypes eBuilding = static_cast<BuildingTypes>(iJ);
					CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
					if(pkBuildingInfo)
					{
						iCount += m_pCityBuildings->GetNumActiveBuilding(eBuilding) * (pkBuildingInfo->GetYieldChange(iI) + m_pCityBuildings->GetBuildingYieldChange((BuildingClassTypes)pkBuildingInfo->GetBuildingClassType(), (YieldTypes)iI));
					}
				}

				// Science from Population
				if((YieldTypes) iI == YIELD_SCIENCE)
				{
					iCount += getPopulation() * GC.getSCIENCE_PER_POPULATION();
				}

				CvAssert(iCount == getBaseYieldRate((YieldTypes)iI));
			}
		}
#endif
		// XXX
	}
}


//	--------------------------------------------------------------------------------
bool CvCity::isCitySelected()
{
	VALIDATE_OBJECT
	auto_ptr<ICvCity1> pCity = GC.WrapCityPointer(this);

	return DLLUI->isCitySelected(pCity.get());
}

//	--------------------------------------------------------------------------------
bool CvCity::isThis(const CvCity& pCity) const
{
	VALIDATE_OBJECT
	return pCity.getX() == getX() && pCity.getY() == getY();
}


//	--------------------------------------------------------------------------------
bool CvCity::canBeSelected() const
{
	VALIDATE_OBJECT
	if((getTeam() == GC.getGame().getActiveTeam()) || GC.getGame().isDebugMode())
	{
		return true;
	}

	return false;
}


//	--------------------------------------------------------------------------------
void CvCity::updateYield(bool bRecalcPlotYields)
{
	VALIDATE_OBJECT
		if (bRecalcPlotYields)
		{
			CvPlot* pLoopPlot;
			int iI;

#if defined(MOD_GLOBAL_CITY_WORKING)
			for (iI = 0; iI < GetNumWorkablePlots(); iI++)
#else
			for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
#endif
			{
				pLoopPlot = GetCityCitizens()->GetCityPlotFromIndex(iI);

				if (pLoopPlot != NULL)
				{
					pLoopPlot->updateYield();
				}
			}
		}
	UpdateAllNonPlotYields();
}


void CvCity::UpdateAllNonPlotYields()
{
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		YieldTypes eYield = (YieldTypes)iI;
		if (eYield == NO_YIELD)
			continue;

		//Simplification - errata yields not worth considering.
		if ((YieldTypes)iI > YIELD_GOLDEN_AGE_POINTS && !MOD_API_UNIFIED_YIELDS_MORE)
			break;

		UpdateCityYields(eYield);
	}

}

void CvCity::UpdateCityYields(YieldTypes eYield)
{
	if (eYield == YIELD_CULTURE)
	{
		SetStaticYield(eYield, getJONSCulturePerTurn(false) * 100);
	}
	else if (eYield == YIELD_FAITH)
	{
		SetStaticYield(eYield, GetFaithPerTurn(false) * 100);
	}
	else
	{
		SetStaticYield(eYield, getYieldRateTimes100(eYield, false, false));
	}

	//don't forget tourism
	if (eYield == YIELD_CULTURE || eYield == YIELD_TOURISM)
	{
		CvString Null;
		GetCityCulture()->CalculateBaseTourismBeforeModifiers(Null);
		GetCityCulture()->CalculateBaseTourism(Null);
	}
}


void CvCity::SetStaticYield(YieldTypes eYield, int iValue)
{
	CvAssertMsg(eYield >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eYield < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	m_aiStaticCityYield[eYield] = iValue;
}

int CvCity::GetStaticYield(YieldTypes eYield) const
{
	VALIDATE_OBJECT
	return m_aiStaticCityYield[eYield];
}



//	--------------------------------------------------------------------------------
/// Connected to capital with industrial route? (Railroads)
bool CvCity::IsIndustrialRouteToCapital() const
{
	return m_bIndustrialRouteToCapital;
}

//	--------------------------------------------------------------------------------
/// Connected to capital with industrial route? (Railroads)
void CvCity::SetIndustrialRouteToCapital(bool bValue)
{
	if(bValue != IsIndustrialRouteToCapital())
	{
		m_bIndustrialRouteToCapital = bValue;
	}
}

//	--------------------------------------------------------------------------------
/// Connected to capital with industrial route? (Railroads)
void CvCity::DoUpdateIndustrialRouteToCapital()
{
	AI_PERF_FORMAT("City-AI-perf.csv", ("CvCity::DoUpdateIndustrialRouteToCapital, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );
	// Capital - what do we want to do about this?
	if(isCapital())
	{
	}
	// Non-capital city
	else
	{
		if(GET_PLAYER(getOwner()).IsCapitalConnectedToCity(this, GC.getGame().GetIndustrialRoute()))
		{
			SetIndustrialRouteToCapital(true);
		}
	}
}

//	--------------------------------------------------------------------------------
void CvCity::SetRouteToCapitalConnected(bool bValue)
{
	bool bUpdateReligion = false;

	if(bValue != m_bRouteToCapitalConnectedThisTurn)
	{
		bUpdateReligion = true;
	}

	m_bRouteToCapitalConnectedThisTurn = bValue;

	if(bUpdateReligion)
	{
		UpdateReligion(GetCityReligions()->GetReligiousMajority());
	}

	if(GC.getGame().getGameTurn() == 0)
	{
		m_bRouteToCapitalConnectedLastTurn = bValue;
	}
}

//	--------------------------------------------------------------------------------
#if defined(MOD_API_EXTENSIONS)
bool CvCity::IsRouteToCapitalConnected(void) const
#else
bool CvCity::IsRouteToCapitalConnected(void)
#endif
{
	return m_bRouteToCapitalConnectedThisTurn;
}


//	--------------------------------------------------------------------------------
void CvCity::createGreatGeneral(UnitTypes eGreatPersonUnit, bool bIsFree)
{
	VALIDATE_OBJECT
	GET_PLAYER(getOwner()).createGreatGeneral(eGreatPersonUnit, getX(), getY(), bIsFree);
}

//	--------------------------------------------------------------------------------
void CvCity::createGreatAdmiral(UnitTypes eGreatPersonUnit, bool bIsFree)
{
	VALIDATE_OBJECT
	GET_PLAYER(getOwner()).createGreatAdmiral(eGreatPersonUnit, getX(), getY(), bIsFree);
}

//	--------------------------------------------------------------------------------
CityTaskResult CvCity::doTask(TaskTypes eTask, int iData1, int iData2, bool bOption, bool bAlt, bool bShift, bool bCtrl)
{
	VALIDATE_OBJECT
	CityTaskResult eResult = TASK_COMPLETED;
	switch(eTask)
	{
	case TASK_RAZE:
		GET_PLAYER(getOwner()).raze(this);
		break;

	case TASK_UNRAZE:
		GET_PLAYER(getOwner()).unraze(this);
		break;

	case TASK_DISBAND:
		GET_PLAYER(getOwner()).disband(this);
		break;

	case TASK_GIFT:
		GET_PLAYER((PlayerTypes)iData1).acquireCity(this, false, true);
		break;

	case TASK_SET_AUTOMATED_CITIZENS:
		break;

	case TASK_SET_AUTOMATED_PRODUCTION:
		setProductionAutomated(bOption, bAlt && bShift && bCtrl);
		break;

	case TASK_SET_EMPHASIZE:
		m_pEmphases->SetEmphasize(((EmphasizeTypes)iData1), bOption);
		break;

	case TASK_NO_AUTO_ASSIGN_SPECIALISTS:
		GetCityCitizens()->SetNoAutoAssignSpecialists(bOption);
		break;

	case TASK_ADD_SPECIALIST:
		GetCityCitizens()->DoAddSpecialistToBuilding(/*eBuilding*/ (BuildingTypes) iData2, true);
		break;

	case TASK_REMOVE_SPECIALIST:
		GetCityCitizens()->DoRemoveSpecialistFromBuilding(/*eBuilding*/ (BuildingTypes) iData2, true);
		GetCityCitizens()->DoAddBestCitizenFromUnassigned();
		break;

	case TASK_CHANGE_WORKING_PLOT:
		GetCityCitizens()->DoAlterWorkingPlot(/*CityPlotIndex*/ iData1);
		break;

	case TASK_REMOVE_SLACKER:
		if (GetCityCitizens()->GetNumDefaultSpecialists() > 0)
		{
			GetCityCitizens()->ChangeNumDefaultSpecialists(-1);
			GetCityCitizens()->DoReallocateCitizens();
		}
		break;

	case TASK_CLEAR_WORKING_OVERRIDE:
		clearWorkingOverride(iData1);
		break;

	case TASK_HURRY:
		hurry((HurryTypes)iData1);
		break;

	case TASK_CONSCRIPT:
		conscript();
		break;

	case TASK_CLEAR_ORDERS:
		clearOrderQueue();
		break;

	case TASK_RALLY_PLOT:
		setRallyPlot(GC.getMap().plot(iData1, iData2));
		break;

	case TASK_CLEAR_RALLY_PLOT:
		setRallyPlot(NULL);
		break;

	case TASK_RANGED_ATTACK:
		eResult = rangeStrike(iData1,iData2);
		break;

	case TASK_CREATE_PUPPET:
		DoCreatePuppet();
		break;

	case TASK_ANNEX_PUPPET:
		DoAnnex();
		break;

	default:
		CvAssertMsg(false, "eTask failed to match a valid option");
		break;
	}

	return eResult;
}


//	--------------------------------------------------------------------------------
void CvCity::chooseProduction(UnitTypes eTrainUnit, BuildingTypes eConstructBuilding, ProjectTypes eCreateProject, bool /*bFinish*/, bool /*bFront*/)
{
	VALIDATE_OBJECT
	CvString strTooltip = GetLocalizedText("TXT_KEY_NOTIFICATION_NEW_CONSTRUCTION", getNameKey());

	CvNotifications* pNotifications = GET_PLAYER(getOwner()).GetNotifications();
	if(pNotifications)
	{
		// Figure out what we just finished so we can package it into something the lua will understand
		OrderTypes eOrder = NO_ORDER;
		int iItemID = -1;

		if(eTrainUnit != NO_UNIT)
		{
			eOrder = ORDER_TRAIN;
			iItemID = eTrainUnit;
		}
		else if(eConstructBuilding != NO_BUILDING)
		{
			eOrder = ORDER_CONSTRUCT;
			iItemID = eConstructBuilding;
		}
		else if(eCreateProject != NO_PROJECT)
		{
			eOrder = ORDER_CREATE;
			iItemID = eCreateProject;
		}

		pNotifications->Add(NOTIFICATION_PRODUCTION, strTooltip, strTooltip, getX(), getY(), eOrder, iItemID);
	}
}

#if defined(MOD_GLOBAL_CITY_WORKING)
//	--------------------------------------------------------------------------------
/// How far out this city may buy plots
int CvCity::getBuyPlotDistance() const
{
	int iDistance = GET_PLAYER(getOwner()).getBuyPlotDistance();
	
	iDistance = std::min(MAX_CITY_RADIUS, std::max(getWorkPlotDistance(), iDistance));
	return iDistance;
}

//	--------------------------------------------------------------------------------
/// How far out this city may buy/work plots
int CvCity::getWorkPlotDistance(int iChange) const
{
	int iDistance = GET_PLAYER(getOwner()).getWorkPlotDistance();
	
	// Change distance based on buildings/wonders in this city
	iDistance += GetCityWorkingChange();
	
	iDistance = std::min(MAX_CITY_RADIUS, std::max(MIN_CITY_RADIUS, iDistance+iChange));
	return iDistance;
}

//	--------------------------------------------------------------------------------
/// How many plots this city may work
int CvCity::GetNumWorkablePlots(int iChange) const
{
	int iWorkablePlots = ((6 * (1+getWorkPlotDistance(iChange)) * getWorkPlotDistance(iChange) / 2) + 1);
	return iWorkablePlots;
}
#endif

//	--------------------------------------------------------------------------------
void CvCity::clearWorkingOverride(int iIndex)
{
	VALIDATE_OBJECT
	CvPlot* pPlot;

	pPlot = GetCityCitizens()->GetCityPlotFromIndex(iIndex);

	if(pPlot != NULL)
	{
		pPlot->setWorkingCityOverride(NULL);
	}
}


//	--------------------------------------------------------------------------------
int CvCity::countNumImprovedPlots(ImprovementTypes eImprovement, bool bPotential) const
{
	VALIDATE_OBJECT
	CvPlot* pLoopPlot;
	int iCount;
	int iI;

	iCount = 0;

	CvCityCitizens* pCityCitizens = GetCityCitizens();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for(iI = 0; iI < GetNumWorkablePlots(); iI++)
#else
	for(iI = 0; iI < NUM_CITY_PLOTS; iI++)
#endif
	{
		pLoopPlot = pCityCitizens->GetCityPlotFromIndex(iI);

		if(pLoopPlot != NULL)
		{
			if(pLoopPlot->getWorkingCity() == this)
			{
				if(eImprovement != NO_IMPROVEMENT)
				{
					if((pLoopPlot->getImprovementType() == eImprovement && !pLoopPlot->IsImprovementPillaged()) || (bPotential && pLoopPlot->canHaveImprovement(eImprovement, getTeam())))
					{
						++iCount;
					}
				}
				else if(pLoopPlot->getImprovementType() != NO_IMPROVEMENT && !pLoopPlot->IsImprovementPillaged())
				{
					iCount++;
				}
			}
		}
	}

	return iCount;
}


//	--------------------------------------------------------------------------------
int CvCity::countNumWaterPlots() const
{
	VALIDATE_OBJECT
	CvPlot* pLoopPlot;
	int iCount;
	int iI;

	iCount = 0;

	CvCityCitizens* pCityCitizens = GetCityCitizens();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for(iI = 0; iI < GetNumWorkablePlots(); iI++)
#else
	for(iI = 0; iI < NUM_CITY_PLOTS; iI++)
#endif
	{
		pLoopPlot = pCityCitizens->GetCityPlotFromIndex(iI);

		if(pLoopPlot != NULL)
		{
			if(pLoopPlot->isWater())
			{
				if(pLoopPlot->getWorkingCity() == this)
				{
					iCount++;
				}
			}
		}
	}

	return iCount;
}

//	--------------------------------------------------------------------------------
int CvCity::countNumRiverPlots() const
{
	VALIDATE_OBJECT
	int iCount = 0;

	CvCityCitizens* pCityCitizens = GetCityCitizens();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for(int iI = 0; iI < GetNumWorkablePlots(); iI++)
#else
	for(int iI = 0; iI < NUM_CITY_PLOTS; iI++)
#endif
	{
		CvPlot* pLoopPlot = pCityCitizens->GetCityPlotFromIndex(iI);

		if(pLoopPlot != NULL)
		{
			if(pLoopPlot->isRiver())
			{
				if(pLoopPlot->getWorkingCity() == this)
				{
					++iCount;
				}
			}
		}
	}

	return iCount;
}

//	--------------------------------------------------------------------------------
int CvCity::countNumForestPlots() const
{
	VALIDATE_OBJECT
	int iCount = 0;

#if defined(MOD_GLOBAL_CITY_WORKING)
	for(int iI = 0; iI < GetNumWorkablePlots(); iI++)
#else
	for(int iI = 0; iI < NUM_CITY_PLOTS; iI++)
#endif
	{
		CvPlot* pLoopPlot = GetCityCitizens()->GetCityPlotFromIndex(iI);

		if(pLoopPlot != NULL)
		{
			if(pLoopPlot->getWorkingCity() == this)
			{
				if(pLoopPlot->getFeatureType() == FEATURE_FOREST)
				{
					++iCount;
				}
			}
		}
	}

	return iCount;
}

//	--------------------------------------------------------------------------------
int CvCity::findPopulationRank()
{
	VALIDATE_OBJECT
	if(!m_bPopulationRankValid)
	{
		int iRank = 1;

		int iLoop;
		CvCity* pLoopCity;
		for(pLoopCity = GET_PLAYER(getOwner()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwner()).nextCity(&iLoop))
		{
			if((pLoopCity->getPopulation() > getPopulation()) ||
			        ((pLoopCity->getPopulation() == getPopulation()) && (pLoopCity->GetID() < GetID())))
			{
				iRank++;
			}
		}

		// shenanigans are to get around the const check
		m_bPopulationRankValid = true;
		m_iPopulationRank = iRank;
	}

	return m_iPopulationRank;
}


//	--------------------------------------------------------------------------------
int CvCity::findBaseYieldRateRank(YieldTypes eYield)
{
	VALIDATE_OBJECT
	if(!m_abBaseYieldRankValid[eYield])
	{
		int iRate = getBaseYieldRate(eYield, false);

		int iRank = 1;

		int iLoop;
		CvCity* pLoopCity;
		for(pLoopCity = GET_PLAYER(getOwner()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwner()).nextCity(&iLoop))
		{
			if((pLoopCity->getBaseYieldRate(eYield, false) > iRate) ||
			        ((pLoopCity->getBaseYieldRate(eYield, false) == iRate) && (pLoopCity->GetID() < GetID())))
			{
				iRank++;
			}
		}

		m_abBaseYieldRankValid.setAt(eYield, true);
		m_aiBaseYieldRank.setAt(eYield, iRank);
	}

	return m_aiBaseYieldRank[eYield];
}


//	--------------------------------------------------------------------------------
int CvCity::findYieldRateRank(YieldTypes eYield)
{
	if(!m_abYieldRankValid[eYield])
	{
		int iRate = getYieldRateTimes100(eYield, false);

		int iRank = 1;

		int iLoop;
		CvCity* pLoopCity;
		for(pLoopCity = GET_PLAYER(getOwner()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwner()).nextCity(&iLoop))
		{
			if ((pLoopCity->getYieldRateTimes100(eYield, false) > iRate) ||
			    ((pLoopCity->getYieldRateTimes100(eYield, false) == iRate) && (pLoopCity->GetID() < GetID())))
			{
				iRank++;
			}
		}

		m_abYieldRankValid.setAt(eYield, true);
		m_aiYieldRank.setAt(eYield, iRank);
	}

	return m_aiYieldRank[eYield];
}


//	--------------------------------------------------------------------------------
// Returns one of the upgrades...
UnitTypes CvCity::allUpgradesAvailable(UnitTypes eUnit, int iUpgradeCount) const
{
	VALIDATE_OBJECT
	UnitTypes eUpgradeUnit;
	bool bUpgradeFound;
	bool bUpgradeAvailable;
	bool bUpgradeUnavailable;

	CvAssertMsg(eUnit != NO_UNIT, "eUnit is expected to be assigned (not NO_UNIT)");

	CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);
	if(pkUnitInfo == NULL)
		return NO_UNIT;

	if(iUpgradeCount > GC.getNumUnitClassInfos())
	{
		return NO_UNIT;
	}

	eUpgradeUnit = NO_UNIT;

	bUpgradeFound = false;
	bUpgradeAvailable = false;
	bUpgradeUnavailable = false;

	CvCivilizationInfo& thisCiv = getCivilizationInfo();

	for(int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		const UnitClassTypes eUnitClass = static_cast<UnitClassTypes>(iI);
		CvUnitClassInfo* pkUnitClassInfo = GC.getUnitClassInfo(eUnitClass);
		if(pkUnitClassInfo)
		{
			if(pkUnitInfo->GetUpgradeUnitClass(iI))
			{
				const UnitTypes eLoopUnit = (UnitTypes) thisCiv.getCivilizationUnits(iI);

				if(eLoopUnit != NO_UNIT)
				{
					bUpgradeFound = true;

					const UnitTypes eTempUnit = allUpgradesAvailable(eLoopUnit, (iUpgradeCount + 1));

					if(eTempUnit != NO_UNIT)
					{
						eUpgradeUnit = eTempUnit;
						bUpgradeAvailable = true;
					}
					else
					{
						bUpgradeUnavailable = true;
					}
				}
			}
		}
	}

	if(iUpgradeCount > 0)
	{
		if(bUpgradeFound && bUpgradeAvailable)
		{
			CvAssertMsg(eUpgradeUnit != NO_UNIT, "eUpgradeUnit is expected to be assigned (not NO_UNIT)");
			return eUpgradeUnit;
		}

		if(canTrain(eUnit, false, false, false, true))
		{
			return eUnit;
		}
	}
	else
	{
		if(bUpgradeFound && !bUpgradeUnavailable)
		{
			return eUpgradeUnit;
		}
	}

	return NO_UNIT;
}


//	--------------------------------------------------------------------------------
bool CvCity::isWorldWondersMaxed() const
{
	VALIDATE_OBJECT
	if(GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		return false;
	}

	if(GC.getMAX_WORLD_WONDERS_PER_CITY() == -1)
	{
		return false;
	}

	if(getNumWorldWonders() >= GC.getMAX_WORLD_WONDERS_PER_CITY())
	{
		return true;
	}

	return false;
}


//	--------------------------------------------------------------------------------
bool CvCity::isTeamWondersMaxed() const
{
	VALIDATE_OBJECT
	if(GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		return false;
	}

	if(GC.getMAX_TEAM_WONDERS_PER_CITY() == -1)
	{
		return false;
	}

	if(getNumTeamWonders() >= GC.getMAX_TEAM_WONDERS_PER_CITY())
	{
		return true;
	}

	return false;
}


//	--------------------------------------------------------------------------------
bool CvCity::isNationalWondersMaxed() const
{
	VALIDATE_OBJECT
	int iMaxNumWonders = (GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman()) ? GC.getMAX_NATIONAL_WONDERS_PER_CITY_FOR_OCC() : GC.getMAX_NATIONAL_WONDERS_PER_CITY();

	if(iMaxNumWonders == -1)
	{
		return false;
	}

	if(getNumNationalWonders() >= iMaxNumWonders)
	{
		return true;
	}

	return false;
}


//	--------------------------------------------------------------------------------
bool CvCity::isBuildingsMaxed() const
{
	VALIDATE_OBJECT
	if(GC.getMAX_BUILDINGS_PER_CITY() == -1)
	{
		return false;
	}

	if(GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		return false;
	}

	if(m_pCityBuildings->GetNumBuildings() >= GC.getMAX_BUILDINGS_PER_CITY())
	{
		return true;
	}

	return false;
}


//	--------------------------------------------------------------------------------
bool CvCity::canTrain(UnitTypes eUnit, bool bContinue, bool bTestVisible, bool bIgnoreCost, bool bWillPurchase, CvString* toolTipSink) const
{
	VALIDATE_OBJECT
	if(eUnit == NO_UNIT)
	{
		return false;
	}

	CvUnitEntry* pkUnitEntry = GC.getUnitInfo(eUnit);
	if(pkUnitEntry == NULL)
	{
		return false;
	}

	if(!(GET_PLAYER(getOwner()).canTrain(eUnit, bContinue, bTestVisible, bIgnoreCost, false, toolTipSink)))
	{
		return false;
	}

	if (!bWillPurchase && pkUnitEntry->IsPurchaseOnly())
	{
		return false;
	}

	if(!bTestVisible)
	{
		CvUnitEntry& thisUnitInfo = *pkUnitEntry;
		// Settlers may not be trained in Cities that are too small
		if(thisUnitInfo.IsFound() || thisUnitInfo.IsFoundAbroad())
		{
			int iSizeRequirement = /*2*/ GC.getCITY_MIN_SIZE_FOR_SETTLERS();
			if(getPopulation() < iSizeRequirement)
			{
				GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_SETTLER_SIZE_LIMIT", "", "", iSizeRequirement);
				if(toolTipSink == NULL)
					return false;
			}
		}

		// See if there are any BuildingClass requirements
		for(auto iBuildingClass : thisUnitInfo.GetBuildingClassRequireds())
		{
			const BuildingClassTypes eBuildingClass = (BuildingClassTypes) iBuildingClass;
			CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo(eBuildingClass);
			if(!pkBuildingClassInfo) continue;
			if(GetNumBuildingClass(eBuildingClass) <= 0)
			{
				const BuildingTypes ePrereqBuilding = GET_PLAYER(getOwner()).GetCivBuilding(eBuildingClass);
				CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(ePrereqBuilding);
				if(pkBuildingInfo)
				{
					GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_UNIT_REQUIRES_BUILDING", pkBuildingInfo->GetDescriptionKey());
					if(toolTipSink == NULL)
						return false;
				}
			}
		}

		// Air units can't be built above capacity
		if (pkUnitEntry->GetDomainType() == DOMAIN_AIR)
		{
			int iNumAirUnits = plot()->countNumAirUnits(getTeam());
			if (iNumAirUnits >= GetMaxAirUnits())
			{
				GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_CITY_AT_AIR_CAPACITY");
				if(toolTipSink == NULL)
					return false;
			}
		}

#if defined(MOD_TROOPS_AND_CROPS_FOR_SP)
		//when lacking Troops, can't built combat unit
		if(GET_PLAYER(getOwner()).IsLackingTroops() && (pkUnitEntry->GetCombat() > 0 || pkUnitEntry->GetRangedCombat() > 0) && !pkUnitEntry->IsNoTroops())
		{
			GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_LACKING_TROOPS");
			if(toolTipSink == NULL)
					return false;
		}
#endif
	}

	if(!plot()->canTrain(eUnit, bContinue, bTestVisible))
	{
		return false;
	}

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(getOwner());
		args->Push(GetID());
		args->Push(eUnit);

		// Attempt to execute the game events.
		// Will return false if there are no registered listeners.
		bool bResult = false;
		if(LuaSupport::CallTestAll(pkScriptSystem, "CityCanTrain", args.get(), bResult))
		{
			// Check the result.
			if(bResult == false)
			{
				return false;
			}
		}
	}

	return true;
}

//	--------------------------------------------------------------------------------
bool CvCity::canTrain(UnitCombatTypes eUnitCombat) const
{
	VALIDATE_OBJECT
	for(int i = 0; i < GC.getNumUnitInfos(); i++)
	{
		const UnitTypes eUnit = static_cast<UnitTypes>(i);
		CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);
		if(pkUnitInfo)
		{
			if(pkUnitInfo->GetUnitCombatType() == eUnitCombat)
			{
				if(canTrain(eUnit))
				{
					return true;
				}
			}
		}
	}

	return false;
}


//	--------------------------------------------------------------------------------
#if defined(MOD_API_EXTENSIONS)
bool CvCity::canConstruct(BuildingTypes eBuilding, bool bContinue, bool bTestVisible, bool bIgnoreCost, bool bWillPurchase, CvString* toolTipSink) const
#else
bool CvCity::canConstruct(BuildingTypes eBuilding, bool bContinue, bool bTestVisible, bool bIgnoreCost, CvString* toolTipSink) const
#endif
{
	VALIDATE_OBJECT
	BuildingTypes ePrereqBuilding = NO_BUILDING;
	int iI;

	if(eBuilding == NO_BUILDING)
	{
		return false;
	}

	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if(pkBuildingInfo == NULL)
	{
		return false;
	}

	CvPlayerAI& kPlayer = GET_PLAYER(getOwner());
	if(!kPlayer.canConstruct(eBuilding, bContinue, bTestVisible, bIgnoreCost, toolTipSink))
	{
		return false;
	}

	if(pkBuildingInfo->IsNoPuppet() && IsPuppet() && !kPlayer.GetPlayerTraits()->IsNoAnnexing())
	{
		return false;
	}

	if (!bWillPurchase && pkBuildingInfo->IsPurchaseOnly())
	{
		return false;
	}

	if (!kPlayer.isHuman() && pkBuildingInfo->IsHumanOnly())
	{
		return false;
	}

	if(m_pCityBuildings->GetNumBuilding(eBuilding) >= GC.getCITY_MAX_NUM_BUILDINGS())
	{
		return false;
	}

	if(!isValidBuildingLocation(eBuilding))
	{
		return false;
	}

	// Local Resource requirements met?
	if(!IsBuildingLocalResourceValid(eBuilding, bTestVisible, toolTipSink))
	{
		return false;
	}

#if defined(MOD_ROG_CORE)

	if (!IsBuildingEmpireResourceValid(eBuilding, toolTipSink))
	{
		return false;
	}

	if (!IsBuildingFeatureValid(eBuilding, toolTipSink))
	{
		return false;
	}
#endif
	if (!IsBuildingPlotValid(eBuilding, toolTipSink))
	{
		return false;
	}
	// Holy city requirement
	if (pkBuildingInfo->IsRequiresHolyCity() && !GetCityReligions()->IsHolyCityAnyReligion())
	{
		return false;
	}

	int iMinNumReligions = pkBuildingInfo->GetMinNumReligions();
	if(iMinNumReligions > 0 && GetCityReligions()->GetNumReligionsWithFollowers() < iMinNumReligions)
	{
		return false;
	}

	CvCivilizationInfo& thisCivInfo = *GC.getCivilizationInfo(getCivilizationType());
	int iNumBuildingClassInfos = GC.getNumBuildingClassInfos();

	// Can't construct a building to reduce occupied unhappiness if the city isn't occupied
	if(pkBuildingInfo->IsNoOccupiedUnhappiness() && !pkBuildingInfo->IsNotNeedOccupied() &&!IsOccupied())
		return false;

	// Does this city have prereq buildings?
	for(auto iBuildingClass : pkBuildingInfo->GetBuildingClassesNeededInCity())
	{
		if(GetNumBuildingClass((BuildingClassTypes)iBuildingClass) <= 0) return false;
	}
	for(auto iBuildingClass : pkBuildingInfo->GetBuildingClassesNeededGlobal())
	{
		if(kPlayer.getBuildingClassCount((BuildingClassTypes)iBuildingClass) <= 0) return false;
	}
	for(auto iBuilding : pkBuildingInfo->GetBuildingsNeededInCity())
	{
		if(GetCityBuildings()->GetNumBuilding((BuildingTypes)iBuilding) <= 0) return false;
	}
	for (auto iBuilding : pkBuildingInfo->GetBuildingsNeededGlobal())
	{
		if (kPlayer.countNumBuildings((BuildingTypes)iBuilding) <= 0) return false;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// Everything above this is what is checked to see if Building shows up in the list of construction items
	///////////////////////////////////////////////////////////////////////////////////

	const CvBuildingClassInfo& kBuildingClassInfo = pkBuildingInfo->GetBuildingClassInfo();
	if(!bTestVisible)
	{
		if(!bContinue)
		{
			if(getFirstBuildingOrder(eBuilding) != -1)
			{
				return false;
			}
		}

		if(!(kBuildingClassInfo.isNoLimit()))
		{
			if(isWorldWonderClass(kBuildingClassInfo))
			{
				if(isWorldWondersMaxed())
				{
					return false;
				}
			}
			else if(isTeamWonderClass(kBuildingClassInfo))
			{
				if(isTeamWondersMaxed())
				{
					return false;
				}
			}
			else if(isNationalWonderClass(kBuildingClassInfo))
			{
				if(isNationalWondersMaxed())
				{
					return false;
				}
			}
			else
			{
				if(isBuildingsMaxed())
				{
					return false;
				}
			}
		}
	}

	for(auto iBuilding : pkBuildingInfo->GetLockedByBuildings())
	{
		if(m_pCityBuildings->GetNumBuilding((BuildingTypes)iBuilding) > 0) return false;
	}

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(getOwner());
		args->Push(GetID());
		args->Push(eBuilding);

		// Attempt to execute the game events.
		// Will return false if there are no registered listeners.
		bool bResult = false;
		if(LuaSupport::CallTestAll(pkScriptSystem, "CityCanConstruct", args.get(), bResult))
		{
			// Check the result.
			if(bResult == false)
			{
				return false;
			}
		}
	}

	return true;
}


//	--------------------------------------------------------------------------------
bool CvCity::canCreate(ProjectTypes eProject, bool bContinue, bool bTestVisible) const
{
	VALIDATE_OBJECT

	if(!(GET_PLAYER(getOwner()).canCreate(eProject, bContinue, bTestVisible)))
	{
		return false;
	}


	if ((getProjectCount(eProject) >= GC.getProjectInfo(eProject)->CityMaxNum()) && ((GC.getProjectInfo(eProject)->CityMaxNum()) >0) )
	{
		return false;
	}
	

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(getOwner());
		args->Push(GetID());
		args->Push(eProject);

		// Attempt to execute the game events.
		// Will return false if there are no registered listeners.
		bool bResult = false;
		if(LuaSupport::CallTestAll(pkScriptSystem, "CityCanCreate", args.get(), bResult))
		{
			// Check the result.
			if(bResult == false)
			{
				return false;
			}
		}
	}

	return true;
}


//	--------------------------------------------------------------------------------
bool CvCity::canPrepare(SpecialistTypes eSpecialist, bool bContinue) const
{
	VALIDATE_OBJECT

	if(!(GET_PLAYER(getOwner()).canPrepare(eSpecialist, bContinue)))
	{
		return false;
	}

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(getOwner());
		args->Push(GetID());
		args->Push(eSpecialist);

		// Attempt to execute the game events.
		// Will return false if there are no registered listeners.
		bool bResult = false;
		if(LuaSupport::CallTestAll(pkScriptSystem, "CityCanPrepare", args.get(), bResult))
		{
			// Check the result.
			if(bResult == false)
			{
				return false;
			}
		}
	}

	return true;
}

//	--------------------------------------------------------------------------------
bool CvCity::canMaintain(ProcessTypes eProcess, bool bContinue) const
{
	VALIDATE_OBJECT

	if(!(GET_PLAYER(getOwner()).canMaintain(eProcess, bContinue)))
	{
		return false;
	}

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(getOwner());
		args->Push(GetID());
		args->Push(eProcess);

		// Attempt to execute the game events.
		// Will return false if there are no registered listeners.
		bool bResult = false;
		if(LuaSupport::CallTestAll(pkScriptSystem, "CityCanMaintain", args.get(), bResult))
		{
			// Check the result.
			if(bResult == false)
			{
				return false;
			}
		}
	}

	return true;
}


//	--------------------------------------------------------------------------------
bool CvCity::canJoin() const
{
	VALIDATE_OBJECT
	return true;
}

//	--------------------------------------------------------------------------------
// Are there a lot of clearable features around this city?
bool CvCity::IsFeatureSurrounded() const
{
	return m_bFeatureSurrounded;
}

//	--------------------------------------------------------------------------------
// Are there a lot of clearable features around this city?
void CvCity::SetFeatureSurrounded(bool bValue)
{
	if(IsFeatureSurrounded() != bValue)
		m_bFeatureSurrounded = bValue;
}

//	--------------------------------------------------------------------------------
// Are there a lot of clearable features around this city?
void CvCity::DoUpdateFeatureSurrounded()
{
	AI_PERF_FORMAT("City-AI-perf.csv", ("CvCity::DoUpdateFeatureSurrounded, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );
	int iTotalPlots = 0;
	int iFeaturedPlots = 0;

	// Look two tiles around this city in every direction to see if at least half the plots are covered in a removable feature
	const int iRange = 2;

	for(int iDX = -iRange; iDX <= iRange; iDX++)
	{
		for(int iDY = -iRange; iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXYWithRangeCheck(getX(), getY(), iDX, iDY, iRange);

			// Increase total plot count
			iTotalPlots++;

			if(pLoopPlot == NULL)
				continue;

			if(pLoopPlot->getFeatureType() == NO_FEATURE)
				continue;

			// Must be able to remove this thing?
			if(!GC.getFeatureInfo(pLoopPlot->getFeatureType())->IsClearable())
				continue;

			iFeaturedPlots++;
		}
	}

	bool bFeatureSurrounded = false;

	// At least half have coverage?
	if(iFeaturedPlots >= iTotalPlots / 2)
		bFeatureSurrounded = true;

	SetFeatureSurrounded(bFeatureSurrounded);
}

//	--------------------------------------------------------------------------------
/// Extra yield for a resource this city is working?
int CvCity::GetResourceExtraYield(ResourceTypes eResource, YieldTypes eYield) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eResource > -1 && eResource < GC.getNumResourceInfos(), "Invalid resource index.");
	CvAssertMsg(eYield > -1 && eYield < NUM_YIELD_TYPES, "Invalid yield index.");
	return ModifierLookup(m_yieldChanges[eYield].forResource, eResource);
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeResourceExtraYield(ResourceTypes eResource, YieldTypes eYield, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eResource > -1 && eResource < GC.getNumResourceInfos(), "Invalid resource index.");
	CvAssertMsg(eYield > -1 && eYield < NUM_YIELD_TYPES, "Invalid yield index.");

	SCityExtraYields& y = m_yieldChanges[eYield];
	if (ModifierUpdateInsertRemove(y.forResource, eResource, iChange, true))
		updateYield();
}


#if defined(MOD_ROG_CORE)
//	--------------------------------------------------------------------------------
/// Extra yield for a improvement this city is working?
int CvCity::GetImprovementExtraYield(ImprovementTypes eImprovement, YieldTypes eYield) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eImprovement > -1 && eImprovement < GC.getNumImprovementInfos(), "eImprovement is expected to be non-negative (invalid Index)");
	CvAssertMsg(eYield > -1 && eYield < NUM_YIELD_TYPES, "Invalid yield index.");
	return ModifierLookup(m_yieldChanges[eYield].forImprovement, eImprovement);

}

//	--------------------------------------------------------------------------------
void CvCity::ChangeImprovementExtraYield(ImprovementTypes eImprovement, YieldTypes eYield, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eImprovement > -1 && eImprovement < GC.getNumImprovementInfos(), "eImprovement is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eYield > -1 && eYield < NUM_YIELD_TYPES, "Invalid yield index.");
	SCityExtraYields& y = m_yieldChanges[eYield];
	if (ModifierUpdateInsertRemove(y.forImprovement, eImprovement, iChange, true))
		updateYield();

}
#endif

//	--------------------------------------------------------------------------------
/// Extra yield for a Feature this city is working?
int CvCity::GetFeatureExtraYield(FeatureTypes eFeature, YieldTypes eYield) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eFeature > -1 && eFeature < GC.getNumFeatureInfos(), "Invalid Feature index.");
	CvAssertMsg(eYield > -1 && eYield < NUM_YIELD_TYPES, "Invalid yield index.");
	return ModifierLookup(m_yieldChanges[eYield].forFeature, eFeature);
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeFeatureExtraYield(FeatureTypes eFeature, YieldTypes eYield, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eFeature > -1 && eFeature < GC.getNumFeatureInfos(), "Invalid Feature index.");
	CvAssertMsg(eYield > -1 && eYield < NUM_YIELD_TYPES, "Invalid yield index.");

	SCityExtraYields& y = m_yieldChanges[eYield];
	if (ModifierUpdateInsertRemove(y.forFeature, eFeature, iChange, true))
		updateYield();
}

//	--------------------------------------------------------------------------------
/// Extra yield for a Terrain this city is working?
int CvCity::GetTerrainExtraYield(TerrainTypes eTerrain, YieldTypes eYield) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eTerrain > -1 && eTerrain < GC.getNumTerrainInfos(), "Invalid Terrain index.");
	CvAssertMsg(eYield > -1 && eYield < NUM_YIELD_TYPES, "Invalid yield index.");
	return ModifierLookup(m_yieldChanges[eYield].forTerrain, eTerrain);
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeTerrainExtraYield(TerrainTypes eTerrain, YieldTypes eYield, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eTerrain > -1 && eTerrain < GC.getNumTerrainInfos(), "Invalid Terrain index.");
	CvAssertMsg(eYield > -1 && eYield < NUM_YIELD_TYPES, "Invalid yield index.");

	SCityExtraYields& y = m_yieldChanges[eYield];
	if (ModifierUpdateInsertRemove(y.forTerrain, eTerrain, iChange, true))
		updateYield();
}

#if defined(MOD_API_UNIFIED_YIELDS) && defined(MOD_API_PLOT_YIELDS)
//	--------------------------------------------------------------------------------
/// Extra yield for a Plot this city is working?
int CvCity::GetPlotExtraYield(PlotTypes ePlot, YieldTypes eYield) const
{
	return ModifierLookup(m_yieldChanges[eYield].forPlot, ePlot);
}

//	--------------------------------------------------------------------------------
void CvCity::ChangePlotExtraYield(PlotTypes ePlot, YieldTypes eYield, int iChange)
{
	SCityExtraYields& y = m_yieldChanges[eYield];
	if (ModifierUpdateInsertRemove(y.forPlot, ePlot, iChange, true))
		updateYield();
}
#endif



#if defined(MOD_ROG_CORE)
bool CvCity::IsHasFeatureLocal(FeatureTypes eFeature) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eFeature > -1 && eFeature < GC.getNumFeatureInfos(), "Invalid resource index.");

	// See if we have the resource linked to this city, but not connected yet
	bool bFoundFeature = false;

	// Loop through all plots near this City to see if we can find eResource - tests are ordered to optimize performance
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
	{
		CvPlot* pLoopPlot = iterateRingPlots(getX(), getY(), iCityPlotLoop);

		// Invalid plot
		if (pLoopPlot == NULL)
			continue;

		// Doesn't have the resource (ignore team first to save time)
		if (pLoopPlot->getFeatureType() != eFeature)
			continue;

		// Not owned by this player
		if (pLoopPlot->getOwner() != getOwner())
			continue;

		if (pLoopPlot->getWorkingCity() != this)
			//if (pLoopPlot->getOwningCityID() != GetID())
			continue;

		bFoundFeature = true;
		break;
	}

	return bFoundFeature;
}
#endif
//	--------------------------------------------------------------------------------
/// Does this City have eResource nearby?
bool CvCity::IsHasResourceLocal(ResourceTypes eResource, bool bTestVisible) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eResource > -1 && eResource < GC.getNumResourceInfos(), "Invalid resource index.");

	// Actually check to see if we have this Resource to use right now
	if(!bTestVisible)
	{
		return m_paiNumResourcesLocal[eResource] > 0;
	}

	// See if we have the resource linked to this city, but not connected yet

	bool bFoundResourceLinked = false;

	// Loop through all plots near this City to see if we can find eResource - tests are ordered to optimize performance
	CvPlot* pLoopPlot;
#if defined(MOD_GLOBAL_CITY_WORKING)
	for(int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for(int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		pLoopPlot = plotCity(getX(), getY(), iCityPlotLoop);

		// Invalid plot
		if(pLoopPlot == NULL)
			continue;

		// Doesn't have the resource (ignore team first to save time)
		if(pLoopPlot->getResourceType() != eResource)
			continue;

		// Not owned by this player
		if(pLoopPlot->getOwner() != getOwner())
			continue;

		// Team can't see the resource here
		if(pLoopPlot->getResourceType(getTeam()) != eResource)
			continue;

		// Resource not linked to this city
		if(pLoopPlot->GetResourceLinkedCity() != this)
			continue;

		bFoundResourceLinked = true;
		break;
	}

	return bFoundResourceLinked;
}

#if defined(MOD_API_EXTENSIONS) || defined(MOD_TRADE_WONDER_RESOURCE_ROUTES)
int CvCity::GetNumResourceLocal(ResourceTypes eResource, bool bImproved)
{
	VALIDATE_OBJECT
	CvAssertMsg(eResource > -1 && eResource < GC.getNumResourceInfos(), "Invalid resource index.");

	if (!bImproved) {
		return m_paiNumResourcesLocal[eResource];
	} else {
		int iCount = 0;
		CvImprovementEntry* pImprovement = GC.GetGameImprovements()->GetImprovementForResource(eResource);
		CvCityCitizens* pCityCitizens = GetCityCitizens();

#if defined(MOD_GLOBAL_CITY_WORKING)
		for(int iI = 0; iI < GetNumWorkablePlots(); iI++)
#else
		for(int iI = 0; iI < NUM_CITY_PLOTS; iI++)
#endif
		{
			CvPlot* pLoopPlot = pCityCitizens->GetCityPlotFromIndex(iI);

			if (pLoopPlot != NULL && pLoopPlot->getWorkingCity() == this) {
				if (pLoopPlot->getResourceType() == eResource && pLoopPlot->getImprovementType() == ((ImprovementTypes) pImprovement->GetID()) && !pLoopPlot->IsImprovementPillaged()) {
					++iCount;
				}
			}
		}
		
		return iCount;
	}
}
#endif

//	--------------------------------------------------------------------------------
void CvCity::ChangeNumResourceLocal(ResourceTypes eResource, int iChange)
{
	VALIDATE_OBJECT

	CvAssertMsg(eResource >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eResource < GC.getNumResourceInfos(), "eIndex expected to be < GC.getNumResourceInfos()");

	if(iChange != 0)
	{
		bool bOldHasResource = IsHasResourceLocal(eResource, /*bTestVisible*/ false);

		m_paiNumResourcesLocal.setAt(eResource, m_paiNumResourcesLocal[eResource] + iChange);

		if(bOldHasResource != IsHasResourceLocal(eResource, /*bTestVisible*/ false))
		{
			if(IsHasResourceLocal(eResource, /*bTestVisible*/ false))
			{
				// Notification letting player know his city gets a bonus for wonders
				int iWonderMod = GC.getResourceInfo(eResource)->getWonderProductionMod();
				if(iWonderMod != 0)
				{
					CvNotifications* pNotifications = GET_PLAYER(getOwner()).GetNotifications();
					if(pNotifications)
					{
						Localization::String strText = Localization::Lookup("TXT_KEY_NOTIFICATION_CITY_RESOURCE_WONDER_MOD");
						strText << getNameKey() << GC.getResourceInfo(eResource)->GetTextKey() << iWonderMod;
						Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_CITY_RESOURCE_WONDER_MOD_SUMMARY");
						strSummary << getNameKey() << GC.getResourceInfo(eResource)->GetTextKey();
						pNotifications->Add(NOTIFICATION_DISCOVERED_BONUS_RESOURCE, strText.toUTF8(), strSummary.toUTF8(), getX(), getY(), eResource);
					}
				}
			}
		}

		// Building Culture change for a local resource
		for(int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
		{
			const BuildingTypes eBuilding = static_cast<BuildingTypes>(iBuildingLoop);
			CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
			if(pkBuildingInfo)
			{
				// Do we have this building?
				if(GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
				{
					// Does eBuilding give culture with eResource?
					int iCulture = pkBuildingInfo->GetResourceCultureChange(eResource);

					if(iCulture != 0)
#if defined(MOD_BUGFIX_MINOR)
						iCulture *= GetCityBuildings()->GetNumBuilding(eBuilding);
#endif
						ChangeBaseYieldRateFromBuildings(YIELD_CULTURE, iCulture * iChange);

					// Does eBuilding give faith with eResource?
					int iFaith = pkBuildingInfo->GetResourceFaithChange(eResource);

					if(iFaith != 0)
#if defined(MOD_BUGFIX_MINOR)
						iFaith *= GetCityBuildings()->GetNumBuilding(eBuilding);
#endif
					ChangeBaseYieldRateFromBuildings(YIELD_FAITH, iFaith * iChange);
				}
			}
		}
		processResource(eResource, iChange);
	}
}

//	--------------------------------------------------------------------------------
/// Does eBuilding pass the local resource requirement test?
bool CvCity::IsBuildingLocalResourceValid(BuildingTypes eBuilding, bool bTestVisible, CvString* toolTipSink) const
{
	VALIDATE_OBJECT

	int iResourceLoop;
	ResourceTypes eResource;

	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if(pkBuildingInfo == NULL)
		return false;

	// ANDs: City must have ALL of these nearby
	for(auto iResource : pkBuildingInfo->GetLocalResourceAnd())
	{
		eResource = (ResourceTypes)iResource;

		// Doesn't require a resource in this AND slot
		if(eResource == NO_RESOURCE)
			continue;

		CvResourceInfo* pkResource = GC.getResourceInfo(eResource);
		if(pkResource == NULL)
			continue;

		// City doesn't have resource locally - return false immediately
		if(!IsHasResourceLocal(eResource, bTestVisible))
		{
			GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_BUILDING_LOCAL_RESOURCE", pkResource->GetTextKey(), pkResource->GetIconString());
			return false;
		}
	}

	int iOrResources = 0;

	// ORs: City must have ONE of these nearby
	for (auto iResource : pkBuildingInfo->GetLocalResourceOr())
	{
		eResource = (ResourceTypes)iResource;

		// Doesn't require a resource in this AND slot
		if(eResource == NO_RESOURCE)
			continue;

		CvResourceInfo* pkResource = GC.getResourceInfo(eResource);
		if(pkResource == NULL)
			continue;

		// City has resource locally - return true immediately
		if(IsHasResourceLocal(eResource, bTestVisible))
			return true;

		// If we get here it means we passed the AND tests but not one of the OR tests
		GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_BUILDING_LOCAL_RESOURCE", pkResource->GetTextKey(), pkResource->GetIconString());

		// Increment counter for OR we don't have
		iOrResources++;
	}

	// No OR resource requirements (and passed the AND test above)
	if(iOrResources == 0)
		return true;

	return false;
}


#if defined(MOD_ROG_CORE)


bool CvCity::IsBuildingEmpireResourceValid(BuildingTypes eBuilding, CvString* toolTipSink) const
{
	VALIDATE_OBJECT
		CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if (pkBuildingInfo == NULL)
		return false;

	// ANDs: City must have ALL of these nearby
	for (auto iResource : pkBuildingInfo->GetEmpireResourceAnd())
	{
		ResourceTypes eResource = (ResourceTypes)iResource;

		// Doesn't require a feature in this AND slot
		if (eResource == NO_RESOURCE)
			continue;

		CvResourceInfo* pkResource = GC.getResourceInfo(eResource);
		if (pkResource == NULL)
			continue;

		//!IsHasFeatureLocal(eResource)
		// City doesn't have feature locally - return false immediately

		if (GET_PLAYER(getOwner()).getNumResourceAvailable(eResource, 1) <= 0)
		{
			GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_BUILDING_EMPIRE_RESOURCE", pkResource->GetTextKey());
			return false;
		}
	}

	int iOrResources = 0;

	// ORs: City must have ONE of these nearby
	for (auto iResource : pkBuildingInfo->GetEmpireResourceOr())
	{
		ResourceTypes eResource = (ResourceTypes)iResource;

		// Doesn't require a feature in this AND slot
		if (eResource == NO_RESOURCE)
			continue;

		CvResourceInfo* pkResource = GC.getResourceInfo(eResource);
		if (pkResource == NULL)
			continue;

		// City has feature locally - return true immediately
		if (GET_PLAYER(getOwner()).getNumResourceAvailable(eResource, 1) > 0)
			return true;

		// If we get here it means we passed the AND tests but not one of the OR tests
		GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_BUILDING_EMPIRE_RESOURCE", pkResource->GetTextKey());

		// Increment counter for OR we don't have
		iOrResources++;
	}

	// No OR resource requirements (and passed the AND test above)
	return iOrResources == 0;
}


bool CvCity::IsBuildingFeatureValid(BuildingTypes eBuilding, CvString* toolTipSink) const
{
	VALIDATE_OBJECT
		CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if (pkBuildingInfo == NULL)
		return false;

	// ANDs: City must have ALL of these nearby
	for (auto iFeature : pkBuildingInfo->GetFeatureAnd())
	{
		FeatureTypes eFeature = (FeatureTypes)iFeature;

		// Doesn't require a feature in this AND slot
		if (eFeature == NO_FEATURE)
			continue;

		CvFeatureInfo* pkFeature = GC.getFeatureInfo(eFeature);
		if (pkFeature == NULL)
			continue;

		// City doesn't have feature locally - return false immediately
		if (!IsHasFeatureLocal(eFeature))
		{
			GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_BUILDING_LOCAL_FEATURE", pkFeature->GetTextKey());
			return false;
		}
	}

	int iOrFeatures = 0;

	// ORs: City must have ONE of these nearby
	for (auto iFeature : pkBuildingInfo->GetFeatureOr())
	{
		FeatureTypes eFeature = (FeatureTypes)iFeature;

		// Doesn't require a feature in this AND slot
		if (eFeature == NO_FEATURE)
			continue;

		CvFeatureInfo* pkFeature = GC.getFeatureInfo(eFeature);
		if (pkFeature == NULL)
			continue;

		// City has feature locally - return true immediately
		if (IsHasFeatureLocal(eFeature))
			return true;

		// If we get here it means we passed the AND tests but not one of the OR tests
		GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_BUILDING_LOCAL_FEATURE", pkFeature->GetTextKey());

		// Increment counter for OR we don't have
		iOrFeatures++;
	}

	// No OR resource requirements (and passed the AND test above)
	return iOrFeatures == 0;
}



#endif
bool CvCity::IsHasPlotLocal(PlotTypes ePlot) const
{
	VALIDATE_OBJECT
	CvAssertMsg(ePlot > -1 && ePlot < GC.getNumPlotInfos(), "Invalid resource index.");

	// See if we have the resource linked to this city, but not connected yet
	bool bFoundPlot = false;
	// Loop through all plots near this City to see if we can find eResource - tests are ordered to optimize performance
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
	{
		CvPlot* pLoopPlot = iterateRingPlots(getX(), getY(), iCityPlotLoop);
		// Invalid plot
		if (pLoopPlot == NULL)
			continue;

		// Doesn't have the resource (ignore team first to save time)
		if (pLoopPlot->getPlotType() != ePlot)
			continue;

		// Not owned by this player
		if (pLoopPlot->getOwner() != getOwner())
			continue;

		if (pLoopPlot->getWorkingCity() != this)
			//if (pLoopPlot->getOwningCityID() != GetID())
			continue;

		bFoundPlot = true;
		break;
	}

	return bFoundPlot;
}
bool CvCity::IsBuildingPlotValid(BuildingTypes eBuilding, CvString* toolTipSink) const
{
	VALIDATE_OBJECT
	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if (pkBuildingInfo == NULL)
		return false;

	// ANDs: City must have ALL of these plots
	for (auto iPlot : pkBuildingInfo->GetPlotAnd())
	{
		PlotTypes ePlot = (PlotTypes)iPlot;
		// Doesn't require a Plot in this AND slot
		if (ePlot == NO_PLOT)
			continue;

		CvPlotInfo* pkPlot = GC.getPlotInfo(ePlot);
		if (pkPlot == NULL)
			continue;

		// City doesn't have Plot locally - return false immediately
		if (!IsHasPlotLocal(ePlot))
		{
			GC.getGame().BuildCannotPerformActionHelpText(toolTipSink, "TXT_KEY_NO_ACTION_BUILDING_LOCAL_FEATURE", pkPlot->GetTextKey());
			return false;
		}
	}
	return true;
}


//	--------------------------------------------------------------------------------
/// What Resource does this City want so that it goes into WLTKD?
ResourceTypes CvCity::GetResourceDemanded(bool bHideUnknown) const
{
	VALIDATE_OBJECT
	ResourceTypes eResourceDemanded = static_cast<ResourceTypes>(m_iResourceDemanded.get());

	// If we're not hiding the result then don't bother with looking at tech
	if(!bHideUnknown)
	{
		return eResourceDemanded;
	}

	if(eResourceDemanded != NO_RESOURCE)
	{
		TechTypes eRevealTech = (TechTypes) GC.getResourceInfo(eResourceDemanded)->getTechReveal();

		// Is there no Reveal Tech or do we have it?
		if(eRevealTech == NO_TECH || GET_TEAM(getTeam()).GetTeamTechs()->HasTech(eRevealTech))
		{
			return eResourceDemanded;
		}
	}

	// We don't have the Tech to reveal the currently demanded Resource
	return NO_RESOURCE;
}

//	--------------------------------------------------------------------------------
/// Sets what Resource this City wants so that it goes into WLTKD
void CvCity::SetResourceDemanded(ResourceTypes eResource)
{
	VALIDATE_OBJECT
	m_iResourceDemanded = (ResourceTypes) eResource;
}

//	--------------------------------------------------------------------------------
/// Picks a Resource for this City to want
void CvCity::DoPickResourceDemanded(bool bCurrentResourceInvalid)
{
	VALIDATE_OBJECT
	AI_PERF_FORMAT("City-AI-perf.csv", ("CvCity::DoPickResourceDemanded, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );
	// Create the list of invalid Luxury Resources
	FStaticVector<ResourceTypes, 64, true, c_eCiv5GameplayDLL, 0> veInvalidLuxuryResources;
	CvPlot* pLoopPlot;
	ResourceTypes eResource;

	// Loop through all resource infos and invalidate resources that only come from minor civs
	for(int iResourceLoop = 0; iResourceLoop < GC.getNumResourceInfos(); iResourceLoop++)
	{
		eResource = (ResourceTypes) iResourceLoop;
		CvResourceInfo* pkResource = GC.getResourceInfo(eResource);
		if (pkResource && pkResource->getResourceUsage() == RESOURCEUSAGE_LUXURY)
		{
			if (pkResource->isOnlyMinorCivs())
			{
				veInvalidLuxuryResources.push_back(eResource);
			}
		}
	}

	// Loop through all Plots near this City to see if there's Luxuries we should invalidate
#if defined(MOD_GLOBAL_CITY_WORKING)
	for(int iPlotLoop = 0; iPlotLoop < GetNumWorkablePlots(); iPlotLoop++)
#else
	for(int iPlotLoop = 0; iPlotLoop < NUM_CITY_PLOTS; iPlotLoop++)
#endif
	{
		pLoopPlot = plotCity(getX(), getY(), iPlotLoop);

		if(pLoopPlot != NULL)
		{
			eResource = pLoopPlot->getResourceType();

			if(eResource != NO_RESOURCE)
			{
				if(GC.getResourceInfo(eResource)->getResourceUsage() == RESOURCEUSAGE_LUXURY)
				{
					veInvalidLuxuryResources.push_back(eResource);
				}
			}
		}
	}

	// Current Resource demanded may not be a valid choice
	ResourceTypes eCurrentResource = GetResourceDemanded(false);
	if(bCurrentResourceInvalid && eCurrentResource != NO_RESOURCE)
	{
		veInvalidLuxuryResources.push_back(eCurrentResource);
	}

	// Create list of valid Luxuries
	FStaticVector<ResourceTypes, 64, true, c_eCiv5GameplayDLL, 0> veValidLuxuryResources;
	for(int iResourceLoop = 0; iResourceLoop < GC.getNumResourceInfos(); iResourceLoop++)
	{
		eResource = (ResourceTypes) iResourceLoop;

		// Is this a Luxury Resource?
		CvResourceInfo* pkResource = GC.getResourceInfo(eResource);
		if(pkResource && pkResource->getResourceUsage() == RESOURCEUSAGE_LUXURY)
		{
			// Is the Resource actually on the map?
			if(GC.getMap().getNumResources(eResource) > 0)
			{
				// Can't be a minor civ only resource!
				if(!GC.getResourceInfo(eResource)->isOnlyMinorCivs())
				{
					// We must not have this already
					if(GET_PLAYER(getOwner()).getNumResourceAvailable(eResource) == 0)
						veValidLuxuryResources.push_back(eResource);
				}
			}
		}
	}

	// Is there actually anything in our vector? - 0 can be valid if we already have everything, for example
	if(veValidLuxuryResources.size() == 0)
	{
		return;
	}

	// Now pick a Luxury we can use
	int iNumAttempts = 0;
	int iVectorLoop;
	int iVectorIndex;
	bool bResourceValid;

	do
	{
		iVectorIndex = GC.getGame().getJonRandNum(veValidLuxuryResources.size(), "Picking random Luxury for City to demand.");
		eResource = (ResourceTypes) veValidLuxuryResources[iVectorIndex];
		bResourceValid = true;

		// Look at all invalid Resources found to see if our randomly-picked Resource matches any
		for(iVectorLoop = 0; iVectorLoop < (int) veInvalidLuxuryResources.size(); iVectorLoop++)
		{
			if(eResource == veInvalidLuxuryResources[iVectorLoop])
			{
				bResourceValid = false;
				break;
			}
		}

		// Not found nearby?
		if(bResourceValid)
		{
			SetResourceDemanded(eResource);

			// Notification
			CvNotifications* pNotifications = GET_PLAYER(getOwner()).GetNotifications();
			if(pNotifications)
			{
				Localization::String strText = Localization::Lookup("TXT_KEY_NOTIFICATION_CITY_RESOURCE_DEMAND");
				strText << getNameKey() << GC.getResourceInfo(eResource)->GetTextKey();
				Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_CITY_RESOURCE_DEMAND");
				strSummary << getNameKey() << GC.getResourceInfo(eResource)->GetTextKey();
				pNotifications->Add(NOTIFICATION_REQUEST_RESOURCE, strText.toUTF8(), strSummary.toUTF8(), getX(), getY(), eResource);
			}

			return;
		}

		iNumAttempts++;
	}
	while(iNumAttempts < 500);

	// If we're on the debug map it's too small for us to care
	if(GC.getMap().getWorldSize() != WORLDSIZE_DEBUG)
	{
		CvAssertMsg(false, "Gameplay: Didn't find a Luxury for City to demand.");
	}
}

//	--------------------------------------------------------------------------------
/// Checks to see if we have the Resource demanded and if so starts WLTKD in this City
void CvCity::DoTestResourceDemanded()
{
	VALIDATE_OBJECT
	ResourceTypes eResource = GetResourceDemanded();

	if(GetWeLoveTheKingDayCounter() > 0 )
	{
		if (!GET_PLAYER(getOwner()).isGoldenAge() || !GET_PLAYER(getOwner()).CanAlwaysWeLoveKindDayInGoldenAge())
		{
			ChangeWeLoveTheKingDayCounter(-1);
		}

		// WLTKD over!
		if(GetWeLoveTheKingDayCounter() == 0)
		{
			DoPickResourceDemanded();

			if(getOwner() == GC.getGame().getActivePlayer())
			{
				Localization::String localizedText;
				// Know what the next Demanded Resource is
				if(GetResourceDemanded() != NO_RESOURCE)
				{
					localizedText = Localization::Lookup("TXT_KEY_MISC_CITY_WLTKD_ENDED_KNOWN_RESOURCE");
					localizedText << getNameKey() << GC.getResourceInfo(GetResourceDemanded())->GetTextKey();
				}
				// Don't know what the next Demanded Resource is
				else
				{
					localizedText = Localization::Lookup("TXT_KEY_MISC_CITY_WLTKD_ENDED_UNKNOWN_RESOURCE");
					localizedText << getNameKey();
				}

				DLLUI->AddCityMessage(0, GetIDInfo(), getOwner(), false, GC.getEVENT_MESSAGE_TIME(), localizedText.toUTF8());
			}
		}
	}
	else
	{
		if(eResource != NO_RESOURCE)
		{
			// Do we have the right Resource?
			if(GET_PLAYER(getOwner()).getNumResourceTotal(eResource) > 0)
			{
				SetWeLoveTheKingDayCounter(/*20*/ GC.getCITY_RESOURCE_WLTKD_TURNS());

				CvNotifications* pNotifications = GET_PLAYER(getOwner()).GetNotifications();
				if(pNotifications)
				{
					Localization::String strText = Localization::Lookup("TXT_KEY_NOTIFICATION_CITY_WLTKD");
					strText << GC.getResourceInfo(eResource)->GetTextKey() << getNameKey();
					Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_CITY_WLTKD");
					strSummary << getNameKey();
					pNotifications->Add(NOTIFICATION_REQUEST_RESOURCE, strText.toUTF8(), strSummary.toUTF8(), getX(), getY(), eResource);
				}
			}
		}
	}
}

//	--------------------------------------------------------------------------------
/// Figure out how long it should be before this City demands a Resource
void CvCity::DoSeedResourceDemandedCountdown()
{
	VALIDATE_OBJECT

	int iNumTurns = /*15*/ GC.getRESOURCE_DEMAND_COUNTDOWN_BASE();

	if(isCapital())
	{
		iNumTurns += /*25*/ GC.getRESOURCE_DEMAND_COUNTDOWN_CAPITAL_ADD();
	}

	int iRand = /*10*/ GC.getRESOURCE_DEMAND_COUNTDOWN_RAND();
	iNumTurns += GC.getGame().getJonRandNum(iRand, "City Resource demanded rand.");

	SetResourceDemandedCountdown(iNumTurns);
}

//	--------------------------------------------------------------------------------
/// How long before we pick a Resource to demand
int CvCity::GetResourceDemandedCountdown() const
{
	VALIDATE_OBJECT
	return m_iDemandResourceCounter;
}

//	--------------------------------------------------------------------------------
/// How long before we pick a Resource to demand
void CvCity::SetResourceDemandedCountdown(int iValue)
{
	VALIDATE_OBJECT
	m_iDemandResourceCounter = iValue;
}

//	--------------------------------------------------------------------------------
/// How long before we pick a Resource to demand
void CvCity::ChangeResourceDemandedCountdown(int iChange)
{
	VALIDATE_OBJECT
	SetResourceDemandedCountdown(GetResourceDemandedCountdown() + iChange);
}

//	--------------------------------------------------------------------------------
int CvCity::getFoodTurnsLeft() const
{
	VALIDATE_OBJECT
	int iFoodLeft;
	int iTurnsLeft;

	iFoodLeft = (growthThreshold() * 100 - getFoodTimes100());

	if(foodDifferenceTimes100() <= 0)
	{
		return iFoodLeft;
	}

	iTurnsLeft = (iFoodLeft / foodDifferenceTimes100());

	if((iTurnsLeft * foodDifferenceTimes100()) <  iFoodLeft)
	{
		iTurnsLeft++;
	}

	return std::max(1, iTurnsLeft);
}


//	--------------------------------------------------------------------------------
bool CvCity::isProduction() const
{
	VALIDATE_OBJECT
	return (headOrderQueueNode() != NULL);
}


//	--------------------------------------------------------------------------------
bool CvCity::isProductionLimited() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
		{
			CvUnitEntry* pkUnitInfo = GC.getUnitInfo((UnitTypes)(pOrderNode->iData1));
			if(pkUnitInfo)
			{
				return isLimitedUnitClass((UnitClassTypes)(pkUnitInfo->GetUnitClassType()));
			}
		}
		break;

		case ORDER_CONSTRUCT:
		{
			CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo((BuildingTypes)pOrderNode->iData1);
			if(pkBuildingInfo)
			{
				return isLimitedWonderClass(pkBuildingInfo->GetBuildingClassInfo());
			}
		}
		break;

		case ORDER_CREATE:
			return isLimitedProject((ProjectTypes)(pOrderNode->iData1));
			break;

		case ORDER_PREPARE:
			break;

		case ORDER_MAINTAIN:
			break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType failed to match a valid option");
			break;
		}
	}

	return false;
}


//	--------------------------------------------------------------------------------
bool CvCity::isProductionUnit() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		return (pOrderNode->eOrderType == ORDER_TRAIN);
	}

	return false;
}


//	--------------------------------------------------------------------------------
bool CvCity::isProductionBuilding() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		return (pOrderNode->eOrderType == ORDER_CONSTRUCT);
	}

	return false;
}


//	--------------------------------------------------------------------------------
bool CvCity::isProductionProject() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		return (pOrderNode->eOrderType == ORDER_CREATE);
	}

	return false;
}


//	--------------------------------------------------------------------------------
bool CvCity::isProductionSpecialist() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		return (pOrderNode->eOrderType == ORDER_PREPARE);
	}

	return false;
}


//	--------------------------------------------------------------------------------
bool CvCity::isProductionProcess() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		return (pOrderNode->eOrderType == ORDER_MAINTAIN);
	}

	return false;
}


//	--------------------------------------------------------------------------------
bool CvCity::canContinueProduction(OrderData order)
{
	VALIDATE_OBJECT
	switch(order.eOrderType)
	{
	case ORDER_TRAIN:
		return canTrain((UnitTypes)(order.iData1), true);
		break;

	case ORDER_CONSTRUCT:
		return canConstruct((BuildingTypes)(order.iData1), true);
		break;

	case ORDER_CREATE:
		return canCreate((ProjectTypes)(order.iData1), true);
		break;

	case ORDER_PREPARE:
		return canPrepare((SpecialistTypes)(order.iData1), true);
		break;

	case ORDER_MAINTAIN:
		return canMaintain((ProcessTypes)(order.iData1), true);
		break;

	default:
		CvAssertMsg(false, "order.eOrderType failed to match a valid option");
		break;
	}

	return false;
}


//	--------------------------------------------------------------------------------
int CvCity::getProductionExperience(UnitTypes eUnit)
{
	VALIDATE_OBJECT
	int iExperience;

	CvPlayerAI& kOwner = GET_PLAYER(getOwner());

	iExperience = getFreeExperience();
	iExperience += kOwner.getFreeExperience();

	if(eUnit != NO_UNIT)
	{
		CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);
		if(pkUnitInfo)
		{
			DomainTypes eDomain = (DomainTypes)pkUnitInfo->GetDomainType();
			UnitCombatTypes eUnitCombat = (UnitCombatTypes)(pkUnitInfo->GetUnitCombatType());

			if (eUnitCombat != NO_UNITCOMBAT)
			{
				iExperience += getUnitCombatFreeExperience(eUnitCombat);
				iExperience += kOwner.getUnitCombatFreeExperiences(eUnitCombat);
			}
			if (eDomain != NO_DOMAIN)
			{
				iExperience += getDomainFreeExperience(eDomain);
				iExperience += getDomainFreeExperienceFromGreatWorks(eDomain);
				iExperience += getDomainFreeExperienceFromGreatWorksGlobal(eDomain);
				iExperience += GetDomainFreeExperiencesPerPop(eDomain) * getPopulation() / 100;
				iExperience += kOwner.GetDomainFreeExperience(eDomain);
				iExperience += kOwner.GetDomainFreeExperiencesPerPopGlobal(eDomain);
			}

			iExperience += getSpecialistFreeExperience();
#if defined(MOD_BELIEF_NEW_EFFECT_FOR_SP)
			//Experence from Religion
			ReligionTypes eCityReligion = GetCityReligions()->GetReligiousMajority();
			if(MOD_BELIEF_NEW_EFFECT_FOR_SP && eCityReligion != NO_RELIGION)
			{
				int iExBouns = GC.getGame().GetGameReligions()->GetReligion(eCityReligion,getOwner())->m_Beliefs.GetHolyCityUnitExperence();
				if(iExBouns != 0 && GetCityReligions()->IsHolyCityForReligion(eCityReligion) && GET_PLAYER(getOwner()).HasReligion(eCityReligion))
				{
					iExperience += iExBouns;
				}
			}
#endif
		}
	}
	int iMaxExperience = GetUnitMaxExperienceLocal();
	if(iMaxExperience > 0 && iExperience > iMaxExperience)
	{
		iExperience = iMaxExperience;
	}

	return std::max(0, iExperience);
}


//	--------------------------------------------------------------------------------
void CvCity::addProductionExperience(CvUnit* pUnit, bool bConscript)
{
	VALIDATE_OBJECT

	if(pUnit->canAcquirePromotionAny())
	{
#if defined(MOD_UNITS_XP_TIMES_100)
		pUnit->changeExperienceTimes100(getProductionExperience(pUnit->getUnitType()) * 100 / ((bConscript) ? 2 : 1));
#else
		pUnit->changeExperience(getProductionExperience(pUnit->getUnitType()) / ((bConscript) ? 2 : 1));
#endif
		
#if !defined(NO_ACHIEVEMENTS)
		// XP2 Achievement
		if (getOwner() != NO_PLAYER)
		{
			CvPlayer& kOwner = GET_PLAYER(getOwner());
			if (!GC.getGame().isGameMultiPlayer() && kOwner.isHuman() && kOwner.isLocalPlayer())
			{
				// This unit begins with a promotion from XP, and part of that XP came from filled Great Work slots
#if defined(MOD_UNITS_XP_TIMES_100)
				if ((pUnit->getExperienceTimes100() / 100) >= pUnit->experienceNeeded() && getDomainFreeExperienceFromGreatWorks((DomainTypes)pUnit->getUnitInfo().GetDomainType()) > 0)
#else
				if (pUnit->getExperience() >= pUnit->experienceNeeded() && getDomainFreeExperienceFromGreatWorks((DomainTypes)pUnit->getUnitInfo().GetDomainType()) > 0)
#endif
				{
					// We have a Royal Library
					BuildingTypes eRoyalLibrary = (BuildingTypes) GC.getInfoTypeForString("BUILDING_ROYAL_LIBRARY", true);
					if (eRoyalLibrary != NO_BUILDING && GetCityBuildings()->GetNumBuilding(eRoyalLibrary) > 0)
					{
						gDLL->UnlockAchievement(ACHIEVEMENT_XP2_19);
					}
				}
			}
		}
#endif
	}

	std::tr1::unordered_set<int> vCityFreePromotions;
	
	ReligionTypes eMajority = GetCityReligions()->GetReligiousMajority();
	if (eMajority != NO_RELIGION)
	{
		const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eMajority, getOwner());
		if (pReligion)
		{
			for(const auto iPromotion : pReligion->m_Beliefs.GetFollowingCityFreePromotion())
			{
				vCityFreePromotions.insert(iPromotion);
			}
		}
	}
	BeliefTypes eBelief = GetCityReligions()->GetSecondaryReligionPantheonBelief();
	if(eBelief != NO_BELIEF)
	{
		const CvBeliefEntry* pkBelief = GC.GetGameBeliefs()->GetEntry(eBelief);
		int iPromotion = pkBelief->GetFollowingCityFreePromotion();
		if(iPromotion != NO_PROMOTION) vCityFreePromotions.insert(iPromotion);
	}
	for(int iI = 0; iI < GC.getNumPromotionInfos(); iI++)
	{
		const PromotionTypes ePromotion = static_cast<PromotionTypes>(iI);
		if(isFreePromotion(ePromotion)) vCityFreePromotions.insert(ePromotion);
	}

	for(const auto iI : vCityFreePromotions)
	{
		const PromotionTypes ePromotion = static_cast<PromotionTypes>(iI);
		CvPromotionEntry* pkPromotionInfo = GC.getPromotionInfo(ePromotion);
		if(pkPromotionInfo && !pUnit->isHasPromotion(ePromotion))
		{
			if(::IsPromotionValidForUnit(ePromotion, *pUnit))
			{
				pUnit->setHasPromotion(ePromotion, true);
			}
		}
	}

	pUnit->testPromotionReady();

#if defined(MOD_PROMOTION_NEW_EFFECT_FOR_SP)
	if(MOD_PROMOTION_NEW_EFFECT_FOR_SP && pUnit->GetMultipleInitExperence() > 0)
	{
		pUnit->changeExperienceTimes100(pUnit->getExperienceTimes100() * (pUnit->GetMultipleInitExperence()) / 100);
	}
#endif	
}


//	--------------------------------------------------------------------------------
UnitTypes CvCity::getProductionUnit() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
			return ((UnitTypes)(pOrderNode->iData1));
			break;

		case ORDER_CONSTRUCT:
		case ORDER_CREATE:
		case ORDER_PREPARE:
		case ORDER_MAINTAIN:
			break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType failed to match a valid option");
			break;
		}
	}

	return NO_UNIT;
}


//	--------------------------------------------------------------------------------
UnitAITypes CvCity::getProductionUnitAI() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
			return ((UnitAITypes)(pOrderNode->iData2));
			break;

		case ORDER_CONSTRUCT:
		case ORDER_CREATE:
		case ORDER_PREPARE:
		case ORDER_MAINTAIN:
			break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType failed to match a valid option");
			break;
		}
	}

	return NO_UNITAI;
}


//	--------------------------------------------------------------------------------
BuildingTypes CvCity::getProductionBuilding() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
			break;

		case ORDER_CONSTRUCT:
			return ((BuildingTypes)(pOrderNode->iData1));
			break;

		case ORDER_CREATE:
		case ORDER_PREPARE:
		case ORDER_MAINTAIN:
			break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType failed to match a valid option");
			break;
		}
	}

	return NO_BUILDING;
}


//	--------------------------------------------------------------------------------
ProjectTypes CvCity::getProductionProject() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
		case ORDER_CONSTRUCT:
			break;

		case ORDER_CREATE:
			return ((ProjectTypes)(pOrderNode->iData1));
			break;

		case ORDER_PREPARE:
		case ORDER_MAINTAIN:
			break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType failed to match a valid option");
			break;
		}
	}

	return NO_PROJECT;
}


//	--------------------------------------------------------------------------------
SpecialistTypes CvCity::getProductionSpecialist() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
		case ORDER_CONSTRUCT:
		case ORDER_CREATE:
			break;

		case ORDER_PREPARE:
			return ((SpecialistTypes)(pOrderNode->iData1));
			break;

		case ORDER_MAINTAIN:
			break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType failed to match a valid option");
			break;
		}
	}

	return NO_SPECIALIST;
}

//	--------------------------------------------------------------------------------
ProcessTypes CvCity::getProductionProcess() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
		case ORDER_CONSTRUCT:
		case ORDER_CREATE:
		case ORDER_PREPARE:
			break;

		case ORDER_MAINTAIN:
			return ((ProcessTypes)(pOrderNode->iData1));
			break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType failed to match a valid option");
			break;
		}
	}

	return NO_PROCESS;
}


//	--------------------------------------------------------------------------------
const char* CvCity::getProductionName() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
		{
			CvUnitEntry* pkUnitInfo = GC.getUnitInfo((UnitTypes)pOrderNode->iData1);
			if(pkUnitInfo)
			{
				return pkUnitInfo->GetDescription();
			}
		}
		break;

		case ORDER_CONSTRUCT:
		{
			CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo((BuildingTypes)pOrderNode->iData1);
			if(pkBuildingInfo)
			{
				return pkBuildingInfo->GetDescription();
			}
		}
		break;

		case ORDER_CREATE:
		{
			CvProjectEntry* pkProjectInfo = GC.getProjectInfo((ProjectTypes)pOrderNode->iData1);
			if(pkProjectInfo)
			{
				return pkProjectInfo->GetDescription();
			}
		}
		break;

		case ORDER_PREPARE:
		{
			CvSpecialistInfo* pkSpecialistInfo = GC.getSpecialistInfo((SpecialistTypes)pOrderNode->iData1);
			if(pkSpecialistInfo)
			{
				return pkSpecialistInfo->GetDescription();
			}
		}
		break;

		case ORDER_MAINTAIN:
		{
			CvProcessInfo* pkProcessInfo = GC.getProcessInfo((ProcessTypes)pOrderNode->iData1);
			if(pkProcessInfo)
			{
				return pkProcessInfo->GetDescription();
			}
		}
		break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType failed to match a valid option");
			break;
		}
	}

	return "";
}


//	--------------------------------------------------------------------------------
int CvCity::getGeneralProductionTurnsLeft() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
			return getProductionTurnsLeft((UnitTypes)pOrderNode->iData1, 0);
			break;

		case ORDER_CONSTRUCT:
			return getProductionTurnsLeft((BuildingTypes)pOrderNode->iData1, 0);
			break;

		case ORDER_CREATE:
			return getProductionTurnsLeft((ProjectTypes)pOrderNode->iData1, 0);
			break;

		case ORDER_PREPARE:
			return getProductionTurnsLeft((SpecialistTypes)pOrderNode->iData1, 0);
			break;

		case ORDER_MAINTAIN:
#if defined(MOD_PROCESS_STOCKPILE)
			return getProductionTurnsLeft((ProcessTypes)pOrderNode->iData1, 0);
#else
			return 0;
#endif
			break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType failed to match a valid option");
			break;
		}
	}

	return 0;
}


//	--------------------------------------------------------------------------------
const char* CvCity::getProductionNameKey() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
		{
			CvUnitEntry* pkUnitInfo = GC.getUnitInfo((UnitTypes)pOrderNode->iData1);
			if(pkUnitInfo)
			{
				return pkUnitInfo->GetTextKey();
			}
		}
		break;

		case ORDER_CONSTRUCT:
		{
			CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo((BuildingTypes)pOrderNode->iData1);
			if(pkBuildingInfo)
			{
				return pkBuildingInfo->GetTextKey();
			}
		}
		break;

		case ORDER_CREATE:
		{
			CvProjectEntry* pkProjectInfo = GC.getProjectInfo((ProjectTypes)pOrderNode->iData1);
			if(pkProjectInfo)
			{
				return pkProjectInfo->GetTextKey();
			}
		}
		break;

		case ORDER_PREPARE:
		{
			CvSpecialistInfo* pkSpecialistInfo = GC.getSpecialistInfo((SpecialistTypes)pOrderNode->iData1);
			if(pkSpecialistInfo)
			{
				return pkSpecialistInfo->GetTextKey();
			}
		}
		break;

		case ORDER_MAINTAIN:
		{
			CvProcessInfo* pkProcessInfo = GC.getProcessInfo((ProcessTypes)pOrderNode->iData1);
			if(pkProcessInfo)
			{
				return pkProcessInfo->GetTextKey();
			}
		}
		break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType failed to match a valid option");
			break;
		}
	}

	return "";
}


//	--------------------------------------------------------------------------------
bool CvCity::isFoodProduction() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
			return isFoodProduction((UnitTypes)(pOrderNode->iData1));
			break;

		case ORDER_CONSTRUCT:
		case ORDER_CREATE:
		case ORDER_PREPARE:
		case ORDER_MAINTAIN:
			break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType failed to match a valid option");
			break;
		}
	}

	return false;
}


//	--------------------------------------------------------------------------------
bool CvCity::isFoodProduction(UnitTypes eUnit) const
{
	CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);
	if(pkUnitInfo == NULL)
		return false;

	if(pkUnitInfo->IsFoodProduction())
	{
		return true;
	}

	if(GET_PLAYER(getOwner()).isMilitaryFoodProduction())
	{
		if(pkUnitInfo->IsMilitaryProduction())
		{
			return true;
		}
	}

	return false;
}


//	--------------------------------------------------------------------------------
int CvCity::getFirstUnitOrder(UnitTypes eUnit) const
{
	VALIDATE_OBJECT
	int iCount = 0;

	const OrderData* pOrderNode = headOrderQueueNode();

	while(pOrderNode != NULL)
	{
		if(pOrderNode->eOrderType == ORDER_TRAIN)
		{
			if(pOrderNode->iData1 == eUnit)
			{
				return iCount;
			}
		}

		iCount++;

		pOrderNode = nextOrderQueueNode(pOrderNode);
	}

	return -1;
}


//	--------------------------------------------------------------------------------
int CvCity::getFirstBuildingOrder(BuildingTypes eBuilding) const
{
	VALIDATE_OBJECT
	int iCount = 0;

	const OrderData* pOrderNode = headOrderQueueNode();

	while(pOrderNode != NULL)
	{
		if(pOrderNode->eOrderType == ORDER_CONSTRUCT)
		{
			if(pOrderNode->iData1 == eBuilding)
			{
				return iCount;
			}
		}

		iCount++;

		pOrderNode = nextOrderQueueNode(pOrderNode);
	}

	return -1;
}


//	--------------------------------------------------------------------------------
int CvCity::getFirstProjectOrder(ProjectTypes eProject) const
{
	VALIDATE_OBJECT
	int iCount = 0;

	const OrderData* pOrderNode = headOrderQueueNode();

	while(pOrderNode != NULL)
	{
		if(pOrderNode->eOrderType == ORDER_CREATE)
		{
			if(pOrderNode->iData1 == eProject)
			{
				return iCount;
			}
		}

		iCount++;

		pOrderNode = nextOrderQueueNode(pOrderNode);
	}

	return -1;
}


//	--------------------------------------------------------------------------------
int CvCity::getFirstSpecialistOrder(SpecialistTypes eSpecialist) const
{
	VALIDATE_OBJECT
	int iCount = 0;

	const OrderData* pOrderNode = headOrderQueueNode();

	while(pOrderNode != NULL)
	{
		if(pOrderNode->eOrderType == ORDER_PREPARE)
		{
			if(pOrderNode->iData1 == eSpecialist)
			{
				return iCount;
			}
		}

		iCount++;

		pOrderNode = nextOrderQueueNode(pOrderNode);
	}

	return -1;
}

//	--------------------------------------------------------------------------------
int CvCity::getNumTrainUnitAI(UnitAITypes eUnitAI) const
{
	VALIDATE_OBJECT
	int iCount = 0;

	const OrderData* pOrderNode = headOrderQueueNode();

	while(pOrderNode != NULL)
	{
		if(pOrderNode->eOrderType == ORDER_TRAIN)
		{
			if(pOrderNode->iData2 == eUnitAI)
			{
				iCount++;
			}
		}

		pOrderNode = nextOrderQueueNode(pOrderNode);
	}

	return iCount;
}


//	--------------------------------------------------------------------------------
int CvCity::getProduction() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
			return getUnitProduction((UnitTypes)(pOrderNode->iData1));
			break;

		case ORDER_CONSTRUCT:
			return m_pCityBuildings->GetBuildingProduction((BuildingTypes)(pOrderNode->iData1));
			break;

		case ORDER_CREATE:
			return getProjectProduction((ProjectTypes)(pOrderNode->iData1));
			break;

		case ORDER_PREPARE:
			return getSpecialistProduction((SpecialistTypes)(pOrderNode->iData1));
			break;

		case ORDER_MAINTAIN:
#if defined(MOD_PROCESS_STOCKPILE)
			return getProcessProduction((ProcessTypes)(pOrderNode->iData1));
#endif
			break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType failed to match a valid option");
			break;
		}
	}

	return 0;
}


//	--------------------------------------------------------------------------------
int CvCity::getProductionTimes100() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
			return getUnitProductionTimes100((UnitTypes)(pOrderNode->iData1));
			break;

		case ORDER_CONSTRUCT:
			return m_pCityBuildings->GetBuildingProductionTimes100((BuildingTypes)(pOrderNode->iData1));
			break;

		case ORDER_CREATE:
			return getProjectProductionTimes100((ProjectTypes)(pOrderNode->iData1));
			break;

		case ORDER_PREPARE:
			return getSpecialistProductionTimes100((SpecialistTypes)(pOrderNode->iData1));
			break;

		case ORDER_MAINTAIN:
#if defined(MOD_PROCESS_STOCKPILE)
			return getProcessProductionTimes100((ProcessTypes)(pOrderNode->iData1));
#endif
			break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType failed to match a valid option");
			break;
		}
	}

	return 0;
}


//	--------------------------------------------------------------------------------
int CvCity::getProductionNeeded() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
			return getProductionNeeded((UnitTypes)(pOrderNode->iData1));
			break;

		case ORDER_CONSTRUCT:
			return getProductionNeeded((BuildingTypes)(pOrderNode->iData1));
			break;

		case ORDER_CREATE:
			return getProductionNeeded((ProjectTypes)(pOrderNode->iData1));
			break;

		case ORDER_PREPARE:
			return getProductionNeeded((SpecialistTypes)(pOrderNode->iData1));
			break;

		case ORDER_MAINTAIN:
#if defined(MOD_PROCESS_STOCKPILE)
			return getProductionNeeded((ProcessTypes)(pOrderNode->iData1));
#endif
			break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType failed to match a valid option");
			break;
		}
	}

	return INT_MAX;
}

//	--------------------------------------------------------------------------------
int CvCity::getProductionNeeded(UnitTypes eUnit) const
{
	VALIDATE_OBJECT
	int iNumProductionNeeded = GET_PLAYER(getOwner()).getProductionNeeded(eUnit);

	return iNumProductionNeeded;
}

//	--------------------------------------------------------------------------------
int CvCity::getProductionNeeded(BuildingTypes eBuilding) const
{
	VALIDATE_OBJECT
	int iNumProductionNeeded = GET_PLAYER(getOwner()).getProductionNeeded(eBuilding);

	return iNumProductionNeeded;
}

//	--------------------------------------------------------------------------------
int CvCity::getProductionNeeded(ProjectTypes eProject) const
{
	VALIDATE_OBJECT
	int iNumProductionNeeded = GET_PLAYER(getOwner()).getProductionNeeded(eProject);

	CvProjectEntry* pProject = GC.getProjectInfo(eProject);
	if (pProject != NULL)
	{
		iNumProductionNeeded += pProject->CostScalerNumberOfRepeats() * getProjectCount(eProject);
		iNumProductionNeeded += pProject->CostScalerEra() * GET_PLAYER(getOwner()).GetCurrentEra();
	}

	return max(1, iNumProductionNeeded);
}

//	--------------------------------------------------------------------------------
int CvCity::getProductionNeeded(SpecialistTypes eSpecialist) const
{
	VALIDATE_OBJECT
	int iNumProductionNeeded = GET_PLAYER(getOwner()).getProductionNeeded(eSpecialist);

	return iNumProductionNeeded;
}

//	--------------------------------------------------------------------------------
int CvCity::getProductionTurnsLeft() const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
			return getProductionTurnsLeft(((UnitTypes)(pOrderNode->iData1)), 0);
			break;

		case ORDER_CONSTRUCT:
			return getProductionTurnsLeft(((BuildingTypes)(pOrderNode->iData1)), 0);
			break;

		case ORDER_CREATE:
			return getProductionTurnsLeft(((ProjectTypes)(pOrderNode->iData1)), 0);
			break;

		case ORDER_PREPARE:
			return getProductionTurnsLeft(((SpecialistTypes)(pOrderNode->iData1)), 0);
			break;

		case ORDER_MAINTAIN:
#if defined(MOD_PROCESS_STOCKPILE)
			return getProductionTurnsLeft(((ProcessTypes)(pOrderNode->iData1)), 0);
#endif
			break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType failed to match a valid option");
			break;
		}
	}

	return INT_MAX;
}


//	--------------------------------------------------------------------------------
int CvCity::getProductionTurnsLeft(UnitTypes eUnit, int iNum) const
{
	VALIDATE_OBJECT
	int iProduction;
	int iFirstUnitOrder;
	int iProductionNeeded;
	int iProductionModifier;

	iProduction = 0;

	iFirstUnitOrder = getFirstUnitOrder(eUnit);

	if((iFirstUnitOrder == -1) || (iFirstUnitOrder == iNum))
	{
		iProduction += getUnitProductionTimes100(eUnit);
	}

	iProductionNeeded = getProductionNeeded(eUnit) * 100;
	iProductionModifier = getProductionModifier(eUnit);

	return getProductionTurnsLeft(iProductionNeeded, iProduction, getProductionDifferenceTimes100(iProductionNeeded, iProduction, iProductionModifier, isFoodProduction(eUnit), (iNum == 0)), getProductionDifferenceTimes100(iProductionNeeded, iProduction, iProductionModifier, isFoodProduction(eUnit), false));
}


//	--------------------------------------------------------------------------------
int CvCity::getProductionTurnsLeft(BuildingTypes eBuilding, int iNum) const
{
	VALIDATE_OBJECT
	int iProduction;
	int iFirstBuildingOrder;
	int iProductionNeeded;
	int iProductionModifier;

	iProduction = 0;

	iFirstBuildingOrder = getFirstBuildingOrder(eBuilding);

	if((iFirstBuildingOrder == -1) || (iFirstBuildingOrder == iNum))
	{
		iProduction += m_pCityBuildings->GetBuildingProductionTimes100(eBuilding);
	}

	iProductionNeeded = getProductionNeeded(eBuilding) * 100;

	iProductionModifier = getProductionModifier(eBuilding);

	return getProductionTurnsLeft(iProductionNeeded, iProduction, getProductionDifferenceTimes100(iProductionNeeded, iProduction, iProductionModifier, false, (iNum == 0)), getProductionDifferenceTimes100(iProductionNeeded, iProduction, iProductionModifier, false, false));
}


//	--------------------------------------------------------------------------------
int CvCity::getProductionTurnsLeft(ProjectTypes eProject, int iNum) const
{
	VALIDATE_OBJECT
	int iProduction;
	int iFirstProjectOrder;
	int iProductionNeeded;
	int iProductionModifier;

	iProduction = 0;

	iFirstProjectOrder = getFirstProjectOrder(eProject);

	if((iFirstProjectOrder == -1) || (iFirstProjectOrder == iNum))
	{
		iProduction += getProjectProductionTimes100(eProject);
	}

	iProductionNeeded = getProductionNeeded(eProject) * 100;
	iProductionModifier = getProductionModifier(eProject);

	return getProductionTurnsLeft(iProductionNeeded, iProduction, getProductionDifferenceTimes100(iProductionNeeded, iProduction, iProductionModifier, false, (iNum == 0)), getProductionDifferenceTimes100(iProductionNeeded, iProduction, iProductionModifier, false, false));
}


//	--------------------------------------------------------------------------------
int CvCity::getProductionTurnsLeft(SpecialistTypes eSpecialist, int iNum) const
{
	VALIDATE_OBJECT
	int iProduction;
	int iFirstSpecialistOrder;
	int iProductionNeeded;
	int iProductionModifier;

	iProduction = 0;

	iFirstSpecialistOrder = getFirstSpecialistOrder(eSpecialist);

	if((iFirstSpecialistOrder == -1) || (iFirstSpecialistOrder == iNum))
	{
		iProduction += getSpecialistProductionTimes100(eSpecialist);
	}

	iProductionNeeded = getProductionNeeded(eSpecialist) * 100;
	iProductionModifier = getProductionModifier(eSpecialist);

	return getProductionTurnsLeft(iProductionNeeded, iProduction, getProductionDifferenceTimes100(iProductionNeeded, iProduction, iProductionModifier, false, (iNum == 0)), getProductionDifferenceTimes100(iProductionNeeded, iProduction, iProductionModifier, false, false));
}

#if defined(MOD_PROCESS_STOCKPILE)
//	--------------------------------------------------------------------------------
int CvCity::getProductionNeeded(ProcessTypes eProcess) const
{
	VALIDATE_OBJECT

	if (eProcess == GC.getInfoTypeForString("PROCESS_STOCKPILE")) {
		return GET_PLAYER(getOwner()).getMaxStockpile();
	}

	return INT_MAX;
}

//	--------------------------------------------------------------------------------
int CvCity::getProductionTurnsLeft(ProcessTypes eProcess, int) const
{
	VALIDATE_OBJECT

	if (eProcess == GC.getInfoTypeForString("PROCESS_STOCKPILE")) {
		int iProduction = getOverflowProduction();
		int iProductionNeeded = GET_PLAYER(getOwner()).getMaxStockpile();
		int iProductionModifier = getProductionModifier(eProcess);
		int iProductionDifference = getProductionDifference(iProductionNeeded, iProduction, iProductionModifier, false, false);

		return getProductionTurnsLeft(iProductionNeeded, iProduction, iProductionDifference, iProductionDifference);
	}

	return INT_MAX;
}
#endif

//	--------------------------------------------------------------------------------
int CvCity::getProductionTurnsLeft(int iProductionNeeded, int iProduction, int iFirstProductionDifference, int iProductionDifference) const
{
	VALIDATE_OBJECT
	int iProductionLeft;
	int iTurnsLeft;

	iProductionLeft = std::max(0, (iProductionNeeded - iProduction - iFirstProductionDifference));

	if(iProductionDifference == 0)
	{
		return iProductionLeft + 1;
	}

	iTurnsLeft = (iProductionLeft / iProductionDifference);

	if((iTurnsLeft * iProductionDifference) < iProductionLeft)
	{
		iTurnsLeft++;
	}

	iTurnsLeft++;

	return std::max(1, iTurnsLeft);
}

//	--------------------------------------------------------------------------------
int CvCity::GetPurchaseCost(UnitTypes eUnit)
{
	VALIDATE_OBJECT

	CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);
	if(pkUnitInfo == NULL)
	{
		//Should never happen
		return 0;
	}

	int iModifier = pkUnitInfo->GetHurryCostModifier();
#ifdef MOD_API_BUILDING_ENABLE_PURCHASE_UNITS
	if (MOD_API_BUILDING_ENABLE_PURCHASE_UNITS)
	{
		int iCollectionsModifer = GC.GetUnitPurchaseCollections()->GetUnitClassPurchaseCost(this, (UnitClassTypes)pkUnitInfo->GetUnitClassType(), YIELD_GOLD);
		if(iCollectionsModifer > 0 && iModifier > 0) iModifier = std::min(iCollectionsModifer, iModifier);
		else if(iCollectionsModifer > 0) iModifier = iCollectionsModifer;
	}	
#endif

	bool bIsSpaceshipPart = pkUnitInfo->GetSpaceshipProject() != NO_PROJECT;

	if (iModifier == -1 && (!bIsSpaceshipPart || !GET_PLAYER(getOwner()).IsEnablesSSPartPurchase()))
	{
		if(iModifier == -1) return -1;
	}

	int iCost = 0;

	if(MOD_GLOBAL_INCREASE_SS_PART_PURCHASE_COST && bIsSpaceshipPart)
	{
		// for SP, Space Ship should not get any discount
		iCost = GetPurchaseCostFromProduction(getProductionNeeded(eUnit), true);
		iCost *= (int)pow(2.0, (double)GET_PLAYER(getOwner()).GetNumSpaceshipPartPurchased());
	}
	else
	{
		iCost = GetPurchaseCostFromProduction(getProductionNeeded(eUnit));
		iCost *= (100 + iModifier);
		iCost /= 100;

		// Cost of purchasing units modified?
		iCost *= (100 + GET_PLAYER(getOwner()).GetUnitPurchaseCostModifier());
		iCost /= 100;
	}

	// Make the number not be funky
	int iDivisor = /*10*/ GC.getGOLD_PURCHASE_VISIBLE_DIVISOR();
	iCost /= iDivisor;
	iCost *= iDivisor;

	return iCost;
}

//	--------------------------------------------------------------------------------
int CvCity::GetFaithPurchaseCost(UnitTypes eUnit, bool bIncludeBeliefDiscounts)
{
	VALIDATE_OBJECT

	int iCost = 0;
	CvPlayer &kPlayer = GET_PLAYER(m_eOwner);

	CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);
	if(pkUnitInfo == NULL)
	{
		//Should never happen
		return iCost;
	}

	// LATE-GAME GREAT PERSON
	SpecialUnitTypes eSpecialUnitGreatPerson = (SpecialUnitTypes) GC.getInfoTypeForString("SPECIALUNIT_PEOPLE");
	if (pkUnitInfo->GetSpecialUnitType() == eSpecialUnitGreatPerson)
	{
		// We must be into the industrial era
#if defined(MOD_CONFIG_GAME_IN_XML)
		if(kPlayer.GetCurrentEra() >= GD_INT_GET(RELIGION_GP_FAITH_PURCHASE_ERA))
#else
		if(kPlayer.GetCurrentEra() >= GC.getInfoTypeForString("ERA_INDUSTRIAL", true /*bHideAssert*/))
#endif
		{
			// Must be proper great person for our civ
			const UnitClassTypes eUnitClass = (UnitClassTypes)pkUnitInfo->GetUnitClassType();
			if (eUnitClass != NO_UNITCLASS)
			{
				const UnitTypes eThisPlayersUnitType = (UnitTypes)kPlayer.getCivilizationInfo().getCivilizationUnits(eUnitClass);
				ReligionTypes eReligion = kPlayer.GetReligions()->GetReligionCreatedByPlayer();

				if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_PROPHET", true /*bHideAssert*/)) //here
				{
					// Can't be bought if didn't start religion
					if (eReligion == NO_RELIGION)
					{
						iCost = -1;
					}
					else
					{
						iCost = kPlayer.GetReligions()->GetCostNextProphet(true /*bIncludeBeliefDiscounts*/, false /*bAdjustForSpeedDifficulty*/);
					}
				}
				else if (eThisPlayersUnitType == eUnit
					|| (MOD_TRAIN_ALL_CORE && kPlayer.GetPlayerTraits()->IsTrainedAll())
					|| kPlayer.GetCanTrainUnitsFromCapturedOriginalCapitals().count(eUnit) > 0
					|| kPlayer.GetUUFromDualEmpire().count(eUnit) > 0
					|| kPlayer.GetUUFromExtra().count(eUnit) > 0
					|| kPlayer.CanAllUc()
				)
				{
					PolicyBranchTypes eBranch = NO_POLICY_BRANCH_TYPE;
					int iNum = kPlayer.getUnitClassesFromFaith(eUnitClass);

					// Check social policy tree
					if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_WRITER", true /*bHideAssert*/))
					{
						eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_AESTHETICS", true /*bHideAssert*/);
					}
					else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_ARTIST", true /*bHideAssert*/))
					{
						eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_AESTHETICS", true /*bHideAssert*/);
					}
					else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_MUSICIAN", true /*bHideAssert*/))
					{
						eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_AESTHETICS", true /*bHideAssert*/);
					}
					else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_SCIENTIST", true /*bHideAssert*/))
					{
						eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_RATIONALISM", true /*bHideAssert*/);
					}
					else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_MERCHANT", true /*bHideAssert*/))
					{
						eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_COMMERCE", true /*bHideAssert*/);
					}
					else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_ENGINEER", true /*bHideAssert*/))
					{
						eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_TRADITION", true /*bHideAssert*/);
					}
					else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_GREAT_GENERAL", true /*bHideAssert*/))
					{
						eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_HONOR", true /*bHideAssert*/);
					}
					else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_GREAT_ADMIRAL", true /*bHideAssert*/))
					{
						eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_EXPLORATION", true /*bHideAssert*/);
					}

					bool bAllUnlockedByBelief = false;
					const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion, getOwner());
					if(pReligion)
					{	
						if (pReligion->m_Beliefs.IsFaithPurchaseAllGreatPeople())
						{
							bAllUnlockedByBelief = true;
						}
					}

#if defined(MOD_RELIGION_POLICY_BRANCH_FAITH_GP)
					if (MOD_RELIGION_POLICY_BRANCH_FAITH_GP)
					{
						bool bIsUnlocked = bAllUnlockedByBelief;

						if (!bIsUnlocked)
						{
							EraTypes eCurrentEra = kPlayer.GetCurrentEra();

							for (int iPolicyLoop = 0; iPolicyLoop < kPlayer.GetPlayerPolicies()->GetPolicies()->GetNumPolicies(); iPolicyLoop++)
							{
								const PolicyTypes eLoopPolicy = static_cast<PolicyTypes>(iPolicyLoop);
								CvPolicyEntry* pkLoopPolicyInfo = GC.getPolicyInfo(eLoopPolicy);
								if (pkLoopPolicyInfo)
								{
									// We have this policy
									if (kPlayer.HasPolicy(eLoopPolicy))
									{
										if (pkLoopPolicyInfo->IsFaithPurchaseUnitClass(eUnitClass, eCurrentEra))
										{
											bIsUnlocked = true;
											break;
										}
									}
								}
							}
						}

						if (bIsUnlocked)
						{
							iCost = GC.getGame().GetGameReligions()->GetFaithGreatPersonNumber(iNum + 1);	
						}
					}
					else
					{
#endif
					
						if (bAllUnlockedByBelief || (eBranch != NO_POLICY_BRANCH_TYPE && kPlayer.GetPlayerPolicies()->IsPolicyBranchFinished(eBranch) && !kPlayer.GetPlayerPolicies()->IsPolicyBranchBlocked(eBranch)))
						{
							iCost = GC.getGame().GetGameReligions()->GetFaithGreatPersonNumber(iNum + 1);	
						}
#if defined(MOD_RELIGION_POLICY_BRANCH_FAITH_GP)
					}
#endif
				}
			}
		}
	}

	// ALL OTHERS
	else
	{
		// Cost goes up in later eras
		iCost = pkUnitInfo->GetFaithCost();
#ifdef MOD_API_BUILDING_ENABLE_PURCHASE_UNITS
		if (MOD_API_BUILDING_ENABLE_PURCHASE_UNITS)
		{
			int iCollectionsMod = GC.GetUnitPurchaseCollections()->GetUnitClassPurchaseCost(this, (UnitClassTypes)pkUnitInfo->GetUnitClassType(), YIELD_FAITH);
			if(iCollectionsMod > 0 && iCost > 0) iCost = std::min(iCollectionsMod, iCost);
			else if(iCollectionsMod > 0) iCost = iCollectionsMod;
		}
#endif
		CvPlayerAI& kOwner = GET_PLAYER(getOwner());
#ifdef MOD_TRAITS_ENABLE_FAITH_PURCHASE_ALL_COMBAT_UNITS
		if (iCost <= 0 && MOD_TRAITS_ENABLE_FAITH_PURCHASE_ALL_COMBAT_UNITS
			&& (pkUnitInfo->GetUnitCombatType() != -1)
			&& kOwner.GetPlayerTraits()->GetFaithPurchaseCombatUnitCostPercent() > 0 && this->canTrain(eUnit))
		{
			iCost = pkUnitInfo->GetProductionCost() * kOwner.GetPlayerTraits()->GetFaithPurchaseCombatUnitCostPercent() / 100;
		}
#endif
		if(pkUnitInfo->IsFaithCostIncrease())
		{
			iCost += pkUnitInfo->GetFaithCostIncrease() * kOwner.getUnitClassesFromFaith((UnitClassTypes)pkUnitInfo->GetUnitClassType());
		}
		EraTypes eEra = GET_TEAM(kOwner.getTeam()).GetCurrentEra();
		int iMultiplier = GC.getEraInfo(eEra)->getFaithCostMultiplier();
		iCost = iCost * iMultiplier / 100;

		if (pkUnitInfo->IsSpreadReligion() || pkUnitInfo->IsRemoveHeresy())
		{
			iMultiplier = (100 + kOwner.GetPlayerPolicies()->GetNumericModifier(POLICYMOD_FAITH_COST_MODIFIER));
			iCost = iCost * iMultiplier / 100;
		}
	}

	// Adjust for game speed
	iCost *= GC.getGame().getGameSpeedInfo().getTrainPercent();
	iCost /= 100;

	// Adjust for difficulty
	if(!isHuman() && !GET_PLAYER(getOwner()).IsAITeammateOfHuman() && !isBarbarian())
	{
		iCost *= GC.getGame().getHandicapInfo().getAITrainPercent();
		iCost /= 100;
	}

	// Modify by any beliefs
	if(bIncludeBeliefDiscounts && (pkUnitInfo->IsSpreadReligion() || pkUnitInfo->IsRemoveHeresy()) && !pkUnitInfo->IsFoundReligion())
	{
		CvGameReligions* pReligions = GC.getGame().GetGameReligions();
		ReligionTypes eMajority = GetCityReligions()->GetReligiousMajority();
		if(eMajority > RELIGION_PANTHEON)
		{
			const CvReligion* pReligion = pReligions->GetReligion(eMajority, getOwner());
			if(pReligion)
			{
				int iReligionCostMod = pReligion->m_Beliefs.GetMissionaryCostModifier();

				if(iReligionCostMod != 0)
				{
					iCost *= (100 + iReligionCostMod);
					iCost /= 100;
				}
			}
		}
	}

	// Make the number not be funky
	int iDivisor = /*10*/ GC.getGOLD_PURCHASE_VISIBLE_DIVISOR();
	iCost /= iDivisor;
	iCost *= iDivisor;

	return iCost;
}

//	--------------------------------------------------------------------------------
int CvCity::GetPurchaseCost(BuildingTypes eBuilding)
{
	VALIDATE_OBJECT

	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if(pkBuildingInfo == NULL)
		return -1;

	int iModifier = pkBuildingInfo->GetHurryCostModifier();
	int iTempMod = GC.getWONDER_GOLDEN_AGE_PURCHASE_MODIFIER();
	// Buiding that only World wonder
	const CvBuildingClassInfo& kbClassInfo = pkBuildingInfo->GetBuildingClassInfo();
	if (kbClassInfo.getMaxGlobalInstances() == 1 && GET_PLAYER(getOwner()).GetPlayerTraits()->CanPurchaseWonderInGoldenAge() && GET_PLAYER(getOwner()).isGoldenAge() && (iTempMod >1))
	{
		iModifier= iModifier+ iTempMod;
	}

	if(iModifier == -1)
		return -1;

#if defined(MOD_BUILDINGS_PRO_RATA_PURCHASE)
	int iProductionNeeded = getProductionNeeded(eBuilding);

	if (MOD_BUILDINGS_PRO_RATA_PURCHASE) {
		// Deduct any current production towards this building
		int iProductionToDate = m_pCityBuildings->GetBuildingProduction(eBuilding);
		iProductionNeeded -= (iProductionToDate * gCustomMods.getOption("BUILDINGS_PRO_RATA_PURCHASE_DEPRECIATION", 80)) / 100;
	}
	
	int iCost = GetPurchaseCostFromProduction(iProductionNeeded);
#else
	int iCost = GetPurchaseCostFromProduction(getProductionNeeded(eBuilding));
#endif
	iCost *= (100 + iModifier);
	iCost /= 100;

	// Cost of purchasing buildings modified?
	iCost *= (100 + GET_PLAYER(getOwner()).GetPlayerPolicies()->GetNumericModifier(POLICYMOD_BUILDING_PURCHASE_COST_MODIFIER));
	iCost /= 100;

	// Make the number not be funky
	int iDivisor = /*10*/ GC.getGOLD_PURCHASE_VISIBLE_DIVISOR();
	iCost /= iDivisor;
	iCost *= iDivisor;

	return iCost;
}

//	--------------------------------------------------------------------------------
int CvCity::GetFaithPurchaseCost(BuildingTypes eBuilding)
{
	int iCost;

	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if(pkBuildingInfo == NULL)
	{
		//Should never happen
		return 0;
	}

	// Cost goes up in later eras
	iCost = pkBuildingInfo->GetFaithCost();
	int iTraitCost = GET_PLAYER(getOwner()).GetPlayerTraits()->GetBuildingClassFaithCost((BuildingClassTypes)pkBuildingInfo->GetBuildingClassType());
	if(iTraitCost > 0 && iCost > 0) iCost = std::min(iTraitCost, iCost);
	else if(iTraitCost > 0) iCost = iTraitCost;

	EraTypes eEra = GET_TEAM(GET_PLAYER(getOwner()).getTeam()).GetCurrentEra();
	int iMultiplier = GC.getEraInfo(eEra)->getFaithCostMultiplier();
	iCost = iCost * iMultiplier / 100;
	iMultiplier = (100 + GET_PLAYER(getOwner()).GetPlayerPolicies()->GetNumericModifier(POLICYMOD_FAITH_COST_MODIFIER));
	iCost = iCost * iMultiplier / 100;

	// Adjust for game speed
	iCost *= GC.getGame().getGameSpeedInfo().getConstructPercent();
	iCost /= 100;

	// Adjust for difficulty
	if(!isHuman() && !GET_PLAYER(getOwner()).IsAITeammateOfHuman() && !isBarbarian())
	{
		iCost *= GC.getGame().getHandicapInfo().getAIConstructPercent();
		iCost /= 100;
	}

	// Make the number not be funky
	int iDivisor = /*10*/ GC.getGOLD_PURCHASE_VISIBLE_DIVISOR();
	iCost /= iDivisor;
	iCost *= iDivisor;

	return iCost;
}

//	--------------------------------------------------------------------------------
int CvCity::GetPurchaseCost(ProjectTypes eProject)
{
	VALIDATE_OBJECT

	int iCost = GetPurchaseCostFromProduction(getProductionNeeded(eProject));

	// Make the number not be funky
	int iDivisor = /*10*/ GC.getGOLD_PURCHASE_VISIBLE_DIVISOR();
	iCost /= iDivisor;
	iCost *= iDivisor;

	return iCost;
}

//	--------------------------------------------------------------------------------
/// Cost of Purchasing something based on the amount of Production it requires to construct
int CvCity::GetPurchaseCostFromProduction(int iProduction, bool bIsNoHurry)
{
	VALIDATE_OBJECT
	int iPurchaseCost;

	// Gold per Production
	int iPurchaseCostBase = iProduction* /*30*/ GC.getGOLD_PURCHASE_GOLD_PER_PRODUCTION();
	// Cost ramps up
	iPurchaseCost = (int) pow((double) iPurchaseCostBase, (double) /*0.75f*/ GC.getHURRY_GOLD_PRODUCTION_EXPONENT());

	// Hurry Mod (Policies, etc.)
	HurryTypes eHurry = (HurryTypes) GC.getInfoTypeForString("HURRY_GOLD");

	if(eHurry != NO_HURRY && !bIsNoHurry)
	{
		iPurchaseCost *= GetHurryModifier(eHurry);
		iPurchaseCost /= 100;
	}

	// Game Speed modifier
	iPurchaseCost *= GC.getGame().getGameSpeedInfo().getHurryPercent();
	iPurchaseCost /= 100;

	return iPurchaseCost;
}

//	--------------------------------------------------------------------------------
void CvCity::setProduction(int iNewValue)
{
	VALIDATE_OBJECT
	if(isProductionUnit())
	{
		setUnitProduction(getProductionUnit(), iNewValue);
	}
	else if(isProductionBuilding())
	{
		m_pCityBuildings->SetBuildingProduction(getProductionBuilding(), iNewValue);
	}
	else if(isProductionProject())
	{
		setProjectProduction(getProductionProject(), iNewValue);
	}
	else if(isProductionSpecialist())
	{
		setSpecialistProduction(getProductionSpecialist(), iNewValue);
	}
}


//	--------------------------------------------------------------------------------
void CvCity::changeProduction(int iChange)
{
	VALIDATE_OBJECT
	if(isProductionUnit())
	{
		changeUnitProduction(getProductionUnit(), iChange);
	}
	else if(isProductionBuilding())
	{
		m_pCityBuildings->ChangeBuildingProduction(getProductionBuilding(), iChange);
	}
	else if(isProductionProject())
	{
		changeProjectProduction(getProductionProject(), iChange);
	}
	else if(isProductionSpecialist())
	{
		changeSpecialistProduction(getProductionSpecialist(), iChange);
	}
}


//	--------------------------------------------------------------------------------
void CvCity::setProductionTimes100(int iNewValue)
{
	VALIDATE_OBJECT
	if(isProductionUnit())
	{
		setUnitProductionTimes100(getProductionUnit(), iNewValue);
	}
	else if(isProductionBuilding())
	{
		m_pCityBuildings->SetBuildingProductionTimes100(getProductionBuilding(), iNewValue);
	}
	else if(isProductionProject())
	{
		setProjectProductionTimes100(getProductionProject(), iNewValue);
	}
	else if(isProductionSpecialist())
	{
		setSpecialistProductionTimes100(getProductionSpecialist(), iNewValue);
	}
}


//	--------------------------------------------------------------------------------
void CvCity::changeProductionTimes100(int iChange)
{
	VALIDATE_OBJECT
	if(isProductionUnit())
	{
		changeUnitProductionTimes100(getProductionUnit(), iChange);
	}
	else if(isProductionBuilding())
	{
		m_pCityBuildings->ChangeBuildingProductionTimes100(getProductionBuilding(), iChange);
	}
	else if(isProductionProject())
	{
		changeProjectProductionTimes100(getProductionProject(), iChange);
	}
	else if(isProductionSpecialist())
	{
		changeSpecialistProductionTimes100(getProductionSpecialist(), iChange);
	}
	else if(isProductionProcess())
	{
		doProcess();
	}
}


//	--------------------------------------------------------------------------------
int CvCity::getProductionModifier(CvString* toolTipSink) const
{
	VALIDATE_OBJECT
	const OrderData* pOrderNode = headOrderQueueNode();

	int iMultiplier = 0;

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
			iMultiplier += getProductionModifier((UnitTypes)(pOrderNode->iData1), toolTipSink);
			break;

		case ORDER_CONSTRUCT:
			iMultiplier += getProductionModifier((BuildingTypes)(pOrderNode->iData1), toolTipSink);
			break;

		case ORDER_CREATE:
			iMultiplier += getProductionModifier((ProjectTypes)(pOrderNode->iData1), toolTipSink);
			break;

		case ORDER_PREPARE:
			iMultiplier += getProductionModifier((SpecialistTypes)(pOrderNode->iData1), toolTipSink);
			break;

		case ORDER_MAINTAIN:
			iMultiplier += getProductionModifier((ProcessTypes)(pOrderNode->iData1), toolTipSink);
			break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType failed to match a valid option");
			break;
		}
	}

	return iMultiplier;
}

//	--------------------------------------------------------------------------------
int CvCity::getGeneralProductionModifiers(CvString* toolTipSink) const
{
	int iMultiplier = 0;

	// Railroad to capital?
	if(IsIndustrialRouteToCapital())
	{
		const int iTempMod = GC.getINDUSTRIAL_ROUTE_PRODUCTION_MOD();
		iMultiplier += iTempMod;
		if(toolTipSink && iTempMod)
		{
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_RAILROAD_CONNECTION", iTempMod);
		}
	}
	return iMultiplier;
}

//	--------------------------------------------------------------------------------
int CvCity::getProductionModifier(UnitTypes eUnit, CvString* toolTipSink) const
{
	VALIDATE_OBJECT

	CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);
	if(pkUnitInfo == NULL)
	{
		//Unit type doesn't exist!
		return 0;
	}

	CvPlayerAI& thisPlayer = GET_PLAYER(getOwner());

	int iMultiplier = getGeneralProductionModifiers(toolTipSink);

	iMultiplier += thisPlayer.getProductionModifier(eUnit, toolTipSink);

	int iTempMod;

	// Capital Settler bonus
	if(isCapital() && pkUnitInfo->IsFound())
	{
		iTempMod = GET_PLAYER(getOwner()).getCapitalSettlerProductionModifier();
		iMultiplier += iTempMod;
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_CAPITAL_SETTLER_PLAYER", iTempMod);
	}

	// Class Production Bonus
	if (thisPlayer.GetUnitClassProductionModifier((UnitClassTypes)pkUnitInfo->GetUnitClassType()) != 0)
	{
		iTempMod = GET_PLAYER(getOwner()).GetUnitClassProductionModifier((UnitClassTypes)pkUnitInfo->GetUnitClassType());
		iMultiplier += iTempMod;
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_UNIT_CLASS", iTempMod);
	}

	// Domain bonus
	iTempMod = getDomainProductionModifier((DomainTypes)(pkUnitInfo->GetDomainType()));
	iMultiplier += iTempMod;
	if(toolTipSink && iTempMod)
	{
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_UNIT_DOMAIN", iTempMod);
	}

	// UnitCombat class bonus
	UnitCombatTypes eUnitCombatType = (UnitCombatTypes)(pkUnitInfo->GetUnitCombatType());
	if(eUnitCombatType != NO_UNITCOMBAT)
	{
		iTempMod = getUnitCombatProductionModifier(eUnitCombatType);
		iMultiplier += iTempMod;
		if(toolTipSink && iTempMod)
		{
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_UNIT_COMBAT_TYPE", iTempMod);
		}
	}

	// Military production bonus
	if(pkUnitInfo->IsMilitaryProduction())
	{
		iTempMod = getMilitaryProductionModifier();
		iMultiplier += iTempMod;
		if(toolTipSink && iTempMod)
		{
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_MILITARY", iTempMod);
		}
	}

	// City Space mod
	if(pkUnitInfo->GetSpaceshipProject() != NO_PROJECT)
	{
		iTempMod = getSpaceProductionModifier();
		iMultiplier += iTempMod;
		if(toolTipSink && iTempMod)
		{
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_SPACE", iTempMod);
		}

		iTempMod = thisPlayer.getSpaceProductionModifier();

		iMultiplier += iTempMod;
		if(toolTipSink && iTempMod)
		{
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_SPACE_PLAYER", iTempMod);
		}
	}

	// Production bonus for having a particular building
	iTempMod = 0;
	int iBuildingMod = 0;
	BuildingTypes eBuilding;
	for(int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		eBuilding = (BuildingTypes) iI;
		CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
		if(pkBuildingInfo)
		{
			if(GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
			{
				iTempMod = pkUnitInfo->GetBuildingProductionModifier(eBuilding);

				if(iTempMod != 0)
				{
#if defined(MOD_BUGFIX_MINOR)
					iTempMod *= GetCityBuildings()->GetNumBuilding(eBuilding);
#endif
					iBuildingMod += iTempMod;
					if(toolTipSink && iTempMod)
					{
						GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_UNIT_WITH_BUILDING", iTempMod, pkBuildingInfo->GetDescription());
					}
				}
			}
		}
	}
	if(iBuildingMod != 0)
	{
		iMultiplier += iBuildingMod;
	}

	return iMultiplier;
}


//	--------------------------------------------------------------------------------
int CvCity::getProductionModifier(BuildingTypes eBuilding, CvString* toolTipSink) const
{
	VALIDATE_OBJECT
	int iMultiplier = getGeneralProductionModifiers(toolTipSink);
	iMultiplier += GET_PLAYER(getOwner()).getProductionModifier(eBuilding, toolTipSink);

	CvBuildingEntry* thisBuildingEntry = GC.getBuildingInfo(eBuilding);
	if(thisBuildingEntry == NULL)	//should never happen
		return -1;

	const CvBuildingClassInfo& kBuildingClassInfo = thisBuildingEntry->GetBuildingClassInfo();

	int iTempMod;

	// Wonder bonus
	if(::isWorldWonderClass(kBuildingClassInfo) ||
	        ::isTeamWonderClass(kBuildingClassInfo) ||
	        ::isNationalWonderClass(kBuildingClassInfo))
	{
		iTempMod = GetWonderProductionModifier();
		iMultiplier += iTempMod;
		if(toolTipSink && iTempMod)
		{
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_WONDER_CITY", iTempMod);
		}

		iTempMod = GET_PLAYER(getOwner()).getWonderProductionModifier();
		iMultiplier += iTempMod;
		if(toolTipSink && iTempMod)
		{
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_WONDER_PLAYER", iTempMod);
		}

		iTempMod = GetLocalResourceWonderProductionMod(eBuilding, toolTipSink);
		iMultiplier += iTempMod;

		ReligionTypes eMajority = GetCityReligions()->GetReligiousMajority();
		if(eMajority != NO_RELIGION)
		{
			const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eMajority, getOwner());
			if(pReligion)
			{
				// Depends on era of wonder
				EraTypes eEra;
				TechTypes eTech = (TechTypes)thisBuildingEntry->GetPrereqAndTech();
				if(eTech != NO_TECH)
				{
					CvTechEntry* pEntry = GC.GetGameTechs()->GetEntry(eTech);
					if(pEntry)
					{
						eEra = (EraTypes)pEntry->GetEra();
						if(eEra != NO_ERA)
						{
							iTempMod = pReligion->m_Beliefs.GetWonderProductionModifier(eEra);
							BeliefTypes eSecondaryPantheon = GetCityReligions()->GetSecondaryReligionPantheonBelief();
							if (eSecondaryPantheon != NO_BELIEF)
							{
								if((int)eEra < GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon)->GetObsoleteEra())
								{
									iTempMod += GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon)->GetWonderProductionModifier();
								}
							}
							iMultiplier += iTempMod;
							if(toolTipSink && iTempMod)
							{
								GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_WONDER_RELIGION", iTempMod);
							}
						}
					}
				}
			}
		}
#if defined(MOD_ROG_CORE)
		int iNumberOfImprovements = 0;
		CvPlot* pLoopPlot = NULL;
		for (int iJ = 0; iJ < GetNumWorkablePlots(); iJ++)
		{
			pLoopPlot = iterateRingPlots(getX(), getY(), iJ);
			if (pLoopPlot != NULL && pLoopPlot->getOwner() == getOwner())
			{
				if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT && !pLoopPlot->IsImprovementPillaged())
				{
					CvImprovementEntry* pImprovementInfo = GC.getImprovementInfo(pLoopPlot->getImprovementType());
					if (pImprovementInfo->GetWonderProductionModifier() > 0)
					{
						iTempMod = pImprovementInfo->GetWonderProductionModifier();
						iMultiplier += iTempMod;
						iNumberOfImprovements++;
					}
				}
			}
		}
		if (toolTipSink && iTempMod && iNumberOfImprovements)
		{
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_WONDER_IMPROVEMENT", iTempMod * iNumberOfImprovements);
		}
#endif
	}
	// Not-wonder bonus
	else
	{
		iTempMod = m_pCityBuildings->GetBuildingProductionModifier();
		iMultiplier += iTempMod;
		if(toolTipSink && iTempMod)
		{
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_BUILDING_CITY", iTempMod);
		}
	}
	
	int iNumFreeWorldWonderPerCity = GET_PLAYER(getOwner()).GetPlayerTraits()->GetNumFreeWorldWonderPerCity();
	if(iNumFreeWorldWonderPerCity > 0 && iNumFreeWorldWonderPerCity - getNumWorldWonders() > 0 && ::isWorldWonderClass(kBuildingClassInfo))
	{
		int iBaseYield = getBaseYieldRate(YIELD_PRODUCTION, false) * 100;
		iBaseYield += (GetYieldPerPopTimes100(YIELD_PRODUCTION) * getPopulation());
		iBaseYield += (GET_PLAYER(getOwner()).GetYieldPerPopChange(YIELD_PRODUCTION) * getPopulation());
		iBaseYield += (GetYieldPerPopInEmpireTimes100(YIELD_PRODUCTION) * GET_PLAYER(getOwner()).getTotalPopulation());
		iBaseYield += (GetYieldPerReligionTimes100(YIELD_PRODUCTION) * GetCityReligions()->GetNumReligionsWithFollowers());

		iBaseYield /= 100;
		iTempMod = getProductionNeeded(eBuilding) * 100 / iBaseYield;
		// Make sure it is not affected by multiplications
		{
			int iYieldMultiplier = 100 + getYieldRateMultiplier(YIELD_PRODUCTION);
			if(iYieldMultiplier > 0) iTempMod = iTempMod * 100 / iYieldMultiplier;
			// if 0, then it is meanless
			else iTempMod = 0;
		}
		if(!IsNoNuclearWinterLocal())
		{
			int iYieldMultiplier = 100 + GC.getGame().GetNuclearWinterYieldMultiplier(YIELD_PRODUCTION);
			if(iYieldMultiplier > 0) iTempMod = iTempMod * 100 / iYieldMultiplier;
			else iTempMod = 0;
		}
		if(iTempMod > 0)
		{
			iMultiplier += iTempMod;
			if(toolTipSink)
			{
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_WONDER_TRAIT", iTempMod);
			}
		}
	}

	// From policies
	iTempMod = GET_PLAYER(getOwner()).GetPlayerPolicies()->GetBuildingClassProductionModifier((BuildingClassTypes)kBuildingClassInfo.GetID());
	if(iTempMod != 0)
	{
		iMultiplier += iTempMod;
		if(toolTipSink && iTempMod)
		{
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_BUILDING_POLICY", iTempMod);
		}
	}

	// From traits
	iTempMod = GET_PLAYER(getOwner()).GetPlayerTraits()->GetCapitalBuildingDiscount(eBuilding);
	if(iTempMod != 0)
	{
		iMultiplier += iTempMod;
		if(toolTipSink && iTempMod)
		{
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_CAPITAL_BUILDING_TRAIT", iTempMod);
		}
	}

	return iMultiplier;
}

//	--------------------------------------------------------------------------------
int CvCity::getProductionModifier(ProjectTypes eProject, CvString* toolTipSink) const
{
	VALIDATE_OBJECT
	int iMultiplier = getGeneralProductionModifiers(toolTipSink);
	iMultiplier += GET_PLAYER(getOwner()).getProductionModifier(eProject, toolTipSink);

	int iTempMod;

	// City Space mod
	if(GC.getProjectInfo(eProject)->IsSpaceship())
	{
		iTempMod = getSpaceProductionModifier();
		iMultiplier += iTempMod;
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_SPACE", iTempMod);
	}

	return iMultiplier;
}


//	--------------------------------------------------------------------------------
int CvCity::getProductionModifier(SpecialistTypes eSpecialist, CvString* toolTipSink) const
{
	VALIDATE_OBJECT
	int iMultiplier = getGeneralProductionModifiers(toolTipSink);
	iMultiplier += GET_PLAYER(getOwner()).getProductionModifier(eSpecialist, toolTipSink);

	return iMultiplier;
}

//	--------------------------------------------------------------------------------
int CvCity::getProductionModifier(ProcessTypes eProcess, CvString* toolTipSink) const
{
	VALIDATE_OBJECT
	int iMultiplier = getGeneralProductionModifiers(toolTipSink);
	iMultiplier += GET_PLAYER(getOwner()).getProductionModifier(eProcess, toolTipSink);

	return iMultiplier;
}

//	--------------------------------------------------------------------------------
int CvCity::getProductionDifference(int /*iProductionNeeded*/, int /*iProduction*/, int iProductionModifier, bool bFoodProduction, bool bOverflow) const
{
	VALIDATE_OBJECT
	// If we're in anarchy, then no Production is done!
	if(GET_PLAYER(getOwner()).IsAnarchy())
	{
		return 0;
	}
	// If we're in Resistance, then no Production is done!
	if(IsResistance() || IsRazing())
	{
		return 0;
	}

	int iFoodProduction = ((bFoodProduction) ? (GetFoodProduction(getYieldRate(YIELD_FOOD, false) - foodConsumption(true))) : 0);

	int iOverflow = ((bOverflow) ? (getOverflowProduction() + getFeatureProduction()) : 0);

	// Sum up difference
	int iBaseProduction = getBaseYieldRate(YIELD_PRODUCTION, false) * 100;
	iBaseProduction += (GetYieldPerPopTimes100(YIELD_PRODUCTION) * getPopulation());
	iBaseProduction += (GET_PLAYER(getOwner()).GetYieldPerPopChange(YIELD_PRODUCTION) * getPopulation());
	iBaseProduction += (GetYieldPerPopInEmpireTimes100(YIELD_PRODUCTION) * GET_PLAYER(getOwner()).getTotalPopulation());
	iBaseProduction += (GetYieldPerReligionTimes100(YIELD_PRODUCTION) * GetCityReligions()->GetNumReligionsWithFollowers());

	int iModifiedProduction = iBaseProduction * getBaseYieldRateModifier(YIELD_PRODUCTION, iProductionModifier);
	iModifiedProduction /= 10000;

	iModifiedProduction += iOverflow;
	iModifiedProduction += iFoodProduction;

	return iModifiedProduction;

}


//	--------------------------------------------------------------------------------
int CvCity::getCurrentProductionDifference(bool bIgnoreFood, bool bOverflow) const
{
	VALIDATE_OBJECT
	return getProductionDifference(getProductionNeeded(), getProduction(), getProductionModifier(), (!bIgnoreFood && isFoodProduction()), bOverflow);
}

//	--------------------------------------------------------------------------------
// What is the production of this city, not counting modifiers specific to what we happen to be building?
int CvCity::getRawProductionDifference(bool bIgnoreFood, bool bOverflow) const
{
	VALIDATE_OBJECT
	return getProductionDifference(getProductionNeeded(), getProduction(), getGeneralProductionModifiers(), (!bIgnoreFood && isFoodProduction()), bOverflow);
}


//	--------------------------------------------------------------------------------
int CvCity::getProductionDifferenceTimes100(int /*iProductionNeeded*/, int /*iProduction*/, int iProductionModifier, bool bFoodProduction, bool bOverflow) const
{
	VALIDATE_OBJECT
	// If we're in anarchy, then no Production is done!
	if(GET_PLAYER(getOwner()).IsAnarchy())
	{
		return 0;
	}
	// If we're in Resistance, then no Production is done!
	if(IsResistance() || IsRazing())
	{
		return 0;
	}

	int iFoodProduction = ((bFoodProduction) ? GetFoodProduction(getYieldRate(YIELD_FOOD, false) - foodConsumption(true)) : 0);
	iFoodProduction *= 100;

	int iOverflow = ((bOverflow) ? (getOverflowProductionTimes100() + getFeatureProduction() * 100) : 0);

	// Sum up difference
	int iBaseProduction = getBaseYieldRate(YIELD_PRODUCTION, false) * 100;
	iBaseProduction += (GetYieldPerPopTimes100(YIELD_PRODUCTION) * getPopulation());
	iBaseProduction += (GET_PLAYER(getOwner()).GetYieldPerPopChange(YIELD_PRODUCTION) * getPopulation());
	iBaseProduction += (GetYieldPerPopInEmpireTimes100(YIELD_PRODUCTION) * GET_PLAYER(getOwner()).getTotalPopulation()) / 100;
	iBaseProduction += (GetYieldPerReligionTimes100(YIELD_PRODUCTION) * GetCityReligions()->GetNumReligionsWithFollowers());

	int iModifiedProduction = iBaseProduction * getBaseYieldRateModifier(YIELD_PRODUCTION, iProductionModifier);
	iModifiedProduction /= 100;

	iModifiedProduction += iOverflow;
	iModifiedProduction += iFoodProduction;

	int iTradeYield = GET_PLAYER(m_eOwner).GetTrade()->GetTradeValuesAtCityTimes100(this, YIELD_PRODUCTION);
	iModifiedProduction += iTradeYield;

	return iModifiedProduction;
}


//	--------------------------------------------------------------------------------
int CvCity::getCurrentProductionDifferenceTimes100(bool bIgnoreFood, bool bOverflow) const
{
	VALIDATE_OBJECT
	return getProductionDifferenceTimes100(getProductionNeeded(), getProductionTimes100(), getProductionModifier(), (!bIgnoreFood && isFoodProduction()), bOverflow);
}

//	--------------------------------------------------------------------------------
// What is the production of this city, not counting modifiers specific to what we happen to be building?
int CvCity::getRawProductionDifferenceTimes100(bool bIgnoreFood, bool bOverflow) const
{
	VALIDATE_OBJECT
	return getProductionDifferenceTimes100(getProductionNeeded(), getProductionTimes100(), getGeneralProductionModifiers(), (!bIgnoreFood && isFoodProduction()), bOverflow);
}


//	--------------------------------------------------------------------------------
int CvCity::getExtraProductionDifference(int iExtra) const
{
	VALIDATE_OBJECT
	return getExtraProductionDifference(iExtra, getProductionModifier());
}

//	--------------------------------------------------------------------------------
int CvCity::getExtraProductionDifference(int iExtra, UnitTypes eUnit) const
{
	VALIDATE_OBJECT
	return getExtraProductionDifference(iExtra, getProductionModifier(eUnit));
}

//	--------------------------------------------------------------------------------
int CvCity::getExtraProductionDifference(int iExtra, BuildingTypes eBuilding) const
{
	VALIDATE_OBJECT
	return getExtraProductionDifference(iExtra, getProductionModifier(eBuilding));
}

//	--------------------------------------------------------------------------------
int CvCity::getExtraProductionDifference(int iExtra, ProjectTypes eProject) const
{
	VALIDATE_OBJECT
	return getExtraProductionDifference(iExtra, getProductionModifier(eProject));
}

//	--------------------------------------------------------------------------------
int CvCity::getExtraProductionDifference(int iExtra, int iModifier) const
{
	VALIDATE_OBJECT
	return ((iExtra * getBaseYieldRateModifier(YIELD_PRODUCTION, iModifier)) / 100);
}

//	--------------------------------------------------------------------------------
/// Convert extra food to production if building a unit built partially from food
int CvCity::GetFoodProduction(int iExcessFood) const
{
	int iRtnValue;

	if(iExcessFood <= 0)
	{
		iRtnValue = 0;
	}
	else if(iExcessFood <= 2)
	{
		iRtnValue = iExcessFood * 100;
	}
	else if(iExcessFood > 2 && iExcessFood <= 4)
	{
		iRtnValue = 200 + (iExcessFood - 2) * 50;
	}
	else
	{
		iRtnValue = 300 + (iExcessFood - 4) * 25;
	}

	return (iRtnValue / 100);
}

//	--------------------------------------------------------------------------------
bool CvCity::canHurry(HurryTypes eHurry, bool bTestVisible) const
{
	VALIDATE_OBJECT
	if(!(GET_PLAYER(getOwner()).IsHasAccessToHurry(eHurry)))
	{
		return false;
	}

	if(getProduction() >= getProductionNeeded())
	{
		return false;
	}

	CvHurryInfo* pkHurryInfo = GC.getHurryInfo(eHurry);
	if(pkHurryInfo == NULL)
		return false;


	// City cannot Hurry Player-level things
	if(pkHurryInfo->getGoldPerBeaker() > 0 || pkHurryInfo->getGoldPerCulture() > 0)
	{
		return false;
	}

	if(!bTestVisible)
	{
		if(!isProductionUnit() && !isProductionBuilding())
		{
			return false;
		}

		if(GET_PLAYER(getOwner()).GetTreasury()->GetGold() < hurryGold(eHurry))
		{
			return false;
		}

		if(maxHurryPopulation() < hurryPopulation(eHurry))
		{
			return false;
		}
	}

	return true;
}

//	--------------------------------------------------------------------------------
bool CvCity::canHurryUnit(HurryTypes eHurry, UnitTypes eUnit, bool bIgnoreNew) const
{
	VALIDATE_OBJECT
	if(!(GET_PLAYER(getOwner()).IsHasAccessToHurry(eHurry)))
	{
		return false;
	}

	if(getUnitProduction(eUnit) >= getProductionNeeded(eUnit))
	{
		return false;
	}

	if(GET_PLAYER(getOwner()).GetTreasury()->GetGold() < getHurryGold(eHurry, getHurryCost(eHurry, false, eUnit, bIgnoreNew), getProductionNeeded(eUnit)))
	{
		return false;
	}

	if(maxHurryPopulation() < getHurryPopulation(eHurry, getHurryCost(eHurry, true, eUnit, bIgnoreNew)))
	{
		return false;
	}

	return true;
}

//	--------------------------------------------------------------------------------
bool CvCity::canHurryBuilding(HurryTypes eHurry, BuildingTypes eBuilding, bool bIgnoreNew) const
{
	VALIDATE_OBJECT
	if(!(GET_PLAYER(getOwner()).IsHasAccessToHurry(eHurry)))
	{
		return false;
	}

	if(m_pCityBuildings->GetBuildingProduction(eBuilding) >= getProductionNeeded(eBuilding))
	{
		return false;
	}

	if(GET_PLAYER(getOwner()).GetTreasury()->GetGold() < getHurryGold(eHurry, getHurryCost(eHurry, false, eBuilding, bIgnoreNew), getProductionNeeded(eBuilding)))
	{
		return false;
	}

	if(maxHurryPopulation() < getHurryPopulation(eHurry, getHurryCost(eHurry, true, eBuilding, bIgnoreNew)))
	{
		return false;
	}

	return true;
}


//	--------------------------------------------------------------------------------
void CvCity::hurry(HurryTypes eHurry)
{
	VALIDATE_OBJECT
	int iHurryGold;
	int iHurryPopulation;

	if(!canHurry(eHurry))
	{
		return;
	}

	iHurryGold = hurryGold(eHurry);
	iHurryPopulation = hurryPopulation(eHurry);

	changeProduction(hurryProduction(eHurry));

	GET_PLAYER(getOwner()).GetTreasury()->ChangeGold(-(iHurryGold));
	changePopulation(-(iHurryPopulation));

	if((getOwner() == GC.getGame().getActivePlayer()) && isCitySelected())
	{
		DLLUI->setDirty(SelectionButtons_DIRTY_BIT, true);
	}
}


//	--------------------------------------------------------------------------------
UnitTypes CvCity::getConscriptUnit() const
{
	VALIDATE_OBJECT
	UnitTypes eBestUnit = NO_UNIT;
	int iBestValue = 0;

	for(int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		const UnitClassTypes eUnitClass = static_cast<UnitClassTypes>(iI);
		CvUnitClassInfo* pkUnitClassInfo = GC.getUnitClassInfo(eUnitClass);
		if(pkUnitClassInfo)
		{
			const UnitTypes eLoopUnit = (UnitTypes)getCivilizationInfo().getCivilizationUnits(iI);
			if(eLoopUnit != NO_UNIT)
			{
				CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eLoopUnit);
				if(pkUnitInfo)
				{
					if(canTrain(eLoopUnit))
					{
						const int iValue = pkUnitInfo->GetConscriptionValue();
						if(iValue > iBestValue)
						{
							iBestValue = iValue;
							eBestUnit = eLoopUnit;
						}
					}
				}
			}
		}
	}

	return eBestUnit;
}


//	--------------------------------------------------------------------------------
int CvCity::getConscriptPopulation() const
{
	VALIDATE_OBJECT
	UnitTypes eConscriptUnit = getConscriptUnit();

	if(eConscriptUnit == NO_UNIT)
	{
		return 0;
	}

	CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eConscriptUnit);
	if(pkUnitInfo == NULL)
	{
		return 0;
	}

	int iConscriptPopulationPerCost = GC.getCONSCRIPT_POPULATION_PER_COST();
	if(iConscriptPopulationPerCost == 0)
	{
		return 0;
	}

	return std::max(1, ((pkUnitInfo->GetProductionCost()) / iConscriptPopulationPerCost));
}


//	--------------------------------------------------------------------------------
int CvCity::conscriptMinCityPopulation() const
{
	VALIDATE_OBJECT
	int iPopulation = GC.getCONSCRIPT_MIN_CITY_POPULATION();

	iPopulation += getConscriptPopulation();

	return iPopulation;
}


//	--------------------------------------------------------------------------------
bool CvCity::canConscript() const
{
	VALIDATE_OBJECT
	if(isDrafted())
	{
		return false;
	}

	if(GET_PLAYER(getOwner()).getConscriptCount() >= GET_PLAYER(getOwner()).getMaxConscript())
	{
		return false;
	}

	if(getPopulation() <= getConscriptPopulation())
	{
		return false;
	}

	if(getPopulation() < conscriptMinCityPopulation())
	{
		return false;
	}

	if(getConscriptUnit() == NO_UNIT)
	{
		return false;
	}

	return true;
}

//	--------------------------------------------------------------------------------
CvUnit* CvCity::initConscriptedUnit()
{
	VALIDATE_OBJECT
	UnitAITypes eCityAI = NO_UNITAI;
	UnitTypes eConscriptUnit = getConscriptUnit();

	if(NO_UNIT == eConscriptUnit)
	{
		return NULL;
	}

	CvUnit* pUnit = GET_PLAYER(getOwner()).initUnit(eConscriptUnit, getX(), getY(), eCityAI);
	CvAssertMsg(pUnit != NULL, "pUnit expected to be assigned (not NULL)");

	if(NULL != pUnit)
	{
		addProductionExperience(pUnit, true);

		pUnit->setMoves(pUnit->maxMoves());
	}

	return pUnit;
}


//	--------------------------------------------------------------------------------
void CvCity::conscript()
{
	VALIDATE_OBJECT
	if(!canConscript())
	{
		return;
	}

	changePopulation(-(getConscriptPopulation()));

	setDrafted(true);

	GET_PLAYER(getOwner()).changeConscriptCount(1);

	CvUnit* pUnit = initConscriptedUnit();
	CvAssertMsg(pUnit != NULL, "pUnit expected to be assigned (not NULL)");

	if(NULL != pUnit)
	{
		if(GC.getGame().getActivePlayer() == getOwner())
		{
			auto_ptr<ICvUnit1> pDllUnit = GC.WrapUnitPointer(pUnit);
			DLLUI->selectUnit(pDllUnit.get(), true, false, true);
		}
	}
}

//	--------------------------------------------------------------------------------
int CvCity::getYieldModifierFromFeature(FeatureTypes eIndex1, YieldTypes eIndex2) const
{
	CvAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex1 < GC.getNumFeatureInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	return m_ppiYieldModifierFromFeature[eIndex1][eIndex2];
}
void CvCity::changeYieldModifierFromFeature(FeatureTypes eIndex1, YieldTypes eIndex2, int iChange)
{
	if (iChange == 0) return;
	CvAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex1 < GC.getNumFeatureInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");

	
	Firaxis::Array<int, NUM_YIELD_TYPES> yields = m_ppiYieldModifierFromFeature[eIndex1];
	yields[eIndex2] = (m_ppiYieldModifierFromFeature[eIndex1][eIndex2] + iChange);
	m_ppiYieldModifierFromFeature[eIndex1] = yields;

	changeFeatureYieldRateModifier(eIndex2, iChange * GetNumFeatureWorked(eIndex1));
	UpdateCityYields(eIndex2);
}


//	--------------------------------------------------------------------------------
int CvCity::getYieldModifierFromTerrain(TerrainTypes eIndex1, YieldTypes eIndex2) const
{
	CvAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex1 < GC.getNumTerrainInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	return m_ppiYieldModifierFromTerrain[eIndex1][eIndex2];
}
void CvCity::changeYieldModifierFromTerrain(TerrainTypes eIndex1, YieldTypes eIndex2, int iChange)
{
	if (iChange == 0) return;
	CvAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex1 < GC.getNumTerrainInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");


	Firaxis::Array<int, NUM_YIELD_TYPES> yields = m_ppiYieldModifierFromTerrain[eIndex1];
	yields[eIndex2] = (m_ppiYieldModifierFromTerrain[eIndex1][eIndex2] + iChange);
	m_ppiYieldModifierFromTerrain[eIndex1] = yields;

	changeTerrainYieldRateModifier(eIndex2, iChange * GetNumTerrainWorked(eIndex1));
	UpdateCityYields(eIndex2);
}

//	--------------------------------------------------------------------------------
int CvCity::getYieldModifierFromImprovement(ImprovementTypes eIndex1, YieldTypes eIndex2) const
{
	CvAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex1 < GC.getNumImprovementInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	return m_ppiYieldModifierFromImprovement[eIndex1][eIndex2];
}
void CvCity::changeYieldModifierFromImprovement(ImprovementTypes eIndex1, YieldTypes eIndex2, int iChange)
{
	if (iChange == 0) return;
	CvAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex1 < GC.getNumImprovementInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");


	Firaxis::Array<int, NUM_YIELD_TYPES> yields = m_ppiYieldModifierFromImprovement[eIndex1];
	yields[eIndex2] = (m_ppiYieldModifierFromImprovement[eIndex1][eIndex2] + iChange);
	m_ppiYieldModifierFromImprovement[eIndex1] = yields;
	
	changeImprovementYieldRateModifier(eIndex2, iChange * GetNumImprovementWorked(eIndex1));
	UpdateCityYields(eIndex2);
}



//	--------------------------------------------------------------------------------
int CvCity::getYieldModifierFromSpecialist(SpecialistTypes eIndex1, YieldTypes eIndex2) const
{
	CvAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex1 < GC.getNumSpecialistInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	return m_ppiYieldModifierFromSpecialist[eIndex1][eIndex2];
}
void CvCity::changeYieldModifierFromSpecialist(SpecialistTypes eIndex1, YieldTypes eIndex2, int iChange)
{
	if (iChange == 0) return;
	CvAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex1 < GC.getNumSpecialistInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");


	Firaxis::Array<int, NUM_YIELD_TYPES> yields = m_ppiYieldModifierFromSpecialist[eIndex1];
	yields[eIndex2] = (m_ppiYieldModifierFromSpecialist[eIndex1][eIndex2] + iChange);
	m_ppiYieldModifierFromSpecialist[eIndex1] = yields;

	changeSpecialistYieldRateModifier(eIndex2, iChange * GetCityCitizens()->GetSpecialistCount(eIndex1));
	UpdateCityYields(eIndex2);
}


//	--------------------------------------------------------------------------------
int CvCity::getYieldModifierFromResource(ResourceTypes eIndex1, YieldTypes eIndex2) const
{
	CvAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex1 < GC.getNumResourceInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	return m_ppiYieldModifierFromResource[eIndex1][eIndex2];
}
void CvCity::changeYieldModifierFromResource(ResourceTypes eIndex1, YieldTypes eIndex2, int iChange)
{
	if (iChange == 0) return;
	CvAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex1 < GC.getNumResourceInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");

	
	Firaxis::Array<int, NUM_YIELD_TYPES> yields = m_ppiYieldModifierFromResource[eIndex1];
	yields[eIndex2] = (m_ppiYieldModifierFromResource[eIndex1][eIndex2] + iChange);
	m_ppiYieldModifierFromResource[eIndex1] = yields;
	
	changeResourceYieldRateModifier(eIndex2, iChange * GetNumResourceLocal(eIndex1));
	UpdateCityYields(eIndex2);
}

//	--------------------------------------------------------------------------------
void CvCity::processFeature(FeatureTypes eFeature, int iChange)
{
	VALIDATE_OBJECT

	// Yield modifier for having a local Improvement
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		const YieldTypes eYield = static_cast<YieldTypes>(iI);
		changeFeatureYieldRateModifier(eYield, (getYieldModifierFromFeature(eFeature, eYield) * iChange));
		// will update yields at the end of CvCityCitizens::SetWorkingPlot
	}
}

void CvCity::processTerrain(TerrainTypes eTerrain, int iChange)
{
	VALIDATE_OBJECT

	// Yield modifier for having a local Improvement
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		const YieldTypes eYield = static_cast<YieldTypes>(iI);
		changeTerrainYieldRateModifier(eYield, (getYieldModifierFromTerrain(eTerrain, eYield) * iChange));
	}
}


//	--------------------------------------------------------------------------------
void CvCity::processImprovement(ImprovementTypes eImprovement, int iChange)
{
	VALIDATE_OBJECT

	// Yield modifier for having a local Improvement
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		const YieldTypes eYield = static_cast<YieldTypes>(iI);
		changeImprovementYieldRateModifier(eYield, (getYieldModifierFromImprovement(eImprovement, eYield) * iChange));
	}
}


//	--------------------------------------------------------------------------------
void CvCity::processResource(ResourceTypes eResource, int iChange)
{
	VALIDATE_OBJECT

	// Yield modifier for having a local resource
	for(int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		const YieldTypes eYield = static_cast<YieldTypes>(iI);
		changeResourceYieldRateModifier(eYield, (getYieldModifierFromResource(eResource, eYield) * iChange));
		UpdateCityYields(eYield);
	}
}


//	--------------------------------------------------------------------------------
void CvCity::processBuilding(BuildingTypes eBuilding, int iChange, bool bFirst, bool bObsolete, bool /*bApplyingAllCitiesBonus*/)
{
	VALIDATE_OBJECT

	CvBuildingEntry* pBuildingInfo = GC.getBuildingInfo(eBuilding);
	
	if(pBuildingInfo == NULL)
		return;

	BuildingClassTypes eBuildingClass = (BuildingClassTypes) pBuildingInfo->GetBuildingClassType();

	CvPlayer& owningPlayer = GET_PLAYER(getOwner());
	CvTeam& owningTeam = GET_TEAM(getTeam());
	CvCivilizationInfo& thisCiv = getCivilizationInfo();

	if(!(owningTeam.isObsoleteBuilding(eBuilding)) || bObsolete)
	{
		// One-shot items
		if(bFirst && iChange > 0)
		{

#if defined(MOD_ROG_CORE)
			if (pBuildingInfo->GetPopulationChange() != 0)
			{
				setPopulation(std::max(1, (getPopulation() + iChange * GC.getBuildingInfo(eBuilding)->GetPopulationChange())));
			}
			// Instant Friendship change with all Minors
			int iMinorFriendshipChange = pBuildingInfo->GetMinorCivFriendship();
			if (iMinorFriendshipChange != 0)
			{
				for (int iMinorLoop = MAX_MAJOR_CIVS; iMinorLoop < MAX_CIV_PLAYERS; iMinorLoop++)
				{
					TeamTypes eTeam = GET_PLAYER((PlayerTypes)iMinorLoop).getTeam();
					if (getTeam() != eTeam && GET_TEAM(eTeam).isHasMet(getTeam()))
					{
						GET_PLAYER((PlayerTypes)iMinorLoop).GetMinorCivAI()->ChangeFriendshipWithMajor(getOwner(), iMinorFriendshipChange);
					}
				}
			}


			if (pBuildingInfo->IsForbiddenForeignSpyGlobal())
			{
				int iLoopCity = 0;
				for (CvCity* pLoopCity = owningPlayer.firstCity(&iLoopCity); pLoopCity != NULL; pLoopCity = owningPlayer.nextCity(&iLoopCity))
				{
					// Spies in the city? YOU'RE OUTTA HERE!~
					CvCityEspionage* pCityEspionage = pLoopCity->GetCityEspionage();
					if (pCityEspionage)
					{
						for (int iPlayerLoop = 0; iPlayerLoop < MAX_MAJOR_CIVS; iPlayerLoop++)
						{
							PlayerTypes eLoopPlayer = (PlayerTypes)iPlayerLoop;

							if (!GET_PLAYER(eLoopPlayer).isAlive())
								continue;

							if (eLoopPlayer == getOwner())
								continue;

							CvPlayerEspionage* pEspionage = GET_PLAYER(eLoopPlayer).GetEspionage();
							if (!pEspionage)
								continue;

							int iAssignedSpy = pCityEspionage->m_aiSpyAssignment[iPlayerLoop];
							if (iAssignedSpy == -1)
								continue;

							// There's a spy! Remove it!
							GET_PLAYER(eLoopPlayer).GetEspionage()->ExtractSpyFromCity(iAssignedSpy);

							// Notify the spy's owner
							CvNotifications* pNotify = GET_PLAYER(eLoopPlayer).GetNotifications();
							if (!pNotify)
								continue;

							CvEspionageSpy* pSpy = pEspionage->GetSpyByID(iAssignedSpy);
							Localization::String strSummary(GetLocalizedText("TXT_KEY_NOTIFICATION_SPY_EVICTED_TRADE_S"));

							// spy owner gets a different notification
							if (eLoopPlayer != getOwner() && GET_PLAYER(eLoopPlayer).isHuman())
							{
								Localization::String strNotification = Localization::Lookup("TXT_KEY_NOTIFICATION_SPY_EVICTED_FPRBIDDEN_YOU");
								strNotification << pEspionage->GetSpyRankName(pSpy->m_eRank);
								strNotification << pSpy->GetSpyName(&GET_PLAYER(eLoopPlayer));
								strNotification << getNameKey();
								pNotify->Add(NOTIFICATION_SPY_EVICTED, strNotification.toUTF8(), strSummary.toUTF8(), -1, -1, eLoopPlayer);
							}
						}
					}
				}
			}


			if (pBuildingInfo->IsForbiddenForeignSpy())
			{
				// Spies in the city? YOU'RE OUTTA HERE!~
				CvCityEspionage* pCityEspionage = GetCityEspionage();
				if (pCityEspionage)
				{
					for (int iPlayerLoop = 0; iPlayerLoop < MAX_MAJOR_CIVS; iPlayerLoop++)
					{
						PlayerTypes eLoopPlayer = (PlayerTypes)iPlayerLoop;

						if (!GET_PLAYER(eLoopPlayer).isAlive())
							continue;

						if (eLoopPlayer == getOwner())
							continue;

						CvPlayerEspionage* pEspionage = GET_PLAYER(eLoopPlayer).GetEspionage();
						if (!pEspionage)
							continue;

						int iAssignedSpy = pCityEspionage->m_aiSpyAssignment[iPlayerLoop];
						if (iAssignedSpy == -1)
							continue;

						// There's a spy! Remove it!
						GET_PLAYER(eLoopPlayer).GetEspionage()->ExtractSpyFromCity(iAssignedSpy);	


						// Notify the spy's owner
						CvNotifications* pNotify = GET_PLAYER(eLoopPlayer).GetNotifications();
						if (!pNotify)
							continue;

						CvEspionageSpy* pSpy = pEspionage->GetSpyByID(iAssignedSpy);
						Localization::String strSummary( GetLocalizedText("TXT_KEY_NOTIFICATION_SPY_EVICTED_TRADE_S"));

						// spy owner gets a different notification
						if (eLoopPlayer != getOwner() && GET_PLAYER(eLoopPlayer).isHuman())
						{
							Localization::String strNotification =Localization::Lookup("TXT_KEY_NOTIFICATION_SPY_EVICTED_FPRBIDDEN_YOU");
							strNotification << pEspionage->GetSpyRankName(pSpy->m_eRank);
							strNotification << pSpy->GetSpyName(&GET_PLAYER(eLoopPlayer));
							strNotification << getNameKey();
							pNotify->Add(NOTIFICATION_SPY_EVICTED, strNotification.toUTF8(), strSummary.toUTF8(), -1, -1, eLoopPlayer);
						}
					}
				}
			}


#endif
			// Capital
			if(pBuildingInfo->IsCapital())
				owningPlayer.setCapitalCity(this);

	
			// Free Units
			CvUnit* pFreeUnit;
			int iFreeUnitLoop;

			int iNumFreeUnitTotal = pBuildingInfo->GetNumFreeUnitTotal();
			int iNumFreeUnitToCiv = pBuildingInfo->GetNumFreeUnit();
			if(iNumFreeUnitTotal > 0)
			{
				std::pair<UnitTypes, int>* pFreeUnits = pBuildingInfo->GetFreeUnits();
				for(int iUnitLoop = 0; iUnitLoop < iNumFreeUnitTotal; iUnitLoop++)
				{
					UnitTypes eUnit = pFreeUnits[iUnitLoop].first;
					int iUnitNum = pFreeUnits[iUnitLoop].second;
					bool bToCivType = iUnitLoop<iNumFreeUnitToCiv;
					initFreeUnit(owningPlayer, eUnit, iUnitNum, bToCivType);
				}

			}

			// Free building
			BuildingClassTypes eFreeBuildingClassThisCity = (BuildingClassTypes)pBuildingInfo->GetFreeBuildingThisCity();
			if(eFreeBuildingClassThisCity != NO_BUILDINGCLASS)
			{
				BuildingTypes eFreeBuildingThisCity = owningPlayer.GetCivBuilding(eFreeBuildingClassThisCity);

				if (eFreeBuildingThisCity != NO_BUILDING)
				{
					m_pCityBuildings->SetNumRealBuilding(eFreeBuildingThisCity, 0);
					m_pCityBuildings->SetNumFreeBuilding(eFreeBuildingThisCity, 1);
				}
			}

			// Free Great Work
			GreatWorkType eGWType = pBuildingInfo->GetFreeGreatWork();
			if (eGWType != NO_GREAT_WORK)
			{
				GreatWorkClass eClass = CultureHelpers::GetGreatWorkClass(eGWType);
				int iGWindex = 	GC.getGame().GetGameCulture()->CreateGreatWork(eGWType, eClass, m_eOwner, owningPlayer.GetCurrentEra(), pBuildingInfo->GetDescription());
				m_pCityBuildings->SetBuildingGreatWork(eBuildingClass, 0, iGWindex);
			}

			// Tech boost for science buildings in capital
			if(owningPlayer.GetPlayerTraits()->IsTechBoostFromCapitalScienceBuildings())
			{
				if(isCapital())
				{
					if(pBuildingInfo->IsScienceBuilding())
					{
						int iMedianTechResearch = owningPlayer.GetPlayerTechs()->GetMedianTechResearch();
						iMedianTechResearch = (iMedianTechResearch * owningPlayer.GetMedianTechPercentage()) / 100;

						TechTypes eCurrentTech = owningPlayer.GetPlayerTechs()->GetCurrentResearch();
						if(eCurrentTech == NO_TECH)
						{
							owningPlayer.changeOverflowResearch(iMedianTechResearch);
						}
						else
						{
							owningTeam.GetTeamTechs()->ChangeResearchProgress(eCurrentTech, iMedianTechResearch, owningPlayer.GetID());
						}
					}
				}
			}

			// TERRA COTTA AWESOME
			if (pBuildingInfo->GetInstantMilitaryIncrease() > 0)
			{
				std::vector<UnitTypes> aExtraUnits;
				std::vector<UnitAITypes> aExtraUnitAITypes;
				CvUnit* pLoopUnit = NULL;
				int iLoop = 0;
				for(pLoopUnit = GET_PLAYER(m_eOwner).firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = GET_PLAYER(m_eOwner).nextUnit(&iLoop))
				{
					if (pLoopUnit->getDomainType() == DOMAIN_LAND && pLoopUnit->IsCombatUnit())
					{
						UnitTypes eCurrentUnitType = pLoopUnit->getUnitType();
						UnitAITypes eCurrentUnitAIType = pLoopUnit->AI_getUnitAIType();

						// check for duplicate unit
						bool bAddUnit = true;
						for (uint ui = 0; ui < aExtraUnits.size(); ui++)
						{
							if (aExtraUnits[ui] == eCurrentUnitType)
							{
								bAddUnit = false;
							}
						}

						if (bAddUnit)
						{
							aExtraUnits.push_back(eCurrentUnitType);
							aExtraUnitAITypes.push_back(eCurrentUnitAIType);
						}
					}
				}

				for (uint ui = 0; ui < aExtraUnits.size(); ui++)
				{
					CvUnit* pNewUnit = GET_PLAYER(m_eOwner).initUnit(aExtraUnits[ui], m_iX, m_iY, aExtraUnitAITypes[ui]);
					bool bJumpSuccess = pNewUnit->jumpToNearestValidPlot();
					if (!bJumpSuccess)
					{
						bool bUnitImmobile = pNewUnit->IsImmobile();
						pNewUnit->kill(false);
						// if this Unit is Immobile, it will Jump fault, but we should not stop loop
						if (!bUnitImmobile) break;
					}
				}
			}
			// END TERRA COTTA AWESOME
		}

		if(pBuildingInfo->GetTrainedFreePromotion() != NO_PROMOTION)
		{
			changeFreePromotionCount(((PromotionTypes)(pBuildingInfo->GetTrainedFreePromotion())), iChange);
		}

		changeTradeRouteDomainRangeModifier(DOMAIN_LAND, pBuildingInfo->GetTradeRouteLandDistanceModifier() * iChange);
		changeTradeRouteDomainRangeModifier(DOMAIN_SEA, pBuildingInfo->GetTradeRouteSeaDistanceModifier() * iChange);

		changeTradeRouteDomainGoldBonus(DOMAIN_LAND, pBuildingInfo->GetTradeRouteLandGoldBonus() * iChange);
		changeTradeRouteDomainGoldBonus(DOMAIN_SEA, pBuildingInfo->GetTradeRouteSeaGoldBonus() * iChange);

#if defined(MOD_GLOBAL_BUILDING_INSTANT_YIELD)
		if (iChange > 0 && pBuildingInfo->IsAllowInstantYield())
		{
			doBuildingInstantYield(pBuildingInfo->GetInstantYieldArray());
		}
#endif


		changeGreatPeopleRateModifier(pBuildingInfo->GetGreatPeopleRateModifier() * iChange);
		changeFreeExperience(pBuildingInfo->GetFreeExperience() * iChange);
		ChangeNumCanAirlift(pBuildingInfo->IsAirlift() ? iChange : 0);
		ChangeMaxAirUnits(pBuildingInfo->GetAirModifier() * iChange);
		changeNukeModifier(pBuildingInfo->GetNukeModifier() * iChange);
		changeHealRate(pBuildingInfo->GetHealRateChange() * iChange);
		ChangeExtraHitPoints(pBuildingInfo->GetExtraCityHitPoints() * iChange);

		ChangeNoOccupiedUnhappinessCount(pBuildingInfo->IsNoOccupiedUnhappiness() * iChange);

#if defined(MOD_ROG_CORE)
		changeExtraDamageHealPercent(pBuildingInfo->GetExtraDamageHealPercent()* iChange);
		changeExtraDamageHeal(pBuildingInfo->GetExtraDamageHeal()* iChange);
		changeExtraBombardRange(pBuildingInfo->GetBombardRange()* iChange);
		changeBombardIndirect(pBuildingInfo->IsBombardIndirect()* iChange);

		changeCityBuildingRangeStrikeModifier(pBuildingInfo->CityRangedStrikeModifier()* iChange);

		changeResetDamageValue(pBuildingInfo->GetResetDamageValue()* iChange);
		changeReduceDamageValue(pBuildingInfo->GetReduceDamageValue()* iChange);

		changeWaterTileDamage(pBuildingInfo->GetWaterTileDamage()* iChange);
		changeWaterTileMovementReduce(pBuildingInfo->GetWaterTileMovementReduce()* iChange);
		changeWaterTileTurnDamage(pBuildingInfo->GetWaterTileTurnDamage()* iChange);
		changeLandTileDamage(pBuildingInfo->GetLandTileDamage()* iChange);
		changeLandTileMovementReduce(pBuildingInfo->GetLandTileMovementReduce()* iChange);
		changeLandTileTurnDamage(pBuildingInfo->GetLandTileTurnDamage()* iChange);
#endif
#if defined(MOD_VOLCANO_BREAK)
		changeImmueVolcanoDamage(pBuildingInfo->IsImmueVolcanoDamage()* iChange);  
#endif
		changeAddsFreshWater(pBuildingInfo->IsAddsFreshWater()* iChange);
		changeExtraAttackOnKill((pBuildingInfo->IsExtraAttackOnKill()) ? iChange : 0);
		changeForbiddenForeignSpyCount(pBuildingInfo->IsForbiddenForeignSpy()* iChange);
		changeExtraAttacks(pBuildingInfo->GetExtraAttacks()* iChange);
		ChangeAllowSpaceshipLaunchCount((pBuildingInfo->IsAllowSpaceshipLaunch()) ? iChange : 0);

#if !defined(MOD_API_EXTENSIONS)
		// Trust the modder if they set a building to negative happiness
		if(pBuildingInfo->GetHappiness() > 0)
#endif
		{
			ChangeBaseHappinessFromBuildings(pBuildingInfo->GetHappiness() * iChange);
		}

#if !defined(MOD_API_EXTENSIONS)
		// Trust the modder if they set a building to negative global happiness
		if(pBuildingInfo->GetUnmoddedHappiness() > 0)
#endif
		{
			ChangeUnmoddedHappinessFromBuildings(pBuildingInfo->GetUnmoddedHappiness() * iChange);
		}

		if(pBuildingInfo->GetUnhappinessModifier() != 0)
		{
			owningPlayer.ChangeUnhappinessMod(pBuildingInfo->GetUnhappinessModifier() * iChange);
		}



		//ChangeBaseYieldRateFromBuildings(YIELD_CULTURE, iBuildingCulture* iChange);

		changeCultureRateModifier(pBuildingInfo->GetCultureRateModifier() * iChange);
		changePlotCultureCostModifier(pBuildingInfo->GetPlotCultureCostModifier() * iChange);
		changePlotBuyCostModifier(pBuildingInfo->GetPlotBuyCostModifier() * iChange);
		ChangeUnitMaxExperienceLocal(pBuildingInfo->GetUnitMaxExperienceLocal() * iChange);
		ChangeTradeRouteRiverBonusModifier(pBuildingInfo->GetTradeRouteRiverBonusModifier() * iChange);
		ChangeSecondCapitalsExtraScore(pBuildingInfo->GetSecondCapitalsExtraScore() * iChange);
		ChangeFoodKeptFromPollution(pBuildingInfo->GetFoodKeptFromPollution() * iChange);
		ChangeNumAllowsFoodTradeRoutes(pBuildingInfo->AllowsFoodTradeRoutes() ? iChange : 0);
		ChangeNumAllowsProductionTradeRoutes(pBuildingInfo->AllowsProductionTradeRoutes() ? iChange : 0);
#if defined(MOD_BUILDINGS_CITY_WORKING)
		changeCityWorkingChange(pBuildingInfo->GetCityWorkingChange() * iChange);
#endif

#if defined(MOD_BUILDINGS_CITY_AUTOMATON_WORKERS)
		changeCityAutomatonWorkersChange(pBuildingInfo->GetCityAutomatonWorkersChange() * iChange);
#endif


		m_pCityReligions->ChangeReligiousPressureModifier(pBuildingInfo->GetReligiousPressureModifier() * iChange);

		changeMaxFoodKeptPercent(pBuildingInfo->GetFoodKept() * iChange);
		changeMilitaryProductionModifier(pBuildingInfo->GetMilitaryProductionModifier() * iChange);
		changeSpaceProductionModifier(pBuildingInfo->GetSpaceProductionModifier() * iChange);
		m_pCityBuildings->ChangeBuildingProductionModifier(pBuildingInfo->GetBuildingProductionModifier() * iChange);
		m_pCityBuildings->ChangeMissionaryExtraSpreads(pBuildingInfo->GetExtraMissionarySpreads() * iChange);
		m_pCityBuildings->ChangeLandmarksTourismPercent(pBuildingInfo->GetLandmarksTourismPercent() * iChange);
		m_pCityBuildings->ChangeGreatWorksTourismModifier(pBuildingInfo->GetGreatWorksTourismModifier() * iChange);
		m_pCityBuildings->ChangeNumBuildingsFromFaith((pBuildingInfo->GetFaithCost() > 0 && pBuildingInfo->IsUnlockedByBelief() && pBuildingInfo->GetProductionCost() == -1) ? iChange : 0);
		ChangeWonderProductionModifier(pBuildingInfo->GetWonderProductionModifier() * iChange);
		changeCapturePlunderModifier(pBuildingInfo->GetCapturePlunderModifier() * iChange);
		ChangeEspionageModifier(pBuildingInfo->GetEspionageModifier() * iChange);
#if defined(MOD_RELIGION_CONVERSION_MODIFIERS)
		ChangeConversionModifier(pBuildingInfo->GetConversionModifier() * iChange);
		owningPlayer.ChangeConversionModifier(pBuildingInfo->GetGlobalConversionModifier() * iChange);
#endif

		changeNukeInterceptionChance(pBuildingInfo->GetNukeInterceptionChance()* iChange);

		ChangeTradeRouteTargetBonus(pBuildingInfo->GetTradeRouteTargetBonus() * iChange);
		ChangeTradeRouteRecipientBonus(pBuildingInfo->GetTradeRouteRecipientBonus() * iChange);
		

		for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
		{
			SpecialistTypes eSpecialist = (SpecialistTypes)iI;
			CvSpecialistInfo* pSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
			if (pSpecialistInfo)
			{
				int iValue = pBuildingInfo->GetSpecificGreatPersonRateModifier(eSpecialist);
				if (iValue > 0)
				{
					ChangeSpecialistRateModifier(eSpecialist, (pBuildingInfo->GetSpecificGreatPersonRateModifier(eSpecialist) * iChange));
				}
			}

			for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
			{
				YieldTypes eYield2 = (YieldTypes)iJ;

				changeSpecialistExtraYield(eSpecialist, eYield2, (pBuildingInfo->GetSpecialistYieldChangeLocal(iI, iJ) * iChange));



				int iYieldMod = pBuildingInfo->GetSpecialistYieldModifier(iI, iJ);
				changeYieldModifierFromSpecialist(eSpecialist, eYield2,  iYieldMod * iChange);


				int iGlobalValue = pBuildingInfo->GetSpecialistYieldModifierGlobal(eSpecialist, eYield2);
				if (iGlobalValue > 0)
				{
					GET_PLAYER(getOwner()).ChangeYieldModifierFromSpecialistGlobal(eSpecialist, eYield2, iGlobalValue);
					int iLoop = 0;
					for (CvCity* pLoopCity = GET_PLAYER(getOwner()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwner()).nextCity(&iLoop))
					{
						pLoopCity->changeYieldModifierFromSpecialist(eSpecialist, eYield2, iGlobalValue * iChange);
					}
				}

			}
		}


#ifdef MOD_PROMOTION_CITY_DESTROYER
		ChangeSiegeKillCitizensModifier(pBuildingInfo->GetSiegeKillCitizensModifier() * iChange);
#endif

#if defined(MOD_NUCLEAR_WINTER_FOR_SP)
		ChangeNumNoNuclearWinterLocal(pBuildingInfo->IsNoNuclearWinterLocal() ? iChange : 0);
#endif
#if defined(MOD_TROOPS_AND_CROPS_FOR_SP)
        ChangeNumEnableCrops(pBuildingInfo->IsEnableCrops() ? iChange : 0);
		ChangeNumEnableArmee(pBuildingInfo->IsEnableArmee() ? iChange : 0);
#endif
#if defined(MOD_INTERNATIONAL_IMMIGRATION_FOR_SP)
		ChangeNumAllScaleImmigrantIn(pBuildingInfo->CanAllScaleImmigrantIn() ? iChange : 0);
#endif

#ifdef MOD_GLOBAL_CORRUPTION
		ChangeCorruptionScoreChangeFromBuilding(pBuildingInfo->GetCorruptionScoreChange() * iChange);
		ChangeCorruptionLevelChangeFromBuilding(pBuildingInfo->GetCorruptionLevelChange() * iChange);

		if (pBuildingInfo->GetCorruptionScoreChange() * iChange != 0 || 
			pBuildingInfo->GetCorruptionLevelChange() * iChange != 0)
		{
			UpdateCorruption();
		}
#endif

		if (pBuildingInfo->AffectSpiesNow() && iChange > 0)
		{
			for (uint ui = 0; ui < MAX_MAJOR_CIVS; ui++)
			{
				PlayerTypes ePlayer = (PlayerTypes)ui;
				GET_PLAYER(ePlayer).GetEspionage()->UpdateCity(this);
			}
		}

		owningPlayer.ChangeProductionNeededUnitModifier(pBuildingInfo->GetGlobalProductionNeededUnitModifier() * iChange);
		owningPlayer.ChangeProductionNeededBuildingModifier(pBuildingInfo->GetGlobalProductionNeededBuildingModifier() * iChange);
		owningPlayer.ChangeProductionNeededProjectModifier(pBuildingInfo->GetGlobalProductionNeededProjectModifier() * iChange);
		owningPlayer.ChangeProductionNeededUnitMax(pBuildingInfo->GetGlobalProductionNeededUnitMax() * iChange);
		owningPlayer.ChangeProductionNeededBuildingMax(pBuildingInfo->GetGlobalProductionNeededBuildingMax() * iChange);
		owningPlayer.ChangeProductionNeededProjectMax(pBuildingInfo->GetGlobalProductionNeededProjectMax() * iChange);

#if defined(MOD_BUILDING_IMPROVEMENT_RESOURCES)
		if (MOD_BUILDING_IMPROVEMENT_RESOURCES)
		{
			std::multimap<int, std::pair<int, int>> piiResourceFromImprovement = pBuildingInfo->GetResourceFromImprovementArray();
			if (piiResourceFromImprovement.empty() == false)
			{
				for (std::multimap<int, std::pair<int, int>>::const_iterator it = piiResourceFromImprovement.begin(); it != piiResourceFromImprovement.end(); ++it)
				{
					if (it->first >= 0 && it->first < GC.getNumResourceInfos() && it->second.first >= 0 && it->second.first < GC.getNumImprovementInfos())
					{
						int OldNum = GetResourceFromImprovement((ResourceTypes)it->first, (ImprovementTypes)it->second.first);

						ChangeResourceFromImprovement((ResourceTypes)it->first, (ImprovementTypes)it->second.first, it->second.second * iChange);

						int NewNum = GetResourceFromImprovement((ResourceTypes)it->first, (ImprovementTypes)it->second.first);

						int iNumResource = (NewNum - OldNum ) * CountUnPillagedImprovement((ImprovementTypes)it->second.first);

						if (iNumResource != 0)
						{
							owningPlayer.changeNumResourceTotal((ResourceTypes)it->first, iNumResource);
						}
					}
				}
			}
		}
#endif

		// Resource loop
		int iCulture, iFaith;
		ResourceTypes eResource;
		for(int iResourceLoop = 0; iResourceLoop < GC.getNumResourceInfos(); iResourceLoop++)
		{
			eResource = (ResourceTypes) iResourceLoop;



			// Does this building add resources?
			int iNumResource = pBuildingInfo->GetResourceQuantity(iResourceLoop) * iChange;
			if(iNumResource != 0)
			{
				owningPlayer.changeNumResourceTotal(eResource, iNumResource);
			}


			if ((pBuildingInfo->GetResourceQuantityFromPOP(iResourceLoop) > 0))
			{
				ChangeResourceQuantityFromPOP(eResource, pBuildingInfo->GetResourceQuantityFromPOP(iResourceLoop) * iChange);
			}


			// Do we have this resource local?
			if(IsHasResourceLocal(eResource, /*bTestVisible*/ false))
			{
				// Our Building does give culture with eResource
				iCulture = GC.getBuildingInfo(eBuilding)->GetResourceCultureChange(eResource);

				if(iCulture != 0)
				{
					ChangeBaseYieldRateFromBuildings(YIELD_CULTURE, iCulture * m_paiNumResourcesLocal[eResource]);
				}

				// What about faith?
				iFaith = GC.getBuildingInfo(eBuilding)->GetResourceFaithChange(eResource);

				if(iFaith != 0)
				{
					ChangeBaseYieldRateFromBuildings(YIELD_FAITH, iFaith* m_paiNumResourcesLocal[eResource]);
				}
			}
		}

		if(pBuildingInfo->IsExtraLuxuries())
		{
			CvPlot* pLoopPlot;

			// Subtract off old luxury counts
#if defined(MOD_GLOBAL_CITY_WORKING)
			for(int iJ = 0; iJ < GetNumWorkablePlots(); iJ++)
#else
			for(int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
#endif
			{
				pLoopPlot = plotCity(getX(), getY(), iJ);

				if(pLoopPlot != NULL && pLoopPlot->getOwner() == getOwner())
				{
					ResourceTypes eLoopResource = pLoopPlot->getResourceType();
					if(eLoopResource != NO_RESOURCE && GC.getResourceInfo(eLoopResource)->getResourceUsage() == RESOURCEUSAGE_LUXURY)
					{
						if(owningTeam.GetTeamTechs()->HasTech((TechTypes) GC.getResourceInfo(eLoopResource)->getTechCityTrade()))
						{
							if(pLoopPlot == plot() || (pLoopPlot->getImprovementType() != NO_IMPROVEMENT && GC.getImprovementInfo(pLoopPlot->getImprovementType())->IsImprovementResourceTrade(eLoopResource)))
							{
								if(!pLoopPlot->IsImprovementPillaged())
								{
									owningPlayer.changeNumResourceTotal(pLoopPlot->getResourceType(), -pLoopPlot->getNumResourceForPlayer(getOwner()), /*bIgnoreResourceWarning*/ true);
								}
							}
						}
					}
				}
			}

			ChangeExtraLuxuryResources(iChange);

			// Add in new luxury counts
#if defined(MOD_GLOBAL_CITY_WORKING)
			for(int iJ = 0; iJ < GetNumWorkablePlots(); iJ++)
#else
			for(int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
#endif
			{
				pLoopPlot = plotCity(getX(), getY(), iJ);

				if(pLoopPlot != NULL && pLoopPlot->getOwner() == getOwner())
				{
					ResourceTypes eLoopResource = pLoopPlot->getResourceType();
					if(eLoopResource != NO_RESOURCE && GC.getResourceInfo(eLoopResource)->getResourceUsage() == RESOURCEUSAGE_LUXURY)
					{
						if(owningTeam.GetTeamTechs()->HasTech((TechTypes) GC.getResourceInfo(eLoopResource)->getTechCityTrade()))
						{
							if(pLoopPlot == plot() || (pLoopPlot->getImprovementType() != NO_IMPROVEMENT && GC.getImprovementInfo(pLoopPlot->getImprovementType())->IsImprovementResourceTrade(eLoopResource)))
							{
								if(!pLoopPlot->IsImprovementPillaged())
								{
									owningPlayer.changeNumResourceTotal(pLoopPlot->getResourceType(), pLoopPlot->getNumResourceForPlayer(getOwner()));
								}
							}
						}
					}
				}
			}
		}

#if defined(MOD_API_UNIFIED_YIELDS)
		CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo(eBuildingClass);
#endif


		YieldTypes eYield;

		for(int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			eYield = (YieldTypes) iI;

			changeSeaPlotYield(eYield, (pBuildingInfo->GetSeaPlotYieldChange(eYield) * iChange));
			changeRiverPlotYield(eYield, (pBuildingInfo->GetRiverPlotYieldChange(eYield) * iChange));
			changeLakePlotYield(eYield, (pBuildingInfo->GetLakePlotYieldChange(eYield) * iChange));
			changeSeaResourceYield(eYield, (pBuildingInfo->GetSeaResourceYieldChange(eYield) * iChange));

			int iBuildingCulture = pBuildingInfo->GetYieldChange(YIELD_CULTURE);

			if (iBuildingCulture > 0)
			{
				ChangeBaseYieldRateFromBuildings(YIELD_CULTURE, owningPlayer.GetPlayerTraits()->GetCultureBuildingYieldChange());
				//iBuildingCulture += owningPlayer.GetPlayerTraits()->GetCultureBuildingYieldChange();
			}

			ChangeBaseYieldRateFromBuildings(eYield, ((pBuildingInfo->GetYieldChange(eYield) + m_pCityBuildings->GetBuildingYieldChange(eBuildingClass, eYield)) * iChange));


			ChangeYieldPerPopTimes100(eYield, pBuildingInfo->GetYieldChangePerPop(eYield) * iChange);
			ChangeYieldPerReligionTimes100(eYield, pBuildingInfo->GetYieldChangePerReligion(eYield) * iChange);
			ChangeYieldPerEra(eYield, pBuildingInfo->GetYieldChangePerEra(eYield) * iChange);
			ChangeYieldModifierPerEra(eYield, pBuildingInfo->GetYieldModifierChangePerEra(eYield) * iChange);
			changeYieldRateModifier(eYield, pBuildingInfo->GetYieldModifier(eYield) * iChange);
			changeYieldRateMultiplier(eYield, pBuildingInfo->GetYieldMultiplier(eYield) * iChange);
			ChangeCityStateTradeRouteYieldModifier(eYield, pBuildingInfo->GetCityStateTradeRouteYieldModifier(eYield) * iChange);

			CvPlayerPolicies* pPolicies = GET_PLAYER(getOwner()).GetPlayerPolicies();
			changeYieldRateModifier(eYield, pPolicies->GetBuildingClassYieldModifier(eBuildingClass, eYield) * iChange);
			ChangeBaseYieldRateFromBuildingsPolicies(eYield, pPolicies->GetBuildingClassYieldChange(eBuildingClass, eYield) * iChange);

			if ((pBuildingInfo->GetYieldFromInternal(eYield) > 0))
			{
				ChangeYieldFromInternalTR(eYield, (pBuildingInfo->GetYieldFromInternal(eYield) * iChange));
			}
#if defined(MOD_ROG_CORE)
			ChangeYieldPerAlly(eYield, pBuildingInfo->GetYieldPerAlly(eYield)* iChange);
			ChangeYieldPerFriend(eYield, pBuildingInfo->GetYieldPerFriend(eYield)* iChange);
			ChangeYieldPerEspionageSpy(eYield, pBuildingInfo->GetYieldPerEspionageSpy(eYield)* iChange);

			if ((pBuildingInfo->GetYieldFromUnitProduction(eYield) > 0))
			{
				ChangeYieldFromUnitProduction(eYield, pBuildingInfo->GetYieldFromUnitProduction(eYield) * iChange);
			}

			if ((pBuildingInfo->GetYieldFromConstruction(eYield) > 0))
			{
				ChangeYieldFromConstruction(eYield, pBuildingInfo->GetYieldFromConstruction(eYield) * iChange);
			}

			for (int iK = 0; iK < NUM_YIELD_TYPES; iK++)
			{
				YieldTypes eYield2 = (YieldTypes)iK;

				//protect against modder stupidity!
				if (eYield == eYield2)
					continue;

				int iGlobalConversionYield = pBuildingInfo->GetYieldFromYieldGlobal(eYield, eYield2);
				if (iGlobalConversionYield > 0)
				{
					GET_PLAYER(getOwner()).changeYieldFromYieldGlobal(eYield, eYield2, iGlobalConversionYield);
					int iLoop = 0;
					for (CvCity* pLoopCity = GET_PLAYER(getOwner()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwner()).nextCity(&iLoop))
					{
						pLoopCity->ChangeBuildingYieldFromYield(eYield, eYield2, (iGlobalConversionYield * iChange));
					}
				}
			}

			ChangeTradeRouteFromTheCityYields(eYield, pBuildingInfo->GetTradeRouteFromTheCityYields(eYield) * iChange);
			ChangeTradeRouteFromTheCityYieldsPerEra(eYield, pBuildingInfo->GetTradeRouteFromTheCityYieldsPerEra(eYield) * iChange);

			if ((pBuildingInfo->GetYieldFromBirth(eYield) > 0))
			{
				ChangeYieldFromBirth(eYield, pBuildingInfo->GetYieldFromBirth(eYield) * iChange);
			}
			if ((pBuildingInfo->GetYieldFromBorderGrowth(eYield) > 0))
			{
				ChangeYieldFromBorderGrowth(eYield, pBuildingInfo->GetYieldFromBorderGrowth(eYield) * iChange);
			}

			if ((pBuildingInfo->GetYieldFromPillage(eYield) > 0))
			{
				ChangeYieldFromPillage(eYield, pBuildingInfo->GetYieldFromPillage(eYield) * iChange);
			}

			ChangeYieldPerPopInEmpireTimes100(eYield, pBuildingInfo->GetYieldChangePerPopInEmpire(eYield)* iChange);

			if ((pBuildingInfo->GetYieldFromProcessModifier(eYield) > 0))
			{
				ChangeYieldFromProcessModifier(eYield, (pBuildingInfo->GetYieldFromProcessModifier(eYield) * iChange));
			}

			int iYieldMod = pBuildingInfo->GetBuildingClassYieldModifier(eBuildingClass, eYield);
			if (iYieldMod != 0)
			{
				changeYieldRateModifier(eYield, iYieldMod * iChange);
			}
#endif


#if defined(MOD_API_UNIFIED_YIELDS)
			// ChangeBaseYieldRateFromBuildings(eYield, GET_PLAYER(getOwner()).GetPlayerTraits()->GetBuildingClassYieldChange(eBuildingClass, eYield) * iChange);
			int iTraitBuildingClassBonus = owningPlayer.GetPlayerTraits()->GetBuildingClassYieldChange(eBuildingClass, eYield);
			if(iTraitBuildingClassBonus > 0)
			{
				ChangeBaseYieldRateFromBuildings(eYield, iTraitBuildingClassBonus * iChange);
			}
#endif


#if defined(MOD_ROG_CORE)
			// Building modifiers
			BuildingClassTypes eBuildingClassLocal;
			for (int iJ = 0; iJ < GC.getNumBuildingClassInfos(); iJ++)
			{
				eBuildingClassLocal = (BuildingClassTypes)iJ;

				CvBuildingClassInfo* pkBuildingClassLocalInfo = GC.getBuildingClassInfo(eBuildingClassLocal);
				if (!pkBuildingClassLocalInfo)
				{
					continue;
				}

				BuildingTypes eLocalBuilding = (BuildingTypes)getCivilizationInfo().getCivilizationBuildings(eBuildingClassLocal);


				if (eLocalBuilding != NO_BUILDING)
				{
					CvBuildingEntry* pkLocalBuilding = GC.getBuildingInfo(eLocalBuilding);
					if (pkLocalBuilding)
					{
	
						//int iYieldChange = 0;

						int iYieldChange = pBuildingInfo->GetBuildingClassLocalYieldChange(iJ, iI);

						if (isWorldWonderClass(*pkBuildingClassLocalInfo) && pBuildingInfo->GetYieldChangeWorldWonder(iI) != 0)
						{
							iYieldChange += pBuildingInfo->GetYieldChangeWorldWonder(iI);
						}

						if (iYieldChange != 0)
						{
							m_pCityBuildings->ChangeBuildingYieldChange(eBuildingClassLocal, eYield, (iYieldChange * iChange));
							changeLocalBuildingClassYield(eBuildingClassLocal, eYield, (iYieldChange* iChange));
						}
					}
				}
			}

			//Global Building Class Yield Changes
			changeYieldRateModifier((YieldTypes)iI, owningPlayer.GetBuildingClassYieldModifier((BuildingClassTypes)pBuildingInfo->GetBuildingClassType(), (YieldTypes)iI) * iChange);
#endif

#if defined(MOD_API_UNIFIED_YIELDS)
			if (pkBuildingClassInfo && isWorldWonderClass(*pkBuildingClassInfo))
			{
				ChangeBaseYieldRateFromBuildings(eYield, GetPlayer()->GetYieldChangeWorldWonder(eYield) * iChange);
				ChangeBaseYieldRateFromBuildings(eYield, GetPlayer()->GetPlayerTraits()->GetYieldChangeWorldWonder(eYield) * iChange);

#if defined(MOD_ROG_CORE)
				int iGlobalWonderBonus = owningPlayer.GetWorldWonderYieldChange(iI);
				if (iGlobalWonderBonus != 0)
				{
					m_pCityBuildings->ChangeBuildingYieldChange(eBuildingClass, eYield, (iGlobalWonderBonus * iChange));
					changeLocalBuildingClassYield(eBuildingClass, eYield, (iGlobalWonderBonus * iChange));
				}
#endif
			}
#endif


#if defined(MOD_ROG_CORE)
			for (int iJ = 0; iJ < GC.getNumImprovementInfos(); iJ++)
			{
				ChangeImprovementExtraYield(((ImprovementTypes)iJ), eYield, (GC.getBuildingInfo(eBuilding)->GetImprovementYieldChange(iJ, eYield) * iChange));

#if defined(MOD_IMPROVEMENT_FUNCTION)
				if (MOD_IMPROVEMENT_FUNCTION)
				{
					int iYieldMod = pBuildingInfo->GetImprovementYieldModifier(((ImprovementTypes)iJ), eYield);
					changeYieldModifierFromImprovement((ImprovementTypes)iJ, eYield, iYieldMod * iChange);
				}
#endif
			}
#endif

			for(int iJ = 0; iJ < GC.getNumResourceInfos(); iJ++)
			{
				ChangeResourceExtraYield(((ResourceTypes)iJ), eYield, (GC.getBuildingInfo(eBuilding)->GetResourceYieldChange(iJ, eYield) * iChange));
				int iYieldMod = pBuildingInfo->GetResourceYieldModifier(((ResourceTypes)iJ), eYield);
				changeYieldModifierFromResource((ResourceTypes)iJ, eYield, iYieldMod * iChange);
			}

			for(int iJ = 0; iJ < GC.getNumFeatureInfos(); iJ++)
			{
				ChangeFeatureExtraYield(((FeatureTypes)iJ), eYield, (GC.getBuildingInfo(eBuilding)->GetFeatureYieldChange(iJ, eYield) * iChange));
				ChangeYieldPerXFeatureFromBuildingsTimes100(((FeatureTypes)iJ), eYield, (GC.getBuildingInfo(eBuilding)->GetYieldPerXFeature(iJ, eYield) * iChange));
				int iYieldMod = pBuildingInfo->GetFeatureYieldModifier((FeatureTypes)iJ, eYield);
				changeYieldModifierFromFeature((FeatureTypes)iJ, eYield, iYieldMod * iChange);
			}

			for(int iJ = 0; iJ < GC.getNumTerrainInfos(); iJ++)
			{
				ChangeTerrainExtraYield(((TerrainTypes)iJ), eYield, (GC.getBuildingInfo(eBuilding)->GetTerrainYieldChange(iJ, eYield) * iChange));
				ChangeYieldPerXTerrainFromBuildingsTimes100(((TerrainTypes)iJ), eYield, (GC.getBuildingInfo(eBuilding)->GetYieldPerXTerrain(iJ, eYield) * iChange));
				int iYieldMod = pBuildingInfo->GetTerrainYieldModifier((TerrainTypes)iJ, eYield);
				changeYieldModifierFromTerrain((TerrainTypes)iJ, eYield, iYieldMod * iChange);
			}

#if defined(MOD_API_UNIFIED_YIELDS) && defined(MOD_API_PLOT_YIELDS)
			for(int iJ = 0; iJ < GC.getNumPlotInfos(); iJ++)
			{
				ChangePlotExtraYield(((PlotTypes)iJ), eYield, (GC.getBuildingInfo(eBuilding)->GetPlotYieldChange(iJ, eYield) * iChange));
			}
#endif

			if(pBuildingInfo->GetEnhancedYieldTech() != NO_TECH)
			{
				if(owningTeam.GetTeamTechs()->HasTech((TechTypes)pBuildingInfo->GetEnhancedYieldTech()))
				{
						ChangeBaseYieldRateFromBuildings(eYield, pBuildingInfo->GetTechEnhancedYieldChange(eYield) * iChange);
				}
			}

			int iBuildingClassBonus = owningPlayer.GetBuildingClassYieldChange(eBuildingClass, eYield);
			if(iBuildingClassBonus > 0)
			{
					ChangeBaseYieldRateFromBuildings(eYield, iBuildingClassBonus * iChange);
			}
		}
		// Hurries Local
		for(int iI = 0; iI < GC.getNumHurryInfos(); iI++)
		{
			ChangeHurryModifierLocal((HurryTypes) iI, (pBuildingInfo->GetHurryModifierLocal(iI) * iChange));
		}

		if(GC.getBuildingInfo(eBuilding)->GetSpecialistType() != NO_SPECIALIST)
		{
			GetCityCitizens()->ChangeBuildingGreatPeopleRateChanges((SpecialistTypes) GC.getBuildingInfo(eBuilding)->GetSpecialistType(), pBuildingInfo->GetGreatPeopleRateChange() * iChange);
		}

		for(int iI = 0; iI < GC.getNumUnitCombatClassInfos(); iI++)
		{
			const UnitCombatTypes eUnitCombatClass = static_cast<UnitCombatTypes>(iI);
			CvBaseInfo* pkUnitCombatClassInfo = GC.getUnitCombatClassInfo(eUnitCombatClass);
			if(pkUnitCombatClassInfo)
			{
				changeUnitCombatFreeExperience(eUnitCombatClass, pBuildingInfo->GetUnitCombatFreeExperience(iI) * iChange);
				changeUnitCombatProductionModifier(eUnitCombatClass, pBuildingInfo->GetUnitCombatProductionModifier(iI) * iChange);
			}
		}

		for(int iI = 0; iI < NUM_DOMAIN_TYPES; iI++)
		{
			changeDomainFreeExperience(((DomainTypes)iI), pBuildingInfo->GetDomainFreeExperience(iI) * iChange);
			changeDomainFreeExperiencesPerPop(((DomainTypes)iI), pBuildingInfo->GetDomainFreeExperiencesPerPop(iI) * iChange);
			changeDomainProductionModifier(((DomainTypes)iI), pBuildingInfo->GetDomainProductionModifier(iI) * iChange);
			changeDomainFreeExperiencesPerPopGlobal(((DomainTypes)iI), pBuildingInfo->GetDomainFreeExperiencesPerPopGlobal(iI) * iChange);
			changeDomainFreeExperiencesPerTurn(((DomainTypes)iI), pBuildingInfo->GetDomainFreeExperiencesPerTurn(iI) * iChange);
			changeDomainEnemyCombatModifier(((DomainTypes)iI), pBuildingInfo->GetDomainEnemyCombatModifier(iI) * iChange);
			changeDomainFriendsCombatModifierLocal(((DomainTypes)iI), pBuildingInfo->GetDomainFriendsCombatModifierLocal(iI) * iChange);
		}

		// Process for our player
		for(int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if(GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam())
			{
				if(pBuildingInfo->IsTeamShare() || (iI == getOwner()))
				{
					GET_PLAYER((PlayerTypes)iI).processBuilding(eBuilding, iChange, bFirst, area());
				}
			}
		}

		// Process for our team
		owningTeam.processBuilding(eBuilding, iChange, bFirst);
#ifdef MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD
		if (pBuildingInfo->HasYieldFromOtherYield())
		{
			bool bNewHasYieldFromOtherYield = false;
			for (size_t i = 0; i < NUM_YIELD_TYPES; i++)
			{
				for (size_t j = 0; j < NUM_YIELD_TYPES; j++)
				{
					const YieldTypes eOutYieldType = static_cast<YieldTypes>(i);
					const YieldTypes eInYieldType = static_cast<YieldTypes>(j);
					auto yieldInValue = pBuildingInfo->GetYieldFromOtherYield(eInYieldType, eOutYieldType, YieldFromYield::IN_VALUE);
					auto yieldOutValue = pBuildingInfo->GetYieldFromOtherYield(eInYieldType, eOutYieldType, YieldFromYield::OUT_VALUE);
					//m_ppiYieldFromOtherYield[eOutYieldType][eInYieldType][IN_VALUE] += pBuildingInfo->GetYieldFromOtherYield(eInYieldType, eOutYieldType, IN_VALUE) * iChange;
					//m_ppiYieldFromOtherYield[eOutYieldType][eInYieldType][OUT_VALUE] +=  * iChange;
					if (yieldInValue > 0 && yieldOutValue > 0) {
						bNewHasYieldFromOtherYield = true;
						if (iChange > 0) {
							for (size_t k = 0; k < iChange; k++) {
								Firaxis::Array<int, YieldFromYieldStruct::STRUCT_LENGTH> yieldConvert;
								yieldConvert[YieldFromYieldStruct::IN_YIELD_TYPE] = eInYieldType;
								yieldConvert[YieldFromYieldStruct::OUT_YIELD_TYPE] = eOutYieldType;
								yieldConvert[YieldFromYieldStruct::IN_YIELD_VALUE] = yieldInValue;
								yieldConvert[YieldFromYieldStruct::OUT_YIELD_VALUE] = yieldOutValue;
								m_ppiYieldFromOtherYield.push_back(yieldConvert);
							}
						}
						else {
							for (size_t k = 0; k < -iChange; k++) {
								for (auto& ite = m_ppiYieldFromOtherYield.begin(); ite != m_ppiYieldFromOtherYield.end(); ite++) {
									if ((*ite)[YieldFromYieldStruct::IN_YIELD_TYPE] == (int)eInYieldType
										&& (*ite)[YieldFromYieldStruct::OUT_YIELD_TYPE] == (int)eOutYieldType
										&& (*ite)[YieldFromYieldStruct::IN_YIELD_VALUE] == yieldInValue
										&& (*ite)[YieldFromYieldStruct::OUT_YIELD_VALUE] == yieldOutValue
										) {
										m_ppiYieldFromOtherYield.erase(ite);
										break;
									}
								}
							}
						}
					}
					
				}
			}

			m_bHasYieldFromOtherYield = bNewHasYieldFromOtherYield;
		}
#endif
	}

	if(!bObsolete)
	{
		m_pCityBuildings->ChangeBuildingDefense(pBuildingInfo->GetDefenseModifier() * iChange);

		owningTeam.changeBuildingClassCount(eBuildingClass, iChange);
		owningPlayer.changeBuildingClassCount(eBuildingClass, iChange);
		ChangeNumBuildingClass(eBuildingClass, iChange);
	}

	UpdateReligion(GetCityReligions()->GetReligiousMajority());

	owningPlayer.DoUpdateHappiness();

	setLayoutDirty(true);
}


//	--------------------------------------------------------------------------------
void CvCity::processProcess(ProcessTypes eProcess, int iChange)
{
	VALIDATE_OBJECT
	int iI;

	const CvProcessInfo* pkProcessInfo = GC.getProcessInfo(eProcess);
	CvAssertFmt(pkProcessInfo != NULL, "Process type %d is invalid", eProcess);
	if (pkProcessInfo != NULL)
	{
		// Convert to another yield
		for(iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			changeProductionToYieldModifier((YieldTypes)iI, (pkProcessInfo->getProductionToYieldModifier(iI)) * iChange);
			UpdateCityYields((YieldTypes)iI);

		}
	}
}

#ifdef MOD_SPECIALIST_RESOURCES
static inline void ChangeResourceFromSpecialist(CvCity* city, CvSpecialistInfo* pkSpecialist, int iChange)
{
	if (!MOD_SPECIALIST_RESOURCES) return;

	CvPlayerAI& owner = GET_PLAYER(city->getOwner());
	for (auto& resourceInfo : pkSpecialist->GetResourceInfo())
	{
		if (!owner.MeetSpecialistResourceRequirement(resourceInfo)) continue;

		owner.changeResourceFromSpecialists(resourceInfo.m_eResource, resourceInfo.m_iQuantity * iChange);
	}
}
#endif

//	--------------------------------------------------------------------------------
void CvCity::processSpecialist(SpecialistTypes eSpecialist, int iChange)
{
	VALIDATE_OBJECT
	int iI;

	CvSpecialistInfo* pkSpecialist = GC.getSpecialistInfo(eSpecialist);
	if(pkSpecialist == NULL)
	{
		//This function requires a valid specialist type.
		return;
	}

	changeBaseGreatPeopleRate(pkSpecialist->getGreatPeopleRateChange() * iChange);

	for(iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		ChangeBaseYieldRateFromSpecialists(((YieldTypes)iI), (pkSpecialist->getYieldChange(iI) * iChange));

		//int globalModifier = GET_PLAYER(getOwner()).GetYieldModifierFromSpecialistGlobal(eSpecialist, ((YieldTypes)iI));
		int LocalModifier = getYieldModifierFromSpecialist(eSpecialist, ((YieldTypes)iI));
		changeSpecialistYieldRateModifier(((YieldTypes)iI), (LocalModifier)*iChange);
	}

	updateExtraSpecialistYield();

	changeSpecialistFreeExperience(pkSpecialist->getExperience() * iChange);

	// Culture
	int iCulturePerSpecialist = GetCultureFromSpecialist(eSpecialist);
	ChangeJONSCulturePerTurnFromSpecialists(iCulturePerSpecialist * iChange);

#ifdef MOD_SPECIALIST_RESOURCES
	ChangeResourceFromSpecialist(this, pkSpecialist, iChange);
#endif

	UpdateAllNonPlotYields();
}

//	--------------------------------------------------------------------------------
void CvCity::initFreeUnit(CvPlayer& owningPlayer, UnitTypes eUnit, int iCount, bool bToCivType)
{
	int iFreeUnitLoop = 0;
	CvUnit* pFreeUnit = NULL;
	CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);
	if(pkUnitInfo)
	{
		if(bToCivType)
		{
			// Get the right unit of this class for this civ
			eUnit = owningPlayer.GetCivUnit((UnitClassTypes)pkUnitInfo->GetUnitClassType());
		}
	}
	pkUnitInfo = GC.getUnitInfo(eUnit);
	if(pkUnitInfo)
	{
		if(pkUnitInfo->IsFound())
		{
			if(GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
			{
				return;
			}
			else if(owningPlayer.GetPlayerTraits()->IsNoAnnexing())
			{
				// drop a merchant of venice instead
				// find the eUnit replacement that's the merchant of venice
				for(int iVeniceSearch = 0; iVeniceSearch < GC.getNumUnitClassInfos(); iVeniceSearch++)
				{
					const UnitClassTypes eVeniceUnitClass = static_cast<UnitClassTypes>(iVeniceSearch);
					CvUnitClassInfo* pkVeniceUnitClassInfo = GC.getUnitClassInfo(eVeniceUnitClass);
					if(pkVeniceUnitClassInfo)
					{
						const UnitTypes eMerchantOfVeniceUnit = (UnitTypes) getCivilizationInfo().getCivilizationUnits(eVeniceUnitClass);
						if (eMerchantOfVeniceUnit != NO_UNIT)
						{
							CvUnitEntry* pVeniceUnitEntry = GC.getUnitInfo(eMerchantOfVeniceUnit);
							if (pVeniceUnitEntry->IsCanBuyCityState())
							{
								eUnit = eMerchantOfVeniceUnit;
								pkUnitInfo = pVeniceUnitEntry;
								break;
							}
						}
					}
				}
			}
		}
		
		SpecialUnitTypes eSpecialUnitGreatPerson = (SpecialUnitTypes) GC.getInfoTypeForString("SPECIALUNIT_PEOPLE");
		for(iFreeUnitLoop = 0; iFreeUnitLoop < iCount; iFreeUnitLoop++)
		{
			// Great prophet?
			if(GC.GetGameUnits()->GetEntry(eUnit)->IsFoundReligion())
			{
				GetCityCitizens()->DoSpawnGreatPerson(eUnit, true /*bIncrementCount*/, true);
			}
			else if(pkUnitInfo->GetSpecialUnitType() == eSpecialUnitGreatPerson)
			{
				GetCityCitizens()->DoSpawnGreatPerson(eUnit, true /*bIncrementCount*/, false);
			}
			else
			{
				pFreeUnit = owningPlayer.initUnit(eUnit, getX(), getY());
				if(!bToCivType && !pFreeUnit->jumpToNearestValidPlot())
				{
					pFreeUnit->kill(false);
				}
				if(pFreeUnit->IsCombatUnit())
				{
					addProductionExperience(pFreeUnit);
				}
			}
		}
	}



}
//	--------------------------------------------------------------------------------
/// Process the majority religion changing for a city
void CvCity::UpdateReligion(ReligionTypes eNewMajority)
{
	updateYield();

	// Reset city level yields
	for(int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
	{
		m_aiBaseYieldRateFromReligion[iYield] = 0;
	}

	BeliefTypes eSecondaryPantheon = GetCityReligions()->GetSecondaryReligionPantheonBelief();
	const CvReligion* pReligion = (eNewMajority != NO_RELIGION) ? GC.getGame().GetGameReligions()->GetReligion(eNewMajority, getOwner()) : 0;
	const CvBeliefEntry* pSecondaryPantheon = (eSecondaryPantheon != NO_BELIEF) ? GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon) : 0;
	TerrainTypes eTerrain = plot()->getTerrainType();

	for(int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
	{
		YieldTypes eYield = (YieldTypes)iYield;
		if(eNewMajority != NO_RELIGION)
		{
			if(pReligion)
			{
				int iFollowers = GetCityReligions()->GetNumFollowers(eNewMajority);

				int iReligionYieldChange = pReligion->m_Beliefs.GetCityYieldChange(getPopulation(), eYield);
#if defined(MOD_API_UNIFIED_YIELDS)
				if (isCapital()) {
					iReligionYieldChange += pReligion->m_Beliefs.GetCapitalYieldChange(getPopulation(), eYield);
				}
				if (isCoastal()) {
					iReligionYieldChange += pReligion->m_Beliefs.GetCoastalCityYieldChange(getPopulation(), eYield);
				}
#endif
				if(eTerrain != NO_TERRAIN)
				{
					iReligionYieldChange += pReligion->m_Beliefs.GetTerrainCityYieldChanges(eTerrain, eYield);
				}
				if (eSecondaryPantheon != NO_BELIEF)
				{
					if(getPopulation() >= pSecondaryPantheon->GetMinPopulation())
					{
						iReligionYieldChange += pSecondaryPantheon->GetCityYieldChange(eYield);
#if defined(MOD_API_UNIFIED_YIELDS)
						if (isCapital()) {
							iReligionYieldChange += pSecondaryPantheon->GetCapitalYieldChange(eYield);
						}
						if (isCoastal()) {
							iReligionYieldChange += pSecondaryPantheon->GetCoastalCityYieldChange(eYield);
						}
#endif
					}
					if(eTerrain != NO_TERRAIN)
					{
						iReligionYieldChange += pSecondaryPantheon->GetTerrainCityYieldChanges(eTerrain, eYield);
					}
				}

				ChangeBaseYieldRateFromReligion(eYield, iReligionYieldChange);

				if(IsRouteToCapitalConnected())
				{
					int iReligionChange = pReligion->m_Beliefs.GetYieldChangeTradeRoute(eYield);
					//BeliefTypes eSecondaryPantheon = GetCityReligions()->GetSecondaryReligionPantheonBelief();
					if (eSecondaryPantheon != NO_BELIEF)
					{
						iReligionChange += pSecondaryPantheon->GetYieldChangeTradeRoute(eYield);
					}

					ChangeBaseYieldRateFromReligion(eYield, iReligionChange);
				}
				
				if (GetCityCitizens()->GetTotalSpecialistCount() > 0)
				{
					ChangeBaseYieldRateFromReligion(eYield, pReligion->m_Beliefs.GetYieldChangeAnySpecialist(eYield));
				}

				// Buildings
				for(int jJ = 0; jJ < GC.getNumBuildingClassInfos(); jJ++)
				{
					BuildingClassTypes eBuildingClass = (BuildingClassTypes)jJ;

					CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo(eBuildingClass);
					if(!pkBuildingClassInfo)
					{
						continue;
					}

					CvCivilizationInfo& playerCivilizationInfo = getCivilizationInfo();
					BuildingTypes eBuilding = (BuildingTypes)playerCivilizationInfo.getCivilizationBuildings(eBuildingClass);

					if(eBuilding != NO_BUILDING)
					{
						if(GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
						{
							int iYieldFromBuilding = pReligion->m_Beliefs.GetBuildingClassYieldChange(eBuildingClass, eYield, iFollowers);
#if defined(MOD_BUGFIX_MINOR)
							iYieldFromBuilding *= GetCityBuildings()->GetNumBuilding(eBuilding);
#endif

							if (isWorldWonderClass(*pkBuildingClassInfo))
							{
								iYieldFromBuilding += pReligion->m_Beliefs.GetYieldChangeWorldWonder(eYield);
							}

							ChangeBaseYieldRateFromReligion(eYield, iYieldFromBuilding);
						}
					}
				}
			}
		}
	}

#if defined(MOD_BALANCE_CORE)
	//Some yield cleanup and refresh here - note that not all of this has to do with religion, however any time religion is updated, that's a good time to update the city's yields.
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		YieldTypes eYield = (YieldTypes)iI;
		if (eYield == NO_YIELD)
			continue;

		UpdateCityYields(eYield);
		UpdateYieldPerXFeature(eYield);
		UpdateYieldPerXTerrain(eYield, TERRAIN_MOUNTAIN);
		UpdateYieldPerXFeature(eYield);
		updateExtraSpecialistYield(eYield);
	}
#endif

	GET_PLAYER(getOwner()).UpdateReligion();
}

//	--------------------------------------------------------------------------------
//very reduced version of UpdateReligion() which assumes only the number of specialists changed
void CvCity::UpdateReligiousYieldFromSpecialist(bool bFirstOneAdded)
{
	const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(GetCityReligions()->GetReligiousMajority(), getOwner());
	if (pReligion)
	{
		for (int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
		{
			int iChange = pReligion->m_Beliefs.GetYieldChangeAnySpecialist((YieldTypes)iYield);
			iChange *= bFirstOneAdded ? 1 : -1;
			ChangeBaseYieldRateFromReligion((YieldTypes)iYield, iChange);
		}
	}
}
//	--------------------------------------------------------------------------------
/// Culture from eSpecialist
int CvCity::GetCultureFromSpecialist(SpecialistTypes eSpecialist) const
{
	CvSpecialistInfo* pkSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
	if(pkSpecialistInfo == NULL)
	{
		//This function REQUIRES a valid specialist type.
		return 0;
	}

	int iCulture = GET_PLAYER(getOwner()).GetSpecialistCultureChange();

	return iCulture;
}

//	--------------------------------------------------------------------------------
CvHandicapInfo& CvCity::getHandicapInfo() const
{
	return GET_PLAYER(getOwner()).getHandicapInfo();
}

//	--------------------------------------------------------------------------------
HandicapTypes CvCity::getHandicapType() const
{
	VALIDATE_OBJECT
	return GET_PLAYER(getOwner()).getHandicapType();
}

//	--------------------------------------------------------------------------------
CvCivilizationInfo& CvCity::getCivilizationInfo() const
{
	return GET_PLAYER(getOwner()).getCivilizationInfo();
}

//	--------------------------------------------------------------------------------
CivilizationTypes CvCity::getCivilizationType() const
{
	VALIDATE_OBJECT
	return GET_PLAYER(getOwner()).getCivilizationType();
}


//	--------------------------------------------------------------------------------
LeaderHeadTypes CvCity::getPersonalityType() const
{
	VALIDATE_OBJECT
	return GET_PLAYER(getOwner()).getPersonalityType();
}


//	--------------------------------------------------------------------------------
ArtStyleTypes CvCity::getArtStyleType() const
{
	VALIDATE_OBJECT
	return GET_PLAYER(getOwner()).getArtStyleType();
}


//	--------------------------------------------------------------------------------
CitySizeTypes CvCity::getCitySizeType() const
{
	VALIDATE_OBJECT
	return ((CitySizeTypes)(range((getPopulation() / 7), 0, (NUM_CITYSIZE_TYPES - 1))));
}


//	--------------------------------------------------------------------------------
bool CvCity::isBarbarian() const
{
	VALIDATE_OBJECT
	return GET_PLAYER(getOwner()).isBarbarian();
}


//	--------------------------------------------------------------------------------
bool CvCity::isHuman() const
{
	VALIDATE_OBJECT
	return GET_PLAYER(getOwner()).isHuman();
}


//	--------------------------------------------------------------------------------
bool CvCity::isVisible(TeamTypes eTeam, bool bDebug) const
{
	VALIDATE_OBJECT
	return plot()->isVisible(eTeam, bDebug);
}


//	--------------------------------------------------------------------------------
bool CvCity::isCapital() const
{
	VALIDATE_OBJECT
	return (GET_PLAYER(getOwner()).getCapitalCity() == this);
}

//	--------------------------------------------------------------------------------
/// Was this city originally any player's capital?
bool CvCity::IsOriginalCapital() const
{
	VALIDATE_OBJECT

	CvPlayerAI& kPlayer = GET_PLAYER(m_eOriginalOwner);
	if (getX() == kPlayer.GetOriginalCapitalX() && getY() == kPlayer.GetOriginalCapitalY())
	{
		return true;
	}

	return false;
}

//	--------------------------------------------------------------------------------
/// Was this city originally a major civ's capital?
bool CvCity::IsOriginalMajorCapital() const
{
	VALIDATE_OBJECT

	PlayerTypes ePlayer;
	for(int iPlayerLoop = 0; iPlayerLoop < MAX_MAJOR_CIVS; iPlayerLoop++)
	{
		ePlayer = (PlayerTypes) iPlayerLoop;
		CvPlayerAI& kPlayer = GET_PLAYER(ePlayer);
		if (getX() == kPlayer.GetOriginalCapitalX() && getY() == kPlayer.GetOriginalCapitalY())
		{
			return true;
		}
	}

	return false;
}

//	--------------------------------------------------------------------------------
bool CvCity::IsEverCapital() const
{
	VALIDATE_OBJECT
	return m_bEverCapital;
}

//	--------------------------------------------------------------------------------
void CvCity::SetEverCapital(bool bValue)
{
	VALIDATE_OBJECT
	if(IsEverCapital() != bValue)
	{
		m_bEverCapital = bValue;
	}
}


//	--------------------------------------------------------------------------------
bool CvCity::isCoastal(int iMinWaterSize) const
{
	VALIDATE_OBJECT
	return plot()->isCoastalArea(iMinWaterSize);
}


#if defined(MOD_VOLCANO_BREAK)
//	--------------------------------------------------------------------------------
int CvCity::getImmueVolcanoDamage() const
{
	return m_iImmueVolcanoDamage;
}
bool CvCity::isImmueVolcanoDamage() const
{
	return (getImmueVolcanoDamage() > 0);
}
void CvCity::changeImmueVolcanoDamage(int iChange)
{
	if (iChange != 0)
	{
		m_iImmueVolcanoDamage = (m_iImmueVolcanoDamage + iChange);
		CvAssert(getImmueVolcanoDamage() >= 0);
	}
}
#endif

#if defined(MOD_API_EXTENSIONS)
//	--------------------------------------------------------------------------------
bool CvCity::isExtraAttackOnKill() const
{
	return (m_iExtraAttackOnKill > 0);
}

//	--------------------------------------------------------------------------------
void CvCity::changeExtraAttackOnKill(int iChange)
{
	if (iChange != 0)
	{
		m_iExtraAttackOnKill = (m_iExtraAttackOnKill + iChange);
		CvAssert(m_iExtraAttackOnKill >= 0);
	}
}


//	--------------------------------------------------------------------------------
bool CvCity::isAddsFreshWater() const
{
	return (m_iAddsFreshWater > 0);
}

//	--------------------------------------------------------------------------------
void CvCity::changeAddsFreshWater(int iChange)
{
	if (iChange != 0)
	{
		m_iAddsFreshWater = (m_iAddsFreshWater + iChange);
		CvAssert(m_iAddsFreshWater >= 0);
	}
}



//	--------------------------------------------------------------------------------
int CvCity::getExtraBombardRange() const
{
	VALIDATE_OBJECT
	return m_iBombardRange;
}

//	--------------------------------------------------------------------------------
void CvCity::changeExtraBombardRange(int iChange)
{
	VALIDATE_OBJECT
		if (iChange != 0)
		{
			m_iBombardRange += iChange;
		}
}

bool CvCity::isBombardIndirect() const
{
	VALIDATE_OBJECT
	return m_iBombardIndirect > 0;
}
void CvCity::changeBombardIndirect(int iValue)
{
	if (iValue != 0)
	{
		m_iBombardIndirect += iValue;
	}
}
#endif



//	--------------------------------------------------------------------------------
int CvCity::getForbiddenForeignSpyCount() const
{
	return m_iForbiddenForeignSpyCount;
}

//	--------------------------------------------------------------------------------
bool CvCity::isForbiddenForeignSpy() const
{
	return (getForbiddenForeignSpyCount() > 0);
}

//	--------------------------------------------------------------------------------
void CvCity::changeForbiddenForeignSpyCount(int iChange)
{
	if (iChange != 0)
	{
		m_iForbiddenForeignSpyCount = (m_iForbiddenForeignSpyCount + iChange);
		CvAssert(getForbiddenForeignSpyCount() >= 0);
	}
}

//	--------------------------------------------------------------------------------
int CvCity::GetAllowSpaceshipLaunchCount() const
{
	return m_iAllowSpaceshipLaunchCount;
}

//	--------------------------------------------------------------------------------
bool CvCity::IsAllowSpaceshipLaunch() const
{
	return (GetAllowSpaceshipLaunchCount() > 0 || isCapital());
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeAllowSpaceshipLaunchCount(int iChange)
{
	if (iChange != 0)
	{
		m_iAllowSpaceshipLaunchCount = (m_iAllowSpaceshipLaunchCount + iChange);
	}
}


//	--------------------------------------------------------------------------------
int CvCity::foodConsumption(bool /*bNoAngry*/, int iExtra) const
{
	VALIDATE_OBJECT
	int iPopulation = getPopulation() + iExtra;

	int iFoodPerPop = /*2*/ GC.getFOOD_CONSUMPTION_PER_POPULATION();

	iFoodPerPop += GetAdditionalFood();

	int iNum = iPopulation * iFoodPerPop;

	// Specialists eat less food? (Policies, etc.)
	if(GET_PLAYER(getOwner()).isHalfSpecialistFood())
	{
		int iFoodReduction = GetCityCitizens()->GetTotalSpecialistCount() * iFoodPerPop;
		iFoodReduction /= 2;
		iNum -= iFoodReduction;
	}

	TerrainTypes eTerrain = plot()->getTerrainType();
	if(eTerrain == NO_TERRAIN) return iNum;

	int iConsumptionModifier = 100;
	ReligionTypes eMajority = GetCityReligions()->GetReligiousMajority();
	if(eMajority != NO_RELIGION)
	{
		iConsumptionModifier += GC.getGame().GetGameReligions()->GetReligion(eMajority, getOwner())->m_Beliefs.GetTerrainCityFoodConsumption(eTerrain);
	}	
	BeliefTypes eSecondaryPantheon = GetCityReligions()->GetSecondaryReligionPantheonBelief();
	if(eSecondaryPantheon != NO_BELIEF)
	{
		iConsumptionModifier += GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon)->GetTerrainCityFoodConsumption(eTerrain);
	}
	iNum = iNum * iConsumptionModifier / 100;
	
	return iNum;
}

//	--------------------------------------------------------------------------------
int CvCity::foodDifference(bool bBottom) const
{
	VALIDATE_OBJECT
	return foodDifferenceTimes100(bBottom) / 100;
}


//	--------------------------------------------------------------------------------
int CvCity::foodDifferenceTimes100(bool bBottom, CvString* toolTipSink) const
{
	VALIDATE_OBJECT
	int iDifference;

	if(isFoodProduction())
	{
		iDifference = std::min(0, GetFoodProduction(getYieldRate(YIELD_FOOD, false) - foodConsumption()) * 100);
	}
	else
	{
		iDifference = (getYieldRateTimes100(YIELD_FOOD, false) - foodConsumption() * 100);
	}

	if(bBottom)
	{
		if((getPopulation() == 1) && (getFood() == 0))
		{
			iDifference = std::max(0, iDifference);
		}
	}

	// Growth Mods - Only apply if the City is growing (and not starving, otherwise it would actually have the OPPOSITE of the intended effect!)
	if(iDifference > 0)
	{
		int iTotalMod = 100;

		// Capital Mod for player. Used for Policies and such
		if(isCapital())
		{
			int iCapitalGrowthMod = GET_PLAYER(getOwner()).GetCapitalGrowthMod();
			if(iCapitalGrowthMod != 0)
			{
				iTotalMod += iCapitalGrowthMod;
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_FOODMOD_CAPITAL", iCapitalGrowthMod);
			}
		}

		// City Mod for player. Used for Policies and such
		int iCityGrowthMod = GET_PLAYER(getOwner()).GetCityGrowthMod();
		if(iCityGrowthMod != 0)
		{
			iTotalMod += iCityGrowthMod;
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_FOODMOD_PLAYER", iCityGrowthMod);
		}

		// Religion growth mod
		int iReligionGrowthMod = 0;
		ReligionTypes eMajority = GetCityReligions()->GetReligiousMajority();
		if(eMajority != NO_RELIGION)
		{
			const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eMajority, getOwner());
			if(pReligion)
			{
				bool bAtPeace = GET_TEAM(getTeam()).getAtWarCount(false) == 0;
				iReligionGrowthMod = pReligion->m_Beliefs.GetCityGrowthModifier(bAtPeace);
				BeliefTypes eSecondaryPantheon = GetCityReligions()->GetSecondaryReligionPantheonBelief();
				if (eSecondaryPantheon != NO_BELIEF)
				{
					iReligionGrowthMod += GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon)->GetCityGrowthModifier();
				}
				iTotalMod += iReligionGrowthMod;
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_FOODMOD_RELIGION", iReligionGrowthMod);
			}
		}

		// Cities stop growing when empire is very unhappy
		if(GET_PLAYER(getOwner()).IsEmpireVeryUnhappy())
		{
			int iMod = /*-100*/ GC.getVERY_UNHAPPY_GROWTH_PENALTY();
			iTotalMod += iMod;
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_FOODMOD_UNHAPPY", iMod);
		}
		// Cities grow slower if the player is over his Happiness Limit
		else if(GET_PLAYER(getOwner()).IsEmpireUnhappy())
		{
			int iMod = /*-75*/ GC.getUNHAPPY_GROWTH_PENALTY();
			iTotalMod += iMod;
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_FOODMOD_UNHAPPY", iMod);
		}
		// WLTKD Growth Bonus
		else if(GetWeLoveTheKingDayCounter() > 0)
		{
			int iMod = /*25*/ GC.getWLTKD_GROWTH_MULTIPLIER();
			iTotalMod += iMod;
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_FOODMOD_WLTKD", iMod);
		}

		iDifference *= iTotalMod;
		iDifference /= 100;
	}

	return iDifference;
}


//	--------------------------------------------------------------------------------
int CvCity::growthThreshold() const
{
	VALIDATE_OBJECT
	int iNumFoodNeeded = GET_PLAYER(getOwner()).getGrowthThreshold(getPopulation());

	return (iNumFoodNeeded);
}


//	--------------------------------------------------------------------------------
int CvCity::productionLeft() const
{
	VALIDATE_OBJECT
	return (getProductionNeeded() - getProduction());
}

//	--------------------------------------------------------------------------------
int CvCity::getHurryCostModifier(HurryTypes eHurry, bool bIgnoreNew) const
{
	VALIDATE_OBJECT
	int iModifier = 100;
	const OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		switch(pOrderNode->eOrderType)
		{
		case ORDER_TRAIN:
			iModifier = getHurryCostModifier(eHurry, (UnitTypes) pOrderNode->iData1, bIgnoreNew);
			break;

		case ORDER_CONSTRUCT:
			iModifier = getHurryCostModifier(eHurry, (BuildingTypes) pOrderNode->iData1, bIgnoreNew);
			break;

		case ORDER_CREATE:
		case ORDER_PREPARE:
		case ORDER_MAINTAIN:
			break;

		default:
			CvAssertMsg(false, "pOrderNode->eOrderType did not match a valid option");
			break;
		}
	}

	return iModifier;
}

//	--------------------------------------------------------------------------------
int CvCity::getHurryCostModifier(HurryTypes eHurry, UnitTypes eUnit, bool bIgnoreNew) const
{
	VALIDATE_OBJECT
	CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);
	if(pkUnitInfo)
	{
		return getHurryCostModifier(eHurry, pkUnitInfo->GetHurryCostModifier(), getUnitProduction(eUnit), bIgnoreNew);
	}

	return 0;
}

//	--------------------------------------------------------------------------------
int CvCity::getHurryCostModifier(HurryTypes eHurry, BuildingTypes eBuilding, bool bIgnoreNew) const
{
	VALIDATE_OBJECT
	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if(pkBuildingInfo)
	{
		return getHurryCostModifier(eHurry, pkBuildingInfo->GetHurryCostModifier(), m_pCityBuildings->GetBuildingProduction(eBuilding), bIgnoreNew);
	}

	return 0;
}

//	--------------------------------------------------------------------------------
int CvCity::getHurryCostModifier(HurryTypes eHurry, int iBaseModifier, int iProduction, bool bIgnoreNew) const
{
	VALIDATE_OBJECT
	int iModifier = 100;
	iModifier *= std::max(0, iBaseModifier + 100);
	iModifier /= 100;

	if(iProduction == 0 && !bIgnoreNew)
	{
		iModifier *= std::max(0, (GC.getNEW_HURRY_MODIFIER() + 100));
		iModifier /= 100;
	}

	// Some places just don't care what kind of Hurry it is (leftover from Civ 4)
	if(eHurry != NO_HURRY)
	{
		iModifier *= GetHurryModifier(eHurry);
		iModifier /= 100;
	}

	return iModifier;
}


//	--------------------------------------------------------------------------------
int CvCity::hurryCost(HurryTypes eHurry, bool bExtra) const
{
	VALIDATE_OBJECT
	return (getHurryCost(bExtra, productionLeft(), getHurryCostModifier(eHurry), getProductionModifier()));
}

//	--------------------------------------------------------------------------------
int CvCity::getHurryCost(HurryTypes eHurry, bool bExtra, UnitTypes eUnit, bool bIgnoreNew) const
{
	VALIDATE_OBJECT
	int iProductionLeft = getProductionNeeded(eUnit) - getUnitProduction(eUnit);

	return getHurryCost(bExtra, iProductionLeft, getHurryCostModifier(eHurry, eUnit, bIgnoreNew), getProductionModifier(eUnit));
}

//	--------------------------------------------------------------------------------
int CvCity::getHurryCost(HurryTypes eHurry, bool bExtra, BuildingTypes eBuilding, bool bIgnoreNew) const
{
	VALIDATE_OBJECT
	int iProductionLeft = getProductionNeeded(eBuilding) - m_pCityBuildings->GetBuildingProduction(eBuilding);

	return getHurryCost(bExtra, iProductionLeft, getHurryCostModifier(eHurry, eBuilding, bIgnoreNew), getProductionModifier(eBuilding));
}

//	--------------------------------------------------------------------------------
int CvCity::getHurryCost(bool bExtra, int iProductionLeft, int iHurryModifier, int iModifier) const
{
	VALIDATE_OBJECT
	int iProduction = (iProductionLeft * iHurryModifier + 99) / 100; // round up

	if(bExtra)
	{
		int iExtraProduction = getExtraProductionDifference(iProduction, iModifier);
		if(iExtraProduction > 0)
		{
			int iAdjustedProd = iProduction * iProduction;

			// round up
			iProduction = (iAdjustedProd + (iExtraProduction - 1)) / iExtraProduction;
		}
	}

	return std::max(0, iProduction);
}

//	--------------------------------------------------------------------------------
int CvCity::hurryGold(HurryTypes eHurry) const
{
	VALIDATE_OBJECT
	int iFullCost = getProductionNeeded();

	return getHurryGold(eHurry, hurryCost(eHurry, false), iFullCost);
}

//	--------------------------------------------------------------------------------
/// Amount of Gold required to hurry Production in a City.  Full cost is the original Production cost of whatever we're rushing - the more expensive the ORIGINAL cost, the more it also costs to Gold rush
int CvCity::getHurryGold(HurryTypes /*eHurry*/, int /*iHurryCost*/, int /*iFullCost*/) const
{
	VALIDATE_OBJECT

	// This should not be used any more. Check GetPurchaseCostFromProduction()
	return 0;
}


//	--------------------------------------------------------------------------------
int CvCity::hurryPopulation(HurryTypes eHurry) const
{
	VALIDATE_OBJECT
	return (getHurryPopulation(eHurry, hurryCost(eHurry, true)));
}

//	--------------------------------------------------------------------------------
int CvCity::getHurryPopulation(HurryTypes eHurry, int iHurryCost) const
{
	VALIDATE_OBJECT
	CvHurryInfo* pkHurryInfo = GC.getHurryInfo(eHurry);
	if(pkHurryInfo == NULL)
	{
		return 0;
	}

	if(pkHurryInfo->getProductionPerPopulation() == 0)
	{
		return 0;
	}

	int iPopulation = (iHurryCost - 1) / GC.getGame().getProductionPerPopulation(eHurry);

	return std::max(1, (iPopulation + 1));
}

//	--------------------------------------------------------------------------------
int CvCity::hurryProduction(HurryTypes eHurry) const
{
	VALIDATE_OBJECT
	int iProduction = 0;

	CvHurryInfo* pkHurryInfo = GC.getHurryInfo(eHurry);
	if(pkHurryInfo)
	{
		if(pkHurryInfo->getProductionPerPopulation() > 0)
		{
			iProduction = (100 * getExtraProductionDifference(hurryPopulation(eHurry) * GC.getGame().getProductionPerPopulation(eHurry))) / std::max(1, getHurryCostModifier(eHurry));
			CvAssert(iProduction >= productionLeft());
		}
		else
		{
			iProduction = productionLeft();
		}
	}

	return iProduction;
}

//	--------------------------------------------------------------------------------
int CvCity::GetHurryModifier(HurryTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumHurryInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	int tmp = 100;
	tmp += GET_PLAYER(getOwner()).getHurryModifier(eIndex);
	tmp += GetHurryModifierLocal(eIndex);
	return tmp < 0 ? 0 : tmp;
}
//	--------------------------------------------------------------------------------
int CvCity::GetHurryModifierLocal(HurryTypes eIndex) const
{
	return m_paiHurryModifier[eIndex];
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeHurryModifierLocal(HurryTypes eIndex, int iChange)
{
	if(iChange != 0)
	{
		CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
		CvAssertMsg(eIndex < GC.getNumHurryInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
		m_paiHurryModifier.setAt(eIndex, m_paiHurryModifier[eIndex] + iChange);
	}
}

int CvCity::GetTradeRouteFromTheCityYields(YieldTypes eIndex) const
{
	if (eIndex < 0 || eIndex >= NUM_YIELD_TYPES)
	{
		return 0;
	}

	return m_aTradeRouteFromTheCityYields[eIndex];
}

void CvCity::ChangeTradeRouteFromTheCityYields(YieldTypes eIndex, int iChange)
{
	if (eIndex < 0 || eIndex >= NUM_YIELD_TYPES)
	{
		return;
	}
	m_aTradeRouteFromTheCityYields[eIndex] += iChange;
}

int CvCity::GetTradeRouteFromTheCityYieldsPerEra(YieldTypes eIndex) const
{
	if (eIndex < 0 || eIndex >= NUM_YIELD_TYPES)
	{
		return 0;
	}

	return m_aTradeRouteFromTheCityYieldsPerEra[eIndex];
}

void CvCity::ChangeTradeRouteFromTheCityYieldsPerEra(YieldTypes eIndex, int iChange)
{
	if (eIndex < 0 || eIndex >= NUM_YIELD_TYPES)
	{
		return;
	}
	m_aTradeRouteFromTheCityYieldsPerEra[eIndex] += iChange;
}

//	--------------------------------------------------------------------------------
int CvCity::GetLastTurnWorkerDisbanded() const
{
	VALIDATE_OBJECT
	return m_iLastTurnWorkerDisbanded;
}

//	--------------------------------------------------------------------------------
void CvCity::SetLastTurnWorkerDisbanded(int iValue)
{
	VALIDATE_OBJECT
	m_iLastTurnWorkerDisbanded = iValue;
}

//	--------------------------------------------------------------------------------
int CvCity::GetDefendedAgainstSpreadUntilTurn() const
{
	VALIDATE_OBJECT
	return m_iDefendedAgainstSpreadUntilTurn;
}

//	--------------------------------------------------------------------------------
void CvCity::SetDefendedAgainstSpreadUntilTurn(int iValue)
{
	VALIDATE_OBJECT
	m_iDefendedAgainstSpreadUntilTurn = iValue;
}

//	--------------------------------------------------------------------------------
int CvCity::maxHurryPopulation() const
{
	VALIDATE_OBJECT
	return (getPopulation() / 2);
}

//	--------------------------------------------------------------------------------
bool CvCity::hasActiveWorldWonder() const
{
	VALIDATE_OBJECT

	CvTeam& kTeam = GET_TEAM(getTeam());

	for(int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		const BuildingTypes eBuilding = static_cast<BuildingTypes>(iI);

		CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
		if(pkBuildingInfo)
		{
			if(isWorldWonderClass(pkBuildingInfo->GetBuildingClassInfo()))
			{
				if(m_pCityBuildings->GetNumRealBuilding(eBuilding) > 0 && !(kTeam.isObsoleteBuilding(eBuilding)))
				{
					return true;
				}
			}
		}
	}

	return false;
}

//	--------------------------------------------------------------------------------
int CvCity::getIndex() const
{
	VALIDATE_OBJECT
	return (GetID() & FLTA_INDEX_MASK);
}


//	--------------------------------------------------------------------------------
IDInfo CvCity::GetIDInfo() const
{
	VALIDATE_OBJECT
	IDInfo city(getOwner(), GetID());
	return city;
}


//	--------------------------------------------------------------------------------
void CvCity::SetID(int iID)
{
	VALIDATE_OBJECT
	m_iID = iID;
}

//	--------------------------------------------------------------------------------
CvPlot* CvCity::plot() const 
{ 
	if ((m_iX != INVALID_PLOT_COORD) && (m_iY != INVALID_PLOT_COORD))
		return GC.getMap().plotUnchecked(m_iX, m_iY); 
	return NULL;
}

//	--------------------------------------------------------------------------------
bool CvCity::at(int iX,  int iY) const
{
	VALIDATE_OBJECT
	return ((getX() == iX) && (getY() == iY));
}


//	--------------------------------------------------------------------------------
bool CvCity::at(CvPlot* pPlot) const
{
	VALIDATE_OBJECT
	return (plot() == pPlot);
}

//	--------------------------------------------------------------------------------
int CvCity::getArea() const
{
	VALIDATE_OBJECT
	return plot()->getArea();
}

//	--------------------------------------------------------------------------------
CvArea* CvCity::area() const
{
	VALIDATE_OBJECT
	return plot()->area();
}


//	--------------------------------------------------------------------------------
CvArea* CvCity::waterArea() const
{
	VALIDATE_OBJECT
	return plot()->waterArea();
}

//	--------------------------------------------------------------------------------
CvUnit* CvCity::GetGarrisonedUnit() const
{
	CvUnit* pGarrison = NULL;

	CvPlot* pPlot = plot();
	if(pPlot)
	{
		UnitHandle garrison = pPlot->getBestDefender(getOwner());
		if(garrison)
		{
			pGarrison = garrison.pointer();
		}
	}

	return pGarrison;
}

//	--------------------------------------------------------------------------------
CvPlot* CvCity::getRallyPlot() const
{
	VALIDATE_OBJECT
	if ((m_iRallyX != INVALID_PLOT_COORD) && (m_iRallyY != INVALID_PLOT_COORD))
	{
		return GC.getMap().plotUnchecked(m_iRallyX, m_iRallyY);
	}
	else
		return NULL;
}


//	--------------------------------------------------------------------------------
void CvCity::setRallyPlot(CvPlot* pPlot)
{
	VALIDATE_OBJECT
	if(getRallyPlot() != pPlot)
	{
		if(pPlot != NULL)
		{
			m_iRallyX = pPlot->getX();
			m_iRallyY = pPlot->getY();
		}
		else
		{
			m_iRallyX = INVALID_PLOT_COORD;
			m_iRallyY = INVALID_PLOT_COORD;
		}

		if(isCitySelected())
		{
			DLLUI->setDirty(ColoredPlots_DIRTY_BIT, true);
		}
	}
}


//	--------------------------------------------------------------------------------
int CvCity::getGameTurnFounded() const
{
	VALIDATE_OBJECT
	return m_iGameTurnFounded;
}


//	--------------------------------------------------------------------------------
void CvCity::setGameTurnFounded(int iNewValue)
{
	VALIDATE_OBJECT
	if(m_iGameTurnFounded != iNewValue)
	{
		m_iGameTurnFounded = iNewValue;
		CvAssert(getGameTurnFounded() >= 0);
	}
}


//	--------------------------------------------------------------------------------
int CvCity::getGameTurnAcquired() const
{
	VALIDATE_OBJECT
	return m_iGameTurnAcquired;
}


//	--------------------------------------------------------------------------------
void CvCity::setGameTurnAcquired(int iNewValue)
{
	VALIDATE_OBJECT
	m_iGameTurnAcquired = iNewValue;
	CvAssert(getGameTurnAcquired() >= 0);
}


//	--------------------------------------------------------------------------------
int CvCity::getGameTurnLastExpanded() const
{
	VALIDATE_OBJECT
	return m_iGameTurnLastExpanded;
}


//	--------------------------------------------------------------------------------
void CvCity::setGameTurnLastExpanded(int iNewValue)
{
	VALIDATE_OBJECT
	if(m_iGameTurnLastExpanded != iNewValue)
	{
		m_iGameTurnLastExpanded = iNewValue;
		CvAssert(m_iGameTurnLastExpanded >= 0);
	}
}


//	--------------------------------------------------------------------------------
#if defined(MOD_GLOBAL_CITY_AUTOMATON_WORKERS)
int CvCity::getPopulation(bool bIncludeAutomatons /* = false */) const
#else
int CvCity::getPopulation() const
#endif
{
	VALIDATE_OBJECT
#if defined(MOD_GLOBAL_CITY_AUTOMATON_WORKERS)
	return m_iPopulation + (bIncludeAutomatons ? getAutomatons() : 0);
#else
	return m_iPopulation;
#endif
}

//	---------------------------------------------------------------------------------
//	Be very careful with setting bReassignPop to false.  This assumes that the caller
//  is manually adjusting the worker assignments *and* handling the setting of
//  the CityCitizens unassigned worker value.
void CvCity::setPopulation(int iNewValue, bool bReassignPop /* = true */)
{
	VALIDATE_OBJECT
	int iOldPopulation;
	
#if defined(MOD_BUGFIX_CITY_CENTRE_WORKING)
	// To fix the "not working the centre tile" bug always call GetCityCitizens()->SetWorkingPlot(plot(), true, false); here
	GetCityCitizens()->SetWorkingPlot(plot(), true, false);
#endif

	iOldPopulation = getPopulation();
	int iPopChange = iNewValue - iOldPopulation;

	if(iOldPopulation != iNewValue)
	{
		CvPlayerAI& kPlayer = GET_PLAYER(getOwner());
		// If we are reducing population, remove the workers first
		if(bReassignPop)
		{
			if(iPopChange < 0)
			{
				// Need to Remove Citizens
				for(int iNewPopLoop = -iPopChange; iNewPopLoop--;)
				{
#if defined(MOD_GLOBAL_CITY_AUTOMATON_WORKERS)
					GetCityCitizens()->DoRemoveWorstCitizen(false, NO_SPECIALIST, iNewValue + getAutomatons());
#else
					GetCityCitizens()->DoRemoveWorstCitizen(false, NO_SPECIALIST, iNewValue);
#endif
				}

				// Fixup the unassigned workers
				int iUnassignedWorkers = GetCityCitizens()->GetNumUnassignedCitizens();
				CvAssert(iUnassignedWorkers >= -iPopChange);
				GetCityCitizens()->ChangeNumUnassignedCitizens(std::max(iPopChange, -iUnassignedWorkers));
			}
		}

		m_iPopulation = iNewValue;

		CvAssert(getPopulation() >= 0);

		kPlayer.invalidatePopulationRankCache();

		if(getPopulation() > getHighestPopulation())
		{
			setHighestPopulation(getPopulation());

			
#if defined(MOD_ROG_CORE)
			if (MOD_ROG_CORE) {
				YieldTypes eYield;
				for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
				{
					eYield = (YieldTypes)iI;
	
					int iCost = iPopChange;
					iCost *= GC.getGame().getGameSpeedInfo().getGrowthPercent();
					iCost /= 100;
					if (GetYieldFromBirth(eYield) > 0 && iCost > 0)
					{
						iCost *= GetYieldFromBirth(eYield);
						doInstantYield(eYield, iCost);
					}
				}
			}
#endif
#if defined(MOD_BELIEF_NEW_EFFECT_FOR_SP)
			if (MOD_BELIEF_NEW_EFFECT_FOR_SP && !IsResistance())
			{
				doRelogionInstantYield(GetCityReligions()->GetReligiousMajority());
				doBeliefInstantYield(GetCityReligions()->GetSecondaryReligionPantheonBelief());
			}
#endif
		}

		area()->changePopulationPerPlayer(getOwner(), (getPopulation() - iOldPopulation));
		kPlayer.changeTotalPopulation(getPopulation() - iOldPopulation);
		GET_TEAM(getTeam()).changeTotalPopulation(getPopulation() - iOldPopulation);
		GC.getGame().changeTotalPopulation(getPopulation() - iOldPopulation);

		plot()->updateYield();

		UpdateReligion(GetCityReligions()->GetReligiousMajority());

		ChangeBaseYieldRateFromMisc(YIELD_SCIENCE, (iNewValue - iOldPopulation) * GC.getSCIENCE_PER_POPULATION());

		if(iPopChange > 0)
		{
			// Give new Population something to do in the City
			if(bReassignPop)
			{
				GetCityCitizens()->ChangeNumUnassignedCitizens(iPopChange);

				// Need to Add Citizens
				for(int iNewPopLoop = 0; iNewPopLoop < iPopChange; iNewPopLoop++)
				{
					GetCityCitizens()->DoAddBestCitizenFromUnassigned();
				}
			}
		}

		setLayoutDirty(true);
		{
			auto_ptr<ICvCity1> pkDllCity(new CvDllCity(this));
			gDLL->GameplayCityPopulationChanged(pkDllCity.get(), iNewValue);
		}

		plot()->plotAction(PUF_makeInfoBarDirty);

		if((getOwner() == GC.getGame().getActivePlayer()) && isCitySelected())
		{
			DLLUI->setDirty(SelectionButtons_DIRTY_BIT, true);
			DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
		}

		// Update Unit Maintenance for the player
		kPlayer.UpdateUnitProductionMaintenanceMod();

		kPlayer.DoUpdateHappiness();

		//updateGenericBuildings();
		updateStrengthValue();

#ifdef MOD_GLOBAL_CITY_SCALES
		if (MOD_GLOBAL_CITY_SCALES)
		{
			CvCityScaleEntry* pNewScaleInfo = GC.getCityScaleInfoByPopulation(getPopulation());
			SetScale(pNewScaleInfo? (CityScaleTypes)pNewScaleInfo->GetID() : NO_CITY_SCALE);
		}
#endif

		DLLUI->setDirty(CityInfo_DIRTY_BIT, true);
	}

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(getX());
		args->Push(getY());
		args->Push(iOldPopulation);
		args->Push(iNewValue);

		bool bResult;
		LuaSupport::CallHook(pkScriptSystem, "SetPopulation", args.get(), bResult);
	}
}

//	---------------------------------------------------------------------------------
//	Be very careful with setting bReassignPop to false.  This assumes that the caller
//  is manually adjusting the worker assignments *and* handling the setting of
//  the CityCitizens unassigned worker value.
void CvCity::changePopulation(int iChange, bool bReassignPop)
{
	VALIDATE_OBJECT
	setPopulation(getPopulation() + iChange, bReassignPop);

	// Update the religious system
	GetCityReligions()->DoPopulationChange(iChange);
}


#if defined(MOD_GLOBAL_CITY_AUTOMATON_WORKERS)
//	--------------------------------------------------------------------------------
int CvCity::getAutomatons() const
{
	VALIDATE_OBJECT
	return m_iAutomatons;
}

//	---------------------------------------------------------------------------------
//	Be very careful with setting bReassignPop to false.  This assumes that the caller
//  is manually adjusting the worker assignments *and* handling the setting of
//  the CityCitizens unassigned worker value.
void CvCity::setAutomatons(int iNewValue, bool bReassignPop /* = true */)
{
	VALIDATE_OBJECT
	int iChange = iNewValue - getAutomatons();

	if (iChange != 0) {
		if (bReassignPop && iChange < 0) {
			// If we are reducing automatons, remove the workers first
			for (int iNewPopLoop = -iChange; iNewPopLoop--;) {
				GetCityCitizens()->DoRemoveWorstCitizen(false, NO_SPECIALIST, iNewValue + getPopulation());
			}

			// Fixup the unassigned workers
			int iUnassignedWorkers = GetCityCitizens()->GetNumUnassignedCitizens();
			CvAssert(iUnassignedWorkers >= -iChange);
			GetCityCitizens()->ChangeNumUnassignedCitizens(std::max(iChange, -iUnassignedWorkers));
		}

		m_iAutomatons = iNewValue;
		CvAssert(getAutomatons() >= 0);

		if (bReassignPop && iChange > 0) {
			// Give new automatons something to do in the City
			GetCityCitizens()->ChangeNumUnassignedCitizens(iChange);

			// Need to Add Citizens
			for (int iNewPopLoop = 0; iNewPopLoop < iChange; iNewPopLoop++) {
				GetCityCitizens()->DoAddBestCitizenFromUnassigned();
			}
		}

		setLayoutDirty(true);
		plot()->plotAction(PUF_makeInfoBarDirty);

		if((getOwner() == GC.getGame().getActivePlayer()) && isCitySelected())
		{
			DLLUI->setDirty(SelectionButtons_DIRTY_BIT, true);
			DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
		}

		DLLUI->setDirty(CityInfo_DIRTY_BIT, true);
	}
}

//	---------------------------------------------------------------------------------
void CvCity::changeAutomatons(int iChange, bool bReassignPop)
{
	VALIDATE_OBJECT
	setAutomatons(getAutomatons() + iChange, bReassignPop);
}
#endif


//	--------------------------------------------------------------------------------
long CvCity::getRealPopulation() const
{
	VALIDATE_OBJECT
	return (((long)(pow((double)getPopulation(), 2.8))) * 1000);
}

//	--------------------------------------------------------------------------------
int CvCity::getHighestPopulation() const
{
	VALIDATE_OBJECT
	return m_iHighestPopulation;
}


//	--------------------------------------------------------------------------------
void CvCity::setHighestPopulation(int iNewValue)
{
	VALIDATE_OBJECT
	m_iHighestPopulation = iNewValue;
	CvAssert(getHighestPopulation() >= 0);
}

//	--------------------------------------------------------------------------------
int CvCity::getNumGreatPeople() const
{
	VALIDATE_OBJECT
	return m_iNumGreatPeople;
}


//	--------------------------------------------------------------------------------
void CvCity::changeNumGreatPeople(int iChange)
{
	VALIDATE_OBJECT
	if(iChange != 0)
	{
		m_iNumGreatPeople = (m_iNumGreatPeople + iChange);
		CvAssert(getNumGreatPeople() >= 0);
	}
}


//	--------------------------------------------------------------------------------
int CvCity::getBaseGreatPeopleRate() const
{
	VALIDATE_OBJECT
	return m_iBaseGreatPeopleRate;
}


//	--------------------------------------------------------------------------------
int CvCity::getGreatPeopleRate() const
{
	VALIDATE_OBJECT
	return ((getBaseGreatPeopleRate() * getTotalGreatPeopleRateModifier()) / 100);
}


//	--------------------------------------------------------------------------------
int CvCity::getTotalGreatPeopleRateModifier() const
{
	VALIDATE_OBJECT
	int iModifier;

	iModifier = getGreatPeopleRateModifier();

	iModifier += GET_PLAYER(getOwner()).getGreatPeopleRateModifier();

	if(GET_PLAYER(getOwner()).isGoldenAge())
	{
		iModifier += GC.getGOLDEN_AGE_GREAT_PEOPLE_MODIFIER();
	}

	return std::max(0, (iModifier + 100));
}


//	--------------------------------------------------------------------------------
void CvCity::changeBaseGreatPeopleRate(int iChange)
{
	VALIDATE_OBJECT
	m_iBaseGreatPeopleRate = (m_iBaseGreatPeopleRate + iChange);
	CvAssert(getBaseGreatPeopleRate() >= 0);
}


//	--------------------------------------------------------------------------------
int CvCity::getGreatPeopleRateModifier() const
{
	VALIDATE_OBJECT
	return m_iGreatPeopleRateModifier;
}


//	--------------------------------------------------------------------------------
void CvCity::changeGreatPeopleRateModifier(int iChange)
{
	VALIDATE_OBJECT
	m_iGreatPeopleRateModifier = (m_iGreatPeopleRateModifier + iChange);
}

//	--------------------------------------------------------------------------------
/// Amount of Culture in this City
int CvCity::GetJONSCultureStored() const
{
	VALIDATE_OBJECT
	return m_iJONSCultureStored;
}

//	--------------------------------------------------------------------------------
/// Sets the amount of Culture in this City
void CvCity::SetJONSCultureStored(int iValue)
{
	VALIDATE_OBJECT
	m_iJONSCultureStored = iValue;
}

//	--------------------------------------------------------------------------------
/// Changes the amount of Culture in this City
void CvCity::ChangeJONSCultureStored(int iChange)
{
	VALIDATE_OBJECT
	SetJONSCultureStored(GetJONSCultureStored() + iChange);
}


//	--------------------------------------------------------------------------------
/// Culture level of this City
int CvCity::GetJONSCultureLevel() const
{
	VALIDATE_OBJECT
	return m_iJONSCultureLevel;
}

//	--------------------------------------------------------------------------------
/// Sets the Culture level of this City
void CvCity::SetJONSCultureLevel(int iValue)
{
	VALIDATE_OBJECT
	m_iJONSCultureLevel = iValue;
}

//	--------------------------------------------------------------------------------
/// Changes the Culture level of this City
void CvCity::ChangeJONSCultureLevel(int iChange)
{
	VALIDATE_OBJECT
	SetJONSCultureLevel(GetJONSCultureLevel() + iChange);
}

//	--------------------------------------------------------------------------------
/// What happens when you have enough Culture to acquire a new Plot?
void CvCity::DoJONSCultureLevelIncrease()
{
	VALIDATE_OBJECT

	int iOverflow = GetJONSCultureStored() - GetJONSCultureThreshold();
#if defined(MOD_UI_CITY_EXPANSION)
	bool bIsHumanControlled = (GET_PLAYER(getOwner()).isHuman() && !IsPuppet());
	bool bSendEvent = true;
	if (!(MOD_UI_CITY_EXPANSION && bIsHumanControlled)) {
		// We need to defer this for humans picking their own tiles
#endif
		SetJONSCultureStored(iOverflow);
		ChangeJONSCultureLevel(1);
#if defined(MOD_UI_CITY_EXPANSION)
	}
#endif

	CvPlot* pPlotToAcquire = GetNextBuyablePlot();

	// maybe the player owns ALL of the plots or there are none avaialable?
	if(pPlotToAcquire)
	{
#if defined(MOD_UI_CITY_EXPANSION)
		// For human players, let them decide which plot to acquire
		if (MOD_UI_CITY_EXPANSION && bIsHumanControlled) {
			// Yep CITY_PLOTS_RADIUS is a #define and not taken from the database - well done Firaxis!
#if defined(MOD_GLOBAL_CITY_WORKING)
			bool bCanAcquirePlot = plotDistance(getX(), getY(), pPlotToAcquire->getX(), pPlotToAcquire->getY()) <= getWorkPlotDistance();
#else
			bool bCanAcquirePlot = plotDistance(getX(), getY(), pPlotToAcquire->getX(), pPlotToAcquire->getY()) <= CITY_PLOTS_RADIUS);
#endif
			if (bCanAcquirePlot && GetBuyPlotCost(pPlotToAcquire->getX(), pPlotToAcquire->getY()) < 1) {
				// Within working/buying distance
				bSendEvent = false;

				CvNotifications* pNotifications = GET_PLAYER(getOwner()).GetNotifications();
				if (pNotifications) {
					Localization::String localizedText = Localization::Lookup("TXT_KEY_NOTIFICATION_CITY_CULTURE_ACQUIRED_NEW_PLOT");
					localizedText << getNameKey();
					Localization::String localizedSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_CITY_CULTURE_ACQUIRED_NEW_PLOT");
					localizedSummary << getNameKey();
					pNotifications->Add(NOTIFICATION_CITY_TILE, localizedText.toUTF8(), localizedSummary.toUTF8(), getX(), getY(), GetID());
				}
			} else {
				// The cheapest plot we can have is outside our working/buying distance, so just acquire it
				DoAcquirePlot(pPlotToAcquire->getX(), pPlotToAcquire->getY());
				// and also the deferred stuff
				SetJONSCultureStored(iOverflow);
				ChangeJONSCultureLevel(1);
			}
		} else {
			// AI or dis-interested human, just acquire the plot normally
#endif
		if(GC.getLogging() && GC.getAILogging())
		{
			CvPlayerAI& kOwner = GET_PLAYER(getOwner());
			CvString playerName;
			FILogFile* pLog;
			CvString strBaseString;
			CvString strOutBuf;
			playerName = kOwner.getCivilizationShortDescription();
			pLog = LOGFILEMGR.GetLog(kOwner.GetCitySpecializationAI()->GetLogFileName(playerName), FILogFile::kDontTimeStamp);
			strBaseString.Format("%03d, ", GC.getGame().getElapsedGameTurns());
			strBaseString += playerName + ", ";
			strOutBuf.Format("%s, City Culture Leveled Up. Level: %d Border Expanded, X: %d, Y: %d", getName().GetCString(), 
												GetJONSCultureLevel(), pPlotToAcquire->getX(), pPlotToAcquire->getY());
			strBaseString += strOutBuf;
			pLog->Msg(strBaseString);
		}
		DoAcquirePlot(pPlotToAcquire->getX(), pPlotToAcquire->getY());
#if defined(MOD_UI_CITY_EXPANSION)
		}
#endif

#if defined(MOD_UI_CITY_EXPANSION)
		// If the human is picking their own tile, the event will be sent when the tile is "bought"
		if (bSendEvent) {
#endif
#if defined(MOD_EVENTS_CITY)
			if (MOD_EVENTS_CITY) {
				GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityBoughtPlot, getOwner(), GetID(), pPlotToAcquire->getX(), pPlotToAcquire->getY(), false, true);
			} else {
#endif
		ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
		if (pkScriptSystem) 
		{
			CvLuaArgsHandle args;
			args->Push(getOwner());
			args->Push(GetID());
			args->Push(pPlotToAcquire->getX());
			args->Push(pPlotToAcquire->getY());
			args->Push(false); // bGold
			args->Push(true); // bFaith/bCulture

			bool bResult;
			LuaSupport::CallHook(pkScriptSystem, "CityBoughtPlot", args.get(), bResult);
		}
#if defined(MOD_EVENTS_CITY)
			}
#endif
#if defined(MOD_UI_CITY_EXPANSION)
		}
#endif

#if defined(MOD_ROG_CORE)
		if (MOD_ROG_CORE) {
			YieldTypes eYield;
			for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
			{
				eYield = (YieldTypes)iI;
				int iCost = GC.getGame().getGameSpeedInfo().getCulturePercent();
				iCost /= 100;
				if (GetYieldFromBorderGrowth(eYield) > 0)
				{
					iCost *= GetYieldFromBorderGrowth(eYield);
					doInstantYield(eYield, iCost);
				}
			}
		}
#endif

#if defined(MOD_UI_CITY_EXPANSION)
	} else if (MOD_UI_CITY_EXPANSION && bIsHumanControlled) {
		// Do the stuff we deferred as we though we'd do it when the human bought a tile but can't as there are no tiles to buy!
		SetJONSCultureStored(iOverflow);
		ChangeJONSCultureLevel(1);
#endif
	}
}

//	--------------------------------------------------------------------------------
/// Amount of Culture needed in this City to acquire a new Plot
int CvCity::GetJONSCultureThreshold() const
{
	VALIDATE_OBJECT
	int iCultureThreshold = /*15*/ GC.getCULTURE_COST_FIRST_PLOT();

	float fExponent = /*1.1f*/ GC.getCULTURE_COST_LATER_PLOT_EXPONENT();

	int iPolicyExponentMod = GET_PLAYER(m_eOwner).GetPlotCultureExponentModifier();
	if(iPolicyExponentMod != 0)
	{
		fExponent = fExponent * (float)((100 + iPolicyExponentMod));
		fExponent /= 100.0f;
	}

	int iAdditionalCost = GetJONSCultureLevel() * /*8*/ GC.getCULTURE_COST_LATER_PLOT_MULTIPLIER();
	iAdditionalCost = (int) pow((double) iAdditionalCost, (double)fExponent);

	iCultureThreshold += iAdditionalCost;

	// More expensive for Minors to claim territory
	if(GET_PLAYER(getOwner()).isMinorCiv())
	{
		iCultureThreshold *= /*150*/ GC.getMINOR_CIV_PLOT_CULTURE_COST_MULTIPLIER();
		iCultureThreshold /= 100;
	}

	// Religion modifier
	int iReligionMod = 0;
	ReligionTypes eMajority = GetCityReligions()->GetReligiousMajority();
	if(eMajority != NO_RELIGION)
	{
		const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eMajority, getOwner());
		if(pReligion)
		{
			iReligionMod = pReligion->m_Beliefs.GetPlotCultureCostModifier();
			BeliefTypes eSecondaryPantheon = GetCityReligions()->GetSecondaryReligionPantheonBelief();
			if (eSecondaryPantheon != NO_BELIEF)
			{
				iReligionMod += GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon)->GetPlotCultureCostModifier();
			}
		}
	}

	// -50 = 50% cost
	int iModifier = GET_PLAYER(getOwner()).GetPlotCultureCostModifier() + m_iPlotCultureCostModifier + iReligionMod;
	if(iModifier != 0)
	{
		iModifier = max(iModifier, /*-85*/ GC.getCULTURE_PLOT_COST_MOD_MINIMUM());	// value cannot reduced by more than 85%
		iCultureThreshold *= (100 + iModifier);
		iCultureThreshold /= 100;
	}

	// Game Speed Mod
	iCultureThreshold *= GC.getGame().getGameSpeedInfo().getCulturePercent();
	iCultureThreshold /= 100;

	// Make the number not be funky
	int iDivisor = /*5*/ GC.getCULTURE_COST_VISIBLE_DIVISOR();
	if(iCultureThreshold > iDivisor * 2)
	{
		iCultureThreshold /= iDivisor;
		iCultureThreshold *= iDivisor;
	}

	return iCultureThreshold;
}


//	--------------------------------------------------------------------------------
int CvCity::getJONSCulturePerTurn(bool bStatic) const
{
	VALIDATE_OBJECT

	// No culture during Resistance
	if(IsResistance() || IsRazing())
	{
		return 0;
	}
	if (bStatic)
	{
		return (GetStaticYield(YIELD_CULTURE) / 100);
	}

	int iCulture = GetBaseJONSCulturePerTurn();

	int iModifier = 100;

	// City modifier
#if defined(MOD_API_UNIFIED_YIELDS)
	// getCultureRateModifier() is just the culture specific building modifiers
	// we want getBaseYieldRateModifier(YIELD_CULTURE) as well
	iModifier = getBaseYieldRateModifier(YIELD_CULTURE, getCultureRateModifier());
#else
	iModifier += getCultureRateModifier();
#endif

	// Player modifier
	iModifier += GET_PLAYER(getOwner()).GetJONSCultureCityModifier();

	// Wonder here?
	if(getNumWorldWonders() > 0)
		iModifier += GET_PLAYER(getOwner()).GetCultureWonderMultiplier();

	// Puppet?
	if(IsPuppet())
	{
		iModifier += GC.getPUPPET_CULTURE_MODIFIER();
	}

	iCulture *= iModifier;
	iCulture /= 100;

	return iCulture;
}

//	--------------------------------------------------------------------------------
int CvCity::GetBaseJONSCulturePerTurn() const
{
	VALIDATE_OBJECT

	int iCulturePerTurn = 0;
	iCulturePerTurn += GetJONSCulturePerTurnFromBuildings();
	iCulturePerTurn += GetJONSCulturePerTurnFromPolicies();
	iCulturePerTurn += GetJONSCulturePerTurnFromSpecialists();

#if defined(MOD_API_UNIFIED_YIELDS)
	// GetJONSCulturePerTurnFromSpecialists() uses the Buildings.SpecialistExtraCulture column,
	// GetBaseYieldRateFromSpecialists(YIELD_CULTURE) gets everything else!
	iCulturePerTurn += GetBaseYieldRateFromSpecialists(YIELD_CULTURE);
	iCulturePerTurn += GetBaseYieldRateFromProjects(YIELD_CULTURE);
#endif
	iCulturePerTurn += (GetYieldPerPopTimes100(YIELD_CULTURE) * getPopulation()) / 100;
	iCulturePerTurn += GET_PLAYER(getOwner()).GetYieldPerPopChange(YIELD_CULTURE) * getPopulation() / 100;
	iCulturePerTurn += (GetYieldPerPopInEmpireTimes100(YIELD_CULTURE) * GET_PLAYER(getOwner()).getTotalPopulation()) / 100;
	iCulturePerTurn += (GetYieldPerReligionTimes100(YIELD_CULTURE) * GetCityReligions()->GetNumReligionsWithFollowers()) /100;
	iCulturePerTurn += (GetYieldPerEra(YIELD_CULTURE) * (GET_PLAYER(getOwner()).GetCurrentEra() + 1));
#if defined(MOD_ROG_CORE)
	iCulturePerTurn += GetBaseYieldRateFromCSAlliance(YIELD_CULTURE);
	iCulturePerTurn += GetBaseYieldRateFromCSFriendship(YIELD_CULTURE);
	iCulturePerTurn += GetBaseYieldRateFromEspionageSpy(YIELD_CULTURE);
#endif

#if defined(MOD_API_UNIFIED_YIELDS)
	if (IsRouteToCapitalConnected())
	{
		iCulturePerTurn += GET_PLAYER(getOwner()).GetYieldChangeTradeRoute(YIELD_CULTURE);
		iCulturePerTurn += GET_PLAYER(getOwner()).GetPlayerTraits()->GetYieldChangeTradeRoute(YIELD_CULTURE);
	}
#endif

#if defined(MOD_GLOBAL_GREATWORK_YIELDTYPES) || defined(MOD_API_UNIFIED_YIELDS)
	iCulturePerTurn += GetBaseYieldRateFromGreatWorks(YIELD_CULTURE);
#else
	iCulturePerTurn += GetJONSCulturePerTurnFromGreatWorks();
#endif
	iCulturePerTurn += GetBaseYieldRateFromTerrain(YIELD_CULTURE);
#if defined(MOD_API_UNIFIED_YIELDS)
	iCulturePerTurn += GetYieldPerTurnFromUnimprovedFeatures(YIELD_CULTURE);
#endif
	iCulturePerTurn += GetYieldPerTurnFromAdjacentFeatures(YIELD_CULTURE);
	iCulturePerTurn += GetJONSCulturePerTurnFromTraits();
	iCulturePerTurn += GetBaseYieldRateFromReligion(YIELD_CULTURE);
	iCulturePerTurn += GetJONSCulturePerTurnFromLeagues();

#if defined(MOD_API_UNIFIED_YIELDS)
	// Process production into culture
	if (getProductionToYieldModifier(YIELD_CULTURE) != 0)
	{
		iCulturePerTurn += (getBasicYieldRateTimes100(YIELD_PRODUCTION, false, true) / 100) * (getProductionToYieldModifier(YIELD_CULTURE) + GetYieldFromProcessModifier(YIELD_CULTURE) + GET_PLAYER(getOwner()).GetYieldFromProcessModifierGlobal(YIELD_CULTURE)) / 100;
	}
	//iCulturePerTurn += (getBasicYieldRateTimes100(YIELD_PRODUCTION, false, true) / 100) * (getProductionToYieldModifier(YIELD_CULTURE)) / 100;

	// Culture from having trade routes
	iCulturePerTurn += GET_PLAYER(m_eOwner).GetTrade()->GetTradeValuesAtCityTimes100(this, YIELD_CULTURE) / 100;
#endif

#ifdef MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD
	if (MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD)
		iCulturePerTurn += GetBaseYieldRateFromOtherYield(YIELD_CULTURE);
#endif

	//Update Yields from yields ... need to sidestep constness
	CvCity* pThisCity = const_cast<CvCity*>(this);
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		YieldTypes eIndex2 = (YieldTypes)iI;
		if (eIndex2 == NO_YIELD)
			continue;
		if (YIELD_CULTURE == eIndex2)
			continue;

		pThisCity->UpdateCityYieldFromYield(YIELD_CULTURE, eIndex2, iCulturePerTurn);

		//NOTE! We flip it here, because we want the OUT yield
		iCulturePerTurn += GetRealYieldFromYield(eIndex2, YIELD_CULTURE);
	}

	if (MOD_API_UNIFIED_YIELDS_MORE)
	{
		iCulturePerTurn += GetYieldFromHealth(YIELD_CULTURE);
		iCulturePerTurn += GetYieldFromHappiness(YIELD_CULTURE);
		iCulturePerTurn += GetYieldFromCrime(YIELD_CULTURE);
	}

#if defined(MOD_NUCLEAR_WINTER_FOR_SP)
	if(!IsNoNuclearWinterLocal()) iCulturePerTurn += GC.getGame().GetYieldFromNuclearWinter(YIELD_CULTURE);
#endif

	return iCulturePerTurn;
}

//	--------------------------------------------------------------------------------
int CvCity::GetJONSCulturePerTurnFromBuildings() const
{
	VALIDATE_OBJECT
	return GetBaseYieldRateFromBuildings(YIELD_CULTURE);
}


//	--------------------------------------------------------------------------------
int CvCity::GetJONSCulturePerTurnFromPolicies() const
{
	VALIDATE_OBJECT

	int iNonSpecialist = GET_PLAYER(m_eOwner).getYieldFromNonSpecialistCitizens(YIELD_CULTURE);
	int iValue = 0;
	if (iNonSpecialist != 0)
	{
		int iBonusTimes100 = (iNonSpecialist * (getPopulation() - GetCityCitizens()->GetTotalSpecialistCount()));
		iBonusTimes100 /= 100;
		iValue += iBonusTimes100;
	}

	return m_iJONSCulturePerTurnFromPolicies + GetBaseYieldRateFromBuildingsPolicies(YIELD_CULTURE) + iValue;
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeJONSCulturePerTurnFromPolicies(int iChange)
{
	VALIDATE_OBJECT
	if(iChange != 0)
	{
		m_iJONSCulturePerTurnFromPolicies += iChange;
		UpdateCityYields(YIELD_CULTURE);
	}
}

//	--------------------------------------------------------------------------------
int CvCity::GetJONSCulturePerTurnFromSpecialists() const
{
	VALIDATE_OBJECT
	return m_iJONSCulturePerTurnFromSpecialists;
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeJONSCulturePerTurnFromSpecialists(int iChange)
{
	VALIDATE_OBJECT
	if(iChange != 0)
	{
		m_iJONSCulturePerTurnFromSpecialists = (m_iJONSCulturePerTurnFromSpecialists + iChange);
	}
}

//	--------------------------------------------------------------------------------
int CvCity::GetJONSCulturePerTurnFromGreatWorks() const
{
#if defined(MOD_GLOBAL_GREATWORK_YIELDTYPES) || defined(MOD_API_UNIFIED_YIELDS)
	return GetCityBuildings()->GetYieldFromGreatWorks(YIELD_CULTURE);
#else
	return GetCityBuildings()->GetCultureFromGreatWorks();
#endif
}

//	--------------------------------------------------------------------------------
int CvCity::GetJONSCulturePerTurnFromTraits() const
{
	VALIDATE_OBJECT
	return GET_PLAYER(m_eOwner).GetPlayerTraits()->GetCityCultureBonus();
}

//	--------------------------------------------------------------------------------
int CvCity::GetJONSCulturePerTurnFromReligion() const
{
	VALIDATE_OBJECT
	return GetBaseYieldRateFromReligion(YIELD_CULTURE);
}

//	--------------------------------------------------------------------------------
int CvCity::GetJONSCulturePerTurnFromLeagues() const
{
	VALIDATE_OBJECT
	int iValue = 0;

	iValue += (getNumWorldWonders() * GC.getGame().GetGameLeagues()->GetWorldWonderYieldChange(getOwner(), YIELD_CULTURE));

	return iValue;
}

//	--------------------------------------------------------------------------------
int CvCity::GetFaithPerTurn(bool bStatic) const
{
	VALIDATE_OBJECT

	// No faith during Resistance
	if(IsResistance() || IsRazing())
	{
		return 0;
	}

	if (bStatic)
	{
		return (GetStaticYield(YIELD_FAITH) / 100);
	}

	int iFaith = GetFaithPerTurnFromBuildings();
	iFaith += GetBaseYieldRateFromProjects(YIELD_FAITH);
#if defined(MOD_API_UNIFIED_YIELDS)
	iFaith += GetBaseYieldRateFromSpecialists(YIELD_FAITH);
#endif
	iFaith += (GetYieldPerPopTimes100(YIELD_FAITH) * getPopulation()) / 100;
	iFaith += GET_PLAYER(getOwner()).GetYieldPerPopChange(YIELD_FAITH)* getPopulation() / 100;
	iFaith += (GetYieldPerPopInEmpireTimes100(YIELD_FAITH) * GET_PLAYER(getOwner()).getTotalPopulation()) / 100;
	iFaith += (GetYieldPerReligionTimes100(YIELD_FAITH) * GetCityReligions()->GetNumReligionsWithFollowers()) /100;
	iFaith += (GetYieldPerEra(YIELD_FAITH) * (GET_PLAYER(getOwner()).GetCurrentEra() + 1));
#if defined(MOD_ROG_CORE)
	iFaith += GetBaseYieldRateFromCSAlliance(YIELD_FAITH);
	iFaith += GetBaseYieldRateFromCSFriendship(YIELD_FAITH);
	iFaith += GetBaseYieldRateFromEspionageSpy(YIELD_FAITH);
#endif

	//Update Yields from yields ... need to sidestep constness
	CvCity* pThisCity = const_cast<CvCity*>(this);
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		YieldTypes eIndex2 = (YieldTypes)iI;
		if (eIndex2 == NO_YIELD)
			continue;
		if (YIELD_FAITH == eIndex2)
			continue;

		pThisCity->UpdateCityYieldFromYield(YIELD_FAITH, eIndex2, iFaith);

		//NOTE! We flip it here, because we want the OUT yield
		iFaith += GetRealYieldFromYield(eIndex2, YIELD_FAITH);
	}

#if defined(MOD_API_UNIFIED_YIELDS)
	if (IsRouteToCapitalConnected())
	{
		iFaith += GET_PLAYER(getOwner()).GetYieldChangeTradeRoute(YIELD_FAITH);
		iFaith += GET_PLAYER(getOwner()).GetPlayerTraits()->GetYieldChangeTradeRoute(YIELD_FAITH);
	}
#endif


	if (MOD_API_UNIFIED_YIELDS_MORE)
	{
		iFaith += GetYieldFromHappiness(YIELD_FAITH);
		iFaith += GetYieldFromHealth(YIELD_FAITH);
		iFaith += GetYieldFromCrime(YIELD_FAITH);
	}

#if defined(MOD_NUCLEAR_WINTER_FOR_SP)
	if(!IsNoNuclearWinterLocal()) iFaith += GC.getGame().GetYieldFromNuclearWinter(YIELD_FAITH);
#endif

#if defined(MOD_GLOBAL_GREATWORK_YIELDTYPES) || defined(MOD_API_UNIFIED_YIELDS)
	iFaith += GetBaseYieldRateFromGreatWorks(YIELD_FAITH);
#endif
	iFaith += GetBaseYieldRateFromTerrain(YIELD_FAITH);
	iFaith += GetFaithPerTurnFromPolicies();
#if defined(MOD_API_UNIFIED_YIELDS)
	iFaith += GetYieldPerTurnFromUnimprovedFeatures(YIELD_FAITH);
#else
	iFaith += GetFaithPerTurnFromTraits();
#endif
	iFaith += GetYieldPerTurnFromAdjacentFeatures(YIELD_FAITH);
	iFaith += GetFaithPerTurnFromReligion();

#if defined(MOD_API_UNIFIED_YIELDS)
	// Process production into faith
	if (getProductionToYieldModifier(YIELD_FAITH) != 0)
	{
		iFaith += (getBasicYieldRateTimes100(YIELD_PRODUCTION, false, false) / 100) * (getProductionToYieldModifier(YIELD_FAITH) + GetYieldFromProcessModifier(YIELD_FAITH) + GET_PLAYER(getOwner()).GetYieldFromProcessModifierGlobal(YIELD_FAITH)) / 100;
	}

	// Faith from having trade routes
	iFaith += GET_PLAYER(m_eOwner).GetTrade()->GetTradeValuesAtCityTimes100(this, YIELD_FAITH) / 100;
#endif

#if defined(MOD_API_UNIFIED_YIELDS)
	int iModifier = 100;

	// City modifier
	iModifier = getBaseYieldRateModifier(YIELD_FAITH);

	// Puppet?
	if(IsPuppet())
	{
		iModifier += GC.getPUPPET_FAITH_MODIFIER();
	}

	iFaith *= iModifier;
	iFaith /= 100;
#else
	// Puppet?
	int iModifier = 0;
	if(IsPuppet())
	{
		iModifier = GC.getPUPPET_FAITH_MODIFIER();
		iFaith *= (100 + iModifier);
		iFaith /= 100;
	}
#endif

	return iFaith;
}

//	--------------------------------------------------------------------------------
int CvCity::GetFaithPerTurnFromBuildings() const
{
	VALIDATE_OBJECT
	return GetBaseYieldRateFromBuildings(YIELD_FAITH);
}



//	--------------------------------------------------------------------------------
int CvCity::GetFaithPerTurnFromPolicies() const
{
	VALIDATE_OBJECT
	int iNonSpecialist = GET_PLAYER(m_eOwner).getYieldFromNonSpecialistCitizens(YIELD_FAITH);
	int iValue = 0;
	if (iNonSpecialist != 0)
	{
		int iBonusTimes100 = (iNonSpecialist * (getPopulation() - GetCityCitizens()->GetTotalSpecialistCount()));
		iBonusTimes100 /= 100;
		iValue += iBonusTimes100;
	}
	return GetBaseYieldRateFromBuildingsPolicies(YIELD_FAITH) + iValue;
}

//	--------------------------------------------------------------------------------
#if defined(MOD_API_UNIFIED_YIELDS)
int CvCity::GetYieldPerTurnFromUnimprovedFeatures(YieldTypes eYield) const
{
	VALIDATE_OBJECT
	int iYield = 0;

	CvPlayer& kPlayer = GET_PLAYER(getOwner());
	ReligionTypes eMajority = GetCityReligions()->GetReligiousMajority();
	const CvReligion* pReligion = (eMajority == NO_RELIGION) ? NULL : GC.getGame().GetGameReligions()->GetReligion(eMajority, getOwner());
	BeliefTypes eSecondaryPantheon = GetCityReligions()->GetSecondaryReligionPantheonBelief();

	for (int iI = 0; iI < GC.getNumFeatureInfos(); iI++)
	{
		FeatureTypes eFeature = (FeatureTypes) iI;

		if (!GC.getFeatureInfo(eFeature)->IsNaturalWonder(true))
		{
			int iBaseYield = kPlayer.getCityYieldFromUnimprovedFeature(eFeature, eYield);
			iBaseYield += kPlayer.GetPlayerTraits()->GetCityYieldFromUnimprovedFeature(eFeature, eYield);
		
			if(pReligion)
			{
				iBaseYield += pReligion->m_Beliefs.GetCityYieldFromUnimprovedFeature(eFeature, eYield);
				if (eSecondaryPantheon != NO_BELIEF)
				{
					iBaseYield += GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon)->GetCityYieldFromUnimprovedFeature(eFeature, eYield);
				}
			}

			if (eYield == YIELD_FAITH && eFeature == FEATURE_FOREST && kPlayer.GetPlayerTraits()->IsFaithFromUnimprovedForest())
			{
				++iBaseYield;
			}
		
			if (iBaseYield > 0)
			{
				int iAdjacentFeatures = 0;

				for (int iDirectionLoop = 0; iDirectionLoop < NUM_DIRECTION_TYPES; ++iDirectionLoop)
				{
					CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), ((DirectionTypes)iDirectionLoop));
					if (pAdjacentPlot && pAdjacentPlot->getFeatureType() == eFeature && pAdjacentPlot->getImprovementType() == NO_IMPROVEMENT)
					{
						iAdjacentFeatures++;
					}
				}

				if (iAdjacentFeatures > 2)
				{
					iYield += iBaseYield * 2;
				}
				else if (iAdjacentFeatures > 0)
				{
					iYield += iBaseYield;
				}
			}
		}
	}
	
	return iYield;
}
#else
int CvCity::GetFaithPerTurnFromTraits() const
{
	VALIDATE_OBJECT

	int iRtnValue = 0;

	if(GET_PLAYER(m_eOwner).GetPlayerTraits()->IsFaithFromUnimprovedForest())
	{
		// See how many tiles adjacent to city are unimproved forest
		int iAdjacentForests = 0;

		for(int iDirectionLoop = 0; iDirectionLoop < NUM_DIRECTION_TYPES; ++iDirectionLoop)
		{
			CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), ((DirectionTypes)iDirectionLoop));
			if(pAdjacentPlot != NULL)
			{
				if(pAdjacentPlot->getFeatureType() == FEATURE_FOREST && pAdjacentPlot->getImprovementType() == NO_IMPROVEMENT)
				{
					iAdjacentForests++;
				}
			}
		}

		// If 3 or more, bonus is +2
		if(iAdjacentForests > 2)
		{
			iRtnValue = 2;
		}
		else if(iAdjacentForests > 0)
		{
			iRtnValue = 1;
		}
	}

	return iRtnValue;
}
#endif


//	--------------------------------------------------------------------------------
int CvCity::GetYieldPerTurnFromAdjacentFeatures(YieldTypes eYield) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eYield >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eYield< NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	CvPlayerTraits* pTraits = GET_PLAYER(m_eOwner).GetPlayerTraits();
	if(!pTraits->IsHasCityYieldPerAdjacentFeature()) return 0;

	int iRtnValue = 0;
	CvPlot* pAdjacentPlot = NULL;
	int iMaxValue = 0;
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		pAdjacentPlot = plotDirection(getX(), getY(), ((DirectionTypes)iI));
		if (!pAdjacentPlot) continue;
		FeatureTypes eFeature = pAdjacentPlot->getFeatureType();
		if (eFeature == NO_FEATURE) continue;
		int iTempMax = pTraits->GetCityYieldPerAdjacentFeature(eFeature, eYield);
		if(iTempMax <= 0) continue;
		iRtnValue++;
		iMaxValue = iTempMax > iMaxValue ? iTempMax : iMaxValue;
	}
	iRtnValue = iRtnValue > iMaxValue ? iMaxValue : iRtnValue;
	return iRtnValue;
}


//	--------------------------------------------------------------------------------
int CvCity::getSpecialistExtraYield(SpecialistTypes eIndex1, YieldTypes eIndex2) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex1 > -1 && eIndex1 < GC.getNumSpecialistInfos(), "eIndex1 expected to be < GC.getNumSpecialistInfos()");
	CvAssertMsg(eIndex2 > -1 && eIndex2 < NUM_YIELD_TYPES, "eIndex2 expected to be < NUM_YIELD_TYPES");
	return ModifierLookup(m_yieldChanges[eIndex2].forSpecialist, eIndex1);
}

//	--------------------------------------------------------------------------------
void CvCity::changeSpecialistExtraYield(SpecialistTypes eIndex1, YieldTypes eIndex2, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex1 > -1 && eIndex1 < GC.getNumSpecialistInfos(), "eIndex1 expected to be < GC.getNumSpecialistInfos()");
	CvAssertMsg(eIndex2 > -1 && eIndex2 < NUM_YIELD_TYPES, "Invalid yield index.");
	SCityExtraYields& y = m_yieldChanges[eIndex2];
	if (ModifierUpdateInsertRemove(y.forSpecialist, eIndex1, iChange, true))
		updateExtraSpecialistYield();
}





//	--------------------------------------------------------------------------------
int CvCity::GetFaithPerTurnFromReligion() const
{
	VALIDATE_OBJECT
	return GetBaseYieldRateFromReligion(YIELD_FAITH);
}

//	--------------------------------------------------------------------------------
int CvCity::getCultureRateModifier() const
{
	VALIDATE_OBJECT
	return m_iCultureRateModifier;
}

//	--------------------------------------------------------------------------------
void CvCity::changeCultureRateModifier(int iChange)
{
	VALIDATE_OBJECT
	if(iChange != 0)
	{
		m_iCultureRateModifier = (m_iCultureRateModifier + iChange);
	}
}

//	--------------------------------------------------------------------------------
int CvCity::GetBaseTourism() const
{
	VALIDATE_OBJECT
	return m_iBaseTourism;
}

//	--------------------------------------------------------------------------------
void CvCity::SetBaseTourism(int iValue)
{
	VALIDATE_OBJECT
	m_iBaseTourism = iValue;
}
//	--------------------------------------------------------------------------------
int CvCity::GetBaseTourismBeforeModifiers() const
{
	VALIDATE_OBJECT
	return m_iBaseTourismBeforeModifiers;
}

//	--------------------------------------------------------------------------------
void CvCity::SetBaseTourismBeforeModifiers(int iValue)
{
	VALIDATE_OBJECT
	m_iBaseTourismBeforeModifiers = iValue;
}


#if defined(MOD_API_EXTENSIONS)
//	--------------------------------------------------------------------------------
int CvCity::getTourismRateModifier() const
{
	VALIDATE_OBJECT
	return GetCityBuildings()->GetGreatWorksTourismModifier();
}

//	--------------------------------------------------------------------------------
void CvCity::changeTourismRateModifier(int iChange)
{
	VALIDATE_OBJECT
	if(iChange != 0)
	{
		GetCityBuildings()->ChangeGreatWorksTourismModifier(iChange);
	}
}
#endif

//	--------------------------------------------------------------------------------
int CvCity::getNumWorldWonders() const
{
	VALIDATE_OBJECT
	return m_iNumWorldWonders;
}


//	--------------------------------------------------------------------------------
void CvCity::changeNumWorldWonders(int iChange)
{
	VALIDATE_OBJECT
	if(iChange != 0)
	{
		m_iNumWorldWonders = (m_iNumWorldWonders + iChange);
		CvAssert(getNumWorldWonders() >= 0);

		// Extra culture for Wonders (Policies, etc.)
		ChangeJONSCulturePerTurnFromPolicies(GET_PLAYER(getOwner()).GetCulturePerWonder() * iChange);
	}
}


//	--------------------------------------------------------------------------------
int CvCity::getNumTeamWonders() const
{
	VALIDATE_OBJECT
	return m_iNumTeamWonders;
}


//	--------------------------------------------------------------------------------
void CvCity::changeNumTeamWonders(int iChange)
{
	VALIDATE_OBJECT
	m_iNumTeamWonders = (m_iNumTeamWonders + iChange);
	CvAssert(getNumTeamWonders() >= 0);
}


//	--------------------------------------------------------------------------------
int CvCity::getNumNationalWonders() const
{
	VALIDATE_OBJECT
	return m_iNumNationalWonders;
}


//	--------------------------------------------------------------------------------
void CvCity::changeNumNationalWonders(int iChange)
{
	VALIDATE_OBJECT
	m_iNumNationalWonders = (m_iNumNationalWonders + iChange);
	CvAssert(getNumNationalWonders() >= 0);
}


//	--------------------------------------------------------------------------------
int CvCity::GetWonderProductionModifier() const
{
	VALIDATE_OBJECT
	return m_iWonderProductionModifier;
}


//	--------------------------------------------------------------------------------
void CvCity::ChangeWonderProductionModifier(int iChange)
{
	VALIDATE_OBJECT
	m_iWonderProductionModifier = (m_iWonderProductionModifier + iChange);
	CvAssert(GetWonderProductionModifier() >= 0);
}

//	--------------------------------------------------------------------------------
int CvCity::GetLocalResourceWonderProductionMod(BuildingTypes eBuilding, CvString* toolTipSink) const
{
	VALIDATE_OBJECT

	int iMultiplier = 0;

	CvAssertMsg(eBuilding > -1 && eBuilding < GC.getNumBuildingInfos(), "Invalid building index. Please show Jon.");
	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if(pkBuildingInfo)
	{
		// Is this even a wonder?
		const CvBuildingClassInfo& kBuildingClassInfo = pkBuildingInfo->GetBuildingClassInfo();
		if(!::isWorldWonderClass(kBuildingClassInfo) &&
		        !::isTeamWonderClass(kBuildingClassInfo) &&
		        !::isNationalWonderClass(kBuildingClassInfo))
		{
			return 0;
		}
		
#if defined(MOD_TRADE_WONDER_RESOURCE_ROUTES)
		// Are we using a trade route to ship the wonder resource from/to this city?
		bool bWonderResourceIn = false;
		bool bWonderResourceOut = false;
		if (MOD_TRADE_WONDER_RESOURCE_ROUTES) {
			CvGameTrade* pGameTrade = GC.getGame().GetGameTrade();
			for (uint ui = 0; ui < pGameTrade->m_aTradeConnections.size(); ui++)
			{
				if (pGameTrade->IsTradeRouteIndexEmpty(ui))
				{
					continue;
				}

				if (pGameTrade->m_aTradeConnections[ui].m_eConnectionType == TRADE_CONNECTION_WONDER_RESOURCE)
				{
					CvCity* pOriginCity = CvGameTrade::GetOriginCity(pGameTrade->m_aTradeConnections[ui]);
					CvCity* pDestCity = CvGameTrade::GetDestCity(pGameTrade->m_aTradeConnections[ui]);

					if (pDestCity->getX() == getX() && pDestCity->getY() == getY())
					{
						ResourceTypes eWonderResource = ::getWonderResource();
						bWonderResourceIn = (eWonderResource != NO_RESOURCE && pOriginCity->GetNumResourceLocal(eWonderResource) > 0);
					}
					else
					{
						if (pOriginCity->getX() == getX() && pOriginCity->getY() == getY())
						{
							bWonderResourceOut = true;
						}
					}

					break;
				}
			}
			// if (bWonderResourceIn) CUSTOMLOG("Shipping a wonder resource into %s", getName().c_str());
			// if (bWonderResourceOut) CUSTOMLOG("Shipping a wonder resource out of %s", getName().c_str());
		}
#endif

		// Resource wonder bonus
		for(int iResourceLoop = 0; iResourceLoop < GC.getNumResourceInfos(); iResourceLoop++)
		{
			const ResourceTypes eResource = static_cast<ResourceTypes>(iResourceLoop);
			CvResourceInfo* pkResource = GC.getResourceInfo(eResource);
			if(pkResource)
			{
				int iBonus = pkResource->getWonderProductionMod();
				if(iBonus != 0)
				{
#if defined(MOD_TRADE_WONDER_RESOURCE_ROUTES)
					bool bHasLocalResource = IsHasResourceLocal(eResource, /*bTestVisible*/ false);
					if (MOD_TRADE_WONDER_RESOURCE_ROUTES) {
						// We're shipping one in, or we're not shipping our only one out, or we have more than one
						bHasLocalResource = bWonderResourceIn || (m_paiNumResourcesLocal[eResource] == 1 && !bWonderResourceOut) || (m_paiNumResourcesLocal[eResource] > 1);
					}
					if(bHasLocalResource)
#else
					if(IsHasResourceLocal(eResource, /*bTestVisible*/ false))
#endif
					{
						// Depends on era of wonder?
						EraTypes eResourceObsoleteEra = pkResource->getWonderProductionModObsoleteEra();
						if (eResourceObsoleteEra != NO_ERA)
						{
							EraTypes eWonderEra;
							TechTypes eTech = (TechTypes)pkBuildingInfo->GetPrereqAndTech();
							if(eTech != NO_TECH)
							{
								CvTechEntry* pEntry = GC.GetGameTechs()->GetEntry(eTech);
								if(pEntry)
								{
									eWonderEra = (EraTypes)pEntry->GetEra();
									if(eWonderEra != NO_ERA)
									{
										if (eWonderEra >= eResourceObsoleteEra)
										{
											continue;
										}
									}
								}
							}
						}

						iMultiplier += iBonus;
						GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_WONDER_LOCAL_RES", iBonus, pkBuildingInfo->GetDescription());
					}
				}
			}
		}

	}

	return iMultiplier;
}


//	--------------------------------------------------------------------------------
int CvCity::getCapturePlunderModifier() const
{
	VALIDATE_OBJECT
	return m_iCapturePlunderModifier;
}


//	--------------------------------------------------------------------------------
void CvCity::changeCapturePlunderModifier(int iChange)
{
	VALIDATE_OBJECT
	m_iCapturePlunderModifier = (m_iCapturePlunderModifier + iChange);
	CvAssert(m_iCapturePlunderModifier >= 0);
}


//	--------------------------------------------------------------------------------
int CvCity::getPlotCultureCostModifier() const
{
	VALIDATE_OBJECT
	return m_iPlotCultureCostModifier;
}


//	--------------------------------------------------------------------------------
void CvCity::changePlotCultureCostModifier(int iChange)
{
	VALIDATE_OBJECT
	m_iPlotCultureCostModifier = (m_iPlotCultureCostModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvCity::getPlotBuyCostModifier() const
{
	VALIDATE_OBJECT
	return m_iPlotBuyCostModifier;
}


//	--------------------------------------------------------------------------------
void CvCity::changePlotBuyCostModifier(int iChange)
{
	VALIDATE_OBJECT
	m_iPlotBuyCostModifier = (m_iPlotBuyCostModifier + iChange);
}

//	--------------------------------------------------------------------------------
int CvCity::GetUnitMaxExperienceLocal() const
{
	VALIDATE_OBJECT
	return m_iUnitMaxExperienceLocal;
}
void CvCity::ChangeUnitMaxExperienceLocal(int iChange)
{
	VALIDATE_OBJECT
	m_iUnitMaxExperienceLocal = (m_iUnitMaxExperienceLocal + iChange);
}

//	--------------------------------------------------------------------------------
int CvCity::GetTradeRouteRiverBonusModifier() const
{
	VALIDATE_OBJECT
	return m_iTradeRouteRiverBonusModifier;
}
void CvCity::ChangeTradeRouteRiverBonusModifier(int iChange)
{
	VALIDATE_OBJECT
	m_iTradeRouteRiverBonusModifier = (m_iTradeRouteRiverBonusModifier + iChange);
}

//	--------------------------------------------------------------------------------
int CvCity::GetSecondCapitalsExtraScore() const
{
	VALIDATE_OBJECT
	return m_iSecondCapitalsExtraScore;
}
void CvCity::ChangeSecondCapitalsExtraScore(int iChange)
{
	VALIDATE_OBJECT
	if(iChange == 0 || !MOD_GLOBAL_CORRUPTION) return;
	CvPlayerAI &kPlayer = GET_PLAYER(getOwner());
	if(!kPlayer.EnableCorruption()) return;

	m_iSecondCapitalsExtraScore += iChange;
	if(m_iSecondCapitalsExtraScore > 0 && !IsSecondCapital())
	{
		SetSecondCapital(true);
		kPlayer.AddSecondCapital(GetID());
		int iLoop = 0;
		for (CvCity* pCity = kPlayer.firstCity(&iLoop); pCity != NULL; pCity = kPlayer.nextCity(&iLoop))
		{
			pCity->UpdateCorruption();
		}
	}
	if(m_iSecondCapitalsExtraScore <= 0 && IsSecondCapital())
	{
		SetSecondCapital(false);
		GET_PLAYER(getOwner()).RemoveSecondCapital(GetID());
	}
}
//	--------------------------------------------------------------------------------
int CvCity::GetFoodKeptFromPollution() const
{
	VALIDATE_OBJECT
	return m_iFoodKeptFromPollution;
}
void CvCity::ChangeFoodKeptFromPollution(int iChange)
{
	VALIDATE_OBJECT
	m_iFoodKeptFromPollution += iChange;
}
//	--------------------------------------------------------------------------------
bool CvCity::IsAllowsFoodTradeRoutes()
{
	return m_iNumAllowsFoodTradeRoutes > 0;
}
void CvCity::ChangeNumAllowsFoodTradeRoutes(int iChange)
{
	m_iNumAllowsFoodTradeRoutes += iChange;
}
bool CvCity::IsAllowsProductionTradeRoutes()
{
	return m_iNumAllowsProductionTradeRoutes > 0;
}
void CvCity::ChangeNumAllowsProductionTradeRoutes(int iChange)
{
	m_iNumAllowsProductionTradeRoutes += iChange;
}

//	--------------------------------------------------------------------------------
#if defined(MOD_BUILDINGS_CITY_WORKING)
//	--------------------------------------------------------------------------------
int CvCity::GetCityWorkingChange() const
{
	VALIDATE_OBJECT
	return m_iCityWorkingChange;
}


//	--------------------------------------------------------------------------------
void CvCity::changeCityWorkingChange(int iChange)
{
	VALIDATE_OBJECT
	if(iChange != 0)
	{
		int iOldPlots = GetNumWorkablePlots();
		int iNewPlots = GetNumWorkablePlots(iChange);
			
		for (int iI = std::min(iOldPlots, iNewPlots); iI < std::max(iOldPlots, iNewPlots); ++iI) {
			CvPlot* pLoopPlot = plotCity(getX(), getY(), iI);

			if (pLoopPlot) {
				pLoopPlot->changeCityRadiusCount(iChange);
				pLoopPlot->changePlayerCityRadiusCount(getOwner(), iChange);
				// remove Citizens when Workable Plots reduce
				if(iChange < 0)
				{
					GetCityCitizens()->SetWorkingPlot(pLoopPlot, false);
					GetCityCitizens()->SetForcedWorkingPlot(pLoopPlot, false);
				}
			}
		}
		m_iCityWorkingChange = (m_iCityWorkingChange + iChange);
	}
}
#endif

#if defined(MOD_BUILDINGS_CITY_AUTOMATON_WORKERS)
//	--------------------------------------------------------------------------------
int CvCity::GetCityAutomatonWorkersChange() const
{
	VALIDATE_OBJECT
	return m_iCityAutomatonWorkersChange;
}

//	--------------------------------------------------------------------------------
void CvCity::changeCityAutomatonWorkersChange(int iChange)
{
	VALIDATE_OBJECT
	if(iChange != 0)
	{
		changeAutomatons(iChange);
		
		m_iCityAutomatonWorkersChange = (m_iCityAutomatonWorkersChange + iChange);
	}
}
#endif

//	--------------------------------------------------------------------------------
int CvCity::getHealRate() const
{
	VALIDATE_OBJECT
	return m_iHealRate;
}

//	--------------------------------------------------------------------------------
void CvCity::changeHealRate(int iChange)
{
	VALIDATE_OBJECT
	m_iHealRate = (m_iHealRate + iChange);
	CvAssert(getHealRate() >= 0);
}

//	--------------------------------------------------------------------------------
int CvCity::GetEspionageModifier() const
{
	VALIDATE_OBJECT
	return m_iEspionageModifier;
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeEspionageModifier(int iChange)
{
	VALIDATE_OBJECT
	m_iEspionageModifier = (m_iEspionageModifier + iChange);
}

#if defined(MOD_RELIGION_CONVERSION_MODIFIERS)
//	--------------------------------------------------------------------------------
int CvCity::GetConversionModifier() const
{
	VALIDATE_OBJECT
	return m_iConversionModifier;
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeConversionModifier(int iChange)
{
	VALIDATE_OBJECT
	m_iConversionModifier = (m_iConversionModifier + iChange);
}
#endif

//	--------------------------------------------------------------------------------
/// Does this city not produce occupied Unhappiness?
bool CvCity::IsNoOccupiedUnhappiness() const
{
	VALIDATE_OBJECT
	return GetNoOccupiedUnhappinessCount() > 0;
}

//	--------------------------------------------------------------------------------
/// Does this city not produce occupied Unhappiness?
int CvCity::GetNoOccupiedUnhappinessCount() const
{
	VALIDATE_OBJECT
	return m_iNoOccupiedUnhappinessCount;
}

//	--------------------------------------------------------------------------------
/// Does this city not produce occupied Unhappiness?
void CvCity::ChangeNoOccupiedUnhappinessCount(int iChange)
{
	VALIDATE_OBJECT
	if(iChange != 0)
		m_iNoOccupiedUnhappinessCount += iChange;
}


//	--------------------------------------------------------------------------------
int CvCity::getFood() const
{
	VALIDATE_OBJECT
	return m_iFood / 100;
}

//	--------------------------------------------------------------------------------
int CvCity::getFoodTimes100() const
{
	VALIDATE_OBJECT
	return m_iFood;
}


//	--------------------------------------------------------------------------------
void CvCity::setFood(int iNewValue)
{
	VALIDATE_OBJECT
	setFoodTimes100(iNewValue*100);
}

//	--------------------------------------------------------------------------------
void CvCity::setFoodTimes100(int iNewValue)
{
	VALIDATE_OBJECT
	if(getFoodTimes100() != iNewValue)
	{
		m_iFood = iNewValue;
	}
}


//	--------------------------------------------------------------------------------
void CvCity::changeFood(int iChange)
{
	VALIDATE_OBJECT
	setFoodTimes100(getFoodTimes100() + 100 * iChange);
}


//	--------------------------------------------------------------------------------
void CvCity::changeFoodTimes100(int iChange)
{
	VALIDATE_OBJECT
	setFoodTimes100(getFoodTimes100() + iChange);
}


//	--------------------------------------------------------------------------------
int CvCity::getFoodKept() const
{
	VALIDATE_OBJECT
	return m_iFoodKept;
}

//	--------------------------------------------------------------------------------
void CvCity::setFoodKept(int iNewValue)
{
	VALIDATE_OBJECT
	m_iFoodKept = iNewValue;
}


//	--------------------------------------------------------------------------------
void CvCity::changeFoodKept(int iChange)
{
	VALIDATE_OBJECT
	setFoodKept(getFoodKept() + iChange);
}

//	--------------------------------------------------------------------------------
int CvCity::GetCuttingBonusModifier() const
{
	ReligionTypes eMajority = GetCityReligions()->GetReligiousMajority();
	int iCuttingBonusModifier = 0;
	if(eMajority != NO_RELIGION)
	{
		const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eMajority, getOwner());
		if(pReligion)
		{
			iCuttingBonusModifier = pReligion->m_Beliefs.GetCuttingBonusModifier();
			BeliefTypes eSecondaryPantheon = GetCityReligions()->GetSecondaryReligionPantheonBelief();
			if (eSecondaryPantheon != NO_BELIEF)
			{
				iCuttingBonusModifier += GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon)->GetCuttingBonusModifier();
			}
		}
	}
	return iCuttingBonusModifier;
}

//	--------------------------------------------------------------------------------
void CvCity::DoCuttingExtraInstantYield(int iBaseYield) 
{
	ReligionTypes eMajority = GetCityReligions()->GetReligiousMajority();
	if(eMajority == NO_RELIGION) return;

	const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eMajority, getOwner());
	if(!pReligion) return;

	CvBeliefEntry* pSecondPantheon = nullptr;
	BeliefTypes eSecondaryPantheon = GetCityReligions()->GetSecondaryReligionPantheonBelief();
	if (eSecondaryPantheon != NO_BELIEF)
	{
		pSecondPantheon = GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon);
	}
	std::ostringstream yieldDetailsTip;
	bool bShowTip = (getOwner() == GC.getGame().getActivePlayer());
	for (int iYieldLoop = 0; iYieldLoop < NUM_YIELD_TYPES; iYieldLoop++)
	{
		YieldTypes eYieldType = (YieldTypes)iYieldLoop;
		CvYieldInfo* pYieldInfo = GC.getYieldInfo(eYieldType);
		int iExtraInstantModifier = pReligion->m_Beliefs.GetCuttingInstantYieldModifier(eYieldType);
		if (pSecondPantheon) iExtraInstantModifier += pSecondPantheon->GetCuttingInstantYieldModifier(eYieldType);

		int iExtraInstantYield = pReligion->m_Beliefs.GetCuttingInstantYield(eYieldType);
		if (pSecondPantheon) iExtraInstantYield += pSecondPantheon->GetCuttingInstantYield(eYieldType);

		if (iExtraInstantYield <= 0 && iExtraInstantModifier <= 0) continue;

		iExtraInstantYield += (iExtraInstantModifier * iBaseYield) / 100;
		doInstantYield(eYieldType, iExtraInstantYield);

		if (!bShowTip) continue;
		if (!yieldDetailsTip.str().empty()) yieldDetailsTip << ", ";
		yieldDetailsTip << pYieldInfo->getColorString()
                        << "+" << iExtraInstantYield
                        << "[ENDCOLOR]" 
                        << pYieldInfo->getIconString();
	}
	if(!yieldDetailsTip.str().empty())
	{
		CvString strBuffer = GetLocalizedText("TXT_KEY_BELIEF_CUTTING_NONUS", getNameKey(), yieldDetailsTip.str().c_str());
		GC.GetEngineUserInterface()->AddCityMessage(0,GetIDInfo(),getOwner(), false, GC.getEVENT_MESSAGE_TIME(), strBuffer);
	}
}
//	--------------------------------------------------------------------------------
int CvCity::getMaxFoodKeptPercent() const
{
	VALIDATE_OBJECT
#ifdef MOD_GLOBAL_CITY_SCALES
	if (MOD_GLOBAL_CITY_SCALES && !CanGrowNormally())
		return 0;
#endif
	return m_iMaxFoodKeptPercent + std::min(0, GetFoodKeptFromPollution());
}


//	--------------------------------------------------------------------------------
void CvCity::changeMaxFoodKeptPercent(int iChange)
{
	VALIDATE_OBJECT
	m_iMaxFoodKeptPercent = (m_iMaxFoodKeptPercent + iChange);
	CvAssert(m_iMaxFoodKeptPercent >= 0);
}


//	--------------------------------------------------------------------------------
int CvCity::getOverflowProduction() const
{
	VALIDATE_OBJECT
	return m_iOverflowProduction / 100;
}


//	--------------------------------------------------------------------------------
void CvCity::setOverflowProduction(int iNewValue)
{
	VALIDATE_OBJECT
	setOverflowProductionTimes100(iNewValue * 100);
}


//	--------------------------------------------------------------------------------
void CvCity::changeOverflowProduction(int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(iChange >= 0, "Production overflow is too low.  Please send a save to Ed.");
	CvAssertMsg(iChange < 250, "Production overflow is too high.  Please send a save to Ed.");
	changeOverflowProductionTimes100(iChange * 100);
}


//	--------------------------------------------------------------------------------
int CvCity::getOverflowProductionTimes100() const
{
	VALIDATE_OBJECT
	return m_iOverflowProduction;
}


//	--------------------------------------------------------------------------------
void CvCity::setOverflowProductionTimes100(int iNewValue)
{
	VALIDATE_OBJECT
	m_iOverflowProduction = iNewValue;
	CvAssert(getOverflowProductionTimes100() >= 0);
}


//	--------------------------------------------------------------------------------
void CvCity::changeOverflowProductionTimes100(int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(iChange >= 0, "Production overflow is too low.  Please send a save to Ed.");
	CvAssertMsg(iChange < 25000, "Production overflow is too high.  Please send a save to Ed.");
	setOverflowProductionTimes100(getOverflowProductionTimes100() + iChange);
}


//	--------------------------------------------------------------------------------
int CvCity::getFeatureProduction() const
{
	VALIDATE_OBJECT
	return m_iFeatureProduction;
}


//	--------------------------------------------------------------------------------
void CvCity::setFeatureProduction(int iNewValue)
{
	VALIDATE_OBJECT
	m_iFeatureProduction = iNewValue;
	CvAssert(getFeatureProduction() >= 0);
}


//	--------------------------------------------------------------------------------
void CvCity::changeFeatureProduction(int iChange)
{
	VALIDATE_OBJECT
	setFeatureProduction(getFeatureProduction() + iChange);
}


//	--------------------------------------------------------------------------------
int CvCity::getMilitaryProductionModifier()	const
{
	VALIDATE_OBJECT
	return m_iMilitaryProductionModifier;
}


//	--------------------------------------------------------------------------------
void CvCity::changeMilitaryProductionModifier(int iChange)
{
	VALIDATE_OBJECT
	m_iMilitaryProductionModifier = (m_iMilitaryProductionModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvCity::getSpaceProductionModifier() const
{
	VALIDATE_OBJECT
	return m_iSpaceProductionModifier;
}


//	--------------------------------------------------------------------------------
void CvCity::changeSpaceProductionModifier(int iChange)
{
	VALIDATE_OBJECT
	m_iSpaceProductionModifier = (m_iSpaceProductionModifier + iChange);
}


//	--------------------------------------------------------------------------------
int CvCity::getFreeExperience() const
{
	VALIDATE_OBJECT
	return m_iFreeExperience;
}


//	--------------------------------------------------------------------------------
void CvCity::changeFreeExperience(int iChange)
{
	VALIDATE_OBJECT
	m_iFreeExperience = (m_iFreeExperience + iChange);
	CvAssert(getFreeExperience() >= 0);
}

//	--------------------------------------------------------------------------------
#if defined(MOD_BELIEF_NEW_EFFECT_FOR_SP)
int CvCity::GetGreatPersonPointsFromReligion(GreatPersonTypes eGreatPersonTypes)
{
	VALIDATE_OBJECT
	int resValue = 0;
	ReligionTypes eReligion = GetCityReligions()->GetReligiousMajority();
	if(eReligion != NO_RELIGION)
	{
		const CvReligion* pkReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion,getOwner());
		if(pkReligion->m_Beliefs.HasGreatPersonPoints())
		{
			resValue += pkReligion->m_Beliefs.GetGreatPersonPoints(eGreatPersonTypes,isCapital(),GetCityReligions()->IsHolyCityForReligion(eReligion));
		}
	}
	
	BeliefTypes eBelief = GetCityReligions()->GetSecondaryReligionPantheonBelief();
	if(eBelief != NO_BELIEF)
	{
		resValue +=  GC.GetGameBeliefs()->GetEntry(eBelief)->GetGreatPersonPoints(eGreatPersonTypes,isCapital(),false);
	}

	return resValue;
}
//	--------------------------------------------------------------------------------
int CvCity::GetReligionExtraMissionarySpreads(ReligionTypes eReligion)
{
	VALIDATE_OBJECT
	if(eReligion == NO_RELIGION) return 0;
	return GC.getGame().GetGameReligions()->GetReligion(eReligion,getOwner())->m_Beliefs.GetCityExtraMissionarySpreads();
}
//	--------------------------------------------------------------------------------
int CvCity::GetBeliefExtraMissionarySpreads(BeliefTypes eBelief)
{
	VALIDATE_OBJECT
	if(eBelief == NO_BELIEF) return 0;
	return GC.GetGameBeliefs()->GetEntry(eBelief)->GetCityExtraMissionarySpreads();
}
#endif	
//	--------------------------------------------------------------------------------
#if defined(MOD_GLOBAL_BUILDING_INSTANT_YIELD)
#if defined(MOD_BELIEF_NEW_EFFECT_FOR_SP)
void CvCity::doRelogionInstantYield(ReligionTypes eReligion)
{
	VALIDATE_OBJECT
	if(eReligion == NO_RELIGION) return;
	const CvReligion* pkReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion,getOwner());
	if(!pkReligion->m_Beliefs.AllowYieldPerBirth()) return;
	for (int iYieldLoop = 0; iYieldLoop < NUM_YIELD_TYPES; iYieldLoop++)
	{
		YieldTypes iYieldType = (YieldTypes)iYieldLoop;
		int iValue = pkReligion->m_Beliefs.GetYieldPerBirth(iYieldType);
		if(iValue > 0)
		{
			iValue *= GC.getGame().getGameSpeedInfo().getFaithPercent();
			iValue /= 100;
			doInstantYield((YieldTypes)iYieldLoop, iValue);
		}
	}
}
void CvCity::doBeliefInstantYield(BeliefTypes eBelief)
{
	VALIDATE_OBJECT
	if(eBelief == NO_BELIEF) return;
	const CvBeliefEntry* pkBelief = GC.GetGameBeliefs()->GetEntry(eBelief);
	if(!pkBelief->AllowYieldPerBirth()) return;
	for (int iYieldLoop = 0; iYieldLoop < NUM_YIELD_TYPES; iYieldLoop++)
	{
		YieldTypes iYieldType = (YieldTypes)iYieldLoop;
		int iValue = pkBelief->GetYieldPerBirth(iYieldType);
		if(iValue > 0)
		{
			iValue *= GC.getGame().getGameSpeedInfo().getFaithPercent();
			iValue /= 100;
			doInstantYield((YieldTypes)iYieldLoop, iValue);
		}
	}
}
#endif
void CvCity::doBuildingInstantYield(int* iInstantYield)
{
	VALIDATE_OBJECT
	if(!iInstantYield) return;
	for (int iYieldLoop = 0; iYieldLoop < NUM_YIELD_TYPES; iYieldLoop++)
	{
		int iValue = iInstantYield[iYieldLoop];
		if(iValue > 0)
		{
			iValue *= GC.getGame().getGameSpeedInfo().getGoldPercent();
			iValue /= 100;
			doInstantYield((YieldTypes)iYieldLoop, iValue);
		}
	}
}
//	--------------------------------------------------------------------------------
void CvCity::doInstantYield(YieldTypes iYield, int iValue)
{
	VALIDATE_OBJECT
	if (iValue > 0)
	{
		CvPlayerAI& thisPlayer = GET_PLAYER(getOwner());
		//Apply yields.
		switch(iYield)
		{
			case NO_YIELD:
			break;
			case YIELD_FOOD:
			{
				changeFood(iValue);
			}
			break;
			case YIELD_PRODUCTION:
			{
				if (getProduction() < getProductionNeeded() && isProduction())
				{
					changeProduction(iValue);
				}
				else
				{
					changeOverflowProduction(iValue);
				}
			}
			break;
			case YIELD_GOLD:
			{
				thisPlayer.GetTreasury()->ChangeGold(iValue);
			}
			break;
			case YIELD_SCIENCE:
			{
				TechTypes eCurrentTech = thisPlayer.GetPlayerTechs()->GetCurrentResearch();
				if(eCurrentTech == NO_TECH)
				{
					thisPlayer.changeOverflowResearch(iValue);
				}
				else
				{
					GET_TEAM(thisPlayer.getTeam()).GetTeamTechs()->ChangeResearchProgress(eCurrentTech, iValue, thisPlayer.GetID());
				}
			}
			break;
			case YIELD_CULTURE:
			{
				thisPlayer.changeJONSCulture(iValue);
				ChangeJONSCultureStored(iValue);
			}
			break;
			case YIELD_FAITH:
			{
				thisPlayer.ChangeFaith(iValue);
			}
			break;
			case YIELD_TOURISM:
			{
				thisPlayer.GetCulture()->AddTourismAllKnownCivs(iValue);
			}
			break;
			case YIELD_GOLDEN_AGE_POINTS:
			{
				thisPlayer.ChangeGoldenAgeProgressMeter(iValue);
			}
			break;
		}
#if defined(SHOW_PLOT_POPUP)
		if(getOwner() == GC.getGame().getActivePlayer())
		{
			//And now notifications.
			CvYieldInfo* pYieldInfo = GC.getYieldInfo(iYield);
			char text[256] = {0};
			sprintf_s(text, "%s+%d[ENDCOLOR] %s", pYieldInfo->getColorString(), iValue, pYieldInfo->getIconString());
			SHOW_PLOT_POPUP(plot(), thisPlayer.GetID(), text);
		}
#endif
	}
}
#endif
//	--------------------------------------------------------------------------------
bool CvCity::CanAirlift() const
{
	if(m_iNumCanAirlift > 0) return true;
#if defined(MOD_EVENTS_CITY_AIRLIFT)
	if (MOD_EVENTS_CITY_AIRLIFT) {
		if (GAMEEVENTINVOKE_TESTANY(GAMEEVENT_CityCanAirlift, getOwner(), GetID()) == GAMEEVENTRETURN_TRUE) {
			return true;
		}
	}
#endif				

	return false;
}
void CvCity::ChangeNumCanAirlift(int iChange)
{
	VALIDATE_OBJECT
	m_iNumCanAirlift += iChange;
}

//	--------------------------------------------------------------------------------
int CvCity::GetMaxAirUnits() const
{
	VALIDATE_OBJECT
	return m_iMaxAirUnits;
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeMaxAirUnits(int iChange)
{
	VALIDATE_OBJECT
	m_iMaxAirUnits += iChange;
}

//	--------------------------------------------------------------------------------
int CvCity::getNukeModifier() const
{
	VALIDATE_OBJECT
	return m_iNukeModifier;
}

//	--------------------------------------------------------------------------------
int CvCity::GetTradeRouteTargetBonus() const
{
	VALIDATE_OBJECT
	return m_iTradeRouteTargetBonus;
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeTradeRouteTargetBonus(int iChange)
{
	VALIDATE_OBJECT
	m_iTradeRouteTargetBonus += iChange;
}

//	--------------------------------------------------------------------------------
int CvCity::GetTradeRouteRecipientBonus() const
{
	VALIDATE_OBJECT
	return m_iTradeRouteRecipientBonus;
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeTradeRouteRecipientBonus(int iChange)
{
	VALIDATE_OBJECT
	m_iTradeRouteRecipientBonus += iChange;
}

//	--------------------------------------------------------------------------------
void CvCity::changeNukeModifier(int iChange)
{
	VALIDATE_OBJECT
	m_iNukeModifier = (m_iNukeModifier + iChange);
}

//	--------------------------------------------------------------------------------
bool CvCity::IsResistance() const
{
	VALIDATE_OBJECT
	return GetResistanceTurns() > 0;
}

//	--------------------------------------------------------------------------------
int CvCity::GetResistanceTurns() const
{
	VALIDATE_OBJECT
	return m_iResistanceTurns;
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeResistanceTurns(int iChange)
{
	VALIDATE_OBJECT
	if(iChange != 0)
	{
		m_iResistanceTurns += iChange;

		auto_ptr<ICvCity1> pCity = GC.WrapCityPointer(this);
		DLLUI->SetSpecificCityInfoDirty(pCity.get(), CITY_UPDATE_TYPE_BANNER);
	}
}

//	--------------------------------------------------------------------------------
void CvCity::DoResistanceTurn()
{
	VALIDATE_OBJECT
	if(IsResistance())
	{
		ChangeResistanceTurns(-1);
	}
}

//	--------------------------------------------------------------------------------
bool CvCity::IsRazing() const
{
	VALIDATE_OBJECT
	return GetRazingTurns() > 0;
}

//	--------------------------------------------------------------------------------
int CvCity::GetRazingTurns() const
{
	VALIDATE_OBJECT
	return m_iRazingTurns;
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeRazingTurns(int iChange)
{
	VALIDATE_OBJECT
	if(iChange != 0)
	{
		m_iRazingTurns += iChange;


		auto_ptr<ICvCity1> pCity = GC.WrapCityPointer(this);
		DLLUI->SetSpecificCityInfoDirty(pCity.get(), CITY_UPDATE_TYPE_BANNER);
	}
}

//	--------------------------------------------------------------------------------
bool CvCity::DoRazingTurn()
{
	VALIDATE_OBJECT

	AI_PERF_FORMAT("City-AI-perf.csv", ("CvCity::DoRazingTurn, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );
	if(IsRazing())
	{
		CvPlayer& kPlayer = GET_PLAYER(getOwner());
		int iPopulationDrop = 1;
		iPopulationDrop *= (100 + kPlayer.GetRazeSpeedModifier());
		iPopulationDrop /= 100;

		ChangeRazingTurns(-1);

		int iPopulationDropActual = min(iPopulationDrop, getPopulation());
		if (kPlayer.GetPlayerTraits()->GetUnitMaxHitPointChangePerRazedCityPop() > 0) {
#ifdef MOD_GLOBAL_CITY_WORKING
			for(int iPlotLoop = 0; iPlotLoop < GetNumWorkablePlots(); iPlotLoop++)
#else
			for(int iPlotLoop = 0; iPlotLoop < NUM_CITY_PLOTS; iPlotLoop++)
#endif
			{
				CvPlot* pLoopPlot = GetCityCitizens()->GetCityPlotFromIndex(iPlotLoop);

				if (pLoopPlot != NULL && pLoopPlot->getWorkingCity() == this)
				{
					for (int iUnitLoop = 0; iUnitLoop < pLoopPlot->getNumUnits(); iUnitLoop++)
					{
						CvUnit* unit = pLoopPlot->getUnitByIndex(iUnitLoop);
						if (unit && unit->IsCombatUnit() && unit->getOwner() == getOwner())
						{
							int iValue = min(kPlayer.GetPlayerTraits()->GetUnitMaxHitPointChangePerRazedCityPopLimit(),
									unit->getMaxHitPointsChangeFromRazedCityPop() + kPlayer.GetPlayerTraits()->GetUnitMaxHitPointChangePerRazedCityPop()* iPopulationDropActual);
							unit->setMaxHitPointsChangeFromRazedCityPop(iValue);
						}
					}
				}
			}
		}

		// Counter has reached 0, disband the City
		if(GetRazingTurns() <= 0 || getPopulation() - iPopulationDrop < 1)
		{
			CvPlot* pkPlot = plot();

			pkPlot->AddArchaeologicalRecord(CvTypes::getARTIFACT_RAZED_CITY(), getOriginalOwner(), getOwner());

			kPlayer.disband(this);
			GC.getGame().addReplayMessage(REPLAY_MESSAGE_CITY_DESTROYED, getOwner(), "", pkPlot->getX(), pkPlot->getY());
			return true;
		}
		// Counter is positive, reduce population
		else
		{
			changePopulation(-iPopulationDrop, true);
		}
	}

	return false;
}

/// Has this City been taken from its owner?
//	--------------------------------------------------------------------------------
bool CvCity::IsOccupied() const
{
	VALIDATE_OBJECT

	// If we're a puppet then we don't count as an occupied city
	if(IsPuppet())
		return false;

	return m_bOccupied;
}

//	--------------------------------------------------------------------------------
/// Has this City been taken from its owner?
void CvCity::SetOccupied(bool bValue)
{
	VALIDATE_OBJECT
	if(IsOccupied() != bValue)
	{
		m_bOccupied = bValue;
	}
}

//	--------------------------------------------------------------------------------
/// Has this City been turned into a puppet by someone capturing it?
bool CvCity::IsPuppet() const
{
	VALIDATE_OBJECT
	return m_bPuppet;
}

//	--------------------------------------------------------------------------------
/// Has this City been turned into a puppet by someone capturing it?
void CvCity::SetPuppet(bool bValue)
{
	VALIDATE_OBJECT
	if(IsPuppet() != bValue)
	{
#if defined(MOD_EVENTS_CITY_PUPPETED)
		if(MOD_EVENTS_CITY_PUPPETED)
		{
			GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityPuppeted, getOwner(), GetID());
		}
#endif
		m_bPuppet = bValue;
	}

#ifdef MOD_GLOBAL_CORRUPTION
	if (MOD_GLOBAL_CORRUPTION)
		UpdateCorruption();
#endif
}

//	--------------------------------------------------------------------------------
/// Turn this City into a puppet
void CvCity::DoCreatePuppet()
{
	VALIDATE_OBJECT

	// Turn this off - used to display info for annex/puppet/raze popup
	SetIgnoreCityForHappiness(false);

	SetPuppet(true);

	setProductionAutomated(true, true);

	int iForceWorkingPuppetRange = 2;

	CvPlot* pLoopPlot;

	// Loop through all plots near this City
#if defined(MOD_GLOBAL_CITY_WORKING)
	for(int iPlotLoop = 0; iPlotLoop < GetNumWorkablePlots(); iPlotLoop++)
#else
	for(int iPlotLoop = 0; iPlotLoop < NUM_CITY_PLOTS; iPlotLoop++)
#endif
	{
		pLoopPlot = plotCity(getX(), getY(), iPlotLoop);

		if(pLoopPlot != NULL)
		{
			// Cut off areas around the city we don't care about
			pLoopPlot = plotXYWithRangeCheck(pLoopPlot->getX(), pLoopPlot->getY(), getX(), getY(), iForceWorkingPuppetRange);

			if(pLoopPlot != NULL)
			{
				pLoopPlot->setWorkingCityOverride(this);
			}
		}
	}

	// Remove any buildings that are not applicable to puppets (but might have been earned through traits/policies)
	for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		BuildingTypes eBuilding = (BuildingTypes) iI;
		CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
		if(pkBuildingInfo)
		{
			if (pkBuildingInfo->IsNoOccupiedUnhappiness())
			{
				GetCityBuildings()->SetNumFreeBuilding(eBuilding, 0);
			}
		}
	}

	UpdateAllNonPlotYields();


	GET_PLAYER(getOwner()).DoUpdateHappiness();
	GET_PLAYER(getOwner()).DoUpdateNextPolicyCost();

	DLLUI->setDirty(CityInfo_DIRTY_BIT, true);
	DLLUI->setDirty(GameData_DIRTY_BIT, true);
}

//	--------------------------------------------------------------------------------
/// Un-puppet a City and force it into the empire
void CvCity::DoAnnex()
{
	VALIDATE_OBJECT

	// Turn this off - used to display info for annex/puppet/raze popup
	SetIgnoreCityForHappiness(false);

	SetPuppet(false);

	setProductionAutomated(false, true);

	UpdateAllNonPlotYields();

	GET_PLAYER(getOwner()).DoUpdateHappiness();

#if !defined(NO_ACHIEVEMENTS)
	if(getOriginalOwner() != GetID())
	{
		if(GET_PLAYER(getOriginalOwner()).isMinorCiv())
		{
			if(!GC.getGame().isGameMultiPlayer() && GET_PLAYER(getOwner()).isHuman())
			{
				bool bUsingXP1Scenario1 = gDLL->IsModActivated(CIV5_XP1_SCENARIO1_MODID);
				if(!bUsingXP1Scenario1)
				{
					gDLL->UnlockAchievement(ACHIEVEMENT_CITYSTATE_ANNEX);
				}
			}
		}
	}
#endif



	GET_PLAYER(getOwner()).DoUpdateNextPolicyCost();

	DLLUI->setDirty(CityInfo_DIRTY_BIT, true);
	DLLUI->setDirty(GameData_DIRTY_BIT, true);
}

//	--------------------------------------------------------------------------------
int CvCity::GetLocalHappiness() const
{
	CvPlayer& kPlayer = GET_PLAYER(m_eOwner);

	int iLocalHappiness = GetBaseHappinessFromBuildings();

	int iHappinessPerGarrison = kPlayer.GetHappinessPerGarrisonedUnit();
	if(iHappinessPerGarrison > 0)
	{
		if(GetGarrisonedUnit() != NULL)
		{
			iLocalHappiness++;
		}
	}

	// Follower beliefs
	int iHappinessFromReligion = 0;
	CvGameReligions* pReligions = GC.getGame().GetGameReligions();

	ReligionTypes eMajority = GetCityReligions()->GetReligiousMajority();
	if(eMajority != NO_RELIGION)
	{
		BeliefTypes eSecondaryPantheon = GetCityReligions()->GetSecondaryReligionPantheonBelief();
		int iFollowers = GetCityReligions()->GetNumFollowers(eMajority);

		const CvReligion* pReligion = pReligions->GetReligion(eMajority, kPlayer.GetID());
		if(pReligion)
		{
			iHappinessFromReligion += pReligion->m_Beliefs.GetHappinessPerCity(getPopulation());
			if (eSecondaryPantheon != NO_BELIEF && getPopulation() >= GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon)->GetMinPopulation())
			{
				iHappinessFromReligion += GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon)->GetHappinessPerCity();
			}
			if(plot()->isRiver())
			{
				iHappinessFromReligion += pReligion->m_Beliefs.GetRiverHappiness();
				if (eSecondaryPantheon != NO_BELIEF)
				{
					iHappinessFromReligion += GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon)->GetRiverHappiness();
				}
			}

			// Buildings
			for(int jJ = 0; jJ < GC.getNumBuildingClassInfos(); jJ++)
			{
				BuildingClassTypes eBuildingClass = (BuildingClassTypes)jJ;

				CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo(eBuildingClass);
				if(!pkBuildingClassInfo)
				{
					continue;
				}

				CvCivilizationInfo& playerCivilizationInfo = kPlayer.getCivilizationInfo();
				BuildingTypes eBuilding = (BuildingTypes)playerCivilizationInfo.getCivilizationBuildings(eBuildingClass);

				if(eBuilding != NO_BUILDING)
				{
					if(GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
					{
#if defined(MOD_BUGFIX_MINOR)
						iHappinessFromReligion += pReligion->m_Beliefs.GetBuildingClassHappiness(eBuildingClass, iFollowers) * GetCityBuildings()->GetNumBuilding(eBuilding);
#else
						iHappinessFromReligion += pReligion->m_Beliefs.GetBuildingClassHappiness(eBuildingClass, iFollowers);
#endif
					}
				}
			}
		}
		iLocalHappiness += iHappinessFromReligion;
	}

	// Policy Num Religion Mods
	iLocalHappiness += kPlayer.getPolicyModifiers(POLICYMOD_HAPPINESS_PER_RELIGION_IN_CITY) * GetCityReligions()->GetNumReligionsWithFollowers();

	// Policy Building Mods
	int iSpecialPolicyBuildingHappiness = 0;
	int iBuildingClassLoop;
	BuildingClassTypes eBuildingClass;
	for(int iPolicyLoop = 0; iPolicyLoop < GC.getNumPolicyInfos(); iPolicyLoop++)
	{
		PolicyTypes ePolicy = (PolicyTypes)iPolicyLoop;
		CvPolicyEntry* pkPolicyInfo = GC.getPolicyInfo(ePolicy);
		if(pkPolicyInfo)
		{
			if(kPlayer.GetPlayerPolicies()->HasPolicy(ePolicy) && !kPlayer.GetPlayerPolicies()->IsPolicyBlocked(ePolicy))
			{
				for(iBuildingClassLoop = 0; iBuildingClassLoop < GC.getNumBuildingClassInfos(); iBuildingClassLoop++)
				{
					eBuildingClass = (BuildingClassTypes) iBuildingClassLoop;

					CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo(eBuildingClass);
					if(!pkBuildingClassInfo)
					{
						continue;
					}

					BuildingTypes eBuilding = (BuildingTypes)kPlayer.getCivilizationInfo().getCivilizationBuildings(eBuildingClass);
					if(eBuilding != NO_BUILDING && GetCityBuildings()->GetNumBuilding(eBuilding) > 0) // slewis - added the NO_BUILDING check for the ConquestDLX scenario which has civ specific wonders
					{
						if(pkPolicyInfo->GetBuildingClassHappiness(eBuildingClass) != 0)
						{
#if defined(MOD_BUGFIX_MINOR)
							iSpecialPolicyBuildingHappiness += pkPolicyInfo->GetBuildingClassHappiness(eBuildingClass) * GetCityBuildings()->GetNumBuilding(eBuilding);
#else
							iSpecialPolicyBuildingHappiness += pkPolicyInfo->GetBuildingClassHappiness(eBuildingClass);
#endif
						}
					}
				}
			}
		}
	}

	iLocalHappiness += iSpecialPolicyBuildingHappiness;

	if (GetWeLoveTheKingDayCounter() > 0)
	{
		iLocalHappiness += kPlayer.GetHappinessInWLTKDCities();
	}

	int iLocalHappinessCap = getPopulation();

	// India has unique way to compute local happiness cap
	if(kPlayer.GetPlayerTraits()->GetCityUnhappinessModifier() != 0)
	{
		// 0.67 per population, rounded up
		iLocalHappinessCap = (iLocalHappinessCap * 20) + 15;
		iLocalHappinessCap /= 30;
	}

	if(iLocalHappinessCap < iLocalHappiness)
	{
		return iLocalHappinessCap;
	}
	else
	{
		return iLocalHappiness;
	}
}

//	--------------------------------------------------------------------------------
int CvCity::GetHappinessFromBuildings() const
{
	return GetUnmoddedHappinessFromBuildings();
}

//	--------------------------------------------------------------------------------
int CvCity::GetBaseHappinessFromBuildings() const
{
	return m_iBaseHappinessFromBuildings;
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeBaseHappinessFromBuildings(int iChange)
{
	m_iBaseHappinessFromBuildings += iChange;
}

//	--------------------------------------------------------------------------------
int CvCity::GetUnmoddedHappinessFromBuildings() const
{
	return m_iUnmoddedHappinessFromBuildings;
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeUnmoddedHappinessFromBuildings(int iChange)
{
	m_iUnmoddedHappinessFromBuildings += iChange;
}

//	--------------------------------------------------------------------------------
/// Used when gathering info for "Annex/Puppet/Raze" popup
bool CvCity::IsIgnoreCityForHappiness() const
{
	return m_bIgnoreCityForHappiness;
}

//	--------------------------------------------------------------------------------
/// Used when gathering info for "Annex/Puppet/Raze" popup
void CvCity::SetIgnoreCityForHappiness(bool bValue)
{
	m_bIgnoreCityForHappiness = bValue;
}

//	--------------------------------------------------------------------------------
/// Find the non-wonder building that provides the highest culture at the least cost
BuildingTypes CvCity::ChooseFreeCultureBuilding() const
{
	BuildingTypes eRtnValue = NO_BUILDING;
	int iNumBuildingInfos = GC.getNumBuildingInfos();
	CvWeightedVector<int, SAFE_ESTIMATE_NUM_BUILDINGS, true> buildingChoices;

	for(int iI = 0; iI < iNumBuildingInfos; iI++)
	{
		const BuildingTypes eBuilding = static_cast<BuildingTypes>(iI);
		CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
		if(pkBuildingInfo)
		{
			const CvBuildingClassInfo& kBuildingClassInfo = pkBuildingInfo->GetBuildingClassInfo();
			if(!isWorldWonderClass(kBuildingClassInfo) && !isNationalWonderClass(kBuildingClassInfo))
			{
				int iCulture = pkBuildingInfo->GetYieldChange(YIELD_CULTURE);
				int iCost = pkBuildingInfo->GetProductionCost();
				if(getFirstBuildingOrder(eBuilding) != -1 || canConstruct(eBuilding))
				{
					if(iCulture > 0 && iCost > 0)
					{
						int iWeight = iCulture * 10000 / iCost;

						if(iWeight > 0)
						{
							buildingChoices.push_back(iI, iWeight);
						}
					}
				}
			}
		}
	}

	if(buildingChoices.size() > 0)
	{
		buildingChoices.SortItems();
		eRtnValue = (BuildingTypes)buildingChoices.GetElement(0);
	}

	return eRtnValue;
}

//	--------------------------------------------------------------------------------
/// Find the non-wonder building that provides the highest culture at the least cost
BuildingTypes CvCity::ChooseFreeFoodBuilding() const
{
	BuildingTypes eRtnValue = NO_BUILDING;
	int iNumBuildingInfos = GC.getNumBuildingInfos();
	CvWeightedVector<int, SAFE_ESTIMATE_NUM_BUILDINGS, true> buildingChoices;

	for(int iI = 0; iI < iNumBuildingInfos; iI++)
	{
		const BuildingTypes eBuilding = static_cast<BuildingTypes>(iI);
		CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
		if(pkBuildingInfo)
		{
			const CvBuildingClassInfo& kBuildingClassInfo = pkBuildingInfo->GetBuildingClassInfo();
			if(!isWorldWonderClass(kBuildingClassInfo) && !isNationalWonderClass(kBuildingClassInfo))
			{
#if defined(MOD_BUGFIX_FREE_FOOD_BUILDING)
				if(!MOD_BUGFIX_FREE_FOOD_BUILDING || (getFirstBuildingOrder(eBuilding) != -1 || canConstruct(eBuilding)))
				{
#endif
					int iFood = pkBuildingInfo->GetFoodKept();
					int iCost = pkBuildingInfo->GetProductionCost();
					if(iFood > 0 && iCost > 0)
					{
						int iWeight = iFood * 10000 / iCost;

						if(iWeight > 0)
						{
							buildingChoices.push_back(iI, iWeight);
						}
					}
#if defined(MOD_BUGFIX_FREE_FOOD_BUILDING)
				}
#endif
			}
		}
	}

	if(buildingChoices.size() > 0)
	{
		buildingChoices.SortItems();
		eRtnValue = (BuildingTypes)buildingChoices.GetElement(0);
	}

	return eRtnValue;
}

//	--------------------------------------------------------------------------------
int CvCity::getCitySizeBoost() const
{
	VALIDATE_OBJECT
	return m_iCitySizeBoost;
}


//	--------------------------------------------------------------------------------
void CvCity::setCitySizeBoost(int iBoost)
{
	VALIDATE_OBJECT
	if(getCitySizeBoost() != iBoost)
	{
		m_iCitySizeBoost = iBoost;

		setLayoutDirty(true);
	}
}


//	--------------------------------------------------------------------------------
bool CvCity::isNeverLost() const
{
	VALIDATE_OBJECT
	return m_bNeverLost;
}


//	--------------------------------------------------------------------------------
void CvCity::setNeverLost(bool bNewValue)
{
	VALIDATE_OBJECT
	m_bNeverLost = bNewValue;
}


//	--------------------------------------------------------------------------------
bool CvCity::isDrafted() const
{
	VALIDATE_OBJECT
	return m_bDrafted;
}


//	--------------------------------------------------------------------------------
void CvCity::setDrafted(bool bNewValue)
{
	VALIDATE_OBJECT
	m_bDrafted = bNewValue;
}

//	--------------------------------------------------------------------------------
bool CvCity::IsOwedCultureBuilding() const
{
	return m_bOwedCultureBuilding;
}

//	--------------------------------------------------------------------------------
void CvCity::SetOwedCultureBuilding(bool bNewValue)
{
	m_bOwedCultureBuilding = bNewValue;
}

#if defined(MOD_BUGFIX_FREE_FOOD_BUILDING)
//	--------------------------------------------------------------------------------
bool CvCity::IsOwedFoodBuilding() const
{
	return m_bOwedFoodBuilding;
}

//	--------------------------------------------------------------------------------
void CvCity::SetOwedFoodBuilding(bool bNewValue)
{
	m_bOwedFoodBuilding = bNewValue;
}
#endif

//	--------------------------------------------------------------------------------
bool CvCity::IsBlockaded() const
{
	VALIDATE_OBJECT
	bool bHasWaterRouteBuilding = false;

	CvBuildingXMLEntries* pkGameBuildings = GC.GetGameBuildings();

	// Loop through adding the available buildings
	for(int i = 0; i < GC.GetGameBuildings()->GetNumBuildings(); i++)
	{
		BuildingTypes eBuilding = (BuildingTypes)i;
		CvBuildingEntry* pkBuildingInfo = pkGameBuildings->GetEntry(i);
		if(pkBuildingInfo)
		{
			if(pkBuildingInfo->AllowsWaterRoutes())
			{
				if(GetCityBuildings()->GetNumActiveBuilding(eBuilding) > 0)
				{
					bHasWaterRouteBuilding = true;
					break;
				}
			}
		}
	}

	// there is no water route building, so it can't be blockaded
	if(!bHasWaterRouteBuilding)
	{
		return false;
	}
	
#if defined(MOD_GLOBAL_ADJACENT_BLOCKADES)
	if (MOD_GLOBAL_ADJACENT_BLOCKADES) {
		for (int iPortLoop = 0; iPortLoop < NUM_DIRECTION_TYPES; ++iPortLoop) {
			CvPlot* pAdjacentPlot = plotDirection(getX(), getY(), ((DirectionTypes)iPortLoop));
			if (pAdjacentPlot && pAdjacentPlot->isWater() && !pAdjacentPlot->isBlockaded(getOwner())) {
				return false;
			}
		}
	
		return true;
	} else {
#endif
		int iRange = 2;
		CvPlot* pLoopPlot = NULL;

		for(int iDX = -iRange; iDX <= iRange; iDX++)
		{
			for(int iDY = -iRange; iDY <= iRange; iDY++)
			{
				pLoopPlot = plotXYWithRangeCheck(getX(), getY(), iDX, iDY, iRange);
				if(!pLoopPlot)
				{
					continue;
				}

				if(pLoopPlot->isWater() && pLoopPlot->getVisibleEnemyDefender(getOwner()))
				{
					return true;
				}
			}
		}

		return false;
#if defined(MOD_GLOBAL_ADJACENT_BLOCKADES)
	}
#endif
}

//	--------------------------------------------------------------------------------
/// Amount of turns left in WLTKD
int CvCity::GetWeLoveTheKingDayCounter() const
{
	VALIDATE_OBJECT
	return m_iWeLoveTheKingDayCounter;
}

//	--------------------------------------------------------------------------------
///Sets number of turns left in WLTKD
//void CvCity::SetWeLoveTheKingDayCounter(int iValue)
//{
	//VALIDATE_OBJECT
	//m_iWeLoveTheKingDayCounter = iValue;
//}

void CvCity::SetWeLoveTheKingDayCounter(int iValue)
{
	VALIDATE_OBJECT

		bool bNewWLTKD = false;
	if (m_iWeLoveTheKingDayCounter <= 0 && iValue > 0)
		bNewWLTKD = true;

	if (bNewWLTKD)
	{
#if defined(MOD_EVENTS_WLKD_DAY)
		GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityBeginsWLTKD, getOwner(), getX(), getY(), iValue);
#endif
	}
	else if (iValue == 0)
	{
#if defined(MOD_EVENTS_WLKD_DAY)
		if(MOD_EVENTS_WLKD_DAY)
		{
			GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityEndsWLTKD, getOwner(), getX(), getY(), iValue);
		}
#endif
	}
	int iWLTKmod = GET_PLAYER(getOwner()).GetPlayerTraits()->GetWLKDLengthChangeModifier();
	if (iWLTKmod > 0 && iValue > GetWeLoveTheKingDayCounter())
	{
		iValue = iValue + ((iValue - GetWeLoveTheKingDayCounter())*(iWLTKmod)/100);
	}
	m_iWeLoveTheKingDayCounter = iValue;
}

//	--------------------------------------------------------------------------------
///Changes number of turns left in WLTKD
void CvCity::ChangeWeLoveTheKingDayCounter(int iChange)
{
	VALIDATE_OBJECT
	SetWeLoveTheKingDayCounter(GetWeLoveTheKingDayCounter() + iChange);
}

//	--------------------------------------------------------------------------------
/// Turn number when AI placed a garrison here
int CvCity::GetLastTurnGarrisonAssigned() const
{
	VALIDATE_OBJECT
	return m_iLastTurnGarrisonAssigned;
}

//	--------------------------------------------------------------------------------
/// Sets turn number when AI placed a garrison: AI sets to INT_MAX if city has walls and doesn't need a garrison to fire
void CvCity::SetLastTurnGarrisonAssigned(int iValue)
{
	VALIDATE_OBJECT
	m_iLastTurnGarrisonAssigned = iValue;
}

//	--------------------------------------------------------------------------------
int CvCity::GetNumThingsProduced() const
{
	VALIDATE_OBJECT
	return m_iThingsProduced;
}

//	--------------------------------------------------------------------------------
bool CvCity::isProductionAutomated() const
{
	VALIDATE_OBJECT
	return m_bProductionAutomated;
}


//	--------------------------------------------------------------------------------
void CvCity::setProductionAutomated(bool bNewValue, bool bClear)
{
	VALIDATE_OBJECT
	if(isProductionAutomated() != bNewValue)
	{
		m_bProductionAutomated = bNewValue;

		if((getOwner() == GC.getGame().getActivePlayer()) && isCitySelected())
		{
			DLLUI->setDirty(SelectionButtons_DIRTY_BIT, true);
		}

		// if automated and not network game and all 3 modifiers down, clear the queue and choose again
		if(bNewValue && bClear)
		{
			clearOrderQueue();
		}
		if(!isProduction())
		{
			AI_chooseProduction(false /*bInterruptWonders*/);
		}
	}
}


//	--------------------------------------------------------------------------------
bool CvCity::isLayoutDirty() const
{
	VALIDATE_OBJECT
	return m_bLayoutDirty;
}


//	--------------------------------------------------------------------------------
void CvCity::setLayoutDirty(bool bNewValue)
{
	VALIDATE_OBJECT
	m_bLayoutDirty = bNewValue;
}

//	--------------------------------------------------------------------------------
PlayerTypes CvCity::getPreviousOwner() const
{
	VALIDATE_OBJECT
	return m_ePreviousOwner;
}


//	--------------------------------------------------------------------------------
void CvCity::setPreviousOwner(PlayerTypes eNewValue)
{
	VALIDATE_OBJECT
	m_ePreviousOwner = eNewValue;
}


//	--------------------------------------------------------------------------------
PlayerTypes CvCity::getOriginalOwner() const
{
	VALIDATE_OBJECT
	return m_eOriginalOwner;
}


//	--------------------------------------------------------------------------------
void CvCity::setOriginalOwner(PlayerTypes eNewValue)
{
	VALIDATE_OBJECT
	m_eOriginalOwner = eNewValue;
}


//	--------------------------------------------------------------------------------
PlayerTypes CvCity::GetPlayersReligion() const
{
	VALIDATE_OBJECT
	return m_ePlayersReligion;
}


//	--------------------------------------------------------------------------------
void CvCity::SetPlayersReligion(PlayerTypes eNewValue)
{
	VALIDATE_OBJECT
	m_ePlayersReligion = eNewValue;
}

//	--------------------------------------------------------------------------------
TeamTypes CvCity::getTeam() const
{
	VALIDATE_OBJECT
	return GET_PLAYER(getOwner()).getTeam();
}


//	--------------------------------------------------------------------------------
int CvCity::getSeaPlotYield(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiSeaPlotYield[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::changeSeaPlotYield(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiSeaPlotYield.setAt(eIndex, m_aiSeaPlotYield[eIndex] + iChange);
		CvAssert(getSeaPlotYield(eIndex) >= 0);

		updateYield();
	}
}

//	--------------------------------------------------------------------------------
int CvCity::getRiverPlotYield(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiRiverPlotYield[eIndex] + GET_PLAYER(getOwner()).getRiverPlotYield(eIndex) + GET_PLAYER(getOwner()).GetPlayerTraits()->GetRiverPlotYieldChanges(eIndex);
}

//	--------------------------------------------------------------------------------
void CvCity::changeRiverPlotYield(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiRiverPlotYield.setAt(eIndex, m_aiRiverPlotYield[eIndex] + iChange);
		CvAssert(getRiverPlotYield(eIndex) >= 0);

		updateYield();
	}
}

//	--------------------------------------------------------------------------------
int CvCity::getLakePlotYield(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiLakePlotYield[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::changeLakePlotYield(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiLakePlotYield.setAt(eIndex, m_aiLakePlotYield[eIndex] + iChange);
		CvAssert(getLakePlotYield(eIndex) >= 0);

		updateYield();
	}
}

//	--------------------------------------------------------------------------------
int CvCity::getSeaResourceYield(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiSeaResourceYield[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::changeSeaResourceYield(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiSeaResourceYield.setAt(eIndex, m_aiSeaResourceYield[eIndex] + iChange);
		CvAssert(getSeaResourceYield(eIndex) >= 0);

		updateYield();
	}
}

#if defined(MOD_API_UNIFIED_YIELDS)
//	--------------------------------------------------------------------------------
/// Yield per turn from Religion
int CvCity::GetYieldPerTurnFromReligion(ReligionTypes eReligion, YieldTypes eYield) const
{
	int iYieldPerTurn = 0;
	const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion, NO_PLAYER);

	if (pReligion)
	{
		if(GetCityReligions()->IsHolyCityForReligion(eReligion))
		{
			// Only do this for food and production, other yields from religion are handled at the player level
			if (eYield == YIELD_FOOD || eYield == YIELD_PRODUCTION)
			{
				iYieldPerTurn += pReligion->m_Beliefs.GetHolyCityYieldChange(eYield);
			}
			int iHolyCityYieldPerForeignFollowers = pReligion->m_Beliefs.GetHolyCityYieldPerForeignFollowers(eYield);
			if(iHolyCityYieldPerForeignFollowers > 0 && GET_PLAYER(getOwner()).HasReligion(eReligion))
			{
				int iFollowers = GET_PLAYER(getOwner()).GetReligions()->GetNumForeignFollowers(false /*bAtPeace*/);
				iYieldPerTurn += iHolyCityYieldPerForeignFollowers * iFollowers /100;
			}
			int iHolyCityYieldPerNativeFollowers = pReligion->m_Beliefs.GetHolyCityYieldPerNativeFollowers(eYield);
			if(iHolyCityYieldPerNativeFollowers > 0 && GET_PLAYER(getOwner()).HasReligion(eReligion))
			{
				int iFollowers = GET_PLAYER(getOwner()).GetReligions()->GetNumNativeFollowers();
				iYieldPerTurn += iHolyCityYieldPerNativeFollowers * iFollowers /100;
			}
		}
		int iCityYieldPerOtherReligion = pReligion->m_Beliefs.GetCityYieldPerOtherReligion(eYield);
		if(iCityYieldPerOtherReligion != 0)
		{
			int iOtherReligions = GetCityReligions()->GetNumReligionsWithFollowers() -1;
			if(iOtherReligions > 0)
			{
				iYieldPerTurn += iCityYieldPerOtherReligion * iOtherReligions;
			}
		}
	}

	return iYieldPerTurn;
}
#endif

//	--------------------------------------------------------------------------------
int CvCity::getBaseYieldRateModifier(YieldTypes eIndex, int iExtra, CvString* toolTipSink) const
{
	VALIDATE_OBJECT
	int iModifier = 0;
	int iTempMod;
	auto& owner = GET_PLAYER(getOwner());
	CvYieldInfo* pYield = GC.getYieldInfo(eIndex);


#if defined(MOD_API_UNIFIED_YIELDS_MORE)
	if (eIndex != YIELD_HEALTH && eIndex != YIELD_FOOD )
	{
		iTempMod = GetYieldModifierFromHealth(eIndex);
		iModifier += iTempMod;
		if (iTempMod != 0 && toolTipSink)
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_FROM_HEALTH_MOD", iTempMod);
	}

	if (eIndex != YIELD_CRIME)
	{
		iTempMod = GetYieldModifierFromCrime(eIndex);
		iModifier += iTempMod;
		if (iTempMod != 0 && toolTipSink)
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_FROM_CRIME_MOD", iTempMod);
	}

	if (eIndex == YIELD_DISEASE)
	{
		iTempMod = owner.GetTrade()->GetNumTradeRoutesUsed(true)* GC.getHEALTH_DISEASE_TRADE_MOD()/100;
		iModifier += iTempMod;
		if (iTempMod != 0 && toolTipSink)
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_FROM_INTER_TRADE_MOD", iTempMod);
	}

	if (eIndex == YIELD_TOURISM)
	{
		iTempMod = owner.getPolicyModifiers(POLICYMOD_TOURISM_MODIFIER_PER_GP_CREATION) * owner.GetNumGreatPersonSincePolicy();
		iModifier += iTempMod;
		if (iTempMod != 0 && toolTipSink)
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_NUM_PER_GP_CREATION", iTempMod);
	}

	if (owner.getYieldModifierFromActiveSpies(eIndex) != 0)
	{
		iTempMod = min(100, (owner.getYieldModifierFromActiveSpies(eIndex) * owner.GetEspionage()->GetNumAssignedSpies()));
		iModifier += iTempMod;
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_SPIES", iTempMod);
	}


	
	if (eIndex == YIELD_CRIME && owner.isGoldenAge())
	{
		iTempMod = GC.getCITY_CRIME_GOLDEN_AGE_YIELD();
		iModifier += iTempMod;
		if (iTempMod != 0 && toolTipSink)
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_GOLDEN_AGE", iTempMod);
	}

	if (eIndex == YIELD_LOYALTY && owner.isGoldenAge())
	{
		iTempMod = GC.getCITY_LOYALTY_GOLDEN_AGE_YIELD();
		iModifier += iTempMod;
		if (iTempMod != 0 && toolTipSink)
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_GOLDEN_AGE", iTempMod);
	}

	if (MOD_DISEASE_BREAK)
	{
		if (eIndex == YIELD_FOOD)
		{
			iTempMod = getYieldRate(YIELD_HEALTH, false) - getYieldRate(YIELD_DISEASE, false);
			if (iTempMod < 0 && toolTipSink)
			{
				iModifier += iTempMod;
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_FROM_HEALTH_MOD", iTempMod);
			}
		}
	}

#endif


	// Yield Rate Modifier
	iTempMod = getYieldRateModifier(eIndex);
	iModifier += iTempMod;
	if(iTempMod != 0 && toolTipSink)
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD", iTempMod);
	
	//Yield Modifier from PerEra
	iTempMod = GetYieldModifierPerEra(eIndex)*(GET_PLAYER(getOwner()).GetCurrentEra()+1);
	iModifier += iTempMod;
	if(iTempMod != 0 && toolTipSink)
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_PER_ERA", iTempMod);

	// Yield Modifier from adjacent Feature
	iTempMod = getAdjacentFeaturesYieldRateModifier(eIndex);
	iModifier += iTempMod;
	if(iTempMod != 0 && toolTipSink)
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_ADJACENT_FEATURE", iTempMod);

	// Feature Yield Rate Modifier
	iTempMod = getFeatureYieldRateModifier(eIndex);
	iModifier += iTempMod;
	if (toolTipSink)
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_FEATURES", iTempMod);

	// Terrain Yield Rate Modifier
	iTempMod = getTerrainYieldRateModifier(eIndex);
	iModifier += iTempMod;
	if (toolTipSink)
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_TERRAINS", iTempMod);

	// Improvement Yield Rate Modifier
	iTempMod = getImprovementYieldRateModifier(eIndex);
	iModifier += iTempMod;
	if (toolTipSink)
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_IMPROVEMENTS", iTempMod);

	// Specialist Yield Rate Modifier
	iTempMod = getSpecialistYieldRateModifier(eIndex);
	iModifier += iTempMod;
	if (toolTipSink)
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_SPECIALIST", iTempMod);


	// Resource Yield Rate Modifier
	iTempMod = getResourceYieldRateModifier(eIndex);
	iModifier += iTempMod;
	if (iTempMod != 0 && toolTipSink)
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_RESOURCES", iTempMod);

	// Happiness Yield Rate Modifier
	iTempMod = getHappinessModifier(eIndex);
	iModifier += iTempMod;
	if(iTempMod != 0 && toolTipSink)
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_HAPPINESS", iTempMod);

	// Area Yield Rate Modifier
	if(area() != NULL)
	{
		iTempMod = area()->getYieldRateModifier(getOwner(), eIndex);
		iModifier += iTempMod;
		if(iTempMod != 0 && toolTipSink)
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_AREA", iTempMod);
	}

	// Player Yield Rate Modifier
	iTempMod = owner.getYieldRateModifier(eIndex);
	iModifier += iTempMod;
	if(iTempMod != 0 && toolTipSink)
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_PLAYER", iTempMod);

	// Player Capital Yield Rate Modifier
	if(isCapital())
	{
		iTempMod = owner.getCapitalYieldRateModifier(eIndex);
		iModifier += iTempMod;
		if(iTempMod != 0 && toolTipSink)
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_CAPITAL", iTempMod);
	}

	if (GetWeLoveTheKingDayCounter() > 0)
	{
		iTempMod = 0;
		iTempMod += owner.getCityLoveKingDayYieldMod(eIndex);
		iModifier += iTempMod;
		if (iTempMod != 0 && toolTipSink)
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_POLICY_CITY_LOVE_KING", iTempMod);
	}

	// Golden Age Yield Modifier
	if (owner.isGoldenAge())
	{
		if (pYield)
		{
			iTempMod = pYield->getGoldenAgeYieldMod();
			iTempMod += owner.getGoldenAgeYieldRateModifier(eIndex);

			iModifier += iTempMod;
			if (iTempMod != 0 && toolTipSink)
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_GOLDEN_AGE", iTempMod);
		}
	}

	// From the number of great work
#ifdef MOD_BALANCE_CORE
	if (MOD_BALANCE_CORE)
	{
		iTempMod = owner.GetYieldModifierFromNumGreakWork(pYield);
		if (iTempMod != 0)
		{
			if (toolTipSink)
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_NUM_GREAT_WORK", iTempMod);
		}
		iModifier += iTempMod;
	}
#endif

	iTempMod = owner.GetYieldModifierFromNumArtifact(pYield);
	if (iTempMod != 0)
	{
		if (toolTipSink)
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_NUM_ARTIFACT", iTempMod);
	}
	iModifier += iTempMod;

	const int iModFromHappiness = owner.GetYieldModifierFromHappiness(pYield);
	if (iModFromHappiness != 0)
	{
		iModifier += iModFromHappiness;
		if (toolTipSink)
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_HAPPINESS_GLOBAL", iModFromHappiness);
	}

	const int iModFromHappinessPolicy = owner.GetYieldModifierFromHappinessPolicy(pYield);
	if (iModFromHappinessPolicy != 0)
	{
		iModifier += iModFromHappinessPolicy;
		if (toolTipSink)
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_HAPPINESS_POLICY", iModFromHappinessPolicy);
	}

	if (getNumWorldWonders() > 0)
	{
#ifdef MOD_ROG_CORE
		if (MOD_ROG_CORE)
		{
			iTempMod = owner.getWorldWonderCityYieldRateModifier(eIndex);
			if (iTempMod != 0 && toolTipSink)
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_LOCAL_CITY_WONDER_GLOBAL", iTempMod);
			iModifier += iTempMod;
		}
#endif


		if (!owner.GetCityWithWorldWonderYieldModifier().empty())
		{
			iTempMod = 0;
			for (const auto& info : owner.GetCityWithWorldWonderYieldModifier())
			{
				if (info.eYield != eIndex) continue;
				iTempMod += info.iYield;
			}
			iModifier += iTempMod;
			if (iTempMod != 0 && toolTipSink)
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_LOCAL_CITY_WONDER", iTempMod);
		}
	}

#ifdef MOD_RESOURCE_EXTRA_BUFF
	if (MOD_RESOURCE_EXTRA_BUFF)
	{
		iTempMod = owner.GetGlobalYieldModifierFromResource(eIndex);
		iModifier += iTempMod;
		if (iTempMod != 0 && toolTipSink)
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_RESOURCE_BUFF", iTempMod);
	}
#endif
	




	int iNumTradeRoutes = owner.GetTrade()->GetNumTradeRoutesUsed(true);
	if (!owner.GetTradeRouteCityYieldModifier().empty() && iNumTradeRoutes > 0)
	{
		iTempMod = 0;
		for (const auto& info : owner.GetTradeRouteCityYieldModifier())
		{
			if (info.eYield != eIndex) continue;
			iTempMod += info.iYield * iNumTradeRoutes;
		}
		iModifier += iTempMod;
		if (iTempMod != 0 && toolTipSink)
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_POLICY_TRADE_ROUTE_NUM", iTempMod);
	}

	int iCityCount = owner.getNumCities();
	if (!owner.GetCityNumberCityYieldModifier().empty() && iCityCount != 0)
	{
		iTempMod = 0;
		for (const auto& info : owner.GetCityNumberCityYieldModifier())
		{
			if (info.eYield != eIndex) continue;
			iTempMod += info.iYield * iCityCount;
		}
		iModifier += iTempMod;
		if (iTempMod != 0 && toolTipSink)
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_POLICY_CITY_NUMBER", iTempMod);
	}

	// Religion Yield Rate Modifier
	ReligionTypes eMajority = GetCityReligions()->GetReligiousMajority();
	const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eMajority, getOwner());
	if(pReligion)
	{
		int iReligionYieldMaxFollowers = pReligion->m_Beliefs.GetMaxYieldModifierPerFollower(eIndex);
		int iFollowers = GetCityReligions()->GetNumFollowers(eMajority);
		iTempMod = 0;
		if (iReligionYieldMaxFollowers > 0)
		{
			int iReligionYieldFollowersTimes100 = pReligion->m_Beliefs.GetYieldModifierPerFollowerTimes100(eIndex) + 100;
			iFollowers = iFollowers * iReligionYieldFollowersTimes100 / 100;
			// From religion belief
			iTempMod = min(iFollowers, iReligionYieldMaxFollowers);
		}

#ifdef MOD_TRAIT_RELIGION_FOLLOWER_EFFECTS
		if (MOD_TRAIT_RELIGION_FOLLOWER_EFFECTS)
		{
			// From traits
			int iReligionFollowerYieldModifierTimes100 = iFollowers * owner.GetPerMajorReligionFollowerYieldModifierTimes100(eIndex) / 100;
			int iTraitModifierTimes100 = iFollowers * owner.GetPlayerTraits()->GetPerMajorReligionFollowerYieldModifierTimes100(eIndex) / 100;
			int iMax = owner.GetPlayerTraits()->GetPerMajorReligionFollowerYieldModifierMax(eIndex);
			if (iMax > 0) iTraitModifierTimes100 = std::min(iMax, iTraitModifierTimes100);

			iReligionFollowerYieldModifierTimes100 += iTraitModifierTimes100;
			iTempMod += iReligionFollowerYieldModifierTimes100;
		}
#endif

		iModifier += iTempMod;
		if (iTempMod != 0 && toolTipSink)
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_BELIEF", iTempMod);
	}

	// Yield Rate Modifier from City States
	iTempMod = GetCityStateTradeRouteYieldModifier(eIndex);	
	iModifier += iTempMod;
	if(iTempMod != 0 && toolTipSink)
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_MINOR_TRADE_LOCAL", iTempMod);

	// Yield Rate Modifier Global from City States
	iTempMod = owner.GetCityStateTradeRouteYieldModifierGlobal(eIndex);
	iModifier += iTempMod;
	if(iTempMod != 0 && toolTipSink)
		GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_YIELD_MINOR_TRADE_GLOBAL", iTempMod);

	// Puppet
	if(IsPuppet())
	{
		switch(eIndex)
		{
		case YIELD_SCIENCE:
			iTempMod = GC.getPUPPET_SCIENCE_MODIFIER();
			iModifier += iTempMod;
			if(iTempMod != 0 && toolTipSink)
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_PUPPET", iTempMod);
			break;
		case YIELD_GOLD:
			iTempMod = GC.getPUPPET_GOLD_MODIFIER();
			iModifier += iTempMod;
			if(iTempMod != 0 && toolTipSink)
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_PUPPET", iTempMod);
			break;
		case YIELD_TOURISM:
			iTempMod = GC.getPUPPET_TOURISM_MODIFIER();
			iModifier += iTempMod;
			if(iTempMod != 0 && toolTipSink)
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_PUPPET", iTempMod);
			break;
		case YIELD_GOLDEN_AGE_POINTS:
			iTempMod = GC.getPUPPET_GOLDEN_AGE_MODIFIER();
			iModifier += iTempMod;
			if(iTempMod != 0 && toolTipSink)
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_PUPPET", iTempMod);
			break;

#if defined(MOD_API_UNIFIED_YIELDS_MORE)
		case YIELD_GREAT_GENERAL_POINTS:
			iTempMod = GC.getPUPPET_GREAT_GENERAL_POINTS_MODIFIER();
			iModifier += iTempMod;
			if (iTempMod != 0 && toolTipSink)
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_PUPPET", iTempMod);
			break;


		case YIELD_GREAT_ADMIRAL_POINTS:
			iTempMod = GC.getPUPPET_GREAT_ADMIRAL_POINTS_MODIFIER();
			iModifier += iTempMod;
			if (iTempMod != 0 && toolTipSink)
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_PUPPET", iTempMod);
			break;


		case YIELD_HEALTH:
			iTempMod = GC.getPUPPET_HEALTH_MODIFIER();
			iModifier += iTempMod;
			if (iTempMod != 0 && toolTipSink)
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_PUPPET", iTempMod);
			break;

		case YIELD_DISEASE:
			iTempMod = GC.getPUPPET_DISEASE_MODIFIER();
			iModifier += iTempMod;
			if (iTempMod != 0 && toolTipSink)
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_PUPPET", iTempMod);
			break;


		case YIELD_CRIME:
			iTempMod = GC.getPUPPET_CRIME_MODIFIER();
			iModifier += iTempMod;
			if (iTempMod != 0 && toolTipSink)
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_PUPPET", iTempMod);
			break;


		case YIELD_LOYALTY:
			iTempMod = GC.getPUPPET_LOYALTY_MODIFIER();
			iModifier += iTempMod;
			if (iTempMod != 0 && toolTipSink)
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_PUPPET", iTempMod);
			break;



		case YIELD_SOVEREIGNTY:
			iTempMod = GC.getPUPPET_SOVEREIGNTY_MODIFIER();
			iModifier += iTempMod;
			if (iTempMod != 0 && toolTipSink)
				GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_PRODMOD_PUPPET", iTempMod);
			break;


		case YIELD_VIOLENCE:
		case YIELD_HERESY:
			break;
#endif
		}
	}

	iModifier += iExtra;

	iModifier += 100;

	// Yield Rate Multiplier
	iTempMod = getYieldRateMultiplier(eIndex);
	if(iTempMod != 0)
	{	
		iModifier *= (iTempMod + 100);
		iModifier /= 100;
		if(iTempMod != 0 && toolTipSink){
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_MULTIPLIER_YIELD", iTempMod);
		}
	}

#if defined(MOD_NUCLEAR_WINTER_FOR_SP)
	// Yield Modifier from Nuclear Winter
	iTempMod = 0;
	if(!IsNoNuclearWinterLocal()) iTempMod = GC.getGame().GetNuclearWinterYieldMultiplier(eIndex);
	if(iTempMod != 0)
	{	
		iModifier *= (iTempMod + 100);
		iModifier /= 100;
		if(iTempMod != 0 && toolTipSink){
			GC.getGame().BuildProdModHelpText(toolTipSink, "TXT_KEY_NUCLERA_WINTER_YIELD", iTempMod);
		}
	}
#endif

	// note: player->invalidateYieldRankCache() must be called for anything that is checked here
	// so if any extra checked things are added here, the cache needs to be invalidated

	return std::max(0, iModifier);
}

//	--------------------------------------------------------------------------------
int CvCity::getHappinessModifier(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	int iModifier = 0;
	CvPlayer &kPlayer = GET_PLAYER(getOwner());

	if (kPlayer.IsEmpireUnhappy())
	{
		int iUnhappy = -1 * kPlayer.GetCachedExcessHappiness();

		// Production and Gold slow down when Empire is Unhappy
		if(eIndex == YIELD_PRODUCTION)
		{
			iModifier = iUnhappy * GC.getVERY_UNHAPPY_PRODUCTION_PENALTY_PER_UNHAPPY();
			iModifier = max (iModifier, GC.getVERY_UNHAPPY_MAX_PRODUCTION_PENALTY());
		}
		else if (eIndex == YIELD_GOLD)
		{
			iModifier = iUnhappy * GC.getVERY_UNHAPPY_GOLD_PENALTY_PER_UNHAPPY();
			iModifier = max (iModifier, GC.getVERY_UNHAPPY_MAX_GOLD_PENALTY());
		}

#if defined(MOD_API_UNIFIED_YIELDS_MORE)
		else if (eIndex == YIELD_DISEASE)
		{
			iModifier = iUnhappy * GC.getVERY_UNHAPPY_DISEASE_PENALTY_PER_UNHAPPY();
			iModifier = min(iModifier, GC.getVERY_UNHAPPY_MAX_DISEASE_PENALTY());
		}

		else if (eIndex == YIELD_CRIME)
		{
			iModifier = iUnhappy * GC.getVERY_UNHAPPY_CRIME_PENALTY_PER_UNHAPPY();
			iModifier = min(iModifier, GC.getVERY_UNHAPPY_MAX_CRIME_PENALTY());
		}
#endif
	}

	return iModifier;
}

//	--------------------------------------------------------------------------------
int CvCity::getYieldRate(YieldTypes eIndex, bool bIgnoreTrade, bool bStatic) const
{
	VALIDATE_OBJECT
	if (bStatic && !bIgnoreTrade)
	{
		return (GetStaticYield(eIndex) / 100);
	}

	return (getYieldRateTimes100(eIndex, bIgnoreTrade) / 100);
}

//	--------------------------------------------------------------------------------
int CvCity::getYieldRateTimes100(YieldTypes eIndex, bool bIgnoreTrade, bool bStatic) const
{
	VALIDATE_OBJECT

	if (bStatic && !bIgnoreTrade)
	{
		return GetStaticYield(eIndex);
	}

	// Resistance - no Science, Gold or Production (Prod handled in ProductionDifference)
	if(IsResistance() || IsRazing())
	{
		if(eIndex == YIELD_GOLD || eIndex == YIELD_SCIENCE)
		{
			return 0;
		}
		if(eIndex == YIELD_TOURISM)
		{
			return 0;
		}
		if(eIndex == YIELD_GOLDEN_AGE_POINTS)
		{
			return 0;
		}
	}

	int iProcessYield = 0;

	if(getProductionToYieldModifier(eIndex) != 0)
	{
#if defined(MOD_PROCESS_STOCKPILE)
		// We want to process production to production and call it stockpiling!
		iProcessYield = getBasicYieldRateTimes100(YIELD_PRODUCTION, false, false) * (getProductionToYieldModifier(eIndex) + GetYieldFromProcessModifier(eIndex) + GET_PLAYER(getOwner()).GetYieldFromProcessModifierGlobal(eIndex)) / 100;
#else
		CvAssertMsg(eIndex != YIELD_PRODUCTION, "GAMEPLAY: should not be trying to convert Production into Production via process.");

		iProcessYield = getYieldRateTimes100(YIELD_PRODUCTION, false) * (getProductionToYieldModifier(eIndex) + GetYieldFromProcessModifier(eIndex) + GET_PLAYER(getOwner()).GetYieldFromProcessModifierGlobal(eIndex)) / 100;
#endif
	}

#if defined(MOD_PROCESS_STOCKPILE)
	int iYield = getBasicYieldRateTimes100(eIndex, bIgnoreTrade, false) + iProcessYield;
	return iYield;
}

int CvCity::getBasicYieldRateTimes100(const YieldTypes eIndex, const bool bIgnoreTrade, const bool bIgnoreFromOtherYield) const
{
#endif
	// Sum up yield rate
	int iBaseYield = getBaseYieldRate(eIndex, bIgnoreFromOtherYield) * 100;
	iBaseYield += (GetYieldPerPopTimes100(eIndex) * getPopulation());
	iBaseYield += (GET_PLAYER(getOwner()).GetYieldPerPopChange(eIndex) * getPopulation());
	iBaseYield += (GetYieldPerPopInEmpireTimes100(eIndex) * GET_PLAYER(m_eOwner).getTotalPopulation());
	iBaseYield += (GetYieldPerReligionTimes100(eIndex) * GetCityReligions()->GetNumReligionsWithFollowers());

	int iNonSpecialist = GET_PLAYER(getOwner()).getYieldFromNonSpecialistCitizens(eIndex);
	if (iNonSpecialist != 0)
	{
		int iBonusTimes100 = (iNonSpecialist * (getPopulation() - GetCityCitizens()->GetTotalSpecialistCount()));
		iBaseYield += iBonusTimes100;
	}

#ifdef MOD_BUGFIX_CITY_NEGATIVE_YIELD_MODIFIED
	int iModifiedYield = iBaseYield;
	if (!MOD_BUGFIX_CITY_NEGATIVE_YIELD_MODIFIED || iBaseYield > 0)
	{
		iModifiedYield *= getBaseYieldRateModifier(eIndex);
		iModifiedYield /= 100;
	}
#else
	int iModifiedYield = iBaseYield * getBaseYieldRateModifier(eIndex);
	iModifiedYield /= 100;
#endif

#if !defined(MOD_PROCESS_STOCKPILE)
	iModifiedYield += iProcessYield;
#endif

	if (!bIgnoreTrade)
	{
		int iTradeYield = GET_PLAYER(m_eOwner).GetTrade()->GetTradeValuesAtCityTimes100(this, eIndex);
		iModifiedYield += iTradeYield;
	}

	return iModifiedYield;
}



//	--------------------------------------------------------------------------------
int CvCity::GetResourceQuantityFromPOP(ResourceTypes eResource) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eResource >= 0, "eResource expected to be >= 0");
	CvAssertMsg(eResource < GC.getNumResourceInfos(), "Invalid resource index");
	return m_aiResourceQuantityFromPOP[eResource];
}
//	--------------------------------------------------------------------------------
void CvCity::ChangeResourceQuantityFromPOP(ResourceTypes eResource, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eResource >= 0, "eResource expected to be >= 0");
	CvAssertMsg(eResource < GC.getNumResourceInfos(), "Invalid resource index");

	if (iChange != 0)
	{
		m_aiResourceQuantityFromPOP[eResource] += iChange;
	}
}


void CvCity::UpdateCityYieldFromYield(YieldTypes eIndex1, YieldTypes eIndex2, int iModifiedYield)
{
	if (iModifiedYield > 0)
	{
		int iYieldVal = GetBuildingYieldFromYield(eIndex1, eIndex2);
		if (iYieldVal > 0)
		{
			int iBonusYield = (iModifiedYield * iYieldVal / 100);
			if (iBonusYield > 0)
			{
				SetRealYieldFromYield(eIndex1, eIndex2, iBonusYield);
			}
			else
			{
				SetRealYieldFromYield(eIndex1, eIndex2, 0);
			}
		}
		else
		{
			SetRealYieldFromYield(eIndex1, eIndex2, 0);
		}
	}
}


//	--------------------------------------------------------------------------------
int CvCity::getBaseYieldRate(YieldTypes eIndex, const bool bIgnoreFromOtherYield) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	int iValue = 0;
#if defined(MOD_GLOBAL_GREATWORK_YIELDTYPES) || defined(MOD_API_UNIFIED_YIELDS)
	iValue += GetBaseYieldRateFromGreatWorks(eIndex);
#endif
	iValue += GetBaseYieldRateFromTerrain(eIndex);
#if defined(MOD_API_UNIFIED_YIELDS)
	iValue += GetYieldPerTurnFromUnimprovedFeatures(eIndex);
#endif
	iValue += GetYieldPerTurnFromAdjacentFeatures(eIndex);

#if defined(MOD_ROG_CORE)
	iValue += GetBaseYieldRateFromCSAlliance(eIndex);
	iValue += GetBaseYieldRateFromCSFriendship(eIndex);
	iValue += GetBaseYieldRateFromEspionageSpy(eIndex);
#endif
	iValue += GetBaseYieldRateFromBuildings(eIndex);
	iValue += GetBaseYieldRateFromBuildingsPolicies(eIndex);
	iValue += GetBaseYieldRateFromSpecialists(eIndex);
	iValue += GetBaseYieldRateFromMisc(eIndex);
	iValue += GetBaseYieldRateFromReligion(eIndex);
	iValue += GetBaseYieldRateFromProjects(eIndex);
	iValue += (GetYieldPerEra(eIndex) * (GET_PLAYER(getOwner()).GetCurrentEra() + 1));

#ifdef MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD
	if (MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD && !bIgnoreFromOtherYield)
	{
		iValue += GetBaseYieldRateFromOtherYield(eIndex);
	}
#endif

	CvCity* pThisCity = const_cast<CvCity*>(this);
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		YieldTypes eIndex2 = (YieldTypes)iI;
		if (eIndex2 == NO_YIELD)
			continue;
		if (eIndex == eIndex2)
			continue;

		pThisCity->UpdateCityYieldFromYield(eIndex, eIndex2, iValue);

		//NOTE! We flip it here, because we want the OUT yield
		iValue += GetRealYieldFromYield(eIndex2, eIndex);
	}

#if defined(MOD_API_UNIFIED_YIELDS)
	if (IsRouteToCapitalConnected())
	{
		iValue += GET_PLAYER(getOwner()).GetYieldChangeTradeRoute(eIndex);
		iValue += GET_PLAYER(getOwner()).GetPlayerTraits()->GetYieldChangeTradeRoute(eIndex);
	}
#endif



#if defined(MOD_API_UNIFIED_YIELDS_MORE)
	
	if (eIndex == YIELD_HEALTH) 
	{
		if (plot()->isFreshWater())
		{
			iValue += GC.getCITY_FRESH_WATER_HEALTH_YIELD();
		}
		else
		{
			iValue += -GC.getCITY_FRESH_WATER_HEALTH_YIELD();
		}
	}


	if (eIndex == YIELD_DISEASE)
	{
		iValue += getDiseaseFromConnectionAndTradeRoute();
	}


	if (eIndex == YIELD_CRIME)
	{
		iValue += getCrimeFromSpy();
		iValue += getCrimeFromOpinion();
		iValue += getCrimeFromGarrisonedUnit();
	}


	if (eIndex == YIELD_HERESY)
	{
		iValue += getHeresyFromDiscord();
	}


	if (eIndex != YIELD_HEALTH && eIndex != YIELD_FOOD)
	{
		iValue += GetYieldFromHealth(eIndex);
	}

	iValue += GetYieldFromHappiness(eIndex);

	if (eIndex != YIELD_CRIME)
	{
		iValue += GetYieldFromCrime(eIndex);
	}

#if defined(MOD_NUCLEAR_WINTER_FOR_SP)
	if(!IsNoNuclearWinterLocal()) iValue += GC.getGame().GetYieldFromNuclearWinter(eIndex);
#endif

	if (MOD_DISEASE_BREAK)
	{
		if (eIndex == YIELD_FOOD)
		{
			int leftHealth = getYieldRate(YIELD_HEALTH, false) - getYieldRate(YIELD_DISEASE, false);
			if (leftHealth < 0)
			{
				iValue += leftHealth;
			}
		}
	}
#endif

	return iValue;
}

#if defined(MOD_API_UNIFIED_YIELDS_MORE)
int CvCity::getHeresyFromDiscord() const
{
	int  iHeresyPerTurnFromDiscord = 0;

	for (int i = 0; i < MAX_MAJOR_CIVS; i++)
	{
		PlayerTypes eTargetPlayer = (PlayerTypes)i;

		if (GET_PLAYER(eTargetPlayer).isEverAlive() && GET_PLAYER(eTargetPlayer).GetReligions()->HasCreatedReligion() && eTargetPlayer != getOwner())
		{
			iHeresyPerTurnFromDiscord += GetCityReligions()->GetNumFollowers(GET_PLAYER(eTargetPlayer).GetReligions()->GetReligionCreatedByPlayer(true));
		}

	}
	return iHeresyPerTurnFromDiscord;
}

int CvCity::getNumForeignSpy() const
{
	int iNumForeignSpy = 0;
	CvCityEspionage* pCityEspionage = GetCityEspionage();
	if (pCityEspionage)
	{
		for (int i = 0; i < MAX_MAJOR_CIVS; i++)
		{
			int iAssignedSpy = pCityEspionage->m_aiSpyAssignment[i];
			// if there is a spy in the city
			if (iAssignedSpy != -1 && !GET_PLAYER((PlayerTypes)i).GetEspionage()->IsDiplomat(iAssignedSpy))
			{
				if ((PlayerTypes)i != getOwner())
				{
					iNumForeignSpy += 1;
				}
			}
		}
	}
	return iNumForeignSpy;
}


int CvCity::getCrimeFromSpy() const
{
	int iNumOtherSpy = getNumForeignSpy();

	if (GetCityEspionage()->m_aiSpyAssignment[getOwner()] != -1)
	{
		iNumOtherSpy -= 1;
	}

	return iNumOtherSpy* GC.getCITY_CRIME_SPY_YIELD();
}

int CvCity::getCrimeFromOpinion() const
{
	int iCrimeFromOpinion = 0;

	PublicOpinionTypes eOpinionInMyCiv = GET_PLAYER(getOwner()).GetCulture()->GetPublicOpinionType();

	if (eOpinionInMyCiv == PUBLIC_OPINION_REVOLUTIONARY_WAVE)
	{
		iCrimeFromOpinion = GC.getCITY_CRIME_OPINION_REVOLUTIONARY_WAVE_YIELD();
	}
	else if (eOpinionInMyCiv == PUBLIC_OPINION_CIVIL_RESISTANCE)
	{
		iCrimeFromOpinion = GC.getCITY_CRIME_OPINION_CIVIL_RESISTANCE_YIELD();
	}
	else if (eOpinionInMyCiv == PUBLIC_OPINION_DISSIDENTS)
	{
		iCrimeFromOpinion = GC.getCITY_CRIME_OPINION_DISSIDENTS_YIELD();
	}
	return iCrimeFromOpinion;
}

int CvCity::getCrimeFromGarrisonedUnit() const
{
	int iCrimeFromGarrisonedUnit = 0;
	// Garrisoned Unit
	CvUnit* pGarrisonedUnit = GetGarrisonedUnit();
	if (pGarrisonedUnit)
	{
		int iGarrisonedStrength = pGarrisonedUnit->GetBaseCombatStrength()/5;
		iGarrisonedStrength = std::min(200, (std::max(5, iGarrisonedStrength)));
		iCrimeFromGarrisonedUnit -= iGarrisonedStrength;
	}
	return iCrimeFromGarrisonedUnit;
}


int CvCity::getDiseaseFromConnectionAndTradeRoute() const
{
	int DiseaseFromConnectionAndTradeRoute = 0;
	DiseaseFromConnectionAndTradeRoute += getTradeRouteNum() * getPopulation() * GC.getHEALTH_DISEASE_CONNECTION_MOD() / 100;
	return DiseaseFromConnectionAndTradeRoute;
}

int CvCity::getTradeRouteNum() const
{
	int num = 0;
	CvGameTrade* pTrade = GC.getGame().GetGameTrade();
	for (uint ui = 0; ui < pTrade->m_aTradeConnections.size(); ui++)
	{
		if (pTrade->IsTradeRouteIndexEmpty(ui))
		{
			continue;
		}
		TradeConnection* pConnection = &(pTrade->m_aTradeConnections[ui]);
		CvCity* pFromCity = GC.getMap().plot(pConnection->m_iOriginX, pConnection->m_iOriginY)->getPlotCity();
		CvCity* pToCity = GC.getMap().plot(pConnection->m_iDestX, pConnection->m_iDestY)->getPlotCity();

		if (pFromCity && isThis(*pFromCity)) num+=1;
		else if (pToCity && isThis(*pToCity)) num+=1;
	}
	return num;
}
#endif

//	--------------------------------------------------------------------------------
CvString CvCity::getYieldRateInfoTool(YieldTypes eIndex, bool bIgnoreTrade) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	CvString szRtnValue = "";
	if(IsResistance() || IsRazing())
	{
		if(eIndex == YIELD_GOLD || eIndex == YIELD_SCIENCE || eIndex == YIELD_TOURISM || eIndex == YIELD_GOLDEN_AGE_POINTS)
		{
			return szRtnValue;
		}
	}

	const char* YieldIcon = GC.getYieldInfo(eIndex)->getIconString();
	int iBaseValue = 0;
	if(getProductionToYieldModifier(eIndex) != 0)
	{
#if defined(MOD_PROCESS_STOCKPILE)
		// We want to process production to production and call it stockpiling!
		iBaseValue = getBasicYieldRateTimes100(YIELD_PRODUCTION, false, false) * (getProductionToYieldModifier(eIndex) + GetYieldFromProcessModifier(eIndex) + GET_PLAYER(getOwner()).GetYieldFromProcessModifierGlobal(eIndex)) / 100;
#else
		CvAssertMsg(eIndex != YIELD_PRODUCTION, "GAMEPLAY: should not be trying to convert Production into Production via process.");

		iBaseValue = getYieldRateTimes100(YIELD_PRODUCTION, false) * (getProductionToYieldModifier(eIndex) + GetYieldFromProcessModifier(eIndex) + GET_PLAYER(getOwner()).GetYieldFromProcessModifierGlobal(eIndex)) / 100;
#endif
	}
	if(iBaseValue != 0)
	{
		double iBaseYieldTimes100 = iBaseValue;
		iBaseYieldTimes100 /= 100;
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_CITY_PRODUCTION_TO", iBaseYieldTimes100, YieldIcon);
	}
	//Yield from Policy
	iBaseValue = GetBaseYieldRateFromPolicy(eIndex);
	if(iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_POLICY", iBaseValue, YieldIcon);
	}
#if defined(MOD_GLOBAL_GREATWORK_YIELDTYPES) || defined(MOD_API_UNIFIED_YIELDS)
	iBaseValue = GetBaseYieldRateFromGreatWorks(eIndex);
	if(iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_GREATWORK", iBaseValue, YieldIcon);
	}
#endif
	iBaseValue = GetBaseYieldRateFromTerrain(eIndex);
	if(iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_TERRAIN", iBaseValue, YieldIcon);
	}
#if defined(MOD_API_UNIFIED_YIELDS)
	iBaseValue = GetYieldPerTurnFromUnimprovedFeatures(eIndex);
	if(iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_UNIMPROVEMENT_FEATURES", iBaseValue, YieldIcon);
	}
#endif
	iBaseValue = GetYieldPerTurnFromAdjacentFeatures(eIndex);
	if(iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_PER_ADJACENT_FEATURES", iBaseValue, YieldIcon);
	}

	//Special case: Yield from trait
	iBaseValue = GetBaseYieldRateFromTrait(eIndex);
	if(iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_TRAITS", iBaseValue, YieldIcon);
	}
	//Special case: Yield from Leagues
	iBaseValue = GetBaseYieldRateFromLeagues(eIndex);
	if(iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_LEAGUES", iBaseValue, YieldIcon);
	}
	iBaseValue = GetBaseYieldRateFromBuildings(eIndex);
	if(iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_BUILDINGS", iBaseValue, YieldIcon);
	}
	iBaseValue = GetBaseYieldRateFromSpecialists(eIndex);
	if(iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_SPECIALISTS", iBaseValue, YieldIcon);
	}
	iBaseValue = GetBaseYieldRateFromMisc(eIndex);
	if(iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_MISC", iBaseValue, YieldIcon);
	}

	iBaseValue = GetBaseYieldRateFromProjects(eIndex);
	if (iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_PROJECTS", iBaseValue, YieldIcon);
	}

	//Special case for FAITH and Culture
	iBaseValue = GetBaseYieldRateFromReligionAllCase(eIndex);
	if(iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_RELIGION", iBaseValue, YieldIcon);
	}

#ifdef MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD
	if (MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD)
	{
		iBaseValue = GetBaseYieldRateFromOtherYield(eIndex);
		for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			YieldTypes eIndex2 = (YieldTypes)iI;
			if (eIndex2 == NO_YIELD)
				continue;
			if (eIndex == eIndex2)
				continue;

			//NOTE! We flip it here, because we want the OUT yield
			iBaseValue += GetRealYieldFromYield(eIndex2, eIndex);
		}

		if(iBaseValue != 0)
		{
			szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_OTHER_YIELD", iBaseValue, YieldIcon);
		}
	}
#endif





#if defined(MOD_API_UNIFIED_YIELDS)
	if (IsRouteToCapitalConnected())
	{
		iBaseValue = GET_PLAYER(getOwner()).GetYieldChangeTradeRoute(eIndex);
		iBaseValue += GET_PLAYER(getOwner()).GetPlayerTraits()->GetYieldChangeTradeRoute(eIndex);
		if(iBaseValue != 0)
		{
			szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_TRADE_ROUTE", iBaseValue, YieldIcon);
		}
	}
#endif

#if defined(MOD_API_UNIFIED_YIELDS_MORE)


	if (eIndex == YIELD_HEALTH) 
	{
		if (plot()->isFreshWater())
		{
			iBaseValue = GC.getCITY_FRESH_WATER_HEALTH_YIELD();
			szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FRESH_WATER", iBaseValue, YieldIcon);
		}
		else
		{
			iBaseValue = -GC.getCITY_FRESH_WATER_HEALTH_YIELD();
			szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_NO_FRESH_WATER", iBaseValue, YieldIcon);
		}
	}

	if (eIndex == YIELD_CRIME)
	{

		iBaseValue = getCrimeFromSpy();
		if (iBaseValue != 0)
		{
			szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_SPIES", iBaseValue, YieldIcon);
		}

		iBaseValue = getCrimeFromOpinion();
		if (iBaseValue != 0)
		{
			szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_OPINION", iBaseValue, YieldIcon);
		}

		iBaseValue = getCrimeFromGarrisonedUnit();
		if (iBaseValue != 0)
		{
			szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_GARRISONED_UNIT", iBaseValue, YieldIcon);
		}
	}

	if (eIndex == YIELD_HERESY)
	{
		iBaseValue = getHeresyFromDiscord();
		if (iBaseValue != 0)
		{
			szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_DISCORD", iBaseValue, YieldIcon);
		}
	}


	if (eIndex != YIELD_HEALTH && eIndex != YIELD_FOOD)
	{
		iBaseValue = GetYieldFromHealth(eIndex);
		if (iBaseValue != 0)
		{
			szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_HEALTH", iBaseValue, YieldIcon);
		}
	}


	if (eIndex == YIELD_DISEASE)
	{
		iBaseValue = getDiseaseFromConnectionAndTradeRoute();
		if (iBaseValue != 0)
		{
			szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_CITY_CONNECTION", iBaseValue, YieldIcon);
		}
	}

	if (MOD_DISEASE_BREAK)
	{
		if (eIndex == YIELD_FOOD)
		{
			iBaseValue = getYieldRate(YIELD_HEALTH, false) - getYieldRate(YIELD_DISEASE, false);
			if (iBaseValue < 0)
			{
				szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_HEALTH", iBaseValue, YieldIcon);
			}
		}
	}

	iBaseValue = GetYieldFromHappiness(eIndex);
	if(iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_HAPPINESS", iBaseValue, YieldIcon);
	}

	if (eIndex != YIELD_CRIME)
	{
		iBaseValue = GetYieldFromCrime(eIndex);
		if(iBaseValue != 0)
		{
			szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_CRIME", iBaseValue, YieldIcon);
		}
	}
#endif

#if defined(MOD_NUCLEAR_WINTER_FOR_SP)
	iBaseValue = 0;
	if(!IsNoNuclearWinterLocal()) iBaseValue = GC.getGame().GetYieldFromNuclearWinter(eIndex);
	if(iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_NUCLEAR_WINTER", iBaseValue, YieldIcon);
	}
#endif

	double iBaseYieldTimes100 = 0.0;
	iBaseValue = ((GetYieldPerPopTimes100(eIndex)+ GET_PLAYER(getOwner()).GetYieldPerPopChange(eIndex)) * getPopulation());
	if(iBaseValue != 0)
	{
		iBaseYieldTimes100 = iBaseValue;
		iBaseYieldTimes100 /= 100;
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_POPULATION", iBaseYieldTimes100, YieldIcon);
	}

	iBaseValue = (GetYieldPerPopInEmpireTimes100(eIndex) * GET_PLAYER(m_eOwner).getTotalPopulation());
	if(iBaseValue != 0)
	{
		iBaseYieldTimes100 = iBaseValue;
		iBaseYieldTimes100 /= 100;
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_TOTAL_POPULATION", iBaseYieldTimes100, YieldIcon);
	}

	iBaseValue = GetYieldPerReligionTimes100(eIndex) * GetCityReligions()->GetNumReligionsWithFollowers();
	if(iBaseValue != 0)
	{
		iBaseYieldTimes100 = iBaseValue;
		iBaseYieldTimes100 /= 100;
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_RELOGION_POPULATION", iBaseYieldTimes100, YieldIcon);
	}

	iBaseValue = GetYieldPerEra(eIndex) * (GET_PLAYER(getOwner()).GetCurrentEra() + 1) * 100;
	if(iBaseValue != 0)
	{
		iBaseYieldTimes100 = iBaseValue;
		iBaseYieldTimes100 /= 100;
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_ERA", iBaseYieldTimes100, YieldIcon);
	}

#if defined(MOD_ROG_CORE)
	iBaseValue = (GET_PLAYER(getOwner()).getYieldFromNonSpecialistCitizens(eIndex)) * (getPopulation() - GetCityCitizens()->GetTotalSpecialistCount());
	if (iBaseValue != 0)
	{
		iBaseYieldTimes100 = iBaseValue;
		iBaseYieldTimes100 /= 100;
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_NON_SPECIALIST_CITIZENS", iBaseYieldTimes100, YieldIcon);
	}

	iBaseValue = GetBaseYieldRateFromEspionageSpy(eIndex);
	if (iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_ESPIONAGE_SPY", iBaseValue, YieldIcon);
	}

	iBaseValue = GetBaseYieldRateFromCSFriendship(eIndex);
	if (iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_CS_FRIENDSHIP", iBaseValue, YieldIcon);
	}

	iBaseValue = GetBaseYieldRateFromCSAlliance(eIndex);
	if (iBaseValue != 0)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_CS_ALLIANCE", iBaseValue, YieldIcon);
	}
#endif

	iBaseValue = GET_PLAYER(m_eOwner).GetTrade()->GetTradeValuesAtCityTimes100(this, eIndex);
	if(iBaseValue != 0 && !bIgnoreTrade)
	{
		szRtnValue += GetLocalizedText("TXT_KEY_CITYVIEW_BASE_YIELD_TT_FROM_TRADE_VALUE", iBaseValue, YieldIcon);
	}
	return szRtnValue;
}

#if defined(MOD_GLOBAL_GREATWORK_YIELDTYPES) || defined(MOD_API_UNIFIED_YIELDS)
//	--------------------------------------------------------------------------------
/// Base yield rate from Great Works
int CvCity::GetBaseYieldRateFromGreatWorks(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	return GetCityBuildings()->GetYieldFromGreatWorks(eIndex);
}
#endif

//	--------------------------------------------------------------------------------
/// Base yield rate from Policy --TODO this is a very simple func now
int CvCity::GetBaseYieldRateFromPolicy(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	int iRtnValue = 0;
	switch (eIndex)
	{
	case YIELD_CULTURE:
		iRtnValue += GetJONSCulturePerTurnFromPolicies();
		break;
	default:
		iRtnValue += GetBaseYieldRateFromBuildingsPolicies(eIndex);
		break;
	}
	return iRtnValue;

}
//	--------------------------------------------------------------------------------
/// Base yield rate from Trait --TODO this is a very simple func now
int CvCity::GetBaseYieldRateFromTrait(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	switch (eIndex)
	{
	case YIELD_CULTURE:
		return GetJONSCulturePerTurnFromTraits();
		break;
	default:
		return 0;
		break;
	}

}
//	--------------------------------------------------------------------------------
/// Base yield rate from Religion Special Case --TODO this is a very simple func now
int CvCity::GetBaseYieldRateFromReligionAllCase(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	return GetBaseYieldRateFromReligion(eIndex);
}
//	--------------------------------------------------------------------------------
/// Base yield rate from Leagues --TODO this is a very simple func now
int CvCity::GetBaseYieldRateFromLeagues(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	switch (eIndex)
	{
	case YIELD_CULTURE:
		return GetJONSCulturePerTurnFromLeagues();
		break;
	default:
		return 0;
		break;
	}

}
//	--------------------------------------------------------------------------------
/// Base yield rate from Terrain
int CvCity::GetBaseYieldRateFromTerrain(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	return m_aiBaseYieldRateFromTerrain[eIndex];
}

//	--------------------------------------------------------------------------------
/// Base yield rate from Terrain
void CvCity::ChangeBaseYieldRateFromTerrain(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiBaseYieldRateFromTerrain.setAt(eIndex, m_aiBaseYieldRateFromTerrain[eIndex] + iChange);

		// JAR - debugging
		s_lastYieldUsedToUpdateRateFromTerrain = eIndex;
		s_changeYieldFromTerreain = iChange;


		if(getTeam() == GC.getGame().getActiveTeam())
		{
			if(isCitySelected())
			{
				DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
			}
		}
	}
}

//	--------------------------------------------------------------------------------
/// Base yield rate from Buildings
int CvCity::GetBaseYieldRateFromBuildings(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	return m_aiBaseYieldRateFromBuildings[eIndex];
}

//	--------------------------------------------------------------------------------
/// Base yield rate from Buildings
void CvCity::ChangeBaseYieldRateFromBuildings(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiBaseYieldRateFromBuildings.setAt(eIndex, m_aiBaseYieldRateFromBuildings[eIndex] + iChange);

		if(getTeam() == GC.getGame().getActiveTeam())
		{
			if(isCitySelected())
			{
				DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
				//DLLUI->setDirty(InfoPane_DIRTY_BIT, true );
			}
		}
	}
}

//	--------------------------------------------------------------------------------
/// Base yield rate from Buildings with Policy bonus
int CvCity::GetBaseYieldRateFromBuildingsPolicies(YieldTypes eIndex) const
{
	return m_aiBaseYieldRateFromBuildingsPolicies[eIndex];
}

//	--------------------------------------------------------------------------------
/// Base yield rate from Buildings with Policy bonus
void CvCity::ChangeBaseYieldRateFromBuildingsPolicies(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiBaseYieldRateFromBuildingsPolicies.setAt(eIndex, m_aiBaseYieldRateFromBuildingsPolicies[eIndex] + iChange);

		if(getTeam() == GC.getGame().getActiveTeam())
		{
			if(isCitySelected())
			{
				DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
			}
		}
	}
}

//	--------------------------------------------------------------------------------
/// Base yield rate from Specialists
int CvCity::GetBaseYieldRateFromSpecialists(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	return m_aiBaseYieldRateFromSpecialists[eIndex];
}

//	--------------------------------------------------------------------------------
/// Base yield rate from Specialists
void CvCity::ChangeBaseYieldRateFromSpecialists(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiBaseYieldRateFromSpecialists.setAt(eIndex, m_aiBaseYieldRateFromSpecialists[eIndex] + iChange);

		if(getTeam() == GC.getGame().getActiveTeam())
		{
			if(isCitySelected())
			{
				DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
			}
		}
	}
}

//	--------------------------------------------------------------------------------
/// Base yield rate from Misc
int CvCity::GetBaseYieldRateFromMisc(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	return m_aiBaseYieldRateFromMisc[eIndex];
}

//	--------------------------------------------------------------------------------
/// Base yield rate from Misc
void CvCity::ChangeBaseYieldRateFromMisc(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiBaseYieldRateFromMisc.setAt(eIndex, m_aiBaseYieldRateFromMisc[eIndex] + iChange);

		if(getTeam() == GC.getGame().getActiveTeam())
		{
			if(isCitySelected())
			{
				DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
			}
		}
	}
}


int CvCity::GetYieldFromInternalTR(YieldTypes eIndex1) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex1 >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex1 < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	return m_aiYieldFromInternalTR[eIndex1];
}

//	--------------------------------------------------------------------------------
/// Extra yield from building
void CvCity::ChangeYieldFromInternalTR(YieldTypes eIndex1, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex1 >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex1 < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	if (iChange != 0)
	{
		m_aiYieldFromInternalTR[eIndex1] = m_aiYieldFromInternalTR[eIndex1] + iChange;
		CvAssert(GetYieldFromInternalTR(eIndex1) >= 0);
	}
}

//	--------------------------------------------------------------------------------
/// process Extra yield from building
int CvCity::GetYieldFromProcessModifier(YieldTypes eIndex1) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex1 >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex1 < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	return m_aiYieldFromProcessModifier[eIndex1];
}

//	--------------------------------------------------------------------------------
/// Extra yield from building
void CvCity::ChangeYieldFromProcessModifier(YieldTypes eIndex1, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex1 >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex1 < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	if (iChange != 0)
	{
		m_aiYieldFromProcessModifier.setAt(eIndex1, m_aiYieldFromProcessModifier[eIndex1] + iChange);
		CvAssert(GetYieldFromProcessModifier(eIndex) >= 0);
	}
}

/// Extra yield from building
int CvCity::GetSpecialistRateModifier(SpecialistTypes eSpecialist) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eSpecialist >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eSpecialist < GC.getNumSpecialistInfos(), "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiSpecialistRateModifier[eSpecialist];
}

//	--------------------------------------------------------------------------------
/// Extra yield from building
void CvCity::ChangeSpecialistRateModifier(SpecialistTypes eSpecialist, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eSpecialist >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eSpecialist < GC.getNumSpecialistInfos(), "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiSpecialistRateModifier[eSpecialist] = m_aiSpecialistRateModifier[eSpecialist] + iChange;
		CvAssert(GetSpecialistRateModifier(eSpecialist) >= 0);
	}
}

//	--------------------------------------------------------------------------------
/// Base yield rate from Religion
int CvCity::GetBaseYieldRateFromReligion(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

#if defined(MOD_API_UNIFIED_YIELDS)
	int iBaseYield = m_aiBaseYieldRateFromReligion[eIndex];

	// This will only return a value for food and production
	iBaseYield += GetYieldPerTurnFromReligion(GetCityReligions()->GetReligiousMajority(), eIndex);
		
	return iBaseYield;
#else
	return m_aiBaseYieldRateFromReligion[eIndex];
#endif
}

#ifdef MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD
int CvCity::GetBaseYieldRateFromOtherYield(YieldTypes eYield) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	int iResult = 0;

	if (!m_bHasYieldFromOtherYield)
	{
		return 0;
	}

	for (size_t iInYield = YIELD_FOOD; iInYield < NUM_YIELD_TYPES; iInYield++)
	{
		
		const YieldTypes eInYield = static_cast<YieldTypes>(iInYield);
		if (eInYield == eYield)
		{
			// It is disabled to get yields from the yields
			continue;
		}
		for (auto& ite = m_ppiYieldFromOtherYield.begin(); ite != m_ppiYieldFromOtherYield.end(); ite++) {
			if ((*ite)[YieldFromYieldStruct::IN_YIELD_TYPE] == eInYield && (*ite)[YieldFromYieldStruct::OUT_YIELD_TYPE] == eYield) {
				const int iInputThreshold = (*ite)[YieldFromYieldStruct::IN_YIELD_VALUE];
				
				if (iInputThreshold <= 0)
				{
					continue;
				}
				const int iInYieldValue = getBasicYieldRateTimes100(eInYield, false, true) / 100;
				const int iInCount = iInYieldValue / iInputThreshold;
				if (iInCount != 0)
				{
					const int iOutUnit = (*ite)[YieldFromYieldStruct::OUT_YIELD_VALUE];
					iResult += iInCount * iOutUnit;
				}
			}
		}
	}

	return iResult;
}
#endif

//	--------------------------------------------------------------------------------
/// Base yield rate from Religion
void CvCity::ChangeBaseYieldRateFromReligion(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiBaseYieldRateFromReligion[eIndex] = m_aiBaseYieldRateFromReligion[eIndex] + iChange;

		if(getTeam() == GC.getGame().getActiveTeam())
		{
			if(isCitySelected())
			{
				DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
			}
		}
	}
}


void CvCity::changeNukeInterceptionChance(int iNewValue)
{
	VALIDATE_OBJECT
		m_iNukeInterceptionChance += iNewValue;
}
int CvCity::getNukeInterceptionChance() const
{
	VALIDATE_OBJECT
		return m_iNukeInterceptionChance;
}

//	--------------------------------------------------------------------------------
/// Extra yield for each pop point
int CvCity::GetYieldPerPopTimes100(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	return m_aiYieldPerPop[eIndex];
}

//	--------------------------------------------------------------------------------
/// Extra yield for each pop point
void CvCity::ChangeYieldPerPopTimes100(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
		m_aiYieldPerPop.setAt(eIndex, m_aiYieldPerPop[eIndex] + iChange);
}

//	--------------------------------------------------------------------------------
/// Extra yield for each pop point Global
int CvCity::GetYieldPerPopInEmpireTimes100(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	return m_aiYieldPerPopInEmpire[eIndex];
}

//	--------------------------------------------------------------------------------
/// Extra yield for each pop point Global
void CvCity::ChangeYieldPerPopInEmpireTimes100(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
		m_aiYieldPerPopInEmpire.setAt(eIndex, m_aiYieldPerPopInEmpire[eIndex] + iChange);
}

//	--------------------------------------------------------------------------------
#if defined(MOD_ROG_CORE)
//	--------------------------------------------------------------------------------
/// Extra yield from building
int CvCity::GetYieldPerAlly(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiYieldPerAlly[eIndex];
}

//	--------------------------------------------------------------------------------
/// Extra yield from building
void CvCity::ChangeYieldPerAlly(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiYieldPerAlly[eIndex] = m_aiYieldPerAlly[eIndex] + iChange;
		CvAssert(GetYieldPerAlly(eIndex) >= 0);
	}
}
//	--------------------------------------------------------------------------------
/// Extra yield from building
int CvCity::GetYieldPerFriend(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiYieldPerFriend[eIndex];
}

//	--------------------------------------------------------------------------------
/// Extra yield from building
void CvCity::ChangeYieldPerFriend(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiYieldPerFriend[eIndex] = m_aiYieldPerFriend[eIndex] + iChange;
		CvAssert(GetYieldPerFriend(eIndex) >= 0);
	}
}

//	--------------------------------------------------------------------------------
/// Extra yield from building
int CvCity::GetRealYieldFromYield(YieldTypes eIndex1, YieldTypes eIndex2) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex1 >= 0, "eIndex1 expected to be >= 0");
	CvAssertMsg(eIndex1 < NUM_YIELD_TYPES, "eIndex1 expected to be < NUM_YIELD_TYPES");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 expected to be >= 0");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 expected to be < NUM_YIELD_TYPES");

	return ModifierLookup(m_yieldChanges[eIndex2].forActualYield, eIndex1);
}

//	--------------------------------------------------------------------------------
/// Extra yield from building
void CvCity::SetRealYieldFromYield(YieldTypes eIndex1, YieldTypes eIndex2, int iValue)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex1 >= 0, "eIndex1 expected to be >= 0");
	CvAssertMsg(eIndex1 < NUM_YIELD_TYPES, "eIndex1 expected to be < NUM_YIELD_TYPES");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 expected to be >= 0");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 expected to be < NUM_YIELD_TYPES");

	ModifierUpdateInsertRemove(m_yieldChanges[eIndex2].forActualYield, eIndex1, iValue, false);
}
//	--------------------------------------------------------------------------------
/// Extra yield from building
int CvCity::GetBuildingYieldFromYield(YieldTypes eIndex1, YieldTypes eIndex2) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex1 < NUM_YIELD_TYPES, "eIndex1 is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");

	return ModifierLookup(m_yieldChanges[eIndex2].forYield, eIndex1);
}

//	--------------------------------------------------------------------------------
/// Extra yield from building
void CvCity::ChangeBuildingYieldFromYield(YieldTypes eIndex1, YieldTypes eIndex2, int iValue)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex1 < NUM_YIELD_TYPES, "eIndex1 is expected to be within maximum bounds (invalid Index)");
	CvAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");

	ModifierUpdateInsertRemove(m_yieldChanges[eIndex2].forYield, eIndex1, iValue, true);
}

//	--------------------------------------------------------------------------------
/// Base yield rate from CS Alliances
int CvCity::GetBaseYieldRateFromCSAlliance(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	int iValue = 0;
	if (GetYieldPerAlly(eIndex) != 0)
	{
		iValue += (GetYieldPerAlly(eIndex) * GET_PLAYER(getOwner()).GetNumCSAllies());
	}
	return (m_aiBaseYieldRateFromCSAlliance[eIndex] + iValue);
}
//	--------------------------------------------------------------------------------
/// Base yield rate from CS Alliances
void CvCity::ChangeBaseYieldRateFromCSAlliance(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiBaseYieldRateFromCSAlliance[eIndex] = m_aiBaseYieldRateFromCSAlliance[eIndex] + iChange;
		CvAssert(GetBaseYieldRateFromCSAlliance(eIndex) >= 0);
	}
}
void CvCity::SetBaseYieldRateFromCSAlliance(YieldTypes eIndex, int iValue)
{
	if (GetBaseYieldRateFromCSAlliance(eIndex) != iValue)
	{
		m_aiBaseYieldRateFromCSAlliance[eIndex] = iValue;
	}
}
//	--------------------------------------------------------------------------------
/// Base yield rate from CS Friendships
int CvCity::GetBaseYieldRateFromCSFriendship(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	int iValue = 0;
	if (GetYieldPerFriend(eIndex) != 0)
	{
		iValue += (GetYieldPerFriend(eIndex) * GET_PLAYER(getOwner()).GetNumCSFriends());
	}
	return (m_aiBaseYieldRateFromCSFriendship[eIndex] + iValue);
}
void CvCity::ChangeBaseYieldRateFromCSFriendship(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiBaseYieldRateFromCSFriendship[eIndex] = m_aiBaseYieldRateFromCSFriendship[eIndex] + iChange;
		CvAssert(GetBaseYieldRateFromCSFriendship(eIndex) >= 0);
	}
}
void CvCity::SetBaseYieldRateFromCSFriendship(YieldTypes eIndex, int iValue)
{
	if (GetBaseYieldRateFromCSFriendship(eIndex) != iValue)
	{
		m_aiBaseYieldRateFromCSFriendship[eIndex] = iValue;
	}
}
//	--------------------------------------------------------------------------------
/// Extra yield from building
int CvCity::GetYieldPerEspionageSpy(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiYieldPerEspionageSpy[eIndex];
}

//	--------------------------------------------------------------------------------
/// Extra yield from building
void CvCity::ChangeYieldPerEspionageSpy(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiYieldPerEspionageSpy[eIndex] = m_aiYieldPerEspionageSpy[eIndex] + iChange;
		CvAssert(GetYieldPerEspionageSpy(eIndex) >= 0);
	}
}



/// Base yield rate from Espionage Spy
int CvCity::GetBaseYieldRateFromEspionageSpy(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	int iValue = 0;
	int iValueSpy = 0;
	if (GetYieldPerEspionageSpy(eIndex) != 0  &&  GET_PLAYER(getOwner()).GetEspionage()->GetNumSpies() > 0)
	{

		CvPlayerEspionage* pkPlayerEspionage = GET_PLAYER(getOwner()).GetEspionage();

		for (uint uiSpy = 0; uiSpy < pkPlayerEspionage->m_aSpyList.size(); ++uiSpy)
		{
			CvEspionageSpy* pSpy = &(pkPlayerEspionage->m_aSpyList[uiSpy]);

			CvPlot* pCityPlot = GC.getMap().plot(pSpy->m_iCityX, pSpy->m_iCityY);
			if (pCityPlot != NULL)
			{
				CvCity* pCity = pCityPlot->getPlotCity();
				if (pCity != NULL && pCity->getOwner() != getOwner())
				{
					iValueSpy = iValueSpy + max(0, pCity->getYieldRate(eIndex, false));
				}
			}
		}
		iValue += (GetYieldPerEspionageSpy(eIndex) * iValueSpy/100);
	}
	return (m_aiBaseYieldRateFromEspionageSpy[eIndex] + iValue);
}
//	--------------------------------------------------------------------------------
/// Base yield rate from CS Alliances
void CvCity::ChangeBaseYieldRateFromEspionageSpy(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiBaseYieldRateFromEspionageSpy[eIndex] = m_aiBaseYieldRateFromEspionageSpy[eIndex] + iChange;
		CvAssert(GetBaseYieldRateFromEspionageSpy(eIndex) >= 0);
	}
}
void CvCity::SetBaseYieldRateFromEspionageSpy(YieldTypes eIndex, int iValue)
{
	if (GetBaseYieldRateFromEspionageSpy(eIndex) != iValue)
	{
		m_aiBaseYieldRateFromEspionageSpy[eIndex] = iValue;
	}
}


/// Extra yield from building
int CvCity::GetYieldFromConstruction(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiYieldFromConstruction[eIndex];
}

//	--------------------------------------------------------------------------------
/// Extra yield from building
void CvCity::ChangeYieldFromConstruction(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiYieldFromConstruction[eIndex] = m_aiYieldFromConstruction[eIndex] + iChange;
		CvAssert(GetYieldFromConstruction(eIndex) >= 0);
	}
}
//	--------------------------------------------------------------------------------
/// Extra yield from building
int CvCity::GetYieldFromUnitProduction(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiYieldFromUnitProduction[eIndex];
}

//	--------------------------------------------------------------------------------
/// Extra yield from building
void CvCity::ChangeYieldFromUnitProduction(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiYieldFromUnitProduction[eIndex] = m_aiYieldFromUnitProduction[eIndex] + iChange;
		CvAssert(GetYieldFromUnitProduction(eIndex) >= 0);
	}
}

//	--------------------------------------------------------------------------------
/// Extra yield from building
int CvCity::GetYieldFromBirth(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiYieldFromBirth[eIndex];
}

//	--------------------------------------------------------------------------------
/// Extra yield from building
void CvCity::ChangeYieldFromBirth(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiYieldFromBirth[eIndex] = m_aiYieldFromBirth[eIndex] + iChange;
		CvAssert(GetYieldFromBirth(eIndex) >= 0);
	}
}
//	--------------------------------------------------------------------------------
/// Extra yield from building
int CvCity::GetYieldFromBorderGrowth(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiYieldFromBorderGrowth[eIndex];
}

//	--------------------------------------------------------------------------------
/// Extra yield from building
void CvCity::ChangeYieldFromBorderGrowth(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiYieldFromBorderGrowth[eIndex] = m_aiYieldFromBorderGrowth[eIndex] + iChange;
		CvAssert(GetYieldFromBorderGrowth(eIndex) >= 0);
	}
}
//	--------------------------------------------------------------------------------

int CvCity::GetYieldFromPillage(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiYieldFromPillage[eIndex];
}

//	--------------------------------------------------------------------------------
/// Extra yield from building
void CvCity::ChangeYieldFromPillage(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiYieldFromPillage[eIndex] = m_aiYieldFromPillage[eIndex] + iChange;
		CvAssert(GetYieldFromPillage(eIndex) >= 0);
	}
}
#endif

//	--------------------------------------------------------------------------------
/// Extra yield for each religion with a follower
int CvCity::GetYieldPerReligionTimes100(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	return m_aiYieldPerReligion[eIndex] + GET_PLAYER(getOwner()).GetYieldChangesPerReligionTimes100(eIndex);
}

//	--------------------------------------------------------------------------------
/// Extra yield for each religion with a follower
void CvCity::ChangeYieldPerReligionTimes100(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiYieldPerReligion[eIndex] = m_aiYieldPerReligion[eIndex] + iChange;
	}
}

//	--------------------------------------------------------------------------------
/// Extra yield for each era
int CvCity::GetYieldPerEra(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	return m_aiYieldPerEra[eIndex];
}
void CvCity::ChangeYieldPerEra(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiYieldPerEra[eIndex] = m_aiYieldPerEra[eIndex] + iChange;
	}
}
int CvCity::GetYieldModifierPerEra(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiYieldModifierPerEra[eIndex];
}
void CvCity::ChangeYieldModifierPerEra(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiYieldModifierPerEra[eIndex] = m_aiYieldModifierPerEra[eIndex] + iChange;
	}
}

//	--------------------------------------------------------------------------------
int CvCity::GetCityStateTradeRouteYieldModifier(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	if(m_aiCityStateTradeRouteYieldModifier[eIndex] != 0)
	{
		return m_aiCityStateTradeRouteYieldModifier[eIndex] * GET_PLAYER(m_eOwner).GetTrade()->GetNumberOfCityStateTradeRoutes();
	}
	return 0;
}
void CvCity::ChangeCityStateTradeRouteYieldModifier(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiCityStateTradeRouteYieldModifier[eIndex] = m_aiCityStateTradeRouteYieldModifier[eIndex] + iChange;
	}
}

//	--------------------------------------------------------------------------------
int CvCity::getYieldRateModifier(YieldTypes eIndex)	const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiYieldRateModifier[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::changeYieldRateModifier(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiYieldRateModifier.setAt(eIndex, m_aiYieldRateModifier[eIndex] + iChange);
		CvAssert(getYieldRate(eIndex, false) >= 0);

		GET_PLAYER(getOwner()).invalidateYieldRankCache(eIndex);
	}
}


//	--------------------------------------------------------------------------------
int CvCity::getYieldRateMultiplier(YieldTypes eIndex)	const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiYieldRateMultiplier[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::changeYieldRateMultiplier(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiYieldRateMultiplier.setAt(eIndex, m_aiYieldRateMultiplier[eIndex] + iChange);
		GET_PLAYER(getOwner()).invalidateYieldRankCache(eIndex);
	}
}


//	--------------------------------------------------------------------------------
// Base yield rate from Adjacent Features
int CvCity::getAdjacentFeaturesYieldRateModifier(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	CvPlayerTraits* pTraits = GET_PLAYER(m_eOwner).GetPlayerTraits();
	if(!pTraits->IsHasCityAdjacentFeatureModifier()) return 0;

	int iRtnValue = 0;
	CvPlot* pAdjacentPlot = NULL;
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		pAdjacentPlot = plotDirection(getX(), getY(), ((DirectionTypes)iI));
		if (pAdjacentPlot)
		{
			FeatureTypes eFeature = pAdjacentPlot->getFeatureType();
			if (eFeature == NO_FEATURE) continue;
			iRtnValue += pTraits->GetCityYieldModifierFromAdjacentFeature(eFeature, eIndex);
		}
	}
	return iRtnValue;
}
//	--------------------------------------------------------------------------------
int CvCity::getFeatureYieldRateModifier(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiFeatureYieldRateModifier[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::changeFeatureYieldRateModifier(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiFeatureYieldRateModifier.setAt(eIndex, m_aiFeatureYieldRateModifier[eIndex] + iChange);
		GET_PLAYER(getOwner()).invalidateYieldRankCache(eIndex);
	}
}




//	--------------------------------------------------------------------------------
int CvCity::getTerrainYieldRateModifier(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiTerrainYieldRateModifier[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::changeTerrainYieldRateModifier(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiTerrainYieldRateModifier.setAt(eIndex, m_aiTerrainYieldRateModifier[eIndex] + iChange);
		GET_PLAYER(getOwner()).invalidateYieldRankCache(eIndex);
	}
}


//	--------------------------------------------------------------------------------
int CvCity::getImprovementYieldRateModifier(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiImprovementYieldRateModifier[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::changeImprovementYieldRateModifier(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiImprovementYieldRateModifier.setAt(eIndex, m_aiImprovementYieldRateModifier[eIndex] + iChange);
		CvAssert(getYieldRate(eIndex, false) >= 0);

		GET_PLAYER(getOwner()).invalidateYieldRankCache(eIndex);
	}
}


//	--------------------------------------------------------------------------------
int CvCity::getSpecialistYieldRateModifier(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiSpecialistYieldRateModifier[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::changeSpecialistYieldRateModifier(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiSpecialistYieldRateModifier.setAt(eIndex, m_aiSpecialistYieldRateModifier[eIndex] + iChange);
		CvAssert(getYieldRate(eIndex, false) >= 0);

		GET_PLAYER(getOwner()).invalidateYieldRankCache(eIndex);
	}
}

//	--------------------------------------------------------------------------------
int CvCity::getResourceYieldRateModifier(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiResourceYieldRateModifier[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::changeResourceYieldRateModifier(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiResourceYieldRateModifier.setAt(eIndex, m_aiResourceYieldRateModifier[eIndex] + iChange);
		CvAssert(getYieldRate(eIndex, false) >= 0);

		GET_PLAYER(getOwner()).invalidateYieldRankCache(eIndex);
	}
}


//	--------------------------------------------------------------------------------
int CvCity::getExtraSpecialistYield(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiExtraSpecialistYield[eIndex];
}


//	--------------------------------------------------------------------------------
int CvCity::getExtraSpecialistYield(YieldTypes eIndex, SpecialistTypes eSpecialist) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	CvAssertMsg(eSpecialist >= 0, "eSpecialist expected to be >= 0");
	CvAssertMsg(eSpecialist < GC.getNumSpecialistInfos(), "GC.getNumSpecialistInfos expected to be >= 0");

	return GetCityCitizens()->GetSpecialistCount(eSpecialist) * getExtraYieldPerSpecialist(eIndex, eSpecialist);
}


//	--------------------------------------------------------------------------------
int CvCity::getExtraYieldPerSpecialist(YieldTypes eIndex, SpecialistTypes eSpecialist) const
{
	int iYieldMultiplier = GET_PLAYER(getOwner()).getSpecialistExtraYield(eSpecialist, eIndex) +
	                       GET_PLAYER(getOwner()).GetPlayerTraits()->GetSpecialistYieldChange(eSpecialist, eIndex);
	
	if (eSpecialist != GC.getDEFAULT_SPECIALIST())
	{
		iYieldMultiplier += GET_PLAYER(getOwner()).getSpecialistExtraYield(eIndex);
	}
	
#if defined(MOD_ROG_CORE)
	iYieldMultiplier += getSpecialistExtraYield(eSpecialist, eIndex);
#endif

#if defined(MOD_API_UNIFIED_YIELDS)
	iYieldMultiplier += GET_PLAYER(getOwner()).getSpecialistYieldChange(eSpecialist, eIndex);

	ReligionTypes eMajority = GetCityReligions()->GetReligiousMajority();
	if(eMajority >= RELIGION_PANTHEON)
	{
		const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eMajority, getOwner());
		if(pReligion)
		{
			iYieldMultiplier += pReligion->m_Beliefs.GetSpecialistYieldChange(eSpecialist, eIndex);
			BeliefTypes eSecondaryPantheon = GetCityReligions()->GetSecondaryReligionPantheonBelief();
			if (eSecondaryPantheon != NO_BELIEF)
			{
				iYieldMultiplier += GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon)->GetSpecialistYieldChange(eSpecialist, eIndex);
			}
		}
	}
#endif
	return iYieldMultiplier;
}


//	--------------------------------------------------------------------------------
int CvCity::getSpecialistYield(YieldTypes eIndex, SpecialistTypes eSpecialist) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	CvSpecialistInfo* pkSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
	if(pkSpecialistInfo == NULL)
	{
		//This function REQUIRES a valid specialist info.
		CvAssert(pkSpecialistInfo);
		return 0;
	}
	int iRtnValue = pkSpecialistInfo->getYieldChange(eIndex);
	iRtnValue += getExtraYieldPerSpecialist(eIndex, eSpecialist);
	return iRtnValue;
}
//	--------------------------------------------------------------------------------
void CvCity::updateExtraSpecialistYield(YieldTypes eYield)
{
	VALIDATE_OBJECT
	int iOldYield;
	int iNewYield;
	int iI;

	CvAssertMsg(eYield >= 0, "eYield expected to be >= 0");
	CvAssertMsg(eYield < NUM_YIELD_TYPES, "eYield expected to be < NUM_YIELD_TYPES");

	iOldYield = getExtraSpecialistYield(eYield);

	iNewYield = 0;

	for(iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
	{
		iNewYield += getExtraSpecialistYield(eYield, ((SpecialistTypes)iI));
	}

	if(iOldYield != iNewYield)
	{
		m_aiExtraSpecialistYield.setAt(eYield, iNewYield);
		ChangeBaseYieldRateFromSpecialists(eYield, (iNewYield - iOldYield));
	}
}


//	--------------------------------------------------------------------------------
void CvCity::updateExtraSpecialistYield()
{
	VALIDATE_OBJECT
	int iI;

	for(iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		updateExtraSpecialistYield((YieldTypes)iI);
	}
}


//	--------------------------------------------------------------------------------
int CvCity::getProductionToYieldModifier(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiProductionToYieldModifier[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::changeProductionToYieldModifier(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if(iChange != 0)
	{
		m_aiProductionToYieldModifier.setAt(eIndex, m_aiProductionToYieldModifier[eIndex] + iChange);
	}
}

//	--------------------------------------------------------------------------------
int CvCity::GetTradeYieldModifier(YieldTypes eIndex, CvString* toolTipSink) const
{
	int iReturnValue = GET_PLAYER(m_eOwner).GetTrade()->GetTradeValuesAtCityTimes100(this, eIndex);
	if (toolTipSink)
	{
		if (iReturnValue != 0)
		{
			switch (eIndex)
			{
			case YIELD_FOOD:
				*toolTipSink += "[NEWLINE][BULLET]";
				*toolTipSink += GetLocalizedText("TXT_KEY_FOOD_FROM_TRADE_ROUTES", iReturnValue / 100.0f);
				break;
			case YIELD_PRODUCTION:
				*toolTipSink += "[NEWLINE][BULLET]";
				*toolTipSink += GetLocalizedText("TXT_KEY_PRODUCTION_FROM_TRADE_ROUTES", iReturnValue / 100.0f);
				break;
			case YIELD_GOLD:
				*toolTipSink += "[NEWLINE][BULLET]";
				*toolTipSink += GetLocalizedText("TXT_KEY_GOLD_FROM_TRADE_ROUTES", iReturnValue / 100.0f);
				break;
			case YIELD_SCIENCE:
				*toolTipSink += "[NEWLINE][BULLET]";
				*toolTipSink += GetLocalizedText("TXT_KEY_SCIENCE_FROM_TRADE_ROUTES", iReturnValue / 100.0f);
				break;
			case YIELD_CULTURE:
				*toolTipSink += "[NEWLINE][BULLET]";
				*toolTipSink += GetLocalizedText("TXT_KEY_CULTURE_FROM_TRADE_ROUTES", iReturnValue / 100.0f);
				break;
			case YIELD_FAITH:
				*toolTipSink += "[NEWLINE][BULLET]";
				*toolTipSink += GetLocalizedText("TXT_KEY_FAITH_FROM_TRADE_ROUTES", iReturnValue / 100.0f);
				break;
			case YIELD_TOURISM:
				*toolTipSink += "[NEWLINE][BULLET]";
				*toolTipSink += GetLocalizedText("TXT_KEY_TOURISM_FROM_TRADE_ROUTES", iReturnValue / 100.0f);
				break;
			case YIELD_GOLDEN_AGE_POINTS:
				*toolTipSink += "[NEWLINE][BULLET]";
				*toolTipSink += GetLocalizedText("TXT_KEY_GOLDEN_AGE_POINTS_FROM_TRADE_ROUTES", iReturnValue / 100.0f);
				break;
			}
		}
	}
	return iReturnValue;
}

//	--------------------------------------------------------------------------------
int CvCity::getDomainFreeExperience(DomainTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	return m_aiDomainFreeExperience[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::changeDomainFreeExperience(DomainTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	m_aiDomainFreeExperience.setAt(eIndex, m_aiDomainFreeExperience[eIndex] + iChange);
	CvAssert(getDomainFreeExperience(eIndex) >= 0);
}

//	--------------------------------------------------------------------------------
int CvCity::GetDomainFreeExperiencesPerPop(DomainTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	return m_aiDomainFreeExperiencesPerPop[eIndex];
}
//	--------------------------------------------------------------------------------
void CvCity::changeDomainFreeExperiencesPerPop(DomainTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	m_aiDomainFreeExperiencesPerPop.setAt(eIndex, m_aiDomainFreeExperiencesPerPop[eIndex] + iChange);
	CvAssert(GetDomainFreeExperiencesPerPop(eIndex) >= 0);
}
int CvCity::GetDomainFreeExperiencesPerPopGlobal(DomainTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	return m_aiDomainFreeExperiencesPerPopGlobal[eIndex];
}

void CvCity::changeDomainFreeExperiencesPerPopGlobal(DomainTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	m_aiDomainFreeExperiencesPerPopGlobal.setAt(eIndex, m_aiDomainFreeExperiencesPerPopGlobal[eIndex] + iChange);
	CvAssert(GetDomainFreeExperiencesPerPopGlobal(eIndex) >= 0);
}

//	--------------------------------------------------------------------------------
int CvCity::GetDomainFreeExperiencesPerTurn(DomainTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	return m_aiDomainFreeExperiencesPerTurn[eIndex];
}
//	--------------------------------------------------------------------------------
void CvCity::changeDomainFreeExperiencesPerTurn(DomainTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	m_aiDomainFreeExperiencesPerTurn.setAt(eIndex, m_aiDomainFreeExperiencesPerTurn[eIndex] + iChange);
	CvAssert(GetDomainFreeExperiencesPerTurn(eIndex) >= 0);
}
//	--------------------------------------------------------------------------------
int CvCity::GetDomainEnemyCombatModifier(DomainTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	return m_aiDomainEnemyCombatModifier[eIndex];
}
//	--------------------------------------------------------------------------------
void CvCity::changeDomainEnemyCombatModifier(DomainTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	m_aiDomainEnemyCombatModifier.setAt(eIndex, m_aiDomainEnemyCombatModifier[eIndex] + iChange);
	CvAssert(GetDomainEnemyCombatModifier(eIndex) >= 0);
}
//	--------------------------------------------------------------------------------
int CvCity::GetDomainFriendsCombatModifierLocal(DomainTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	return m_aiDomainFriendsCombatModifierLocal[eIndex];
}
//	--------------------------------------------------------------------------------
void CvCity::changeDomainFriendsCombatModifierLocal(DomainTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	m_aiDomainFriendsCombatModifierLocal.setAt(eIndex, m_aiDomainFriendsCombatModifierLocal[eIndex] + iChange);
	CvAssert(GetDomainFriendsCombatModifierLocal(eIndex) >= 0);
}

//	--------------------------------------------------------------------------------
int CvCity::getDomainFreeExperienceFromGreatWorks(DomainTypes eIndex) const
{
	VALIDATE_OBJECT
		CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");

	int iXP = 0;

	CvBuildingXMLEntries* pkBuildings = GetCityBuildings()->GetBuildings();
	for(int iBuilding = 0; iBuilding < GetCityBuildings()->GetBuildings()->GetNumBuildings(); iBuilding++)
	{
		CvBuildingEntry* pInfo = pkBuildings->GetEntry(iBuilding);
		if(pInfo)
		{
			if (pInfo->GetDomainFreeExperiencePerGreatWork(eIndex) != 0)
			{
				int iGreatWorks = GetCityBuildings()->GetNumGreatWorksInBuilding((BuildingClassTypes)pInfo->GetBuildingClassType());
				iXP += (iGreatWorks * pInfo->GetDomainFreeExperiencePerGreatWork(eIndex));
			}
		}
	}

	return iXP;
}

#if defined(MOD_ROG_CORE)
//	--------------------------------------------------------------------------------
int CvCity::getDomainFreeExperienceFromGreatWorksGlobal(DomainTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");

	int iMod = GET_PLAYER(getOwner()).GetDomainFreeExperiencePerGreatWorkGlobal(eIndex);
	if (iMod <= 0)
	{
		return iMod;
	}

	int iXP = 0;
	int iLoop = 0;
	int iGreatWorks = 0;
	iGreatWorks = GET_PLAYER(getOwner()).GetCulture()->GetNumGreatWorks();

	iXP += (iGreatWorks * iMod);

	if (iXP > 150)
	{
		iXP = 150;
	}

	return iXP;
}
#endif
//	--------------------------------------------------------------------------------
int CvCity::getDomainProductionModifier(DomainTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	return m_aiDomainProductionModifier[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::changeDomainProductionModifier(DomainTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	m_aiDomainProductionModifier.setAt(eIndex, m_aiDomainProductionModifier[eIndex] + iChange);
}


//	--------------------------------------------------------------------------------
bool CvCity::isEverOwned(PlayerTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < MAX_PLAYERS, "eIndex expected to be < MAX_PLAYERS");
	return m_abEverOwned[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::setEverOwned(PlayerTypes eIndex, bool bNewValue)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < MAX_PLAYERS, "eIndex expected to be < MAX_PLAYERS");
	m_abEverOwned.setAt(eIndex, bNewValue);
}

//	--------------------------------------------------------------------------------
bool CvCity::isRevealed(TeamTypes eIndex, bool bDebug) const
{
	VALIDATE_OBJECT
	if(bDebug && GC.getGame().isDebugMode())
	{
		return true;
	}
	else
	{
		CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
		CvAssertMsg(eIndex < MAX_TEAMS, "eIndex expected to be < MAX_TEAMS");

		return m_abRevealed[eIndex];
	}
}


//	--------------------------------------------------------------------------------
bool CvCity::setRevealed(TeamTypes eIndex, bool bNewValue)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < MAX_TEAMS, "eIndex expected to be < MAX_TEAMS");

	if(isRevealed(eIndex, false) != bNewValue)
	{
		m_abRevealed.setAt(eIndex, bNewValue);

		return true;
	}
	return false;
}


//	--------------------------------------------------------------------------------
const char* CvCity::getNameKey() const
{
	VALIDATE_OBJECT
	return m_strName.c_str();
}


//	--------------------------------------------------------------------------------
const CvString CvCity::getName() const
{
	VALIDATE_OBJECT
	return GetLocalizedText(m_strName.c_str());
}


//	--------------------------------------------------------------------------------
void CvCity::setName(const char* szNewValue, bool bFound)
{
	VALIDATE_OBJECT
	CvString strName(szNewValue);
	gDLL->stripSpecialCharacters(strName);

	if(!strName.IsEmpty())
	{
		if(GET_PLAYER(getOwner()).isCityNameValid(strName, false))
		{
			m_strName = strName;

			if(isCitySelected())
			{
				DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
			}


			auto_ptr<ICvCity1> pCity = GC.WrapCityPointer(this);
			DLLUI->SetSpecificCityInfoDirty(pCity.get(), CITY_UPDATE_TYPE_BANNER);
		}
		if(bFound)
		{
			doFoundMessage();
		}
	}
}


//	--------------------------------------------------------------------------------
void CvCity::doFoundMessage()
{
	VALIDATE_OBJECT
	if(getOwner() == GC.getGame().getActivePlayer())
	{
		Localization::String localizedText = Localization::Lookup("TXT_KEY_MISC_CITY_HAS_BEEN_FOUNDED");
		localizedText << getNameKey();
		DLLUI->AddCityMessage(0, GetIDInfo(), getOwner(), false, -1, localizedText.toUTF8(), NULL /*ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_CITY_EDIT")->getPath()*/, MESSAGE_TYPE_MAJOR_EVENT, NULL, NO_COLOR, getX(), getY());
	}

	Localization::String localizedText = Localization::Lookup("TXT_KEY_MISC_CITY_IS_FOUNDED");
	localizedText << getNameKey();
	GC.getGame().addReplayMessage(REPLAY_MESSAGE_CITY_FOUNDED, getOwner(), localizedText.toUTF8(), getX(), getY());
}

//	--------------------------------------------------------------------------------
bool CvCity::IsExtraLuxuryResources()
{
	return (m_iCountExtraLuxuries > 0);
}

//	--------------------------------------------------------------------------------
void CvCity::SetExtraLuxuryResources(int iNewValue)
{
	m_iCountExtraLuxuries = iNewValue;
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeExtraLuxuryResources(int iChange)
{
	m_iCountExtraLuxuries += iChange;
}

//	--------------------------------------------------------------------------------
int CvCity::getProjectProduction(ProjectTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex expected to be < GC.getNumProjectInfos()");
	return m_paiProjectProduction[eIndex] / 100;
}


//	--------------------------------------------------------------------------------
void CvCity::setProjectProduction(ProjectTypes eIndex, int iNewValue)
{
	VALIDATE_OBJECT
	setProjectProductionTimes100(eIndex, iNewValue*100);
}


//	--------------------------------------------------------------------------------
void CvCity::changeProjectProduction(ProjectTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	changeProjectProductionTimes100(eIndex, iChange*100);
}

//	--------------------------------------------------------------------------------
int CvCity::getProjectProductionTimes100(ProjectTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex expected to be < GC.getNumProjectInfos()");
	return m_paiProjectProduction[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::setProjectProductionTimes100(ProjectTypes eIndex, int iNewValue)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex expected to be < GC.getNumProjectInfos()");

	if(getProjectProductionTimes100(eIndex) != iNewValue)
	{
		m_paiProjectProduction.setAt(eIndex, iNewValue);
		CvAssert(getProjectProductionTimes100(eIndex) >= 0);

		if((getOwner() == GC.getGame().getActivePlayer()) && isCitySelected())
		{
			DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
		}

		auto_ptr<ICvCity1> pCity = GC.WrapCityPointer(this);
		DLLUI->SetSpecificCityInfoDirty(pCity.get(), CITY_UPDATE_TYPE_BANNER);
	}
}


//	--------------------------------------------------------------------------------
void CvCity::changeProjectProductionTimes100(ProjectTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex expected to be < GC.getNumProjectInfos()");
	setProjectProductionTimes100(eIndex, (getProjectProductionTimes100(eIndex) + iChange));
}


//	--------------------------------------------------------------------------------
int CvCity::getSpecialistProduction(SpecialistTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumSpecialistInfos(), "eIndex expected to be < GC.getNumSpecialistInfos()");
	return m_paiSpecialistProduction[eIndex] / 100;
}


//	--------------------------------------------------------------------------------
void CvCity::setSpecialistProduction(SpecialistTypes eIndex, int iNewValue)
{
	VALIDATE_OBJECT
	setSpecialistProductionTimes100(eIndex, iNewValue*100);
}


//	--------------------------------------------------------------------------------
void CvCity::changeSpecialistProduction(SpecialistTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	changeSpecialistProductionTimes100(eIndex,iChange*100);
}

//	--------------------------------------------------------------------------------
int CvCity::getSpecialistProductionTimes100(SpecialistTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumSpecialistInfos(), "eIndex expected to be < GC.getNumSpecialistInfos()");
	return m_paiSpecialistProduction[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::setSpecialistProductionTimes100(SpecialistTypes eIndex, int iNewValue)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumSpecialistInfos(), "eIndex expected to be < GC.getNumSpecialistInfos()");

	if(getSpecialistProductionTimes100(eIndex) != iNewValue)
	{
		m_paiSpecialistProduction.setAt(eIndex, iNewValue);
		CvAssert(getSpecialistProductionTimes100(eIndex) >= 0);

		if((getOwner() == GC.getGame().getActivePlayer()) && isCitySelected())
		{
			DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
		}

		auto_ptr<ICvCity1> pCity = GC.WrapCityPointer(this);
		DLLUI->SetSpecificCityInfoDirty(pCity.get(), CITY_UPDATE_TYPE_BANNER);
	}
}


//	--------------------------------------------------------------------------------
void CvCity::changeSpecialistProductionTimes100(SpecialistTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumSpecialistInfos(), "eIndex expected to be < GC.getNumSpecialistInfos()");
	setSpecialistProductionTimes100(eIndex, (getSpecialistProductionTimes100(eIndex) + iChange));
}

#if defined(MOD_PROCESS_STOCKPILE)
//	--------------------------------------------------------------------------------
int CvCity::getProcessProduction(ProcessTypes eIndex) const
{
	return getProcessProductionTimes100(eIndex) / 100;
}

//	--------------------------------------------------------------------------------
int CvCity::getProcessProductionTimes100(ProcessTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumProcessInfos(), "eIndex expected to be < GC.getNumProcessInfos()");
	
	if (eIndex == GC.getInfoTypeForString("PROCESS_STOCKPILE")) {
		return getBasicYieldRateTimes100(YIELD_PRODUCTION, false, false);
	}

	return 0;
}
#endif

//	--------------------------------------------------------------------------------
CvCityBuildings* CvCity::GetCityBuildings() const
{
	VALIDATE_OBJECT
	return m_pCityBuildings;
}

//	--------------------------------------------------------------------------------
int CvCity::getUnitProduction(UnitTypes eIndex)	const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumUnitInfos(), "eIndex expected to be < GC.getNumUnitInfos()");
	return m_paiUnitProduction[eIndex] / 100;
}


//	--------------------------------------------------------------------------------
void CvCity::setUnitProduction(UnitTypes eIndex, int iNewValue)
{
	VALIDATE_OBJECT
	setUnitProductionTimes100(eIndex, iNewValue * 100);
}


//	--------------------------------------------------------------------------------
void CvCity::changeUnitProduction(UnitTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	changeUnitProductionTimes100(eIndex, iChange * 100);
}


//	--------------------------------------------------------------------------------
int CvCity::getUnitProductionTimes100(UnitTypes eIndex)	const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumUnitInfos(), "eIndex expected to be < GC.getNumUnitInfos()");
	return m_paiUnitProduction[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::setUnitProductionTimes100(UnitTypes eIndex, int iNewValue)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumUnitInfos(), "eIndex expected to be < GC.getNumUnitInfos()");

	if(getUnitProductionTimes100(eIndex) != iNewValue)
	{
		m_paiUnitProduction.setAt(eIndex, iNewValue);
		CvAssert(getUnitProductionTimes100(eIndex) >= 0);

		if((getOwner() == GC.getGame().getActivePlayer()) && isCitySelected())
		{
			DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
		}

		auto_ptr<ICvCity1> pCity = GC.WrapCityPointer(this);
		DLLUI->SetSpecificCityInfoDirty(pCity.get(), CITY_UPDATE_TYPE_BANNER);
	}
}


//	--------------------------------------------------------------------------------
void CvCity::changeUnitProductionTimes100(UnitTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	setUnitProductionTimes100(eIndex, (getUnitProductionTimes100(eIndex) + iChange));
}


//	--------------------------------------------------------------------------------
int CvCity::getUnitProductionTime(UnitTypes eIndex)	const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumUnitInfos(), "eIndex expected to be < GC.getNumUnitInfos()");
	return m_paiUnitProductionTime[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::setUnitProductionTime(UnitTypes eIndex, int iNewValue)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumUnitInfos(), "eIndex expected to be < GC.getNumUnitInfos()");
	m_paiUnitProductionTime.setAt(eIndex, iNewValue);
	CvAssert(getUnitProductionTime(eIndex) >= 0);
}


//	--------------------------------------------------------------------------------
void CvCity::changeUnitProductionTime(UnitTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	setUnitProductionTime(eIndex, (getUnitProductionTime(eIndex) + iChange));
}


//	--------------------------------------------------------------------------------
int CvCity::getUnitCombatFreeExperience(UnitCombatTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumUnitCombatClassInfos(), "eIndex expected to be < GC.getNumUnitCombatInfos()");
	return m_paiUnitCombatFreeExperience[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::changeUnitCombatFreeExperience(UnitCombatTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumUnitCombatClassInfos(), "eIndex expected to be < GC.getNumUnitCombatInfos()");
	m_paiUnitCombatFreeExperience.setAt(eIndex, m_paiUnitCombatFreeExperience[eIndex] + iChange);
	CvAssert(getUnitCombatFreeExperience(eIndex) >= 0);
}


//	--------------------------------------------------------------------------------
int CvCity::getUnitCombatProductionModifier(UnitCombatTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumUnitCombatClassInfos(), "eIndex expected to be < GC.getNumUnitCombatInfos()");
	return m_paiUnitCombatProductionModifier[eIndex];
}


//	--------------------------------------------------------------------------------
void CvCity::changeUnitCombatProductionModifier(UnitCombatTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumUnitCombatClassInfos(), "eIndex expected to be < GC.getNumUnitCombatInfos()");
	m_paiUnitCombatProductionModifier.setAt(eIndex, m_paiUnitCombatProductionModifier[eIndex] + iChange);
	CvAssert(getUnitCombatProductionModifier(eIndex) >= 0);
}


//	--------------------------------------------------------------------------------
int CvCity::getFreePromotionCount(PromotionTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumPromotionInfos(), "eIndex expected to be < GC.getNumPromotionInfos()");
	return m_paiFreePromotionCount[eIndex];
}


//	--------------------------------------------------------------------------------
bool CvCity::isFreePromotion(PromotionTypes eIndex) const
{
	VALIDATE_OBJECT
	return (getFreePromotionCount(eIndex) > 0);
}


//	--------------------------------------------------------------------------------
void CvCity::changeFreePromotionCount(PromotionTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumPromotionInfos(), "eIndex expected to be < GC.getNumPromotionInfos()");
	m_paiFreePromotionCount.setAt(eIndex, m_paiFreePromotionCount[eIndex] + iChange);
	CvAssert(getFreePromotionCount(eIndex) >= 0);
}

//	--------------------------------------------------------------------------------
int CvCity::getFreeFollowingPromotionCount(PromotionTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < GC.getNumPromotionInfos(), "eIndex expected to be < GC.getNumPromotionInfos()");
	if (!MOD_BELIEF_NEW_EFFECT_FOR_SP) return 0;

	ReligionTypes eMajority = GetCityReligions()->GetReligiousMajority();
	if (eMajority == NO_RELIGION) return 0;

	const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eMajority, getOwner());
	if (!pReligion) return 0;

	const std::tr1::unordered_set<int>& eReligionPromotions = pReligion->m_Beliefs.GetFollowingCityFreePromotion();
	if (eReligionPromotions.count(eIndex) > 0) return 1;
    return 0;
}

//	--------------------------------------------------------------------------------
int CvCity::getTradeRouteDomainRangeModifier(DomainTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	return m_viTradeRouteDomainRangeModifier[eIndex];
}
void CvCity::changeTradeRouteDomainRangeModifier(DomainTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	m_viTradeRouteDomainRangeModifier.setAt(eIndex, m_viTradeRouteDomainRangeModifier[eIndex] + iChange);
}


//	--------------------------------------------------------------------------------
int CvCity::getTradeRouteDomainGoldBonus(DomainTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	return m_viTradeRouteDomainGoldBonus[eIndex];
}
void CvCity::changeTradeRouteDomainGoldBonus(DomainTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	m_viTradeRouteDomainGoldBonus.setAt(eIndex, m_viTradeRouteDomainGoldBonus[eIndex] + iChange);
}
//	--------------------------------------------------------------------------------
int CvCity::getSpecialistFreeExperience() const
{
	VALIDATE_OBJECT
	return m_iSpecialistFreeExperience;
}


//	--------------------------------------------------------------------------------
void CvCity::changeSpecialistFreeExperience(int iChange)
{
	VALIDATE_OBJECT
	m_iSpecialistFreeExperience += iChange;
	CvAssert(m_iSpecialistFreeExperience >= 0);
}


//	--------------------------------------------------------------------------------
void CvCity::updateStrengthValue()
{
	VALIDATE_OBJECT
	AI_PERF_FORMAT("City-AI-perf.csv", ("CvCity::updateStrengthValue, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );
	// Default Strength
	int iStrengthValue = /*600*/ GC.getCITY_STRENGTH_DEFAULT();

	// Population mod
	iStrengthValue += getPopulation() * /*25*/ GC.getCITY_STRENGTH_POPULATION_CHANGE();

	// Building Defense
	int iBuildingDefense = m_pCityBuildings->GetBuildingDefense();
	int iBuildingDefenseMod = 100 + m_pCityBuildings->GetBuildingDefenseMod() + GET_PLAYER(m_eOwner).getCityDefenseModifierGlobal();
	iBuildingDefense *= iBuildingDefenseMod;
	iBuildingDefense /= 100;

	iStrengthValue += iBuildingDefense;


#if defined(MOD_ROG_CORE)
	if (getProductionProcess() != NO_PROCESS)
	{
		CvProcessInfo* pkProcessInfo = GC.getProcessInfo(getProductionProcess());
		if (pkProcessInfo && pkProcessInfo->getDefenseValue() != 0)
		{

			iStrengthValue += (getYieldRate(YIELD_PRODUCTION, false) * pkProcessInfo->getDefenseValue());
		}
	}
#endif

	// Garrisoned Unit
	CvUnit* pGarrisonedUnit = GetGarrisonedUnit();
	int iStrengthFromUnits = 0;
	if(pGarrisonedUnit)
	{
#if defined(MOD_UNITS_MAX_HP)
		int iMaxHits = pGarrisonedUnit->GetMaxHitPoints();
#else
		int iMaxHits = GC.getMAX_HIT_POINTS();
#endif
		iStrengthFromUnits = pGarrisonedUnit->GetBaseCombatStrength() * 100 * (iMaxHits - pGarrisonedUnit->getDamage()) / iMaxHits;
	}

	iStrengthValue += ((iStrengthFromUnits * 100) / /*300*/ GC.getCITY_STRENGTH_UNIT_DIVISOR());

	// Tech Progress increases City Strength
	int iTechProgress = GET_TEAM(getTeam()).GetTeamTechs()->GetNumTechsKnown() * 100 / GC.getNumTechInfos();

	// Want progress to be a value between 0 and 5
	double fTechProgress = iTechProgress / 100.0 * /*5*/ GC.getCITY_STRENGTH_TECH_BASE();
	double fTechExponent = /*2.0f*/ GC.getCITY_STRENGTH_TECH_EXPONENT();
	int iTechMultiplier = /*2*/ GC.getCITY_STRENGTH_TECH_MULTIPLIER();

	// The way all of this adds up...
	// 25% of the way through the game provides an extra 3.12
	// 50% of the way through the game provides an extra 12.50
	// 75% of the way through the game provides an extra 28.12
	// 100% of the way through the game provides an extra 50.00

	double fTechMod = pow(fTechProgress, fTechExponent);
	fTechMod *= iTechMultiplier;

	fTechMod *= 100;	// Bring it back into hundreds
	iStrengthValue += (int)(fTechMod + 0.005);	// Adding a small amount to prevent small fp accuracy differences from generating a different integer result on the Mac and PC. Assuming fTechMod is positive, round to nearest hundredth

	int iStrengthMod = 0;

	// Player-wide strength mod (Policies, etc.)
	iStrengthMod += GET_PLAYER(getOwner()).GetCityStrengthMod();

#if defined(MOD_ROG_CORE)
	iStrengthMod += GET_PLAYER(getOwner()).GetGlobalCityStrengthMod();
#endif

#if defined(MOD_API_UNIFIED_YIELDS_MORE)
	// Crime
	iStrengthMod += GetYieldModifierFromCrime(YIELD_CRIME);
#endif

	// Apply Mod
	iStrengthValue *= (100 + iStrengthMod);
	iStrengthValue /= 100;

	m_iStrengthValue = iStrengthValue;

	// Terrain mod
	if(plot()->isHills())
	{
		m_iStrengthValue += /*3*/ GC.getCITY_STRENGTH_HILL_CHANGE();
	}
	if (plot()->isMountain())
	{
		m_iStrengthValue += GC.getCITY_STRENGTH_MOUNTAIN_CHANGE();
	}

	// Policies
	m_iStrengthValue += GET_PLAYER(getOwner()).GetPlayerPolicies()->GetNumericModifier(POLICYMOD_CITY_DEFENSE_BOOST);

	DLLUI->setDirty(CityInfo_DIRTY_BIT, true);
}

//	--------------------------------------------------------------------------------
int CvCity::getStrengthValue(bool bForRangeStrike) const
{
	VALIDATE_OBJECT
	// Strike strikes are weaker
	if(bForRangeStrike)
	{
		int iValue = m_iStrengthValue;

		iValue -= m_pCityBuildings->GetBuildingDefense();


#if defined(MOD_ROG_CORE)
		if (getProductionProcess() != NO_PROCESS)
		{
			CvProcessInfo* pkProcessInfo = GC.getProcessInfo(getProductionProcess());
			if (pkProcessInfo && pkProcessInfo->getDefenseValue() != 0)
			{
				iValue -= ((getYieldRate(YIELD_PRODUCTION, false) * pkProcessInfo->getDefenseValue()) / 100);
			}
		}
#endif

		CvAssertMsg(iValue > 0, "City strength should always be greater than zero. Please show Jon this and send your last 5 autosaves.");

		iValue *= /*40*/ GC.getCITY_RANGED_ATTACK_STRENGTH_MULTIPLIER();
		iValue /= 100;

		if(GetGarrisonedUnit())
		{
			iValue *= (100 + GET_PLAYER(m_eOwner).GetGarrisonedCityRangeStrikeModifier());
			iValue /= 100;
		}

		// Religion city strike mod
		int iReligionCityStrikeMod = 0;
		ReligionTypes eMajority = GetCityReligions()->GetReligiousMajority();
		if(eMajority != NO_RELIGION)
		{
			const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eMajority, getOwner());
			if(pReligion)
			{
				iReligionCityStrikeMod = pReligion->m_Beliefs.GetCityRangeStrikeModifier();
				BeliefTypes eSecondaryPantheon = GetCityReligions()->GetSecondaryReligionPantheonBelief();
				if (eSecondaryPantheon != NO_BELIEF)
				{
					iReligionCityStrikeMod += GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon)->GetCityRangeStrikeModifier();
				}
				if(iReligionCityStrikeMod > 0)
				{
					iValue *= (100 + iReligionCityStrikeMod);
					iValue /= 100;
				}
			}
		}

#if defined(MOD_ROG_CORE)
		if (getCityBuildingRangeStrikeModifier() != 0)
		{
			iValue *= (100 + getCityBuildingRangeStrikeModifier());
			iValue /= 100;
		}
#endif

#if defined(MOD_ROG_CORE)
		iValue *= (100 + GET_PLAYER(getOwner()).GetGlobalRangedStrikeModifier());
		iValue /= 100;
#endif

		return iValue;
	}

	return m_iStrengthValue;
}

//	--------------------------------------------------------------------------------
int CvCity::GetPower() const
{
	VALIDATE_OBJECT
	return int(pow((double) getStrengthValue() / 100, 1.5));		// This is the same math used to calculate Unit Power in CvUnitEntry
}


//	--------------------------------------------------------------------------------
int CvCity::getDamage() const
{
	VALIDATE_OBJECT
	return m_iDamage;
}

//	--------------------------------------------------------------------------------
void CvCity::setDamage(int iValue, bool noMessage)
{
	float fDelay = 0.0f;

	VALIDATE_OBJECT

	if(iValue < 0)
		iValue = 0;
	else if(iValue > GetMaxHitPoints())
		iValue = GetMaxHitPoints();

	if(iValue != getDamage())
	{
		int iOldValue = getDamage();
		auto_ptr<ICvCity1> pDllCity(new CvDllCity(this));
		gDLL->GameplayCitySetDamage(pDllCity.get(), iValue, iOldValue);

		// send the popup text if the player can see this plot
		if(!noMessage && plot()->GetActiveFogOfWarMode() == FOGOFWARMODE_OFF)
		{
			char text[256];
			text[0] = NULL;
			int iNewValue = MIN(GetMaxHitPoints(),iValue);
			int iDiff = iOldValue - iNewValue;
			if(iNewValue < iOldValue)
			{
				sprintf_s(text, "[COLOR_GREEN]+%d[ENDCOLOR]", iDiff);
				fDelay = GC.getPOST_COMBAT_TEXT_DELAY() * 2;
			}
			else
			{
				sprintf_s(text, "[COLOR_RED]%d[ENDCOLOR]", iDiff);
			}

#if defined(SHOW_PLOT_POPUP)
			SHOW_PLOT_POPUP(plot(), NO_PLAYER, text, fDelay);
#else
			DLLUI->AddPopupText(m_iX, m_iY, text, fDelay);
#endif
		}
		m_iDamage = iValue;
	}
}

//	--------------------------------------------------------------------------------
void CvCity::changeDamage(int iChange)
{
	VALIDATE_OBJECT
	if(0 != iChange)
	{
		setDamage(getDamage() + iChange);
	}
}

//	--------------------------------------------------------------------------------
/// Can a specific plot be bought for the city
bool CvCity::CanBuyPlot(int iPlotX, int iPlotY, bool bIgnoreCost)
{
	VALIDATE_OBJECT
	CvPlot* pTargetPlot = NULL;

	if(GC.getBUY_PLOTS_DISABLED())
	{
		return false;
	}

	pTargetPlot = GC.getMap().plot(iPlotX, iPlotY);

	if(!pTargetPlot)
	{
		// no plot to buy
		return false;
	}

	// if this plot belongs to someone, bail!
	if (pTargetPlot->getOwner() != NO_PLAYER)
	{
		if (GET_PLAYER(getOwner()).GetPlayerTraits()->IsBuyOwnedTiles())
		{
			if (pTargetPlot->getOwner() == getOwner() || pTargetPlot->isCity())
				return false;

			// Bad idea for AI to steal?
			if (!GET_PLAYER(getOwner()).isHuman() && GET_PLAYER(getOwner()).isMajorCiv() && GET_PLAYER(getOwner()).GetDiplomacyAI()->IsPlayerBadTheftTarget(pTargetPlot->getOwner(), pTargetPlot))
				return false;
		}
		else
		{
			return false;
		}
	}

	//: can't buy plot with enemy combat units 
	if (MOD_ROG_CORE && pTargetPlot->IsActualEnemyUnit(getOwner(), true))
		return false;


	// Must be adjacent to a plot owned by this city
	CvPlot* pAdjacentPlot;
	bool bFoundAdjacent = false;
	for(int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		pAdjacentPlot = plotDirection(pTargetPlot->getX(), pTargetPlot->getY(), ((DirectionTypes)iI));

		if(pAdjacentPlot != NULL)
		{
			if(pAdjacentPlot->getOwner() == getOwner())
			{
				if(pAdjacentPlot->GetCityPurchaseID() == GetID())
				{
					bFoundAdjacent = true;
					break;
				}
			}
		}
	}

	if(!bFoundAdjacent)
		return false;

	// Max range of 3
#if defined(MOD_GLOBAL_CITY_WORKING)
	const int iMaxRange = getBuyPlotDistance();
#else
	const int iMaxRange = /*3*/ GC.getMAXIMUM_BUY_PLOT_DISTANCE();
#endif
	if(plotDistance(iPlotX, iPlotY, getX(), getY()) > iMaxRange)
		return false;

	// check money
	if(!bIgnoreCost)
	{
		if(GET_PLAYER(getOwner()).GetTreasury()->GetGold() < GetBuyPlotCost(pTargetPlot->getX(), pTargetPlot->getY()))
		{
			return false;
		}
	}

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(getOwner());
		args->Push(GetID());
		args->Push(iPlotX);
		args->Push(iPlotY);

		// Attempt to execute the game events.
		// Will return false if there are no registered listeners.
		bool bResult = false;
		if(LuaSupport::CallTestAll(pkScriptSystem, "CityCanBuyPlot", args.get(), bResult))
		{
			// Check the result.
			if(bResult == false)
			{
				return false;
			}
		}
	}

	return true;
}


//	--------------------------------------------------------------------------------
/// Can this city buy a plot, any plot?
bool CvCity::CanBuyAnyPlot(void)
{
	VALIDATE_OBJECT
	CvPlot* pLoopPlot = NULL;
	CvPlot* pThisPlot = plot();
#if defined(MOD_GLOBAL_CITY_WORKING)
	const int iMaxRange = getBuyPlotDistance();
#else
	const int iMaxRange = GC.getMAXIMUM_BUY_PLOT_DISTANCE();
#endif
	CvMap& thisMap = GC.getMap();

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(getOwner());
		args->Push(GetID());

		// Attempt to execute the game events.
		// Will return false if there are no registered listeners.
		bool bResult = false;
		if(LuaSupport::CallTestAll(pkScriptSystem, "CityCanBuyAnyPlot", args.get(), bResult))
		{
			// Check the result.
			if(bResult == false)
			{
				return false;
			}
		}
	}

	for(int iDX = -iMaxRange; iDX <= iMaxRange; iDX++)
	{
		for(int iDY = -iMaxRange; iDY <= iMaxRange; iDY++)
		{
			pLoopPlot = plotXYWithRangeCheck(getX(), getY(), iDX, iDY, iMaxRange);
			if(pLoopPlot != NULL)
			{
				if(pLoopPlot->getOwner() != NO_PLAYER)
				{
					continue;
				}

				// we can use the faster, but slightly inaccurate pathfinder here - after all we just need the existence of a path
				int iInfluenceCost = thisMap.calculateInfluenceDistance(pThisPlot, pLoopPlot, iMaxRange, false);

				if(iInfluenceCost > 0)
				{
					return true;
				}
			}
		}
	}

	return false;
}


//	--------------------------------------------------------------------------------
/// Which plot will we buy next
CvPlot* CvCity::GetNextBuyablePlot(void)
{
	VALIDATE_OBJECT
	std::vector<int> aiPlotList;
	aiPlotList.resize(20, -1);
	GetBuyablePlotList(aiPlotList);

	int iListLength = 0;
	for(uint ui = 0; ui < aiPlotList.size(); ui++)
	{
		if(aiPlotList[ui] >= 0)
		{
			iListLength++;
		}
		else
		{
			break;
		}
	}

	CvPlot* pPickedPlot = NULL;
	if(iListLength > 0)
	{
		int iPickedIndex = GC.getGame().getJonRandNum(iListLength, "GetNextBuyablePlot picker");
		pPickedPlot = GC.getMap().plotByIndex(aiPlotList[iPickedIndex]);
	}

	return pPickedPlot;
}

//	--------------------------------------------------------------------------------
void CvCity::GetBuyablePlotList(std::vector<int>& aiPlotList)
{
	VALIDATE_OBJECT
	aiPlotList.resize(20, -1);
	int iResultListIndex = 0;

	int iLowestCost = INT_MAX;
	CvPlot* pLoopPlot = NULL;
	CvPlot* pThisPlot = plot();
	const int iMaxRange = /*5*/ GC.getMAXIMUM_ACQUIRE_PLOT_DISTANCE();
	CvMap& thisMap = GC.getMap();
	TeamTypes thisTeam = getTeam();

	int iPLOT_INFLUENCE_DISTANCE_MULTIPLIER =	/*100*/ GC.getPLOT_INFLUENCE_DISTANCE_MULTIPLIER();
	int iPLOT_INFLUENCE_RING_COST =				/*100*/ GC.getPLOT_INFLUENCE_RING_COST();
	int iPLOT_INFLUENCE_WATER_COST =			/* 25*/ GC.getPLOT_INFLUENCE_WATER_COST();
	int iPLOT_INFLUENCE_IMPROVEMENT_COST =		/* -5*/ GC.getPLOT_INFLUENCE_IMPROVEMENT_COST();
	int iPLOT_INFLUENCE_ROUTE_COST =			/*0*/	GC.getPLOT_INFLUENCE_ROUTE_COST();
	int iPLOT_INFLUENCE_RESOURCE_COST =			/*-105*/ GC.getPLOT_INFLUENCE_RESOURCE_COST();
	int iPLOT_INFLUENCE_NW_COST =				/*-105*/ GC.getPLOT_INFLUENCE_NW_COST();
	int iPLOT_INFLUENCE_YIELD_POINT_COST =		/*-1*/	GC.getPLOT_INFLUENCE_YIELD_POINT_COST();

	int iPLOT_INFLUENCE_NO_ADJACENT_OWNED_COST = /*1000*/ GC.getPLOT_INFLUENCE_NO_ADJACENT_OWNED_COST();

	int iYieldLoop;

	int iDirectionLoop;
	bool bFoundAdjacentOwnedByCity;

	int iDX, iDY;

	ImprovementTypes eBarbCamptype = (ImprovementTypes)GC.getBARBARIAN_CAMP_IMPROVEMENT();

	for (iDX = -iMaxRange; iDX <= iMaxRange; iDX++)
	{
		for (iDY = -iMaxRange; iDY <= iMaxRange; iDY++)
		{
			pLoopPlot = plotXYWithRangeCheck(getX(), getY(), iDX, iDY, iMaxRange);
			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->getOwner() != NO_PLAYER)
				{
#if defined(MOD_ROG_CORE)
					if (MOD_ROG_CORE && GET_PLAYER(getOwner()).GetPlayerTraits()->IsBuyOwnedTiles())
					{
						if (pLoopPlot->getOwner() == getOwner() || pLoopPlot->isCity())
						{
							continue;
						}
					}
					else
					{
#endif
						continue;
#if defined(MOD_ROG_CORE)
					}
#endif
				}

#if defined(MOD_EVENTS_CITY_BORDERS)
				// This can be used to implement a 12-mile limit
				if (MOD_EVENTS_CITY_BORDERS) {
					if (GAMEEVENTINVOKE_TESTALL(GAMEEVENT_CityCanAcquirePlot, getOwner(), GetID(), pLoopPlot->getX(), pLoopPlot->getY()) == GAMEEVENTRETURN_FALSE) {
						continue;
					}
				} else {
#endif				
				ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
				if (pkScriptSystem) 
				{
					CvLuaArgsHandle args;
					args->Push(getOwner());
					args->Push(GetID());
					args->Push(pLoopPlot->getX());
					args->Push(pLoopPlot->getY());

					bool bResult = false;
					if (LuaSupport::CallTestAll(pkScriptSystem, "CityCanAcquirePlot", args.get(), bResult))
					{
						if (bResult == false) {
							continue;
						}
					}
				}
#if defined(MOD_EVENTS_CITY_BORDERS)
				}
#endif				

				// we can use the faster, but slightly inaccurate pathfinder here - after all we are using a rand in the equation
				int iInfluenceCost = thisMap.calculateInfluenceDistance(pThisPlot, pLoopPlot, iMaxRange, false) * iPLOT_INFLUENCE_DISTANCE_MULTIPLIER;

				if (iInfluenceCost > 0)
				{
					// Modifications for tie-breakers in a ring

					// Resource Plots claimed first
					ResourceTypes eResource = pLoopPlot->getResourceType(thisTeam);
					if (eResource != NO_RESOURCE)
					{
						iInfluenceCost += iPLOT_INFLUENCE_RESOURCE_COST;
						bool bBonusResource = GC.getResourceInfo(eResource)->getResourceUsage() == RESOURCEUSAGE_BONUS;
						if (bBonusResource)
						{
#if defined(MOD_GLOBAL_CITY_WORKING)
							if (plotDistance(pLoopPlot->getX(),pLoopPlot->getY(),getX(),getY()) > getWorkPlotDistance())
#else	
							if (plotDistance(pLoopPlot->getX(),pLoopPlot->getY(),getX(),getY()) > NUM_CITY_RINGS)
#endif
							{
								// undo the bonus - we can't work this tile from this city
								iInfluenceCost -= iPLOT_INFLUENCE_RESOURCE_COST;
							}
							else
							{
								// very slightly decrease value of bonus resources
								++iInfluenceCost;
							}
						}
					}
					else 
					{

						// Water Plots claimed later
						if (pLoopPlot->isWater())
						{
							iInfluenceCost += iPLOT_INFLUENCE_WATER_COST;
						}

						// if we can't work this tile in this city make it much less likely to be picked
#if defined(MOD_GLOBAL_CITY_WORKING)
						if (plotDistance(pLoopPlot->getX(),pLoopPlot->getY(),getX(),getY()) > getWorkPlotDistance())
#else	
						if (plotDistance(pLoopPlot->getX(),pLoopPlot->getY(),getX(),getY()) > NUM_CITY_RINGS)
#endif
						{
							iInfluenceCost += iPLOT_INFLUENCE_RING_COST;
						}

					}

					// improved tiles get a slight priority (unless they are barbarian camps!)
					ImprovementTypes thisImprovement = pLoopPlot->getImprovementType();
					if (thisImprovement != NO_IMPROVEMENT)
					{
						if (thisImprovement == eBarbCamptype)
						{
							iInfluenceCost += iPLOT_INFLUENCE_RING_COST;
						}
						else
						{
							iInfluenceCost += iPLOT_INFLUENCE_IMPROVEMENT_COST;
						}
					}

					// roaded tiles get a priority - [not any more: weight above is 0 by default]
					if (pLoopPlot->getRouteType() != NO_ROUTE)
					{
						iInfluenceCost += iPLOT_INFLUENCE_ROUTE_COST;
					}

					// while we're at it grab Natural Wonders quickly also
					if (pLoopPlot->IsNaturalWonder(true))
					{
						iInfluenceCost += iPLOT_INFLUENCE_NW_COST;
					}

					// More Yield == more desirable
					for (iYieldLoop = 0; iYieldLoop < NUM_YIELD_TYPES; iYieldLoop++)
					{
						iInfluenceCost += (iPLOT_INFLUENCE_YIELD_POINT_COST * pLoopPlot->getYield((YieldTypes) iYieldLoop));
					}

					// all other things being equal move towards unclaimed resources
					bool bUnownedNaturalWonderAdjacentCount = false;
					for (int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
					{
						CvPlot* pAdjacentPlot = plotDirection(pLoopPlot->getX(), pLoopPlot->getY(), ((DirectionTypes)iI));

						if (pAdjacentPlot != NULL)
						{
							if (pAdjacentPlot->getOwner() == NO_PLAYER)
							{
								int iPlotDistance = plotDistance(getX(), getY(), pAdjacentPlot->getX(), pAdjacentPlot->getY());
								ResourceTypes eAdjacentResource = pAdjacentPlot->getResourceType(thisTeam);
								if (eAdjacentResource != NO_RESOURCE)
								{
									// if we are close enough to work, or this is not a bonus resource
#if defined(MOD_GLOBAL_CITY_WORKING)
									if (iPlotDistance <= getWorkPlotDistance() || GC.getResourceInfo(eAdjacentResource)->getResourceUsage() != RESOURCEUSAGE_BONUS)
#else	
									if (iPlotDistance <= NUM_CITY_RINGS || GC.getResourceInfo(eAdjacentResource)->getResourceUsage() != RESOURCEUSAGE_BONUS)
#endif
									{
										--iInfluenceCost;
									}
								}
								if (pAdjacentPlot->IsNaturalWonder(true))
								{
#if defined(MOD_GLOBAL_CITY_WORKING)
									if (iPlotDistance <= getWorkPlotDistance()) // grab for this city
#else	
									if (iPlotDistance <= NUM_CITY_RINGS) // grab for this city
#endif
									{
										bUnownedNaturalWonderAdjacentCount = true;
									}
									--iInfluenceCost; // but we will slightly grow towards it for style in any case
								}
							}
						}
					}

					// move towards unclaimed NW
					iInfluenceCost += bUnownedNaturalWonderAdjacentCount ? -1 : 0;

					// Plots not adjacent to another Plot acquired by this City are pretty much impossible to get
					bFoundAdjacentOwnedByCity = false;
					for (iDirectionLoop = 0; iDirectionLoop < NUM_DIRECTION_TYPES; iDirectionLoop++)
					{
						CvPlot* pAdjacentPlot = plotDirection(pLoopPlot->getX(), pLoopPlot->getY(), (DirectionTypes) iDirectionLoop);

						if (pAdjacentPlot != NULL)
						{
							// Have to check plot ownership first because the City IDs match between different players!!!
							if (pAdjacentPlot->getOwner() == getOwner() && pAdjacentPlot->GetCityPurchaseID() == GetID())
							{
								bFoundAdjacentOwnedByCity = true;
								break;
							}
						}
					}
					if (!bFoundAdjacentOwnedByCity)
					{
						iInfluenceCost += iPLOT_INFLUENCE_NO_ADJACENT_OWNED_COST;
					}

#if defined(MOD_UI_CITY_EXPANSION)
					// Group very similiar "cost" tiles - ie 683 and 684 cost tiles will appear to be the same value
					int iDivisor = /*5*/ GC.getPLOT_INFLUENCE_COST_VISIBLE_DIVISOR();
					iInfluenceCost /= iDivisor;
					iInfluenceCost *= iDivisor;
#endif

					// Are we cheap enough to get picked next?
					if (iInfluenceCost < iLowestCost)
					{
						// clear reset list
						for(uint ui = 0; ui < aiPlotList.size(); ui++)
						{
							aiPlotList[ui] = -1;
						}
						iResultListIndex = 0;
						iLowestCost = iInfluenceCost;
						// this will "fall through" to the next conditional
					}

					if (iInfluenceCost == iLowestCost)
					{
						aiPlotList[iResultListIndex] = pLoopPlot->GetPlotIndex();
						iResultListIndex++;
					}
				}
			}
		}
	}

}

//	--------------------------------------------------------------------------------
/// How much will purchasing this plot cost -- (-1,-1) will return the generic price
int CvCity::GetBuyPlotCost(int iPlotX, int iPlotY) const
{
	VALIDATE_OBJECT
	if(iPlotX == -1 && iPlotY == -1)
	{
		return GET_PLAYER(getOwner()).GetBuyPlotCost();
	}

	CvPlot* pPlot = GC.getMap().plot(iPlotX, iPlotY);
	if(!pPlot)
	{
		return -1;
	}

	// Base cost
	int iCost = GET_PLAYER(getOwner()).GetBuyPlotCost();

	// Influence cost factor (e.g. Hills are more expensive than flat land)
	CvMap& thisMap = GC.getMap();
	CvPlot* pThisPlot = plot();
#if defined(MOD_GLOBAL_CITY_WORKING)
	const int iMaxRange = getBuyPlotDistance();
#else
	const int iMaxRange = /*3*/ GC.getMAXIMUM_BUY_PLOT_DISTANCE();
#endif
	if(plotDistance(iPlotX, iPlotY, getX(), getY()) > iMaxRange)
		return 9999; // Critical hit!

	int iPLOT_INFLUENCE_BASE_MULTIPLIER = /*100*/ GC.getPLOT_INFLUENCE_BASE_MULTIPLIER();
	int iPLOT_INFLUENCE_DISTANCE_MULTIPLIER = /*100*/ GC.getPLOT_INFLUENCE_DISTANCE_MULTIPLIER();
	int iPLOT_INFLUENCE_DISTANCE_DIVISOR = /*3*/ GC.getPLOT_INFLUENCE_DISTANCE_DIVISOR();
	int iPLOT_BUY_RESOURCE_COST = /*-100*/ GC.getPLOT_BUY_RESOURCE_COST();
	int iDistance = thisMap.calculateInfluenceDistance(pThisPlot, pPlot, iMaxRange, false);
	iDistance -= GetCheapestPlotInfluence(); // Reduce distance by the cheapest available (so that the costs don't ramp up ridiculously fast)

	int iInfluenceCostFactor = iPLOT_INFLUENCE_BASE_MULTIPLIER;
	iInfluenceCostFactor += (iDistance * iPLOT_INFLUENCE_DISTANCE_MULTIPLIER) / iPLOT_INFLUENCE_DISTANCE_DIVISOR;
	if(pPlot->getResourceType(getTeam()) != NO_RESOURCE)
		iInfluenceCostFactor += iPLOT_BUY_RESOURCE_COST;

	if(iInfluenceCostFactor > 100)
	{
		iCost *= iInfluenceCostFactor;
		iCost /= 100;
	}

#if defined(MOD_UI_CITY_EXPANSION)
	if (MOD_UI_CITY_EXPANSION && GET_PLAYER(getOwner()).isHuman()) {
		// If we have a culture surplus, we get a discount on the tile
		if (GetJONSCultureStored() >= GetJONSCultureThreshold()) {
			iCost -= GET_PLAYER(getOwner()).GetBuyPlotCost();
		}
	}
#endif

#if defined(MOD_ROG_CORE)
	//Owned by someone? Much more expensive!
	if (MOD_ROG_CORE && GET_PLAYER(getOwner()).GetPlayerTraits()->IsBuyOwnedTiles())
	{
		if ((pPlot->getOwner() != NO_PLAYER) && (pPlot->getOwner() != getOwner()))
		{
			iCost *= 3;
			iCost /= 2;
		}
	}
#endif

	// Game Speed Mod
	iCost *= GC.getGame().getGameSpeedInfo().getGoldPercent();
	iCost /= 100;

	iCost *= (100 + getPlotBuyCostModifier());
	iCost /= 100;

	// Now round so the number looks neat
	int iDivisor = /*5*/ GC.getPLOT_COST_APPEARANCE_DIVISOR();
	iCost /= iDivisor;
	iCost *= iDivisor;

	return iCost;
}

//	--------------------------------------------------------------------------------
/// Buy the plot and set it's owner to us (executed by the network code)
void CvCity::BuyPlot(int iPlotX, int iPlotY)
{
	VALIDATE_OBJECT
	CvPlot* pPlot = GC.getMap().plot(iPlotX, iPlotY);
	if(!pPlot)
	{
		return;
	}

	int iCost = GetBuyPlotCost(iPlotX, iPlotY);
	CvPlayer& thisPlayer = GET_PLAYER(getOwner());
	thisPlayer.GetTreasury()->LogExpenditure("", iCost, 1);
	thisPlayer.GetTreasury()->ChangeGold(-iCost);
#if defined(MOD_UI_CITY_EXPANSION)
	bool bWithGold = true;
	if (MOD_UI_CITY_EXPANSION && GET_PLAYER(getOwner()).isHuman()) {
		// If we have a culture surplus, we got a discount on the tile, so remove the surplus
		int iOverflow = GetJONSCultureStored() - GetJONSCultureThreshold();
		if (iOverflow >= 0) {
			SetJONSCultureStored(iOverflow);
			ChangeJONSCultureLevel(1);
			bWithGold = false;
		}
	}
#endif

#if defined(MOD_UI_CITY_EXPANSION)
	if (iCost > 0) {
		// Only do this if we actually paid for the plot (as opposed to getting it for free via city growth)
#endif
		thisPlayer.ChangeNumPlotsBought(1);

		// See if there's anyone else nearby that could get upset by this action
		CvCity* pNearbyCity;
#if defined(MOD_GLOBAL_CITY_WORKING)
		for(int iI = 0; iI < GetNumWorkablePlots(); iI++)
#else
		for(int iI = 0; iI < NUM_CITY_PLOTS; iI++)
#endif
		{
			pPlot = plotCity(iPlotX, iPlotY, iI);

			if(pPlot != NULL)
			{
				pNearbyCity = pPlot->getPlotCity();

				if(pNearbyCity)
				{
					if(pNearbyCity->getOwner() != getOwner())
					{
						pNearbyCity->AI_ChangeNumPlotsAcquiredByOtherPlayer(getOwner(), 1);
					}
				}
			}
		}
#if defined(MOD_UI_CITY_EXPANSION)
	}
#endif

	if(GC.getLogging() && GC.getAILogging())
	{
		CvPlayerAI& kOwner = GET_PLAYER(getOwner());
		CvString playerName;
		FILogFile* pLog;
		CvString strBaseString;
		CvString strOutBuf;
		playerName = kOwner.getCivilizationShortDescription();
		pLog = LOGFILEMGR.GetLog(kOwner.GetCitySpecializationAI()->GetLogFileName(playerName), FILogFile::kDontTimeStamp);
		strBaseString.Format("%03d, ", GC.getGame().getElapsedGameTurns());
		strBaseString += playerName + ", ";
		strOutBuf.Format("%s, City Plot Purchased, X: %d, Y: %d", getName().GetCString(), iPlotX, iPlotY);
		strBaseString += strOutBuf;
		pLog->Msg(strBaseString);
	}
	DoAcquirePlot(iPlotX, iPlotY);
		
#if defined(MOD_EVENTS_CITY)
	if (MOD_EVENTS_CITY) {
#if defined(MOD_UI_CITY_EXPANSION)
		GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityBoughtPlot, getOwner(), GetID(), iPlotX, iPlotY, bWithGold, !bWithGold);
#else
		GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityBoughtPlot, getOwner(), GetID(), iPlotX, iPlotY, true, false);
#endif
	} else {
#endif
	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if (pkScriptSystem) 
	{
		CvLuaArgsHandle args;
		args->Push(getOwner());
		args->Push(GetID());
#if defined(MOD_BUGFIX_MINOR)
		args->Push(iPlotX);
		args->Push(iPlotY);
#else
		args->Push(plot()->getX());
		args->Push(plot()->getY());
#endif
		args->Push(true); // bGold
		args->Push(false); // bFaith/bCulture

		bool bResult;
		LuaSupport::CallHook(pkScriptSystem, "CityBoughtPlot", args.get(), bResult);
	}
#if defined(MOD_EVENTS_CITY)
	}
#endif

#if !defined(NO_ACHIEVEMENTS)
	//Achievement test for purchasing 1000 tiles
	if(thisPlayer.isHuman() && !GC.getGame().isGameMultiPlayer())
	{
		gDLL->IncrementSteamStatAndUnlock(ESTEAMSTAT_TILESPURCHASED, 1000, ACHIEVEMENT_PURCHASE_1000TILES);
	}
#endif
}

//	--------------------------------------------------------------------------------
/// Acquire the plot and set it's owner to us
void CvCity::DoAcquirePlot(int iPlotX, int iPlotY)
{
	VALIDATE_OBJECT
	CvPlot* pPlot = GC.getMap().plot(iPlotX, iPlotY);
	if(!pPlot)
	{
		return;
	}

	GET_PLAYER(getOwner()).AddAPlot(pPlot);
	pPlot->setOwner(getOwner(), GetID(), /*bCheckUnits*/ true, /*bUpdateResources*/ true);

	DoUpdateCheapestPlotInfluence();
}

//	--------------------------------------------------------------------------------
/// Compute how valuable buying a plot is to this city
int CvCity::GetBuyPlotScore(int& iBestX, int& iBestY)
{
	VALIDATE_OBJECT
	CvPlot* pLoopPlot = NULL;
#if defined(MOD_GLOBAL_CITY_WORKING)
	const int iMaxRange = getBuyPlotDistance();
#else
	const int iMaxRange = /*3*/ GC.getMAXIMUM_BUY_PLOT_DISTANCE();
#endif

	int iBestScore = -1;
	int iTempScore;

	int iDX, iDY;

	for(iDX = -iMaxRange; iDX <= iMaxRange; iDX++)
	{
		for(iDY = -iMaxRange; iDY <= iMaxRange; iDY++)
		{
			pLoopPlot = plotXYWithRangeCheck(getX(), getY(), iDX, iDY, iMaxRange);
			if(pLoopPlot != NULL)
			{
				// Can we actually buy this plot?
				if(CanBuyPlot(pLoopPlot->getX(), pLoopPlot->getY()))
				{
					iTempScore = GetIndividualPlotScore(pLoopPlot);

					if(iTempScore > iBestScore)
					{
						iBestScore = iTempScore;
						iBestX = pLoopPlot->getX();
						iBestY = pLoopPlot->getY();
					}
				}
			}
		}
	}

	return iBestScore;
}

//	--------------------------------------------------------------------------------
/// Compute value of a plot we might buy
int CvCity::GetIndividualPlotScore(const CvPlot* pPlot) const
{
	VALIDATE_OBJECT
	int iRtnValue = 0;
	ResourceTypes eResource;
	int iYield;
	int iI;
	YieldTypes eSpecializationYield = NO_YIELD;
	CitySpecializationTypes eSpecialization;
	CvCity* pCity;

	eSpecialization = GetCityStrategyAI()->GetSpecialization();

	if(eSpecialization != NO_CITY_SPECIALIZATION)
	{
		eSpecializationYield = GC.getCitySpecializationInfo(eSpecialization)->GetYieldType();
	}

	// Does it have a resource?
	eResource = pPlot->getResourceType(getTeam());
	if(eResource != NO_RESOURCE)
	{
		CvResourceInfo *pkResource = GC.getResourceInfo(eResource);
		if (pkResource)
		{
			if(GET_TEAM(getTeam()).GetTeamTechs()->HasTech((TechTypes)pkResource->getTechReveal()))
			{
				int iRevealPolicy = pkResource->getPolicyReveal();
				if (iRevealPolicy == NO_POLICY || GET_PLAYER(getOwner()).GetPlayerPolicies()->HasPolicy((PolicyTypes)iRevealPolicy))
				{
					ResourceUsageTypes eResourceUsage = GC.getResourceInfo(eResource)->getResourceUsage();
					if(eResourceUsage == RESOURCEUSAGE_STRATEGIC)
					{
						iRtnValue += /* 50 */ GC.getAI_PLOT_VALUE_STRATEGIC_RESOURCE();
					}

					// Luxury resource?
					else if(eResourceUsage == RESOURCEUSAGE_LUXURY)
					{
						int iLuxuryValue = /* 40 */ GC.getAI_PLOT_VALUE_LUXURY_RESOURCE();

						// Luxury we don't have yet?
						if(GET_PLAYER(getOwner()).getNumResourceTotal(eResource) == 0)
							iLuxuryValue *= 2;

						iRtnValue += iLuxuryValue;
					}
				}
			}
		}
	}

	int iYieldValue = 0;
	int iTempValue;

	YieldTypes eYield;

	CvCityStrategyAI* pCityStrategyAI = GetCityStrategyAI();

	// Valuate the yields from this plot
	for(iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		eYield = (YieldTypes) iI;

		iYield = pPlot->calculateNatureYield(eYield, getTeam());
		iTempValue = 0;

		if(eYield == eSpecializationYield)
			iTempValue += iYield* /*20*/ GC.getAI_PLOT_VALUE_SPECIALIZATION_MULTIPLIER();

		else
			iTempValue += iYield* /*10*/ GC.getAI_PLOT_VALUE_YIELD_MULTIPLIER();

		// Deficient? If so, give it a boost
		if(pCityStrategyAI->IsYieldDeficient(eYield))
			iTempValue *= /*5*/ GC.getAI_PLOT_VALUE_DEFICIENT_YIELD_MULTIPLIER();

		iYieldValue += iTempValue;
	}

	iRtnValue += iYieldValue;

	// For each player not on our team, check how close their nearest city is to this plot
	CvPlayer& owningPlayer = GET_PLAYER(m_eOwner);
	CvDiplomacyAI* owningPlayerDiploAI = owningPlayer.GetDiplomacyAI();

	bool bAmerica = GET_PLAYER(getOwner()).GetPlayerTraits()->IsBuyOwnedTiles() && iRtnValue > 0;

	for(iI = 0; iI < MAX_MAJOR_CIVS; iI++)
	{
		CvPlayer& loopPlayer = GET_PLAYER((PlayerTypes)iI);
		if(loopPlayer.isAlive())
		{
			if(loopPlayer.getTeam() != getTeam())
			{
				DisputeLevelTypes eLandDisputeLevel = owningPlayerDiploAI->GetLandDisputeLevel((PlayerTypes)iI);

				if(eLandDisputeLevel != NO_DISPUTE_LEVEL && eLandDisputeLevel != DISPUTE_LEVEL_NONE)
				{
					pCity = GC.getMap().findCity(pPlot->getX(), pPlot->getY(), (PlayerTypes)iI, NO_TEAM, true /*bSameArea */);

					if(pCity)
					{
						int iDistance = plotDistance(pPlot->getX(), pPlot->getY(), pCity->getX(), pCity->getY());

						// Only want to account for civs with a city within 10 tiles
						if(iDistance < 10)
						{
							bool bManifestDestiny = bAmerica && pPlot->getOwner() == loopPlayer.GetID();

							switch(eLandDisputeLevel)
							{
							case DISPUTE_LEVEL_FIERCE:
								iRtnValue += (10 - iDistance) * /* 6 */ GC.getAI_PLOT_VALUE_FIERCE_DISPUTE();

								if (bManifestDestiny)
									iRtnValue *= 8;

								break;
							case DISPUTE_LEVEL_STRONG:
								iRtnValue += (10 - iDistance) * /* 4 */GC.getAI_PLOT_VALUE_STRONG_DISPUTE();

								if (bManifestDestiny)
									iRtnValue *= 4;

								break;
							case DISPUTE_LEVEL_WEAK:
								iRtnValue += (10 - iDistance) * /* 2 */ GC.getAI_PLOT_VALUE_WEAK_DISPUTE();

								if (bManifestDestiny)
									iRtnValue *= 2;

								break;
							}
						}
					}
				}
			}
		}
	}

	// Modify value based on cost - the higher it is compared to the "base" cost the less the value
	int iCost = GetBuyPlotCost(pPlot->getX(), pPlot->getY());
	iRtnValue *= GET_PLAYER(getOwner()).GetBuyPlotCost();

	// Protect against div by 0.
	CvAssertMsg(iCost != 0, "Plot cost is 0");
	if(iCost != 0)
		iRtnValue /= iCost;
	else
		iRtnValue = 0;

	return iRtnValue;
}

//	--------------------------------------------------------------------------------
/// What is the cheapest plot we can get
int CvCity::GetCheapestPlotInfluence() const
{
	return m_iCheapestPlotInfluence;
}

//	--------------------------------------------------------------------------------
/// What is the cheapest plot we can get
void CvCity::SetCheapestPlotInfluence(int iValue)
{
	if(m_iCheapestPlotInfluence != iValue)
		m_iCheapestPlotInfluence = iValue;

	CvAssertMsg(m_iCheapestPlotInfluence > 0, "Cheapest plot influence should never be 0 or less.");
}

//	--------------------------------------------------------------------------------
/// What is the cheapest plot we can get
void CvCity::DoUpdateCheapestPlotInfluence()
{
	int iLowestCost = INT_MAX;

	CvPlot* pLoopPlot = NULL;
	CvPlot* pThisPlot = plot();
	const int iMaxRange = /*5*/ GC.getMAXIMUM_ACQUIRE_PLOT_DISTANCE();
	CvMap& thisMap = GC.getMap();

	int iDX, iDY;

	for(iDX = -iMaxRange; iDX <= iMaxRange; iDX++)
	{
		for(iDY = -iMaxRange; iDY <= iMaxRange; iDY++)
		{
			pLoopPlot = plotXYWithRangeCheck(getX(), getY(), iDX, iDY, iMaxRange);
			if(pLoopPlot != NULL)
			{
#if defined(MOD_BUGFIX_MINOR)
				// If the plot's owned by anyone, we can't acquire it, so it doesn't matter
#else
				// If the plot's not owned by us, it doesn't matter
#endif
				if(pLoopPlot->getOwner() != NO_PLAYER)
					continue;
					
#if defined(MOD_EVENTS_CITY_BORDERS)
				// If we can't acquire it, it also doesn't matter
				if (MOD_EVENTS_CITY_BORDERS) {
					if (GAMEEVENTINVOKE_TESTALL(GAMEEVENT_CityCanAcquirePlot, getOwner(), GetID(), pLoopPlot->getX(), pLoopPlot->getY()) == GAMEEVENTRETURN_FALSE) {
						continue;
					}
				}
#endif				

				// we can use the faster, but slightly inaccurate pathfinder here - after all we are using a rand in the equation
				int iInfluenceCost = thisMap.calculateInfluenceDistance(pThisPlot, pLoopPlot, iMaxRange, false);

				if(iInfluenceCost > 0)
				{
					// Are we the cheapest yet?
					if(iInfluenceCost < iLowestCost)
						iLowestCost = iInfluenceCost;
				}
			}
		}
	}

	SetCheapestPlotInfluence(iLowestCost);
}

//	--------------------------------------------------------------------------------
/// Setting the danger value threat amount
void CvCity::SetThreatValue(int iThreatValue)
{
	VALIDATE_OBJECT
	m_iThreatValue = iThreatValue;
}

//	--------------------------------------------------------------------------------
/// Getting the danger value threat amount
int CvCity::getThreatValue(void)
{
	VALIDATE_OBJECT
	return m_iThreatValue;
}

//	--------------------------------------------------------------------------------
void CvCity::clearOrderQueue()
{
	VALIDATE_OBJECT
	while(headOrderQueueNode() != NULL)
	{
		popOrder(0);
	}
}


//	--------------------------------------------------------------------------------
void CvCity::pushOrder(OrderTypes eOrder, int iData1, int iData2, bool bSave, bool bPop, bool bAppend, bool bRush)
{
	VALIDATE_OBJECT
	OrderData order;
	bool bValid;

	if(bPop)
	{
		clearOrderQueue();
	}

	bValid = false;

	switch(eOrder)
	{
	case ORDER_TRAIN:
		if(canTrain((UnitTypes)iData1))
		{
			CvUnitEntry* pkUnitInfo = GC.getUnitInfo((UnitTypes)iData1);
			if(pkUnitInfo)
			{
				if(iData2 == -1)
				{
					iData2 = pkUnitInfo->GetDefaultUnitAIType();
				}

				GET_PLAYER(getOwner()).changeUnitClassMaking(((UnitClassTypes)(pkUnitInfo->GetUnitClassType())), 1);

				bValid = true;
			}
		}
		break;

	case ORDER_CONSTRUCT:
		if(canConstruct((BuildingTypes)iData1))
		{
			CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo((BuildingTypes)iData1);
			if(pkBuildingInfo)
			{
				GET_PLAYER(getOwner()).changeBuildingClassMaking(((BuildingClassTypes)(pkBuildingInfo->GetBuildingClassType())), 1);

				bValid = true;
			}
		}
		break;

	case ORDER_CREATE:
		if(canCreate((ProjectTypes)iData1))
		{
			GET_TEAM(getTeam()).changeProjectMaking(((ProjectTypes)iData1), 1);
			GET_PLAYER(getOwner()).changeProjectMaking(((ProjectTypes)iData1), 1);

			bValid = true;
		}
		break;

	case ORDER_PREPARE:
		if(canPrepare((SpecialistTypes)iData1))
		{
			bValid = true;
		}
		break;

	case ORDER_MAINTAIN:
		if(canMaintain((ProcessTypes)iData1))
		{
			bValid = true;
		}
		break;

	default:
		CvAssertMsg(false, "iOrder did not match a valid option");
		break;
	}

	if(!bValid)
	{
		return;
	}

	order.eOrderType = eOrder;
	order.iData1 = iData1;
	order.iData2 = iData2;
	order.bSave = bSave;
	order.bRush = bRush;

	if(bAppend)
	{
		m_orderQueue.insertAtEnd(&order);
	}
	else
	{
		stopHeadOrder();
		m_orderQueue.insertAtBeginning(&order);
	}

	if(!bAppend || (getOrderQueueLength() == 1))
	{
		startHeadOrder();
	}

#if defined(MOD_ROG_CORE)
	if (eOrder == ORDER_MAINTAIN && (ProcessTypes)iData1 != NO_PROCESS)
	{
		CvProcessInfo* pkProcessInfo = GC.getProcessInfo((ProcessTypes)iData1);
		if (pkProcessInfo && pkProcessInfo->getDefenseValue() != 0)
		{
			updateStrengthValue();
		}
	}
#endif

	if((getTeam() == GC.getGame().getActiveTeam()) || GC.getGame().isDebugMode())
	{
		if(isCitySelected())
		{
			DLLUI->setDirty(SelectionButtons_DIRTY_BIT, true);
			DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
			DLLUI->setDirty(PlotListButtons_DIRTY_BIT, true);
		}
	}

	DLLUI->setDirty(CityInfo_DIRTY_BIT, true);

	auto_ptr<ICvCity1> pCity = GC.WrapCityPointer(this);
	DLLUI->SetSpecificCityInfoDirty(pCity.get(), CITY_UPDATE_TYPE_PRODUCTION);
}


//	--------------------------------------------------------------------------------
void CvCity::popOrder(int iNum, bool bFinish, bool bChoose)
{
	VALIDATE_OBJECT

	CvPlayerAI& kOwner = GET_PLAYER(getOwner());	//Used often later on

	OrderData* pOrderNode;
	SpecialistTypes eSpecialist;
	ProjectTypes eCreateProject;
	BuildingTypes eConstructBuilding;
	UnitTypes eTrainUnit;
	UnitAITypes eTrainAIUnit;
	bool bWasFoodProduction;
	bool bStart;
	bool bMessage;
	int iCount;
	int iProductionNeeded;

	bWasFoodProduction = isFoodProduction();

	if(iNum == -1)
	{
		iNum = (getOrderQueueLength() - 1);
	}

	iCount = 0;

	pOrderNode = headOrderQueueNode();

	while(pOrderNode != NULL)
	{
		if(iCount == iNum)
		{
			break;
		}

		iCount++;

		pOrderNode = nextOrderQueueNode(pOrderNode);
	}

	if(pOrderNode == NULL)
	{
		return;
	}

	if(bFinish)
	{
		m_iThingsProduced++;
	}

	if(bFinish && pOrderNode->bSave)
	{
		pushOrder(pOrderNode->eOrderType, pOrderNode->iData1, pOrderNode->iData2, true, false, true);
	}

#if defined(MOD_ROG_CORE)
	bool bUpdateStrength = false;
#endif

	eTrainUnit = NO_UNIT;
	eConstructBuilding = NO_BUILDING;
	eCreateProject = NO_PROJECT;
	eSpecialist = NO_SPECIALIST;

	switch(pOrderNode->eOrderType)
	{
	case ORDER_TRAIN:
		eTrainUnit = ((UnitTypes)(pOrderNode->iData1));
		eTrainAIUnit = ((UnitAITypes)(pOrderNode->iData2));
		CvAssertMsg(eTrainUnit != NO_UNIT, "eTrainUnit is expected to be assigned a valid unit type");
		CvAssertMsg(eTrainAIUnit != NO_UNITAI, "eTrainAIUnit is expected to be assigned a valid unit AI type");

		kOwner.changeUnitClassMaking(((UnitClassTypes)(GC.getUnitInfo(eTrainUnit)->GetUnitClassType())), -1);

		if(bFinish)
		{
			int iResult = CreateUnit(eTrainUnit, false, false, eTrainAIUnit);
			
			if(iResult != FFreeList::INVALID_INDEX)
			{
#if defined(MOD_EVENTS_CITY)
				if (MOD_EVENTS_CITY) {
					GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityTrained, getOwner(), GetID(), iResult, false, false);
				} else {
#endif
				ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
				if (pkScriptSystem) 
				{
					CvLuaArgsHandle args;
					args->Push(getOwner());
					args->Push(GetID());
					args->Push(GET_PLAYER(getOwner()).getUnit(iResult)->GetID()); // This is probably just iResult
					args->Push(false); // bGold
					args->Push(false); // bFaith/bCulture

					bool bResult;
					LuaSupport::CallHook(pkScriptSystem, "CityTrained", args.get(), bResult);
				}
#if defined(MOD_EVENTS_CITY)
				}
#endif

				iProductionNeeded = getProductionNeeded(eTrainUnit) * 100;

				// max overflow is the value of the item produced (to eliminate prebuild exploits)
				int iOverflow = getUnitProductionTimes100(eTrainUnit) - iProductionNeeded;
				int iMaxOverflow = std::max(iProductionNeeded, getCurrentProductionDifferenceTimes100(false, false));
				int iLostProduction = std::max(0, iOverflow - iMaxOverflow);
				iOverflow = std::min(iMaxOverflow, iOverflow);
				if(iOverflow > 0)
				{
					changeOverflowProductionTimes100(iOverflow);
				}
				setUnitProduction(eTrainUnit, 0);

				int iProductionGold = ((iLostProduction * GC.getMAXED_UNIT_GOLD_PERCENT()) / 100);
				if(iProductionGold > 0)
				{
					kOwner.GetTreasury()->ChangeGoldTimes100(iProductionGold);
				}
			}
			else
			{
				// create notification
				setUnitProduction(eTrainUnit, getProductionNeeded(eTrainUnit) - 1);

				CvNotifications* pNotifications = kOwner.GetNotifications();
				if(pNotifications)
				{
					Localization::String strText = Localization::Lookup("TXT_KEY_NOTIFICATION_REMOVED_UNIT");
					strText << getNameKey();
					strText << GC.getUnitInfo(eTrainUnit)->GetDescription();
					Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_REMOVED_UNIT");
					pNotifications->Add(NOTIFICATION_GENERIC, strText.toUTF8(), strSummary.toUTF8(), getX(), getY(), -1);
				}
			}
		}
		break;

	case ORDER_CONSTRUCT:
	{
		eConstructBuilding = ((BuildingTypes)(pOrderNode->iData1));

		CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eConstructBuilding);

		if(pkBuildingInfo)
		{
			kOwner.changeBuildingClassMaking(((BuildingClassTypes)(pkBuildingInfo->GetBuildingClassType())), -1);

			if(bFinish)
			{
				bool bResult = CreateBuilding(eConstructBuilding);
				DEBUG_VARIABLE(bResult);
				CvAssertMsg(bResult, "CreateBuilding failed");

#if defined(MOD_EVENTS_CITY)
				if (MOD_EVENTS_CITY) {
					GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityConstructed, getOwner(), GetID(), eConstructBuilding, false, false);
				} else {
#endif
				ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
				if (pkScriptSystem) 
				{
					CvLuaArgsHandle args;
					args->Push(getOwner());
					args->Push(GetID());
					args->Push(eConstructBuilding);
					args->Push(false); // bGold
					args->Push(false); // bFaith/bCulture

					bool bScriptResult;
					LuaSupport::CallHook(pkScriptSystem, "CityConstructed", args.get(), bScriptResult);
				}
#if defined(MOD_EVENTS_CITY)
				}
#endif


#if defined(MOD_ROG_CORE)
				if (MOD_ROG_CORE) {
					YieldTypes eYield;
					for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
					{
						eYield = (YieldTypes)iI;
						int iCost = pkBuildingInfo->GetProductionCost();
						iCost *= GC.getGame().getGameSpeedInfo().getConstructPercent();
						iCost /= 100;
						if (GetYieldFromConstruction(eYield) > 0 && iCost > 0)
						{
							iCost *= GetYieldFromConstruction(eYield) / 100;
							doInstantYield(eYield, iCost);
						}
					}
				}
#endif

				iProductionNeeded = getProductionNeeded(eConstructBuilding) * 100;
				// max overflow is the value of the item produced (to eliminate prebuild exploits)
				int iOverflow = m_pCityBuildings->GetBuildingProductionTimes100(eConstructBuilding) - iProductionNeeded;
				int iMaxOverflow = std::max(iProductionNeeded, getCurrentProductionDifferenceTimes100(false, false));
				int iLostProduction = std::max(0, iOverflow - iMaxOverflow);
				iOverflow = std::min(iMaxOverflow, iOverflow);
				if(iOverflow > 0)
				{
					changeOverflowProductionTimes100(iOverflow);
				}
				m_pCityBuildings->SetBuildingProduction(eConstructBuilding, 0);

				int iProductionGold = ((iLostProduction * GC.getMAXED_BUILDING_GOLD_PERCENT()) / 100);
				if(iProductionGold > 0)
				{
					kOwner.GetTreasury()->ChangeGoldTimes100(iProductionGold);
				}
				
#if defined(MOD_TRADE_WONDER_RESOURCE_ROUTES)
				if (MOD_TRADE_WONDER_RESOURCE_ROUTES) {
					// If the AI has just finished building a World Wonder, cancel any marble trade routes into this city
					if (!GetPlayer()->isHuman()) {
						const BuildingClassTypes eBuildingClass = (BuildingClassTypes)pkBuildingInfo->GetBuildingClassType();
						CvBuildingClassInfo* pBuildingClass = GC.getBuildingClassInfo(eBuildingClass);
						if (pBuildingClass && ::isWorldWonderClass(*pBuildingClass)) {
							CvGameTrade* pGameTrade = GC.getGame().GetGameTrade();
							for (uint ui = 0; ui < pGameTrade->m_aTradeConnections.size(); ui++) {
								if (pGameTrade->IsTradeRouteIndexEmpty(ui)) {
									continue;
								}

								TradeConnection kConnection = pGameTrade->m_aTradeConnections[ui];

								if (kConnection.m_eConnectionType == TRADE_CONNECTION_WONDER_RESOURCE) {
									CvCity* pDestCity = CvGameTrade::GetDestCity(kConnection);

									if (pDestCity->getX() == getX() && pDestCity->getY() == getY()) {
										kConnection.m_iCircuitsCompleted = kConnection.m_iCircuitsToComplete;
									}
								}
							}
						}
					}
				}
#endif

				if(GC.getLogging() && GC.getAILogging())
				{
					CvBuildingEntry* pkConstructBuildingInfo = GC.getBuildingInfo(eConstructBuilding);
					if(pkConstructBuildingInfo)
					{
						if(kOwner.GetWonderProductionAI()->IsWonder(*pkConstructBuildingInfo))
						{
							CvString playerName;
							FILogFile* pLog;
							CvString strBaseString;
							CvString strOutBuf;
							playerName = kOwner.getCivilizationShortDescription();
							pLog = LOGFILEMGR.GetLog(kOwner.GetCitySpecializationAI()->GetLogFileName(playerName), FILogFile::kDontTimeStamp);
							strBaseString.Format("%03d, ", GC.getGame().getElapsedGameTurns());
							strBaseString += playerName + ", ";
							strOutBuf.Format("%s, WONDER - Finished %s", getName().GetCString(), pkConstructBuildingInfo->GetDescription());
							strBaseString += strOutBuf;
							pLog->Msg(strBaseString);
						}
					}

				}
			}
		}
		break;
	}

	case ORDER_CREATE:
		eCreateProject = ((ProjectTypes)(pOrderNode->iData1));

		GET_TEAM(getTeam()).changeProjectMaking(eCreateProject, -1);
		kOwner.changeProjectMaking(eCreateProject, -1);

		if(bFinish)
		{
			bool bResult = CreateProject(eCreateProject);
			DEBUG_VARIABLE(bResult);
			CvAssertMsg(bResult, "Failed to create project");

#if defined(MOD_EVENTS_CITY)
			if (MOD_EVENTS_CITY) {
				GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityCreated, getOwner(), GetID(), eCreateProject, false, false);
			} else {
#endif
			ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
			if (pkScriptSystem) 
			{
				CvLuaArgsHandle args;
				args->Push(getOwner());
				args->Push(GetID());
				args->Push(eCreateProject);
				args->Push(false); // bGold
				args->Push(false); // bFaith/bCulture

				bool bScriptResult;
				LuaSupport::CallHook(pkScriptSystem, "CityCreated", args.get(), bScriptResult);
			}
#if defined(MOD_EVENTS_CITY)
			}
#endif

			iProductionNeeded = getProductionNeeded(eCreateProject) * 100;
			// max overflow is the value of the item produced (to eliminate prebuild exploits)
			int iOverflow = getProjectProductionTimes100(eCreateProject) - iProductionNeeded;
			int iMaxOverflow = std::max(iProductionNeeded, getCurrentProductionDifferenceTimes100(false, false));
			int iLostProduction = std::max(0, iOverflow - iMaxOverflow);
			iOverflow = std::min(iMaxOverflow, iOverflow);
			if(iOverflow > 0)
			{
				changeOverflowProductionTimes100(iOverflow);
			}
			setProjectProduction(eCreateProject, 0);

			int iProductionGold = ((iLostProduction * GC.getMAXED_PROJECT_GOLD_PERCENT()) / 100);
			if(iProductionGold > 0)
			{
				kOwner.GetTreasury()->ChangeGoldTimes100(iProductionGold);
			}
		}
		break;

	case ORDER_PREPARE:

		if(bFinish)
		{
			eSpecialist = ((SpecialistTypes)(pOrderNode->iData1));

#if defined(MOD_EVENTS_CITY)
			if (MOD_EVENTS_CITY) {
				GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityPrepared, getOwner(), GetID(), eSpecialist, false, false);
			}
#endif

			iProductionNeeded = getProductionNeeded(eSpecialist) * 100;

			// max overflow is the value of the item produced (to eliminate prebuild exploits)
			int iOverflow = getSpecialistProductionTimes100(eSpecialist) - iProductionNeeded;
			int iMaxOverflow = std::max(iProductionNeeded, getCurrentProductionDifferenceTimes100(false, false));
			iOverflow = std::min(iMaxOverflow, iOverflow);
			if(iOverflow > 0)
			{
				changeOverflowProductionTimes100(iOverflow);
			}

			setSpecialistProduction(eSpecialist, 0);
		}

		break;

	case ORDER_MAINTAIN:

#if defined(MOD_ROG_CORE)
		if ((ProcessTypes)pOrderNode->iData1 != NO_PROCESS)
		{
			CvProcessInfo* pkProcessInfo = GC.getProcessInfo((ProcessTypes)pOrderNode->iData1);
			if (pkProcessInfo && pkProcessInfo->getDefenseValue() != 0)
			{
				bUpdateStrength = true;
			}
		}
#endif

		break;

	default:
		CvAssertMsg(false, "pOrderNode->eOrderType is not a valid option");
		break;
	}

	if(m_unitBeingBuiltForOperation.IsValid())
	{
		kOwner.CityUncommitToBuildUnitForOperationSlot(m_unitBeingBuiltForOperation);
		m_unitBeingBuiltForOperation.Invalidate();
	}

	if(pOrderNode == headOrderQueueNode())
	{
		bStart = true;
		stopHeadOrder();
	}
	else
	{
		bStart = false;
	}

	m_orderQueue.deleteNode(pOrderNode);
	pOrderNode = NULL;
	if(bFinish)
	{
		CleanUpQueue(); // cleans out items from the queue that may be invalidated by the recent construction
	}

	if(bStart)
	{
		startHeadOrder();
	}

	if((getTeam() == GC.getGame().getActiveTeam()) || GC.getGame().isDebugMode())
	{
		if(isCitySelected())
		{
			DLLUI->setDirty(SelectionButtons_DIRTY_BIT, true);
			DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
		}
	}

	bMessage = false;

	if(bChoose)
	{
		if(getOrderQueueLength() == 0)
		{
			if(!isHuman() || isProductionAutomated())
			{
				AI_chooseProduction(false /*bInterruptWonders*/);
			}
			else
			{
				chooseProduction(eTrainUnit, eConstructBuilding, eCreateProject, bFinish);

				bMessage = true;
			}
		}
	}

	if(bFinish && !bMessage)
	{
		if(getOwner() == GC.getGame().getActivePlayer())
		{
			Localization::String localizedText;
			if(eTrainUnit != NO_UNIT)
			{
				CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eTrainUnit);
				if(pkUnitInfo)
				{
					localizedText = Localization::Lookup(((isLimitedUnitClass((UnitClassTypes)(pkUnitInfo->GetUnitClassType()))) ? "TXT_KEY_MISC_TRAINED_UNIT_IN_LIMITED" : "TXT_KEY_MISC_TRAINED_UNIT_IN"));
					localizedText << pkUnitInfo->GetTextKey() << getNameKey();
				}
			}
			else if(eConstructBuilding != NO_BUILDING)
			{
				CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eConstructBuilding);
				if(pkBuildingInfo)
				{
					localizedText = Localization::Lookup(((isLimitedWonderClass(pkBuildingInfo->GetBuildingClassInfo())) ? "TXT_KEY_MISC_CONSTRUCTED_BUILD_IN_LIMITED" : "TXT_KEY_MISC_CONSTRUCTED_BUILD_IN"));
					localizedText << pkBuildingInfo->GetTextKey() << getNameKey();
				}
			}
			else if(eCreateProject != NO_PROJECT)
			{
				localizedText = Localization::Lookup(((isLimitedProject(eCreateProject)) ? "TXT_KEY_MISC_CREATED_PROJECT_IN_LIMITED" : "TXT_KEY_MISC_CREATED_PROJECT_IN"));
				localizedText << GC.getProjectInfo(eCreateProject)->GetTextKey() << getNameKey();
			}
			if(isProduction())
			{
				localizedText = Localization::Lookup(((isProductionLimited()) ? "TXT_KEY_MISC_WORK_HAS_BEGUN_LIMITED" : "TXT_KEY_MISC_WORK_HAS_BEGUN"));
				localizedText << getProductionNameKey();
			}
			DLLUI->AddCityMessage(0, GetIDInfo(), getOwner(), false, GC.getEVENT_MESSAGE_TIME(), localizedText.toUTF8()/*, szSound, MESSAGE_TYPE_MINOR_EVENT, szIcon, (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"), getX(), getY(), true, true*/);
		}
	}

#if defined(MOD_ROG_CORE)
	if (bUpdateStrength)
	{
		updateStrengthValue();
	}
#endif

	if((getTeam() == GC.getGame().getActiveTeam()) || GC.getGame().isDebugMode())
	{
		if(isCitySelected())
		{
			DLLUI->setDirty(SelectionButtons_DIRTY_BIT, true);
			DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
			DLLUI->setDirty(PlotListButtons_DIRTY_BIT, true);
		}
		DLLUI->setDirty(CityInfo_DIRTY_BIT, true);
	}
}

//	--------------------------------------------------------------------------------
void CvCity::swapOrder(int iNum)
{
	// okay, this only swaps the order with the next one up in the queue
	VALIDATE_OBJECT

	if(iNum == 0)
	{
		stopHeadOrder();
	}

	m_orderQueue.swapUp(iNum);

	if(iNum == 0)
	{
		startHeadOrder();
	}

	if((getTeam() == GC.getGame().getActiveTeam()) || GC.getGame().isDebugMode())
	{
		if(isCitySelected())
		{
			//DLLUI->setDirty(InfoPane_DIRTY_BIT, true );
			DLLUI->setDirty(SelectionButtons_DIRTY_BIT, true);
			DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
			DLLUI->setDirty(PlotListButtons_DIRTY_BIT, true);
		}
		DLLUI->setDirty(CityInfo_DIRTY_BIT, true);
	}
}


//	--------------------------------------------------------------------------------
void CvCity::startHeadOrder()
{
	VALIDATE_OBJECT
	OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		if(pOrderNode->eOrderType == ORDER_MAINTAIN)
		{
			processProcess(((ProcessTypes)(pOrderNode->iData1)), 1);
		}
	}
}


//	--------------------------------------------------------------------------------
void CvCity::stopHeadOrder()
{
	VALIDATE_OBJECT
	OrderData* pOrderNode = headOrderQueueNode();

	if(pOrderNode != NULL)
	{
		if(pOrderNode->eOrderType == ORDER_MAINTAIN)
		{
			processProcess(((ProcessTypes)(pOrderNode->iData1)), -1);
		}
	}
}


//	--------------------------------------------------------------------------------
int CvCity::getOrderQueueLength()
{
	VALIDATE_OBJECT
	return m_orderQueue.getLength();
}


//	--------------------------------------------------------------------------------
OrderData* CvCity::getOrderFromQueue(int iIndex)
{
	VALIDATE_OBJECT
	OrderData* pOrderNode;

	pOrderNode = m_orderQueue.nodeNum(iIndex);

	if(pOrderNode != NULL)
	{
		return pOrderNode;
	}
	else
	{
		return NULL;
	}
}


//	--------------------------------------------------------------------------------
OrderData* CvCity::nextOrderQueueNode(OrderData* pNode)
{
	VALIDATE_OBJECT
	return m_orderQueue.next(pNode);
}

//	--------------------------------------------------------------------------------
const OrderData* CvCity::nextOrderQueueNode(const OrderData* pNode) const
{
	VALIDATE_OBJECT
	return m_orderQueue.next(pNode);
}


//	--------------------------------------------------------------------------------
const OrderData* CvCity::headOrderQueueNode() const
{
	VALIDATE_OBJECT
	return m_orderQueue.head();
}

//	--------------------------------------------------------------------------------
OrderData* CvCity::headOrderQueueNode()
{
	VALIDATE_OBJECT
	return m_orderQueue.head();
}


//	--------------------------------------------------------------------------------
const OrderData* CvCity::tailOrderQueueNode() const
{
	VALIDATE_OBJECT
	return m_orderQueue.tail();
}

//	--------------------------------------------------------------------------------
/// remove items in the queue that are no longer valid
bool CvCity::CleanUpQueue(void)
{
	VALIDATE_OBJECT
	bool bOK = true;

	for(int iI = (getOrderQueueLength() - 1); iI >= 0; iI--)
	{
		OrderData* pOrder = getOrderFromQueue(iI);

		if(pOrder != NULL)
		{
			if(!canContinueProduction(*pOrder))
			{
				popOrder(iI, false, true);
				bOK = false;
			}
		}
	}

	return bOK;
}

//	--------------------------------------------------------------------------------
int CvCity::CreateUnit(UnitTypes eUnitType, bool bIsGold, bool bIsFaith, UnitAITypes eAIType, bool bUseToSatisfyOperation)
{
	VALIDATE_OBJECT
	CvPlayer& thisPlayer = GET_PLAYER(getOwner());
	CvUnit* pUnit = thisPlayer.initUnit(eUnitType, getX(), getY(), eAIType);
	CvAssertMsg(pUnit, "");
	if(!pUnit)
	{
		CvAssertMsg(false, "CreateUnit failed");
		return FFreeList::INVALID_INDEX;
	}

	if(pUnit->IsHasNoValidMove())
	{
		pUnit->kill(false);
		return FFreeList::INVALID_INDEX;
	}
	CvUnitEntry & pkUnitInfo = pUnit->getUnitInfo();
	addProductionExperience(pUnit);

	CvPlot* pRallyPlot = getRallyPlot();
	if(pRallyPlot != NULL)
	{
		pUnit->PushMission(CvTypes::getMISSION_MOVE_TO(), pRallyPlot->getX(), pRallyPlot->getY());
	}

	if(bUseToSatisfyOperation && m_unitBeingBuiltForOperation.IsValid())
	{
		thisPlayer.CityFinishedBuildingUnitForOperationSlot(m_unitBeingBuiltForOperation, pUnit);
		m_unitBeingBuiltForOperation.Invalidate();
	}

	// Any AI unit with explore AI as a secondary unit AI (e.g. warriors) are assigned that unit AI if this AI player needs to explore more
	else if(!pUnit->isHuman() && !thisPlayer.isMinorCiv())
	{
		EconomicAIStrategyTypes eStrategy = (EconomicAIStrategyTypes) GC.getInfoTypeForString("ECONOMICAISTRATEGY_NEED_RECON");
		if(thisPlayer.GetEconomicAI()->IsUsingStrategy(eStrategy))
		{
			if(pkUnitInfo.GetUnitAIType(UNITAI_EXPLORE) && pUnit->AI_getUnitAIType() != UNITAI_EXPLORE)
			{

				// Now make sure there isn't a critical military threat
				CvMilitaryAI* thisPlayerMilAI = thisPlayer.GetMilitaryAI();
				int iThreat = thisPlayerMilAI->GetThreatTotal();
				iThreat += thisPlayerMilAI->GetBarbarianThreatTotal();
				if(iThreat < thisPlayerMilAI->GetThreatWeight(THREAT_CRITICAL))
				{
					pUnit->AI_setUnitAIType(UNITAI_EXPLORE);
					if(GC.getLogging() && GC.getAILogging())
					{
						CvString strLogString;
						strLogString.Format("Assigning explore unit AI to %s, X: %d, Y: %d", pUnit->getName().GetCString(), pUnit->getX(), pUnit->getY());
						thisPlayer.GetHomelandAI()->LogHomelandMessage(strLogString);
					}
				}
				else
				{
					if(GC.getLogging() && GC.getAILogging())
					{
						CvString strLogString;
						strLogString.Format("Not assigning explore AI to %s due to threats, X: %d, Y: %d", pUnit->getName().GetCString(), pUnit->getX(), pUnit->getY());
						thisPlayer.GetHomelandAI()->LogHomelandMessage(strLogString);
					}
				}
			}
		}
		eStrategy = (EconomicAIStrategyTypes) GC.getInfoTypeForString("ECONOMICAISTRATEGY_NEED_RECON_SEA");
		EconomicAIStrategyTypes eOtherStrategy = (EconomicAIStrategyTypes) GC.getInfoTypeForString("ECONOMICAISTRATEGY_REALLY_NEED_RECON_SEA");
		if(thisPlayer.GetEconomicAI()->IsUsingStrategy(eStrategy) || thisPlayer.GetEconomicAI()->IsUsingStrategy(eOtherStrategy))
		{
			if(pkUnitInfo.GetUnitAIType(UNITAI_EXPLORE_SEA))
			{
				pUnit->AI_setUnitAIType(UNITAI_EXPLORE_SEA);
				if(GC.getLogging() && GC.getAILogging())
				{
					CvString strLogString;
					strLogString.Format("Assigning explore sea unit AI to %s, X: %d, Y: %d", pUnit->getName().GetCString(), pUnit->getX(), pUnit->getY());
					thisPlayer.GetHomelandAI()->LogHomelandMessage(strLogString);
				}
			}
		}
	}
	int iPopConsume = pkUnitInfo.GetTrainPopulationConsume();
	if(iPopConsume > 0 && !bIsFaith)
	{
		if(pUnit->getUnitClassType() == GC.getInfoTypeForString("UNITCLASS_SETTLER"))
		{
			iPopConsume += thisPlayer.getPolicyModifiers(POLICYMOD_SETTLER_POPULATION_CONSUME);
		}
		iPopConsume = std::min(getPopulation() -1, iPopConsume);
		if(iPopConsume != 0)
		{
			pUnit->SetExtraPopConsume(iPopConsume);
			changePopulation(-iPopConsume);
			if (thisPlayer.isHuman())
			{
				CvNotifications *pNotifications = thisPlayer.GetNotifications();
				if (pNotifications)
				{
					Localization::String strMessage = Localization::Lookup("TXT_KEY_NOTIFICATION_SETTLER_TRAINED_CITY");
					strMessage << iPopConsume;
					strMessage << getNameKey();
					Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SETTLER_TRAINED_CITY_SHORT");
					pNotifications->Add(NOTIFICATION_STARVING, strMessage.toUTF8(), strSummary.toUTF8(), getX(), getY(), -1);
				}
			}
		}
	}
	//Increment for stat tracking and achievements
	if(pUnit->isHuman())
	{
		IncrementUnitStatCount(pUnit);
	}

	YieldTypes eYield;
	int iCost = pkUnitInfo.GetProductionCost();
	iCost *= GC.getGame().getGameSpeedInfo().getConstructPercent();
	iCost /= 100;
	int iStrength = std::max(pUnit->GetBaseCombatStrength(), pUnit->GetBaseRangedCombatStrength());
	PromotionTypes ePromotionOceanImpassable = (PromotionTypes)GC.getPROMOTION_OCEAN_IMPASSABLE();
	int iTempValue = 0;
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		eYield = (YieldTypes)iI;
		iTempValue = GetYieldFromUnitProduction(eYield);
		if (iTempValue > 0 && !bIsGold && !bIsFaith)
		{
			iTempValue *= iCost;
			iTempValue /= 100;
			doInstantYield(eYield, iTempValue);
		}
		iTempValue = pkUnitInfo.GetInstantYieldFromTrainings(eYield);
		if (iTempValue > 0)
		{
			doInstantYield(eYield, iTempValue);
		}
		if(eYield == YIELD_CULTURE)
		{
			iTempValue = thisPlayer.GetPlayerTraits()->GetCultureBonusUnitStrengthModify();
			iTempValue *= iStrength;
			iTempValue /= 100;
			if(iTempValue > 0)
			{
				doInstantYield(eYield, iTempValue);
				if (thisPlayer.isHuman())
				{
					ICvUserInterface2* pkDLLInterface = GC.GetEngineUserInterface();
					CvString strBuffer = GetLocalizedText("TXT_KEY_TRAIT_CULTURE_FROM_UNIT", iTempValue, pUnit->getName());
					pkDLLInterface->AddMessage(0, thisPlayer.GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer /*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
				}
			}
			iTempValue = thisPlayer.getPolicyModifiers(POLICYMOD_DEEP_WATER_NAVAL_CULTURE_STRENGTH_MODIFIER);
			iTempValue *= iStrength;
			iTempValue /= 100;
			if(iTempValue > 0 && pUnit->getDomainType() == DOMAIN_SEA && !pUnit->isHasPromotion(ePromotionOceanImpassable))
			{
				doInstantYield(eYield, iTempValue);
				if (thisPlayer.isHuman())
				{
					ICvUserInterface2* pkDLLInterface = GC.GetEngineUserInterface();
					CvString strBuffer = GetLocalizedText("TXT_KEY_POLICY_CULTURE_FROM_UNIT", iTempValue, pUnit->getName());
					pkDLLInterface->AddMessage(0, thisPlayer.GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer /*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
				}
			}
			
		}
		
	}
	return pUnit->GetID();
}

//	--------------------------------------------------------------------------------
bool CvCity::CreateBuilding(BuildingTypes eBuildingType)
{
	VALIDATE_OBJECT

	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuildingType);
	if(pkBuildingInfo == NULL)
		return false;

	const BuildingClassTypes eBuildingClass = (BuildingClassTypes)pkBuildingInfo->GetBuildingClassType();

	CvPlayerAI& kPlayer = GET_PLAYER(getOwner());

	if(kPlayer.isBuildingClassMaxedOut(eBuildingClass, 0))
	{
		kPlayer.removeBuildingClass(eBuildingClass);
	}

	m_pCityBuildings->SetNumRealBuilding(eBuildingType, m_pCityBuildings->GetNumRealBuilding(eBuildingType) + 1);

#if !defined(NO_ACHIEVEMENTS)
	//Achievements
	if(kPlayer.isHuman() && !GC.getGame().isGameMultiPlayer())
	{
		CvBuildingClassInfo* pBuildingClass = GC.getBuildingClassInfo(eBuildingClass);
		if(pBuildingClass && ::isWorldWonderClass(*pBuildingClass))
		{
			int iCount = 0;
			CvGameTrade* pGameTrade = GC.getGame().GetGameTrade();
			for (uint ui = 0; ui < pGameTrade->m_aTradeConnections.size(); ui++)
			{
				if (pGameTrade->IsTradeRouteIndexEmpty(ui))
				{
					continue;
				}

				if (pGameTrade->m_aTradeConnections[ui].m_eConnectionType == TRADE_CONNECTION_PRODUCTION)
				{
					CvCity* pDestCity = CvGameTrade::GetDestCity(pGameTrade->m_aTradeConnections[ui]);
					if (pDestCity->getX() == getX() && pDestCity->getY() == getY())
					{
						iCount++;
					}
				}
			}

			if (iCount >= 3) 
			{
				gDLL->UnlockAchievement(ACHIEVEMENT_XP2_31);
			}
		}

		CheckForAchievementBuilding(eBuildingType);
	}
#endif

	return true;
}


//	--------------------------------------------------------------------------------
bool CvCity::CreateProject(ProjectTypes eProjectType, bool bIsCapture)
{
	VALIDATE_OBJECT

	CvPlayer& thisPlayer = GET_PLAYER(getOwner());
	CvTeam& thisTeam = GET_TEAM(getTeam());
	thisTeam.changeProjectCount(eProjectType, 1, bIsCapture);

	changeProjectCount(eProjectType, 1);

	ProjectTypes ApolloProgram = (ProjectTypes) GC.getSPACE_RACE_TRIGGER_PROJECT();
	ProjectTypes capsuleID = (ProjectTypes) GC.getSPACESHIP_CAPSULE();
	ProjectTypes boosterID = (ProjectTypes) GC.getSPACESHIP_BOOSTER();
	ProjectTypes stasisID = (ProjectTypes) GC.getSPACESHIP_STASIS();
	ProjectTypes engineID = (ProjectTypes) GC.getSPACESHIP_ENGINE();

	enum eSpaceshipState
	{
	    eUnderConstruction	= 0x0000,
	    eFrame				= 0x0001,
	    eCapsule			= 0x0002,
	    eStasis_Chamber		= 0x0004,
	    eEngine				= 0x0008,
	    eBooster1			= 0x0010,
	    eBooster2			= 0x0020,
	    eBooster3			= 0x0040,
	    eConstructed		= 0x0080,
	};

	if(eProjectType == ApolloProgram)
	{
		CvCity* theCapital = thisPlayer.getCapitalCity();
		if(theCapital)
		{
			auto_ptr<ICvPlot1> pDllPlot(new CvDllPlot(theCapital->plot()));
			gDLL->GameplaySpaceshipRemoved(pDllPlot.get());
			gDLL->GameplaySpaceshipCreated(pDllPlot.get(), eUnderConstruction + eFrame);
		}
	}
	else if(GC.getProjectInfo(eProjectType)->IsSpaceship())
	{
		VictoryTypes eVictory = (VictoryTypes)GC.getProjectInfo(eProjectType)->GetVictoryPrereq();

		if(NO_VICTORY != eVictory && GET_TEAM(getTeam()).canLaunch(eVictory))
		{
			auto_ptr<ICvPlot1> pDllPlot(new CvDllPlot(plot()));
			gDLL->GameplaySpaceshipEdited(pDllPlot.get(), eConstructed);
			gDLL->sendLaunch(getOwner(), eVictory);
		}
		else
		{
			//show the spaceship progress

			// this section is kind of hard-coded but it is completely hard-coded on the engine-side so I have to give it the numbers it expects
			int spaceshipState = eFrame;

			if((thisTeam.getProjectCount((ProjectTypes)capsuleID)) == 1)
			{
				spaceshipState += eCapsule;
			}

			if((thisTeam.getProjectCount((ProjectTypes)stasisID)) == 1)
			{
				spaceshipState += eStasis_Chamber;
			}

			if((thisTeam.getProjectCount((ProjectTypes)engineID)) == 1)
			{
				spaceshipState += eEngine;
			}

			if((thisTeam.getProjectCount((ProjectTypes)boosterID)) >= 1)
			{
				spaceshipState += eBooster1;
			}

			if((thisTeam.getProjectCount((ProjectTypes)boosterID)) >= 2)
			{
				spaceshipState += eBooster2;
			}

			if((thisTeam.getProjectCount((ProjectTypes)boosterID)) == 3)
			{
				spaceshipState += eBooster3;
			}

			auto_ptr<ICvPlot1> pDllPlot(new CvDllPlot(plot()));
			gDLL->GameplaySpaceshipEdited(pDllPlot.get(), spaceshipState);
		}
	}
	CvProjectEntry* pProject = GC.getProjectInfo(eProjectType);
	if (pProject)
	{
		GET_PLAYER(getOwner()).GetTreasury()->ChangeBaseBuildingGoldMaintenance(pProject->GetGoldMaintenance()); // Maintenance cost
		
		PromotionTypes eFreePromotion = (PromotionTypes)pProject->GetFreePromotion();
		if (eFreePromotion != NO_PROMOTION) GET_PLAYER(getOwner()).ChangeFreePromotionCount(eFreePromotion, 1);
	}

	GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityProjectComplete, getOwner(), GetID(), eProjectType);

	return true;
}

void CvCity::changeProjectCount(ProjectTypes eProject, int iValue)
{
	VALIDATE_OBJECT
	CvAssertMsg(eProject >= 0, "ePlayer expected to be >= 0");
	CvAssertMsg(eProject < GC.getNumProjectInfos(), "ePlayer expected to be < NUM_DOMAIN_TYPES");
	m_aiNumProjects[eProject] = m_aiNumProjects[eProject] + iValue;

	YieldTypes eYield;
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		eYield = (YieldTypes)iI;
		CvProjectEntry* pProject = GC.getProjectInfo(eProject);
		if (pProject != NULL)
		{
			ChangeBaseYieldRateFromProjects(eYield, (pProject->GetYieldChange(eYield) * iValue));
			changeYieldRateModifier(eYield, (pProject->GetYieldModifier(eYield) * iValue));
		}
	}
}

//	--------------------------------------------------------------------------------
/// Base yield rate from Specialists
int CvCity::GetBaseYieldRateFromProjects(YieldTypes eIndex) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	return m_aiBaseYieldRateFromProjects[eIndex];
}

//	--------------------------------------------------------------------------------
/// Base yield rate from Specialists
void CvCity::ChangeBaseYieldRateFromProjects(YieldTypes eIndex, int iChange)
{
	VALIDATE_OBJECT
	CvAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiBaseYieldRateFromProjects.setAt(eIndex, m_aiBaseYieldRateFromProjects[eIndex] + iChange);

		if (getTeam() == GC.getGame().getActiveTeam())
		{
			if (isCitySelected())
			{
				DLLUI->setDirty(CityScreen_DIRTY_BIT, true);
			}
		}
	}
}




int CvCity::getProjectCount(ProjectTypes eProject) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eProject >= 0, "ePlayer expected to be >= 0");
	CvAssertMsg(eProject < GC.getNumProjectInfos(), "ePlayer expected to be < NUM_DOMAIN_TYPES");
	return m_aiNumProjects[eProject];
}

//	--------------------------------------------------------------------------------
bool CvCity::CanPlaceUnitHere(UnitTypes eUnitType)
{
	VALIDATE_OBJECT
	bool bCombat = false;

	CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnitType);
	if(pkUnitInfo == NULL)
		return false;

	// slewis - modifying 1upt
	if (pkUnitInfo->IsTrade())
	{
		return true;
	}

	if(pkUnitInfo->GetCombat() > 0 || pkUnitInfo->GetRange() > 0)
	{
		bCombat = true;
	}

#if defined(MOD_GLOBAL_STACKING_RULES)
	int iSameTypeUnits = 0;
#endif

	CvPlot* pPlot = plot();

	const IDInfo* pUnitNode;
	const CvUnit* pLoopUnit;

	pUnitNode = pPlot->headUnitNode();

	while(pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(*pUnitNode);
		pUnitNode = pPlot->nextUnitNode(pUnitNode);

		if(pLoopUnit != NULL)
		{
			// if a trade unit is here, ignore
			if (pLoopUnit->isTrade())
			{
				continue;
			}

			// Units of the same type OR Units belonging to different civs
			if(CvGameQueries::AreUnitsSameType(eUnitType, pLoopUnit->getUnitType()))
			{
#if defined(MOD_GLOBAL_STACKING_RULES)
				if (++iSameTypeUnits >= GC.getCITY_UNIT_LIMIT())
#endif
				return false;
			}
		}
	}

	return true;
}

//	--------------------------------------------------------------------------------
// Is this city allowed to purchase something right now?
bool CvCity::IsCanPurchase(bool bTestPurchaseCost, bool bTestTrainable, UnitTypes eUnitType, BuildingTypes eBuildingType, ProjectTypes eProjectType, YieldTypes ePurchaseYield)
{
	CvAssertMsg(eUnitType >= 0 || eBuildingType >= 0 || eProjectType >= 0, "No valid passed in");
	CvAssertMsg(!(eUnitType >= 0 && eBuildingType >= 0) && !(eUnitType >= 0 && eProjectType >= 0) && !(eBuildingType >= 0 && eProjectType >= 0), "Only one being passed");
	
	// Can't purchase anything in a puppeted city
	// slewis - The Venetian Exception
	CvPlayerAI& kPlayer = GET_PLAYER(m_eOwner);
	bool bIsPuppet = IsPuppet();
	bool bVenetianException = false;
	bool bAllowsPuppetPurchase = kPlayer.IsAllowPuppetPurchase();

	if (kPlayer.GetPlayerTraits()->IsNoAnnexing() && bIsPuppet)
	{
		bVenetianException = true;
	}

	if (bIsPuppet && !bAllowsPuppetPurchase)
	{
		if (eUnitType > NO_UNIT)
		{
			CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnitType);
			if (pkUnitInfo && pkUnitInfo->IsPuppetPurchaseOverride())
			{
				bAllowsPuppetPurchase = true;
			}
		}
		else if (eBuildingType > NO_BUILDING)
		{
			CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuildingType);
			if (pkBuildingInfo && pkBuildingInfo->IsPuppetPurchaseOverride())
			{
				bAllowsPuppetPurchase = true;
			}
		}
	}

	if (bIsPuppet && !bVenetianException && !bAllowsPuppetPurchase)
	{
		return false;
	}

	// Check situational reasons we can't purchase now (similar to not having enough gold or faith)
	if(bTestPurchaseCost)
	{
		// Can't purchase things if the city is in resistance or is being razed
		if(IsResistance() || IsRazing())
			return false;

		// if we're purchasing a unit
		if(eUnitType >= 0)
		{
			// if we can't add this unit to this tile, then don't!
			if(!CanPlaceUnitHere(eUnitType))
				return false;
		}
	}

	// What are we buying this with?
	switch(ePurchaseYield)
	{
	case YIELD_GOLD:
	{
		int iGoldCost = -1;

		// Unit
		if(eUnitType != NO_UNIT)
		{
			if(!canTrain(eUnitType, false, !bTestTrainable, false /*bIgnoreCost*/, true /*bWillPurchase*/))
				return false;

			iGoldCost = GetPurchaseCost(eUnitType);
		}
		// Building
		else if(eBuildingType != NO_BUILDING)
		{
#if defined(MOD_API_EXTENSIONS)
			if(!canConstruct(eBuildingType, false, !bTestTrainable, false /*bIgnoreCost*/, true /*bWillPurchase*/))
#else
			if(!canConstruct(eBuildingType, false, !bTestTrainable))
#endif
			{
				bool bAlreadyUnderConstruction = canConstruct(eBuildingType, true, !bTestTrainable) && getFirstBuildingOrder(eBuildingType) != -1;
				if(!bAlreadyUnderConstruction)
				{
					return false;
				}
			}

			iGoldCost = GetPurchaseCost(eBuildingType);
		}
		// Project
		else if(eProjectType != NO_PROJECT)
		{
			if(/*1*/ GC.getPROJECT_PURCHASING_DISABLED() == 1)
				return false;

			if(!canCreate(eProjectType, false, !bTestTrainable))
				return false;

			iGoldCost = GetPurchaseCost(eProjectType);
		}

		if(iGoldCost == -1)
		{
			return false;
		}
		else
		{
			if(bTestPurchaseCost)
			{
				// Trying to buy something when you don't have enough money!!
				if(iGoldCost > GET_PLAYER(getOwner()).GetTreasury()->GetGold())
					return false;
			}
		}
	}
	break;

	case YIELD_FAITH:
	{
		int iFaithCost = -1;

		// Does this city have a majority religion?
		ReligionTypes eReligion = GetCityReligions()->GetReligiousMajority();
#if defined(MOD_BUGFIX_MINOR)
		// Permit faith purchases from pantheon beliefs
		if(eReligion < RELIGION_PANTHEON)
#else
		if(eReligion <= RELIGION_PANTHEON)
#endif
		{
			return false;
		}

		// Unit
		if(eUnitType != NO_UNIT)
		{
			iFaithCost = GetFaithPurchaseCost(eUnitType, true);
			if(iFaithCost < 1)
			{
				return false;
			}

			CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnitType);
			if(pkUnitInfo)
			{
#if defined(MOD_BUGFIX_MINOR)
				if (pkUnitInfo->IsRequiresEnhancedReligion() && !(GC.getGame().GetGameReligions()->GetReligion(eReligion, m_eOwner)->m_bEnhanced))
#else
				if (pkUnitInfo->IsRequiresEnhancedReligion() && !(GC.getGame().GetGameReligions()->GetReligion(eReligion, NO_PLAYER)->m_bEnhanced))
#endif
				{
					return false;
				}

				
				if (pkUnitInfo->IsRequiresFaithPurchaseEnabled())
				{
					TechTypes ePrereqTech = (TechTypes)pkUnitInfo->GetPrereqAndTech();
					if (ePrereqTech == -1)
					{
						const CvReligion *pReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion, m_eOwner);
						if (!pReligion->m_Beliefs.IsFaithBuyingEnabled((EraTypes)0)) // Ed?
						{
							return false;
						}
						if(!canTrain(eUnitType, false, !bTestTrainable, false /*bIgnoreCost*/, true /*bWillPurchase*/))
						{
							return false;
						}
					}
					else
					{
						CvTechEntry *pkTechInfo = GC.GetGameTechs()->GetEntry(ePrereqTech);
						if (!pkTechInfo)
						{
							return false;
						}
						else
						{
							const CvReligion *pReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion, m_eOwner);
							if (!pReligion->m_Beliefs.IsFaithBuyingEnabled((EraTypes)pkTechInfo->GetEra()))
							{
								return false;
							}
							if(!canTrain(eUnitType, false, !bTestTrainable, false /*bIgnoreCost*/, true /*bWillPurchase*/))
							{
								return false;
							}
						}
					}
				}
#if defined(MOD_BUGFIX_MINOR)
				else
				{
					// Missionaries, Inquisitors and Prophets
					// We need a full religion and not just a pantheon,
					// and also to test that the player can build the unit, specifically the check for a civ specific version of the unit
					if(eReligion <= RELIGION_PANTHEON || !canTrain(eUnitType, false, !bTestTrainable, true /*bIgnoreCost*/, true /*bWillPurchase*/))
					{
						return false;
					}
				}
#endif
			}
		}
		// Building
		else if(eBuildingType != NO_BUILDING)
		{
			CvBuildingEntry* pkBuildingInfo = GC.GetGameBuildings()->GetEntry(eBuildingType);
 
			// Religion-enabled building
			if(pkBuildingInfo && pkBuildingInfo->IsUnlockedByBelief())
			{
				ReligionTypes eMajority = GetCityReligions()->GetReligiousMajority();
				if(eMajority <= RELIGION_PANTHEON)
				{
					return false;
				}
				const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eMajority, getOwner());
				if(pReligion == NULL || !pReligion->m_Beliefs.IsBuildingClassEnabled((BuildingClassTypes)pkBuildingInfo->GetBuildingClassType()))
				{
					return false;
				}
#if defined(MOD_BUGFIX_MINOR)
			}
#endif

#if defined(MOD_API_EXTENSIONS)
				if(!canConstruct(eBuildingType, false, !bTestTrainable, true /*bIgnoreCost*/, true /*bWillPurchase*/))
#else
				if(!canConstruct(eBuildingType, false, !bTestTrainable, true /*bIgnoreCost*/))
#endif
				{
					return false;
				}

				if(GetCityBuildings()->GetNumBuilding(eBuildingType) > 0)
				{
					return false;
				}

				TechTypes ePrereqTech = (TechTypes)pkBuildingInfo->GetPrereqAndTech();
				if(ePrereqTech != NO_TECH)
				{
					CvTechEntry *pkTechInfo = GC.GetGameTechs()->GetEntry(ePrereqTech);
					if (pkTechInfo && !GET_TEAM(GET_PLAYER(getOwner()).getTeam()).GetTeamTechs()->HasTech(ePrereqTech))
					{
						return false;
					}
				}

				// Does this city have prereq buildings?
				for(auto iBuildingClass : pkBuildingInfo->GetBuildingClassesNeededInCity())
				{
					if(GetNumBuildingClass((BuildingClassTypes)iBuildingClass) <= 0) return false;
				}
				for(auto iBuildingClass : pkBuildingInfo->GetBuildingClassesNeededGlobal())
				{
					if(kPlayer.getBuildingClassCount((BuildingClassTypes)iBuildingClass) <= 0) return false;
				}
				for(auto iBuilding : pkBuildingInfo->GetBuildingsNeededInCity())
				{
					if(GetCityBuildings()->GetNumBuilding((BuildingTypes)iBuilding) <= 0) return false;
				}
				for (auto iBuilding : pkBuildingInfo->GetBuildingsNeededGlobal())
				{
					if (kPlayer.countNumBuildings((BuildingTypes)iBuilding) <= 0) return false;
				}
#if !defined(MOD_BUGFIX_MINOR)
			}
#endif

			iFaithCost = GetFaithPurchaseCost(eBuildingType);
			if(iFaithCost < 1) return false;
		}

		if(iFaithCost > 0)
		{
			if(bTestPurchaseCost)
			{
				// Trying to buy something when you don't have enough faith!!
				if(iFaithCost > kPlayer.GetFaith())
				{
					return false;
				}
			}
		}
	}
	break;
	}

	return true;
}

#if defined(MOD_AI_SMART_V3)
bool CvCity::IsCanGoldPurchase(OrderData* pOrder)
{
	UnitTypes eUnitType = NO_UNIT;
	BuildingTypes eBuildingType = NO_BUILDING;
	ProjectTypes eProjectType = NO_PROJECT;

	switch(pOrder->eOrderType)
	{
		case ORDER_TRAIN:
			eUnitType = ((UnitTypes)(pOrder->iData1));
			break;

		case ORDER_CONSTRUCT:
			eBuildingType = ((BuildingTypes)(pOrder->iData1));
			break;

		case ORDER_CREATE:
			eProjectType = ((ProjectTypes)(pOrder->iData1));
			break;

		default:
			return false;
	}

	return IsCanPurchase(true, true, eUnitType, eBuildingType, eProjectType, YIELD_GOLD);
}

void CvCity::PurchaseCurrentOrder()
{
	UnitTypes eUnitType = NO_UNIT;
	BuildingTypes eBuildingType = NO_BUILDING;
	ProjectTypes eProjectType = NO_PROJECT;
	OrderData* pOrder = getOrderFromQueue(0);

	if (pOrder)
	{
		switch(pOrder->eOrderType)
		{
			case ORDER_TRAIN:
				eUnitType = ((UnitTypes)(pOrder->iData1));
				break;

			case ORDER_CONSTRUCT:
				eBuildingType = ((BuildingTypes)(pOrder->iData1));
				break;

			case ORDER_CREATE:
				eProjectType = ((ProjectTypes)(pOrder->iData1));
				break;

			default:
				return;
		}
	}

	Purchase(eUnitType, eBuildingType, eProjectType, YIELD_GOLD);
}
#endif

//	--------------------------------------------------------------------------------
// purchase something at the city
void CvCity::Purchase(UnitTypes eUnitType, BuildingTypes eBuildingType, ProjectTypes eProjectType, YieldTypes ePurchaseYield)
{
	VALIDATE_OBJECT

	CvPlayer& kPlayer = GET_PLAYER(getOwner());

	switch(ePurchaseYield)
	{
	case YIELD_GOLD:
	{
		// Can we actually buy this thing?
		if(!IsCanPurchase(/*bTestPurchaseCost*/ true, /*bTestTrainable*/ true, eUnitType, eBuildingType, eProjectType, YIELD_GOLD))
			return;

		int iGoldCost = 0;
		
		kPlayer.GetTreasury();

		// Unit
		if(eUnitType != NO_UNIT){
			iGoldCost = GetPurchaseCost(eUnitType);
			CvUnitEntry* pGameUnit = GC.getUnitInfo(eUnitType);
			if(pGameUnit != NULL)
			{
				kPlayer.GetTreasury()->LogExpenditure((CvString)pGameUnit->GetText(), iGoldCost, 2);
			}
		// Building
		}else if(eBuildingType != NO_BUILDING){
			iGoldCost = GetPurchaseCost(eBuildingType);
			CvBuildingEntry* pGameBuilding = GC.getBuildingInfo(eBuildingType);
			if(pGameBuilding != NULL)
			{
				kPlayer.GetTreasury()->LogExpenditure((CvString)pGameBuilding->GetText(), iGoldCost, 2);
			}
		// Project
		} else if(eProjectType != NO_PROJECT){
			iGoldCost = GetPurchaseCost(eProjectType);
			kPlayer.GetTreasury()->LogExpenditure((CvString)GC.getProjectInfo(eProjectType)->GetText(), iGoldCost, 2);
		}

		kPlayer.GetTreasury()->ChangeGold(-iGoldCost);

		bool bResult = false;
		if(eUnitType >= 0)
		{
			int iResult = CreateUnit(eUnitType, true, false);
			CvAssertMsg(iResult != FFreeList::INVALID_INDEX, "Unable to create unit");
			if (iResult != FFreeList::INVALID_INDEX)
			{
				CvUnit* pUnit = kPlayer.getUnit(iResult);
				if(pUnit->getUnitInfo().GetSpaceshipProject() != NO_PROJECT)
				{
					kPlayer.ChangeNumSpaceshipPartPurchased(1);
				}
				
				if (!pUnit->getUnitInfo().CanMoveAfterPurchase() && !kPlayer.IsPlayerMoveAfterCreated())
				{
					pUnit->setMoves(0);
				}

#if defined(MOD_EVENTS_CITY)
				if (MOD_EVENTS_CITY) {
					GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityTrained, getOwner(), GetID(), pUnit->GetID(), true, false);
				} else {
#endif
				ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
				if (pkScriptSystem) 
				{
					CvLuaArgsHandle args;
					args->Push(getOwner());
					args->Push(GetID());
					args->Push(pUnit->GetID());
					args->Push(true); // bGold
					args->Push(false); // bFaith/bCulture

					bool bScriptResult;
					LuaSupport::CallHook(pkScriptSystem, "CityTrained", args.get(), bScriptResult);
				}
#if defined(MOD_EVENTS_CITY)
			}
#endif
			}
		}
		else if(eBuildingType >= 0)
		{
			bResult = CreateBuilding(eBuildingType);

#if defined(MOD_EVENTS_CITY)
			if (MOD_EVENTS_CITY) {
				GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityConstructed, getOwner(), GetID(), eBuildingType, true, false);
			} else {
#endif
			ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
			if (pkScriptSystem) 
			{
				CvLuaArgsHandle args;
				args->Push(getOwner());
				args->Push(GetID());
				args->Push(eBuildingType);
				args->Push(true); // bGold
				args->Push(false); // bFaith/bCulture

				bool bScriptResult;
				LuaSupport::CallHook(pkScriptSystem, "CityConstructed", args.get(), bScriptResult);
			}
#if defined(MOD_EVENTS_CITY)
			}
#endif

			CleanUpQueue(); // cleans out items from the queue that may be invalidated by the recent construction
			CvAssertMsg(bResult, "Unable to create building");
		}
		else if(eProjectType >= 0)
		{
			bResult = CreateProject(eProjectType);
			CvAssertMsg(bResult, "Unable to create project");

#if defined(MOD_EVENTS_CITY)
			if (MOD_EVENTS_CITY) {
				GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityCreated, getOwner(), GetID(), eProjectType, true, false);
			} else {
#endif
			ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
			if (pkScriptSystem) 
			{
				CvLuaArgsHandle args;
				args->Push(getOwner());
				args->Push(GetID());
				args->Push(eProjectType);
				args->Push(true); // bGold
				args->Push(false); // bFaith/bCulture

				bool bScriptResult;
				LuaSupport::CallHook(pkScriptSystem, "CityCreated", args.get(), bScriptResult);
			}
#if defined(MOD_EVENTS_CITY)
			}
#endif
		}
	}
	break;
	case YIELD_FAITH:
	{
		int iFaithCost = 0;

		// Can we actually buy this thing?
		if(!IsCanPurchase(/*bTestPurchaseCost*/ true, /*bTestTrainable*/ true, eUnitType, eBuildingType, eProjectType, YIELD_FAITH))
			return;

		// Unit
		if(eUnitType != NO_UNIT)
			iFaithCost = GetFaithPurchaseCost(eUnitType, true  /*bIncludeBeliefDiscounts*/);
		// Building
		else if(eBuildingType != NO_BUILDING)
			iFaithCost = GetFaithPurchaseCost(eBuildingType);

		if(eUnitType >= 0)
		{
			int iResult = CreateUnit(eUnitType, false, true);
			CvAssertMsg(iResult != FFreeList::INVALID_INDEX, "Unable to create unit");
			if (iResult == FFreeList::INVALID_INDEX)
				return;	// Can't create the unit, most likely we have no place for it.  We have not deducted the cost yet so just exit.

			CvUnit* pUnit = kPlayer.getUnit(iResult);
#if defined(MOD_BUGFIX_MOVE_AFTER_PURCHASE)
			if (!pUnit->getUnitInfo().CanMoveAfterPurchase())
			{
#endif
				pUnit->setMoves(0);
#if defined(MOD_BUGFIX_MOVE_AFTER_PURCHASE)
			}
#endif

#if defined(MOD_EVENTS_CITY)
			if (MOD_EVENTS_CITY) {
				GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityTrained, getOwner(), GetID(), pUnit->GetID(), false, true);
			} else {
#endif
			ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
			if (pkScriptSystem) 
			{
				CvLuaArgsHandle args;
				args->Push(getOwner());
				args->Push(GetID());
				args->Push(pUnit->GetID());
				args->Push(false); // bGold
				args->Push(true); // bFaith/bCulture

				bool bResult;
				LuaSupport::CallHook(pkScriptSystem, "CityTrained", args.get(), bResult);
			}
#if defined(MOD_EVENTS_CITY)
			}
#endif

			// Prophets are always of the religion the player founded
			ReligionTypes eReligion;
			if(pUnit->getUnitInfo().IsFoundReligion())
			{
				eReligion = kPlayer.GetReligions()->GetReligionCreatedByPlayer();
			}
			else
			{
				eReligion = GetCityReligions()->GetReligiousMajority();
			}
			pUnit->GetReligionData()->SetReligion(eReligion);

			int iReligionSpreads = pUnit->getUnitInfo().GetReligionSpreads();
			int iReligiousStrength = pUnit->getUnitInfo().GetReligiousStrength();

			// Missionary strength
			if(iReligionSpreads > 0 && eReligion > RELIGION_PANTHEON)
			{
				// missionary spreads can be buffed but not prophets
				if (!pUnit->getUnitInfo().IsFoundReligion())
				{
					iReligionSpreads += GetCityBuildings()->GetMissionaryExtraSpreads();
#if defined(MOD_BELIEF_NEW_EFFECT_FOR_SP)
					if (MOD_BELIEF_NEW_EFFECT_FOR_SP)
					{
						iReligionSpreads += GetReligionExtraMissionarySpreads(eReligion);
						iReligionSpreads += GetBeliefExtraMissionarySpreads(GetCityReligions()->GetSecondaryReligionPantheonBelief());
					}
#endif
				}
				pUnit->GetReligionData()->SetSpreadsLeft(iReligionSpreads);
				pUnit->GetReligionData()->SetReligiousStrength(iReligiousStrength);
			}

			if (pUnit->getUnitInfo().GetOneShotTourism() > 0)
			{
				pUnit->SetTourismBlastStrength(kPlayer.GetCulture()->GetTourismBlastStrength(pUnit->getUnitInfo().GetOneShotTourism()));
			}

			kPlayer.ChangeFaith(-iFaithCost);

			UnitClassTypes eUnitClass = pUnit->getUnitClassType();
			if (pUnit->getUnitInfo().IsFaithCostIncrease())
			{
				kPlayer.incrementUnitClassesFromFaith(eUnitClass);
			}
			if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_GREAT_ADMIRAL"))
			{
				CvPlot *pSpawnPlot = kPlayer.GetGreatAdmiralSpawnPlot(pUnit);
				if (pUnit->plot() != pSpawnPlot)
				{
					pUnit->setXY(pSpawnPlot->getX(), pSpawnPlot->getY());
				}
			}
			else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_PROPHET"))
			{
				kPlayer.GetReligions()->ChangeNumProphetsSpawned(1, false);
			}
			if(GC.getLogging())
			{
				CvString strLogMsg;
				CvString temp;
				strLogMsg = kPlayer.getCivilizationShortDescription();
				strLogMsg += ", FAITH UNIT PURCHASE, ";
				strLogMsg += pUnit->getName();
				strLogMsg += ", ";
				strLogMsg += getName();
				strLogMsg += ", Faith Cost: ";
				temp.Format("%d", iFaithCost);
				strLogMsg += temp;
				strLogMsg += ", Faith Left: ";
				temp.Format("%d", kPlayer.GetFaith());
				strLogMsg += temp;
				GC.getGame().GetGameReligions()->LogReligionMessage(strLogMsg);
			}
		}

		else if(eBuildingType >= 0)
		{
			bool bResult = false;
			bResult = CreateBuilding(eBuildingType);
			CleanUpQueue(); // cleans out items from the queue that may be invalidated by the recent construction
			CvAssertMsg(bResult, "Unable to create building");

#if defined(MOD_EVENTS_CITY)
			if (MOD_EVENTS_CITY) {
				GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityConstructed, getOwner(), GetID(), eBuildingType, false, true);
			} else {
#endif
			ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
			if (pkScriptSystem)
			{
				CvLuaArgsHandle args;
				args->Push(getOwner());
				args->Push(GetID());
				args->Push(eBuildingType);
				args->Push(false); // bGold
				args->Push(true); // bFaith/bCulture

				bool bScriptResult;
				LuaSupport::CallHook(pkScriptSystem, "CityConstructed", args.get(), bScriptResult);
			}
#if defined(MOD_EVENTS_CITY)
			}
#endif

			kPlayer.ChangeFaith(-iFaithCost);

			if(GC.getLogging())
			{
				CvString strLogMsg;
				CvString temp;
				strLogMsg = kPlayer.getCivilizationShortDescription();
				strLogMsg += ", FAITH BUILDING PURCHASE, ";

				CvBuildingXMLEntries* pGameBuildings = GC.GetGameBuildings();
				if(pGameBuildings != NULL)
				{
					CvBuildingEntry* pBuildingEntry = pGameBuildings->GetEntry(eBuildingType);
					if(pBuildingEntry != NULL)
					{
						strLogMsg += pBuildingEntry->GetDescription();
						strLogMsg += ", ";
					}
				}
				strLogMsg += getName();
				strLogMsg += ", Faith Cost: ";
				temp.Format("%d", iFaithCost);
				strLogMsg += temp;
				strLogMsg += ", Faith Left: ";
				temp.Format("%d", kPlayer.GetFaith());
				strLogMsg += temp;
				GC.getGame().GetGameReligions()->LogReligionMessage(strLogMsg);
			}
		}
	}
	break;
	}
}


// Protected Functions...

//	--------------------------------------------------------------------------------
void CvCity::doGrowth()
{
	VALIDATE_OBJECT
	AI_PERF_FORMAT("City-AI-perf.csv", ("CvCity::doGrowth, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );
	// here would be a good place to override this in Lua

	// No growth or starvation if being razed
	if(IsRazing())
	{
		return;
	}

	int iDiff = foodDifferenceTimes100();

	if(iDiff < 0)
	{
		CvNotifications* pNotifications = GET_PLAYER(getOwner()).GetNotifications();
		if(pNotifications)
		{
			Localization::String text = Localization::Lookup("TXT_KEY_NTFN_CITY_STARVING");
			text << getNameKey();
			Localization::String summary = Localization::Lookup("TXT_KEY_NTFN_CITY_STARVING_S");
			summary << getNameKey();

			pNotifications->Add(NOTIFICATION_STARVING, text.toUTF8(), summary.toUTF8(), getX(), getY(), -1);
		}
	}

	changeFoodTimes100(iDiff);
	changeFoodKept(iDiff/100);

	int iGrowthThreshold = growthThreshold();
	setFoodKept(range(getFoodKept(), 0, ((iGrowthThreshold * getMaxFoodKeptPercent()) / 100)));

	int iLoopTime = 1;
#ifdef MOD_GLOBAL_UNLIMITED_ONE_TURN_GROWTH
	if (MOD_GLOBAL_UNLIMITED_ONE_TURN_GROWTH) iLoopTime = 1000;
#endif
	while (getFood() >= iGrowthThreshold && iLoopTime--)
	{
		if (GetCityCitizens()->IsForcedAvoidGrowth())  // don't grow a city if we are at avoid growth
		{
			setFood(iGrowthThreshold);
			break;
		}

		changeFood(-(std::max(0, (iGrowthThreshold - getFoodKept()))));
		changePopulation(1);
		iGrowthThreshold = growthThreshold();

		// Only show notification if the city is small
		if(getPopulation() <= GC.getMAX_POPULATION_INCREASE_NOTIOFACATION())
		{
			CvNotifications* pNotifications = GET_PLAYER(getOwner()).GetNotifications();
			if(pNotifications)
			{
				Localization::String localizedText = Localization::Lookup("TXT_KEY_NOTIFICATION_CITY_GROWTH");
				localizedText << getNameKey() << getPopulation();
				Localization::String localizedSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_CITY_GROWTH");
				localizedSummary << getNameKey();
				pNotifications->Add(NOTIFICATION_CITY_GROWTH, localizedText.toUTF8(), localizedSummary.toUTF8(), getX(), getY(), GetID());
			}
		}
	}

	if (getFood() < 0)
	{
		changeFood(-(getFood()));

		if(getPopulation() > 1)
		{
			changePopulation(-1);
		}
	}
}

//	--------------------------------------------------------------------------------
bool CvCity::doCheckProduction()
{
	AI_PERF_FORMAT("City-AI-perf.csv", ("CvCity::doCheckProduction, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );
	VALIDATE_OBJECT
	OrderData* pOrderNode;
	UnitTypes eUpgradeUnit;
	int iUpgradeProduction;
	int iProductionGold;
	int iI;
	bool bOK = true;

	int iMaxedUnitGoldPercent = GC.getMAXED_UNIT_GOLD_PERCENT();
	int iMaxedBuildingGoldPercent = GC.getMAXED_BUILDING_GOLD_PERCENT();
	int iMaxedProjectGoldPercent = GC.getMAXED_PROJECT_GOLD_PERCENT();

	CvPlayerAI& thisPlayer = GET_PLAYER(getOwner());

	int iNumUnitInfos = GC.getNumUnitInfos();
	{
		AI_PERF_FORMAT_NESTED("City-AI-perf.csv", ("CvCity::doCheckProduction_Unit, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );
		for(iI = 0; iI < iNumUnitInfos; iI++)
		{
			const UnitTypes eUnit = static_cast<UnitTypes>(iI);
			CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);
			if(pkUnitInfo)
			{
				int iUnitProduction = getUnitProduction(eUnit);
				if(iUnitProduction > 0)
				{
					if(thisPlayer.isProductionMaxedUnitClass((UnitClassTypes)(pkUnitInfo)->GetUnitClassType()))
					{
						iProductionGold = ((iUnitProduction * iMaxedUnitGoldPercent) / 100);

						if(iProductionGold > 0)
						{
							thisPlayer.GetTreasury()->ChangeGold(iProductionGold);

							if(getOwner() == GC.getGame().getActivePlayer())
							{
								Localization::String localizedText = Localization::Lookup("TXT_KEY_MISC_LOST_WONDER_PROD_CONVERTED");
								localizedText << getNameKey() << GC.getUnitInfo((UnitTypes)iI)->GetTextKey() << iProductionGold;
								DLLUI->AddCityMessage(0, GetIDInfo(), getOwner(), false, GC.getEVENT_MESSAGE_TIME(), localizedText.toUTF8());
							}
						}

						setUnitProduction(((UnitTypes)iI), 0);
					}
				}
			}
		}
	}

	int iNumBuildingInfos = GC.getNumBuildingInfos();
	{
		AI_PERF_FORMAT_NESTED("City-AI-perf.csv", ("CvCity::doCheckProduction_Building, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );

		int iPlayerLoop;
		PlayerTypes eLoopPlayer;

		for(iI = 0; iI < iNumBuildingInfos; iI++)
		{
			const BuildingTypes eExpiredBuilding = static_cast<BuildingTypes>(iI);
			CvBuildingEntry* pkExpiredBuildingInfo = GC.getBuildingInfo(eExpiredBuilding);

			//skip if null
			if(pkExpiredBuildingInfo == NULL)
				continue;

			int iBuildingProduction = m_pCityBuildings->GetBuildingProduction(eExpiredBuilding);
			if(iBuildingProduction > 0)
			{
				const BuildingClassTypes eExpiredBuildingClass = (BuildingClassTypes)(pkExpiredBuildingInfo->GetBuildingClassType());

				if(thisPlayer.isProductionMaxedBuildingClass(eExpiredBuildingClass))
				{
					// Beaten to a world wonder by someone?
					if(isWorldWonderClass(pkExpiredBuildingInfo->GetBuildingClassInfo()))
					{
						for(iPlayerLoop = 0; iPlayerLoop < MAX_MAJOR_CIVS; iPlayerLoop++)
						{
							eLoopPlayer = (PlayerTypes) iPlayerLoop;

							// Found the culprit
							if(GET_PLAYER(eLoopPlayer).getBuildingClassCount(eExpiredBuildingClass) > 0)
							{
								GET_PLAYER(getOwner()).GetDiplomacyAI()->ChangeNumWondersBeatenTo(eLoopPlayer, 1);
								break;
							}
						}

						auto_ptr<ICvCity1> pDllCity(new CvDllCity(this));
						DLLUI->AddDeferredWonderCommand(WONDER_REMOVED, pDllCity.get(), (BuildingTypes) eExpiredBuilding, 0);
#if !defined(NO_ACHIEVEMENTS)
						//Add "achievement" for sucking it up
						gDLL->IncrementSteamStatAndUnlock(ESTEAMSTAT_BEATWONDERS, 10, ACHIEVEMENT_SUCK_AT_WONDERS);
#endif
					}

					iProductionGold = ((iBuildingProduction * iMaxedBuildingGoldPercent) / 100);

					if(iProductionGold > 0)
					{
						thisPlayer.GetTreasury()->ChangeGold(iProductionGold);

						if(getOwner() == GC.getGame().getActivePlayer())
						{
							Localization::String localizedText = Localization::Lookup("TXT_KEY_MISC_LOST_WONDER_PROD_CONVERTED");
							localizedText << getNameKey() << pkExpiredBuildingInfo->GetTextKey() << iProductionGold;
							DLLUI->AddCityMessage(0, GetIDInfo(), getOwner(), false, GC.getEVENT_MESSAGE_TIME(), localizedText.toUTF8());
						}
					}

					m_pCityBuildings->SetBuildingProduction(eExpiredBuilding, 0);
				}
			}
		}
	}

	{
		AI_PERF_FORMAT_NESTED("City-AI-perf.csv", ("CvCity::doCheckProduction_Project, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );
		int iNumProjectInfos = GC.getNumProjectInfos();
		for(iI = 0; iI < iNumProjectInfos; iI++)
		{
			int iProjectProduction = getProjectProduction((ProjectTypes)iI);
			if(iProjectProduction > 0)
			{
				if(thisPlayer.isProductionMaxedProject((ProjectTypes)iI))
				{
					iProductionGold = ((iProjectProduction * iMaxedProjectGoldPercent) / 100);

					if(iProductionGold > 0)
					{
						thisPlayer.GetTreasury()->ChangeGold(iProductionGold);

						if(getOwner() == GC.getGame().getActivePlayer())
						{
							Localization::String localizedText = Localization::Lookup("TXT_KEY_MISC_LOST_WONDER_PROD_CONVERTED");
							localizedText << getNameKey() << GC.getProjectInfo((ProjectTypes)iI)->GetTextKey() << iProductionGold;
							DLLUI->AddCityMessage(0, GetIDInfo(), getOwner(), false, GC.getEVENT_MESSAGE_TIME(), localizedText.toUTF8());
						}
					}

					setProjectProduction(((ProjectTypes)iI), 0);
				}
			}
		}
	}

	if(!isProduction() && isHuman() && !isProductionAutomated() && !IsIgnoreCityForHappiness())
	{
		chooseProduction();
		return bOK;
	}

	{
		AI_PERF_FORMAT_NESTED("City-AI-perf.csv", ("CvCity::doCheckProduction_UpgradeUnit, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );
		// Can now construct an Upgraded version of this Unit
		for(iI = 0; iI < iNumUnitInfos; iI++)
		{
			if(getFirstUnitOrder((UnitTypes)iI) != -1)
			{
				// If we can still actually train this Unit type then don't auto-upgrade it yet
				if(canTrain((UnitTypes)iI, true))
				{
					continue;
				}

				eUpgradeUnit = allUpgradesAvailable((UnitTypes)iI);

				if(eUpgradeUnit != NO_UNIT)
				{
					CvAssertMsg(eUpgradeUnit != iI, "Trying to upgrade a Unit to itself");
					iUpgradeProduction = getUnitProduction((UnitTypes)iI);
					setUnitProduction(((UnitTypes)iI), 0);
					setUnitProduction(eUpgradeUnit, iUpgradeProduction);

					pOrderNode = headOrderQueueNode();

					while(pOrderNode != NULL)
					{
						if(pOrderNode->eOrderType == ORDER_TRAIN)
						{
							if(pOrderNode->iData1 == iI)
							{
								thisPlayer.changeUnitClassMaking(((UnitClassTypes)(GC.getUnitInfo((UnitTypes)(pOrderNode->iData1))->GetUnitClassType())), -1);
								pOrderNode->iData1 = eUpgradeUnit;
								thisPlayer.changeUnitClassMaking(((UnitClassTypes)(GC.getUnitInfo((UnitTypes)(pOrderNode->iData1))->GetUnitClassType())), 1);
							}
						}

						pOrderNode = nextOrderQueueNode(pOrderNode);
					}
				}
			}
		}
	}

	{
		AI_PERF_FORMAT_NESTED("City-AI-perf.csv", ("CvCity::doCheckProduction_UpgradeBuilding, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );
		// Can now construct an Upgraded version of this Building
		for (OrderData* pOrderNode = headOrderQueueNode(); pOrderNode != NULL; pOrderNode = nextOrderQueueNode(pOrderNode))
		{
			if(pOrderNode->eOrderType != ORDER_CONSTRUCT) continue;

			const BuildingTypes eBuilding = (BuildingTypes)pOrderNode->iData1;
			CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
			if(!pkBuildingInfo) continue;

			BuildingClassTypes eBuildingClass = (BuildingClassTypes) pkBuildingInfo->GetReplacementBuildingClass();
			if(eBuildingClass == NO_BUILDINGCLASS) continue;
			BuildingTypes eUpgradeBuilding = thisPlayer.GetCivBuilding(eBuildingClass);

			if(!canConstruct(eUpgradeBuilding)) continue;
			CvAssertMsg(eUpgradeBuilding != eBuilding, "Trying to upgrade a Building to itself");
			iUpgradeProduction = m_pCityBuildings->GetBuildingProduction(eBuilding);
			m_pCityBuildings->SetBuildingProduction((eBuilding), 0);
			m_pCityBuildings->SetBuildingProduction(eUpgradeBuilding, iUpgradeProduction);

			CvBuildingEntry* pkUpgradeBuildingInfo = GC.getBuildingInfo(eUpgradeBuilding);
			if(NULL != pkUpgradeBuildingInfo)
			{
				const BuildingClassTypes eOrderBuildingClass = (BuildingClassTypes)pkBuildingInfo->GetBuildingClassType();
				const BuildingClassTypes eUpgradeBuildingClass = (BuildingClassTypes)pkUpgradeBuildingInfo->GetBuildingClassType();

				thisPlayer.changeBuildingClassMaking(eOrderBuildingClass, -1);
				pOrderNode->iData1 = eUpgradeBuilding;
				thisPlayer.changeBuildingClassMaking(eUpgradeBuildingClass, 1);
			}
		}
	}

	{
		AI_PERF_FORMAT_NESTED("City-AI-perf.csv", ("CvCity::doCheckProduction_CleanupQueue, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );
		bOK = CleanUpQueue();
	}

	return bOK;
}


//	--------------------------------------------------------------------------------
void CvCity::doProduction(bool bAllowNoProduction)
{
	VALIDATE_OBJECT
	AI_PERF_FORMAT("City-AI-perf.csv", ("CvCity::doProduction, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );

	if(!isHuman() || isProductionAutomated())
	{
		if(!isProduction() || isProductionProcess() || AI_isChooseProductionDirty())
		{
			AI_chooseProduction(false /*bInterruptWonders*/);
		}
	}

	if(!bAllowNoProduction && !isProduction())
	{
		return;
	}

	if (!isProduction())
	{
		changeOverflowProductionTimes100(getCurrentProductionDifferenceTimes100(false, false));
		return;
	}

	int iMaxProductionCount = 1;
	iMaxProductionCount += GET_PLAYER(getOwner()).getPolicyModifiers(POLICYMOD_CITY_EXTRA_PRODUCTION_COUNT);
#ifdef MOD_GLOBAL_UNLIMITED_ONE_TURN_PRODUCTION
	if (MOD_GLOBAL_UNLIMITED_ONE_TURN_PRODUCTION) iMaxProductionCount += 4;
#endif

	for (int iProductionCount = 0; iProductionCount < iMaxProductionCount && isProduction(); iProductionCount++)
	{
		if (isProductionBuilding())
		{
			const OrderData *pOrderNode = headOrderQueueNode();
			int iData1 = -1;
			if (pOrderNode != NULL)
			{
				iData1 = pOrderNode->iData1;
			}

			const BuildingTypes eBuilding = static_cast<BuildingTypes>(iData1);
			CvBuildingEntry *pkBuildingInfo = GC.getBuildingInfo(eBuilding);
			if (pkBuildingInfo)
			{
				if (isWorldWonderClass(pkBuildingInfo->GetBuildingClassInfo()))
				{
					if (m_pCityBuildings->GetBuildingProduction(eBuilding) == 0) // otherwise we are probably already showing this
					{
						auto_ptr<ICvCity1> pDllCity(new CvDllCity(this));
						DLLUI->AddDeferredWonderCommand(WONDER_CREATED, pDllCity.get(), eBuilding, 0);
					}
				}
			}
		}

		// notice: To avoid product duplicated, we only count the difference production once.
		changeProductionTimes100(iProductionCount == 0 ? getCurrentProductionDifferenceTimes100(false, true) : getOverflowProductionTimes100());

#if defined(MOD_PROCESS_STOCKPILE)
		if (!(MOD_PROCESS_STOCKPILE && isProductionProcess()))
#endif
			setOverflowProduction(0);
		setFeatureProduction(0);

#if defined(MOD_PROCESS_STOCKPILE)
		if (getProduction() >= getProductionNeeded())
#else
		if (getProduction() >= getProductionNeeded() && !isProductionProcess())
#endif
		{
#if defined(MOD_PROCESS_STOCKPILE)
			popOrder(0, !isProductionProcess(), true);
			if (!isHuman() || isProductionAutomated())
			{
				AI_chooseProduction(false /*bInterruptWonders*/); // the previous order is finished. choose next one.
			}
#else
			popOrder(0, true, true);
#endif
		}
		else
		{
			break;
		}
	}
}


//	--------------------------------------------------------------------------------
void CvCity::doProcess()
{
	ProcessTypes eProcess = getProductionProcess();
	CvAssertMsg(eProcess != NO_PROCESS, "Invalid Process for city production. Please send Anton your save file and version.");
	if (eProcess == NO_PROCESS) return;

	// Contribute production to a League project
	for(int iI = 0; iI < GC.getNumLeagueProjectInfos(); iI++)
	{
		LeagueProjectTypes eLeagueProject = (LeagueProjectTypes) iI;
		CvLeagueProjectEntry* pInfo = GC.getLeagueProjectInfo(eLeagueProject);
		if (pInfo)
		{
			if (pInfo->GetProcess() == eProcess)
			{
				GC.getGame().GetGameLeagues()->DoLeagueProjectContribution(getOwner(), eLeagueProject, getCurrentProductionDifferenceTimes100(false, true));
			}
		}
	}
	
#if defined(MOD_PROCESS_STOCKPILE)
	if (MOD_PROCESS_STOCKPILE && eProcess == GC.getInfoTypeForString("PROCESS_STOCKPILE"))
	{
		int iPile = getCurrentProductionDifferenceTimes100(false, false);
		// Can't use changeOverflowProductionTimes100() here as it asserts above 250 production
		setOverflowProductionTimes100(getOverflowProductionTimes100() + iPile);
		// CUSTOMLOG("Adding %i production to the stockpile of %s (for a total of %i)", iPile/100, getName().c_str(), getOverflowProduction());
	}
#endif
}


//	--------------------------------------------------------------------------------
void CvCity::doDecay()
{
#if defined(MOD_CITY_NO_DECAY)
	if(MOD_CITY_NO_DECAY) return;
#endif
	VALIDATE_OBJECT
	if(!isHuman()) return;
	AI_PERF_FORMAT("City-AI-perf.csv", ("CvCity::doDecay, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );
	int iI;

	int iBuildingProductionDecayTime = GC.getBUILDING_PRODUCTION_DECAY_TIME();
	int iBuildingProductionDecayPercent = GC.getBUILDING_PRODUCTION_DECAY_PERCENT();

	int iNumBuildingInfos = GC.getNumBuildingInfos();
	for(iI = 0; iI < iNumBuildingInfos; iI++)
	{
		if(getProductionBuilding() != ((BuildingTypes)iI))
		{
			if(m_pCityBuildings->GetBuildingProduction((BuildingTypes)iI) > 0)
			{
				m_pCityBuildings->ChangeBuildingProductionTime(((BuildingTypes)iI), 1);
				if(m_pCityBuildings->GetBuildingProductionTime((BuildingTypes)iI) > iBuildingProductionDecayTime)
				{
					m_pCityBuildings->SetBuildingProduction(((BuildingTypes)iI), ((m_pCityBuildings->GetBuildingProduction((BuildingTypes)iI) * iBuildingProductionDecayPercent) / 100));
				}
			}
			else
			{
				m_pCityBuildings->SetBuildingProductionTime(((BuildingTypes)iI), 0);
			}
		}
	}

	int iUnitProductionDecayTime = GC.getUNIT_PRODUCTION_DECAY_TIME();
	int iUnitProductionDecayPercent = GC.getUNIT_PRODUCTION_DECAY_PERCENT();

	int iNumUnitInfos = GC.getNumUnitInfos();
	for(iI = 0; iI < iNumUnitInfos; iI++)
	{
		const UnitTypes eUnit = static_cast<UnitTypes>(iI);
		CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);
		if(pkUnitInfo)
		{
			if(getProductionUnit() != eUnit)
			{
				if(getUnitProduction(eUnit) > 0)
				{
					changeUnitProductionTime(eUnit, 1);
					if(getUnitProductionTime(eUnit) > iUnitProductionDecayTime)
					{
						setUnitProduction(eUnit, ((getUnitProduction(eUnit) * iUnitProductionDecayPercent) / 100));
					}
				}
				else
				{
					setUnitProductionTime(eUnit, 0);
				}
			}
		}
	}
}


//	--------------------------------------------------------------------------------
CvCityStrategyAI* CvCity::GetCityStrategyAI() const
{
	VALIDATE_OBJECT
	return m_pCityStrategyAI;
}

//	--------------------------------------------------------------------------------
CvCityCitizens* CvCity::GetCityCitizens() const
{
	VALIDATE_OBJECT
	return m_pCityCitizens;
}

//	--------------------------------------------------------------------------------
CvCityReligions* CvCity::GetCityReligions() const
{
	VALIDATE_OBJECT
	return m_pCityReligions;
}

//	--------------------------------------------------------------------------------
CvCityEmphases* CvCity::GetCityEmphases() const
{
	VALIDATE_OBJECT
	return m_pEmphases;
}

//	--------------------------------------------------------------------------------
CvCityEspionage* CvCity::GetCityEspionage() const
{
	VALIDATE_OBJECT
	return m_pCityEspionage;
}

//	--------------------------------------------------------------------------------
CvCityCulture* CvCity::GetCityCulture() const
{
	VALIDATE_OBJECT
		return m_pCityCulture;
}

// Private Functions...

//	--------------------------------------------------------------------------------
void CvCity::read(FDataStream& kStream)
{
	VALIDATE_OBJECT
	// Init data before load
	reset();

	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;
	MOD_SERIALIZE_INIT_READ(kStream);

	kStream >> m_iID;
	kStream >> m_iX;
	kStream >> m_iY;
	kStream >> m_iRallyX;
	kStream >> m_iRallyY;
	kStream >> m_iGameTurnFounded;
	kStream >> m_iGameTurnAcquired;
	kStream >> m_iPopulation;
	kStream >> m_iHighestPopulation;
#if defined(MOD_GLOBAL_CITY_AUTOMATON_WORKERS)
	MOD_SERIALIZE_READ(89, kStream, m_iAutomatons, 0);
#endif
#if defined(MOD_BUILDINGS_CITY_AUTOMATON_WORKERS)
	MOD_SERIALIZE_READ(90, kStream, m_iCityAutomatonWorkersChange, 0);
#endif
	kStream >> m_iNumGreatPeople;
	kStream >> m_iBaseGreatPeopleRate;
	kStream >> m_iGreatPeopleRateModifier;
	kStream >> m_iJONSCultureStored;
	kStream >> m_iJONSCultureLevel;
	kStream >> m_iJONSCulturePerTurnFromBuildings;
	kStream >> m_iJONSCulturePerTurnFromPolicies;
	kStream >> m_iJONSCulturePerTurnFromSpecialists;
	kStream >> m_iCultureRateModifier;
	kStream >> m_iNumWorldWonders;
	kStream >> m_iNumTeamWonders;
	kStream >> m_iNumNationalWonders;
	kStream >> m_iWonderProductionModifier;
	kStream >> m_iCapturePlunderModifier;
	kStream >> m_iPlotCultureCostModifier;
	kStream >> m_iPlotBuyCostModifier;
	kStream >> m_iUnitMaxExperienceLocal;
	kStream >> m_iTradeRouteRiverBonusModifier;
	kStream >> m_iSecondCapitalsExtraScore;
	kStream >> m_iFoodKeptFromPollution;
	kStream >> m_iNumAllowsFoodTradeRoutes;
	kStream >> m_iNumAllowsProductionTradeRoutes;
#if defined(MOD_BUILDINGS_CITY_WORKING)
	MOD_SERIALIZE_READ(23, kStream, m_iCityWorkingChange, 0);
#endif
#if defined(MOD_BUILDINGS_CITY_AUTOMATON_WORKERS)
	// SEE ABOVE - MOD_SERIALIZE_READ(90, kStream, m_iCityAutomatonWorkersChange, 0);
#endif
#if defined(MOD_VOLCANO_BREAK)
	kStream >> m_iImmueVolcanoDamage;
#endif
	kStream >> m_iAddsFreshWater;
	kStream >> m_iExtraAttackOnKill;
	kStream >> m_iForbiddenForeignSpyCount;
	kStream >> m_iAllowSpaceshipLaunchCount;
#ifdef MOD_ROG_CORE
	kStream >> m_iCityBuildingRangeStrikeModifier;
	kStream >> m_iExtraDamageHealPercent;
	kStream >> m_iExtraDamageHeal;
	kStream >> m_iBombardRange;
	kStream >> m_iBombardIndirect;
	kStream >> m_iNumAttacks;
	kStream >> m_iAttacksMade;
	kStream >> m_iNukeInterceptionChance;
	
	kStream >> m_iResetDamageValue;
	kStream >> m_iReduceDamageValue;


	kStream >> m_iWaterTileDamage;
	kStream >> m_iWaterTileMovementReduce;
	kStream >> m_iWaterTileTurnDamage;
	kStream >> m_iLandTileDamage;
	kStream >> m_iLandTileMovementReduce;
	kStream >> m_iLandTileTurnDamage;
#endif


	kStream >> m_iMaintenance;
	kStream >> m_iHealRate;
	kStream >> m_iEspionageModifier;
#if defined(MOD_RELIGION_CONVERSION_MODIFIERS)
	MOD_SERIALIZE_READ(23, kStream, m_iConversionModifier, 0);
#endif
	kStream >> m_iNoOccupiedUnhappinessCount;
	kStream >> m_iFood;
	kStream >> m_iFoodKept;
	kStream >> m_iMaxFoodKeptPercent;
	kStream >> m_iOverflowProduction;
	kStream >> m_iFeatureProduction;
	kStream >> m_iMilitaryProductionModifier;
	kStream >> m_iSpaceProductionModifier;
	kStream >> m_iFreeExperience;
	kStream >> m_iNumCanAirlift;
	kStream >> m_iCurrAirlift; // unused

	if (uiVersion >= 6)
	{
		kStream >> m_iMaxAirUnits; 
	}
	else
	{
		kStream >> m_iMaxAirUnits;

		// Forcibly override this since we didn't track this number before
		m_iMaxAirUnits = GC.getBASE_CITY_AIR_STACKING();

		// Note that this can get boosted further below once we know which buildings we have
	}
	kStream >> m_iAirModifier; // unused
	kStream >> m_iNukeModifier;

	if (uiVersion >= 2)
	{
		kStream >> m_iTradeRouteTargetBonus;
		kStream >> m_iTradeRouteRecipientBonus;
	}
	else
	{
		m_iTradeRouteTargetBonus = 0;
		m_iTradeRouteRecipientBonus = 0;
	}

	kStream >> m_iCultureUpdateTimer;
	kStream >> m_iCitySizeBoost;
	kStream >> m_iSpecialistFreeExperience;
	kStream >> m_iStrengthValue;
	kStream >> m_iDamage;
	kStream >> m_iThreatValue;
	kStream >> m_iGarrisonedUnit;
	m_iResourceDemanded = CvInfosSerializationHelper::ReadHashed(kStream);
	kStream >> m_iWeLoveTheKingDayCounter;
	kStream >> m_iLastTurnGarrisonAssigned;
	kStream >> m_iThingsProduced;
	kStream >> m_iDemandResourceCounter;
	kStream >> m_iResistanceTurns;
	kStream >> m_iRazingTurns;
	kStream >> m_iCountExtraLuxuries;
	kStream >> m_iCheapestPlotInfluence;
	kStream >> m_unitBeingBuiltForOperation.m_iOperationID;
	kStream >> m_unitBeingBuiltForOperation.m_iArmyID;
	kStream >> m_unitBeingBuiltForOperation.m_iSlotID;

	kStream >> m_bNeverLost;
	kStream >> m_bDrafted;
	kStream >> m_bAirliftTargeted;  // unused
	kStream >> m_bProductionAutomated;
	kStream >> m_bMadeAttack;
	kStream >> m_bOccupied;
	kStream >> m_bPuppet;
	kStream >> m_bEverCapital;
	kStream >> m_bIndustrialRouteToCapital;
	kStream >> m_bFeatureSurrounded;

	kStream >> m_eOwner;
	kStream >> m_ePreviousOwner;
	kStream >> m_eOriginalOwner;
	kStream >> m_ePlayersReligion;

	kStream >> m_aiSeaPlotYield;
	kStream >> m_aiRiverPlotYield;
	kStream >> m_aiLakePlotYield;
	kStream >> m_aiSeaResourceYield;
	kStream >> m_aiBaseYieldRateFromTerrain;
	kStream >> m_aiBaseYieldRateFromBuildings;
	kStream >> m_aiBaseYieldRateFromBuildingsPolicies;
	kStream >> m_aiBaseYieldRateFromSpecialists;
	kStream >> m_aiBaseYieldRateFromProjects;
	kStream >> m_aiBaseYieldRateFromMisc;
	kStream >> m_aiBaseYieldRateFromReligion;
	kStream >> m_aiYieldPerPop;
	kStream >> m_aiYieldPerPopInEmpire;
	kStream >> m_aiYieldFromInternalTR;
	kStream >> m_aiYieldFromProcessModifier;
	kStream >> m_aiNumProjects;
#if defined(MOD_ROG_CORE)
	kStream >> m_aiYieldPerAlly;
	kStream >> m_aiYieldPerFriend;
	kStream >> m_aiNumTimesAttackedThisTurn;
	kStream >> m_aiBaseYieldRateFromCSAlliance;
	kStream >> m_aiBaseYieldRateFromCSFriendship;
	kStream >> m_aiSpecialistRateModifier;
	kStream >> m_aiYieldPerEspionageSpy;
	kStream >> m_aiBaseYieldRateFromEspionageSpy;
	kStream >> m_aiYieldFromConstruction;
	kStream >> m_aiYieldFromUnitProduction;
	kStream >> m_aiYieldFromBirth;
	kStream >> m_aiYieldFromBorderGrowth;
	kStream >> m_aiYieldFromPillage;
	kStream >> m_aiResourceQuantityFromPOP;
#endif

	if (uiVersion >= 4)
	{
		kStream >> m_aiYieldPerReligion;
	}
	else
	{
		for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			m_aiYieldPerReligion[iI] = 0;
		}
	}
	kStream >> m_aiYieldPerEra;
	kStream >> m_aiYieldModifierPerEra;
	kStream >> m_aiCityStateTradeRouteYieldModifier;
	kStream >> m_aiYieldRateModifier;
	kStream >> m_aiYieldRateMultiplier;
	kStream >> m_aiPowerYieldRateModifier;
	kStream >> m_aiFeatureYieldRateModifier;
	kStream >> m_aiTerrainYieldRateModifier;
	kStream >> m_aiImprovementYieldRateModifier;
	kStream >> m_aiSpecialistYieldRateModifier;
	kStream >> m_aiResourceYieldRateModifier;
	kStream >> m_aiExtraSpecialistYield;
	kStream >> m_aiProductionToYieldModifier;
	kStream >> m_aiDomainFreeExperience;
	kStream >> m_aiDomainProductionModifier;
	kStream >> m_aiDomainFreeExperiencesPerPop;
	kStream >> m_aiDomainFreeExperiencesPerPopGlobal;
	kStream >> m_aiDomainFreeExperiencesPerTurn;
	kStream >> m_aiDomainEnemyCombatModifier;
	kStream >> m_aiDomainFriendsCombatModifierLocal;

	kStream >> m_abEverOwned;
	kStream >> m_abRevealed;

	kStream >> m_strName;
	kStream >> m_strScriptData;

	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_paiNoResource.dirtyGet());
	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_paiFreeResource.dirtyGet());
	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_paiNumResourcesLocal.dirtyGet());

	kStream >> m_paiSpecialistProduction;
	kStream >> m_paiProjectProduction;

	m_pCityBuildings->Read(kStream);

	if (uiVersion < 6)
	{
		CvCivilizationInfo& thisCivInfo = *GC.getCivilizationInfo(getCivilizationType());
		for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
		{
			BuildingClassTypes eBuildingClass = (BuildingClassTypes)iI;
			BuildingTypes eBuilding = (BuildingTypes)(thisCivInfo.getCivilizationBuildings(eBuildingClass));
			if (eBuilding != NO_BUILDING)
			{
				CvBuildingEntry *pkEntry = GC.getBuildingInfo(eBuilding);
				if (pkEntry)
				{
					if (pkEntry->GetAirModifier() > 0 && m_pCityBuildings->GetNumBuilding(eBuilding) > 0)
					{
#if defined(MOD_BUGFIX_MINOR)
						m_iMaxAirUnits += pkEntry->GetAirModifier() * m_pCityBuildings->GetNumBuilding(eBuilding);
#else
						m_iMaxAirUnits += pkEntry->GetAirModifier();
#endif
					}
				}
			}
		}
	}

	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_paiUnitProduction.dirtyGet());
	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_paiUnitProductionTime.dirtyGet());

	kStream >> m_paiSpecialistCount;
	kStream >> m_paiMaxSpecialistCount;
	kStream >> m_paiForceSpecialistCount;
	kStream >> m_paiFreeSpecialistCount;
	kStream >> m_paiImprovementFreeSpecialists;
	kStream >> m_paiUnitCombatFreeExperience;
	kStream >> m_paiUnitCombatProductionModifier;

	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_paiFreePromotionCount.dirtyGet());
	kStream >> m_viTradeRouteDomainRangeModifier;
	kStream >> m_viTradeRouteDomainGoldBonus;

#ifdef MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD
	kStream >> m_ppiYieldFromOtherYield;
	kStream >> m_bHasYieldFromOtherYield;
#endif

	UINT uLength;
	kStream >> uLength;
	for(UINT uIdx = 0; uIdx < uLength; ++uIdx)
	{
		OrderData  Data;

		kStream >> Data.eOrderType;
		if (uiVersion >= 5)
		{
			// Translate the data
			switch (Data.eOrderType)
			{
			case ORDER_TRAIN:
				Data.iData1 = CvInfosSerializationHelper::ReadHashed(kStream);
				kStream >> Data.iData2;		// This is a UnitAIType, but no code respects the ordering in GC.getUnitAIInfo
				break;

			case ORDER_CONSTRUCT:
				Data.iData1 = CvInfosSerializationHelper::ReadHashed(kStream);
				kStream >> Data.iData2;
				break;

			case ORDER_CREATE:
				Data.iData1 = CvInfosSerializationHelper::ReadHashed(kStream);
				kStream >> Data.iData2;
				break;

			case ORDER_PREPARE:
				Data.iData1 = CvInfosSerializationHelper::ReadHashed(kStream);
				kStream >> Data.iData2;
				break;

			case ORDER_MAINTAIN:
				Data.iData1 = CvInfosSerializationHelper::ReadHashed(kStream);
				kStream >> Data.iData2;
				break;

			default:
				CvAssertMsg(false, "pData->eOrderType failed to match a valid option");
				kStream >> Data.iData1;
				kStream >> Data.iData2;
				break;
			}
		}
		else
		{
			kStream >> Data.iData1;
			kStream >> Data.iData2;
		}

		kStream >> Data.bSave;
		kStream >> Data.bRush;

		bool bIsValid = false;
		switch (Data.eOrderType)
		{
			case ORDER_TRAIN:
				bIsValid = GC.getUnitInfo( (UnitTypes)Data.iData1 ) != NULL;
				CvAssertMsg(bIsValid, "Unit in build queue is invalid");
				break;

			case ORDER_CONSTRUCT:
				bIsValid = GC.getBuildingInfo( (BuildingTypes)Data.iData1 ) != NULL;
				CvAssertMsg(bIsValid, "Building in build queue is invalid");
				break;

			case ORDER_CREATE:
				bIsValid = GC.getProjectInfo( (ProjectTypes)Data.iData1 ) != NULL;
				CvAssertMsg(bIsValid, "Project in build queue is invalid");
				break;

			case ORDER_PREPARE:
				bIsValid = GC.getSpecialistInfo( (SpecialistTypes)Data.iData1 ) != NULL;
				CvAssertMsg(bIsValid, "Specialize in build queue is invalid");
				break;

			case ORDER_MAINTAIN:
				bIsValid = GC.getProcessInfo( (ProcessTypes)Data.iData1 ) != NULL;
				CvAssertMsg(bIsValid, "Process in build queue is invalid");
				break;
		}

		if (bIsValid)
			m_orderQueue.insertAtEnd(&Data);
	}

	kStream >> m_iPopulationRank;
	kStream >> m_bPopulationRankValid;
	kStream >> m_aiBaseYieldRank;
	kStream >> m_abBaseYieldRankValid;
	kStream >> m_aiYieldRank;
	kStream >> m_abYieldRankValid;
	kStream >> m_paiHurryModifier;

	kStream >> m_iGameTurnLastExpanded;
	m_strName = "";

	// City Building Happiness
	kStream >> m_iBaseHappinessFromBuildings;
	kStream >> m_iUnmoddedHappinessFromBuildings;

	kStream >> m_bRouteToCapitalConnectedLastTurn;
	kStream >> m_bRouteToCapitalConnectedThisTurn;
	kStream >> m_strName;

	kStream >> m_bOwedCultureBuilding;



#if defined(MOD_BUGFIX_FREE_FOOD_BUILDING)
	MOD_SERIALIZE_READ(30, kStream, m_bOwedFoodBuilding, false);
#endif


	m_pCityStrategyAI->Read(kStream);
	if(m_eOwner != NO_PLAYER)
	{
		GET_PLAYER(getOwner()).GetFlavorManager()->AddFlavorRecipient(m_pCityStrategyAI, false /* bPropogateFlavorValues */);
	}
	m_pCityCitizens->Read(kStream);
	kStream >> *m_pCityReligions;
	m_pEmphases->Read(kStream);

	kStream >> *m_pCityEspionage;

#if defined(MOD_NUCLEAR_WINTER_FOR_SP)
	kStream >> m_iNumNoNuclearWinterLocal;
#endif
#if defined(MOD_TROOPS_AND_CROPS_FOR_SP)
	kStream >> m_iCityEnableCrops;
	kStream >> m_iCityEnableArmee;
#endif
#if defined(MOD_INTERNATIONAL_IMMIGRATION_FOR_SP)
	kStream >> m_bCanDoImmigration;
	kStream >> m_iNumAllScaleImmigrantIn;
#endif
#ifdef MOD_GLOBAL_CITY_SCALES
	int iCityScale;
	kStream >> iCityScale;
	m_eCityScale = (CityScaleTypes)iCityScale;
#endif

#ifdef MOD_GLOBAL_CORRUPTION
	kStream >> m_iCachedCorruptionScore;
	kStream >> (int&) m_eCachedCorruptionLevel;
	kStream >> m_iCorruptionScoreChangeFromBuilding;
	kStream >> m_iCorruptionLevelChangeFromBuilding;
#endif

	kStream >> m_bIsSecondCapital;

#ifdef MOD_PROMOTION_CITY_DESTROYER
	kStream >> m_iSiegeKillCitizensModifier;
#endif

	kStream >> m_yieldChanges;
#ifdef MOD_BUILDING_IMPROVEMENT_RESOURCES
	kStream >> m_ppiResourceFromImprovement;
#endif
	kStream >> m_paiNumTerrainWorked;
	kStream >> m_paiNumFeaturelessTerrainWorked;	
	kStream >> m_paiNumImprovementWorked;
	kStream >> m_paiNumFeatureWorked;
	kStream >> m_iNumNearbyMountains;
	kStream >> m_iAdditionalFood;
	kStream >> m_iBaseTourism;
	kStream >> m_iBaseTourismBeforeModifiers;
	kStream >> m_iLastTurnWorkerDisbanded;
	kStream >> m_iDefendedAgainstSpreadUntilTurn;

#ifdef MOD_API_UNIFIED_YIELDS_MORE	
	kStream >> m_bIsColony;
	kStream >> m_iOrganizedCrime;
	kStream >> m_iResistanceCounter;
	kStream >> m_iPlagueCounter;
	kStream >> m_iPlagueTurns;
	kStream >> m_iPlagueType;
	kStream >> m_iLoyaltyCounter;
	kStream >> m_iDisloyaltyCounter;
	kStream >> m_iLoyaltyStateType;
	kStream >> m_aiYieldModifierFromHealth;
	kStream >> m_aiYieldModifierFromCrime;
	kStream >> m_aiYieldFromHappiness;
	kStream >> m_aiYieldFromHealth;
	kStream >> m_aiYieldFromCrime;
	kStream >> m_aiStaticCityYield;
#endif
	kStream >> m_ppiYieldModifierFromFeature;
	kStream >> m_ppiYieldModifierFromTerrain;
	kStream >> m_ppiYieldModifierFromImprovement;
	kStream >> m_ppiYieldModifierFromSpecialist;
	kStream >> m_ppiYieldModifierFromResource;

	kStream >> m_aTradeRouteFromTheCityYields;
	kStream >> m_aTradeRouteFromTheCityYieldsPerEra;

	if (uiVersion >= 3)
	{
		kStream >> m_iExtraHitPoints;
	}
	else
	{
		// Recalculate
		int iTotalExtraHitPoints = 0;
		for(int eBuildingType = 0; eBuildingType < GC.getNumBuildingInfos(); eBuildingType++)
		{
			const BuildingTypes eBuilding = static_cast<BuildingTypes>(eBuildingType);
			CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);

			if (pkBuildingInfo)
			{
				int iCount = m_pCityBuildings->GetNumBuilding(eBuilding);
				if(iCount > 0)
				{
					iTotalExtraHitPoints += (pkBuildingInfo->GetExtraCityHitPoints() * iCount);
				}
			}
		}

		// Change all at once, rather than one by one, else the clamping might adjust the current damage.
		ChangeExtraHitPoints(iTotalExtraHitPoints);
	}
	kStream >> m_paiNumBuildingClasses;

	CvCityManager::OnCityCreated(this);
}

//	--------------------------------------------------------------------------------
void CvCity::write(FDataStream& kStream) const
{
	VALIDATE_OBJECT

	// Current version number
	uint uiVersion = 6;
	kStream << uiVersion;
	MOD_SERIALIZE_INIT_WRITE(kStream);

	kStream << m_iID;
	kStream << m_iX;
	kStream << m_iY;
	kStream << m_iRallyX;
	kStream << m_iRallyY;
	kStream << m_iGameTurnFounded;
	kStream << m_iGameTurnAcquired;
	kStream << m_iPopulation;
	kStream << m_iHighestPopulation;
#if defined(MOD_GLOBAL_CITY_AUTOMATON_WORKERS)
	MOD_SERIALIZE_WRITE(kStream, m_iAutomatons);
#endif
#if defined(MOD_BUILDINGS_CITY_AUTOMATON_WORKERS)
	MOD_SERIALIZE_WRITE(kStream, m_iCityAutomatonWorkersChange);
#endif
	kStream << m_iNumGreatPeople;
	kStream << m_iBaseGreatPeopleRate;
	kStream << m_iGreatPeopleRateModifier;
	kStream << m_iJONSCultureStored;
	kStream << m_iJONSCultureLevel;
	kStream << m_iJONSCulturePerTurnFromBuildings;
	kStream << m_iJONSCulturePerTurnFromPolicies;
	kStream << m_iJONSCulturePerTurnFromSpecialists;
	kStream << m_iCultureRateModifier;
	kStream << m_iNumWorldWonders;
	kStream << m_iNumTeamWonders;
	kStream << m_iNumNationalWonders;
	kStream << m_iWonderProductionModifier;
	kStream << m_iCapturePlunderModifier;  // Added for Version 3
	kStream << m_iPlotCultureCostModifier; // Added for Version 3
	kStream << m_iPlotBuyCostModifier; // Added for Version 12
	kStream << m_iUnitMaxExperienceLocal;
	kStream << m_iTradeRouteRiverBonusModifier;
	kStream << m_iSecondCapitalsExtraScore;
	kStream << m_iFoodKeptFromPollution;
	kStream << m_iNumAllowsFoodTradeRoutes;
	kStream << m_iNumAllowsProductionTradeRoutes;
#if defined(MOD_BUILDINGS_CITY_WORKING)
	MOD_SERIALIZE_WRITE(kStream, m_iCityWorkingChange);
#endif
#if defined(MOD_BUILDINGS_CITY_AUTOMATON_WORKERS)
	// SEE ABOVE - MOD_SERIALIZE_WRITE(kStream, m_iCityAutomatonWorkersChange);
#endif
#if defined(MOD_VOLCANO_BREAK)
	kStream << m_iImmueVolcanoDamage;
#endif
	kStream << m_iAddsFreshWater;
	kStream << m_iExtraAttackOnKill;
	kStream << m_iForbiddenForeignSpyCount;
	kStream << m_iAllowSpaceshipLaunchCount;
#ifdef MOD_ROG_CORE
	kStream << m_iCityBuildingRangeStrikeModifier;
	kStream << m_iExtraDamageHealPercent;
	kStream << m_iExtraDamageHeal;
	kStream << m_iBombardRange;
	kStream << m_iBombardIndirect;
	kStream << m_iNumAttacks;
	kStream << m_iAttacksMade;
	kStream << m_iNukeInterceptionChance;

	kStream << m_iResetDamageValue;
	kStream << m_iReduceDamageValue;

	kStream << m_iWaterTileDamage;
	kStream << m_iWaterTileMovementReduce;
	kStream << m_iWaterTileTurnDamage;
	kStream << m_iLandTileDamage;
	kStream << m_iLandTileMovementReduce;
	kStream << m_iLandTileTurnDamage;
#endif





	kStream << m_iMaintenance;
	kStream << m_iHealRate;
	kStream << m_iEspionageModifier;
#if defined(MOD_RELIGION_CONVERSION_MODIFIERS)
	MOD_SERIALIZE_WRITE(kStream, m_iConversionModifier);
#endif
	kStream << m_iNoOccupiedUnhappinessCount;
	kStream << m_iFood;
	kStream << m_iFoodKept;
	kStream << m_iMaxFoodKeptPercent;
	kStream << m_iOverflowProduction;
	kStream << m_iFeatureProduction;
	kStream << m_iMilitaryProductionModifier;
	kStream << m_iSpaceProductionModifier;
	kStream << m_iFreeExperience;
	kStream << m_iNumCanAirlift;
	kStream << m_iCurrAirlift; // unused
	kStream << m_iMaxAirUnits;
	kStream << m_iAirModifier; // unused
	kStream << m_iNukeModifier;
	kStream << m_iTradeRouteTargetBonus;
	kStream << m_iTradeRouteRecipientBonus;
	kStream << m_iCultureUpdateTimer;
	kStream << m_iCitySizeBoost;
	kStream << m_iSpecialistFreeExperience;
	kStream << m_iStrengthValue;
	kStream << m_iDamage;
	kStream << m_iThreatValue;
	kStream << m_iGarrisonedUnit;
	CvInfosSerializationHelper::WriteHashed(kStream, (ResourceTypes)(m_iResourceDemanded.get()));
	kStream << m_iWeLoveTheKingDayCounter;
	kStream << m_iLastTurnGarrisonAssigned;
	kStream << m_iThingsProduced;
	kStream << m_iDemandResourceCounter;
	kStream << m_iResistanceTurns;
	kStream << m_iRazingTurns;
	kStream << m_iCountExtraLuxuries;
	kStream << m_iCheapestPlotInfluence;
	kStream << m_unitBeingBuiltForOperation.m_iOperationID;
	kStream << m_unitBeingBuiltForOperation.m_iArmyID;
	kStream << m_unitBeingBuiltForOperation.m_iSlotID;

	kStream << m_bNeverLost;
	kStream << m_bDrafted;
	kStream << m_bAirliftTargeted;  // unused
	kStream << m_bProductionAutomated;
	kStream << m_bMadeAttack;
	kStream << m_bOccupied;
	kStream << m_bPuppet;
	kStream << m_bEverCapital;
	kStream << m_bIndustrialRouteToCapital;
	kStream << m_bFeatureSurrounded;

	kStream << m_eOwner;
	kStream << m_ePreviousOwner;
	kStream << m_eOriginalOwner;
	kStream << m_ePlayersReligion;

	kStream << m_aiSeaPlotYield;
	kStream << m_aiRiverPlotYield;
	kStream << m_aiLakePlotYield;
	kStream << m_aiSeaResourceYield;
	kStream << m_aiBaseYieldRateFromTerrain;
	kStream << m_aiBaseYieldRateFromBuildings;
	kStream << m_aiBaseYieldRateFromBuildingsPolicies;
	kStream << m_aiBaseYieldRateFromSpecialists;
	kStream << m_aiBaseYieldRateFromProjects;
	kStream << m_aiBaseYieldRateFromMisc;
	kStream << m_aiBaseYieldRateFromReligion;
	kStream << m_aiYieldPerPop;
	kStream << m_aiYieldPerPopInEmpire;
	kStream << m_aiYieldFromInternalTR;
	kStream << m_aiYieldFromProcessModifier;
	kStream << m_aiNumProjects;
#if defined(MOD_ROG_CORE)
	kStream << m_aiYieldPerAlly;
	kStream << m_aiYieldPerFriend;
	kStream << m_aiNumTimesAttackedThisTurn;
	kStream << m_aiBaseYieldRateFromCSAlliance;
	kStream << m_aiBaseYieldRateFromCSFriendship;
	kStream << m_aiSpecialistRateModifier;
	kStream << m_aiYieldPerEspionageSpy;
	kStream << m_aiBaseYieldRateFromEspionageSpy;
	kStream << m_aiYieldFromConstruction;
	kStream << m_aiYieldFromUnitProduction;
	kStream << m_aiYieldFromBirth;
	kStream << m_aiYieldFromBorderGrowth;
	kStream << m_aiYieldFromPillage;
	kStream << m_aiResourceQuantityFromPOP;
#endif
	kStream << m_aiYieldPerReligion;
	kStream << m_aiYieldPerEra;
	kStream << m_aiYieldModifierPerEra;
	kStream << m_aiCityStateTradeRouteYieldModifier;
	kStream << m_aiYieldRateModifier;
	kStream << m_aiYieldRateMultiplier;
	kStream << m_aiPowerYieldRateModifier;
	kStream << m_aiFeatureYieldRateModifier;
	kStream << m_aiTerrainYieldRateModifier;
	kStream << m_aiImprovementYieldRateModifier;
	kStream << m_aiSpecialistYieldRateModifier;
	kStream << m_aiResourceYieldRateModifier;
	kStream << m_aiExtraSpecialistYield;
	kStream << m_aiProductionToYieldModifier;
	kStream << m_aiDomainFreeExperience;
	kStream << m_aiDomainProductionModifier;
	kStream << m_aiDomainFreeExperiencesPerPop;
	kStream << m_aiDomainFreeExperiencesPerPopGlobal;
	kStream << m_aiDomainFreeExperiencesPerTurn;
	kStream << m_aiDomainEnemyCombatModifier;
	kStream << m_aiDomainFriendsCombatModifierLocal;

	kStream << m_abEverOwned;
	kStream << m_abRevealed;

	kStream << m_strName;
	kStream << m_strScriptData;

	CvInfosSerializationHelper::WriteHashedDataArray<ResourceTypes, int>(kStream, m_paiNoResource);
	CvInfosSerializationHelper::WriteHashedDataArray<ResourceTypes, int>(kStream, m_paiFreeResource);
	CvInfosSerializationHelper::WriteHashedDataArray<ResourceTypes, int>(kStream, m_paiNumResourcesLocal);

	kStream << m_paiSpecialistProduction;
	kStream << m_paiProjectProduction;

	m_pCityBuildings->Write(kStream);

	CvInfosSerializationHelper::WriteHashedDataArray<UnitTypes, int>(kStream, m_paiUnitProduction);
	CvInfosSerializationHelper::WriteHashedDataArray<UnitTypes, int>(kStream, m_paiUnitProductionTime);

	kStream << m_paiSpecialistCount;
	kStream << m_paiMaxSpecialistCount;
	kStream << m_paiForceSpecialistCount;
	kStream << m_paiFreeSpecialistCount;
	kStream << m_paiImprovementFreeSpecialists;
	kStream << m_paiUnitCombatFreeExperience;
	kStream << m_paiUnitCombatProductionModifier;

	CvInfosSerializationHelper::WriteHashedDataArray<PromotionTypes, int>(kStream, m_paiFreePromotionCount);
	kStream << m_viTradeRouteDomainRangeModifier;
	kStream << m_viTradeRouteDomainGoldBonus;

#ifdef MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD
	kStream << m_ppiYieldFromOtherYield;
	kStream << m_bHasYieldFromOtherYield;
#endif

	//  Write m_orderQueue
	UINT uLength = (UINT)m_orderQueue.getLength();
	kStream << uLength;
	for(UINT uIdx = 0; uIdx < uLength; ++uIdx)
	{
		OrderData* pData = m_orderQueue.getAt(uIdx);

		kStream << pData->eOrderType;
		// Now we have to translate the data because most of them contain indices into Infos tables and it is very bad to save an index since the table order can change
		switch (pData->eOrderType)
		{
			case ORDER_TRAIN:
				CvInfosSerializationHelper::WriteHashed(kStream, (UnitTypes)(pData->iData1));
				kStream << pData->iData2;	// This is a UnitAIType, but no code respects the ordering in GC.getUnitAIInfo so just write out the index
				break;

			case ORDER_CONSTRUCT:
				CvInfosSerializationHelper::WriteHashed(kStream, (BuildingTypes)pData->iData1);
				kStream << pData->iData2;
				break;

			case ORDER_CREATE:
				CvInfosSerializationHelper::WriteHashed(kStream, (ProjectTypes)pData->iData1);
				kStream << pData->iData2;
				break;

			case ORDER_PREPARE:
				CvInfosSerializationHelper::WriteHashed(kStream, (SpecialistTypes)pData->iData1);
				kStream << pData->iData2;
				break;

			case ORDER_MAINTAIN:
				CvInfosSerializationHelper::WriteHashed(kStream, (ProcessTypes)pData->iData1);
				kStream << pData->iData2;
				break;

			default:
				CvAssertMsg(false, "pData->eOrderType failed to match a valid option");
				kStream << pData->iData1;
				kStream << pData->iData2;
				break;
		}
		kStream << pData->bSave;
		kStream << pData->bRush;
	}

	kStream << m_iPopulationRank;
	kStream << m_bPopulationRankValid;
	kStream << m_aiBaseYieldRank;
	kStream << m_abBaseYieldRankValid;
	kStream << m_aiYieldRank;
	kStream << m_abYieldRankValid;
	kStream << m_paiHurryModifier;

	kStream << m_iGameTurnLastExpanded;

	kStream << m_iBaseHappinessFromBuildings;
	kStream << m_iUnmoddedHappinessFromBuildings;

	kStream << m_bRouteToCapitalConnectedLastTurn;
	kStream << m_bRouteToCapitalConnectedThisTurn;
	kStream << m_strName;
	kStream << m_bOwedCultureBuilding;



#if defined(MOD_BUGFIX_FREE_FOOD_BUILDING)
	MOD_SERIALIZE_WRITE(kStream, m_bOwedFoodBuilding);
#endif

	m_pCityStrategyAI->Write(kStream);
	m_pCityCitizens->Write(kStream);
	kStream << *m_pCityReligions;
	m_pEmphases->Write(kStream);
	kStream << *m_pCityEspionage;

#if defined(MOD_NUCLEAR_WINTER_FOR_SP)
	kStream << m_iNumNoNuclearWinterLocal;
#endif
#if defined(MOD_TROOPS_AND_CROPS_FOR_SP)
	kStream << m_iCityEnableCrops;
	kStream << m_iCityEnableArmee;
#endif
#if defined(MOD_INTERNATIONAL_IMMIGRATION_FOR_SP)
	kStream << m_bCanDoImmigration;
	kStream << m_iNumAllScaleImmigrantIn;
#endif
#ifdef MOD_GLOBAL_CITY_SCALES
	kStream << (int) m_eCityScale;
#endif

#ifdef MOD_GLOBAL_CORRUPTION
	kStream << m_iCachedCorruptionScore;
	kStream << (int) m_eCachedCorruptionLevel;
	kStream << m_iCorruptionScoreChangeFromBuilding;
	kStream << m_iCorruptionLevelChangeFromBuilding;
#endif

	kStream << m_bIsSecondCapital;

#ifdef MOD_PROMOTION_CITY_DESTROYER
	kStream << m_iSiegeKillCitizensModifier;
#endif

	kStream << m_yieldChanges;
#ifdef MOD_BUILDING_IMPROVEMENT_RESOURCES
	kStream << m_ppiResourceFromImprovement;
#endif
	kStream << m_paiNumTerrainWorked;
	kStream << m_paiNumFeaturelessTerrainWorked;
	kStream << m_paiNumImprovementWorked;
	kStream << m_paiNumFeatureWorked;
	kStream << m_iNumNearbyMountains;
	kStream << m_iAdditionalFood;
	kStream << m_iBaseTourism;
	kStream << m_iBaseTourismBeforeModifiers;
	kStream << m_iLastTurnWorkerDisbanded;
	kStream << m_iDefendedAgainstSpreadUntilTurn;

#ifdef MOD_API_UNIFIED_YIELDS_MORE
	kStream << m_bIsColony;
	kStream << m_iOrganizedCrime;
	kStream << m_iResistanceCounter;
	kStream << m_iPlagueCounter;
	kStream << m_iPlagueTurns;
	kStream << m_iPlagueType;
	kStream << m_iLoyaltyCounter;
	kStream << m_iDisloyaltyCounter;
	kStream << m_iLoyaltyStateType;
	kStream << m_aiYieldModifierFromHealth;
	kStream << m_aiYieldModifierFromCrime;
	kStream << m_aiYieldFromHappiness;
	kStream << m_aiYieldFromHealth;
	kStream << m_aiYieldFromCrime;
	kStream << m_aiStaticCityYield;
#endif
	kStream << m_ppiYieldModifierFromFeature;
	kStream << m_ppiYieldModifierFromTerrain;
	kStream << m_ppiYieldModifierFromImprovement;
	kStream << m_ppiYieldModifierFromSpecialist;
	kStream << m_ppiYieldModifierFromResource;

	kStream << m_aTradeRouteFromTheCityYields;
	kStream << m_aTradeRouteFromTheCityYieldsPerEra;

	kStream << m_iExtraHitPoints;
	kStream << m_paiNumBuildingClasses;
}


//	--------------------------------------------------------------------------------
bool CvCity::isValidBuildingLocation(BuildingTypes eBuilding) const
{
	VALIDATE_OBJECT

	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if(pkBuildingInfo == NULL)
		return false;

	// Requires Capital
	if(pkBuildingInfo->IsCapitalOnly())
	{
		if (!isCapital())
			return false;
	}
	// Requires Original Capital
	if(pkBuildingInfo->IsOriginalCapitalOnly())
	{
		if (!IsOriginalCapital())
			return false;
	}
	// Requires coast
	if(pkBuildingInfo->IsWater())
	{
		if (!isCoastal(pkBuildingInfo->GetMinAreaSize()))
			return false;
	}

	// Requires River
	if(pkBuildingInfo->IsRiver())
	{
		if(!(plot()->isRiver()))
			return false;
	}

	// Requires Fresh Water
	if(pkBuildingInfo->IsFreshWater())
	{
		if(!plot()->isFreshWater())
			return false;
	}

#if defined(MOD_BUILDING_NEW_EFFECT_FOR_SP)
	if(pkBuildingInfo->IsAnyWater())
	{
		if(!(isCoastal(pkBuildingInfo->GetMinAreaSize()) || plot()->isFreshWater()))
			return false;
	}
	if(pkBuildingInfo->IsRiverOrCoastal())
	{
		if(!(isCoastal(pkBuildingInfo->GetMinAreaSize()) || plot()->isRiver()))
			return false;
	}
#endif
#ifdef MOD_GLOBAL_CORRUPTION
	if(MOD_GLOBAL_CORRUPTION && GET_PLAYER(getOwner()).EnableCorruption())
	{
		int iCorruptionLevel = GetCorruptionLevel() - GC.getInfoTypeForString("CORRUPTION_LV0", true);
		int iMinLevel = pkBuildingInfo->GetMinCorruptionLevelNeeded();
		if(iMinLevel >= 0 && iCorruptionLevel < iMinLevel) return false;
		int iMaxLevel = pkBuildingInfo->GetMaxCorruptionLevelNeeded();
		if(iMaxLevel >= 0 && iCorruptionLevel > iMinLevel) return false;
	}
#endif

	// Requires adjacent Mountain
	if(pkBuildingInfo->IsMountain())
	{
		bool bFoundMountain = false;

		CvPlot* pAdjacentPlot;
		for(int iDirectionLoop = 0; iDirectionLoop < NUM_DIRECTION_TYPES; iDirectionLoop++)
		{
			pAdjacentPlot = plotDirection(getX(), getY(), ((DirectionTypes)iDirectionLoop));

			if(pAdjacentPlot != NULL)
			{
				if(pAdjacentPlot->isMountain())
				{
					bFoundMountain = true;
					break;
				}
			}
		}

		if(!bFoundMountain)
			return false;
	}

	// Requires nearby Mountain (within 2 tiles)
	if(pkBuildingInfo->IsNearbyMountainRequired())
	{
		bool bFoundMountain = false;

		const int iMountainRange = 2;
		CvPlot* pLoopPlot;

		for(int iDX = -iMountainRange; iDX <= iMountainRange; iDX++)
		{
			for(int iDY = -iMountainRange; iDY <= iMountainRange; iDY++)
			{
				pLoopPlot = plotXYWithRangeCheck(getX(), getY(), iDX, iDY, iMountainRange);
				if(pLoopPlot)
				{
					if(pLoopPlot->isMountain() && !pLoopPlot->IsNaturalWonder(true) && pLoopPlot->getOwner() == getOwner())
					{
						bFoundMountain = true;
						break;
					}
				}
			}

			if(bFoundMountain == true)
				break;
		}

		if(!bFoundMountain)
			return false;
	}

	// Requires Hills
	if(pkBuildingInfo->IsHill())
	{
		if(!(plot()->isHills() || plot()->isMountain()))
			return false;
	}

	// Requires Flat
	if(pkBuildingInfo->IsFlat())
	{
		if(plot()->isHills() || plot()->isMountain())
			return false;
	}

	// Requires city not built on certain terrain?
	TerrainTypes eTerrainProhibited = (TerrainTypes) pkBuildingInfo->GetProhibitedCityTerrain();
	if(eTerrainProhibited != NO_TERRAIN)
	{
		if(plot()->getTerrainType() == eTerrainProhibited)
		{
			return false;
		}
	}

	// Requires city to be on or next to a particular terrain type?
	TerrainTypes eTerrainRequired = (TerrainTypes) pkBuildingInfo->GetNearbyTerrainRequired();
	if(eTerrainRequired != NO_TERRAIN)
	{
		bool bFoundTerrain = false;

		// City on the right terrain?
		if(plot()->getTerrainType() == eTerrainRequired)
			bFoundTerrain = true;

		// Check adjacent plots
		if(!bFoundTerrain)
		{
			CvPlot* pAdjacentPlot;
			for(int iDirectionLoop = 0; iDirectionLoop < NUM_DIRECTION_TYPES; iDirectionLoop++)
			{
				pAdjacentPlot = plotDirection(getX(), getY(), ((DirectionTypes)iDirectionLoop));

				if(pAdjacentPlot != NULL)
				{
					// City adjacent to the right terrain?
					if(pAdjacentPlot->getTerrainType() == eTerrainRequired)
					{
						bFoundTerrain = true;
						break;
					}
				}
			}
		}

		// Didn't find nearby required terrain
		if(!bFoundTerrain)
			return false;
	}

	return true;
}


// CACHE: cache frequently used values
///////////////////////////////////////

//	--------------------------------------------------------------------------------
void CvCity::invalidatePopulationRankCache()
{
	VALIDATE_OBJECT
	m_bPopulationRankValid = false;
}

//	--------------------------------------------------------------------------------
void CvCity::invalidateYieldRankCache(YieldTypes eYield)
{
	VALIDATE_OBJECT
	CvAssertMsg(eYield >= NO_YIELD && eYield < NUM_YIELD_TYPES, "invalidateYieldRankCache passed bogus yield index");

	if(eYield == NO_YIELD)
	{
		for(int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			m_abBaseYieldRankValid.setAt(iI, false);
			m_abYieldRankValid.setAt(iI, false);
		}
	}
	else
	{
		m_abBaseYieldRankValid.setAt(eYield, false);
		m_abYieldRankValid.setAt(eYield, false);
	}
}


//	--------------------------------------------------------------------------------
bool CvCity::isMadeAttack() const
{
	VALIDATE_OBJECT
		//return m_bMadeAttack;
		return m_iAttacksMade >= m_iNumAttacks;
}


void CvCity::changeExtraAttacks(int iChange)
{
	VALIDATE_OBJECT
		if (iChange != 0)
		{
			m_iNumAttacks += iChange;

			DLLUI->setDirty(CityInfo_DIRTY_BIT, true);
		}
}

//	--------------------------------------------------------------------------------
void CvCity::setMadeAttack(bool bNewValue)
{
	VALIDATE_OBJECT
		//m_bMadeAttack = bNewValue;
		if (bNewValue)
		{
			m_iAttacksMade++;
		}
		else
		{
			m_iAttacksMade = 0;
		}
}

#if defined(MOD_ROG_CORE)
void CvCity::ChangeNumTimesAttackedThisTurn(PlayerTypes ePlayer, int iValue)
{
	VALIDATE_OBJECT
	CvAssertMsg(ePlayer >= 0, "ePlayer expected to be >= 0");
	CvAssertMsg(ePlayer < REALLY_MAX_PLAYERS, "ePlayer expected to be < NUM_DOMAIN_TYPES");
	m_aiNumTimesAttackedThisTurn[ePlayer] += iValue;
}
int CvCity::GetNumTimesAttackedThisTurn(PlayerTypes ePlayer) const
{
	VALIDATE_OBJECT
	CvAssertMsg(ePlayer >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(ePlayer < REALLY_MAX_PLAYERS, "eIndex expected to be < NUM_DOMAIN_TYPES");
	return m_aiNumTimesAttackedThisTurn[ePlayer];
}


int CvCity::getCityBuildingRangeStrikeModifier() const
{
	VALIDATE_OBJECT
	return m_iCityBuildingRangeStrikeModifier;
}
void CvCity::changeCityBuildingRangeStrikeModifier(int iValue)
{
	if (iValue != 0)
	{
		m_iCityBuildingRangeStrikeModifier += iValue;
	}
}

//	--------------------------------------------------------------------------------
int CvCity::getExtraDamageHealPercent() const
{
	VALIDATE_OBJECT
	return m_iExtraDamageHealPercent;
}
void CvCity::changeExtraDamageHealPercent(int iChange)
{
	VALIDATE_OBJECT
	if (iChange != 0)
	{
		m_iExtraDamageHealPercent += iChange;
	}
}

//	--------------------------------------------------------------------------------
int CvCity::getExtraDamageHeal() const
{
	VALIDATE_OBJECT
		return m_iExtraDamageHeal;
}

//	--------------------------------------------------------------------------------
void CvCity::changeExtraDamageHeal(int iChange)
{
	VALIDATE_OBJECT
		if (iChange != 0)
		{
			m_iExtraDamageHeal += iChange;
		}
}

//	--------------------------------------------------------------------------------
int CvCity::getResetDamageValue() const
{
	VALIDATE_OBJECT
		return m_iResetDamageValue;
}

//	--------------------------------------------------------------------------------
void CvCity::changeResetDamageValue(int iChange)
{
	VALIDATE_OBJECT
		if (iChange != 0)
		{
			m_iResetDamageValue += iChange;
		}
}
//	--------------------------------------------------------------------------------
int CvCity::getReduceDamageValue() const
{
	VALIDATE_OBJECT
		return m_iReduceDamageValue;
}

//	--------------------------------------------------------------------------------
void CvCity::changeReduceDamageValue(int iChange)
{
	VALIDATE_OBJECT
		if (iChange != 0)
		{
			m_iReduceDamageValue += iChange;
		}
}




//	--------------------------------------------------------------------------------
int CvCity::getWaterTileDamage() const
{
	VALIDATE_OBJECT
		return m_iWaterTileDamage;
}

//	--------------------------------------------------------------------------------
void CvCity::changeWaterTileDamage(int iChange)
{
	VALIDATE_OBJECT
		if (iChange != 0)
		{
			m_iWaterTileDamage += iChange;
		}
}

//	--------------------------------------------------------------------------------
int CvCity::getWaterTileMovementReduce() const
{
	VALIDATE_OBJECT
		return m_iWaterTileMovementReduce;
}

//	--------------------------------------------------------------------------------
void CvCity::changeWaterTileMovementReduce(int iChange)
{
	VALIDATE_OBJECT
		if (iChange != 0)
		{
			m_iWaterTileMovementReduce += iChange;
		}
}


//	--------------------------------------------------------------------------------
int CvCity::getWaterTileTurnDamage() const
{
	VALIDATE_OBJECT
		return m_iWaterTileTurnDamage;
}

//	--------------------------------------------------------------------------------
void CvCity::changeWaterTileTurnDamage(int iChange)
{
	VALIDATE_OBJECT
		if (iChange != 0)
		{
			m_iWaterTileTurnDamage += iChange;
		}
}


//	--------------------------------------------------------------------------------
int CvCity::getLandTileDamage() const
{
	VALIDATE_OBJECT
		return m_iLandTileDamage;
}

//	--------------------------------------------------------------------------------
void CvCity::changeLandTileDamage(int iChange)
{
	VALIDATE_OBJECT
		if (iChange != 0)
		{
			m_iLandTileDamage += iChange;
		}
}


//	--------------------------------------------------------------------------------
int CvCity::getLandTileMovementReduce() const
{
	VALIDATE_OBJECT
		return m_iLandTileMovementReduce;
}

//	--------------------------------------------------------------------------------
void CvCity::changeLandTileMovementReduce(int iChange)
{
	VALIDATE_OBJECT
		if (iChange != 0)
		{
			m_iLandTileMovementReduce += iChange;
		}
}

//	--------------------------------------------------------------------------------
int CvCity::getLandTileTurnDamage() const
{
	VALIDATE_OBJECT
		return m_iLandTileTurnDamage;
}

//	--------------------------------------------------------------------------------
void CvCity::changeLandTileTurnDamage(int iChange)
{
	VALIDATE_OBJECT
		if (iChange != 0)
		{
			m_iLandTileTurnDamage += iChange;
		}
}
#endif



#if defined(MOD_EVENTS_CITY_BOMBARD)
//	--------------------------------------------------------------------------------
int CvCity::getBombardRange() const
{
	bool bIndirectFireAllowed;
	return getBombardRange(bIndirectFireAllowed);
}

//	--------------------------------------------------------------------------------
int CvCity::getBombardRange(bool& bIndirectFireAllowed) const
{
	VALIDATE_OBJECT
	
	if (MOD_ROG_CORE_BOMBARD_RANGE_BUILDINGS)
	{
		bIndirectFireAllowed = isBombardIndirect();

		if (GET_TEAM(getTeam()).isBombardIndirect())
		{
			bIndirectFireAllowed = true;
		}
		return GC.getCITY_ATTACK_RANGE() + getExtraBombardRange() + GET_TEAM(getTeam()).GetBombardRange();
	}

	if (MOD_EVENTS_CITY_BOMBARD) 
	{
		int iValue = 0;
		if (GAMEEVENTINVOKE_VALUE(iValue, GAMEEVENT_GetBombardRange, getOwner(), GetID()) == GAMEEVENTRETURN_VALUE) {
			// Defend against modder stupidity!
			if (iValue != 0 && ::abs(iValue) <= /*2*/ GD_INT_GET(MAX_CITY_ATTACK_RANGE)) {
				bIndirectFireAllowed = (iValue < 0);
				return ::abs(iValue);
			}
		}
	}

	bIndirectFireAllowed = GC.getCAN_CITY_USE_INDIRECT_FIRE();
	return GC.getCITY_ATTACK_RANGE();
}
#endif

//	--------------------------------------------------------------------------------
bool CvCity::canRangeStrike() const
{
	VALIDATE_OBJECT

	// Can't shoot more than once per turn
	if(isMadeAttack())
		return false;

	// Can't shoot when in resistance
	if(IsResistance() || IsRazing())
		return false;

	// Can't shoot if we have no HP left (shouldn't really ever happen)
	if(getDamage() == GetMaxHitPoints())
		return false;

	// Apparently it's possible for someone to fire during another player's turn
	if(!GET_PLAYER(getOwner()).isTurnActive())
		return false;

	return true;
}

//	--------------------------------------------------------------------------------
bool CvCity::CanRangeStrikeNow() const
{
	if(!canRangeStrike())
	{
		return false;
	}

	int iRange = getBombardRange();

	CvPlot* pPlot = plot();
	for (int iRing = 1; iRing <= min(5, iRange); iRing++)
	{
		for (int i = RING_PLOTS[iRing - 1]; i < RING_PLOTS[iRing]; i++)
		{
			CvPlot* pTargetPlot = iterateRingPlots(pPlot, i);
			if (!pTargetPlot)
				continue;

			//this checks everything, visibility, LOS, target type etc
			if (canRangeStrikeAt(pTargetPlot->getX(), pTargetPlot->getY()))
				return true;
		}
	}

	return false;
}



//	--------------------------------------------------------------------------------
/// Does this City have a Building that allows it to Range Strike?
bool CvCity::IsHasBuildingThatAllowsRangeStrike() const
{
	VALIDATE_OBJECT
	bool bHasBuildingThatAllowsRangeStrike = false;

	for(int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
	{
		const BuildingTypes eBuilding = static_cast<BuildingTypes>(iBuildingLoop);
		CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);

		if(pkBuildingInfo)
		{
			// Has this Building
			if(GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
			{
				// Does it grant Range Strike ability?
				if(pkBuildingInfo->IsAllowsRangeStrike())
				{
					bHasBuildingThatAllowsRangeStrike = true;
					break;
				}
			}
		}
	}

	return bHasBuildingThatAllowsRangeStrike;
}

//	--------------------------------------------------------------------------------
bool CvCity::canRangeStrikeAt(int iX, int iY) const
{
	VALIDATE_OBJECT
	if(!canRangeStrike())
	{
		return false;
	}

	const CvPlot* pTargetPlot = GC.getMap().plot(iX, iY);

	if(NULL == pTargetPlot)
		return false;

	if(!pTargetPlot->isVisible(getTeam()))
		return false;

	bool bIndirectFire = false;
	int iAttackRange = getBombardRange(bIndirectFire);


	if(plotDistance(plot()->getX(), plot()->getY(), pTargetPlot->getX(), pTargetPlot->getY()) > iAttackRange)
		return false;


	if (!bIndirectFire && !plot()->canSeePlot(pTargetPlot, getTeam(), iAttackRange, NO_DIRECTION))
		return false;

	// Can't attack other cities directly
	if (pTargetPlot->isCity())
		return false;

	if (!canRangedStrikeTarget(*pTargetPlot))
		return false;

	return true;
}

//	----------------------------------------------------------------------------
CityTaskResult CvCity::rangeStrike(int iX, int iY)
{
	VALIDATE_OBJECT
	CvUnit* pDefender;

	CityTaskResult eResult = TASK_ABORTED;

	CvPlot* pPlot = GC.getMap().plot(iX, iY);
	if(NULL == pPlot)
	{
		return eResult;
	}

	if(!canRangeStrikeAt(iX, iY))
	{
		return eResult;
	}

	setMadeAttack(true);

	// No City
	if(!pPlot->isCity())
	{
		pDefender = rangedStrikeTarget(pPlot);

		CvAssert(pDefender != NULL);
		if(!pDefender) return TASK_ABORTED;

		CvCombatInfo kCombatInfo;
		CvUnitCombat::GenerateRangedCombatInfo(*this, pDefender, *pPlot, &kCombatInfo);

		uint uiParentEventID = 0;
		if(!CvPreGame::quickCombat())
		{
			// Center camera here!
			bool isTargetVisibleToActivePlayer = pPlot->isActiveVisible(false);
			if(isTargetVisibleToActivePlayer)
			{
				auto_ptr<ICvPlot1> pDllPlot = GC.WrapPlotPointer(pPlot);
				DLLUI->lookAt(pDllPlot.get(), CAMERALOOKAT_NORMAL);
			}

			kCombatInfo.setVisualizeCombat(pPlot->isActiveVisible(false));

			auto_ptr<ICvCombatInfo1> pDllCombatInfo(new CvDllCombatInfo(&kCombatInfo));
			uiParentEventID = gDLL->GameplayCityCombat(pDllCombatInfo.get());

			// Set the combat units so that other missions do not continue until combat is over.
			pDefender->setCombatCity(this);
			setCombatUnit(pDefender);
			eResult = TASK_QUEUED;
		}
		else
		{
			eResult = TASK_COMPLETED;
		}

		CvUnitCombat::ResolveCombat(kCombatInfo, uiParentEventID);
	}

	return eResult;
}

//	--------------------------------------------------------------------------------
bool CvCity::canRangedStrikeTarget(const CvPlot& targetPlot) const
{
	VALIDATE_OBJECT
	return (const_cast<CvCity*>(this)->rangedStrikeTarget(const_cast<CvPlot*>(&targetPlot)) != 0);
}

//	--------------------------------------------------------------------------------
CvUnit* CvCity::rangedStrikeTarget(CvPlot* pPlot)
{
	VALIDATE_OBJECT
	UnitHandle pDefender = pPlot->getBestDefender(NO_PLAYER, getOwner(), NULL, true, false, false, /*bNoncombatAllowed*/ true);

	if(pDefender)
	{
		if(!pDefender->IsDead())
		{
#if defined(MOD_GLOBAL_SUBS_UNDER_ICE_IMMUNITY)
			// If the defender is a sub and the plot is ice, return NULL
			if (pDefender.pointer()->getInvisibleType() == 0 && pPlot->getFeatureType() == FEATURE_ICE) {
				return NULL;
			}
#endif

			return pDefender.pointer();
		}
	}

	return NULL;
}

//	--------------------------------------------------------------------------------
int CvCity::rangeCombatUnitDefense(const CvUnit* pDefender) const
{
	int iDefenderStrength = 0;

	// Use Ranged combat value for defender, UNLESS it's a boat
	if (pDefender->isEmbarked())
	{
		iDefenderStrength = pDefender->GetEmbarkedUnitDefense();;
	}

	else if(!pDefender->isRangedSupportFire() && !pDefender->getDomainType() == DOMAIN_SEA && pDefender->GetMaxRangedCombatStrength(NULL, /*pCity*/ NULL, false, false) > 0)
	{
		iDefenderStrength = pDefender->GetMaxRangedCombatStrength(NULL, /*pCity*/ NULL, false, false);

		// Ranged units take less damage from one another
		iDefenderStrength *= /*125*/ GC.getRANGE_ATTACK_RANGED_DEFENDER_MOD();
		iDefenderStrength /= 100;
	}
	else
	{
		iDefenderStrength = pDefender->GetMaxDefenseStrength(pDefender->plot(), NULL, /*bFromRangedAttack*/ true);
	}

	return iDefenderStrength;
}

//	--------------------------------------------------------------------------------
int CvCity::rangeCombatDamage(const CvUnit* pDefender, CvCity* pCity, bool bIncludeRand) const
{
	VALIDATE_OBJECT
	int iAttackerStrength;

	iAttackerStrength = getStrengthValue(true);

	int iDefenderStrength;

	// No City
	if(pCity == NULL)
	{
		// If this is a defenseless unit, do a fixed amount of damage
		if(!pDefender->IsCanDefend())
		{
			return GC.getNONCOMBAT_UNIT_RANGED_DAMAGE();
		}

		iDefenderStrength = rangeCombatUnitDefense(pDefender);

	}
	// City
	else
	{
		iDefenderStrength = pCity->getStrengthValue();
	}

	// The roll will vary damage between 30 and 40 (out of 100) for two units of identical strength

	int iAttackerDamage = /*250*/ GC.getRANGE_ATTACK_SAME_STRENGTH_MIN_DAMAGE();

	int iAttackerRoll = 0;
	if(bIncludeRand)
	{
		iAttackerRoll = GC.getGame().getJonRandNum(/*300*/ GC.getRANGE_ATTACK_SAME_STRENGTH_POSSIBLE_EXTRA_DAMAGE(), "City Ranged Attack Damage");
	}
	else
	{
		iAttackerRoll = /*300*/ GC.getRANGE_ATTACK_SAME_STRENGTH_POSSIBLE_EXTRA_DAMAGE();
		iAttackerRoll -= 1;	// Subtract 1 here, because this is the amount normally "lost" when doing a rand roll
		iAttackerRoll /= 2;	// The divide by 2 is to provide the average damage
	}
	iAttackerDamage += iAttackerRoll;


	double fStrengthRatio = (double(iAttackerStrength) / iDefenderStrength);

	// In case our strength is less than the other guy's, we'll do things in reverse then make the ratio 1 over the result
	if(iDefenderStrength > iAttackerStrength)
	{
		fStrengthRatio = (double(iDefenderStrength) / iAttackerStrength);
	}

	fStrengthRatio = (fStrengthRatio + 3) / 4;
	fStrengthRatio = pow(fStrengthRatio, 4.0);
	fStrengthRatio = (fStrengthRatio + 1) / 2;

	if(iDefenderStrength > iAttackerStrength)
	{
		fStrengthRatio = 1 / fStrengthRatio;
	}

	iAttackerDamage = int(iAttackerDamage * fStrengthRatio);

	// Bring it back out of hundreds
	iAttackerDamage /= 100;

	// Always do at least 1 damage
	int iMinDamage = /*1*/ GC.getMIN_CITY_STRIKE_DAMAGE();
	if(iAttackerDamage < iMinDamage)
		iAttackerDamage = iMinDamage;

	return iAttackerDamage;
}

//	--------------------------------------------------------------------------------
int CvCity::GetAirStrikeDefenseDamage(const CvUnit* pAttacker, bool bIncludeRand) const
{
	int iAttackerStrength = pAttacker->GetMaxRangedCombatStrength(NULL, /*pCity*/ NULL, true, false);
	int iDefenderStrength = getStrengthValue(false);

	// The roll will vary damage between 2 and 3 (out of 10) for two units of identical strength

	int iDefenderDamageRatio = GetMaxHitPoints() - getDamage();
	int iDefenderDamage = /*200*/ GC.getAIR_STRIKE_SAME_STRENGTH_MIN_DEFENSE_DAMAGE() * iDefenderDamageRatio / GetMaxHitPoints();

	int iDefenderRoll = 0;
	if(bIncludeRand)
	{
		iDefenderRoll = /*200*/ GC.getGame().getJonRandNum(GC.getAIR_STRIKE_SAME_STRENGTH_POSSIBLE_EXTRA_DEFENSE_DAMAGE(), "Unit Air Strike Combat Damage");
		iDefenderRoll *= iDefenderDamageRatio;
		iDefenderRoll /= GetMaxHitPoints();
	}
	else
	{
		iDefenderRoll = /*200*/ GC.getAIR_STRIKE_SAME_STRENGTH_POSSIBLE_EXTRA_DEFENSE_DAMAGE();
		iDefenderRoll -= 1;	// Subtract 1 here, because this is the amount normally "lost" when doing a rand roll
		iDefenderRoll *= iDefenderDamageRatio;
		iDefenderRoll /= GetMaxHitPoints();
		iDefenderRoll /= 2;	// The divide by 2 is to provide the average damage
	}
	iDefenderDamage += iDefenderRoll;

	double fStrengthRatio = (double(iDefenderStrength) / iAttackerStrength);

	// In case our strength is less than the other guy's, we'll do things in reverse then make the ratio 1 over the result
	if (iAttackerStrength > iDefenderStrength)
	{
		fStrengthRatio = (double(iAttackerStrength) / iDefenderStrength);
	}

	fStrengthRatio = (fStrengthRatio + 3) / 4;
	fStrengthRatio = pow(fStrengthRatio, 4.0);
	fStrengthRatio = (fStrengthRatio + 1) / 2;

	if (iAttackerStrength > iDefenderStrength)
	{
		fStrengthRatio = 1 / fStrengthRatio;
	}

	iDefenderDamage = int(iDefenderDamage * fStrengthRatio);

	// Bring it back out of hundreds
	iDefenderDamage /= 100;

	// Always do at least 1 damage
	int iMinDamage = /*1*/ GC.getMIN_CITY_STRIKE_DAMAGE();
	if(iDefenderDamage < iMinDamage)
		iDefenderDamage = iMinDamage;

	return iDefenderDamage;
}

//	--------------------------------------------------------------------------------
void CvCity::DoNearbyEnemy()
{
	AI_PERF_FORMAT("City-AI-perf.csv", ("CvCity::DoNearbyEnemy, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), GetPlayer()->getCivilizationShortDescription(), getName().c_str()) );
	// Can't actually range strike
	if(!canRangeStrike())
		return;

	int iSearchRange = getBombardRange();
	
	if (iSearchRange == 0) return;

	CvPlot* pBestPlot = NULL;

	bool bFoundEnemy = false;

	for(int iDX = -(iSearchRange); iDX <= iSearchRange && !pBestPlot; iDX++)
	{
		for(int iDY = -(iSearchRange); iDY <= iSearchRange && !pBestPlot; iDY++)
		{
			CvPlot* pLoopPlot = plotXYWithRangeCheck(getX(), getY(), iDX, iDY, iSearchRange);

			if(pLoopPlot != NULL)
			{
				if(pLoopPlot->isVisibleEnemyUnit(getOwner()))
				{
					if(canRangeStrikeAt(pLoopPlot->getX(), pLoopPlot->getY()))
					{
						bFoundEnemy = true;

						// Notification
						CvNotifications* pNotifications = GET_PLAYER(getOwner()).GetNotifications();
						if(pNotifications)
						{
							Localization::String strText = Localization::Lookup("TXT_KEY_NOTIFICATION_CITY_CAN_SHOOT");
							strText << getNameKey();
							Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_CITY_CAN_SHOOT");
							strSummary << getNameKey();
							pNotifications->Add(NOTIFICATION_CITY_RANGE_ATTACK, strText.toUTF8(), strSummary.toUTF8(), getX(), getY(), GetID());
						}

						break;
					}
				}
			}
		}

		if(bFoundEnemy)
			break;
	}
}

//	--------------------------------------------------------------------------------
void CvCity::CheckForAchievementBuilding(BuildingTypes eBuilding)
{
	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if(pkBuildingInfo == NULL)
		return;

#if !defined(NO_ACHIEVEMENTS)
	const char* szBuildingTypeChar = pkBuildingInfo->GetType();
	CvString szBuilding = szBuildingTypeChar;

	if(szBuilding == "BUILDING_LONGHOUSE")
	{
		CvPlot* pLoopPlot;
		int nForests = 0;
#if defined(MOD_GLOBAL_CITY_WORKING)
		for(int iI = 0; iI < GetNumWorkablePlots(); iI++)
#else
		for(int iI = 0; iI < NUM_CITY_PLOTS; iI++)
#endif
		{
			pLoopPlot = plotCity(getX(), getY(), iI);

			if(pLoopPlot != NULL)
			{
				if(pLoopPlot->getOwner() == getOwner())
				{
					if(pLoopPlot->getFeatureType() == FEATURE_FOREST)
					{
						nForests++;
					}
				}
			}
		}
		if(nForests >=4)
		{
			gDLL->UnlockAchievement(ACHIEVEMENT_SPECIAL_LONGHOUSE);
		}
	}
	if(szBuilding == "BUILDING_FLOATING_GARDENS")
	{
		int iCityX = getX();
		int iCityY = getY();
		PlayerTypes eCityOwner = getOwner();
#if defined(MOD_GLOBAL_CITY_WORKING)
		for(int iI = 0; iI < GetNumWorkablePlots(); iI++)
#else
		for(int iI = 0; iI < NUM_CITY_PLOTS; iI++)
#endif
		{
			CvPlot* pLoopPlot = plotCity(iCityX, iCityY, iI);

			if(pLoopPlot != NULL && pLoopPlot->getOwner() == eCityOwner && pLoopPlot->isLake())
			{
				gDLL->UnlockAchievement(ACHIEVEMENT_SPECIAL_GARDENS);
				break;
			}
		}
	}
	//DLC_06 achievement: Build Statue of Zeus and Temple of Artemis in same city
	if(szBuilding == "BUILDING_STATUE_ZEUS" || szBuilding == "BUILDING_TEMPLE_ARTEMIS")
	{
		CvString szOtherWonder = "";
		if(szBuilding == "BUILDING_STATUE_ZEUS")
		{
			szOtherWonder = "BUILDING_TEMPLE_ARTEMIS";
		}
		else
		{
			szOtherWonder = "BUILDING_STATUE_ZEUS";
		}
		BuildingTypes eOtherWonder = (BuildingTypes)GC.getInfoTypeForString(szOtherWonder, true);
		if(eOtherWonder != NO_BUILDING)
		{
			PlayerTypes eCityOwner = getOwner();
			if(GetCityBuildings()->GetNumBuilding(eOtherWonder) > 0)
			{
				if(GetCityBuildings()->GetBuildingOriginalOwner(eOtherWonder) == eCityOwner)
				{
					gDLL->UnlockAchievement(ACHIEVEMENT_SPECIAL_ZEUS_AND_ARTEMIS);
				}
			}
		}
	}
#endif
}

//	--------------------------------------------------------------------------------
void CvCity::IncrementUnitStatCount(CvUnit* pUnit)
{
	CvString szUnitType = pUnit->getUnitInfo().GetType();

#if !defined(NO_ACHIEVEMENTS)
	if(szUnitType == "UNIT_WARRIOR")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_WARRIOR);
	}
	else if(szUnitType == "UNIT_SETTLER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_SETTLER);
	}
	else if(szUnitType == "UNIT_WORKER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_WORKER);
	}
	else if(szUnitType == "UNIT_WORKBOAT")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_WORKBOAT);
	}
	else if(szUnitType == "UNIT_GREAT_GENERAL")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_GREATGENERALS);
	}
	else if(szUnitType == "UNIT_SS_STASIS_CHAMBER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_SSSTASISCHAMBER);
	}
	else if(szUnitType == "UNIT_SS_ENGINE")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_SSENGINE);
	}
	else if(szUnitType == "UNIT_SS_COCKPIT")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_SSCOCKPIT);
	}
	else if(szUnitType == "UNIT_SS_BOOSTER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_SSBOOSTER);
	}
	else if(szUnitType == "UNIT_MISSILE_CRUISER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_MISSILECRUISER);
	}
	else if(szUnitType == "UNIT_NUCLEAR_SUBMARINE")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_NUCLEARSUBMARINE);
	}
	else if(szUnitType == "UNIT_CARRIER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_CARRIER);
	}
	else if(szUnitType == "UNIT_BATTLESHIP")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_BATTLESHIP);
	}
	else if(szUnitType == "UNIT_SUBMARINE")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_SUBMARINE);
	}
	else if(szUnitType == "UNIT_DESTROYER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_DESTROYER);
	}
	else if(szUnitType == "UNIT_IRONCLAD")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_IRONCLAD);
	}
	else if(szUnitType == "UNIT_FRIGATE")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_FRIGATE);
	}
	else if(szUnitType == "UNIT_ENGLISH_SHIPOFTHELINE")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_SHIPOFTHELINE);
	}
	else if(szUnitType == "UNIT_CARAVEL")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_CARAVEL);
	}
	else if(szUnitType == "UNIT_TRIREME")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_TRIREME);
	}
	else if(szUnitType == "UNIT_MECH")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_GIANTDEATHROBOT);
	}
	else if(szUnitType == "UNIT_NUCLEAR_MISSILE")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_NUCLEARMISSILE);
	}
	else if(szUnitType == "UNIT_STEALTH_BOMBER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_STEALTHBOMBER);
	}
	else if(szUnitType == "UNIT_JET_FIGHTER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_JETFIGHTER);
	}
	else if(szUnitType == "UNIT_GUIDED_MISSILE")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_GUIDEDMISSILE);
	}
	else if(szUnitType == "UNIT_MODERN_ARMOR")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_MODERNARMOR);
	}
	else if(szUnitType == "UNIT_HELICOPTER_GUNSHIP")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_HELICOPTERGUNSHIP);
	}
	else if(szUnitType == "UNIT_MOBILE_SAM")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_MOBILESAM);
	}
	else if(szUnitType == "UNIT_ROCKET_ARTILLERY")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_ROCKETARTILLERY);
	}
	else if(szUnitType == "UNIT_MECHANIZED_INFANTRY")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_MECHANIZEDINFANTRY);
	}
	else if(szUnitType == "UNIT_ATOMIC_BOMB")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_ATOMICBOMB);
	}
	else if(szUnitType == "UNIT_BOMBER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_BOMBER);
	}
	else if(szUnitType == "UNIT_AMERICAN_B17")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_B17);
	}
	else if(szUnitType == "UNIT_FIGHTER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_FIGHTER);
	}
	else if(szUnitType == "UNIT_JAPANESE_ZERO")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_ZERO);
	}
	else if(szUnitType == "UNIT_PARATROOPER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_PARATROOPER);
	}
	else if(szUnitType == "UNIT_TANK")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_TANK);
	}
	else if(szUnitType == "UNIT_GERMAN_PANZER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_PANZER);
	}
	else if(szUnitType == "UNIT_ARTILLERY")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_ARTILLERY);
	}
	else if(szUnitType == "UNIT_ANTI_AIRCRAFT_GUN")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_ANTIAIRCRAFTGUN);
	}
	else if(szUnitType == "UNIT_ANTI_TANK_GUN")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_ANTITANKGUN);
	}
	else if(szUnitType == "UNIT_INFANTRY")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_INFANTRY);
	}
	else if(szUnitType == "UNIT_FRENCH_FOREIGNLEGION")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_FOREIGNLEGION);
	}
	else if(szUnitType == "UNIT_CAVALRY")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_CAVALRY);
	}
	else if(szUnitType == "UNIT_RUSSIAN_COSSACK")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_COSSACK);
	}
	else if(szUnitType == "UNIT_RIFLEMAN")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_RIFLEMAN);
	}
	else if(szUnitType == "UNIT_LANCER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_LANCER);
	}
	else if(szUnitType == "UNIT_OTTOMAN_SIPAHI")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_SIPAHI);
	}
	else if(szUnitType == "UNIT_CANNON")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_CANNON);
	}
	else if(szUnitType == "UNIT_MUSKETMAN")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_MUSKETMAN);
	}
	else if(szUnitType == "UNIT_AMERICAN_MINUTEMAN")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_MINUTEMAN);
	}
	else if(szUnitType == "UNIT_FRENCH_MUSKETEER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_MUSKETEER);
	}
	else if(szUnitType == "UNIT_OTTOMAN_JANISSARY")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_JANISSARY);
	}
	else if(szUnitType == "UNIT_LONGSWORDSMAN")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_LONGSWORDSMAN);
	}
	else if(szUnitType == "UNIT_JAPANESE_SAMURAI")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_SAMURAI);
	}
	else if(szUnitType == "UNIT_TREBUCHET")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_TREBUCHET);
	}
	else if(szUnitType == "UNIT_KNIGHT")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_KNIGHT);
	}
	else if(szUnitType == "UNIT_SIAMESE_WARELEPHANT")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_NARESUANSELEPHANT);
	}
	else if(szUnitType == "UNIT_SONGHAI_MUSLIMCAVALRY")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_MANDEKALUCAVALRY);
	}
	else if(szUnitType == "UNIT_CROSSBOWMAN")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_CROSSBOWMAN);
	}
	else if(szUnitType == "UNIT_CHINESE_CHUKONU")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_CHUKONU);
	}
	else if(szUnitType == "UNIT_ARABIAN_CAMELARCHER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_CAMELARCHER);
	}
	else if(szUnitType == "UNIT_ENGLISH_LONGBOWMAN")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_LONGBOWMAN);
	}
	else if(szUnitType == "UNIT_PIKEMAN")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_PIKEMAN);
	}
	else if(szUnitType == "UNIT_GERMAN_LANDSKNECHT")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_LANDSKNECHT);
	}
	else if(szUnitType == "UNIT_CATAPULT")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_CATAPULT);
	}
	else if(szUnitType == "UNIT_ROMAN_BALLISTA")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_BALLISTA);
	}
	else if(szUnitType == "UNIT_HORSEMAN")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_HORSEMAN);
	}
	else if(szUnitType == "UNIT_GREEK_COMPANIONCAVALRY")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_COMPANIONCAVALRY);
	}
	else if(szUnitType == "UNIT_SWORDSMAN")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_SWORDSMAN);
	}
	else if(szUnitType == "UNIT_IROQUOIAN_MOHAWKWARRIOR")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_MOHAWKWARRIOR);
	}
	else if(szUnitType == "UNIT_ROMAN_LEGION")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_LEGION);
	}
	else if(szUnitType == "UNIT_CHARIOT_ARCHER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_CHARIOTARCHER);
	}
	else if(szUnitType == "UNIT_EGYPTIAN_WARCHARIOT")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_WARCHARIOT);
	}
	else if(szUnitType == "UNIT_INDIAN_WARELEPHANT")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_WARELEPHANT);
	}
	else if(szUnitType == "UNIT_SPEARMAN")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_SPEARMAN);
	}
	else if(szUnitType == "UNIT_GREEK_HOPLITE")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_HOPLITE);
	}
	else if(szUnitType == "UNIT_PERSIAN_IMMORTAL")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_IMMORTAL);
	}
	else if(szUnitType == "UNIT_ARCHER")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_ARCHER);
	}
	else if(szUnitType == "UNIT_SCOUT")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_SCOUT);
	}
	else if(szUnitType == "UNIT_AZTEC_JAGUAR")
	{
		gDLL->IncrementSteamStat(ESTEAMSTAT_JAGUAR);
	}
	else
	{
		OutputDebugString("\nNo stat for selected unit type.\n");
	}

	bool bAllUnitsUnlocked;

	bAllUnitsUnlocked = AreAllUnitsBuilt();
	if(bAllUnitsUnlocked)
	{
		gDLL->UnlockAchievement(ACHIEVEMENT_ALL_UNITS);
	}
#endif
}

//	--------------------------------------------------------------------------------
// Check to see if all the units have been built
bool CvCity::AreAllUnitsBuilt()
{
	int iI;
	int iUnitStatStart = 1;   //As they're defined on the backend
	int iUnitStatEnd = 79;
	int32 nStat = 0;

	for(iI = iUnitStatStart; iI < iUnitStatEnd; iI++)
	{
		if(gDLL->GetSteamStat((ESteamStat)iI, &nStat))
		{
			if(nStat <= 0)
			{
				return false;
			}
		}
	}
	//Whoops, one is out of order
	if(gDLL->GetSteamStat(ESTEAMSTAT_CAVALRY, &nStat))
	{
		if(nStat <=0)
		{
			return false;
		}
	}
	return true;

}

//	--------------------------------------------------------------------------------
/// Build a unit needed to fill in an army for an operation
bool CvCity::CommitToBuildingUnitForOperation()
{
	VALIDATE_OBJECT
	UnitTypes eBestUnit;
	UnitAITypes eUnitAI;

	CvPlayerAI& kPlayer = GET_PLAYER(getOwner());

	OperationSlot thisOperationSlot = kPlayer.PeekAtNextUnitToBuildForOperationSlot(getArea());

	if(thisOperationSlot.IsValid())
	{

		CvArmyAI* pThisArmy = kPlayer.getArmyAI(thisOperationSlot.m_iArmyID);

		if(pThisArmy)
		{
			// figure out the primary and secondary unit type to potentially build
			int iFormationIndex = pThisArmy->GetFormationIndex();
			CvMultiUnitFormationInfo* thisFormation = GC.getMultiUnitFormationInfo(iFormationIndex);
			if(thisFormation)
			{
				const CvFormationSlotEntry& slotEntry = thisFormation->getFormationSlotEntry(thisOperationSlot.m_iSlotID);

				eUnitAI = (UnitAITypes)slotEntry.m_primaryUnitType;
				eBestUnit = m_pCityStrategyAI->GetUnitProductionAI()->RecommendUnit(eUnitAI);
				if(eBestUnit == NO_UNIT)
				{
					eUnitAI = (UnitAITypes)slotEntry.m_secondaryUnitType;
					eBestUnit = m_pCityStrategyAI->GetUnitProductionAI()->RecommendUnit(eUnitAI);
				}

				if(eBestUnit != NO_UNIT)
				{
					// Always try to rush units for operational AI if possible
					pushOrder(ORDER_TRAIN, eBestUnit, eUnitAI, false, false, false, true /*bRush*/);
					OperationSlot thisOperationSlot2 = kPlayer.CityCommitToBuildUnitForOperationSlot(getArea(), getProductionTurnsLeft(), this);
					m_unitBeingBuiltForOperation = thisOperationSlot2;
					return true;
				}
			}
		}
	}
	return false;
}

//	--------------------------------------------------------------------------------
/// Which unit would we build if we are building one for an operation?
UnitTypes CvCity::GetUnitForOperation()
{
	VALIDATE_OBJECT
	UnitTypes eBestUnit;
	UnitAITypes eUnitAI;

	CvPlayerAI& kPlayer = GET_PLAYER(getOwner());

	OperationSlot thisOperationSlot = kPlayer.PeekAtNextUnitToBuildForOperationSlot(getArea());

	if(thisOperationSlot.IsValid())
	{
		CvArmyAI* pThisArmy = kPlayer.getArmyAI(thisOperationSlot.m_iArmyID);

		if(pThisArmy)
		{
			// figure out the primary and secondary unit type to potentially build
			int iFormationIndex = pThisArmy->GetFormationIndex();
			CvMultiUnitFormationInfo* thisFormation = GC.getMultiUnitFormationInfo(iFormationIndex);
			if(thisFormation)
			{
				const CvFormationSlotEntry& slotEntry = thisFormation->getFormationSlotEntry(thisOperationSlot.m_iSlotID);

				eUnitAI = (UnitAITypes)slotEntry.m_primaryUnitType;
				eBestUnit = m_pCityStrategyAI->GetUnitProductionAI()->RecommendUnit(eUnitAI);
				if(eBestUnit == NO_UNIT)
				{
					eUnitAI = (UnitAITypes)slotEntry.m_secondaryUnitType;
					eBestUnit = m_pCityStrategyAI->GetUnitProductionAI()->RecommendUnit(eUnitAI);
				}

				if(eBestUnit != NO_UNIT)
				{
					return eBestUnit;
				}
			}
		}
	}
	return NO_UNIT;
}

//	--------------------------------------------------------------------------------
/// What does a City shoot when attacking a Unit?
const char* CvCity::GetCityBombardEffectTag() const
{
	EraTypes eCityEra = GET_TEAM(getTeam()).GetCurrentEra();

	return GC.getEraInfo(eCityEra)->GetCityBombardEffectTag();
}

//	--------------------------------------------------------------------------------
uint CvCity::GetCityBombardEffectTagHash() const
{
	EraTypes eCityEra = GET_TEAM(getTeam()).GetCurrentEra();

	return GC.getEraInfo(eCityEra)->GetCityBombardEffectTagHash();
}

//	---------------------------------------------------------------------------
int CvCity::GetMaxHitPoints() const
{
	return GC.getMAX_CITY_HIT_POINTS() + m_iExtraHitPoints;
}

//	--------------------------------------------------------------------------------
int CvCity::GetExtraHitPoints() const
{
	return m_iExtraHitPoints;
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeExtraHitPoints(int iValue)
{
	if (iValue != 0)
	{
		m_iExtraHitPoints += iValue;
		FAssertMsg(m_iExtraHitPoints >= 0, "Trying to set ExtraHitPoints to a negative value");
		if (m_iExtraHitPoints < 0)
			m_iExtraHitPoints = 0;

		int iCurrentDamage = getDamage();
		if (iCurrentDamage > GetMaxHitPoints())
			setDamage(iCurrentDamage);		// Call setDamage, it will clamp the value.
	}
}

//	--------------------------------------------------------------------------------
const FAutoArchive& CvCity::getSyncArchive() const
{
	return m_syncArchive;
}

//	--------------------------------------------------------------------------------
FAutoArchive& CvCity::getSyncArchive()
{
	return m_syncArchive;
}

//	--------------------------------------------------------------------------------
std::string CvCity::debugDump(const FAutoVariableBase& /*var*/) const
{
	std::string result = "Game Turn : ";
	char gameTurnBuffer[8] = {0};
	int gameTurn = GC.getGame().getGameTurn();
	sprintf_s(gameTurnBuffer, "%d\0", gameTurn);
	result += gameTurnBuffer;
	return result;
}

//	--------------------------------------------------------------------------------
std::string CvCity::stackTraceRemark(const FAutoVariableBase& var) const
{
	std::string result = debugDump(var);
	if(&var == &m_aiBaseYieldRateFromTerrain)
	{
		result += std::string("\nlast yield used to update from terrain = ") + FSerialization::toString(s_lastYieldUsedToUpdateRateFromTerrain) + std::string("\n");
		result += std::string("change value used for update = ") + FSerialization::toString(s_changeYieldFromTerreain) + std::string("\n");
	}
	return result;
}

//	---------------------------------------------------------------------------
bool CvCity::IsBusy() const
{
	return getCombatUnit() != NULL;
}

//	---------------------------------------------------------------------------
const CvUnit* CvCity::getCombatUnit() const
{
	return ::getUnit(m_combatUnit);
}

//	---------------------------------------------------------------------------
CvUnit* CvCity::getCombatUnit()
{
	return ::getUnit(m_combatUnit);
}

//	---------------------------------------------------------------------------
void CvCity::setCombatUnit(CvUnit* pCombatUnit, bool /*bAttacking*/)
{
	if(pCombatUnit != NULL)
	{
		CvAssertMsg(getCombatUnit() == NULL , "Combat Unit is not expected to be assigned");
		CvAssertMsg(!(plot()->isCityFighting()), "(plot()->isCityFighting()) did not return false as expected");
		m_combatUnit = pCombatUnit->GetIDInfo();
	}
	else
	{
		clearCombat();
	}
}

//	----------------------------------------------------------------------------
void CvCity::clearCombat()
{
	if(getCombatUnit() != NULL)
	{
		CvAssertMsg(plot()->isCityFighting(), "plot()->isCityFighting is expected to be true");
		m_combatUnit.reset();
	}
}

//	----------------------------------------------------------------------------
//	Return true if the city is fighting with someone.	Equivalent to the CvUnit call.
bool CvCity::isFighting() const
{
	return getCombatUnit() != NULL;
}

#if defined(MOD_API_EXTENSIONS)
//	----------------------------------------------------------------------------
bool CvCity::HasBelief(BeliefTypes iBeliefType) const
{
	const ReligionTypes iReligion = GetCityReligions()->GetReligiousMajority();
	const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(iReligion, getOwner());

	return (pReligion && pReligion->m_Beliefs.HasBelief(iBeliefType));
}

bool CvCity::HasBuilding(BuildingTypes iBuildingType) const
{
	return (GetCityBuildings()->GetNumBuilding(iBuildingType) > 0);
}

bool CvCity::HasBuildingClass(BuildingClassTypes iBuildingClassType) const
{
	return HasBuilding((BuildingTypes) getCivilizationInfo().getCivilizationBuildings(iBuildingClassType));

}

int CvCity::GetNumBuildingClass(BuildingClassTypes iBuildingClassType) const
{
	return m_paiNumBuildingClasses[iBuildingClassType];
}
void CvCity::ChangeNumBuildingClass(BuildingClassTypes iBuildingClassType, int iValue)
{
	m_paiNumBuildingClasses[iBuildingClassType] += iValue;
}

bool CvCity::HasAnyWonder() const
{
	return (getNumWorldWonders() > 0);
}

bool CvCity::HasWonder(BuildingTypes iBuildingType) const
{
	return HasBuilding(iBuildingType);
}

bool CvCity::IsCivilization(CivilizationTypes iCivilizationType) const
{
	return (GET_PLAYER(getOwner()).getCivilizationType() == iCivilizationType);
}

bool CvCity::HasFeature(FeatureTypes iFeatureType) const
{
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		if (pLoopPlot->HasFeature(iFeatureType)) {
			return true;
		}
	}

	return false;
}

bool CvCity::HasWorkedFeature(FeatureTypes iFeatureType) const
{
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Not being worked by this city
		if (pLoopPlot->getWorkingCity() != this || !pLoopPlot->isBeingWorked()) {
			continue;
		}

		if (pLoopPlot->HasFeature(iFeatureType)) {
			return true;
		}
	}

	return false;
}

bool CvCity::HasAnyNaturalWonder() const
{
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		if (pLoopPlot->IsNaturalWonder(true)) {
			return true;
		}
	}

	return false;
}

bool CvCity::HasNaturalWonder(FeatureTypes iFeatureType) const
{
	return HasFeature(iFeatureType);
}

bool CvCity::HasImprovement(ImprovementTypes iImprovementType) const
{
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		if (pLoopPlot->HasImprovement(iImprovementType)) {
			return true;
		}
	}

	return false;
}

bool CvCity::HasWorkedImprovement(ImprovementTypes iImprovementType) const
{
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Not being worked by this city
		if (pLoopPlot->getWorkingCity() != this || !pLoopPlot->isBeingWorked()) {
			continue;
		}

		if (pLoopPlot->HasImprovement(iImprovementType)) {
			return true;
		}
	}

	return false;
}

bool CvCity::HasPlotType(PlotTypes iPlotType) const
{
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		if (pLoopPlot->HasPlotType(iPlotType)) {
			return true;
		}
	}

	return false;
}

bool CvCity::HasWorkedPlotType(PlotTypes iPlotType) const
{
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Not being worked by this city
		if (pLoopPlot->getWorkingCity() != this || !pLoopPlot->isBeingWorked()) {
			continue;
		}

		if (pLoopPlot->HasPlotType(iPlotType)) {
			return true;
		}
	}

	return false;
}

bool CvCity::HasAnyReligion() const
{
	return (GetCityReligions()->IsReligionInCity());
}

bool CvCity::HasReligion(ReligionTypes iReligionType) const
{
	return (HasAnyReligion() && GetCityReligions()->GetNumFollowers(iReligionType) > 0);
}

bool CvCity::HasResource(ResourceTypes iResourceType) const
{
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Team can't see the resource here
		if (pLoopPlot->getResourceType(getTeam()) != iResourceType) {
			continue;
		}

		// Resource not linked to this city
		// if (pLoopPlot->GetResourceLinkedCity() != this) {
		// 	continue;
		// }

		if (pLoopPlot->HasResource(iResourceType)) {
			return true;
		}
	}

	return false;
}

bool CvCity::HasWorkedResource(ResourceTypes iResourceType) const
{
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Team can't see the resource here
		if (pLoopPlot->getResourceType(getTeam()) != iResourceType) {
			continue;
		}

		// Not being worked by this city
		if (pLoopPlot->getWorkingCity() != this || !pLoopPlot->isBeingWorked()) {
			continue;
		}

		if (pLoopPlot->HasResource(iResourceType)) {
			return true;
		}
	}

	return false;
}

bool CvCity::IsConnectedToCapital() const
{
	return GET_PLAYER(getOwner()).IsCapitalConnectedToCity((CvCity*) this);
}

bool CvCity::IsConnectedTo(CvCity* pCity) const
{
	return GET_PLAYER(getOwner()).IsCityConnectedToCity((CvCity*) this, pCity);
}

bool CvCity::HasSpecialistSlot(SpecialistTypes iSpecialistType) const
{
	for (int iBuildingType = 0; iBuildingType < GC.getNumBuildingInfos(); iBuildingType++) {
		if (HasBuilding((BuildingTypes)iBuildingType)) {
			CvBuildingEntry* pkBuilding = GC.getBuildingInfo((BuildingTypes)iBuildingType);
			if (pkBuilding && pkBuilding->GetSpecialistType() == iSpecialistType && pkBuilding->GetSpecialistCount() > 0) {
				return true;
			}
		}
	}

	return false;
}

bool CvCity::HasSpecialist(SpecialistTypes iSpecialistType) const
{
	return (GetCityCitizens()->GetSpecialistCount(iSpecialistType) > 0);
}

bool CvCity::HasTerrain(TerrainTypes iTerrainType) const
{
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		if (pLoopPlot->HasTerrain(iTerrainType)) {
			return true;
		}
	}

	return false;
}

bool CvCity::HasWorkedTerrain(TerrainTypes iTerrainType) const
{
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Not being worked by this city
		if (pLoopPlot->getWorkingCity() != this || !pLoopPlot->isBeingWorked()) {
			continue;
		}

		if (pLoopPlot->HasTerrain(iTerrainType)) {
			return true;
		}
	}

	return false;
}

bool CvCity::HasAnyDomesticTradeRoute() const
{
	CvGameTrade* pTrade = GC.getGame().GetGameTrade();
	for (uint iTradeRoute = 0; iTradeRoute < pTrade->m_aTradeConnections.size(); iTradeRoute++) {
		if (pTrade->IsTradeRouteIndexEmpty(iTradeRoute)) {
			continue;
		}

		TradeConnection* pConnection = &(pTrade->m_aTradeConnections[iTradeRoute]);
		CvCity* pFromCity = GC.getMap().plot(pConnection->m_iOriginX, pConnection->m_iOriginY)->getPlotCity();
		CvCity* pToCity = GC.getMap().plot(pConnection->m_iDestX, pConnection->m_iDestY)->getPlotCity();

		if (pFromCity && isThis(*pFromCity) && pToCity->getOwner() == getOwner()) {
			return true;
		}
	}

	return false;
}

bool CvCity::HasAnyInternationalTradeRoute() const
{
	CvGameTrade* pTrade = GC.getGame().GetGameTrade();
	for (uint iTradeRoute = 0; iTradeRoute < pTrade->m_aTradeConnections.size(); iTradeRoute++) {
		if (pTrade->IsTradeRouteIndexEmpty(iTradeRoute)) {
			continue;
		}

		TradeConnection* pConnection = &(pTrade->m_aTradeConnections[iTradeRoute]);
		CvCity* pFromCity = GC.getMap().plot(pConnection->m_iOriginX, pConnection->m_iOriginY)->getPlotCity();
		CvCity* pToCity = GC.getMap().plot(pConnection->m_iDestX, pConnection->m_iDestY)->getPlotCity();

		if (pFromCity && isThis(*pFromCity) && pToCity->getOwner() != getOwner()) {
			return true;
		}
	}

	return false;
}

bool CvCity::HasTradeRouteToAnyCity() const
{
	CvGameTrade* pTrade = GC.getGame().GetGameTrade();
	for (uint iTradeRoute = 0; iTradeRoute < pTrade->m_aTradeConnections.size(); iTradeRoute++) {
		if (pTrade->IsTradeRouteIndexEmpty(iTradeRoute)) {
			continue;
		}

		TradeConnection* pConnection = &(pTrade->m_aTradeConnections[iTradeRoute]);
		CvCity* pFromCity = GC.getMap().plot(pConnection->m_iOriginX, pConnection->m_iOriginY)->getPlotCity();

		if (pFromCity && isThis(*pFromCity)) {
			return true;
		}
	}

	return false;
}

bool CvCity::HasTradeRouteTo(CvCity* pCity) const
{
	CvGameTrade* pTrade = GC.getGame().GetGameTrade();
	for (uint iTradeRoute = 0; iTradeRoute < pTrade->m_aTradeConnections.size(); iTradeRoute++) {
		if (pTrade->IsTradeRouteIndexEmpty(iTradeRoute)) {
			continue;
		}

		TradeConnection* pConnection = &(pTrade->m_aTradeConnections[iTradeRoute]);
		CvCity* pFromCity = GC.getMap().plot(pConnection->m_iOriginX, pConnection->m_iOriginY)->getPlotCity();
		CvCity* pToCity = GC.getMap().plot(pConnection->m_iDestX, pConnection->m_iDestY)->getPlotCity();

		if (pFromCity && isThis(*pFromCity) && pToCity && pToCity->isThis(*pCity)) {
			return true;
		}
	}

	return false;
}

bool CvCity::HasTradeRouteFromAnyCity() const
{
	CvGameTrade* pTrade = GC.getGame().GetGameTrade();
	for (uint iTradeRoute = 0; iTradeRoute < pTrade->m_aTradeConnections.size(); iTradeRoute++) {
		if (pTrade->IsTradeRouteIndexEmpty(iTradeRoute)) {
			continue;
		}

		TradeConnection* pConnection = &(pTrade->m_aTradeConnections[iTradeRoute]);
		CvCity* pToCity = GC.getMap().plot(pConnection->m_iDestX, pConnection->m_iDestY)->getPlotCity();

		if (pToCity && isThis(*pToCity)) {
			return true;
		}
	}

	return false;
}

bool CvCity::HasTradeRouteFrom(CvCity* pCity) const
{
	CvGameTrade* pTrade = GC.getGame().GetGameTrade();
	for (uint iTradeRoute = 0; iTradeRoute < pTrade->m_aTradeConnections.size(); iTradeRoute++) {
		if (pTrade->IsTradeRouteIndexEmpty(iTradeRoute)) {
			continue;
		}

		TradeConnection* pConnection = &(pTrade->m_aTradeConnections[iTradeRoute]);
		CvCity* pFromCity = GC.getMap().plot(pConnection->m_iOriginX, pConnection->m_iOriginY)->getPlotCity();
		CvCity* pToCity = GC.getMap().plot(pConnection->m_iDestX, pConnection->m_iDestY)->getPlotCity();

		if (pToCity && isThis(*pToCity) && pFromCity && pFromCity->isThis(*pCity)) {
			return true;
		}
	}

	return false;
}

bool CvCity::IsOnFeature(FeatureTypes iFeatureType) const
{
	return plot()->HasFeature(iFeatureType);
}

bool CvCity::IsAdjacentToFeature(FeatureTypes iFeatureType) const
{
	return plot()->IsAdjacentToFeature(iFeatureType);
}

bool CvCity::IsWithinDistanceOfFeature(FeatureTypes iFeatureType, int iDistance) const
{
	return plot()->IsWithinDistanceOfFeature(iFeatureType, iDistance);
}

bool CvCity::IsOnImprovement(ImprovementTypes iImprovementType) const
{
	return plot()->HasImprovement(iImprovementType);
}

bool CvCity::IsAdjacentToImprovement(ImprovementTypes iImprovementType) const
{
	return plot()->IsAdjacentToImprovement(iImprovementType);
}

bool CvCity::IsWithinDistanceOfImprovement(ImprovementTypes iImprovementType, int iDistance) const
{
	return plot()->IsWithinDistanceOfImprovement(iImprovementType, iDistance);
}

bool CvCity::IsOnPlotType(PlotTypes iPlotType) const
{
	return plot()->HasPlotType(iPlotType);
}

bool CvCity::IsAdjacentToPlotType(PlotTypes iPlotType) const
{
	return plot()->IsAdjacentToPlotType(iPlotType);
}

bool CvCity::IsWithinDistanceOfPlotType(PlotTypes iPlotType, int iDistance) const
{
	return plot()->IsWithinDistanceOfPlotType(iPlotType, iDistance);
}

bool CvCity::IsOnResource(ResourceTypes iResourceType) const
{
	return plot()->HasResource(iResourceType);
}

bool CvCity::IsAdjacentToResource(ResourceTypes iResourceType) const
{
	return plot()->IsAdjacentToResource(iResourceType);
}

bool CvCity::IsWithinDistanceOfResource(ResourceTypes iResourceType, int iDistance) const
{
	return plot()->IsWithinDistanceOfResource(iResourceType, iDistance);
}

bool CvCity::IsOnTerrain(TerrainTypes iTerrainType) const
{
	return plot()->HasTerrain(iTerrainType);
}

bool CvCity::IsAdjacentToTerrain(TerrainTypes iTerrainType) const
{
	return plot()->IsAdjacentToTerrain(iTerrainType);
}

bool CvCity::IsWithinDistanceOfTerrain(TerrainTypes iTerrainType, int iDistance) const
{
	return plot()->IsWithinDistanceOfTerrain(iTerrainType, iDistance);
}

int CvCity::CountFeature(FeatureTypes iFeatureType) const
{
	int iCount = 0;
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Not owned by this city
		if (pLoopPlot->getWorkingCity() != this) {
			continue;
		}

		if (pLoopPlot->HasFeature(iFeatureType)) {
			++iCount;
		}
	}
	
	return iCount;
}

int CvCity::CountWorkedFeature(FeatureTypes iFeatureType) const
{
	int iCount = 0;
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Not being worked by this city
		if (pLoopPlot->getWorkingCity() != this || !pLoopPlot->isBeingWorked()) {
			continue;
		}

		if (pLoopPlot->HasFeature(iFeatureType)) {
			++iCount;
		}
	}
	
	return iCount;
}

int CvCity::CountImprovement(ImprovementTypes iImprovementType) const
{
	int iCount = 0;
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Not owned by this city
		if (pLoopPlot->getWorkingCity() != this) {
			continue;
		}

		if (pLoopPlot->HasImprovement(iImprovementType)) {
			++iCount;
		}
	}
	
	return iCount;
}

int CvCity::CountUnPillagedImprovement(ImprovementTypes iImprovementType) const
{
	int iCount = 0;
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Not owned by this city
		if (pLoopPlot->getWorkingCity() != this) {
			continue;
		}

		if (pLoopPlot->HasImprovement(iImprovementType) && !pLoopPlot->IsImprovementPillaged() ) {
			++iCount;
		}
	}

	return iCount;
}


int CvCity::CountWorkedImprovement(ImprovementTypes iImprovementType) const
{
	int iCount = 0;
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Not being worked by this city
		if (pLoopPlot->getWorkingCity() != this || !pLoopPlot->isBeingWorked()) {
			continue;
		}

		if (pLoopPlot->HasImprovement(iImprovementType) && !pLoopPlot->IsImprovementPillaged()) {
			++iCount;
		}
	}
	
	return iCount;
}

int CvCity::CountPlotType(PlotTypes iPlotType) const
{
	int iCount = 0;
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Not owned by this city
		if (pLoopPlot->getWorkingCity() != this) {
			continue;
		}

		if (pLoopPlot->HasPlotType(iPlotType)) {
			++iCount;
		}
	}
	
	return iCount;
}

int CvCity::CountWorkedPlotType(PlotTypes iPlotType) const
{
	int iCount = 0;
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Not being worked by this city
		if (pLoopPlot->getWorkingCity() != this || !pLoopPlot->isBeingWorked()) {
			continue;
		}

		if (pLoopPlot->HasPlotType(iPlotType)) {
			++iCount;
		}
	}
	
	return iCount;
}

int CvCity::CountResource(ResourceTypes iResourceType) const
{
	int iCount = 0;
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Not owned by this city
		if (pLoopPlot->getWorkingCity() != this) {
			continue;
		}

		if (pLoopPlot->HasResource(iResourceType)) {
			++iCount;
		}
	}
	
	return iCount;
}

int CvCity::CountWorkedResource(ResourceTypes iResourceType) const
{
	int iCount = 0;
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Not being worked by this city
		if (pLoopPlot->getWorkingCity() != this || !pLoopPlot->isBeingWorked()) {
			continue;
		}

		if (pLoopPlot->HasResource(iResourceType)) {
			++iCount;
		}
	}
	
	return iCount;
}

int CvCity::CountTerrain(TerrainTypes iTerrainType) const
{
	int iCount = 0;
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Not owned by this city
		if (pLoopPlot->getWorkingCity() != this) {
			continue;
		}

		if (pLoopPlot->HasTerrain(iTerrainType)) {
			++iCount;
		}
	}
	
	return iCount;
}

int CvCity::CountWorkedTerrain(TerrainTypes iTerrainType) const
{
	int iCount = 0;
	int iX = getX(); int iY = getY(); int iOwner = getOwner();

#if defined(MOD_GLOBAL_CITY_WORKING)
	for (int iCityPlotLoop = 0; iCityPlotLoop < GetNumWorkablePlots(); iCityPlotLoop++)
#else
	for (int iCityPlotLoop = 0; iCityPlotLoop < NUM_CITY_PLOTS; iCityPlotLoop++)
#endif
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iCityPlotLoop);

		// Invalid plot or not owned by this player
		if (pLoopPlot == NULL || pLoopPlot->getOwner() != iOwner) {
			continue;
		}

		// Not being worked by this city
		if (pLoopPlot->getWorkingCity() != this || !pLoopPlot->isBeingWorked()) {
			continue;
		}

		if (pLoopPlot->HasTerrain(iTerrainType)) {
			++iCount;
		}
	}

	return iCount;
}
#endif
//-------------------------------------------------------------------------------
void CvCity::DoReligionFounderChange()
{
	if(!MOD_GLOBAL_HOLY_CITY_FOUNDER_CHANGE) return;
	if(IsResistance() || IsRazing()) return;

	CvPlayerAI &pPlayer = GET_PLAYER(getOwner());
	if(pPlayer.GetReligions()->HasCreatedReligion() || !pPlayer.isMajorCiv()) return;

	ReligionTypes eMajorityReligion = GetCityReligions()->GetReligiousMajority();
	if(!GetCityReligions()->IsHolyCityForReligion(eMajorityReligion)) return;
	
	CvGameReligions* pGameReligions = GC.getGame().GetGameReligions();
	const CvReligion* pkReligion = pGameReligions->GetReligion(eMajorityReligion, NO_PLAYER);
	bool bHasAlter = false;
	// Regained
	if(pkReligion->m_eOriginalFounder == getOwner())
	{
		bHasAlter = true;
	}
	else
	{
		int iNumCityThisReligion = 0;
		int iNumFollowers = 0;
		int iLoop = 0;
		for(CvCity* pLoopCity = pPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = pPlayer.nextCity(&iLoop))
		{
			iNumFollowers += pLoopCity->GetCityReligions()->GetNumFollowers(eMajorityReligion);
			if (pLoopCity->GetCityReligions()->GetReligiousMajority() != eMajorityReligion) continue;
			iNumCityThisReligion++;
		}
		if (iNumCityThisReligion * 4 / 3 < pPlayer.getNumCities()) return;
		if (iNumFollowers * 2  < pGameReligions->GetNumFollowers(eMajorityReligion)) return;
		bHasAlter = true;
	}

	if(!bHasAlter) return;

	PlayerTypes eOldFounder = pkReligion->m_eFounder;
	pGameReligions->SetFounder(eMajorityReligion, getOwner());
	pGameReligions->UpdateAllCitiesThisReligion(eMajorityReligion);
	CvString strSummary = GetLocalizedText("TXT_KEY_HOLY_CITY_OCCUPIED_ALTER");
	CvString strBuffer = GetLocalizedText("TXT_KEY_HOLY_CITY_OCCUPIED_ALTER_TT", pPlayer.getCivilizationShortDescriptionKey(), pkReligion->GetName(), getNameKey());
	for(int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		PlayerTypes ePlayer = (PlayerTypes)iI;
		if(ePlayer != GC.getGame().getActivePlayer()) continue;
		if(!GET_PLAYER(ePlayer).isAlive()) continue;
		if(ePlayer == getOwner() || GET_TEAM(pPlayer.getTeam()).isHasMet(GET_PLAYER(ePlayer).getTeam()))
		{
			CvNotifications *pNotifications = GET_PLAYER(ePlayer).GetNotifications();
			if(pNotifications) pNotifications->Add(NOTIFICATION_RELIGION_ENHANCED, strBuffer, strSummary, -1, -1, eMajorityReligion, -1);
		}
	}
	GAMEEVENTINVOKE_HOOK(GAMEEVENT_ReligionFounderChanged, eOldFounder, GetID(), getOwner(), eMajorityReligion, pkReligion->m_eOriginalFounder == getOwner());
}
//-------------------------------------------------------------------------------
#ifdef MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD
bool CvCity::HasYieldFromOtherYield() const
{
	return m_bHasYieldFromOtherYield;
}
#endif

#if defined(MOD_NUCLEAR_WINTER_FOR_SP)
bool CvCity::IsNoNuclearWinterLocal() const
{
	return m_iNumNoNuclearWinterLocal > 0;
}
void CvCity::ChangeNumNoNuclearWinterLocal(int iChange)
{
	m_iNumNoNuclearWinterLocal += iChange;
}
#endif
//-------------------------------------------------------------------------------
#if defined(MOD_TROOPS_AND_CROPS_FOR_SP)
bool CvCity::HasEnableCrops() const  
{  
    return m_iCityEnableCrops > 0;  
}
void CvCity::ChangeNumEnableCrops(int iChange)
{
	m_iCityEnableCrops += iChange;
}
bool CvCity::HasEnableArmee() const  
{  
    return m_iCityEnableArmee > 0;  
}
void CvCity::ChangeNumEnableArmee(int iChange)
{
	m_iCityEnableArmee += iChange;
}
#endif
//-------------------------------------------------------------------------------
#if defined(MOD_INTERNATIONAL_IMMIGRATION_FOR_SP)
bool CvCity::IsCanDoImmigration() const
{
	return m_bCanDoImmigration;
}
void CvCity::SetCanDoImmigration(bool bValue)
{
	m_bCanDoImmigration = bValue;
}
bool CvCity::CanImmigrantIn() const
{
	return !IsPuppet() && !IsRazing() && !IsResistance() && !GetCityCitizens()->IsForcedAvoidGrowth()
		&& IsCanDoImmigration() && CanGrowNormally() && (CanScaleImmigrantIn() || CanAllScaleImmigrantIn())
		&& GetCityCitizens()->GetSpecialistCount((SpecialistTypes)GC.getInfoTypeForString("SPECIALIST_CITIZEN")) <= 0;
}
bool CvCity::CanImmigrantOut() const
{
	return IsCanDoImmigration() && CanScaleImmigrantOut();
}
bool CvCity::CanAllScaleImmigrantIn() const
{
	return m_iNumAllScaleImmigrantIn > 0;
}
void CvCity::ChangeNumAllScaleImmigrantIn(int iChange)
{
	m_iNumAllScaleImmigrantIn += iChange;
}
#endif
#ifdef MOD_GLOBAL_CITY_SCALES
void CvCity::SetScale(CityScaleTypes eNewScale)
{
	if (!MOD_GLOBAL_CITY_SCALES) return;
	CityScaleTypes eOldScale = GetScale();

	if (eOldScale == eNewScale)
	{
		return;
	}

	m_eCityScale = eNewScale;
	UpdateScaleBuildings();

	if (!CanGrowNormally())
	{
		setFood(0);
		setFoodKept(0);
	}

#ifdef MOD_EVENTS_CITY_SCALES
	if (MOD_EVENTS_CITY_SCALES)
		GAMEEVENTINVOKE_HOOK(GAMEEVENT_OnCityScaleChange, getOwner(), GetID(), eOldScale, eNewScale);
#endif
}

void CvCity::UpdateScaleBuildings()
{
	CityScaleTypes eScale = GetScale();
	CvCityScaleEntry *pNewScale = GC.getCityScaleInfo(eScale);
	auto &pAllScales = GC.getCityScaleInfo();
	CvPlayerAI &pkOwner = GET_PLAYER(getOwner());
	std::tr1::unordered_map<BuildingClassTypes, int> buildingClassNum;

	// clear all scale buildings
	for (auto *pScale : pAllScales)
	{
		if (pScale == nullptr)
			continue;

		for (auto& vFreeBuildings : pScale->GetFreeBuildingClassInfo())
		{
			buildingClassNum[vFreeBuildings.m_eBuildingClass] = 0;
		}

		for (auto& vFreeBuildings : pScale->GetFreeBuildingClassInfoFromPolicies())
		{
			buildingClassNum[vFreeBuildings.m_eBuildingClass] = 0;
		}

		for (auto& vFreeBuildings : pScale->GetFreeBuildingClassInfoFromTraits())
		{
			buildingClassNum[vFreeBuildings.m_eBuildingClass] = 0;
		}
	}

	// add new scale buildings
	if (pNewScale)
	{
		for (auto& vFreeBuildings : pNewScale->GetFreeBuildingClassInfo())
		{
			buildingClassNum[vFreeBuildings.m_eBuildingClass] += vFreeBuildings.m_iNum;
		}
		for (auto& vFreeBuildings : pNewScale->GetFreeBuildingClassInfoFromPolicies())
		{
			if (pkOwner.HasPolicy(vFreeBuildings.m_eRequiredPolicy) && !pkOwner.GetPlayerPolicies()->IsPolicyBlocked(vFreeBuildings.m_eRequiredPolicy))
			{
				buildingClassNum[vFreeBuildings.m_eBuildingClass] += vFreeBuildings.m_iNum;
			}
		}
		for (auto& vFreeBuildings : pNewScale->GetFreeBuildingClassInfoFromTraits())
		{
			if (pkOwner.isMajorCiv() && pkOwner.GetPlayerTraits()->HasTrait(vFreeBuildings.m_eRequiredTrait))
			{
				buildingClassNum[vFreeBuildings.m_eBuildingClass] += vFreeBuildings.m_iNum;
			}
		}
	}

	for (auto iter = buildingClassNum.begin(); iter != buildingClassNum.end(); iter++)
	{
		BuildingTypes eBuilding = pkOwner.GetCivBuilding(iter->first);
		GetCityBuildings()->SetNumRealBuilding(eBuilding, iter->second);
	}
}

bool CvCity::CanGrowNormally() const
{
	auto* info = GetScaleInfo();
	if (info == nullptr || !info->NeedGrowthBuilding())
	{
		return true;
	}

	for (auto& it = info->GetBuildingsSupportGrowth().begin(); it != info->GetBuildingsSupportGrowth().end(); it++)
	{
		auto eBuilding = *it;
		if (HasBuilding(eBuilding))
		{
			return true;
		}
	}
	return false;
}

bool CvCity::CanScaleImmigrantIn() const
{
	auto* info = GetScaleInfo();
	if (info == nullptr) return true;
	return info->CanImmigrantIn();
}
bool CvCity::CanScaleImmigrantOut() const
{
	auto* info = GetScaleInfo();
	if (info == nullptr) return true;
	return info->CanImmigrantOut();
}
#endif

#ifdef MOD_PROMOTION_CITY_DESTROYER
int CvCity::GetSiegeKillCitizensModifier() const
{
	return m_iSiegeKillCitizensModifier;
}
void CvCity::ChangeSiegeKillCitizensModifier(int iChange)
{
	m_iSiegeKillCitizensModifier += iChange;
}
#endif

#ifdef MOD_GLOBAL_CORRUPTION
int CvCity::GetCorruptionScore() const
{
	return m_iCachedCorruptionScore;
}

CorruptionLevelTypes CvCity::GetCorruptionLevel() const
{
	return m_eCachedCorruptionLevel;
}

void CvCity::UpdateCorruption()
{
	CvPlayerAI& owner = GET_PLAYER(getOwner());
	if (!owner.EnableCorruption())
	{
		return;
	}

	const int oldScore = m_iCachedCorruptionScore;
	const CorruptionLevelTypes eOldLevel = m_eCachedCorruptionLevel;
	auto* pOldLevel = GC.getCorruptionLevelInfo(eOldLevel);

	const int newScore = CalculateTotalCorruptionScore();
	CvCorruptionLevel* pNewLevel = nullptr;
	if (isCapital())
	{
		pNewLevel = GC.getCapitalCityCorruptionLevel();
	}
	else if (IsPuppet())
	{
		pNewLevel = GC.getPuppetCityCorruptionLevel();
	}
	else
	{
		pNewLevel = DecideCorruptionLevelForNormalCity(newScore);
	}

	m_iCachedCorruptionScore = newScore;
	m_eCachedCorruptionLevel = pNewLevel ? static_cast<CorruptionLevelTypes>(pNewLevel->GetID()) : INVALID_CORRUPTION;

	if (pNewLevel == pOldLevel)
	{
		return;
	}

	bool haveOldPublicSecurityBuilding = false;
	if (pOldLevel)
	{
		auto cityHall = owner.GetCivBuilding(pOldLevel->GetCityHallBuildingClass());
		if (cityHall != NO_BUILDING)
		{
			GetCityBuildings()->SetNumRealBuilding(cityHall, 0);
		}
		auto publicSecurity = owner.GetCivBuilding(pOldLevel->GetPublicSecurityBuildingClass());
		if (publicSecurity != NO_BUILDING)
		{
			haveOldPublicSecurityBuilding = GetCityBuildings()->GetNumRealBuilding(publicSecurity) > 0;
			GetCityBuildings()->SetNumRealBuilding(publicSecurity, 0);
		}
	}

	if (pNewLevel)
	{
		auto cityHall = owner.GetCivBuilding(pNewLevel->GetCityHallBuildingClass());
		if (cityHall != NO_BUILDING)
		{
			GetCityBuildings()->SetNumRealBuilding(cityHall, 1);
		}

		auto publicSecurity = owner.GetCivBuilding(pNewLevel->GetPublicSecurityBuildingClass());
		if (publicSecurity != NO_BUILDING && haveOldPublicSecurityBuilding 
				&& pOldLevel->GetScoreLowerBoundBase() > pNewLevel->GetScoreLowerBoundBase())
		{
			GetCityBuildings()->SetNumRealBuilding(publicSecurity, 1);
		}
	}
}

int CvCity::CalculateCorruptionScoreFromResource() const
{
	auto resource = plot() ? plot()->getResourceType() : NO_RESOURCE;
	auto* resourceInfo = GC.getResourceInfo(resource);
	return resourceInfo != nullptr ? resourceInfo->GetCorruptionScoreChange() : 0;
}

int CvCity::CalculateCorruptionScoreFromTrait() const
{
	CvPlayerAI& owner = GET_PLAYER(getOwner());
	int TraitBounsTotal = 0;
	if(plot())
	{
		TraitBounsTotal += plot()->isRiver() ? owner.GetPlayerTraits()->GetRiverCorruptionScoreChange() : 0;
	}
	return TraitBounsTotal;
}

int CvCity::CalculateTotalCorruptionScore() const
{
	CvPlayerAI& owner = GET_PLAYER(getOwner());

	// Base Score
	int score = 0;
	score += CalculateCorruptionScoreFromDistance();
	score += CalculateCorruptionScoreFromCoastalBonus();
	score += CalculateCorruptionScoreFromResource();
	score += GetCorruptionScoreChangeFromBuilding();
	score += CalculateCorruptionScoreFromTrait();
	score = std::max(0, score);

	// Score Modifier
	int modifier = 100;
	modifier += CalculateCorruptionScoreModifierFromSpy();
	modifier += CalculateCorruptionScoreModifierFromTrait();
	modifier += owner.GetCorruptionScoreModifierFromPolicy();
	modifier = std::max(0, modifier);

	score = score * modifier / 100;
	score = std::max(0, score);
	return score;
}

int CvCity::GetCorruptionScoreModifierFromPolicy() const
{
	CvPlayerAI &owner = GET_PLAYER(getOwner());
	return owner.GetCorruptionScoreModifierFromPolicy();
}

int CvCity::CalculateCorruptionScoreFromDistance() const
{
	CvPlayerAI& owner = GET_PLAYER(getOwner());
	CvCity* capital = owner.getCapitalCity();
	if (capital == nullptr || capital == this)
	{
		return 0;
	}

	int score = plotDistance(capital->plot()->getX(), capital->plot()->getY(), this->plot()->getX(), this->plot()->getY()) * GC.getCORRUPTION_SCORE_PER_DISTANCE(); // calculate by major capital
	for (const int cityId : owner.GetSecondCapitals()) // calculate by second capitals
	{
		CvCity* pSecondCapital = owner.getCity(cityId);
		if (pSecondCapital == nullptr)
		{
			continue;
		}

		int scoreBySecondCapital = plotDistance(pSecondCapital->plot()->getX(), pSecondCapital->plot()->getY(), this->plot()->getX(), this->plot()->getY()) * GC.getCORRUPTION_SCORE_PER_DISTANCE();
		scoreBySecondCapital += pSecondCapital->GetSecondCapitalsExtraScore();
		score = std::min(scoreBySecondCapital, score);
	}

	return score;
}

int CvCity::CalculateCorruptionScoreFromCoastalBonus() const
{
	CvPlayerAI& owner = GET_PLAYER(getOwner());
	CvCity* capital = owner.getCapitalCity();
	if (capital == nullptr || capital == this)
	{
		return 0;
	}
	return capital->isCoastal() && plot()->isCoastalArea() ? GC.getCORRUPTION_SCORE_COASTAL_BONUS() : 0;
}

int CvCity::CalculateCorruptionScoreModifierFromSpy() const
{
	CvPlayerAI& owner = GET_PLAYER(getOwner());
	auto* espionage = owner.GetEspionage();
	if (espionage == nullptr)
	{
		return 0;
	}

	int spyIndex = espionage->GetSpyIndexInCity(const_cast<CvCity*>(this));
	if (spyIndex == -1)
	{
		return 0;
	}

	int rank = espionage->m_aSpyList[spyIndex].m_eRank + 1;
	if (rank == 0) {
		return 0;
	}
	if (rank == 1) {
		return -33;
	}
	if (rank == 2) {
		return -67;
	}
	return -100;
}

int CvCity::CalculateCorruptionScoreModifierFromTrait()  const
{
	if(plot() != NULL)
	{
		return plot()->CalculateCorruptionScoreModifierFromTrait(getOwner());
	}
	return 0;
}

bool CvCity::IsCorruptionLevelReduceByOne() const
{
	CvPlayerAI& owner = GET_PLAYER(getOwner());
	return owner.IsCorruptionLevelReduceByOne() || owner.GetPlayerTraits()->GetCorruptionLevelReduceByOne();
}

bool CvCity::IsSecondCapital() const
{
	return m_bIsSecondCapital;
}
void CvCity::SetSecondCapital(bool value)
{
	m_bIsSecondCapital = value;
}

int CvCity::GetMaxCorruptionLevel() const
{
	CvPlayerAI& owner = GET_PLAYER(getOwner());
	return owner.GetPlayerTraits()->GetMaxCorruptionLevel();
}

CvCorruptionLevel* CvCity::DecideCorruptionLevelForNormalCity(const int score) const
{
	CvPlayerAI& owner = GET_PLAYER(getOwner());
	CvCorruptionLevel* result = nullptr;
	int resultIndex = -1;
	for (int index = 0; index < GC.getOrderedNormalCityCorruptionLevels().size(); index++)
	{
		auto* level = GC.getOrderedNormalCityCorruptionLevels()[index];
		if (level->GetScoreLowerBound(GC.getMap().getGridWidth(), GC.getMap().getGridHeight()) > score)
		{
			break;
		}
		result = level;
		resultIndex = index;
	}

	if (resultIndex > 1)
	{
		if (IsCorruptionLevelReduceByOne())
		{
			resultIndex--;
			result = GC.getOrderedNormalCityCorruptionLevels()[resultIndex];
		}
	}

	if (GetCorruptionLevelChangeFromBuilding() != 0)
	{
		bool oldNoneZero = resultIndex > 0;
		resultIndex = resultIndex + GetCorruptionLevelChangeFromBuilding();
		if (oldNoneZero)
		{
			resultIndex = std::max(1, resultIndex);
		}
		else
		{
			resultIndex = std::max(0, resultIndex);
		}

		if (resultIndex > GC.getOrderedNormalCityCorruptionLevels().size() - 1)
		{
			resultIndex = GC.getOrderedNormalCityCorruptionLevels().size() - 1;
		}
		result = GC.getOrderedNormalCityCorruptionLevels()[resultIndex];
	}

	if (GetMaxCorruptionLevel() >= 0 && resultIndex > GetMaxCorruptionLevel())
	{
		resultIndex = GetMaxCorruptionLevel();
		result = GC.getOrderedNormalCityCorruptionLevels()[resultIndex];
	}

	return result;
}

int CvCity::GetCorruptionScoreChangeFromBuilding() const
{
	return m_iCorruptionScoreChangeFromBuilding;
}

void CvCity::ChangeCorruptionScoreChangeFromBuilding(int value)
{
	m_iCorruptionScoreChangeFromBuilding += value;
}

int CvCity::GetCorruptionLevelChangeFromBuilding() const
{
	return m_iCorruptionLevelChangeFromBuilding;
}

void CvCity::ChangeCorruptionLevelChangeFromBuilding(int value)
{
	m_iCorruptionLevelChangeFromBuilding += value;
}
#endif

int CvCity::getLocalBuildingClassYield(BuildingClassTypes eBuilding, YieldTypes eYield)	const
{
	return ModifierLookup(m_yieldChanges[eYield].forLocalBuilding, eBuilding);
}

//	--------------------------------------------------------------------------------
void CvCity::changeLocalBuildingClassYield(BuildingClassTypes eBuilding, YieldTypes eYield, int iChange)
{
	SCityExtraYields& y = m_yieldChanges[eYield];
	ModifierUpdateInsertRemove(y.forLocalBuilding, eBuilding, iChange, true);
}

#if defined(MOD_API_UNIFIED_YIELDS_MORE)

int CvCity::GetAdditionalFood() const
{
	VALIDATE_OBJECT
	return m_iAdditionalFood;
}
void CvCity::SetAdditionalFood(int iValue)
{
	VALIDATE_OBJECT
	m_iAdditionalFood = iValue;
}

bool CvCity::IsColony() const
{
	return m_bIsColony;
}
void CvCity::SetColony(bool bValue)
{
	if (m_bIsColony != bValue)
	{
		m_bIsColony = bValue;
	}
}

int CvCity::GetOrganizedCrime() const
{
	return m_iOrganizedCrime;
}
void CvCity::SetOrganizedCrime(int iValue)
{
	if (m_iOrganizedCrime != iValue)
	{
		m_iOrganizedCrime = iValue;
	}
}
bool CvCity::HasOrganizedCrime()
{
	return(m_iOrganizedCrime >= 0);
}

void CvCity::ChangeResistanceCounter(int iValue)
{
	if (iValue != 0)
	{
		m_iResistanceCounter += iValue;
	}
}
void CvCity::SetResistanceCounter(int iValue)
{
	if (iValue != m_iResistanceCounter)
	{
		m_iResistanceCounter = iValue;
	}
}
int CvCity::GetResistanceCounter() const
{
	return m_iResistanceCounter;
}

void CvCity::ChangePlagueCounter(int iValue)
{
	if (iValue != 0)
	{
		m_iPlagueCounter += iValue;
	}
}
void CvCity::SetPlagueCounter(int iValue)
{
	if (iValue != m_iPlagueCounter)
	{
		m_iPlagueCounter = iValue;
	}
}
int CvCity::GetPlagueCounter() const
{
	return m_iPlagueCounter;
}

int CvCity::GetPlagueTurns() const
{
	return m_iPlagueTurns;
}
void CvCity::ChangePlagueTurns(int iValue) //Set in city::doturn
{
	if (iValue != 0)
	{
		m_iPlagueTurns += iValue;
	}
}
void CvCity::SetPlagueTurns(int iValue)
{
	if (iValue != m_iPlagueTurns)
	{
		m_iPlagueTurns = iValue;
	}
}

int CvCity::GetPlagueType() const
{
	return m_iPlagueType;
}
void CvCity::SetPlagueType(int iValue)
{
	if (iValue != m_iPlagueTurns)
	{
		m_iPlagueType = iValue;
	}
}
bool CvCity::HasPlague()
{
	return(m_iPlagueType >= 0);
}

void CvCity::ChangeLoyaltyCounter(int iValue)
{
	if (iValue != 0)
	{
		m_iLoyaltyCounter += iValue;
	}
}
void CvCity::SetLoyaltyCounter(int iValue)
{
	if (iValue != m_iLoyaltyCounter)
	{
		m_iLoyaltyCounter = iValue;
	}
}
int CvCity::GetLoyaltyCounter() const
{
	return m_iLoyaltyCounter;
}

void CvCity::ChangeDisloyaltyCounter(int iValue)
{
	if (iValue != 0)
	{
		m_iDisloyaltyCounter += iValue;
	}
}
void CvCity::SetDisloyaltyCounter(int iValue)
{
	if (iValue != m_iDisloyaltyCounter)
	{
		m_iDisloyaltyCounter = iValue;
	}
}
int CvCity::GetDisloyaltyCounter() const
{
	return m_iDisloyaltyCounter;
}

int CvCity::GetLoyaltyState() const
{
	return m_iLoyaltyStateType;
}
void CvCity::SetLoyaltyState(int iLoyalty)
{
	if (iLoyalty != m_iLoyaltyStateType)
	{
		int iOldLoyalty = m_iLoyaltyStateType;
		GAMEEVENTINVOKE_HOOK(GAMEEVENT_LoyaltyStateChanged, getOwner(), GetID(), iOldLoyalty, iLoyalty);
		m_iLoyaltyStateType = iLoyalty;
	}
}

void CvCity::SetYieldModifierFromHealth(YieldTypes eYield, int iValue)
{
	if (GetYieldModifierFromHealth(eYield) != iValue)
	{
		m_aiYieldModifierFromHealth[eYield] = iValue;
		UpdateCityYields(eYield);
	}
}
int CvCity::GetYieldModifierFromHealth(YieldTypes eYield) const
{
	VALIDATE_OBJECT
	return m_aiYieldModifierFromHealth[eYield];
}

void CvCity::SetYieldModifierFromCrime(YieldTypes eYield, int iValue)
{
	if (GetYieldModifierFromCrime(eYield) != iValue)
	{
		m_aiYieldModifierFromCrime[eYield] = iValue;
		UpdateCityYields(eYield);
	}
}
int CvCity::GetYieldModifierFromCrime(YieldTypes eYield) const
{
	VALIDATE_OBJECT
	return m_aiYieldModifierFromCrime[eYield];
}

void CvCity::SetYieldFromHappiness(YieldTypes eYield, int iValue)
{
	if (GetYieldFromHappiness(eYield) != iValue)
	{
		m_aiYieldFromHappiness[eYield] = iValue;
		UpdateCityYields(eYield);
	}
}
int CvCity::GetYieldFromHappiness(YieldTypes eYield) const
{
	VALIDATE_OBJECT
	return m_aiYieldFromHappiness[eYield];
}


void CvCity::SetYieldFromHealth(YieldTypes eYield, int iValue)
{
	if (GetYieldFromHealth(eYield) != iValue)
	{
		m_aiYieldFromHealth[eYield] = iValue;
		UpdateCityYields(eYield);
	}
}
int CvCity::GetYieldFromHealth(YieldTypes eYield) const
{
	VALIDATE_OBJECT
	return m_aiYieldFromHealth[eYield];
}
void CvCity::SetYieldFromCrime(YieldTypes eYield, int iValue)
{
	if (GetYieldFromCrime(eYield) != iValue)
	{
		m_aiYieldFromCrime[eYield] = iValue;
		UpdateCityYields(eYield);
	}
}
int CvCity::GetYieldFromCrime(YieldTypes eYield) const
{
	VALIDATE_OBJECT
	CvAssertMsg(eYield >= 0, "eYield expected to be >= 0");
	CvAssertMsg(eYield < NUM_YIELD_TYPES, "eYield expected to be < NUM_YIELD_TYPES");

	return m_aiYieldFromCrime[eYield];
}

#endif


#if defined(MOD_ROG_CORE)
//	--------------------------------------------------------------------------------
int CvCity::GetNearbyMountains() const
{
	VALIDATE_OBJECT
		return m_iNumNearbyMountains;
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeNearbyMountains(int iChange)
{
	VALIDATE_OBJECT
		SetNearbyMountains(GetNearbyMountains() + iChange);
}
//	--------------------------------------------------------------------------------
void CvCity::SetNearbyMountains(int iValue)
{
	VALIDATE_OBJECT
		m_iNumNearbyMountains = iValue;
}

void CvCity::UpdateYieldPerXTerrain(YieldTypes eYield, TerrainTypes eTerrain)
{
	VALIDATE_OBJECT
		int iYield = 0;

	int iValidTilesTerrain = 0;
	int iBaseYieldBuildings = 0;

	//Passed in a terrain? Let's only update that.
	if (eTerrain != NO_TERRAIN)
	{
		iBaseYieldBuildings = GetYieldPerXTerrainFromBuildingsTimes100(eTerrain, eYield);

		if (iBaseYieldBuildings > 0)
		{
			if (eTerrain == TERRAIN_MOUNTAIN)
			{
				iValidTilesTerrain = GetNearbyMountains();
			}
			else if (eTerrain == TERRAIN_SNOW)
			{
				iValidTilesTerrain = CountTerrain(TERRAIN_SNOW);
			}
			else
			{
				iValidTilesTerrain = GetNumTerrainWorked(eTerrain);
			}

			iYield = (iValidTilesTerrain * iBaseYieldBuildings) / 100;

			//iDifference determines +/- of difference of old value
			int iDifference = iYield - GetYieldPerXTerrain(eTerrain, eYield);

			//Change base rate first
			ChangeBaseYieldRateFromBuildings(eYield, iDifference);

			//then set base rate for retrieval next time.
			SetYieldPerXTerrain(eTerrain, eYield, iYield);
		}
		else if (GetYieldPerXTerrain(eTerrain, eYield) > 0)
		{
			//No bonuses? Clear it out.
			ChangeBaseYieldRateFromBuildings(eYield, -GetYieldPerXTerrain(eTerrain, eYield));
			SetYieldPerXTerrain(eTerrain, eYield, 0);
		}
	}
	else
	{
		for (int iI = 0; iI < GC.getNumTerrainInfos(); iI++)
		{
			eTerrain = (TerrainTypes)iI;
			if (eTerrain == NO_TERRAIN)
			{
				continue;
			}

			iBaseYieldBuildings = GetYieldPerXTerrainFromBuildingsTimes100(eTerrain, eYield);

			if (iBaseYieldBuildings > 0)
			{
				if (eTerrain == TERRAIN_MOUNTAIN)
				{
					iValidTilesTerrain = GetNearbyMountains();
				}
				else if (eTerrain == TERRAIN_SNOW)
				{
					iValidTilesTerrain = CountTerrain(TERRAIN_SNOW);
				}
				else
				{
					iValidTilesTerrain = GetNumTerrainWorked(eTerrain);
				}

				iYield = (iValidTilesTerrain * iBaseYieldBuildings) / 100;

				//iDifference determines +/- of difference of old value
				int iDifference = iYield - GetYieldPerXTerrain(eTerrain, eYield);

				//Change base rate first
				ChangeBaseYieldRateFromBuildings(eYield, iDifference);

				//then set base rate for retrieval next time.
				SetYieldPerXTerrain(eTerrain, eYield, iYield);
			}
			else if (GetYieldPerXTerrain(eTerrain, eYield) > 0)
			{
				//No bonuses? Clear it out.
				ChangeBaseYieldRateFromBuildings(eYield, -GetYieldPerXTerrain(eTerrain, eYield));
				SetYieldPerXTerrain(eTerrain, eYield, 0);
			}
		}
	}
}

//	--------------------------------------------------------------------------------
int CvCity::GetNumTerrainWorked(TerrainTypes eTerrain)
{
	CvAssertMsg(eTerrain >= 0, "eTerrain is expected to be non-negative (invalid Index)");
	CvAssertMsg(eTerrain < GC.getNumTerrainInfos(), "eTerrain is expected to be within maximum bounds (invalid Index)");
	return m_paiNumTerrainWorked[eTerrain];
}
//	--------------------------------------------------------------------------------
void CvCity::ChangeNumTerrainWorked(TerrainTypes eTerrain, int iChange)
{
	CvAssertMsg(eTerrain >= 0, "eTerrain is expected to be non-negative (invalid Index)");
	CvAssertMsg(eTerrain < GC.getNumTerrainInfos(), "eTerrain is expected to be within maximum bounds (invalid Index)");
	m_paiNumTerrainWorked[eTerrain] = m_paiNumTerrainWorked[eTerrain] + iChange;
	CvAssert(GetNumTerrainWorked(eTerrain) >= 0);

	//Update yields
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{

		UpdateYieldPerXTerrain(((YieldTypes)iI), eTerrain);
	}
	processTerrain(eTerrain, iChange);
}

int CvCity::GetNumFeaturelessTerrainWorked(TerrainTypes eTerrain)
{
	CvAssertMsg(eTerrain >= 0, "eTerrain is expected to be non-negative (invalid Index)");
	CvAssertMsg(eTerrain < GC.getNumTerrainInfos(), "eTerrain is expected to be within maximum bounds (invalid Index)");
	return m_paiNumFeaturelessTerrainWorked[eTerrain];
}
//	--------------------------------------------------------------------------------
void CvCity::ChangeNumFeaturelessTerrainWorked(TerrainTypes eTerrain, int iChange)
{
	CvAssertMsg(eTerrain >= 0, "eTerrain is expected to be non-negative (invalid Index)");
	CvAssertMsg(eTerrain < GC.getNumTerrainInfos(), "eTerrain is expected to be within maximum bounds (invalid Index)");
	m_paiNumFeaturelessTerrainWorked[eTerrain] = m_paiNumFeaturelessTerrainWorked[eTerrain] + iChange;
	CvAssert(GetNumFeaturelessTerrainWorked(eTerrain) >= 0);

	//Update yields
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		UpdateYieldPerXTerrain(((YieldTypes)iI), eTerrain);
	}
}
//	--------------------------------------------------------------------------------
int CvCity::GetNumFeatureWorked(FeatureTypes eFeature)
{
	CvAssertMsg(eFeature >= 0, "eFeature is expected to be non-negative (invalid Index)");
	CvAssertMsg(eFeature < GC.getNumFeatureInfos(), "eFeature is expected to be within maximum bounds (invalid Index)");
	return m_paiNumFeatureWorked[eFeature];
}
//	--------------------------------------------------------------------------------
void CvCity::ChangeNumFeatureWorked(FeatureTypes eFeature, int iChange)
{
	CvAssertMsg(eFeature >= 0, "eFeature is expected to be non-negative (invalid Index)");
	CvAssertMsg(eFeature < GC.getNumFeatureInfos(), "eFeature is expected to be within maximum bounds (invalid Index)");
	m_paiNumFeatureWorked[eFeature] = m_paiNumFeatureWorked[eFeature] + iChange;
	CvAssert(GetNumFeatureWorked(eFeature) >= 0);

	//Update yields
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		UpdateYieldPerXFeature(((YieldTypes)iI), eFeature);
	}
	processFeature(eFeature, iChange);
}


//	--------------------------------------------------------------------------------
int CvCity::GetNumImprovementWorked(ImprovementTypes eImprovement)
{
	CvAssertMsg(eImprovement >= 0, "eImprovement is expected to be non-negative (invalid Index)");
	CvAssertMsg(eImprovement < GC.getNumImprovementInfos(), "eImprovement is expected to be within maximum bounds (invalid Index)");
	return m_paiNumImprovementWorked[eImprovement];
}
//	--------------------------------------------------------------------------------
void CvCity::ChangeNumImprovementWorked(ImprovementTypes eImprovement, int iChange)
{
	CvAssertMsg(eImprovement >= 0, "eImprovement is expected to be non-negative (invalid Index)");
	CvAssertMsg(eImprovement < GC.getNumImprovementInfos(), "eImprovement is expected to be within maximum bounds (invalid Index)");
	m_paiNumImprovementWorked[eImprovement] = m_paiNumImprovementWorked[eImprovement] + iChange;
	CvAssert(GetNumImprovementWorked(eImprovement) >= 0);

	processImprovement(eImprovement, iChange);
}


//	--------------------------------------------------------------------------------
void CvCity::SetYieldPerXTerrain(TerrainTypes eTerrain, YieldTypes eYield, int iValue)
{
	VALIDATE_OBJECT
	CvAssertMsg(eTerrain > -1 && eTerrain < GC.getNumTerrainInfos(), "Invalid Terrain index.");
	CvAssertMsg(eYield > -1 && eYield < NUM_YIELD_TYPES, "Invalid yield index.");

	SCityExtraYields& y = m_yieldChanges[eYield];
	if (ModifierUpdateInsertRemove(y.forXTerrain, eTerrain, iValue, false))
		updateYield(false);
}

//	--------------------------------------------------------------------------------
/// Extra yield for a Terrain this city is working?
int CvCity::GetYieldPerXTerrain(TerrainTypes eTerrain, YieldTypes eYield) const
{
	VALIDATE_OBJECT
		CvAssertMsg(eTerrain > -1 && eTerrain < GC.getNumTerrainInfos(), "Invalid Terrain index.");
	CvAssertMsg(eYield > -1 && eYield < NUM_YIELD_TYPES, "Invalid yield index.");

	return ModifierLookup(m_yieldChanges[eYield].forXTerrain, eTerrain);
}

//	--------------------------------------------------------------------------------
/// Extra yield for a Terrain this city is working?
int CvCity::GetYieldPerXTerrainFromBuildingsTimes100(TerrainTypes eTerrain, YieldTypes eYield) const
{
	VALIDATE_OBJECT
		CvAssertMsg(eTerrain > -1 && eTerrain < GC.getNumTerrainInfos(), "Invalid Terrain index.");
	CvAssertMsg(eYield > -1 && eYield < NUM_YIELD_TYPES, "Invalid yield index.");

	return ModifierLookup(m_yieldChanges[eYield].forTerrainFromBuildings, eTerrain);
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeYieldPerXTerrainFromBuildingsTimes100(TerrainTypes eTerrain, YieldTypes eYield, int iChange)
{
	VALIDATE_OBJECT
		CvAssertMsg(eTerrain > -1 && eTerrain < GC.getNumTerrainInfos(), "Invalid Terrain index.");
	CvAssertMsg(eYield > -1 && eYield < NUM_YIELD_TYPES, "Invalid yield index.");

	SCityExtraYields& y = m_yieldChanges[eYield];
	if (ModifierUpdateInsertRemove(y.forTerrainFromBuildings, eTerrain, iChange, true))
		updateYield(false);
}

//	--------------------------------------------------------------------------------
/// Extra yield for a Feature this city is working?
int CvCity::GetYieldPerXFeatureFromBuildingsTimes100(FeatureTypes eFeature, YieldTypes eYield) const
{
	VALIDATE_OBJECT
		CvAssertMsg(eFeature > -1 && eFeature < GC.getNumFeatureInfos(), "Invalid Feature index.");
	CvAssertMsg(eYield > -1 && eYield < NUM_YIELD_TYPES, "Invalid yield index.");

	return ModifierLookup(m_yieldChanges[eYield].forFeatureFromBuildings, eFeature);
}

//	--------------------------------------------------------------------------------
void CvCity::ChangeYieldPerXFeatureFromBuildingsTimes100(FeatureTypes eFeature, YieldTypes eYield, int iChange)
{
	VALIDATE_OBJECT
		CvAssertMsg(eFeature > -1 && eFeature < GC.getNumFeatureInfos(), "Invalid Feature index.");
	CvAssertMsg(eYield > -1 && eYield < NUM_YIELD_TYPES, "Invalid yield index.");

	SCityExtraYields& y = m_yieldChanges[eYield];
	if (ModifierUpdateInsertRemove(y.forFeatureFromBuildings, eFeature, iChange, true))
	{
		updateYield(false);
		UpdateYieldPerXFeature(eYield, eFeature);
	}
}

//	--------------------------------------------------------------------------------
/// Extra yield for a Feature this city is working?
int CvCity::GetYieldPerXFeature(FeatureTypes eFeature, YieldTypes eYield) const
{
	VALIDATE_OBJECT
    CvAssertMsg(eFeature > -1 && eFeature < GC.getNumFeatureInfos(), "Invalid Terrain index.");
	CvAssertMsg(eYield > -1 && eYield < NUM_YIELD_TYPES, "Invalid yield index.");

	return ModifierLookup(m_yieldChanges[eYield].forXFeature, eFeature);
}


//	--------------------------------------------------------------------------------
void CvCity::SetYieldPerXFeature(FeatureTypes eFeature, YieldTypes eYield, int iValue)
{
	VALIDATE_OBJECT
	CvAssertMsg(eFeature > -1 && eFeature < GC.getNumFeatureInfos(), "Invalid Feature index.");
	CvAssertMsg(eYield > -1 && eYield < NUM_YIELD_TYPES, "Invalid yield index.");

	SCityExtraYields& y = m_yieldChanges[eYield];
	if (ModifierUpdateInsertRemove(y.forXFeature, eFeature, iValue, false))
		updateYield(false);
}

void CvCity::UpdateYieldPerXFeature(YieldTypes eYield, FeatureTypes eFeature)
{
	VALIDATE_OBJECT
		int iYieldBase = 0;

	int iValidTiles = 0;
	int iBaseYield = 0;


	//If we passed in a feature, let's only refresh that.
	if (eFeature != NO_FEATURE)
	{
		iYieldBase = 0;

		iBaseYield = GetYieldPerXFeatureFromBuildingsTimes100(eFeature, eYield);

		bool bTest = false;
		if (iBaseYield <= 0)
		{
			ChangeBaseYieldRateFromBuildings(eYield, -GetYieldPerXFeature(eFeature, eYield));
			SetYieldPerXFeature(eFeature, eYield, 0);
		}
		if (iBaseYield > 0)
		{
			bTest = true;
		}

		if (bTest)
		{
			iValidTiles = GetNumFeatureWorked(eFeature);
			if (iValidTiles > 0)
			{
				//Gain 1 yield per x valid tiles - so if 'x' is 3, and you have 3 tiles that match, you get 1 yield
				iYieldBase = (iValidTiles * iBaseYield) / 100;

				//iDifference determines +/- of difference of old value
				int iBaseDifference = iYieldBase - GetYieldPerXFeature(eFeature, eYield);

				//Change base rate first
				ChangeBaseYieldRateFromBuildings(eYield, iBaseDifference);
				SetYieldPerXFeature(eFeature, eYield, iYieldBase);

			}
			else
			{
				ChangeBaseYieldRateFromBuildings(eYield, -GetYieldPerXFeature(eFeature, eYield));
				SetYieldPerXFeature(eFeature, eYield, 0);

			}
		}
	}
	else
	{
		for (int iI = 0; iI < GC.getNumFeatureInfos(); iI++)
		{
			eFeature = (FeatureTypes)iI;
			if (eFeature == NO_FEATURE)
			{
				continue;
			}
			iYieldBase = 0;

			iBaseYield = GetYieldPerXFeatureFromBuildingsTimes100(eFeature, eYield);

			bool bTest = false;
			if (iBaseYield <= 0)
			{
				ChangeBaseYieldRateFromBuildings(eYield, -GetYieldPerXFeature(eFeature, eYield));
				SetYieldPerXFeature(eFeature, eYield, 0);
			}
			if (iBaseYield > 0)
			{
				bTest = true;
			}

			if (bTest)
			{
				iValidTiles = GetNumFeatureWorked(eFeature);
				if (iValidTiles > 0)
				{
					//Gain 1 yield per x valid tiles - so if 'x' is 3, and you have 3 tiles that match, you get 1 yield
					iYieldBase = (iValidTiles * iBaseYield) / 100;

					//iDifference determines +/- of difference of old value
					int iDifference = iYieldBase - GetYieldPerXFeature(eFeature, eYield);

					//Change base rate first
					ChangeBaseYieldRateFromBuildings(eYield, iDifference);
					SetYieldPerXFeature(eFeature, eYield, iYieldBase);
				}
				else
				{
					ChangeBaseYieldRateFromBuildings(eYield, -GetYieldPerXFeature(eFeature, eYield));
					SetYieldPerXFeature(eFeature, eYield, 0);

				}
			}
		}
	}
}
#endif

#if defined(MOD_BUILDING_IMPROVEMENT_RESOURCES)
//	--------------------------------------------------------------------------------
/// Extra Resource From Improvement
int CvCity::GetResourceFromImprovement(ResourceTypes eResource, ImprovementTypes eImprovement) const
{
	VALIDATE_OBJECT

	CvAssertMsg(eResource >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eResource < GC.getNumResourceInfos(), "eIndex expected to be < GC.getNumResourceInfos()");

	CvAssertMsg(eImprovement >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eImprovement < GC.getNumImprovementInfos(), "eIndex expected to be < GC.getNumImprovementInfos()");

	if (eResource != NO_RESOURCE && eImprovement != NO_IMPROVEMENT)
	{
		std::map<std::pair<int, int>, short>::const_iterator it = m_ppiResourceFromImprovement.find(std::make_pair((int)eResource, (int)eImprovement));
		if (it != m_ppiResourceFromImprovement.end()) // find returns the iterator to map::end if the key eResource is not present in the map
		{
			return it->second;
		}
	}

	return 0;
}

//	--------------------------------------------------------------------------------
/// Extra Resource From Improvement
void CvCity::ChangeResourceFromImprovement(ResourceTypes eResource, ImprovementTypes eImprovement, int iChange)
{
	VALIDATE_OBJECT

	CvAssertMsg(eResource >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eResource < GC.getNumResourceInfos(), "eIndex expected to be < GC.getNumResourceInfos()");

	CvAssertMsg(eImprovement >= 0, "eIndex expected to be >= 0");
	CvAssertMsg(eImprovement < GC.getNumImprovementInfos(), "eIndex expected to be < GC.getNumImprovementInfos()");


	if (eResource != NO_RESOURCE && eImprovement != NO_IMPROVEMENT && iChange != 0)
	{
		std::map<std::pair<int, int>, short>::iterator it = m_ppiResourceFromImprovement.find(std::make_pair((int)eResource, (int)eImprovement));
		if (it == m_ppiResourceFromImprovement.end()) // if the key (eGreatPerson, eEra) does not exist
		{
			m_ppiResourceFromImprovement.insert(std::pair<std::pair<int, int>, short>(std::make_pair((int)eResource, (int)eImprovement), iChange));
		}
		else // if the key (eResource, eImprovement) does exist
		{
			it->second += iChange;
		}
	}
}
#endif



FDataStream& operator<<(FDataStream& saveTo, const SCityExtraYields& readFrom)
{
	saveTo << readFrom.forTerrain;
	saveTo << readFrom.forXTerrain;
	saveTo << readFrom.forTerrainFromBuildings;
	saveTo << readFrom.forTerrainFromReligion;

	saveTo << readFrom.forFeature;
	saveTo << readFrom.forXFeature;
	saveTo << readFrom.forFeatureFromBuildings;
	saveTo << readFrom.forFeatureFromReligion;
	saveTo << readFrom.forFeatureUnimproved;

	saveTo << readFrom.forImprovement;
	saveTo << readFrom.forSpecialist;
	saveTo << readFrom.forResource;
	saveTo << readFrom.forPlot;
	saveTo << readFrom.forYield;
	saveTo << readFrom.forActualYield;
	saveTo << readFrom.forLocalBuilding;
	saveTo << readFrom.forReligionBuilding;
	return saveTo;
}
FDataStream& operator>>(FDataStream& loadFrom, SCityExtraYields& writeTo)
{
	loadFrom >> writeTo.forTerrain;
	loadFrom >> writeTo.forXTerrain;
	loadFrom >> writeTo.forTerrainFromBuildings;
	loadFrom >> writeTo.forTerrainFromReligion;

	loadFrom >> writeTo.forFeature;
	loadFrom >> writeTo.forXFeature;
	loadFrom >> writeTo.forFeatureFromBuildings;
	loadFrom >> writeTo.forFeatureFromReligion;
	loadFrom >> writeTo.forFeatureUnimproved;

	loadFrom >> writeTo.forImprovement;
	loadFrom >> writeTo.forSpecialist;
	loadFrom >> writeTo.forResource;
	loadFrom >> writeTo.forPlot;
	loadFrom >> writeTo.forYield;
	loadFrom >> writeTo.forActualYield;
	loadFrom >> writeTo.forLocalBuilding;
	loadFrom >> writeTo.forReligionBuilding;
	return loadFrom;
}







