/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#pragma once

#ifndef CIV5_TRAIT_CLASSES_H
#define CIV5_TRAIT_CLASSES_H

#include "CustomMods.h"

struct FreeResourceXCities
{
	FreeResourceXCities():
		m_iNumCities(0),
		m_iResourceQuantity(0)
	{};

	int m_iNumCities;
	int m_iResourceQuantity;
};

struct MayaBonusChoice
{
	MayaBonusChoice():
		m_eUnitType(NO_UNIT),
		m_iBaktunJustFinished(0)
	{};

	MayaBonusChoice(const MayaBonusChoice& in)
	{
		m_eUnitType = in.m_eUnitType;
		m_iBaktunJustFinished = in.m_iBaktunJustFinished;
	};

	UnitTypes m_eUnitType;
	int m_iBaktunJustFinished;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvTraitEntry
//!  \brief		A single entry in the trait XML file
//
//!  Key Attributes:
//!  - Used to be called CvTraitInfo
//!  - Populated from XML\Civilizations\CIV5Traits.xml
//!  - Array of these contained in CvTraitXMLEntries class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvTraitEntry: public CvBaseInfo
{
public:
	CvTraitEntry(void);
	~CvTraitEntry(void);

	int GetLevelExperienceModifier() const;
	int GetGreatPeopleRateModifier() const;
	int GetGreatScientistRateModifier() const;
	int GetGreatGeneralRateModifier() const;
	int GetGreatGeneralExtraBonus() const;
	int GetGreatPersonGiftInfluence() const;
	int GetMaxGlobalBuildingProductionModifier() const;
	int GetMaxTeamBuildingProductionModifier() const;
	int GetMaxPlayerBuildingProductionModifier() const;
	int GetCityUnhappinessModifier() const;
	int GetPopulationUnhappinessModifier() const;
	int GetCityStateBonusModifier() const;
	int GetCityStateFriendshipModifier() const;
	int GetCityStateCombatModifier() const;
	int GetLandBarbarianConversionPercent() const;
	int GetLandBarbarianConversionExtraUnits() const;
	int GetSeaBarbarianConversionPercent() const;
	int GetCapitalBuildingModifier() const;
	int GetPlotBuyCostModifier() const;
#if defined(MOD_TRAITS_CITY_WORKING)
	int GetCityWorkingChange() const;
#endif
#if defined(MOD_TRAITS_CITY_AUTOMATON_WORKERS)
	int GetCityAutomatonWorkersChange() const;
#endif
	int GetPlotCultureCostModifier() const;
	int GetCultureFromKills() const;
	int GetFaithFromKills() const;
	int GetCityCultureBonus() const;
	int GetCapitalThemingBonusModifier() const;
	int GetPolicyCostModifier() const;
	int GetCityConnectionTradeRouteChange() const;
	int GetWonderProductionModifier() const;
	int GetPlunderModifier() const;
	int GetImprovementMaintenanceModifier() const;
	int GetGoldenAgeDurationModifier() const;
	int GetGoldenAgeMoveChange() const;
	int GetGoldenAgeCombatModifier() const;
	int GetGoldenAgeTourismModifier() const;
	int GetGoldenAgeGreatArtistRateModifier() const;
	int GetGoldenAgeGreatMusicianRateModifier() const;
	int GetGoldenAgeGreatWriterRateModifier() const;
	int GetExtraEmbarkMoves() const;
	int GetFreeUnitClassType() const;
	int GetNaturalWonderFirstFinderGold() const;
	int GetNaturalWonderSubsequentFinderGold() const;
	int GetNaturalWonderYieldModifier() const;
	int GetNaturalWonderHappinessModifier() const;
	int GetNearbyImprovementCombatBonus() const;
	int GetNearbyImprovementBonusRange() const;
	int GetCultureBuildingYieldChange() const;
	int GetCombatBonusVsHigherTech() const;
	int GetAwayFromCapitalCombatModifier() const;
	int GetAwayFromCapitalCombatModifierMax() const;
	int GetInfluenceFromGreatPeopleBirth() const;
	int GetWLKDLengthChangeModifier() const;
	int GetCombatBonusVsLargerCiv() const;
	int GetLandUnitMaintenanceModifier() const;
	int GetNavalUnitMaintenanceModifier() const;
	int GetRazeSpeedModifier() const;
	int GetDOFGreatPersonModifier() const;
	int GetLuxuryHappinessRetention() const;
#if defined(MOD_TRAITS_EXTRA_SUPPLY)
	int GetExtraSupply() const;
	int GetExtraSupplyPerCity() const;
	int GetExtraSupplyPerPopulation() const;
#endif
	int GetExtraSpies() const;
	int GetUnresearchedTechBonusFromKills() const;
	int GetExtraFoundedCityTerritoryClaimRange() const;
	int GetFreeSocialPoliciesPerEra() const;
	int GetFreeGreatPeoplePerEra() const;
	int GetExtraUnitPlayerInstances() const;
	int GetConquestCasualtiesModifier() const;
	int GetOwnedReligionUnitCultureExtraTurns() const;
	int GetNumTradeRoutesModifier() const;
	int GetTradeRouteResourceModifier() const;
	int GetUniqueLuxuryCities() const;
	int GetUniqueLuxuryQuantity() const;
	int GetAllyCityStateCombatModifier() const;
	int GetAllyCityStateCombatModifierMax() const;
	int	GetAdequateLuxuryCompleteQuestInfluenceModifier() const;
	int GetAdequateLuxuryCompleteQuestInfluenceModifierMax() const;
	int GetWorkerSpeedModifier() const;
	int GetAfraidMinorPerTurnInfluence() const;
	int GetLandTradeRouteRangeBonus() const;
	int GetGoldenAgeMinorPerTurnInfluence() const;
#if defined(MOD_TRAITS_TRADE_ROUTE_BONUSES)
	int GetSeaTradeRouteRangeBonus() const;
#endif
	int GetTradeReligionModifier() const;
	int GetTradeBuildingModifier() const;
#if defined(MOD_TRAIT_NEW_EFFECT_FOR_SP)
	int GetNumFreeWorldWonderPerCity() const;
	int GetExceedingHappinessImmigrationModifier() const;
	int GetNumCityYieldPerAdjacentFeature() const;
	int GetNumCityAdjacentFeatureModifier() const;
	int GetPromotionWhenKilledUnit() const;
	int GetPromotionRadiusWhenKilledUnit() const;
	int GetAttackBonusAdjacentWhenUnitKilled() const;
	int GetKilledAttackBonusDecreasePerTurn() const;
	int GetTriggersIdeologyTech() const;
	int GetNaturalWonderCorruptionScoreChange() const;
	int GetNaturalWonderCorruptionRadius() const;
	int GetCultureBonusUnitStrengthModify() const;
	int GetRiverCorruptionScoreChange() const;
	int GetGreatWorksTourism() const;
	int GetCiviliansFreePromotion() const;
	int GetTradeRouteLandGoldBonus() const;
	int GetTradeRouteSeaGoldBonus() const;
#endif

	TechTypes GetFreeUnitPrereqTech() const;
	ImprovementTypes GetCombatBonusImprovement() const;
	BuildingTypes GetFreeBuilding() const;
	BuildingTypes GetFreeBuildingOnConquest() const;

	bool IsTrainedAll() const;
	bool IsCanConquerUC() const;
	bool IsWLKDCityNoResearchCost() const;
	bool IsGoodyUnitUpgradeFirst() const;
	bool IsFightWellDamaged() const;
	bool IsBuyOwnedTiles() const;
	bool IsMoveFriendlyWoodsAsRoad() const;
	bool IsFasterAlongRiver() const;
	bool IsFasterInHills() const;
	bool IsEmbarkedAllWater() const;
	bool IsEmbarkedToLandFlatCost() const;
	bool IsNoHillsImprovementMaintenance() const;
	bool IsTechBoostFromCapitalScienceBuildings() const;
	bool IsArtistGoldenAgeTechBoost() const;
	bool IsStaysAliveZeroCities() const;
	bool IsFaithFromUnimprovedForest() const;
#if defined(MOD_TRAITS_ANY_BELIEF)
	bool IsAnyBelief() const;
#endif
	bool IsGoldenAgeOnWar() const;
	bool IsNoResistance() const;
	bool IsBonusReligiousBelief() const;
	bool IsAbleToAnnexCityStates() const;
	bool IsAbleToDualEmpire() const;
	bool IsNoDoDeficit() const;
	bool IsCrossesMountainsAfterGreatGeneral() const;
#if defined(MOD_TRAITS_CROSSES_ICE)
	bool IsCrossesIce() const;
#endif
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS)
	bool IsGGFromBarbarians() const;
#endif
	bool IsMayaCalendarBonuses() const;
	bool IsNoAnnexing() const;
	bool IsTechFromCityConquer() const;
	bool IsUniqueLuxuryRequiresNewArea() const;
	bool IsRiverTradeRoad() const;
	bool IsAngerFreeIntrusionOfCityStates() const;

