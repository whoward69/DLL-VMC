/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#pragma once

#ifndef CIV5_BELIEF_CLASSES_H
#define CIV5_BELIEF_CLASSES_H

#include "CustomMods.h"

class CvReligion;

#define SAFE_ESTIMATE_NUM_BELIEFS 100

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvBeliefEntry
//!  \brief		A single entry in the Belief XML file
//
//!  Key Attributes:
//!  - Used to be called CvBeliefInfo
//!  - Populated from XML\Civilizations\CIV5Beliefs.xml
//!  - Array of these contained in CvBeliefXMLEntries class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvBeliefEntry: public CvBaseInfo
{
public:
	CvBeliefEntry(void);
	~CvBeliefEntry(void);

	int GetMinPopulation() const;
	int GetMinFollowers() const;
	int GetMaxDistance() const;
	int GetCityGrowthModifier() const;
	int GetFaithFromKills() const;
	int GetFaithFromDyingUnits() const;
	int GetRiverHappiness() const;
	int GetHappinessPerCity() const;
	int GetHappinessPerXPeacefulForeignFollowers() const;
	int GetPlotCultureCostModifier() const;
	int GetCityRangeStrikeModifier() const;
	int GetCombatModifierEnemyCities() const;
	int GetCombatModifierFriendlyCities() const;
	int GetFriendlyHealChange() const;
	int GetCityStateFriendshipModifier() const;
	int GetLandBarbarianConversionPercent() const;
	int GetWonderProductionModifier() const;
	int GetPlayerHappiness() const;
	int GetPlayerCultureModifier() const;
	float GetHappinessPerFollowingCity() const;
	int GetGoldPerFollowingCity() const;
	int GetGoldPerXFollowers() const;
	int GetGoldWhenCityAdopts() const;
	int GetSciencePerOtherReligionFollower() const;
	int GetSpreadDistanceModifier() const;
	int GetSpreadStrengthModifier() const;
	int GetProphetStrengthModifier() const;
	int GetProphetCostModifier() const;
	int GetMissionaryStrengthModifier() const;
	int GetMissionaryCostModifier() const;
	int GetFriendlyCityStateSpreadModifier() const;
	int GetGreatPersonExpendedFaith() const;
	int GetCityStateMinimumInfluence() const;
	int GetCityStateInfluenceModifier() const;
	int GetOtherReligionPressureErosion() const;
	int GetSpyPressure() const;
	int GetInquisitorPressureRetention() const;
	int GetFaithBuildingTourism() const;

	bool IsPantheonBelief() const;
	bool IsFounderBelief() const;
	bool IsFollowerBelief() const;
	bool IsEnhancerBelief() const;
	bool IsReformationBelief() const;
	bool RequiresPeace() const;
	bool ConvertsBarbarians() const;
	bool FaithPurchaseAllGreatPeople() const;

	EraTypes GetObsoleteEra() const;
	ResourceTypes GetResourceRevealed() const;
	TechTypes GetSpreadModifierDoublingTech() const;

	const char* getShortDescription() const;
	void setShortDescription(const char* szVal);

	// Arrays
	int GetCityYieldChange(int i) const;
	int GetHolyCityYieldChange(int i) const;
	int GetYieldChangePerForeignCity(int i) const;
	int GetYieldChangePerXForeignFollowers(int i) const;
#if defined(MOD_API_UNIFIED_YIELDS)
	int GetYieldPerFollowingCity(int i) const;
	int GetYieldPerXFollowers(int i) const;
	int GetHolyCityYieldPerForeignFollowers(int i) const;
	int GetHolyCityYieldPerNativeFollowers(int i) const;
	int GetCityYieldPerOtherReligion(int i) const;
	int GetYieldPerOtherReligionFollower(int i) const;
	int GetCuttingInstantYieldModifier(int i) const;
	int GetCuttingInstantYield(int i) const;
#endif
	int GetResourceQuantityModifier(int i) const;
	int GetImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2) const;
	int GetImprovementAdjacentCityYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2) const;
	int GetBuildingClassYieldChange(int i, int j) const;
	int GetBuildingClassHappiness(int i) const;
	int GetBuildingClassTourism(int i) const;
	int GetFeatureYieldChange(int i, int j) const;
