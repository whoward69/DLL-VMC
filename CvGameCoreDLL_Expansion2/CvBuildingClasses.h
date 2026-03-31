/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#pragma once

#ifndef CIV5_BUILDING_CLASSES_H
#define CIV5_BUILDING_CLASSES_H

#include "CvDatabaseUtility.h"
#include <array>

#define MAX_THEMING_BONUSES 12

#ifdef MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD
enum YieldFromYield {
	IN_VALUE = 0,
	OUT_VALUE,
	LENGTH,
};
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvThemingBonusInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvThemingBonusInfo
{
	friend class CvBuildingEntry;

public:
	CvThemingBonusInfo() :
	  m_iBonus(0),
	  m_bSameEra(false),
	  m_bUniqueEras(false),
#if defined(MOD_API_EXTENSIONS)
	  m_bConsecutiveEras(false),
#endif
	  m_bMustBeArt(false),
	  m_bMustBeArtifact(false),
	  m_bMustBeEqualArtArtifact(false),
	  m_bRequiresOwner(false),
	  m_bRequiresAnyButOwner(false),
	  m_bRequiresSamePlayer(false),
	  m_bRequiresUniquePlayers(false),
	  m_iAIPriority(0)
	  {

	  };

	int GetBonus() {return m_iBonus;};
	CvString GetDescription() {return m_strDescription;};
	bool IsSameEra() {return m_bSameEra;};
#if defined(MOD_API_EXTENSIONS)
	bool IsUniqueEras() {return m_bUniqueEras || IsConsecutiveEras();};
	bool IsConsecutiveEras() {return m_bConsecutiveEras;};
#else
	bool IsUniqueEras() {return m_bUniqueEras;};
#endif
	bool IsMustBeArt() {return m_bMustBeArt;};
	bool IsMustBeArtifact() {return m_bMustBeArtifact;};
	bool IsMustBeEqualArtArtifact() {return m_bMustBeEqualArtArtifact;};
	bool IsRequiresOwner() {return m_bRequiresOwner;};
	bool IsRequiresAnyButOwner() {return m_bRequiresAnyButOwner;};
	bool IsRequiresSamePlayer() {return m_bRequiresSamePlayer;};
	bool IsRequiresUniquePlayers() {return m_bRequiresUniquePlayers;};
	int GetAIPriority() {return m_iAIPriority;};

protected:
	int m_iBonus;
	CvString m_strDescription;
	bool m_bSameEra;
	bool m_bUniqueEras;
#if defined(MOD_API_EXTENSIONS)
	bool m_bConsecutiveEras;
#endif
	bool m_bMustBeArt;
	bool m_bMustBeArtifact;
	bool m_bMustBeEqualArtArtifact;
	bool m_bRequiresOwner;
	bool m_bRequiresAnyButOwner;
	bool m_bRequiresSamePlayer;
	bool m_bRequiresUniquePlayers;
	int m_iAIPriority;
};


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvDoubleYieldInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvDoubleYieldInfo
{
	friend class CvBuildingEntry;

public:
	CvDoubleYieldInfo() :
		m_iYieldIn(NO_YIELD),
		m_iYieldOut(NO_YIELD),
		m_iValue(0)
	{

	};

	YieldTypes GetYieldIn() { return m_iYieldIn; };
	YieldTypes GetYieldOut() { return m_iYieldOut; };
	int GetValue() const { return m_iValue; };

protected:
	YieldTypes m_iYieldIn;
	YieldTypes m_iYieldOut;
	int m_iValue;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvBuildingEntry
//!  \brief		A single building available in the game
//
//!  Key Attributes:
//!  - Used to be called CvBuildingInfo
//!  - Populated from XML\Buildings\CIV5BuildingInfos.xml
//!  - Array of these contained in CvBuildingXMLEntries class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvBuildingEntry: public CvBaseInfo
{
public:
	CvBuildingEntry(void);
	~CvBuildingEntry(void);

	virtual bool CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility);

	// Accessor Functions (Non-Arrays)
	int GetBuildingClassType() const;
	const CvBuildingClassInfo& GetBuildingClassInfo() const;

	int GetNearbyTerrainRequired() const;
	int GetProhibitedCityTerrain() const;
	int GetVictoryPrereq() const;
	int GetFreeStartEra() const;
	int GetMaxStartEra() const;
	int GetObsoleteTech() const;
	int GetEnhancedYieldTech() const;
	int GetTechEnhancedTourism() const;
	int GetGoldMaintenance() const;
	int GetReplacementBuildingClass() const;
	int GetPrereqAndTech() const;
	int GetTechNoPrereqClasses() const;
	int GetPolicyBranchType() const;
	int GetPolicyNeededType() const;
	bool IsPuppetPurchaseOverride() const;
	bool IsAllowsPuppetPurchase() const;
	int GetSpecialistType() const;
	int GetSpecialistCount() const;
	int GetSpecialistExtraCulture() const;
	int GetGreatPeopleRateChange() const;
	GreatWorkSlotType GetGreatWorkSlotType() const;
#if defined(MOD_GLOBAL_GREATWORK_YIELDTYPES)
	YieldTypes GetGreatWorkYieldType() const;
#endif
	int GetGreatWorkCount() const;
	GreatWorkType GetFreeGreatWork() const;
	int GetFreeBuildingClass() const;
	int GetFreeBuildingThisCity() const;
	int GetFreePromotion() const;
	int GetFreePromotion2() const;
	int GetFreePromotion3() const;
	int GetTrainedFreePromotion() const;
	int GetFreePromotionRemoved() const;
	int GetProductionCost() const;
	int GetFaithCost() const;
	int GetLeagueCost() const;
	int GetNumCityCostMod() const;
	int GetHurryCostModifier() const;
	int GetNumCitiesPrereq() const;
	int GetUnitLevelPrereq() const;
	int GetCultureRateModifier() const;
	int GetGlobalCultureRateModifier() const;
	int GetGreatPeopleRateModifier() const;
	int GetGlobalGreatPeopleRateModifier() const;
	int GetGreatGeneralRateModifier() const;
	int GetGreatPersonExpendGold() const;
	int GetUnitUpgradeCostMod() const;
	int GetGoldenAgeModifier() const;
	int GetFreeExperience() const;
	int GetGlobalFreeExperience() const;
	int GetFoodKept() const;
	bool IsAirlift() const;
	int GetAirModifier() const;
	int GetNukeModifier() const;
	int GetNukeExplosionRand() const;
	int GetWorkerSpeedModifier() const;
	int GetMilitaryProductionModifier() const;
	int GetSpaceProductionModifier() const;
	int GetGlobalSpaceProductionModifier() const;
	int GetBuildingProductionModifier() const;
	int GetWonderProductionModifier() const;
	int GetCityConnectionTradeRouteModifier() const;
	int GetCapturePlunderModifier() const;
	int GetPolicyCostModifier() const;
	int GetGlobalPlotCultureCostModifier() const;
	int GetPlotCultureCostModifier() const;
	int GetGlobalPlotBuyCostModifier() const;
	int GetPlotBuyCostModifier() const;
#if defined(MOD_BUILDINGS_CITY_WORKING)
	int GetGlobalCityWorkingChange() const;
	int GetCityWorkingChange() const;
#endif
#if defined(MOD_BUILDINGS_CITY_AUTOMATON_WORKERS)
	int GetGlobalCityAutomatonWorkersChange() const;
	int GetCityAutomatonWorkersChange() const;
#endif

#if defined(MOD_GLOBAL_BUILDING_INSTANT_YIELD)
	int GetInstantYield(int i) const;
	int* GetInstantYieldArray() const;
	bool IsAllowInstantYield() const;
#endif

#if defined(MOD_ROG_CORE)
	int GetGreatWorkYieldChange(int i) const;
	int* GetGreatWorkYieldChangeArray() const;

	int GetYieldChangePerPopInEmpire(int i) const;
#endif

	int  GetExtraAttacks() const;
	int GetNukeInterceptionChance() const;

#if defined(MOD_ROG_CORE)
	int GetExtraDamageHealPercent() const;
	int GetExtraDamageHeal() const;
	int GetBombardRange() const;
	int IsBombardIndirect() const;
	int CityRangedStrikeModifier() const;
	int GetPopulationChange() const;
	int GetMinorCivFriendship() const;
	int GetLiberatedInfluence() const;
	int GetExtraUnitPlayerInstances() const;
	int GetGlobalCityStrengthMod() const;
	int GetGlobalRangedStrikeModifier() const;
	int GetResearchTotalCostModifier() const;
	int GetResearchTotalCostModifierGoldenAge() const;
	int GetResetDamageValue() const;
	int GetReduceDamageValue() const;

	int GetWaterTileDamage() const;
	int GetWaterTileMovementReduce() const;
	int GetWaterTileTurnDamage() const;
	int GetLandTileDamage() const;
	int GetLandTileMovementReduce() const;
	int GetLandTileTurnDamage() const;


	int GetWaterTileDamageGlobal() const;
	int GetWaterTileMovementReduceGlobal() const;
	int GetWaterTileTurnDamageGlobal() const;
	int GetLandTileDamageGlobal() const;
	int GetLandTileMovementReduceGlobal() const;
	int GetLandTileTurnDamageGlobal() const;
#endif

	int GetYieldPerFriend(int i) const;
	int GetYieldPerAlly(int i) const;
	int GetResourceQuantityFromPOP(int i) const;
	int GetYieldPerEspionageSpy(int i) const;
	int GetYieldChangeWorldWonder(int i) const;
	int GetYieldChangeWorldWonderGlobal(int i) const;
	int GetYieldFromInternal(int i) const;
	int* GetYieldFromInternalArray() const;
	int GetYieldFromProcessModifier(int i) const;
	int* GetYieldFromProcessModifierArray() const;
	int GetYieldFromProcessModifierGlobal(int i) const;
	int* GetYieldFromProcessModifierArrayGlobal() const;
	int GetResourceYieldChangeGlobal(int iResource, int iYieldType) const;
	int GetImprovementYieldChange(int i, int j) const;
	int* GetImprovementYieldChangeArray(int i) const;

	int GetImprovementYieldChangeGlobal(int i, int j) const;
	int* GetImprovementYieldChangeGlobalArray(int i) const;

	int GetFeatureYieldChangesGlobal(int i, int j) const;
	int GetTerrainYieldChangesGlobal(int i, int j) const;

	int GetYieldPerXTerrain(int i, int j) const;
	int* GetYieldPerXTerrainArray(int i) const;

	int GetYieldPerXFeature(int i, int j) const;
	int* GetYieldPerXFeatureArray(int i) const;

	int GetSpecialistYieldChangeLocal(int i, int j) const;
	int* GetSpecialistYieldChangeLocalArray(int i) const;

	int GetBuildingClassYieldModifier(int i, int j) const;
	int GetBuildingClassLocalYieldChange(int i, int j) const;

#if defined(MOD_ROG_CORE)
	int GetSpecificGreatPersonRateModifier(int) const;
#endif

	int GetMinAreaSize() const;
	int GetConquestProbability() const;
	int GetHealRateChange() const;
	int GetHappiness() const;
	int GetUnmoddedHappiness() const;
	int GetUnhappinessModifier() const;
	int GetHappinessPerCity() const;
	int GetHappinessPerXPolicies() const;
	int GetCityCountUnhappinessMod() const;
	bool IsNoOccupiedUnhappiness() const;
	bool IsNotNeedOccupied() const;
	bool IsAllowSpaceshipLaunch() const;
	int GetGlobalPopulationChange() const;
	int GetTechShare() const;
	int GetFreeTechs() const;
	int GetFreePolicies() const;
	int GetFreeGreatPeople() const;
	int GetMedianTechPercentChange() const;
	int GetGold() const;
	bool IsNearbyMountainRequired() const;
	bool IsAllowsRangeStrike() const;
	int GetDefenseModifier() const;
	int GetGlobalDefenseModifier() const;
	int GetExtraCityHitPoints() const;
	int GetMinorFriendshipChange() const;
	int GetMinorFriendshipAnchorChange() const;
	int GetMinorQuestFriendshipMod() const;
	int GetVictoryPoints() const;
	int GetExtraMissionarySpreads() const;
	int GetReligiousPressureModifier() const;
	int GetEspionageModifier() const;
	int GetGlobalEspionageModifier() const;
	int GetGlobalEspionageSpeedModifier() const;
	int GetExtraSpies() const;
	int GetSpyRankChange() const;
	int GetTradeRouteRecipientBonus() const;
	int GetTradeRouteTargetBonus() const;
	int GetNumTradeRouteBonus() const;
	int GetInstantSpyRankChange() const;
#if defined(MOD_RELIGION_CONVERSION_MODIFIERS)
	int GetConversionModifier() const;
	int GetGlobalConversionModifier() const;
#endif
	int GetLandmarksTourismPercent() const;
	int GetInstantMilitaryIncrease() const;
	int GetGreatWorksTourismModifier() const;
	int GetXBuiltTriggersIdeologyChoice() const;
	int GetTradeRouteSeaDistanceModifier() const;
	int GetTradeRouteSeaGoldBonus() const;
	int GetTradeRouteLandDistanceModifier() const;
	int GetTradeRouteLandGoldBonus() const;
	int GetGreatScientistBeakerModifier() const;
	int GetExtraLeagueVotes() const;
	int GetPreferredDisplayPosition() const;
	int GetPortraitIndex() const;
	bool IsTeamShare() const;
	bool IsWater() const;
	bool IsRiver() const;
	bool IsFreshWater() const;
#if defined(MOD_VOLCANO_BREAK)
	bool IsImmueVolcanoDamage() const;
#endif
	bool IsAddsFreshWater() const;
	bool IsPurchaseOnly() const;
	bool IsHumanOnly() const;
	bool IsMoveAfterCreated() const;
	bool IsMountain() const;
	bool IsHill() const;
	bool IsFlat() const;
	bool IsFoundsReligion() const;
	bool IsReligious() const;
	bool IsBorderObstacle() const;
	bool IsPlayerBorderObstacle() const;
	bool IsCityWall() const;
	bool IsCapital() const;
	bool IsGoldenAge() const;
	bool IsMapCentering() const;
	bool IsNeverCapture() const;
	bool IsNukeImmune() const;
	bool IsExtraAttackOnKill() const;
	bool IsForbiddenForeignSpy() const;
	bool IsForbiddenForeignSpyGlobal() const;
	bool IsExtraLuxuries() const;
	bool IsDiplomaticVoting() const;
	bool AllowsWaterRoutes() const;
	bool IsScienceBuilding() const;
	bool IsUnlockedByBelief() const;
	bool IsUnlockedByLeague() const;
	bool IsRequiresHolyCity() const;
	bool AffectSpiesNow() const;
	bool IsEspionage() const;
	bool AllowsFoodTradeRoutes() const;
	bool AllowsProductionTradeRoutes() const;
	bool NullifyInfluenceModifier() const;
	const char* GetArtDefineTag() const;
	void SetArtDefineTag(const char* szVal);
	const bool GetArtInfoCulturalVariation() const;
	const bool GetArtInfoEraVariation() const;
	const bool GetArtInfoRandomVariation() const;

	const char* GetWonderSplashAudio() const;
	CvString GetThemingBonusHelp() const;

	// Accessor Functions (Arrays)

	int GetYieldChange(int i) const;
	int* GetYieldChangeArray() const;
	int GetYieldChangePerEra(int i) const;
	int* GetYieldChangePerEraArray() const;
	int GetYieldModifierChangePerEra(int i) const;
	int* GetYieldModifierChangePerEraArray() const;
	int GetCityStateTradeRouteYieldModifier(int i) const;
	int GetCityStateTradeRouteYieldModifierGlobal(int i) const;
	int GetYieldChangePerPop(int i) const;
	int* GetYieldChangePerPopArray() const;
	int GetYieldChangePerReligion(int i) const;
	int* GetYieldChangePerReligionArray() const;
	int GetYieldModifier(int i) const;;
	int* GetYieldModifierArray() const;
	int GetYieldMultiplier(int i) const;;
	int* GetYieldMultiplierArray() const;

#if defined(MOD_ROG_CORE)
	int GetYieldFromConstruction(int i) const;
	int* GetYieldFromConstructionArray() const;
	int GetYieldFromUnitProduction(int i) const;
	int* GetYieldFromUnitProductionArray() const;
	int GetYieldFromBirth(int i) const;
	int* GetYieldFromBirthArray() const;
	int GetYieldFromBorderGrowth(int i) const;
	int* GetYieldFromBorderGrowthArray() const;

	int GetYieldFromPillage(int i) const;
	int* GetYieldFromPillageArray() const;
	int GetYieldFromPillageGlobal(int i) const;
	int* GetYieldFromPillageGlobalArray() const;
	int GetYieldFromPillageGlobalPlayer(int i) const;
	int* GetYieldFromPillageGlobalPlayerArray() const;

	int GetYieldModifierFromWonder(int i) const;;
	int* GetYieldModifierFromWonderArray() const;
#endif

	int GetAreaYieldModifier(int i) const;
	int* GetAreaYieldModifierArray() const;
	int GetGlobalYieldModifier(int i) const;
	int* GetGlobalYieldModifierArray() const;
	int GetTechEnhancedYieldChange(int i) const;
	int* GetTechEnhancedYieldChangeArray() const;
	int GetSeaPlotYieldChange(int i) const;
	int* GetSeaPlotYieldChangeArray() const;
	int GetRiverPlotYieldChange(int i) const;
	int* GetRiverPlotYieldChangeArray() const;
	int GetRiverPlotYieldChangeGlobal(int i) const;
	int* GetRiverPlotYieldChangeGlobalArray() const;
	int GetLakePlotYieldChange(int i) const;
	int* GetLakePlotYieldChangeArray() const;
	int GetSeaResourceYieldChange(int i) const;
	int* GetSeaResourceYieldChangeArray() const;
	int GetSpecialistCount(int i) const;
	int GetUnitCombatFreeExperience(int i) const;
	int GetUnitCombatProductionModifier(int i) const;
	int GetDomainFreeExperience(int i) const;
	int GetDomainFreeExperiencePerGreatWork(int i) const;
	int GetDomainFreeExperiencesPerPop(int i) const;
	int GetDomainFreeExperiencesPerPopGlobal(int i) const;
	int GetDomainFreeExperiencesPerTurn(int i) const;
	int GetDomainFreeExperiencesPerTurnGlobal(int i) const;
	int GetDomainEnemyCombatModifier(int i) const;
	int GetDomainEnemyCombatModifierGlobal(int i) const;
	int GetDomainFriendsCombatModifierLocal(int i) const;
#if defined(MOD_ROG_CORE)
	int GetDomainFreeExperiencePerGreatWorkGlobal(int i) const;
	int GetDomainFreeExperienceGlobal(int i) const;
	const std::map<int, int>& GetUnitTypePrmoteHealGlobal() const;
#endif
	const std::map<int, int>& GetEraUnitClassMaxInstances() const;

#if defined(MOD_NUCLEAR_WINTER_FOR_SP)
	bool IsNoNuclearWinterLocal() const;
#endif

#if defined(MOD_TROOPS_AND_CROPS_FOR_SP)
	int GetDomainTroops(int i) const;
	int GetNumCrops() const;
	int GetNumArmee() const;
	bool IsEnableCrops() const;
	bool IsEnableArmee() const;
#endif

	int GetDomainProductionModifier(int i) const;
	int GetResourceQuantityRequirement(int i) const;
	int GetResourceQuantity(int i) const;
	int GetResourceCultureChange(int i) const;
	int GetResourceFaithChange(int i) const;
	int GetProductionTraits(int i) const;
	int GetPrereqNumOfBuildingClass(int i) const;
	int GetFlavorValue(int i) const;

	const std::vector<int>& GetLockedByBuildings() const;
	const std::vector<int>& GetPrereqAndTechs() const;
	const std::vector<int>& GetLocalResourceAnd() const;
	const std::vector<int>& GetLocalResourceOr() const;
	const std::vector<int>& GetEmpireResourceAnd() const;
	const std::vector<int>& GetEmpireResourceOr() const;
	const std::vector<int>& GetFeatureOr() const;
	const std::vector<int>& GetFeatureAnd() const;
	const std::vector<int>& GetPlotAnd() const;

	int GetHurryModifier(int i) const;
	int GetHurryModifierLocal(int i) const;
	const std::vector<int>& GetBuildingClassesNeededInCity() const;
	const std::vector<int>& GetBuildingClassesNeededGlobal() const;
	const std::vector<int>& GetBuildingsNeededInCity() const;
	const std::vector<int>& GetBuildingsNeededGlobal() const;
#if defined(MOD_BUILDING_NEW_EFFECT_FOR_SP)
	int GetCityDefenseModifierGlobal() const;
	int GetUnitMaxExperienceLocal() const;
	int GetExtraSellRefund() const;
	int GetExtraSellRefundModifierPerEra() const;
	int GetMinNumReligions() const;
	int GetLandmarksTourismPercentGlobal() const;
	int GetGreatWorksTourismModifierGlobal() const;
	int GetTradeRouteRiverBonusModifier() const;
	int GetTradeRouteSeaGoldBonusGlobal() const;
	int GetTradeRouteLandGoldBonusGlobal() const;
	bool IsAnyWater() const;
	bool IsRiverOrCoastal() const;
#endif
	int GetNumFreeUnit() const;
	int GetNumFreeUnitTotal() const;
	std::pair<UnitTypes, int>* GetFreeUnits() const;

	int GetYieldFromYieldGlobal(int i, int j) const;
	int GetResourceYieldChange(int i, int j) const;
	int* GetResourceYieldChangeArray(int i) const;
	int GetFeatureYieldChange(int i, int j) const;
	int* GetFeatureYieldChangeArray(int i) const;


	int GetTerrainYieldModifier(int i, int j) const;
	int* GetTerrainYieldModifierArray(int i) const;

	int GetSpecialistYieldChange(int i, int j) const;
	int* GetSpecialistYieldChangeArray(int i) const;
	int GetImprovementYieldModifier(int i, int j) const;
	int* GetImprovementYieldModifierArray(int i) const;
	int GetSpecialistYieldModifier(int i, int j) const;
	int* GetSpecialistYieldModifierArray(int i) const;
	int GetSpecialistYieldModifierGlobal(int i, int j) const;
	int* GetSpecialistYieldModifierGlobalArray(int i) const;
	int GetFeatureYieldModifier(int i, int j) const;
	int* GetFeatureYieldModifierArray(int i) const;
	int GetResourceYieldModifier(int i, int j) const;
	int* GetResourceYieldModifierArray(int i) const;
	int GetTerrainYieldChange(int i, int j) const;
	int* GetTerrainYieldChangeArray(int i) const;
#if defined(MOD_API_UNIFIED_YIELDS) && defined(MOD_API_PLOT_YIELDS)
	int GetPlotYieldChange(int i, int j) const;
	int* GetPlotYieldChangeArray(int i) const;
#endif
	int GetBuildingClassYieldChange(int i, int j) const;
	int GetBuildingClassHappiness(int i) const;

#if defined(MOD_BUILDING_IMPROVEMENT_RESOURCES) 
	std::multimap<int, std::pair<int, int>> GetResourceFromImprovementArray() const;
#endif

	CvThemingBonusInfo *GetThemingBonusInfo(int i) const;
	int GetNumThemingBonuses() const {return m_iNumThemingBonuses;};

#ifdef MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD
	int GetYieldFromOtherYield(const YieldTypes eInType, const YieldTypes eOutType, const YieldFromYield eConvertType) const;
	bool HasYieldFromOtherYield() const;
#endif

#ifdef MOD_GLOBAL_CITY_SCALES
	CityScaleTypes GetEnableCityScaleGrowth() const;
	bool GetEnableAllCityScaleGrowth() const;
#endif

#ifdef MOD_BUILDINGS_GOLDEN_AGE_EXTEND
	int GetGoldenAgeUnitCombatModifier() const;
	int GetGoldenAgeMeterMod() const;
#endif

#ifdef MOD_PROMOTION_CITY_DESTROYER
	int GetSiegeKillCitizensModifier() const;
#endif

#if defined(MOD_INTERNATIONAL_IMMIGRATION_FOR_SP)
	bool CanAllScaleImmigrantIn() const;
#endif

#ifdef MOD_GLOBAL_CORRUPTION
	int GetCorruptionScoreChange() const;
	int GetCorruptionLevelChange() const;
	int GetCorruptionPolicyCostModifier() const;
	int GetMinCorruptionLevelNeeded() const;
	int GetMaxCorruptionLevelNeeded() const;
#endif

	int GetGlobalProductionNeededUnitModifier() const;
	int GetGlobalProductionNeededBuildingModifier() const;
	int GetGlobalProductionNeededProjectModifier() const;
	int GetGlobalProductionNeededUnitMax() const;
	int GetGlobalProductionNeededBuildingMax() const;
	int GetGlobalProductionNeededProjectMax() const;
	bool IsDummyBuilding() const;
	bool IsNoPuppet() const;

	int GetInstantResearchFromFriendlyGreatScientist() const;
	int GetGlobalGrowthFoodNeededModifier() const;
	int GetSecondCapitalsExtraScore() const;
	int GetFoodKeptFromPollution() const;

	int GetTradeRouteFromTheCityYields(YieldTypes eYieldTypes) const;
	int GetTradeRouteFromTheCityYieldsPerEra(YieldTypes eYieldTypes) const;

	bool IsCapitalOnly() const;
	bool IsOriginalCapitalOnly() const;

private:
	int m_iBuildingClassType;
	const CvBuildingClassInfo* m_pkBuildingClassInfo;

	int m_iNearbyTerrainRequired;
	int m_iProhibitedCityTerrain;
	int m_iVictoryPrereq;
	int m_iFreeStartEra;
	int m_iMaxStartEra;
	int m_iObsoleteTech;
	int m_iEnhancedYieldTech;
	int m_iTechEnhancedTourism;
	int m_iGoldMaintenance;
	int m_iReplacementBuildingClass;
	int m_iPrereqAndTech;
	int m_iTechNoPrereqClasses;
	int m_iPolicyBranchType;
	int m_iPolicyNeededType;
	bool m_bPuppetPurchaseOverride;
	bool m_bAllowsPuppetPurchase;
	int m_iSpecialistType;
	int m_iSpecialistCount;
	int m_iSpecialistExtraCulture;
	int m_iGreatPeopleRateChange;
	GreatWorkSlotType m_eGreatWorkSlotType;
#if defined(MOD_GLOBAL_GREATWORK_YIELDTYPES)
	YieldTypes m_eGreatWorkYieldType;
#endif
	int m_iGreatWorkCount;
	GreatWorkType m_eFreeGreatWork;
	int m_iFreeBuildingClass;
	int m_iFreeBuildingThisCity;
	int m_iFreePromotion;
	int m_iFreePromotion2;
	int m_iFreePromotion3;
	int m_iTrainedFreePromotion;
	int m_iFreePromotionRemoved;
	int m_iProductionCost;
	int m_iFaithCost;
	int m_iLeagueCost;
	int m_iNumCityCostMod;
	int m_iHurryCostModifier;
	int m_iNumCitiesPrereq;
	int m_iUnitLevelPrereq;
	int m_iCultureRateModifier;
	int m_iGlobalCultureRateModifier;
	int m_iGreatPeopleRateModifier;
	int m_iGlobalGreatPeopleRateModifier;
	int m_iGreatGeneralRateModifier;
	int m_iGreatPersonExpendGold;
	int m_iUnitUpgradeCostMod;
	int m_iGoldenAgeModifier;
	int m_iFreeExperience;
	int m_iGlobalFreeExperience;
	int m_iFoodKept;
	bool m_bAirlift;
	int m_iAirModifier;
	int m_iNukeModifier;
	int m_iNukeExplosionRand;
	int m_iWorkerSpeedModifier;
	int m_iMilitaryProductionModifier;
	int m_iSpaceProductionModifier;
	int m_iGlobalSpaceProductionModifier;
	int m_iBuildingProductionModifier;
	int m_iWonderProductionModifier;
	int m_iCityConnectionTradeRouteModifier;
	int m_iCapturePlunderModifier;
	int m_iPolicyCostModifier;
	int m_iGlobalPlotCultureCostModifier;
	int m_iPlotCultureCostModifier;
	int m_iGlobalPlotBuyCostModifier;
	int m_iPlotBuyCostModifier;
#if defined(MOD_BUILDINGS_CITY_WORKING)
	int m_iGlobalCityWorkingChange;
	int m_iCityWorkingChange;
#endif
#if defined(MOD_BUILDINGS_CITY_AUTOMATON_WORKERS)
	int m_iGlobalCityAutomatonWorkersChange;
	int m_iCityAutomatonWorkersChange;
#endif

#if defined(MOD_GLOBAL_BUILDING_INSTANT_YIELD)
	int* m_piInstantYield;
	int m_iInstantYieldCount;
#endif

#if defined(MOD_ROG_CORE)
	int* m_piGreatWorkYieldChange;
	int m_iExtraDamageHealPercent;
	int m_iExtraDamageHeal;
	int m_iBombardRange;
	int m_bBombardIndirect;
	int m_iRangedStrikeModifier;
	int m_iPopulationChange;
	int m_iMinorCivFriendship;
	int m_iLiberatedInfluence;
	int m_iExtraUnitPlayerInstances;
	int m_iResetDamageValue;
	int m_iReduceDamageValue;

	int m_iGlobalCityStrengthMod;
	int m_iGlobalRangedStrikeModifier;
	int m_iResearchTotalCostModifier;
	int m_iResearchTotalCostModifierGoldenAge;
	int m_iWaterTileDamage;
	int m_iWaterTileMovementReduce;
	int m_iWaterTileTurnDamage;
	int m_iLandTileDamage;
	int m_iLandTileMovementReduce;
	int m_iLandTileTurnDamage;

	int m_iWaterTileDamageGlobal;
	int m_iWaterTileMovementReduceGlobal;
	int m_iWaterTileTurnDamageGlobal;
	int m_iLandTileDamageGlobal;
	int m_iLandTileMovementReduceGlobal;
	int m_iLandTileTurnDamageGlobal;
#endif

	int m_iNukeInterceptionChance;
	int m_iExtraAttacks;

#if defined(MOD_ROG_CORE)
	std::map<int, std::map<int, int>> m_ppiResourceYieldChangeGlobal;

	std::map<int, int> m_piYieldChangePerPopInEmpire;

	int** m_ppaiImprovementYieldChange;
	int** m_ppaiImprovementYieldChangeGlobal;
	int** m_ppiFeatureYieldChangesGlobal;
	int** m_ppiTerrainYieldChangesGlobal;
	int** m_ppaiYieldPerXTerrain;
	int** m_ppaiYieldPerXFeature;
	int** m_ppaiSpecialistYieldChangeLocal;
#endif
	int* m_piYieldFromInternal;
	int* m_piYieldFromProcessModifier;
	int* m_piYieldFromProcessModifierGlobal;

#if defined(MOD_ROG_CORE)
	int* m_paiSpecificGreatPersonRateModifier;
	int* m_piYieldPerEspionageSpy;
	int* m_piYieldChangeWorldWonder;
	int* m_piYieldChangeWorldWonderGlobal;
	int* m_piResourceQuantityFromPOP;
	int* m_piYieldPerFriend;
	int* m_piYieldPerAlly;
#endif

#if defined(MOD_ROG_CORE)
	int** m_ppiBuildingClassYieldModifiers;
	int** m_ppiBuildingClassLocalYieldChanges;
#endif

	int m_iMinAreaSize;
	int m_iConquestProbability;
	int m_iHealRateChange;
	int m_iHappiness;
	int m_iUnmoddedHappiness;
	int m_iUnhappinessModifier;
	int m_iHappinessPerCity;
	int m_iHappinessPerXPolicies;
	int m_iCityCountUnhappinessMod;
	bool m_bNoOccupiedUnhappiness;
	bool m_bNotNeedOccupied;
	bool m_bAllowSpaceshipLaunch;
	int m_iGlobalPopulationChange;
	int m_iTechShare;
	int m_iFreeTechs;
	int m_iFreePolicies;
	int m_iFreeGreatPeople;
	int m_iMedianTechPercentChange;
	int m_iGold;
	bool m_bNearbyMountainRequired;
	bool m_bAllowsRangeStrike;
	int m_iDefenseModifier;
	int m_iGlobalDefenseModifier;
	int m_iExtraCityHitPoints;
	int m_iMissionType;
	int m_iMinorFriendshipChange;
	int m_iMinorFriendshipAnchorChange;
	int m_iMinorQuestFriendshipMod;
	int m_iVictoryPoints;
	int m_iExtraMissionarySpreads;
	int m_iReligiousPressureModifier;
	int m_iEspionageModifier;
	int m_iGlobalEspionageModifier;
	int m_iGlobalEspionageSpeedModifier;
	int m_iExtraSpies;
	int m_iSpyRankChange;
	int m_iInstantSpyRankChange;

#ifdef MOD_BUILDINGS_GOLDEN_AGE_EXTEND
	int m_iGoldenAgeUnitCombatModifier;
	int m_iGoldenAgeMeterMod;
#endif

#if defined(MOD_RELIGION_CONVERSION_MODIFIERS)
	int m_iConversionModifier;
	int m_iGlobalConversionModifier;
#endif

#ifdef MOD_PROMOTION_CITY_DESTROYER
	int m_iSiegeKillCitizensModifier = 0;
#endif

	int m_iLandmarksTourismPercent;
	int m_iInstantMilitaryIncrease;
	int m_iGreatWorksTourismModifier;
	int m_iXBuiltTriggersIdeologyChoice;
	int m_iTradeRouteRecipientBonus;
	int m_iTradeRouteTargetBonus;
	int m_iNumTradeRouteBonus;
	int m_iTradeRouteSeaDistanceModifier;
	int m_iTradeRouteSeaGoldBonus;
	int m_iTradeRouteLandDistanceModifier;
	int m_iTradeRouteLandGoldBonus;
	int m_iGreatScientistBeakerModifier;
	int m_iExtraLeagueVotes;
	int m_iPreferredDisplayPosition;
	int m_iPortraitIndex;

	bool m_bTeamShare;
	bool m_bWater;
	bool m_bRiver;
	bool m_bFreshWater;
#if defined(MOD_VOLCANO_BREAK)
	bool m_bImmueVolcanoDamage;
#endif
	bool m_bAddsFreshWater;
	bool m_bPurchaseOnly;
	bool m_bHumanOnly;
	bool m_bMoveAfterCreated;
	bool m_bMountain;
	bool m_bHill;
	bool m_bFlat;
	bool m_bFoundsReligion;
	bool m_bIsReligious;
	bool m_bBorderObstacle;
	bool m_bPlayerBorderObstacle;
	bool m_bCapital;
	bool m_bGoldenAge;
	bool m_bMapCentering;
	bool m_bNeverCapture;
	bool m_bNukeImmune;
	bool m_bExtraAttackOnKill;
	bool m_bIsForbiddenForeignSpy;
	bool m_bIsForbiddenForeignSpyGlobal;
	bool m_bExtraLuxuries;
	bool m_bDiplomaticVoting;
	bool m_bAllowsWaterRoutes;
	bool m_bCityWall;
	bool m_bUnlockedByBelief;
	bool m_bUnlockedByLeague;
	bool m_bRequiresHolyCity;
	bool m_bAffectSpiesNow;
	bool m_bEspionage;
	bool m_bAllowsFoodTradeRoutes;
	bool m_bAllowsProductionTradeRoutes;
	bool m_bNullifyInfluenceModifier;

	bool m_bArtInfoCulturalVariation;
	bool m_bArtInfoEraVariation;
	bool m_bArtInfoRandomVariation;

	CvString m_strArtDefineTag;
	CvString m_strWonderSplashAudio;
	CvString m_strThemingBonusHelp;

	// Arrays
	int* m_piResourceQuantityRequirements;
	int* m_piResourceQuantity;
	int* m_piResourceCultureChanges;
	int* m_piResourceFaithChanges;
	int* m_piProductionTraits;
	int* m_piSeaPlotYieldChange;
	int* m_piRiverPlotYieldChange;
	int* m_piRiverPlotYieldChangeGlobal;
	int* m_piLakePlotYieldChange;
	int* m_piSeaResourceYieldChange;
	int* m_piYieldChange;
	int* m_piYieldChangePerEra;
	int* m_piYieldModifierChangePerEra;
	int* m_piCityStateTradeRouteYieldModifier;
	int* m_piCityStateTradeRouteYieldModifierGlobal;
	int* m_piYieldChangePerPop;
	int* m_piYieldChangePerReligion;
	int* m_piYieldModifier;
	int* m_piYieldMultiplier;

#if defined(MOD_ROG_CORE)
	int* m_piYieldFromConstruction;
	int* m_piYieldFromUnitProduction;
	int* m_piYieldFromBirth;
	int* m_piYieldFromBorderGrowth;
	int* m_piYieldFromPillage;
	int* m_piYieldFromPillageGlobal;
	int* m_piYieldFromPillageGlobalPlayer;
	int* m_piYieldModifierFromWonder;
#endif


	int* m_piAreaYieldModifier;
	int* m_piGlobalYieldModifier;
	int* m_piTechEnhancedYieldChange;
	int* m_piUnitCombatFreeExperience;
	int* m_piUnitCombatProductionModifiers;
	int* m_piDomainFreeExperience;
	int* m_piDomainFreeExperiencePerGreatWork;
	int* m_piDomainFreeExperiencesPerPop;
	int* m_piDomainFreeExperiencesPerPopGlobal;
	int* m_piDomainFreeExperiencesPerTurn;
	int* m_piDomainFreeExperiencesPerTurnGlobal;
	int* m_piDomainEnemyCombatModifier;
	int* m_piDomainEnemyCombatModifierGlobal;
	int* m_piDomainFriendsCombatModifierLocal;

#if defined(MOD_ROG_CORE)
	int* m_piDomainFreeExperiencePerGreatWorkGlobal;
	std::map<int, int> m_piDomainFreeExperienceGlobal;
	std::map<int, int> m_piUnitTypePrmoteHealGlobal;
#endif
	std::map<int, int> m_mapUnitClassMaxInstances;

#if defined(MOD_NUCLEAR_WINTER_FOR_SP)
	bool m_bNoNuclearWinterLocal = false;
#endif

#if defined(MOD_TROOPS_AND_CROPS_FOR_SP)
	int* m_piDomainTroops;
	int  m_iNumCrops;
	int  m_iNumArmee;
	int  m_bEnableCrops;
	int  m_bEnableArmee;
#endif


	int* m_piDomainProductionModifier;
	int* m_piPrereqNumOfBuildingClass;
	int* m_piFlavorValue;

	std::vector<int> m_piLockedByBuildings;
	std::vector<int> m_piPrereqAndTechs;
	std::vector<int> m_piLocalResourceAnds;
	std::vector<int> m_piLocalResourceOrs;
	std::vector<int> m_piEmpireResourceAnds;
	std::vector<int> m_piEmpireResourceOrs;
	std::vector<int> m_piLocalFeatureOrs;
	std::vector<int> m_piLocalFeatureAnds;
	std::vector<int> m_piLocalPlotAnds;

	int* m_paiHurryModifier;
	int* m_paiHurryModifierLocal;

	std::vector<int> m_setBuildingClassesNeededInCity;
	std::vector<int> m_setBuildingClassesNeededGlobal;
	std::vector<int> m_setBuildingsNeededInCity;
	std::vector<int> m_setBuildingsNeededGlobal;
#if defined(MOD_BUILDING_NEW_EFFECT_FOR_SP)
	int m_iCityDefenseModifierGlobal;
	int m_iUnitMaxExperienceLocal;
	int m_iExtraSellRefund;
	int m_iExtraSellRefundModifierPerEra;
	int m_iMinNumReligions;
	int m_iLandmarksTourismPercentGlobal;
	int m_iGreatWorksTourismModifierGlobal;
	int m_iTradeRouteRiverBonusModifier;
	int m_iTradeRouteSeaGoldBonusGlobal;
	int m_iTradeRouteLandGoldBonusGlobal;
	bool m_bAnyWater;
	bool m_bRiverOrCoastal;
#endif
	int m_iNumFreeUnit;
	int m_iNumFreeUnitTotal;
	std::pair<UnitTypes, int>* m_pFreeUnits;

	CvDoubleYieldInfo* m_paYieldFromYieldGlobal;

	int** m_ppaiResourceYieldChange;
	int** m_ppaiFeatureYieldChange;
	int** m_ppaiTerrainYieldModifier;  
	int** m_ppaiSpecialistYieldChange;
	int** m_ppaiImprovementYieldModifier;
	int** m_ppaiSpecialistYieldModifier;
	int** m_ppaiSpecialistYieldModifierGlobal;
	int** m_ppaiFeatureYieldModifier;
	int** m_ppaiResourceYieldModifier;
	int** m_ppaiTerrainYieldChange;
#if defined(MOD_API_UNIFIED_YIELDS) && defined(MOD_API_PLOT_YIELDS)
	int** m_ppaiPlotYieldChange;
#endif
	int** m_ppiBuildingClassYieldChanges;
	int* m_paiBuildingClassHappiness;

#if defined(MOD_BUILDING_IMPROVEMENT_RESOURCES)
	std::multimap<int, std::pair<int, int>> m_piiResourceFromImprovement;
#endif

	CvThemingBonusInfo* m_paThemingBonusInfo;
	int m_iNumThemingBonuses;

#ifdef MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD
	int m_ppiYieldFromOtherYield[NUM_YIELD_TYPES][NUM_YIELD_TYPES][YieldFromYield::LENGTH];
	bool m_bHasYieldFromOtherYield = false;
#endif

#ifdef MOD_GLOBAL_CITY_SCALES
	CityScaleTypes m_eEnableCityScaleGrowth = NO_CITY_SCALE;
	bool m_bEnableAllCityScaleGrowth = false;
#endif

#if defined(MOD_INTERNATIONAL_IMMIGRATION_FOR_SP)
	bool m_bCanAllScaleImmigrantIn = false;
#endif

#ifdef MOD_GLOBAL_CORRUPTION
	int m_iCorruptionScoreChange = 0;
	int m_iCorruptionLevelChange = 0;
	int m_iCorruptionPolicyCostModifier = 0;
	int m_iMinCorruptionLevelNeeded = -1;
	int m_iMaxCorruptionLevelNeeded = -1;
#endif

	int m_iGlobalProductionNeededUnitModifier = 0;
	int m_iGlobalProductionNeededBuildingModifier = 0;
	int m_iGlobalProductionNeededProjectModifier = 0;
	int m_iGlobalProductionNeededUnitMax = 0;
	int m_iGlobalProductionNeededBuildingMax = 0;
	int m_iGlobalProductionNeededProjectMax = 0;
	bool m_bDummyBuilding = false;
	bool m_bNoPuppet = false;

	int m_iInstantResearchFromFriendlyGreatScientist = 0;

	int m_iGlobalGrowthFoodNeededModifier = 0;
	int m_iSecondCapitalsExtraScore = 0;
	int m_iFoodKeptFromPollution = 0;

	bool m_bCapitalOnly;
	bool m_bOriginalCapitalOnly;

	std::tr1::array<int, YieldTypes::NUM_YIELD_TYPES> m_aTradeRouteFromTheCityYields;
	std::tr1::array<int, YieldTypes::NUM_YIELD_TYPES> m_aTradeRouteFromTheCityYieldsPerEra;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvBuildingXMLEntries
//!  \brief		Game-wide information about buildings
//
//! Key Attributes:
//! - Plan is it will be contained in CvGameRules object within CvGame class
//! - Populated from XML\Buildings\CIV5BuildingInfos.xml
//! - Contains an array of CvBuildingEntry from the above XML file
//! - One instance for the entire game
//! - Accessed heavily by CvCityBuildings class (which stores the building state for 1 city)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvBuildingXMLEntries
{
public:
	CvBuildingXMLEntries(void);
	~CvBuildingXMLEntries(void);

	// Accessor functions
	std::vector<CvBuildingEntry*>& GetBuildingEntries();
	int GetNumBuildings();
	_Ret_maybenull_ CvBuildingEntry* GetEntry(int index);

	void DeleteArray();

private:
	std::vector<CvBuildingEntry*> m_paBuildingEntries;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvCityBuildings
//!  \brief		Information about the buildings constructed in a single city
//
//!  Key Attributes:
//!  - Plan is it will be contained in CvCityState object within CvCity class
//!  - One instance for each city
//!  - Accessed by any class that needs to check building construction status
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvCityBuildings
{
public:
	CvCityBuildings(void);
	~CvCityBuildings(void);
	void Init(CvBuildingXMLEntries* pBuildings, CvCity* pCity);
	void Uninit();
	void Reset();
	void Read(FDataStream& kStream);
	void Write(FDataStream& kStream);

	// Accessor functions
	CvBuildingXMLEntries* GetBuildings() const;

	int GetNumBuildings() const;
	void ChangeNumBuildings(int iChange);

	int GetNumBuilding(BuildingTypes eIndex) const;
	int GetNumActiveBuilding(BuildingTypes eIndex) const;

	bool IsBuildingSellable(const CvBuildingEntry& kBuilding) const;
	void DoSellBuilding(BuildingTypes eIndex);
	int GetSellBuildingRefund(BuildingTypes eIndex) const;

	bool IsSoldBuildingThisTurn() const;
	void SetSoldBuildingThisTurn(bool bValue);

	int GetTotalBaseBuildingMaintenance() const;

	int GetBuildingProduction(BuildingTypes eIndex) const;
	int GetBuildingProductionTimes100(BuildingTypes eIndex) const;
	void SetBuildingProduction(BuildingTypes eIndex, int iNewValue);
	void SetBuildingProductionTimes100(BuildingTypes eIndex, int iNewValue);
	void ChangeBuildingProduction(BuildingTypes eIndex, int iChange);
	void ChangeBuildingProductionTimes100(BuildingTypes eIndex, int iChange);

	int GetBuildingProductionTime(BuildingTypes eIndex) const;
	void SetBuildingProductionTime(BuildingTypes eIndex, int iNewValue);
	void ChangeBuildingProductionTime(BuildingTypes eIndex, int iChange);

	int GetBuildingOriginalOwner(BuildingTypes eIndex) const;
	void SetBuildingOriginalOwner(BuildingTypes eIndex, int iNewValue);
	int GetBuildingOriginalTime(BuildingTypes eIndex) const;
	void SetBuildingOriginalTime(BuildingTypes eIndex, int iNewValue);

	int GetNumRealBuilding(BuildingTypes eIndex) const;
	void SetNumRealBuilding(BuildingTypes eIndex, int iNewValue);
	void SetNumRealBuildingTimed(BuildingTypes eIndex, int iNewValue, bool bFirst, PlayerTypes eOriginalOwner, int iOriginalTime);

	int GetNumFreeBuilding(BuildingTypes eIndex) const;
	void SetNumFreeBuilding(BuildingTypes eIndex, int iNewValue);

	int GetBuildingYieldChange(BuildingClassTypes eBuildingClass, YieldTypes eYield) const;
	void SetBuildingYieldChange(BuildingClassTypes eBuildingClass, YieldTypes eYield, int iChange);
	void ChangeBuildingYieldChange(BuildingClassTypes eBuildingClass, YieldTypes eYield, int iChange);

	int GetBuildingGreatWork(BuildingClassTypes eBuildingClass, int iSlot) const;
	void SetBuildingGreatWork(BuildingClassTypes eBuildingClass, int iSlot, int iGreatWorkIndex);
	bool IsHoldingGreatWork(BuildingClassTypes eBuildingClass) const;
	int GetNumGreatWorksInBuilding(BuildingClassTypes eBuildingClass) const;
  
	bool HasAnyAvailableGreatWorkSlot() const;
	bool HasAvailableGreatWorkSlot(GreatWorkSlotType eGreatWorkSlot) const;
	int GetNumAvailableGreatWorkSlots() const;
	int GetNumAvailableGreatWorkSlots(GreatWorkSlotType eGreatWorkSlot) const;
	bool GetNextAvailableGreatWorkSlot(BuildingClassTypes *eBuildingClass, int *iSlot) const;
	bool GetNextAvailableGreatWorkSlot(GreatWorkSlotType eGreatWorkSlot, BuildingClassTypes *eBuildingClass, int *iSlot) const;

#if defined(MOD_GLOBAL_GREATWORK_YIELDTYPES) || defined(MOD_API_UNIFIED_YIELDS)
	int GetYieldFromGreatWorks(YieldTypes eYield) const;
#endif
	int GetCultureFromGreatWorks() const;
#if defined(MOD_GLOBAL_GREATWORK_YIELDTYPES)
#endif
	int GetNumGreatWorks(bool bIgnoreYield = true, bool bIncludeArtifact = true, bool bIncludeGreatWork = true) const;

	int GetNumGreatWorks(GreatWorkSlotType eGreatWorkSlot) const;

	int GetLandmarksTourismPercent() const;
	void ChangeLandmarksTourismPercent(int iChange);
	int GetGreatWorksTourismModifier() const;
	void ChangeGreatWorksTourismModifier(int iChange);

#if defined(MOD_GLOBAL_GREATWORK_YIELDTYPES)
	int GetThemingBonuses(YieldTypes eYield) const;
#else
	int GetThemingBonuses() const;
#endif
	int GetNumBuildingsFromFaith() const;
	void ChangeNumBuildingsFromFaith(int iChange);

	int GetBuildingProductionModifier() const;
	void ChangeBuildingProductionModifier(int iChange);

	int GetBuildingDefense() const;
	void ChangeBuildingDefense(int iChange);
	int GetBuildingDefenseMod() const;
	void ChangeBuildingDefenseMod(int iChange);

	int GetMissionaryExtraSpreads() const;
	void ChangeMissionaryExtraSpreads(int iChange);

	void IncrementWonderStats(BuildingClassTypes eIndex);
	bool CheckForAllWondersBuilt();
	bool CheckForSevenAncientWondersBuilt();

#if defined(MOD_ROG_CORE)
	const std::vector<BuildingTypes>& GetAllBuildingsHere() const { return m_buildingsThatExistAtLeastOnce; }
#endif

private:
	void NotifyNewBuildingStarted(BuildingTypes eIndex);

	int m_iNumBuildings;
	int m_iBuildingProductionModifier;
	int m_iBuildingDefense;
	int m_iBuildingDefenseMod;
	int m_iMissionaryExtraSpreads;
	int m_iLandmarksTourismPercent;
	int m_iGreatWorksTourismModifier;

	int m_iNumBuildingsFromFaith;

	bool m_bSoldBuildingThisTurn;

	int* m_paiBuildingProduction;
	int* m_paiBuildingProductionTime;
	int* m_paiBuildingOriginalOwner;
	int* m_paiBuildingOriginalTime;
	int* m_paiNumRealBuilding;
	int* m_paiNumFreeBuilding;

#if defined(MOD_ROG_CORE)
	std::vector<BuildingTypes> m_buildingsThatExistAtLeastOnce;
#endif

	std::vector<BuildingYieldChange> m_aBuildingYieldChange;
	std::vector<BuildingGreatWork> m_aBuildingGreatWork;

	CvBuildingXMLEntries* m_pBuildings;
	CvCity* m_pCity;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Helper Functions to serialize arrays of variable length (based on number of buildings defined in game)
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
namespace BuildingArrayHelpers
{
void Read(FDataStream& kStream, int* paiBuildingArray);
void Write(FDataStream& kStream, int* paiBuildingArray, int iArraySize);
}

#endif //CIV5_BUILDING_CLASSES_H