	const char* getShortDescription() const;
	void setShortDescription(const char* szVal);

	// Arrays
	int GetExtraYieldThreshold(int i) const;
	int GetYieldChange(int i) const;
	int GetYieldChangeStrategicResources(int i) const;
	int GetYieldChangeNaturalWonder(int i) const;
	int GetYieldChangePerTradePartner(int i) const;
	int GetYieldChangeIncomingTradeRoute(int i) const;
	int GetYieldModifier(int i) const;
	int GetGoldenAgeYieldModifier(int i) const;
	int GetStrategicResourceQuantityModifier(int i) const;
	int GetObsoleteTech() const;
	int GetPrereqTech() const;
#if defined(MOD_TRAITS_OTHER_PREREQS)
	int GetObsoleteBelief() const;
	int GetPrereqBelief() const;
	int GetObsoletePolicy() const;
	int GetPrereqPolicy() const;
#endif
	int GetResourceQuantityModifier(int i) const;
	int GetMovesChangeUnitCombat(const int unitCombatID) const;
	int GetMaintenanceModifierUnitCombat(const int unitCombatID) const;
	int GetImprovementYieldChanges(ImprovementTypes eIndex1, YieldTypes eIndex2) const;
#if defined(MOD_API_UNIFIED_YIELDS) && defined(MOD_API_PLOT_YIELDS)
	int GetPlotYieldChanges(PlotTypes eIndex1, YieldTypes eIndex2) const;
#endif
#if defined(MOD_API_UNIFIED_YIELDS)
	int GetBuildingClassYieldChanges(BuildingClassTypes eIndex1, YieldTypes eIndex2) const;
	int GetCapitalYieldChanges(int i) const;
	int GetCityYieldChanges(int i) const;
	int GetCoastalCityYieldChanges(int i) const;
	int GetGreatWorkYieldChanges(int i) const;
	int GetFeatureYieldChanges(FeatureTypes eIndex1, YieldTypes eIndex2) const;
	int GetResourceYieldChanges(ResourceTypes eIndex1, YieldTypes eIndex2) const;
	int GetTerrainYieldChanges(TerrainTypes eIndex1, YieldTypes eIndex2) const;
	int GetYieldFromKills(YieldTypes eYield) const;
	int GetYieldFromBarbarianKills(YieldTypes eYield) const;
	int GetYieldChangeTradeRoute(int i) const;
	int GetYieldChangeWorldWonder(int i) const;
	int GetRiverPlotYieldChanges(int i) const;
	int GetTradeRouteYieldChange(DomainTypes eIndex1, YieldTypes eIndex2) const;
#endif
	int GetSpecialistYieldChanges(SpecialistTypes eIndex1, YieldTypes eIndex2) const;
#if defined(MOD_API_UNIFIED_YIELDS)
	int GetGreatPersonExpendedYield(GreatPersonTypes eIndex1, YieldTypes eIndex2) const;
	int GetGoldenAgeGreatPersonRateModifier(GreatPersonTypes eIndex1) const;
	int GetCityYieldFromUnimprovedFeature(FeatureTypes eIndex1, YieldTypes eIndex2) const;
#endif
	int GetUnimprovedFeatureYieldChanges(FeatureTypes eIndex1, YieldTypes eIndex2) const;
	int GetCityYieldModifierFromAdjacentFeature(FeatureTypes eIndex1, YieldTypes eIndex2) const;
	int GetCityYieldPerAdjacentFeature(FeatureTypes eIndex1, YieldTypes eIndex2) const;
	FreeResourceXCities GetFreeResourceXCities(ResourceTypes eResource) const;

	bool IsFreePromotionUnitCombat(const int promotionID, const int unitCombatID) const;
#if defined(MOD_TRAIT_NEW_EFFECT_FOR_SP)
	bool IsFreePromotionUnitClass(const int promotionID, const int unitClassID) const;
#endif
	bool IsHasBuildingClassFaithCost() const;
	int GetBuildingClassFaithCost(int iBuildingClass) const;
	bool IsObsoleteByTech(TeamTypes eTeam);
	bool IsEnabledByTech(TeamTypes eTeam);
#if defined(MOD_TRAITS_OTHER_PREREQS)
	bool IsObsoleteByBelief(PlayerTypes ePlayer);
	bool IsEnabledByBelief(PlayerTypes ePlayer);
	bool IsObsoleteByPolicy(PlayerTypes ePlayer);
	bool IsEnabledByPolicy(PlayerTypes ePlayer);
#endif

#ifdef MOD_TRAITS_CAN_FOUND_MOUNTAIN_CITY
	bool IsCanFoundMountainCity() const;
#endif
#ifdef MOD_TRAITS_CAN_FOUND_COAST_CITY
	bool IsCanFoundCoastCity() const;
#endif

	bool NoTrain(UnitClassTypes eUnitClassType);

	int GetSeaTradeRouteYieldPerEraTimes100(const YieldTypes eYield) const;
	int GetSeaTradeRouteYieldTimes100(const YieldTypes eYield) const;
#ifdef MOD_TRAIT_RELIGION_FOLLOWER_EFFECTS
	int GetPerMajorReligionFollowerYieldModifierTimes100(const YieldTypes eYield) const;
	int GetPerMajorReligionFollowerYieldModifierMax(const YieldTypes eYield) const;
#endif

#ifdef MOD_TRAITS_SPREAD_RELIGION_AFTER_KILLING
	int GetSpreadReligionFromKilledUnitStrengthPercent() const;
	int GetSpreadReligionRadius() const;
#endif

#ifdef MOD_TRAITS_COMBAT_BONUS_FROM_CAPTURED_HOLY_CITY
	int GetInflictDamageChangePerCapturedHolyCity() const;
	int GetDamageChangePerCapturedHolyCity() const;
#endif

#ifdef MOD_TRAITS_SIEGE_BONUS_IF_SAME_RELIGION
	int GetSiegeDamagePercentIfSameReligion() const;
#endif

#ifdef MOD_TRAITS_ENABLE_FAITH_PURCHASE_ALL_COMBAT_UNITS
	int GetFaithPurchaseCostPercent() const;
#endif

#ifdef MOD_GLOBAL_CORRUPTION
	bool GetCorruptionLevelReduceByOne() const;
	int GetMaxCorruptionLevel() const;
#endif

	int GetFreePolicyWhenFirstConquerMajorCapital() const;
	int GetInstantTourismBombWhenFirstConquerMajorCapital() const;

	int GetUnitMaxHitPointChangePerRazedCityPop() const;
	int GetUnitMaxHitPointChangePerRazedCityPopLimit() const;

	int GetGoldenAgeResearchTotalCostModifier() const;
	int GetGoldenAgeResearchCityCountCostModifier() const;
	int GetOthersTradeBonusModifier() const;

	int GetGoldenAgeGrowThresholdModifier() const;

	int GetShareAllyResearchPercent() const;
	bool CanPurchaseWonderInGoldenAge() const;
	bool CanDiplomaticMarriage() const;

	virtual bool CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility);

protected:
	int m_iLevelExperienceModifier;
	int m_iGreatPeopleRateModifier;
	int m_iGreatScientistRateModifier;
	int m_iGreatGeneralRateModifier;
	int m_iGreatGeneralExtraBonus;
	int m_iGreatPersonGiftInfluence;
	int m_iMaxGlobalBuildingProductionModifier;
	int m_iMaxTeamBuildingProductionModifier;
	int m_iMaxPlayerBuildingProductionModifier;
	int m_iCityUnhappinessModifier;
	int m_iPopulationUnhappinessModifier;
	int m_iCityStateBonusModifier;
	int m_iCityStateFriendshipModifier;
	int m_iCityStateCombatModifier;
	int m_iLandBarbarianConversionPercent;
	int m_iLandBarbarianConversionExtraUnits;
	int m_iSeaBarbarianConversionPercent;
	int m_iCapitalBuildingModifier;
	int m_iPlotBuyCostModifier;
#if defined(MOD_TRAITS_CITY_WORKING)
	int m_iCityWorkingChange;
#endif
#if defined(MOD_TRAITS_CITY_AUTOMATON_WORKERS)
	int m_iCityAutomatonWorkersChange;