#if defined(MOD_API_UNIFIED_YIELDS)
	int GetCityYieldFromUnimprovedFeature(int i, int j) const;
	int GetUnimprovedFeatureYieldChange(int i, int j) const;
#endif
	int GetResourceYieldChange(int i, int j) const;
	int GetTerrainYieldChange(int i, int j) const;
	int GetTerrainYieldChangeAdditive(int i, int j) const;
	int GetTerrainCityYieldChanges(int i, int j) const;
#if defined(MOD_API_UNIFIED_YIELDS)
	int GetTradeRouteYieldChange(int i, int j) const;
	int GetSpecialistYieldChange(int i, int j) const;
	int GetGreatPersonExpendedYield(int i, int j) const;
	int GetGoldenAgeGreatPersonRateModifier(int i) const;
	int GetCapitalYieldChange(int i) const;
	int GetCoastalCityYieldChange(int i) const;
	int GetGreatWorkYieldChange(int i) const;
	int GetYieldFromKills(YieldTypes eYield) const;
	int GetYieldFromBarbarianKills(YieldTypes eYield) const;
#endif
#if defined(MOD_RELIGION_PLOT_YIELDS)
	int GetPlotYieldChange(int i, int j) const;
#endif
#if defined(MOD_BELIEF_NEW_EFFECT_FOR_SP)
	int GetGoldenAgeModifier() const;
	int GetExtraSpies() const;
	bool IsGreatPersonPointsCapital() const;
	bool IsGreatPersonPointsPerCity() const;
	bool IsGreatPersonPointsHolyCity() const;
	bool IsInquisitorProhibitSpreadInAlly() const;
	int GetGreatPersonPoints(int i, bool bCapital, bool bHolyCity) const;
	int GetTerrainCityFoodConsumption(int i) const;
	int GetFreePromotionForProphet() const;
	int GetFounderFreePromotion() const;
	int GetFollowingCityFreePromotion() const;
	int GetLandmarksTourismPercent() const;
	int GetHolyCityUnitExperence() const;
	int GetHolyCityPressureModifier() const;
	int GetSameReligionMinorRecoveryModifier() const;
	int GetInquisitionFervorTimeModifier() const;
	int GetCuttingBonusModifier() const;
	int GetCityExtraMissionarySpreads() const;
	bool AllowYieldPerBirth() const;
	int GetYieldPerBirth(int i) const;
	int GetLakePlotYieldChange(int i) const;
	int GetRiverPlotYieldChange(int i) const;
#endif
	int GetExtraFlavorValue(int i) const;
	int GetCivilizationFlavorValue(int i) const;
	int GetResourceHappiness(int i) const;
	int GetYieldChangeAnySpecialist(int i) const;
	int GetYieldChangeTradeRoute(int i) const;
	int GetYieldChangeNaturalWonder(int i) const;
	int GetYieldChangeWorldWonder(int i) const;
	int GetYieldModifierNaturalWonder(int i) const;
	int GetMaxYieldModifierPerFollower(int i) const;
	int GetYieldModifierPerFollowerTimes100(int i) const;
	bool IsFaithUnitPurchaseEra(int i) const;
	bool IsBuildingClassEnabled(int i) const;

	virtual bool CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility);