#endif
	int m_iPlotCultureCostModifier;
	int m_iCultureFromKills;
	int m_iFaithFromKills;
	int m_iCityCultureBonus;
	int m_iCapitalThemingBonusModifier;
	int m_iPolicyCostModifier;
	int m_iCityConnectionTradeRouteChange;
	int m_iWonderProductionModifier;
	int m_iPlunderModifier;
	int m_iImprovementMaintenanceModifier;
	int m_iGoldenAgeDurationModifier;
	int m_iGoldenAgeMoveChange;
	int m_iGoldenAgeCombatModifier;
	int m_iGoldenAgeTourismModifier;
	int m_iGoldenAgeGreatArtistRateModifier;
	int m_iGoldenAgeGreatMusicianRateModifier;
	int m_iGoldenAgeGreatWriterRateModifier;
	int m_iObsoleteTech;
	int m_iPrereqTech;
#if defined(MOD_TRAITS_OTHER_PREREQS)
	int m_iObsoleteBelief;
	int m_iPrereqBelief;
	int m_iObsoletePolicy;
	int m_iPrereqPolicy;
#endif
	int m_iExtraEmbarkMoves;
	int m_iFreeUnitClassType;
	int m_iNaturalWonderFirstFinderGold;
	int m_iNaturalWonderSubsequentFinderGold;
	int m_iNaturalWonderYieldModifier;
	int m_iNaturalWonderHappinessModifier;
	int m_iNearbyImprovementCombatBonus;
	int m_iNearbyImprovementBonusRange;
	int m_iCultureBuildingYieldChange;
	int m_iCombatBonusVsHigherTech;
	int m_iAwayFromCapitalCombatModifier;
	int m_iAwayFromCapitalCombatModifierMax;
	int m_iInfluenceFromGreatPeopleBirth;
	int m_iWLKDLengthChangeModifier;
	int m_iCombatBonusVsLargerCiv;
	int m_iLandUnitMaintenanceModifier;
	int m_iNavalUnitMaintenanceModifier;
	int m_iRazeSpeedModifier;
	int m_iDOFGreatPersonModifier;
	int m_iLuxuryHappinessRetention;
#if defined(MOD_TRAITS_EXTRA_SUPPLY)
	int m_iExtraSupply;
	int m_iExtraSupplyPerCity;
	int m_iExtraSupplyPerPopulation;
#endif
	int m_iExtraSpies;
	int m_iUnresearchedTechBonusFromKills;
	int m_iExtraFoundedCityTerritoryClaimRange;
	int m_iFreeSocialPoliciesPerEra;
	int m_iFreeGreatPeoplePerEra;
	int m_iExtraUnitPlayerInstances;
	int m_iConquestCasualtiesModifier;
	int m_iOwnedReligionUnitCultureExtraTurns;
	int m_iNumTradeRoutesModifier;
	int m_iTradeRouteResourceModifier;
	int m_iUniqueLuxuryCities;
	int m_iUniqueLuxuryQuantity;
	int m_iAllyCityStateCombatModifier;
	int m_iAllyCityStateCombatModifierMax;
	int	m_iAdequateLuxuryCompleteQuestInfluenceModifier;
	int m_iAdequateLuxuryCompleteQuestInfluenceModifierMax;
	int m_iWorkerSpeedModifier;
	int m_iAfraidMinorPerTurnInfluence;
	int m_iLandTradeRouteRangeBonus;
	int m_iGoldenAgeMinorPerTurnInfluence;
#if defined(MOD_TRAITS_TRADE_ROUTE_BONUSES)
	int m_iSeaTradeRouteRangeBonus;
#endif
	int m_iTradeReligionModifier;
	int m_iTradeBuildingModifier;
#if defined(MOD_TRAIT_NEW_EFFECT_FOR_SP)
	int m_iNumFreeWorldWonderPerCity;
	int m_iExceedingHappinessImmigrationModifier;
	int m_iNumCityAdjacentFeatureModifier;
	int m_iNumCityYieldPerAdjacentFeature;
	int m_iPromotionWhenKilledUnit;
	int m_iPromotionRadiusWhenKilledUnit;
	int m_iAttackBonusAdjacentWhenUnitKilled;
	int m_iKilledAttackBonusDecreasePerTurn;
	int m_iTriggersIdeologyTech;
	int m_iNaturalWonderCorruptionScoreChange;
	int m_iNaturalWonderCorruptionRadius;
	int m_iCultureBonusUnitStrengthModify;
	int m_iRiverCorruptionScoreChange;
	int m_iGreatWorksTourism;
	int m_iCiviliansFreePromotion;
	int m_iTradeRouteLandGoldBonus;
	int m_iTradeRouteSeaGoldBonus;
#endif

	TechTypes m_eFreeUnitPrereqTech;
	ImprovementTypes m_eCombatBonusImprovement;
	BuildingTypes m_eFreeBuilding;
	BuildingTypes m_eFreeBuildingOnConquest;

	
	bool m_bTrainedAll;
	bool m_bCanConquerUC;
	bool m_bFightWellDamaged;
	bool m_bBuyOwnedTiles;
	bool m_bMoveFriendlyWoodsAsRoad;
	bool m_bFasterAlongRiver;
	bool m_bFasterInHills;
	bool m_bEmbarkedAllWater;
	bool m_bEmbarkedToLandFlatCost;
	bool m_bNoHillsImprovementMaintenance;
	bool m_bTechBoostFromCapitalScienceBuildings;
	bool m_bArtistGoldenAgeTechBoost;
	bool m_bStaysAliveZeroCities;
	bool m_bFaithFromUnimprovedForest;
	bool m_bWLKDCityNoResearchCost;
	bool m_bGoodyUnitUpgradeFirst;
#if defined(MOD_TRAITS_ANY_BELIEF)
	bool m_bAnyBelief;
#endif
	bool m_bGoldenAgeOnWar;
	bool m_bNoResistance;
	bool m_bBonusReligiousBelief;
	bool m_bAbleToAnnexCityStates;
	bool m_bAbleToDualEmpire = false;
	bool m_bNoDoDeficit = false;
	bool m_bCrossesMountainsAfterGreatGeneral;
#if defined(MOD_TRAITS_CROSSES_ICE)
	bool m_bCrossesIce;
#endif
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS)
	bool m_bGGFromBarbarians;
#endif
	bool m_bMayaCalendarBonuses;
	bool m_bNoAnnexing;
	bool m_bTechFromCityConquer;
	bool m_bUniqueLuxuryRequiresNewArea;
	bool m_bRiverTradeRoad;
	bool m_bAngerFreeIntrusionOfCityStates;
#ifdef MOD_TRAITS_CAN_FOUND_MOUNTAIN_CITY
	bool m_bCanFoundMountainCity;
#endif
#ifdef MOD_TRAITS_CAN_FOUND_COAST_CITY
	bool m_bCanFoundCoastCity;
#endif

	CvString m_strShortDescription;

	// Arrays
	int* m_paiExtraYieldThreshold;
	int* m_paiYieldChange;
	int* m_paiYieldChangeStrategicResources;
	int* m_paiYieldChangeNaturalWonder;
	int* m_paiYieldChangePerTradePartner;
	int* m_paiYieldChangeIncomingTradeRoute;
	int* m_paiYieldModifier;
	int* m_paiGoldenAgeYieldModifier;
	int* m_piStrategicResourceQuantityModifier;
	int* m_piResourceQuantityModifiers;
	int* m_piMovesChangeUnitCombats;
	int* m_piMaintenanceModifierUnitCombats;
	int** m_ppiImprovementYieldChanges;
#if defined(MOD_API_UNIFIED_YIELDS) && defined(MOD_API_PLOT_YIELDS)
	int** m_ppiPlotYieldChanges;
#endif
#if defined(MOD_API_UNIFIED_YIELDS)
	int** m_ppiBuildingClassYieldChanges;
	int* m_piCapitalYieldChanges;
	int* m_piCityYieldChanges;
	int* m_piCoastalCityYieldChanges;
	int* m_piGreatWorkYieldChanges;
	int** m_ppiFeatureYieldChanges;
	int** m_ppiResourceYieldChanges;
	int** m_ppiTerrainYieldChanges;
	int* m_piYieldFromKills;
	int* m_piYieldFromBarbarianKills;
	int* m_piYieldChangeTradeRoute;
	int* m_piYieldChangeWorldWonder;
	int* m_piRiverPlotYieldChanges;
	int** m_ppiTradeRouteYieldChange;
#endif
	int** m_ppiSpecialistYieldChanges;
#if defined(MOD_API_UNIFIED_YIELDS)
	int** m_ppiGreatPersonExpendedYield;
	int* m_piGoldenAgeGreatPersonRateModifier;
	int** m_ppiCityYieldFromUnimprovedFeature;
#endif
	int** m_ppiUnimprovedFeatureYieldChanges;
	int** m_ppiCityYieldModifierFromAdjacentFeature;
	int** m_ppiCityYieldPerAdjacentFeature;

	std::multimap<int, int> m_FreePromotionUnitCombats;
#if defined(MOD_TRAIT_NEW_EFFECT_FOR_SP)
	std::multimap<int, int> m_FreePromotionUnitClasses;
#endif
	int* m_piBuildingClassFaithCost = nullptr;
	std::vector<FreeResourceXCities> m_aFreeResourceXCities;
	std::vector<bool> m_abNoTrainUnitClass;

	int m_piSeaTradeRouteYieldPerEraTimes100[NUM_YIELD_TYPES];
	int m_piSeaTradeRouteYieldTimes100[NUM_YIELD_TYPES];

#ifdef MOD_TRAIT_RELIGION_FOLLOWER_EFFECTS
	int m_piPerMajorReligionFollowerYieldModifierTimes100[NUM_YIELD_TYPES];
	int m_piPerMajorReligionFollowerYieldModifierMax[NUM_YIELD_TYPES];
#endif

#ifdef MOD_TRAITS_SPREAD_RELIGION_AFTER_KILLING
	int m_iSpreadReligionFromKilledUnitStrengthPercent = 0;
	int m_iSpreadReligionRadius = 0;
#endif

#ifdef MOD_TRAITS_COMBAT_BONUS_FROM_CAPTURED_HOLY_CITY
	int m_iInflictDamageChangePerCapturedHolyCity = 0;
	int m_iDamageChangePerCapturedHolyCity = 0;
#endif

#ifdef MOD_TRAITS_SIEGE_BONUS_IF_SAME_RELIGION
	int m_iSiegeDamagePercentIfSameReligion = 0;
#endif

#ifdef MOD_TRAITS_ENABLE_FAITH_PURCHASE_ALL_COMBAT_UNITS
	int m_iFaithPurchaseCombatUnitCostPercent = 0;
#endif

#ifdef MOD_GLOBAL_CORRUPTION
	bool m_bCorruptionLevelReduceByOne = 0;
	int m_iMaxCorruptionLevel = -1;
#endif

	int m_iFreePolicyWhenFirstConquerMajorCapital = 0;
	int m_iInstantTourismBombWhenFirstConquerMajorCapital = 0;

	int m_iUnitMaxHitPointChangePerRazedCityPop = 0;
	int m_iUnitMaxHitPointChangePerRazedCityPopLimit = 0;

	int m_iGoldenAgeResearchTotalCostModifier= 0;
	int m_iGoldenAgeResearchCityCountCostModifier = 0;
	int m_iOthersTradeBonusModifier = 0;

	int m_iGoldenAgeGrowThresholdModifier = 0;

	int m_iShareAllyResearchPercent = 0;
	bool m_bCanPurchaseWonderInGoldenAge = false;
	bool m_bCanDiplomaticMarriage = false;
private:
	CvTraitEntry(const CvTraitEntry&);
	CvTraitEntry& operator=(const CvTraitEntry&);
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvTraitXMLEntries
//!  \brief		Game-wide information about civilization traits
//
//! Key Attributes:
//! - Plan is it will be contained in CvGameRules object within CvGame class
//! - Populated from XML\GameInfo\CIV5Traits.xml
//! - Contains an array of CvTraitEntry from the above XML file
//! - One instance for the entire game
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvTraitXMLEntries
{
public:
	CvTraitXMLEntries(void);
	~CvTraitXMLEntries(void);

	// Accessor functions
	std::vector<CvTraitEntry*>& GetTraitEntries();
	int GetNumTraits();
	CvTraitEntry* GetEntry(int index);

	void DeleteArray();

private:
	std::vector<CvTraitEntry*> m_paTraitEntries;
};

#define SAFE_ESTIMATE_NUM_FREE_UNITS 5