protected:
	int m_iMinPopulation;
	int m_iMinFollowers;
	int m_iMaxDistance;
	int m_iCityGrowthModifier;
	int m_iFaithFromKills;
	int m_iFaithFromDyingUnits;
	int m_iRiverHappiness;
	int m_iHappinessPerCity;
	int m_iHappinessPerXPeacefulForeignFollowers;
	int m_iPlotCultureCostModifier;
	int m_iCityRangeStrikeModifier;
	int m_iCombatModifierEnemyCities;
	int m_iCombatModifierFriendlyCities;
	int m_iFriendlyHealChange;
	int m_iCityStateFriendshipModifier;
	int m_iLandBarbarianConversionPercent;
	int m_iWonderProductionModifier;
	int m_iPlayerHappiness;
	int m_iPlayerCultureModifier;

	float m_fHappinessPerFollowingCity;

	int m_iGoldPerFollowingCity;
	int m_iGoldPerXFollowers;
	int m_iGoldWhenCityAdopts;
	int m_iSciencePerOtherReligionFollower;
	int m_iSpreadDistanceModifier;
	int m_iSpreadStrengthModifier;
	int m_iProphetStrengthModifier;
	int m_iProphetCostModifier;
	int m_iMissionaryStrengthModifier;
	int m_iMissionaryCostModifier;
	int m_iFriendlyCityStateSpreadModifier;
	int m_iGreatPersonExpendedFaith;
	int m_iCityStateMinimumInfluence;
	int m_iCityStateInfluenceModifier;
	int m_iOtherReligionPressureErosion;
	int m_iSpyPressure;
	int m_iInquisitorPressureRetention;
	int m_iFaithBuildingTourism;
#if defined(MOD_BELIEF_NEW_EFFECT_FOR_SP)
	int m_iGoldenAgeModifier;
	int m_iExtraSpies;
	bool m_bGreatPersonPointsCapital;
	bool m_bGreatPersonPointsPerCity;
	bool m_bGreatPersonPointsHolyCity;
	bool m_bInquisitorProhibitSpreadInAlly;
	int* m_piGreatPersonPoints;
	int* m_piTerrainCityFoodConsumption;
	int m_iFreePromotionForProphet;
	int m_iFounderFreePromotion;
	int m_iFollowingCityFreePromotion;
	int m_iLandmarksTourismPercent;
	int m_iHolyCityUnitExperence;
	int m_iHolyCityPressureModifier;
	int m_iSameReligionMinorRecoveryModifier;
	int m_iInquisitionFervorTimeModifier;
	int m_iCuttingBonusModifier;
	int m_iCityExtraMissionarySpreads;
	bool m_bAllowYieldPerBirth;
	int* m_piYieldPerBirth;
	int* m_piLakePlotYieldChange;
	int* m_piRiverPlotYieldChange;
#endif

	bool m_bPantheon;
	bool m_bFounder;
	bool m_bFollower;
	bool m_bEnhancer;
	bool m_bReformer;
	bool m_bRequiresPeace;
	bool m_bConvertsBarbarians;
	bool m_bFaithPurchaseAllGreatPeople;

	EraTypes m_eObsoleteEra;
	ResourceTypes m_eResourceRevealed;
	TechTypes m_eSpreadModifierDoublingTech;

	CvString m_strShortDescription;

	// Arrays
	int* m_paiCityYieldChange;
	int* m_paiHolyCityYieldChange;
	int* m_paiYieldChangePerForeignCity;
	int* m_paiYieldChangePerXForeignFollowers;
#if defined(MOD_API_UNIFIED_YIELDS)
	int* m_piYieldPerFollowingCity;
	int* m_piYieldPerXFollowers;
	int* m_piHolyCityYieldPerForeignFollowers;
	int* m_piHolyCityYieldPerNativeFollowers;
	int* m_piCityYieldPerOtherReligion;
	int* m_piYieldPerOtherReligionFollower;
	int* m_piCuttingInstantYieldModifier;
	int* m_piCuttingInstantYield;
#endif
	int* m_piResourceQuantityModifiers;
	int** m_ppiImprovementYieldChanges;
	int** m_ppiImprovementAdjacentCityYieldChanges;
	int** m_ppiBuildingClassYieldChanges;
	int* m_paiBuildingClassHappiness;
	int* m_paiBuildingClassTourism;
	int** m_ppaiFeatureYieldChange;
#if defined(MOD_API_UNIFIED_YIELDS)
	int** m_ppiCityYieldFromUnimprovedFeature;
	int** m_ppiUnimprovedFeatureYieldChanges;