struct FreeTraitUnit
{
	UnitTypes m_iFreeUnit;
	TechTypes m_ePrereqTech;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvPlayerTraits
//!  \brief		Information about the traits of a player
//
//!  Key Attributes:
//!  - One instance for each player
//!  - Accessed by any class that needs to check trait info
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvPlayerTraits
{
public:
	CvPlayerTraits(void);
	~CvPlayerTraits(void);
	void Init(CvTraitXMLEntries* pTraits, CvPlayer* pPlayer);
	void Uninit();
	void Reset();
	void InitPlayerTraits();

	// Accessor functions
	bool HasTrait(TraitTypes eTrait) const;
	int GetGreatPeopleRateModifier() const
	{
		return m_iGreatPeopleRateModifier;
	};
	int GetGreatScientistRateModifier() const
	{
		return m_iGreatScientistRateModifier;
	};
	int GetGreatGeneralRateModifier() const
	{
		return m_iGreatGeneralRateModifier;
	};
	int GetGreatGeneralExtraBonus() const
	{
		return m_iGreatGeneralExtraBonus;
	};
	int GetGreatPersonGiftInfluence() const
	{
		return m_iGreatPersonGiftInfluence;
	};
	int GetLevelExperienceModifier() const
	{
		return m_iLevelExperienceModifier;
	};
	int GetMaxGlobalBuildingProductionModifier() const
	{
		return m_iMaxGlobalBuildingProductionModifier;
	};
	int GetMaxTeamBuildingProductionModifier() const
	{
		return m_iMaxTeamBuildingProductionModifier;
	};
	int GetMaxPlayerBuildingProductionModifier() const
	{
		return m_iMaxPlayerBuildingProductionModifier;
	};
	int GetCityUnhappinessModifier() const
	{
		return m_iCityUnhappinessModifier;
	};
	int GetPopulationUnhappinessModifier() const
	{
		return m_iPopulationUnhappinessModifier;
	};
	int GetCityStateBonusModifier() const
	{
		return m_iCityStateBonusModifier;
	};
	int GetCityStateFriendshipModifier() const
	{
		return m_iCityStateFriendshipModifier;
	};
	int GetCityStateCombatModifier() const
	{
		return m_iCityStateCombatModifier;
	};
	int GetLandBarbarianConversionPercent() const
	{
		return m_iLandBarbarianConversionPercent;
	};
	int GetLandBarbarianConversionExtraUnits() const
	{
		return m_iLandBarbarianConversionExtraUnits;
	};
	int GetSeaBarbarianConversionPercent() const
	{
		return m_iSeaBarbarianConversionPercent;
	};
	int GetCapitalBuildingModifier() const
	{
		return m_iCapitalBuildingModifier;
	};
	int GetPlotBuyCostModifier() const
	{
		return m_iPlotBuyCostModifier;
	};
#if defined(MOD_TRAITS_CITY_WORKING)
	int GetCityWorkingChange() const
	{
		return m_iCityWorkingChange;
	};
#endif
#if defined(MOD_TRAITS_CITY_AUTOMATON_WORKERS)
	int GetCityAutomatonWorkersChange() const
	{
		return m_iCityAutomatonWorkersChange;
	};
#endif
	int GetPlotCultureCostModifier() const
	{
		return m_iPlotCultureCostModifier;
	};
	int GetCultureFromKills() const
	{
		return m_iCultureFromKills;
	};
	int GetFaithFromKills() const
	{
		return m_iFaithFromKills;
	};
	int GetCityCultureBonus() const
	{
		return m_iCityCultureBonus;
	};
	int GetCapitalThemingBonusModifier() const
	{
		return m_iCapitalThemingBonusModifier;
	}
	int GetPolicyCostModifier() const
	{
		return m_iPolicyCostModifier;
	};
	int GetCityConnectionTradeRouteChange() const
	{
		return m_iCityConnectionTradeRouteChange;
	};
	int GetWonderProductionModifier() const
	{
		return m_iWonderProductionModifier;
	};
	int GetPlunderModifier() const
	{
		return m_iPlunderModifier;
	};
	int GetImprovementMaintenanceModifier() const
	{
		return m_iImprovementMaintenanceModifier;
	};
	int GetGoldenAgeDurationModifier() const
	{
		return m_iGoldenAgeDurationModifier;
	};
	int GetGoldenAgeMoveChange() const
	{
		return m_iGoldenAgeMoveChange;
	};
	int GetGoldenAgeCombatModifier() const
	{
		return m_iGoldenAgeCombatModifier;
	};
	int GetGoldenAgeTourismModifier() const
	{
		return m_iGoldenAgeTourismModifier;
	};
	int GetGoldenAgeGreatArtistRateModifier() const
	{
		return m_iGoldenAgeGreatArtistRateModifier;
	};
	int GetGoldenAgeGreatMusicianRateModifier() const
	{
		return m_iGoldenAgeGreatMusicianRateModifier;
	};
	int GetGoldenAgeGreatWriterRateModifier() const
	{
		return m_iGoldenAgeGreatWriterRateModifier;
	};
	int GetExtraEmbarkMoves() const
	{
		return m_iExtraEmbarkMoves;
	};
	int GetFirstFreeUnit(TechTypes eTech);
	int GetNextFreeUnit();
	int GetNaturalWonderFirstFinderGold() const
	{
		return m_iNaturalWonderFirstFinderGold;
	};
	int GetNaturalWonderSubsequentFinderGold() const
	{
		return m_iNaturalWonderSubsequentFinderGold;
	};
	int GetNaturalWonderYieldModifier() const
	{
		return m_iNaturalWonderYieldModifier;
	};
	int GetNaturalWonderHappinessModifier() const
	{
		return m_iNaturalWonderHappinessModifier;
	};
	int GetNearbyImprovementCombatBonus() const
	{
		return m_iNearbyImprovementCombatBonus;
	};
	int GetNearbyImprovementBonusRange() const
	{
		return m_iNearbyImprovementBonusRange;
	};
	int GetCultureBuildingYieldChange() const
	{
		return m_iCultureBuildingYieldChange;
	};
	int GetCombatBonusVsHigherTech() const
	{
		return m_iCombatBonusVsHigherTech;
	};
	int GetAwayFromCapitalCombatModifier() const
	{
		return m_iAwayFromCapitalCombatModifier;
	};
	int GetAwayFromCapitalCombatModifierMax() const
	{
		return m_iAwayFromCapitalCombatModifierMax;
	};
	int GetInfluenceFromGreatPeopleBirth() const
	{
		return m_iInfluenceFromGreatPeopleBirth;
	};
	int GetWLKDLengthChangeModifier() const
	{
		return m_iWLKDLengthChangeModifier;
	};
	int GetCombatBonusVsLargerCiv() const
	{
		return m_iCombatBonusVsLargerCiv;
	};
	int GetLandUnitMaintenanceModifier() const
	{
		return m_iLandUnitMaintenanceModifier;
	};
	int GetNavalUnitMaintenanceModifier() const
	{
		return m_iNavalUnitMaintenanceModifier;
	};
	int GetRazeSpeedModifier() const
	{
		return m_iRazeSpeedModifier;
	};
	int GetDOFGreatPersonModifier() const
	{
		return m_iDOFGreatPersonModifier;
	};
	int GetLuxuryHappinessRetention() const
	{
		return m_iLuxuryHappinessRetention;
	};
#if defined(MOD_TRAITS_EXTRA_SUPPLY)
	int GetExtraSupply() const
	{
		return m_iExtraSupply;
	};
	int GetExtraSupplyPerCity() const
	{
		return m_iExtraSupplyPerCity;
	};
	int GetExtraSupplyPerPopulation() const
	{
		return m_iExtraSupplyPerPopulation;
	};
#endif
	int GetExtraSpies() const
	{
		return m_iExtraSpies;
	};
	int GetUnresearchedTechBonusFromKills() const
	{
		return m_iUnresearchedTechBonusFromKills;
	}
	int GetExtraFoundedCityTerritoryClaimRange () const
	{
		return m_iExtraFoundedCityTerritoryClaimRange;
	}
	int GetFreeSocialPoliciesPerEra() const
	{
		return m_iFreeSocialPoliciesPerEra;
	}
	int GetFreeGreatPeoplePerEra() const
	{
		return m_iFreeGreatPeoplePerEra;
	}
	int GetExtraUnitPlayerInstances() const
	{
		return m_iExtraUnitPlayerInstances;
	}
	int GetConquestCasualtiesModifier() const
	{
		return m_iConquestCasualtiesModifier;
	}
	int GetOwnedReligionUnitCultureExtraTurns() const
	{
		return m_iOwnedReligionUnitCultureExtraTurns;
	}
	int GetNumTradeRoutesModifier() const
	{
		return m_iNumTradeRoutesModifier;
	}
	int GetTradeRouteResourceModifier() const
	{
		return m_iTradeRouteResourceModifier;
	}
	int GetUniqueLuxuryCities() const
	{
		return m_iUniqueLuxuryCities;
	}
	int GetUniqueLuxuryQuantity() const
	{
		return m_iUniqueLuxuryQuantity;
	}
	int GetAllyCityStateCombatModifier() const
	{
		return m_iAllyCityStateCombatModifier;
	}
	int GetAllyCityStateCombatModifierMax() const
	{
		return m_iAllyCityStateCombatModifierMax;
	}
	int GetAdequateLuxuryCompleteQuestInfluenceModifier() const
	{
		return m_iAdequateLuxuryCompleteQuestInfluenceModifier;
	}
		int GetAdequateLuxuryCompleteQuestInfluenceModifierMax() const
	{
		return m_iAdequateLuxuryCompleteQuestInfluenceModifierMax;
	}
	int GetWorkerSpeedModifier() const
	{
		return m_iWorkerSpeedModifier;
	}
	int GetAfraidMinorPerTurnInfluence() const
	{
		return m_iAfraidMinorPerTurnInfluence;
	}
	int GetLandTradeRouteRangeBonus() const
	{
		return m_iLandTradeRouteRangeBonus;
	}
	int GetGoldenAgeMinorPerTurnInfluence() const
	{
		return m_iGoldenAgeMinorPerTurnInfluence;
	}
#if defined(MOD_TRAITS_TRADE_ROUTE_BONUSES)
	int GetSeaTradeRouteRangeBonus() const
	{
		return m_iSeaTradeRouteRangeBonus;
	}
#endif
	int GetTradeReligionModifier() const
	{
		return m_iTradeReligionModifier;
	}
	int GetTradeBuildingModifier() const
	{
		return m_iTradeBuildingModifier;
	}

#if defined(MOD_TRAIT_NEW_EFFECT_FOR_SP)
	int GetNumFreeWorldWonderPerCity() const
	{
		return m_iNumFreeWorldWonderPerCity;
	}
	int GetExceedingHappinessImmigrationModifier() const
	{
		return m_iExceedingHappinessImmigrationModifier;
	}
	bool IsHasCityYieldPerAdjacentFeature() const
	{
		return m_bHasCityYieldPerAdjacentFeature;
	}
	bool IsHasCityAdjacentFeatureModifier() const
	{
		return m_bHasCityAdjacentFeatureModifier;
	}
	int GetPromotionWhenKilledUnit() const
	{
		return m_iPromotionWhenKilledUnit;
	}
	int GetPromotionRadiusWhenKilledUnit() const
	{
		return m_iPromotionRadiusWhenKilledUnit;
	}
	int GetAttackBonusAdjacentWhenUnitKilled() const
	{
		return m_iAttackBonusAdjacentWhenUnitKilled;
	}
	int GetKilledAttackBonusDecreasePerTurn() const
	{
		return m_iKilledAttackBonusDecreasePerTurn;
	}
	int GetTriggersIdeologyTech() const
	{
		return m_iTriggersIdeologyTech;
	}
	int GetNaturalWonderCorruptionScoreChange() const
	{
		return m_iNaturalWonderCorruptionScoreChange;
	}
	int GetNaturalWonderCorruptionRadius() const
	{
		return m_iNaturalWonderCorruptionRadius;
	}
	int GetCultureBonusUnitStrengthModify() const
	{
		return m_iCultureBonusUnitStrengthModify;
	}
	int GetRiverCorruptionScoreChange() const
	{
		return m_iRiverCorruptionScoreChange;
	}
	int GetGreatWorksTourism() const
	{
		return m_iGreatWorksTourism;
	}

	int GetCiviliansFreePromotion() const
	{
		return m_iCiviliansFreePromotion;
	}

	int GetTradeRouteLandGoldBonus() const
	{
		return m_iTradeRouteLandGoldBonus;
	}
	int GetTradeRouteSeaGoldBonus() const
	{
		return m_iTradeRouteSeaGoldBonus;
	}
#endif
	bool IsTrainedAll() const
	{
		return m_bTrainedAll;
	};
	bool IsCanConquerUC() const
	{
		return m_bCanConquerUC;
	};
	bool IsFightWellDamaged() const
	{
		return m_bFightWellDamaged;
	};
	bool IsBuyOwnedTiles() const
	{
		return m_bBuyOwnedTiles;
	};
	bool IsMoveFriendlyWoodsAsRoad() const
	{
		return m_bMoveFriendlyWoodsAsRoad;
	};
	bool IsFasterAlongRiver() const
	{
		return m_bFasterAlongRiver;
	};
	bool IsFasterInHills() const
	{
		return m_bFasterInHills;
	};
	bool IsEmbarkedAllWater() const
	{
		return m_bEmbarkedAllWater;
	};
	bool IsEmbarkedToLandFlatCost() const
	{
		return m_bEmbarkedToLandFlatCost;
	};
	bool IsNoHillsImprovementMaintenance() const
	{
		return m_bNoHillsImprovementMaintenance;
	};
	bool IsTechBoostFromCapitalScienceBuildings() const
	{
		return m_bTechBoostFromCapitalScienceBuildings;
	};
	bool IsArtistGoldenAgeTechBoost() const
	{
		return m_bArtistGoldenAgeTechBoost;
	};
	bool IsStaysAliveZeroCities() const
	{
		return m_bStaysAliveZeroCities;
	};
	bool IsFaithFromUnimprovedForest() const
	{
		return m_bFaithFromUnimprovedForest;
	};
#if defined(MOD_TRAITS_ANY_BELIEF)
	bool IsAnyBelief() const
	{
		return m_bAnyBelief;
	};
#endif
	bool IsGoldenAgeOnWar() const
	{
		return m_bGoldenAgeOnWar;
	};
	bool IsNoResistance() const
	{
		return m_bNoResistance;
	};
	bool IsBonusReligiousBelief() const
	{
		return m_bBonusReligiousBelief;
	};
	bool IsAbleToAnnexCityStates() const
	{
		return m_bAbleToAnnexCityStates;
	};
	bool IsAbleToDualEmpire() const
	{
		return m_bAbleToDualEmpire;
	};
	bool IsNoDoDeficit() const
	{
		return m_bNoDoDeficit;
	};
	bool IsCrossesMountainsAfterGreatGeneral() const
	{
		return m_bCrossesMountainsAfterGreatGeneral;
	};
#if defined(MOD_TRAITS_CROSSES_ICE)
	bool IsCrossesIce() const
	{
		return m_bCrossesIce;
	};
#endif
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS)
	bool IsGGFromBarbarians() const
	{
		return m_bGGFromBarbarians;
	};
#endif
	bool IsMayaCalendarBonuses() const
	{
		return m_bMayaCalendarBonuses;
	};
	bool IsNoAnnexing() const
	{
		return m_bNoAnnexing;
	};
	bool IsTechFromCityConquer() const
	{
		return m_bTechFromCityConquer;
	};
	bool IsUniqueLuxuryRequiresNewArea() const
	{
		return m_bUniqueLuxuryRequiresNewArea;
	}
	bool WillGetUniqueLuxury(CvArea *pArea) const;
	bool IsRiverTradeRoad() const
	{
		return m_bRiverTradeRoad;
	}
	bool IsAngerFreeIntrusionOfCityStates() const
	{
		return m_bAngerFreeIntrusionOfCityStates;
	}
	int GetExtraYieldThreshold(YieldTypes eYield) const
	{
		return m_iExtraYieldThreshold[(int)eYield];
	};
	int GetFreeCityYield(YieldTypes eYield) const
	{
		return m_iFreeCityYield[(int)eYield];
	};
	int GetYieldChangeStrategicResources(YieldTypes eYield) const
	{
		return m_iYieldChangeStrategicResources[(int)eYield];
	};
	int GetYieldChangeNaturalWonder(YieldTypes eYield) const
	{
		return m_iYieldChangeNaturalWonder[(int)eYield];
	};
	int GetYieldChangePerTradePartner(YieldTypes eYield) const
	{
		return m_iYieldChangePerTradePartner[(int)eYield];
	};
	int GetYieldChangeIncomingTradeRoute(YieldTypes eYield) const
	{
		return m_iYieldChangeIncomingTradeRoute[(int)eYield];
	};
	int GetYieldRateModifier(YieldTypes eYield) const
	{
		return m_iYieldRateModifier[(int)eYield];
	};
	int GetGoldenAgeYieldRateModifier(YieldTypes eYield) const
	{
		return m_iGoldenAgeYieldRateModifier[(int)eYield];
	};
	int GetStrategicResourceQuantityModifier(TerrainTypes eTerrain) const
	{
		return m_iStrategicResourceQuantityModifier[(int)eTerrain];
	};
	int GetResourceQuantityModifier(ResourceTypes eResource) const
	{
		return ((uint)eResource < m_aiResourceQuantityModifier.size())?m_aiResourceQuantityModifier[(int)eResource]:0;
	};
	int GetMovesChangeUnitCombat(const int unitCombatID) const;
	int GetMaintenanceModifierUnitCombat(const int unitCombatID) const;
	int GetImprovementYieldChange(ImprovementTypes eImprovement, YieldTypes eYield) const;
#if defined(MOD_API_UNIFIED_YIELDS) && defined(MOD_API_PLOT_YIELDS)
	int GetPlotYieldChange(PlotTypes ePlot, YieldTypes eYield) const;
#endif
#if defined(MOD_API_UNIFIED_YIELDS)
	int GetBuildingClassYieldChange(BuildingClassTypes eBuildingClass, YieldTypes eYield) const;
	int GetCapitalYieldChanges(YieldTypes eYield) const
	{
		return m_iCapitalYieldChanges[(int)eYield];
	};
	int GetCityYieldChanges(YieldTypes eYield) const
	{
		return m_iCityYieldChanges[(int)eYield];
	};
	int GetCoastalCityYieldChanges(YieldTypes eYield) const
	{
		return m_iCoastalCityYieldChanges[(int)eYield];
	};
	int GetGreatWorkYieldChanges(YieldTypes eYield) const
	{
		return m_iGreatWorkYieldChanges[(int)eYield];
	};
	int GetFeatureYieldChange(FeatureTypes eFeature, YieldTypes eYield) const;
	int GetResourceYieldChange(ResourceTypes eResource, YieldTypes eYield) const;
	int GetTerrainYieldChange(TerrainTypes eTerrain, YieldTypes eYield) const;
	int GetYieldFromKills(YieldTypes eYield) const;
	int GetYieldFromBarbarianKills(YieldTypes eYield) const;
	int GetYieldChangeTradeRoute(YieldTypes eYield) const
	{
		return m_iYieldChangeTradeRoute[(int)eYield];
	};
	int GetYieldChangeWorldWonder(YieldTypes eYield) const
	{
		return m_iYieldChangeWorldWonder[(int)eYield];
	};
	int GetRiverPlotYieldChanges(YieldTypes eYield) const
	{
		return m_iRiverPlotYieldChanges[(int)eYield];
	};
	int GetTradeRouteYieldChange(DomainTypes eDomain, YieldTypes eYield) const;
#endif
	int GetSpecialistYieldChange(SpecialistTypes eSpecialist, YieldTypes eYield) const;
#if defined(MOD_API_UNIFIED_YIELDS)
	int GetGreatPersonExpendedYield(GreatPersonTypes eGreatPerson, YieldTypes eYield) const;
	int GetGoldenAgeGreatPersonRateModifier(GreatPersonTypes eGreatPerson) const;
	int GetCityYieldFromUnimprovedFeature(FeatureTypes eFeature, YieldTypes eYield) const;
#endif
	int GetUnimprovedFeatureYieldChange(FeatureTypes eFeature, YieldTypes eYield) const;
	int GetCityYieldModifierFromAdjacentFeature(FeatureTypes eFeature, YieldTypes eYield) const;
	int GetCityYieldPerAdjacentFeature(FeatureTypes eFeature, YieldTypes eYield) const;
	FreeResourceXCities GetFreeResourceXCities(ResourceTypes eResource) const;

	bool HasFreePromotionUnitCombat(const int promotionID, const int unitCombatID) const;
#if defined(MOD_TRAIT_NEW_EFFECT_FOR_SP)
	bool HasFreePromotionUnitClass(const int promotionID, const int unitClassID) const;
#endif
	int GetBuildingClassFaithCost(BuildingClassTypes eBuildingClass) const;

	// Public functions to make trait-based game state changes
	void AddUniqueLuxuries(CvCity *pCity);
#if defined(MOD_EVENTS_UNIT_CAPTURE)
	bool CheckForBarbarianConversion(CvUnit* pByUnit, CvPlot* pPlot);
#else
	bool CheckForBarbarianConversion(CvPlot* pPlot);
#endif
	int GetCapitalBuildingDiscount(BuildingTypes eBuilding);
	BuildingTypes GetFreeBuilding() const;
	BuildingTypes GetFreeBuildingOnConquest() const;
	void SetDefeatedBarbarianCampGuardType(UnitTypes eType)
	{
		m_eCampGuardType = eType;
	};
	UnitTypes GetDefeatedBarbarianCampGuardType() const
	{
		return m_eCampGuardType;
	};
	void SetCombatBonusImprovementType(ImprovementTypes eType)
	{
		m_eCombatBonusImprovement = eType;
	};
	ImprovementTypes GetCombatBonusImprovementType() const
	{
		return m_eCombatBonusImprovement;
	};
	bool IsAbleToCrossMountains() const;
#if defined(MOD_TRAITS_CROSSES_ICE)
	bool IsAbleToCrossIce() const;
#endif

	bool NoTrain(UnitClassTypes eUnitClassType);