#endif
	int** m_ppaiResourceYieldChange;
	int** m_ppaiTerrainYieldChange;
	int** m_ppaiTerrainYieldChangeAdditive;
	int** m_ppaiTerrainCityYieldChanges;
#if defined(MOD_API_UNIFIED_YIELDS)
	int** m_ppiTradeRouteYieldChange;
	int** m_ppiSpecialistYieldChange;
	int** m_ppiGreatPersonExpendedYield;
	int* m_piGoldenAgeGreatPersonRateModifier;
	int* m_piCapitalYieldChange;
	int* m_piCoastalCityYieldChange;
	int* m_piGreatWorkYieldChange;
	int* m_piYieldFromKills;
	int* m_piYieldFromBarbarianKills;
#endif
#if defined(MOD_RELIGION_PLOT_YIELDS)
	int** m_ppiPlotYieldChange;
#endif
	int* m_piExtraFlavorValue;
	int* m_piCivilizationFlavorValue;

	int* m_piResourceHappiness;
	int* m_piYieldChangeAnySpecialist;
	int* m_piYieldChangeTradeRoute;
	int* m_piYieldChangeNaturalWonder;
	int* m_piYieldChangeWorldWonder;
	int* m_piYieldModifierNaturalWonder;
	int* m_piMaxYieldModifierPerFollower;
	int* m_piYieldModifierPerFollowerTimes100;
	bool* m_pbFaithPurchaseUnitEraEnabled;
    bool* m_pbBuildingClassEnabled;

private:
	CvBeliefEntry(const CvBeliefEntry&);
	CvBeliefEntry& operator=(const CvBeliefEntry&);
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvBeliefXMLEntries
//!  \brief		Game-wide information about religious beliefs
//
//! Key Attributes:
//! - Plan is it will be contained in CvGameRules object within CvGame class
//! - Populated from XML\GameInfo\CIV5Beliefs.xml
//! - Contains an array of CvBeliefEntry from the above XML file
//! - One instance for the entire game
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvBeliefXMLEntries
{
public:
	CvBeliefXMLEntries(void);
	~CvBeliefXMLEntries(void);

	// Accessor functions
	std::vector<CvBeliefEntry*>& GetBeliefEntries();
	int GetNumBeliefs();
	CvBeliefEntry* GetEntry(int index);

	void DeleteArray();

private:
	std::vector<CvBeliefEntry*> m_paBeliefEntries;
};

typedef FStaticVector<int, 5, false, c_eCiv5GameplayDLL >BeliefList;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvReligionBeliefs
//!  \brief		Information about the beliefs of a religion
//
//!  Key Attributes:
//!  - One instance for each religion (or pantheon)
//!  - Accessed by any class that needs to check belief info
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvReligionBeliefs
{
public:
	CvReligionBeliefs(void);
	~CvReligionBeliefs(void);
	CvReligionBeliefs(const CvReligionBeliefs& source);
	void Uninit();
	void Reset();
	void AddBelief(BeliefTypes eBelief, PlayerTypes ePlayer);

	// Accessor functions
	bool HasBelief(BeliefTypes eBelief) const;
	BeliefTypes GetBelief(int iIndex) const;
	int GetNumBeliefs() const;

	int GetFaithFromDyingUnits() const
	{
		return m_iFaithFromDyingUnits;
	};
	int GetRiverHappiness() const
	{
		return m_iRiverHappiness;
	};
	int GetPlotCultureCostModifier() const
	{
		return m_iPlotCultureCostModifier;
	};
	int GetCityRangeStrikeModifier() const
	{
		return m_iCityRangeStrikeModifier;
	};
	int GetCombatModifierEnemyCities() const
	{
		return m_iCombatModifierEnemyCities;
	};
	int GetCombatModifierFriendlyCities() const
	{
		return m_iCombatModifierFriendlyCities;
	};
	int GetFriendlyHealChange() const
	{
		return m_iFriendlyHealChange;
	};
	int GetCityStateFriendshipModifier() const
	{
		return m_iCityStateFriendshipModifier;
	};
	int GetLandBarbarianConversionPercent() const
	{
		return m_iLandBarbarianConversionPercent;
	};
	int GetSpreadDistanceModifier() const
	{
		return m_iSpreadDistanceModifier;
	};
	int GetSpreadStrengthModifier() const
	{
		return m_iSpreadStrengthModifier;
	};
	int GetProphetStrengthModifier() const
	{
		return m_iProphetStrengthModifier;
	};
	int GetProphetCostModifier() const
	{
		return m_iProphetCostModifier;
	};
	int GetMissionaryStrengthModifier() const
	{
		return m_iMissionaryStrengthModifier;
	};
	int GetMissionaryCostModifier() const
	{
		return m_iMissionaryCostModifier;
	};
	int GetFriendlyCityStateSpreadModifier() const
	{
		return m_iFriendlyCityStateSpreadModifier;
	};
	int GetGreatPersonExpendedFaith() const
	{
		return m_iGreatPersonExpendedFaith;
	};
	int GetCityStateMinimumInfluence() const
	{
		return m_iCityStateMinimumInfluence;
	}
	int GetCityStateInfluenceModifier() const
	{
		return m_iCityStateInfluenceModifier;
	}
	int GetOtherReligionPressureErosion() const
	{
		return m_iOtherReligionPressureErosion;
	}
	int GetSpyPressure() const
	{
		return m_iSpyPressure;
	}
	int GetInquisitorPressureRetention() const
	{
		return m_iInquisitorPressureRetention;
	}
	int GetFaithBuildingTourism() const
	{
		return m_iFaithBuildingTourism;
	}

#if defined(MOD_BELIEF_NEW_EFFECT_FOR_SP)
	int GetGoldenAgeModifier() const
	{
		return m_iGoldenAgeModifier;
	}
	int GetExtraSpies() const
	{
		return m_iExtraSpies;
	}
	bool HasGreatPersonPoints() const
	{
		return m_bGreatPersonPoints;
	}
	bool IsInquisitorProhibitSpreadInAlly() const
	{
		return m_iNumInquisitorProhibitSpreadInAlly > 0;
	}
	const std::vector<int>& GetFreePromotionForProphet() const
	{
		return m_vFreePromotionForProphet;
	}
	int GetFounderFreePromotion() const
	{
		return m_iFounderFreePromotion;
	}
	const std::tr1::unordered_set<int>& GetFollowingCityFreePromotion() const
	{
		return m_vFollowingCityFreePromotion;
	}
	int GetLandmarksTourismPercent() const
	{
		return m_iLandmarksTourismPercent;
	}
	int GetHolyCityUnitExperence() const
	{
		return m_iHolyCityUnitExperence;
	}
	int GetHolyCityPressureModifier() const
	{
		return m_iHolyCityPressureModifier;
	}
	int GetSameReligionMinorRecoveryModifier() const
	{
		return m_iSameReligionMinorRecoveryModifier;
	}
	int GetInquisitionFervorTimeModifier() const
	{
		return m_iInquisitionFervorTimeModifier;
	}
	int GetCuttingBonusModifier() const
	{
		return m_iCuttingBonusModifier;
	}
	int GetCityExtraMissionarySpreads() const
	{
		return m_iCityExtraMissionarySpreads;
	}
	bool AllowYieldPerBirth() const
	{
		return m_bAllowYieldPerBirth;
	}
#endif

	EraTypes GetObsoleteEra() const
	{
		return m_eObsoleteEra;
	};
	ResourceTypes GetResourceRevealed() const
	{
		return m_eResourceRevealed;
	};
	TechTypes GetSpreadModifierDoublingTech() const
	{
		return m_eSpreadModifierDoublingTech;
	};

	int GetFaithFromKills(int iDistance) const;
	int GetHappinessPerCity(int iPopulation) const;
	int GetHappinessPerXPeacefulForeignFollowers() const;
	int GetWonderProductionModifier(EraTypes eWonderEra) const;
	int GetPlayerHappiness(bool bAtPeace) const;
	int GetPlayerCultureModifier(bool bAtPeace) const;
	float GetHappinessPerFollowingCity() const;
	int GetGoldPerFollowingCity() const;
	int GetGoldPerXFollowers() const;
	int GetGoldWhenCityAdopts() const;
	int GetSciencePerOtherReligionFollower() const;
	int GetCityGrowthModifier(bool bAtPeace) const;

	int GetCityYieldChange(int iPopulation, YieldTypes eYield) const;
	int GetHolyCityYieldChange(YieldTypes eYield) const;
	int GetYieldChangePerForeignCity(YieldTypes eYield) const;
	int GetYieldChangePerXForeignFollowers(YieldTypes eYield) const;
#if defined(MOD_API_UNIFIED_YIELDS)
	int GetYieldPerFollowingCity(YieldTypes eYield) const;
	int GetYieldPerXFollowers(YieldTypes eYield) const;
	int GetHolyCityYieldPerForeignFollowers(YieldTypes eYield) const;
	int GetHolyCityYieldPerNativeFollowers(YieldTypes eYield) const;
	int GetCityYieldPerOtherReligion(YieldTypes eYield) const;
	int GetYieldPerOtherReligionFollower(YieldTypes eYield) const;
	int GetCuttingInstantYieldModifier(YieldTypes eYield) const;
	int GetCuttingInstantYield(YieldTypes eYield) const;
#endif
	int GetResourceQuantityModifier(ResourceTypes eResource) const;
	int GetImprovementYieldChange(ImprovementTypes eImprovement, YieldTypes eYield) const;
	int GetImprovementAdjacentCityYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2) const;
	int GetBuildingClassYieldChange(BuildingClassTypes eBuildingClass, YieldTypes eYieldType, int iFollowers) const;
	int GetBuildingClassHappiness(BuildingClassTypes eBuildingClass, int iFollowers) const;
	int GetBuildingClassTourism(BuildingClassTypes eBuildingClass) const;
	int GetFeatureYieldChange(FeatureTypes eFeature, YieldTypes eYieldType) const;