	int GetSeaTradeRouteYieldPerEraTimes100(const YieldTypes eYieldType) const
	{
		return m_piSeaTradeRouteYieldPerEraTimes100[eYieldType];
	}
	int GetSeaTradeRouteYieldTimes100(const YieldTypes eYieldType) const
	{
		return m_piSeaTradeRouteYieldTimes100[eYieldType];
	}

#ifdef MOD_TRAIT_RELIGION_FOLLOWER_EFFECTS
	int GetPerMajorReligionFollowerYieldModifierTimes100(const YieldTypes eYieldType) const
	{
		return m_piPerMajorReligionFollowerYieldModifierTimes100[eYieldType];
	}
	int GetPerMajorReligionFollowerYieldModifierMax(const YieldTypes eYieldType) const
	{
		return m_piPerMajorReligionFollowerYieldModifierMax[eYieldType];
	}
#endif

#ifdef MOD_TRAITS_SPREAD_RELIGION_AFTER_KILLING
	int GetSpreadReligionFromKilledUnitStrengthPercent() const;
	int GetSpreadReligionRadius() const;
#endif

#ifdef MOD_TRAITS_COMBAT_BONUS_FROM_CAPTURED_HOLY_CITY
	int GetInflictDamageChangePerCapturedHolyCity() const;
	int GetDamageChangePerCapturedHolyCity() const;
#endif

#ifdef MOD_TRAITS_SIEGE_BONUS_IF_SAME_RELIGION
	int GetSiegeDamagePercentIfSameReligion() const;
#endif

#ifdef MOD_TRAITS_ENABLE_FAITH_PURCHASE_ALL_COMBAT_UNITS
	int GetFaithPurchaseCombatUnitCostPercent() const;
#endif

	// Maya calendar routines
	bool IsUsingMayaCalendar() const;
	bool IsEndOfMayaLongCount();
	CvString GetMayaCalendarString();
	CvString GetMayaCalendarLongString();
	void ChooseMayaBoost();
	void ComputeMayaDate();
	int GetUnitBaktun(UnitTypes eUnit) const;
	void SetUnitBaktun(UnitTypes eUnit);
	bool IsFreeMayaGreatPersonChoice() const;

#ifdef MOD_TRAITS_CAN_FOUND_MOUNTAIN_CITY
	bool IsCanFoundMountainCity() const;
#endif

#ifdef MOD_TRAITS_CAN_FOUND_COAST_CITY
	bool IsCanFoundCoastCity() const;
#endif

#ifdef MOD_GLOBAL_CORRUPTION
	bool GetCorruptionLevelReduceByOne() const;
	int GetMaxCorruptionLevel() const;
#endif

	int GetFreePolicyWhenFirstConquerMajorCapital() const;
	int GetInstantTourismBombWhenFirstConquerMajorCapital() const;

	int GetUnitMaxHitPointChangePerRazedCityPop() const;
	int GetUnitMaxHitPointChangePerRazedCityPopLimit() const;

	int GetGoldenAgeResearchTotalCostModifier() const;
	int GetGoldenAgeResearchCityCountCostModifier() const;
	int GetOthersTradeBonusModifier() const;

	int GetGoldenAgeGrowThresholdModifier() const;

	int GetShareAllyResearchPercent() const;
	bool CanPurchaseWonderInGoldenAge() const;
	bool CanDiplomaticMarriage() const;
	bool IsWLKDCityNoResearchCost() const;
	bool IsGoodyUnitUpgradeFirst() const;

	// Serialization
	void Read(FDataStream& kStream);
	void Write(FDataStream& kStream);

private:
#if defined(MOD_EVENTS_UNIT_CAPTURE)
	bool ConvertBarbarianCamp(CvUnit* pByUnit, CvPlot* pPlot);
	bool ConvertBarbarianNavalUnit(CvUnit* pByUnit, UnitHandle pUnit);
#else
	bool ConvertBarbarianCamp(CvPlot* pPlot);
	bool ConvertBarbarianNavalUnit(UnitHandle pUnit);
#endif

	CvTraitXMLEntries* m_pTraits;
	CvPlayer* m_pPlayer;

	// Cached data about this player's traits
	int m_iGreatPeopleRateModifier;
	int m_iGreatScientistRateModifier;
	int m_iGreatGeneralRateModifier;
	int m_iGreatGeneralExtraBonus;
	int m_iGreatPersonGiftInfluence;
	int m_iLevelExperienceModifier;
	int m_iMaxGlobalBuildingProductionModifier;
	int m_iMaxTeamBuildingProductionModifier;
	int m_iMaxPlayerBuildingProductionModifier;
	int m_iCityUnhappinessModifier;
	int m_iPopulationUnhappinessModifier;
	int m_iCityStateBonusModifier;
	int m_iCityStateFriendshipModifier;
	int m_iCityStateCombatModifier;
	int m_iLandBarbarianConversionPercent;
	int m_iLandBarbarianConversionExtraUnits;
	int m_iSeaBarbarianConversionPercent;
	int m_iCapitalBuildingModifier;
	int m_iPlotBuyCostModifier;
#if defined(MOD_TRAITS_CITY_WORKING)
	int m_iCityWorkingChange;
#endif
#if defined(MOD_TRAITS_CITY_AUTOMATON_WORKERS)
	int m_iCityAutomatonWorkersChange;
#endif
	int m_iPlotCultureCostModifier;
	int m_iCultureFromKills;
	int m_iFaithFromKills;
	int m_iCityCultureBonus;
	int m_iCapitalThemingBonusModifier;
	int m_iPolicyCostModifier;
	int m_iCityConnectionTradeRouteChange;
	int m_iWonderProductionModifier;
	int m_iPlunderModifier;
	int m_iImprovementMaintenanceModifier;
	int m_iGoldenAgeDurationModifier;
	int m_iGoldenAgeMoveChange;
	int m_iGoldenAgeCombatModifier;
	int m_iGoldenAgeTourismModifier;
	int m_iGoldenAgeGreatArtistRateModifier;
	int m_iGoldenAgeGreatMusicianRateModifier;
	int m_iGoldenAgeGreatWriterRateModifier;
	int m_iExtraEmbarkMoves;
	int m_iNaturalWonderFirstFinderGold;
	int m_iNaturalWonderSubsequentFinderGold;
	int m_iNaturalWonderYieldModifier;
	int m_iNaturalWonderHappinessModifier;
	int m_iNearbyImprovementCombatBonus;
	int m_iNearbyImprovementBonusRange;
	int m_iCultureBuildingYieldChange;
	int m_iCombatBonusVsHigherTech;
	int m_iAwayFromCapitalCombatModifier;
	int m_iAwayFromCapitalCombatModifierMax;
	int m_iInfluenceFromGreatPeopleBirth;
	int m_iWLKDLengthChangeModifier;
	int m_iCombatBonusVsLargerCiv;
	int m_iLandUnitMaintenanceModifier;
	int m_iNavalUnitMaintenanceModifier;
	int m_iRazeSpeedModifier;
	int m_iDOFGreatPersonModifier;
	int m_iLuxuryHappinessRetention;
#if defined(MOD_TRAITS_EXTRA_SUPPLY)
	int m_iExtraSupply;
	int m_iExtraSupplyPerCity;
	int m_iExtraSupplyPerPopulation;
#endif
	int m_iExtraSpies;
	int m_iUnresearchedTechBonusFromKills;
	int m_iExtraFoundedCityTerritoryClaimRange;
	int m_iFreeSocialPoliciesPerEra;
	int m_iFreeGreatPeoplePerEra;
	int m_iExtraUnitPlayerInstances;
	int m_iConquestCasualtiesModifier;
	int m_iOwnedReligionUnitCultureExtraTurns;
	int m_iNumTradeRoutesModifier;
	int m_iTradeRouteResourceModifier;
	int m_iUniqueLuxuryCities;
	int m_iUniqueLuxuryQuantity;
	int m_iAllyCityStateCombatModifier;
	int m_iAllyCityStateCombatModifierMax;
	int m_iUniqueLuxuryCitiesPlaced;
	int	m_iAdequateLuxuryCompleteQuestInfluenceModifier;
	int m_iAdequateLuxuryCompleteQuestInfluenceModifierMax;
	int m_iWorkerSpeedModifier;
	int m_iAfraidMinorPerTurnInfluence; 
	int m_iLandTradeRouteRangeBonus;
	int m_iGoldenAgeMinorPerTurnInfluence;
#if defined(MOD_TRAITS_TRADE_ROUTE_BONUSES)
	int m_iSeaTradeRouteRangeBonus;
#endif
	int m_iTradeReligionModifier;
	int m_iTradeBuildingModifier;
#if defined(MOD_TRAIT_NEW_EFFECT_FOR_SP)
	int m_iExceedingHappinessImmigrationModifier;
	int m_iNumFreeWorldWonderPerCity;
	bool m_bHasCityYieldPerAdjacentFeature = false;
	bool m_bHasCityAdjacentFeatureModifier = false;
	int m_iPromotionWhenKilledUnit;
	int m_iPromotionRadiusWhenKilledUnit;
	int m_iAttackBonusAdjacentWhenUnitKilled;
	int m_iKilledAttackBonusDecreasePerTurn;
	int m_iTriggersIdeologyTech;
	int m_iNaturalWonderCorruptionScoreChange;
	int m_iNaturalWonderCorruptionRadius;
	int m_iCultureBonusUnitStrengthModify;
	int m_iRiverCorruptionScoreChange;
	int m_iGreatWorksTourism;
	int m_iCiviliansFreePromotion;
	int m_iTradeRouteLandGoldBonus;
	int m_iTradeRouteSeaGoldBonus;
#endif
	// Saved
	bool m_bTrainedAll = false;
	bool m_bCanConquerUC = false;
	bool m_bFightWellDamaged = false;
	bool m_bBuyOwnedTiles = false;
	bool m_bMoveFriendlyWoodsAsRoad = false;
	bool m_bFasterAlongRiver = false;
	bool m_bFasterInHills = false;
	bool m_bEmbarkedAllWater = false;
	bool m_bEmbarkedToLandFlatCost = false;
	bool m_bNoHillsImprovementMaintenance = false;
	bool m_bTechBoostFromCapitalScienceBuildings = false;
	bool m_bArtistGoldenAgeTechBoost = false;
	bool m_bStaysAliveZeroCities = false;
	bool m_bFaithFromUnimprovedForest = false;
#if defined(MOD_TRAITS_ANY_BELIEF)
	bool m_bAnyBelief;
#endif
	bool m_bGoldenAgeOnWar = false;
	bool m_bNoResistance = false;
	bool m_bBonusReligiousBelief = false;
	bool m_bAbleToAnnexCityStates = false;
	bool m_bAbleToDualEmpire = false;
	bool m_bNoDoDeficit = false;
	bool m_bCrossesMountainsAfterGreatGeneral = false;
#if defined(MOD_TRAITS_CROSSES_ICE)
	bool m_bCrossesIce = false;
#endif
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS)
	bool m_bGGFromBarbarians = false;