#if defined(MOD_API_UNIFIED_YIELDS)
	int GetCityYieldFromUnimprovedFeature(FeatureTypes eFeature, YieldTypes eYieldType) const;
	int GetUnimprovedFeatureYieldChange(FeatureTypes eFeature, YieldTypes eYieldType) const;
#endif
	int GetResourceYieldChange(ResourceTypes eResource, YieldTypes eYieldType) const;
	int GetTerrainYieldChange(TerrainTypes eTerrain, YieldTypes eYieldType) const;
	int GetTerrainYieldChangeAdditive(TerrainTypes eTerrain, YieldTypes eYieldType) const;
	int GetTerrainCityYieldChanges(TerrainTypes eTerrain, YieldTypes eYieldType) const;
#if defined(MOD_API_UNIFIED_YIELDS)
	int GetTradeRouteYieldChange(DomainTypes eDomain, YieldTypes eYieldType) const;
	int GetSpecialistYieldChange(SpecialistTypes eSpecialist, YieldTypes eYieldType) const;
	int GetGreatPersonExpendedYield(GreatPersonTypes eGreatPerson, YieldTypes eYieldType) const;
	int GetGoldenAgeGreatPersonRateModifier(GreatPersonTypes eGreatPerson) const;
	int GetCapitalYieldChange(int iPopulation, YieldTypes eYield) const;
	int GetCoastalCityYieldChange(int iPopulation, YieldTypes eYield) const;
	int GetGreatWorkYieldChange(int iPopulation, YieldTypes eYield) const;
	int GetYieldFromBarbarianKills(YieldTypes eYield) const;
	int GetYieldFromKills(YieldTypes eYield) const;
#endif
#if defined(MOD_RELIGION_PLOT_YIELDS)
	int GetPlotYieldChange(PlotTypes ePlot, YieldTypes eYieldType) const;
#endif
#if defined(MOD_BELIEF_NEW_EFFECT_FOR_SP)
	int GetGreatPersonPoints(GreatPersonTypes eGreatPersonTypes, bool bCapital, bool bHolyCity) const;
	int GetTerrainCityFoodConsumption(TerrainTypes eTerrain) const;
	int GetYieldPerBirth(YieldTypes eYieldType) const;
	int GetLakePlotYieldChange(YieldTypes eYieldType) const;
	int GetRiverPlotYieldChange(YieldTypes eYieldType) const;