#endif
	bool m_bMayaCalendarBonuses = false;
	bool m_bNoAnnexing = false;
	bool m_bTechFromCityConquer = false;
	bool m_bUniqueLuxuryRequiresNewArea = false;
	bool m_bRiverTradeRoad = false;
	bool m_bAngerFreeIntrusionOfCityStates = false;

#ifdef MOD_TRAITS_CAN_FOUND_MOUNTAIN_CITY
	bool m_bCanFoundMountainCity = false;
#endif
#ifdef MOD_TRAITS_CAN_FOUND_COAST_CITY
	bool m_bCanFoundCoastCity = false;
#endif

	UnitTypes m_eCampGuardType;
	unsigned int m_uiFreeUnitIndex;
	TechTypes m_eFreeUnitPrereqTech;
	ImprovementTypes m_eCombatBonusImprovement;
	BuildingTypes m_eFreeBuilding;
	BuildingTypes m_eFreeBuildingOnConquest;

	int m_iExtraYieldThreshold[NUM_YIELD_TYPES];
	int m_iFreeCityYield[NUM_YIELD_TYPES];
	int m_iYieldChangeStrategicResources[NUM_YIELD_TYPES];
	int m_iYieldChangeNaturalWonder[NUM_YIELD_TYPES];
	int m_iYieldChangePerTradePartner[NUM_YIELD_TYPES];
	int m_iYieldChangeIncomingTradeRoute[NUM_YIELD_TYPES];
	int m_iYieldRateModifier[NUM_YIELD_TYPES];
	int m_iGoldenAgeYieldRateModifier[NUM_YIELD_TYPES];
	int m_iStrategicResourceQuantityModifier[NUM_TERRAIN_TYPES];
	std::vector<int> m_aiResourceQuantityModifier;
	std::vector<bool> m_abNoTrain;
	FStaticVector<FreeTraitUnit, SAFE_ESTIMATE_NUM_FREE_UNITS, true, c_eCiv5GameplayDLL, 0> m_aFreeTraitUnits;
	std::vector<int> m_aiBuildingClassFaithCost;
	std::vector<int> m_aUniqueLuxuryAreas;

	// Maya calendar bonus data
	int m_iBaktunPreviousTurn;  // Saved
	int m_iBaktun;	 // Not saved
	int m_iKatun;    // Not saved
	int m_iTun;      // Not saved
	int m_iWinal;    // Not saved
	int m_iKin;      // Not saved
	std::vector<MayaBonusChoice> m_aMayaBonusChoices;  // Saved

	std::vector<int> m_paiMovesChangeUnitCombat;
	std::vector<int> m_paiMaintenanceModifierUnitCombat;

	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppaaiImprovementYieldChange;
#if defined(MOD_API_UNIFIED_YIELDS) && defined(MOD_API_PLOT_YIELDS)
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiPlotYieldChange;
#endif
#if defined(MOD_API_UNIFIED_YIELDS)
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiBuildingClassYieldChange;
	int m_iCityYieldChanges[NUM_YIELD_TYPES];
	int m_iCapitalYieldChanges[NUM_YIELD_TYPES];
	int m_iCoastalCityYieldChanges[NUM_YIELD_TYPES];
	int m_iGreatWorkYieldChanges[NUM_YIELD_TYPES];
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiFeatureYieldChange;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiResourceYieldChange;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiTerrainYieldChange;
	int m_iYieldFromKills[NUM_YIELD_TYPES];
	int m_iYieldFromBarbarianKills[NUM_YIELD_TYPES];
	int m_iYieldChangeTradeRoute[NUM_YIELD_TYPES];
	int m_iYieldChangeWorldWonder[NUM_YIELD_TYPES];
	int m_iRiverPlotYieldChanges[NUM_YIELD_TYPES];
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiTradeRouteYieldChange;
#endif
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppaaiSpecialistYieldChange;
#if defined(MOD_API_UNIFIED_YIELDS)
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiGreatPersonExpendedYield;
	std::vector<int> m_piGoldenAgeGreatPersonRateModifier;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiCityYieldFromUnimprovedFeature;
#endif
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppaaiUnimprovedFeatureYieldChange;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiCityYieldModifierFromAdjacentFeature;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiCityYieldPerAdjacentFeature;

	std::vector<FreeResourceXCities> m_aFreeResourceXCities;

	int m_piSeaTradeRouteYieldPerEraTimes100[NUM_YIELD_TYPES];
	int m_piSeaTradeRouteYieldTimes100[NUM_YIELD_TYPES];

#ifdef MOD_TRAIT_RELIGION_FOLLOWER_EFFECTS
	int m_piPerMajorReligionFollowerYieldModifierTimes100[NUM_YIELD_TYPES];
	int m_piPerMajorReligionFollowerYieldModifierMax[NUM_YIELD_TYPES];
#endif

#ifdef MOD_TRAITS_SPREAD_RELIGION_AFTER_KILLING
	int m_iSpreadReligionFromKilledUnitStrengthPercent = 0;
	int m_iSpreadReligionRadius = 0;
#endif

#ifdef MOD_TRAITS_COMBAT_BONUS_FROM_CAPTURED_HOLY_CITY
	int m_iInflictDamageChangePerCapturedHolyCity = 0;
	int m_iDamageChangePerCapturedHolyCity = 0;
#endif

#ifdef MOD_TRAITS_SIEGE_BONUS_IF_SAME_RELIGION
	int m_iSiegeDamagePercentIfSameReligion = 0;
#endif

#ifdef MOD_TRAITS_ENABLE_FAITH_PURCHASE_ALL_COMBAT_UNITS
	int m_iFaithPurchaseCombatUnitCostPercent = 0;
#endif

#ifdef MOD_GLOBAL_CORRUPTION
	bool m_bCorruptionLevelReduceByOne = 0;
	int m_iMaxCorruptionLevel = -1;
#endif

	int m_iFreePolicyWhenFirstConquerMajorCapital = 0;
	int m_iInstantTourismBombWhenFirstConquerMajorCapital = 0;

	int m_iUnitMaxHitPointChangePerRazedCityPop = 0;
	int m_iUnitMaxHitPointChangePerRazedCityPopLimit = 0;

	int m_iGoldenAgeResearchTotalCostModifier= 0;
	int m_iGoldenAgeResearchCityCountCostModifier = 0;
	int m_iOthersTradeBonusModifier = 0;

	int m_iGoldenAgeGrowThresholdModifier = 0;

	int m_iShareAllyResearchPercent = 0;
	bool m_bCanPurchaseWonderInGoldenAge = false;
	bool m_bCanDiplomaticMarriage = false;
	bool m_bWLKDCityNoResearchCost = false;
	bool m_bGoodyUnitUpgradeFirst = false;
};

#endif //CIV5_TRAIT_CLASSES_H