#endif
	int GetResourceHappiness(ResourceTypes eResource) const;
	int GetYieldChangeAnySpecialist(YieldTypes eYieldType) const;
	int GetYieldChangeTradeRoute(YieldTypes eYieldType) const;
	int GetYieldChangeNaturalWonder(YieldTypes eYieldType) const;
	int GetYieldChangeWorldWonder(YieldTypes eYieldType) const;
	int GetYieldModifierNaturalWonder(YieldTypes eYieldType) const;
	int GetMaxYieldModifierPerFollower(YieldTypes eYieldType) const;
	int GetYieldModifierPerFollowerTimes100(YieldTypes eYieldType) const;

	bool IsBuildingClassEnabled(BuildingClassTypes eType) const;
	bool IsFaithBuyingEnabled(EraTypes eEra) const;
	bool IsConvertsBarbarians() const;
	bool IsFaithPurchaseAllGreatPeople() const;

	// Serialization
	void Read(FDataStream& kStream);
	void Write(FDataStream& kStream) const;

private:
	// Cached data about this religion's beliefs
	int m_iFaithFromDyingUnits;
	int m_iRiverHappiness;
	int m_iPlotCultureCostModifier;
	int m_iCityRangeStrikeModifier;
	int m_iCombatModifierEnemyCities;
	int m_iCombatModifierFriendlyCities;
	int m_iFriendlyHealChange;
	int m_iCityStateFriendshipModifier;
	int m_iLandBarbarianConversionPercent;
	int m_iSpreadDistanceModifier;
	int m_iSpreadStrengthModifier;
	int m_iProphetStrengthModifier;
	int m_iProphetCostModifier;
	int m_iMissionaryStrengthModifier;
	int m_iMissionaryCostModifier;
	int m_iFriendlyCityStateSpreadModifier;
	int m_iGreatPersonExpendedFaith;
	int m_iCityStateMinimumInfluence;
	int m_iCityStateInfluenceModifier;
	int m_iOtherReligionPressureErosion;
	int m_iSpyPressure;
	int m_iInquisitorPressureRetention;
	int m_iFaithBuildingTourism;

#if defined(MOD_BELIEF_NEW_EFFECT_FOR_SP)
	int m_iGoldenAgeModifier;
	int m_iExtraSpies;
	bool m_bGreatPersonPoints;
	std::vector<int> m_vFreePromotionForProphet;
	int m_iFounderFreePromotion;
	std::tr1::unordered_set<int> m_vFollowingCityFreePromotion;
	int m_iLandmarksTourismPercent;
	int m_iHolyCityUnitExperence;
	int m_iHolyCityPressureModifier;
	int m_iSameReligionMinorRecoveryModifier;
	int m_iInquisitionFervorTimeModifier;
	int m_iNumInquisitorProhibitSpreadInAlly;
	int m_iCuttingBonusModifier;
	int m_iCityExtraMissionarySpreads;
	bool m_bAllowYieldPerBirth;
#endif
	std::vector<std::vector<int>> m_vImprovementYieldChanges;
	std::vector<std::vector<int>> m_vImprovementAdjacentCityYieldChanges;

	EraTypes m_eObsoleteEra;
	ResourceTypes m_eResourceRevealed;
	TechTypes m_eSpreadModifierDoublingTech;

	BeliefList m_ReligionBeliefs;

	// Arrays
	int* m_paiBuildingClassEnabled;
};

namespace CvBeliefHelpers
{
#if defined(MOD_EVENTS_UNIT_CAPTURE)
	bool ConvertBarbarianUnit(const CvUnit *pByUnit, UnitHandle pUnit);
#else
	bool ConvertBarbarianUnit(CvPlayer *pPlayer, UnitHandle pUnit);
#endif
}

#endif //CIV5_BELIEF_CLASSES_H