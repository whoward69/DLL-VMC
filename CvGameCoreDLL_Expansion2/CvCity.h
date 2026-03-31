/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#pragma once

// city.h

#include "CvCorruption.h"
#ifndef CIV5_CITY_H
#define CIV5_CITY_H

#include "LinkedList.h"
#include "CvUnit.h"
#include "CvAIOperation.h"
#include "CvEnumSerialization.h"
#include "FStlContainerSerialization.h"
#include "FAutoVariable.h"
#include "FAutoVector.h"
#include "CvBuildingClasses.h"
#include "CvPreGame.h"

#include "CvSerialize.h"

// 0 = center of city, 1-6 = the edge of city on points, 7-12 = one tile out
#define NUM_CITY_BUILDING_DISPLAY_SLOTS (13)

class CvPlot;
class CvArea;
class CvGenericBuilding;
class CvCityBuildings;
class CvCityStrategyAI;
class CvCityCitizens;
class CvCityEmphases;
class CvCityReligions;
class CvCityEspionage;
class CvCityCulture;
class CvPlayer;


 struct SCityExtraYields
{
	vector<pair<TerrainTypes, int>> forTerrain, forXTerrain, forTerrainFromBuildings, forTerrainFromReligion;
	vector<pair<FeatureTypes, int>> forFeature, forXFeature, forFeatureFromBuildings, forFeatureFromReligion, forFeatureUnimproved;
	vector<pair<ImprovementTypes, int>> forImprovement;
	vector<pair<SpecialistTypes, int>> forSpecialist;
	vector<pair<ResourceTypes, int>> forResource;
	vector<pair<PlotTypes, int>> forPlot;
	vector<pair<YieldTypes, int>> forYield, forActualYield;
	vector<pair<BuildingClassTypes, int>> forLocalBuilding, forReligionBuilding;
};




#ifdef MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD
enum YieldFromYieldStruct {
	IN_YIELD_TYPE = 0,
	OUT_YIELD_TYPE,
	IN_YIELD_VALUE,
	OUT_YIELD_VALUE,
	STRUCT_LENGTH,
};
#endif

class CvCity : CvGameObjectExtractable
{

public:
	CvCity();
	virtual ~CvCity();

	void ExtractToArg(BasicArguments* arg);
	static void PushToLua(lua_State* L, BasicArguments* arg);
	static void RegistInstanceFunctions();
	static void RegistStaticFunctions();
	static CvCity* Provide(PlayerTypes player, int cityID);

#if defined(MOD_API_EXTENSIONS)
	void init(int iID, PlayerTypes eOwner, int iX, int iY, bool bBumpUnits = true, bool bInitialFounding = true, ReligionTypes eInitialReligion = NO_RELIGION, const char* szName = NULL);
#else
	void init(int iID, PlayerTypes eOwner, int iX, int iY, bool bBumpUnits = true, bool bInitialFounding = true);
#endif
	void uninit();
	void reset(int iID = 0, PlayerTypes eOwner = NO_PLAYER, int iX = 0, int iY = 0, bool bConstructorCall = false);
	void setupGraphical();
	void setupWonderGraphics();
	void setupBuildingGraphics();
	void setupSpaceshipGraphics();


#if defined(MOD_GLOBAL_VENICE_KEEPS_RESOURCES) || defined(MOD_GLOBAL_CS_MARRIAGE_KEEPS_RESOURCES)
	void kill(bool bKeepResources = false);
	void PreKill(bool bKeepResources = false);
#else
	void kill();
	void PreKill();
#endif
	void PostKill(bool bCapital, CvPlot* pPlot, PlayerTypes eOwner);

	CvPlayer* GetPlayer();

	void doTurn();

	bool isThis(const CvCity& pCity) const;

	bool isCitySelected();
	bool canBeSelected() const;
	void updateSelectedCity();

	void updateYield(bool bRecalcPlotYields = true);


	void UpdateAllNonPlotYields();
	void UpdateCityYields(YieldTypes eYield);
	void SetStaticYield(YieldTypes eYield, int iValue);
	int GetStaticYield(YieldTypes eYield) const;


	bool IsIndustrialRouteToCapital() const;
	void SetIndustrialRouteToCapital(bool bValue);
	void DoUpdateIndustrialRouteToCapital();

	void SetRouteToCapitalConnected(bool bValue);
#if defined(MOD_API_EXTENSIONS)
	bool IsRouteToCapitalConnected(void) const;
#else
	bool IsRouteToCapitalConnected(void);
#endif

	void createGreatGeneral(UnitTypes eGreatPersonUnit, bool bIsFree);
	void createGreatAdmiral(UnitTypes eGreatPersonUnit, bool bIsFree);

	CityTaskResult doTask(TaskTypes eTask, int iData1 = -1, int iData2 = -1, bool bOption = false, bool bAlt = false, bool bShift = false, bool bCtrl = false);

	void chooseProduction(UnitTypes eTrainUnit = NO_UNIT, BuildingTypes eConstructBuilding = NO_BUILDING, ProjectTypes eCreateProject = NO_PROJECT, bool bFinish = false, bool bFront = false);

#if defined(MOD_GLOBAL_CITY_WORKING)
	int getBuyPlotDistance() const;
	int getWorkPlotDistance(int iChange = 0) const;
	int GetNumWorkablePlots(int iChange = 0) const;
#endif

	void clearWorkingOverride(int iIndex);
	int countNumImprovedPlots(ImprovementTypes eImprovement = NO_IMPROVEMENT, bool bPotential = false) const;
	int countNumWaterPlots() const;
	int countNumRiverPlots() const;
	int countNumForestPlots() const;

	int findPopulationRank();
	int findBaseYieldRateRank(YieldTypes eYield);
	int findYieldRateRank(YieldTypes eYield);

	UnitTypes allUpgradesAvailable(UnitTypes eUnit, int iUpgradeCount = 0) const;
	bool isWorldWondersMaxed() const;
	bool isTeamWondersMaxed() const;
	bool isNationalWondersMaxed() const;
	bool isBuildingsMaxed() const;

	bool canTrain(UnitTypes eUnit, bool bContinue = false, bool bTestVisible = false, bool bIgnoreCost = false, bool bWillPurchase = false, CvString* toolTipSink = NULL) const;
	bool canTrain(UnitCombatTypes eUnitCombat) const;
#if defined(MOD_API_EXTENSIONS)
	bool canConstruct(BuildingTypes eBuilding, bool bContinue = false, bool bTestVisible = false, bool bIgnoreCost = false, bool bWillPurchase = false, CvString* toolTipSink = NULL) const;
#else
	bool canConstruct(BuildingTypes eBuilding, bool bContinue = false, bool bTestVisible = false, bool bIgnoreCost = false, CvString* toolTipSink = NULL) const;
#endif
	bool canCreate(ProjectTypes eProject, bool bContinue = false, bool bTestVisible = false) const;
	bool canPrepare(SpecialistTypes eSpecialist, bool bContinue = false) const;
	bool canMaintain(ProcessTypes eProcess, bool bContinue = false) const;
	bool canJoin() const;

	bool IsFeatureSurrounded() const;
	void SetFeatureSurrounded(bool bValue);
	void DoUpdateFeatureSurrounded();

	const SCityExtraYields& GetYieldChanges(YieldTypes eYield) const { return m_yieldChanges[eYield]; }

	int GetResourceExtraYield(ResourceTypes eResource, YieldTypes eYield) const;
	void ChangeResourceExtraYield(ResourceTypes eResource, YieldTypes eYield, int iChange);

	int GetFeatureExtraYield(FeatureTypes eFeature, YieldTypes eYield) const;
	void ChangeFeatureExtraYield(FeatureTypes eFeature, YieldTypes eYield, int iChange);

	int GetTerrainExtraYield(TerrainTypes eTerrain, YieldTypes eYield) const;
	void ChangeTerrainExtraYield(TerrainTypes eTerrain, YieldTypes eYield, int iChange);


	int GetImprovementExtraYield(ImprovementTypes eImprovement, YieldTypes eYield) const;
	void ChangeImprovementExtraYield(ImprovementTypes eImprovement, YieldTypes eYield, int iChange);

#if defined(MOD_ROG_CORE)
	int GetYieldPerXFeature(FeatureTypes eFeature, YieldTypes eYield) const;

	int GetYieldPerXTerrain(TerrainTypes eTerrain, YieldTypes eYield) const;

	void UpdateYieldPerXTerrain(YieldTypes eYield, TerrainTypes eTerrain = NO_TERRAIN);

	void ChangeNumTerrainWorked(TerrainTypes eTerrain, int iChange);
	int GetNumTerrainWorked(TerrainTypes eTerrain);

	void ChangeNumFeaturelessTerrainWorked(TerrainTypes eTerrain, int iChange);
	int GetNumFeaturelessTerrainWorked(TerrainTypes eTerrain);

	void ChangeNumFeatureWorked(FeatureTypes eFeature, int iChange);
	int GetNumFeatureWorked(FeatureTypes eFeature);


	void SetYieldPerXTerrain(TerrainTypes eTerrain, YieldTypes eYield, int iValue);


	int GetYieldPerXTerrainFromBuildingsTimes100(TerrainTypes eTerrain, YieldTypes eYield) const;
	void ChangeYieldPerXTerrainFromBuildingsTimes100(TerrainTypes eTerrain, YieldTypes eYield, int iChange);


	int GetYieldPerXFeatureFromBuildingsTimes100(FeatureTypes eFeature, YieldTypes eYield) const;
	void ChangeYieldPerXFeatureFromBuildingsTimes100(FeatureTypes eFeature, YieldTypes eYield, int iChange);


	void SetYieldPerXFeature(FeatureTypes eFeature, YieldTypes eYield, int iValue);
	void UpdateYieldPerXFeature(YieldTypes eYield, FeatureTypes eFeature = NO_FEATURE);

	void ChangeNumImprovementWorked(ImprovementTypes eImprovement, int iChange);
	int GetNumImprovementWorked(ImprovementTypes eImprovement);
#endif



	int getSpecialistExtraYield(SpecialistTypes eIndex1, YieldTypes eIndex2) const;
	void changeSpecialistExtraYield(SpecialistTypes eIndex1, YieldTypes eIndex2, int iChange);


#if defined(MOD_API_UNIFIED_YIELDS) && defined(MOD_API_PLOT_YIELDS)
	int GetPlotExtraYield(PlotTypes ePlot, YieldTypes eYield) const;
	void ChangePlotExtraYield(PlotTypes ePlot, YieldTypes eYield, int iChange);
#endif

	bool IsHasResourceLocal(ResourceTypes eResource, bool bTestVisible) const;
#if defined(MOD_API_EXTENSIONS) || defined(MOD_TRADE_WONDER_RESOURCE_ROUTES)
	int GetNumResourceLocal(ResourceTypes eResource, bool bImproved = false);
#endif
	void ChangeNumResourceLocal(ResourceTypes eResource, int iChange);

	bool IsBuildingLocalResourceValid(BuildingTypes eBuilding, bool bTestVisible, CvString* toolTipSink = NULL) const;

#if defined(MOD_ROG_CORE)
	bool IsHasFeatureLocal(FeatureTypes eFeature) const;
	bool IsBuildingFeatureValid(BuildingTypes eBuilding, CvString* toolTipSink = NULL) const;
	bool IsBuildingEmpireResourceValid(BuildingTypes eBuilding, CvString* toolTipSink = NULL) const;
#endif
	bool IsHasPlotLocal(PlotTypes ePlot) const;
	bool IsBuildingPlotValid(BuildingTypes eBuilding, CvString* toolTipSink = NULL) const;
	// Resource Demanded

	ResourceTypes GetResourceDemanded(bool bHideUnknown = true) const;
	void SetResourceDemanded(ResourceTypes eResource);
	void DoPickResourceDemanded(bool bCurrentResourceInvalid = true);
	void DoTestResourceDemanded();

	void DoSeedResourceDemandedCountdown();
	int GetResourceDemandedCountdown() const;
	void SetResourceDemandedCountdown(int iValue);
	void ChangeResourceDemandedCountdown(int iChange);

	// End Resource Demanded

	int getFoodTurnsLeft() const;
	bool isProduction() const;
	bool isProductionLimited() const;
	bool isProductionUnit() const;
	bool isProductionBuilding() const;
	bool isProductionProject() const;
	bool isProductionSpecialist() const;
	bool isProductionProcess() const;

	bool canContinueProduction(OrderData order);
	int getProductionExperience(UnitTypes eUnit = NO_UNIT);
	void addProductionExperience(CvUnit* pUnit, bool bConscript = false);

	UnitTypes getProductionUnit() const;
	UnitAITypes getProductionUnitAI() const;
	BuildingTypes getProductionBuilding() const;
	ProjectTypes getProductionProject() const;
	SpecialistTypes getProductionSpecialist() const;
	ProcessTypes getProductionProcess() const;
	const char* getProductionName() const;
	const char* getProductionNameKey() const;
	int getGeneralProductionTurnsLeft() const;

	bool isFoodProduction() const;
	bool isFoodProduction(UnitTypes eUnit) const;
	int getFirstUnitOrder(UnitTypes eUnit) const;
	int getFirstBuildingOrder(BuildingTypes eBuilding) const;
	int getFirstProjectOrder(ProjectTypes eProject) const;
	int getFirstSpecialistOrder(SpecialistTypes eSpecialist) const;
	int getNumTrainUnitAI(UnitAITypes eUnitAI) const;

	int getProduction() const;
	int getProductionTimes100() const;
	int getProductionNeeded() const;
	int getProductionNeeded(UnitTypes eUnit) const;
	int getProductionNeeded(BuildingTypes eBuilding) const;
	int getProductionNeeded(ProjectTypes eProject) const;
	int getProductionNeeded(SpecialistTypes eSpecialist) const;
	int getProductionTurnsLeft() const;
	int getProductionTurnsLeft(UnitTypes eUnit, int iNum) const;
	int getProductionTurnsLeft(BuildingTypes eBuilding, int iNum) const;
	int getProductionTurnsLeft(ProjectTypes eProject, int iNum) const;
	int getProductionTurnsLeft(SpecialistTypes eSpecialist, int iNum) const;
#if defined(MOD_PROCESS_STOCKPILE)
	int getProductionNeeded(ProcessTypes eProcess) const;
	int getProductionTurnsLeft(ProcessTypes eProcess, int iNum) const;
#endif
	int GetPurchaseCost(UnitTypes eUnit);
	int GetFaithPurchaseCost(UnitTypes eUnit, bool bIncludeBeliefDiscounts);
	int GetPurchaseCost(BuildingTypes eBuilding);
	int GetFaithPurchaseCost(BuildingTypes eBuilding);
	int GetPurchaseCost(ProjectTypes eProject);
	int GetPurchaseCostFromProduction(int iProduction, bool bIsNoHurry = false);

	int getProductionTurnsLeft(int iProductionNeeded, int iProduction, int iFirstProductionDifference, int iProductionDifference) const;
	void setProduction(int iNewValue);
	void changeProduction(int iChange);
	void setProductionTimes100(int iNewValue);
	void changeProductionTimes100(int iChange);

	int getProductionModifier(_In_opt_ CvString* toolTipSink = NULL) const;
	int getGeneralProductionModifiers(_In_opt_ CvString* toolTipSink = NULL) const;
	int getProductionModifier(UnitTypes eUnit, _In_opt_ CvString* toolTipSink = NULL) const;
	int getProductionModifier(BuildingTypes eBuilding, _In_opt_ CvString* toolTipSink = NULL) const;
	int getProductionModifier(ProjectTypes eProject, _In_opt_ CvString* toolTipSink = NULL) const;
	int getProductionModifier(SpecialistTypes eSpecialist, _In_opt_ CvString* toolTipSink = NULL) const;
	int getProductionModifier(ProcessTypes eProcess, _In_opt_ CvString* toolTipSink = NULL) const;

	int getOverflowProductionDifference(int iProductionNeeded, int iProduction, int iProductionModifier, int iDiff, int iModifiedProduction) const;
	int getProductionDifference(int iProductionNeeded, int iProduction, int iProductionModifier, bool bFoodProduction, bool bOverflow) const;
	int getCurrentProductionDifference(bool bIgnoreFood, bool bOverflow) const;
	int getRawProductionDifference(bool bIgnoreFood, bool bOverflow) const;
	int getProductionDifferenceTimes100(int iProductionNeeded, int iProduction, int iProductionModifier, bool bFoodProduction, bool bOverflow) const;
	int getCurrentProductionDifferenceTimes100(bool bIgnoreFood, bool bOverflow) const;
	int getRawProductionDifferenceTimes100(bool bIgnoreFood, bool bOverflow) const;
	int getExtraProductionDifference(int iExtra) const;
	int GetFoodProduction(int iExcessFood) const;

	bool canHurry(HurryTypes eHurry, bool bTestVisible = false) const;
	void hurry(HurryTypes eHurry);

	UnitTypes getConscriptUnit() const;
	CvUnit* initConscriptedUnit();
	int getConscriptPopulation() const;
	int conscriptMinCityPopulation() const;
	bool canConscript() const;
	void conscript();

	int getYieldModifierFromFeature(FeatureTypes eIndex1, YieldTypes eIndex2) const;
	void changeYieldModifierFromFeature(FeatureTypes eIndex1, YieldTypes eIndex2, int iChange);

	int getYieldModifierFromTerrain(TerrainTypes eIndex1, YieldTypes eIndex2) const;
	void changeYieldModifierFromTerrain(TerrainTypes eIndex1, YieldTypes eIndex2, int iChange);

	int getYieldModifierFromImprovement(ImprovementTypes eIndex1, YieldTypes eIndex2) const;
	void changeYieldModifierFromImprovement(ImprovementTypes eIndex1, YieldTypes eIndex2, int iChange);

	int getYieldModifierFromSpecialist(SpecialistTypes eIndex1, YieldTypes eIndex2) const;
	void changeYieldModifierFromSpecialist(SpecialistTypes eIndex1, YieldTypes eIndex2, int iChange);

	int getYieldModifierFromResource(ResourceTypes eIndex1, YieldTypes eIndex2) const;
	void changeYieldModifierFromResource(ResourceTypes eIndex1, YieldTypes eIndex2, int iChange);

	void processFeature(FeatureTypes eFeature, int iChange);
	void processTerrain(TerrainTypes eTerrain, int iChange);
	void processImprovement(ImprovementTypes eImprovement, int iChange);
	void processResource(ResourceTypes eResource, int iChange);
	void processBuilding(BuildingTypes eBuilding, int iChange, bool bFirst, bool bObsolete = false, bool bApplyingAllCitiesBonus = false);
	void processProcess(ProcessTypes eProcess, int iChange);
	void processSpecialist(SpecialistTypes eSpecialist, int iChange);

	void initFreeUnit(CvPlayer& owningPlayer, UnitTypes eUnit, int iCount, bool bToCivType);

	void UpdateReligion(ReligionTypes eNewMajority);
	void UpdateReligiousYieldFromSpecialist(bool bFirstOneAdded);

	int GetCultureFromSpecialist(SpecialistTypes eSpecialist) const;

	CvHandicapInfo& getHandicapInfo() const;
	HandicapTypes getHandicapType() const;

	CvCivilizationInfo& getCivilizationInfo() const;
	CivilizationTypes getCivilizationType() const;

	LeaderHeadTypes getPersonalityType() const;

	ArtStyleTypes getArtStyleType() const;

	CitySizeTypes getCitySizeType() const;

	bool isBarbarian() const;
	bool isHuman() const;
	bool isVisible(TeamTypes eTeam, bool bDebug) const;

	bool isCapital() const;
	bool IsOriginalCapital() const;
	bool IsOriginalMajorCapital() const; // is the original capital of a major civ
	bool IsEverCapital() const;
	void SetEverCapital(bool bValue);

	bool isCoastal(int iMinWaterSize = -1) const;

#if defined(MOD_VOLCANO_BREAK)
	int getImmueVolcanoDamage() const;
	void changeImmueVolcanoDamage(int iChange);
	bool isImmueVolcanoDamage() const;
#endif

	void changeAddsFreshWater(int iChange);
	bool isAddsFreshWater() const;
	
	void changeExtraAttackOnKill(int iChange);
	bool isExtraAttackOnKill() const;


	int getForbiddenForeignSpyCount() const;
	void changeForbiddenForeignSpyCount(int iChange);
	bool isForbiddenForeignSpy() const;

	int GetAllowSpaceshipLaunchCount() const;
	void ChangeAllowSpaceshipLaunchCount(int iChange);
	bool IsAllowSpaceshipLaunch() const;

	int foodConsumption(bool bNoAngry = false, int iExtra = 0) const;
	int foodDifference(bool bBottom = true) const;
	int foodDifferenceTimes100(bool bBottom = true, CvString* toolTipSink = NULL) const;
	int growthThreshold() const;

	int productionLeft() const;
	int hurryCost(HurryTypes eHurry, bool bExtra) const;
	int getHurryCostModifier(HurryTypes eHurry, bool bIgnoreNew = false) const;
	int hurryGold(HurryTypes eHurry) const;
	int hurryPopulation(HurryTypes eHurry) const;
	int hurryProduction(HurryTypes eHurry) const;
	int maxHurryPopulation() const;

	bool hasActiveWorldWonder() const;

	inline int GetID() const
	{
		return m_iID;
	}

	int getIndex() const;
	IDInfo GetIDInfo() const;
	void SetID(int iID);

	inline int getX() const
	{
		return m_iX;
	}

	inline int getY() const
	{
		return m_iY;
	}

	bool at(int iX, int iY) const;
	bool at(CvPlot* pPlot) const;
	CvPlot* plot() const;
	int getArea() const;
	CvArea* area() const;
	CvArea* waterArea() const;

	CvUnit* GetGarrisonedUnit() const;

	CvPlot* getRallyPlot() const;
	void setRallyPlot(CvPlot* pPlot);

	int getGameTurnFounded() const;
	void setGameTurnFounded(int iNewValue);

	int getGameTurnAcquired() const;
	void setGameTurnAcquired(int iNewValue);

	int getGameTurnLastExpanded() const;
	void setGameTurnLastExpanded(int iNewValue);

#if defined(MOD_GLOBAL_CITY_AUTOMATON_WORKERS)
	int getPopulation(bool bIncludeAutomatons = false) const;
#else
	int getPopulation() const;
#endif
	void setPopulation(int iNewValue, bool bReassignPop = true);
	void changePopulation(int iChange, bool bReassignPop = true);
	
#if defined(MOD_GLOBAL_CITY_AUTOMATON_WORKERS)
	int getAutomatons() const;
	void setAutomatons(int iNewValue, bool bReassignPop = true);
	void changeAutomatons(int iChange, bool bReassignPop = true);
#endif

	long getRealPopulation() const;

	int getHighestPopulation() const;
	void setHighestPopulation(int iNewValue);

	int getNumGreatPeople() const;
	void changeNumGreatPeople(int iChange);

	int getBaseGreatPeopleRate() const;
	int getGreatPeopleRate() const;
	int getTotalGreatPeopleRateModifier() const;
	void changeBaseGreatPeopleRate(int iChange);

	int getGreatPeopleRateModifier() const;
	void changeGreatPeopleRateModifier(int iChange);

	// Culture stuff

	int GetJONSCultureStored() const;
	void SetJONSCultureStored(int iValue);
	void ChangeJONSCultureStored(int iChange);

	int GetJONSCultureLevel() const;
	void SetJONSCultureLevel(int iValue);
	void ChangeJONSCultureLevel(int iChange);
	void DoJONSCultureLevelIncrease();
	int GetJONSCultureThreshold() const;

	int getJONSCulturePerTurn(bool bStatic = true) const;

	int GetBaseJONSCulturePerTurn() const;

	int GetJONSCulturePerTurnFromBuildings() const;


	int GetJONSCulturePerTurnFromPolicies() const;
	void ChangeJONSCulturePerTurnFromPolicies(int iChange);

	int GetJONSCulturePerTurnFromSpecialists() const;
	void ChangeJONSCulturePerTurnFromSpecialists(int iChange);

	int GetJONSCulturePerTurnFromGreatWorks() const;

	int GetJONSCulturePerTurnFromTraits() const;

	int GetJONSCulturePerTurnFromReligion() const;

	int GetJONSCulturePerTurnFromLeagues() const;

	int getCultureRateModifier() const;
	void changeCultureRateModifier(int iChange);

	void SetBaseTourism(int iValue);
	int GetBaseTourism() const;
	void SetBaseTourismBeforeModifiers(int iValue);
	int GetBaseTourismBeforeModifiers() const;
	// END Culture

#if defined(MOD_API_EXTENSIONS)
	int getTourismRateModifier() const;
	void changeTourismRateModifier(int iChange);
#endif


	int GetFaithPerTurn(bool bStatic = true) const;

	int GetFaithPerTurnFromBuildings() const;

	int GetFaithPerTurnFromPolicies() const;

#if defined(MOD_API_UNIFIED_YIELDS)
	int GetYieldPerTurnFromUnimprovedFeatures(YieldTypes eYield) const;
#else
	int GetFaithPerTurnFromTraits() const;
#endif
	int GetYieldPerTurnFromAdjacentFeatures(YieldTypes eYield) const;

	int GetFaithPerTurnFromReligion() const;

	int getNumWorldWonders() const;
	void changeNumWorldWonders(int iChange);

	int getNumTeamWonders() const;
	void changeNumTeamWonders(int iChange);

	int getNumNationalWonders() const;
	void changeNumNationalWonders(int iChange);

	int GetWonderProductionModifier() const;
	void ChangeWonderProductionModifier(int iChange);

	int GetLocalResourceWonderProductionMod(BuildingTypes eBuilding, CvString* toolTipSink = NULL) const;

	int getCapturePlunderModifier() const;
	void changeCapturePlunderModifier(int iChange);

	int getPlotCultureCostModifier() const;
	void changePlotCultureCostModifier(int iChange);

	int getPlotBuyCostModifier() const;
	void changePlotBuyCostModifier(int iChange);
	int GetUnitMaxExperienceLocal() const;
	void ChangeUnitMaxExperienceLocal(int iChange);
	int GetTradeRouteRiverBonusModifier() const;
	void ChangeTradeRouteRiverBonusModifier(int iChange);
	int GetSecondCapitalsExtraScore() const;
	void ChangeSecondCapitalsExtraScore(int iChange);
	int GetFoodKeptFromPollution() const;
	void ChangeFoodKeptFromPollution(int iChange);
	bool IsAllowsFoodTradeRoutes();
	void ChangeNumAllowsFoodTradeRoutes(int iChange);
	bool IsAllowsProductionTradeRoutes();
	void ChangeNumAllowsProductionTradeRoutes(int iChange);
#if defined(MOD_BUILDINGS_CITY_WORKING)
	int GetCityWorkingChange() const;
	void changeCityWorkingChange(int iChange);
#endif
#if defined(MOD_BUILDINGS_CITY_AUTOMATON_WORKERS)
	int GetCityAutomatonWorkersChange() const;
	void changeCityAutomatonWorkersChange(int iChange);
#endif

	int getHealRate() const;
	void changeHealRate(int iChange);

	int GetEspionageModifier() const;
	void ChangeEspionageModifier(int iChange);

#if defined(MOD_RELIGION_CONVERSION_MODIFIERS)
	int GetConversionModifier() const;
	void ChangeConversionModifier(int iChange);
#endif

	bool IsNoOccupiedUnhappiness() const;
	int GetNoOccupiedUnhappinessCount() const;
	void ChangeNoOccupiedUnhappinessCount(int iChange);

	int getFood() const;
	int getFoodTimes100() const;
	void setFood(int iNewValue);
	void setFoodTimes100(int iNewValue);
	void changeFood(int iChange);
	void changeFoodTimes100(int iChange);

	int getFoodKept() const;
	void setFoodKept(int iNewValue);
	void changeFoodKept(int iChange);

	int getMaxFoodKeptPercent() const;
	void changeMaxFoodKeptPercent(int iChange);

	int getOverflowProduction() const;
	void setOverflowProduction(int iNewValue);
	void changeOverflowProduction(int iChange);
	int getOverflowProductionTimes100() const;
	void setOverflowProductionTimes100(int iNewValue);
	void changeOverflowProductionTimes100(int iChange);

	int getFeatureProduction()const;
	void setFeatureProduction(int iNewValue);
	void changeFeatureProduction(int iChange);

	int getMilitaryProductionModifier() const;
	void changeMilitaryProductionModifier(int iChange);

	int getSpaceProductionModifier() const;
	void changeSpaceProductionModifier(int iChange);

	int getFreeExperience() const;
	void changeFreeExperience(int iChange);

#if defined(MOD_BELIEF_NEW_EFFECT_FOR_SP)
	int GetGreatPersonPointsFromReligion(GreatPersonTypes eGreatPersonTypes);
	int GetReligionExtraMissionarySpreads(ReligionTypes eReligion);
	int GetBeliefExtraMissionarySpreads(BeliefTypes eBelief);
	int GetCuttingBonusModifier() const;
	void DoCuttingExtraInstantYield(int iBaseYield);
#endif	
#if defined(MOD_GLOBAL_BUILDING_INSTANT_YIELD)
#if defined(MOD_BELIEF_NEW_EFFECT_FOR_SP)
	void doRelogionInstantYield(ReligionTypes eReligion);
	void doBeliefInstantYield(BeliefTypes eBelief);
#endif	
	void doBuildingInstantYield(int* iInstantYieldArray);
	void doInstantYield(YieldTypes iYield, int iValue);
#endif

	void ChangeNumCanAirlift(int iChange);
	bool CanAirlift() const;

	int GetMaxAirUnits() const;
	void ChangeMaxAirUnits(int iChange);

	int getNukeModifier() const;
	void changeNukeModifier(int iChange);

	int GetTradeRouteTargetBonus() const;
	void ChangeTradeRouteTargetBonus(int iChange);

	int GetTradeRouteRecipientBonus() const;
	void ChangeTradeRouteRecipientBonus(int iChange);

	bool isAreaBorderObstacle() const;

	bool IsResistance() const;
	int GetResistanceTurns() const;
	void ChangeResistanceTurns(int iChange);
	void DoResistanceTurn();

	bool IsRazing() const;
	int GetRazingTurns() const;
	void ChangeRazingTurns(int iChange);
	bool DoRazingTurn();

	bool IsOccupied() const;
	void SetOccupied(bool bValue);

	bool IsPuppet() const;
	void SetPuppet(bool bValue);
	void DoCreatePuppet();
	void DoAnnex();

	int GetLocalHappiness() const;
	int GetHappinessFromBuildings() const;
	int GetBaseHappinessFromBuildings() const;
	void ChangeBaseHappinessFromBuildings(int iChange);
	int GetUnmoddedHappinessFromBuildings() const;
	void ChangeUnmoddedHappinessFromBuildings(int iChange);

	bool IsIgnoreCityForHappiness() const;
	void SetIgnoreCityForHappiness(bool bValue);

	BuildingTypes ChooseFreeCultureBuilding() const;
	BuildingTypes ChooseFreeFoodBuilding() const;

	int getCitySizeBoost() const;
	void setCitySizeBoost(int iBoost);

	bool isNeverLost() const;
	void setNeverLost(bool bNewValue);

	bool isDrafted() const;
	void setDrafted(bool bNewValue);

	bool IsOwedCultureBuilding() const;
	void SetOwedCultureBuilding(bool bNewValue);

#if defined(MOD_BUGFIX_FREE_FOOD_BUILDING)
	bool IsOwedFoodBuilding() const;
	void SetOwedFoodBuilding(bool bNewValue);
#endif

	void ChangeNearbyMountains(int iNewValue);
	int GetNearbyMountains() const;
	void SetNearbyMountains(int iValue);

	bool IsBlockaded() const;

	int GetWeLoveTheKingDayCounter() const;
	void SetWeLoveTheKingDayCounter(int iValue);
	void ChangeWeLoveTheKingDayCounter(int iChange);

	int GetLastTurnGarrisonAssigned() const;
	void SetLastTurnGarrisonAssigned(int iValue);

	int GetNumThingsProduced() const;

	bool isProductionAutomated() const;
	void setProductionAutomated(bool bNewValue, bool bClear);

	bool isLayoutDirty() const;
	void setLayoutDirty(bool bNewValue);

	inline PlayerTypes getOwner() const
	{
		return m_eOwner;
	}

	TeamTypes getTeam() const;

	PlayerTypes getPreviousOwner() const;
	void setPreviousOwner(PlayerTypes eNewValue);

	PlayerTypes getOriginalOwner() const;
	void setOriginalOwner(PlayerTypes eNewValue);

	PlayerTypes GetPlayersReligion() const;
	void SetPlayersReligion(PlayerTypes eNewValue);

	// Yield

	int getSeaPlotYield(YieldTypes eIndex) const;
	void changeSeaPlotYield(YieldTypes eIndex, int iChange);

	int getRiverPlotYield(YieldTypes eIndex) const;
	void changeRiverPlotYield(YieldTypes eIndex, int iChange);

	int getLakePlotYield(YieldTypes eIndex) const;
	void changeLakePlotYield(YieldTypes eIndex, int iChange);

	int getSeaResourceYield(YieldTypes eIndex) const;
	void changeSeaResourceYield(YieldTypes eIndex, int iChange);

#if defined(MOD_API_UNIFIED_YIELDS)
	int GetYieldPerTurnFromReligion(ReligionTypes eReligion, YieldTypes eYield) const;
#endif

#ifdef MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD
	int GetBaseYieldRateFromOtherYield(YieldTypes eYield) const;
#endif

	int getBaseYieldRateModifier(YieldTypes eIndex, int iExtra = 0, CvString* toolTipSink = NULL) const;
	int getYieldRate(YieldTypes eIndex, bool bIgnoreTrade, bool bStatic = true) const;
	int getYieldRateTimes100(YieldTypes eIndex, bool bIgnoreTrade, bool bStatic = true) const;
	CvString getYieldRateInfoTool(YieldTypes eIndex, bool bIgnoreTrade = true) const;
#if defined(MOD_PROCESS_STOCKPILE)
	int getBasicYieldRateTimes100(const YieldTypes eIndex, const bool bIgnoreTrade, const bool bIgnoreFromOtherYield) const;
#endif

	// Base Yield
	int getBaseYieldRate(const YieldTypes eIndex, const bool bIgnoreFromOtherYield) const;
	void UpdateCityYieldFromYield(YieldTypes eIndex1, YieldTypes eIndex2, int iModifiedYield);
#if defined(MOD_API_UNIFIED_YIELDS_MORE)
	int getNumForeignSpy() const;
	int getCrimeFromSpy() const;
	int getCrimeFromOpinion() const;
	int getCrimeFromGarrisonedUnit() const;
	int getDiseaseFromConnectionAndTradeRoute() const;
	int getTradeRouteNum() const;
	int getHeresyFromDiscord() const;
#endif

#if defined(MOD_GLOBAL_GREATWORK_YIELDTYPES) || defined(MOD_API_UNIFIED_YIELDS)
	int GetBaseYieldRateFromGreatWorks(YieldTypes eIndex) const;
#endif

	int GetBaseYieldRateFromPolicy(YieldTypes eIndex) const;
	int GetBaseYieldRateFromTrait(YieldTypes eIndex) const;
	int GetBaseYieldRateFromReligionAllCase(YieldTypes eIndex) const;
	int GetBaseYieldRateFromLeagues(YieldTypes eIndex) const;
	int GetBaseYieldRateFromTerrain(YieldTypes eIndex) const;
	void ChangeBaseYieldRateFromTerrain(YieldTypes eIndex, int iChange);

	int GetBaseYieldRateFromBuildings(YieldTypes eIndex) const;
	void ChangeBaseYieldRateFromBuildings(YieldTypes eIndex, int iChange);

	int GetBaseYieldRateFromBuildingsPolicies(YieldTypes eIndex) const;
	void ChangeBaseYieldRateFromBuildingsPolicies(YieldTypes eIndex, int iChange);

	int GetBaseYieldRateFromSpecialists(YieldTypes eIndex) const;
	void ChangeBaseYieldRateFromSpecialists(YieldTypes eIndex, int iChange);

	int GetBaseYieldRateFromProjects(YieldTypes eIndex) const;
	void ChangeBaseYieldRateFromProjects(YieldTypes eIndex, int iChange);

	int GetBaseYieldRateFromMisc(YieldTypes eIndex) const;
	void ChangeBaseYieldRateFromMisc(YieldTypes eIndex, int iChange);

	int GetBaseYieldRateFromReligion(YieldTypes eIndex) const;
	void ChangeBaseYieldRateFromReligion(YieldTypes eIndex, int iChange);

	int GetYieldPerPopTimes100(YieldTypes eIndex) const;
	void ChangeYieldPerPopTimes100(YieldTypes eIndex, int iChange);

	int GetYieldPerPopInEmpireTimes100(YieldTypes eIndex) const;
	void ChangeYieldPerPopInEmpireTimes100(YieldTypes eIndex, int iChange);

	int GetYieldPerReligionTimes100(YieldTypes eIndex) const;
	void ChangeYieldPerReligionTimes100(YieldTypes eIndex, int iChange);
	// END Base Yield

	int GetYieldPerEra(YieldTypes eIndex) const;
	void ChangeYieldPerEra(YieldTypes eIndex, int iChange);
	int GetYieldModifierPerEra(YieldTypes eIndex) const;
	void ChangeYieldModifierPerEra(YieldTypes eIndex, int iChange);

	int GetCityStateTradeRouteYieldModifier(YieldTypes eIndex) const;
	void ChangeCityStateTradeRouteYieldModifier(YieldTypes eIndex, int iChange);

	void changeNukeInterceptionChance(int iValue);
	int getNukeInterceptionChance() const;


	int GetResourceQuantityFromPOP(ResourceTypes eResource) const;
	void ChangeResourceQuantityFromPOP(ResourceTypes eResource, int iChange);

	int GetYieldFromInternalTR(YieldTypes eIndex1) const;
	void ChangeYieldFromInternalTR(YieldTypes eIndex, int iChange);

	int GetYieldFromProcessModifier(YieldTypes eIndex1) const;
	void ChangeYieldFromProcessModifier(YieldTypes eIndex, int iChange);

	int GetRealYieldFromYield(YieldTypes eIndex1, YieldTypes eIndex2) const;
	void SetRealYieldFromYield(YieldTypes eIndex1, YieldTypes eIndex2, int iValue);

#if defined(MOD_ROG_CORE)
	void ChangeSpecialistRateModifier(SpecialistTypes eSpecialist, int iChange);
	int GetSpecialistRateModifier(SpecialistTypes eSpecialist) const;

	int getLocalBuildingClassYield(BuildingClassTypes eIndex1, YieldTypes eIndex2)	const;
	void changeLocalBuildingClassYield(BuildingClassTypes eIndex1, YieldTypes eIndex2, int iChange);
#endif


#if defined(MOD_BUILDING_IMPROVEMENT_RESOURCES)
	int GetResourceFromImprovement(ResourceTypes eResource, ImprovementTypes eImprovement) const;
	void ChangeResourceFromImprovement(ResourceTypes eResource, ImprovementTypes eImprovement, int iChange);
#endif

	int getYieldRateModifier(YieldTypes eIndex) const;
	void changeYieldRateModifier(YieldTypes eIndex, int iChange);

	int getYieldRateMultiplier(YieldTypes eIndex) const;
	void changeYieldRateMultiplier(YieldTypes eIndex, int iChange);

	int getPowerYieldRateModifier(YieldTypes eIndex) const;
	void changePowerYieldRateModifier(YieldTypes eIndex, int iChange);

	int getAdjacentFeaturesYieldRateModifier(YieldTypes eIndex) const;

	int getFeatureYieldRateModifier(YieldTypes eIndex) const;
	void changeFeatureYieldRateModifier(YieldTypes eIndex, int iChange);

	int getTerrainYieldRateModifier(YieldTypes eIndex) const;
	void changeTerrainYieldRateModifier(YieldTypes eIndex, int iChange);

	int getImprovementYieldRateModifier(YieldTypes eIndex) const;
	void changeImprovementYieldRateModifier(YieldTypes eIndex, int iChange);

	int getSpecialistYieldRateModifier(YieldTypes eIndex) const;
	void changeSpecialistYieldRateModifier(YieldTypes eIndex, int iChange);

	int getResourceYieldRateModifier(YieldTypes eIndex) const;
	void changeResourceYieldRateModifier(YieldTypes eIndex, int iChange);

	int getHappinessModifier(YieldTypes eIndex) const;

	int getExtraSpecialistYield(YieldTypes eIndex) const;
	int getExtraSpecialistYield(YieldTypes eIndex, SpecialistTypes eSpecialist) const;
	int getExtraYieldPerSpecialist(YieldTypes eIndex, SpecialistTypes eSpecialist) const;
	int getSpecialistYield(YieldTypes eIndex, SpecialistTypes eSpecialist) const;
	void updateExtraSpecialistYield(YieldTypes eYield);
	void updateExtraSpecialistYield();

	int getProductionToYieldModifier(YieldTypes eIndex) const;
	void changeProductionToYieldModifier(YieldTypes eIndex, int iChange);

	int GetTradeYieldModifier(YieldTypes eIndex, CvString* toolTipSink = NULL) const;

	// END Yield

	int getDomainFreeExperience(DomainTypes eIndex) const;
	void changeDomainFreeExperience(DomainTypes eIndex, int iChange);

	int getDomainFreeExperienceFromGreatWorks(DomainTypes eIndex) const;
	int GetDomainFreeExperiencesPerPop(DomainTypes eIndex) const;
	void changeDomainFreeExperiencesPerPop(DomainTypes eIndex, int iChange);
	int GetDomainFreeExperiencesPerPopGlobal(DomainTypes eIndex) const;
	void changeDomainFreeExperiencesPerPopGlobal(DomainTypes eIndex, int iChange);
	int GetDomainFreeExperiencesPerTurn(DomainTypes eIndex) const;
	void changeDomainFreeExperiencesPerTurn(DomainTypes eIndex, int iChange);
	int GetDomainEnemyCombatModifier(DomainTypes eIndex) const;
	void changeDomainEnemyCombatModifier(DomainTypes eIndex, int iChange);
	int GetDomainFriendsCombatModifierLocal(DomainTypes eIndex) const;
	void changeDomainFriendsCombatModifierLocal(DomainTypes eIndex, int iChange);
	
#if defined(MOD_ROG_CORE)
	int GetBaseYieldRateFromCSAlliance(YieldTypes eIndex) const;
	void ChangeBaseYieldRateFromCSAlliance(YieldTypes eIndex, int iChange);
	void SetBaseYieldRateFromCSAlliance(YieldTypes eIndex, int iValue);

	int GetBaseYieldRateFromCSFriendship(YieldTypes eIndex) const;
	void ChangeBaseYieldRateFromCSFriendship(YieldTypes eIndex, int iChange);
	void SetBaseYieldRateFromCSFriendship(YieldTypes eIndex, int iValue);

	int GetYieldPerAlly(YieldTypes eIndex) const;
	void ChangeYieldPerAlly(YieldTypes eYield, int iChange);
	int GetYieldPerFriend(YieldTypes eIndex) const;
	void ChangeYieldPerFriend(YieldTypes eYield, int iChange);
	int GetBuildingYieldFromYield(YieldTypes eIndex1, YieldTypes eIndex2) const;
	void ChangeBuildingYieldFromYield(YieldTypes eIndex, YieldTypes eIndex2, int iValue);
	int GetYieldPerEspionageSpy(YieldTypes eIndex) const;
	void ChangeYieldPerEspionageSpy(YieldTypes eYield, int iChange);
	int GetBaseYieldRateFromEspionageSpy(YieldTypes eIndex) const;
	void ChangeBaseYieldRateFromEspionageSpy(YieldTypes eIndex, int iChange);
	void SetBaseYieldRateFromEspionageSpy(YieldTypes eIndex, int iValue);

	int GetYieldFromConstruction(YieldTypes eIndex) const;
	void ChangeYieldFromConstruction(YieldTypes eIndex, int iChange);
	int GetYieldFromUnitProduction(YieldTypes eIndex) const;
	void ChangeYieldFromUnitProduction(YieldTypes eIndex, int iChange);
	int GetYieldFromBirth(YieldTypes eIndex) const;
	void ChangeYieldFromBirth(YieldTypes eIndex, int iChange);
	int GetYieldFromBorderGrowth(YieldTypes eIndex) const;
	void ChangeYieldFromBorderGrowth(YieldTypes eIndex, int iChange);
	int GetYieldFromPillage(YieldTypes eIndex) const;
	void ChangeYieldFromPillage(YieldTypes eIndex, int iChange);
	int getDomainFreeExperienceFromGreatWorksGlobal(DomainTypes eIndex) const;
#endif

	int getDomainProductionModifier(DomainTypes eIndex) const;
	void changeDomainProductionModifier(DomainTypes eIndex, int iChange);

	bool isEverOwned(PlayerTypes eIndex) const;
	void setEverOwned(PlayerTypes eIndex, bool bNewValue);

	bool isRevealed(TeamTypes eIndex, bool bDebug) const;
	bool setRevealed(TeamTypes eIndex, bool bNewValue);

	const CvString getName() const;
	const char* getNameKey() const;
	void setName(const char* szNewValue, bool bFound = false);
	void doFoundMessage();

	bool IsExtraLuxuryResources();
	void SetExtraLuxuryResources(int iNewValue);
	void ChangeExtraLuxuryResources(int iChange);

	CvCityBuildings* GetCityBuildings() const;

	int getProjectProduction(ProjectTypes eIndex) const;
	void setProjectProduction(ProjectTypes eIndex, int iNewValue);
	void changeProjectProduction(ProjectTypes eIndex, int iChange);
	int getProjectProductionTimes100(ProjectTypes eIndex) const;
	void setProjectProductionTimes100(ProjectTypes eIndex, int iNewValue);
	void changeProjectProductionTimes100(ProjectTypes eIndex, int iChange);

	int getSpecialistProduction(SpecialistTypes eIndex) const;
	void setSpecialistProduction(SpecialistTypes eIndex, int iNewValue);
	void changeSpecialistProduction(SpecialistTypes eIndex, int iChange);
	int getSpecialistProductionTimes100(SpecialistTypes eIndex) const;
	void setSpecialistProductionTimes100(SpecialistTypes eIndex, int iNewValue);
	void changeSpecialistProductionTimes100(SpecialistTypes eIndex, int iChange);

#if defined(MOD_PROCESS_STOCKPILE)
	int getProcessProduction(ProcessTypes eIndex) const;
	int getProcessProductionTimes100(ProcessTypes eIndex) const;
#endif

	int getUnitProduction(UnitTypes eIndex) const;
	void setUnitProduction(UnitTypes eIndex, int iNewValue);
	void changeUnitProduction(UnitTypes eIndex, int iChange);
	int getUnitProductionTimes100(UnitTypes eIndex) const;
	void setUnitProductionTimes100(UnitTypes eIndex, int iNewValue);
	void changeUnitProductionTimes100(UnitTypes eIndex, int iChange);

	int getUnitProductionTime(UnitTypes eIndex) const;
	void setUnitProductionTime(UnitTypes eIndex, int iNewValue);
	void changeUnitProductionTime(UnitTypes eIndex, int iChange);

	int getUnitCombatFreeExperience(UnitCombatTypes eIndex) const;
	void changeUnitCombatFreeExperience(UnitCombatTypes eIndex, int iChange);

	int getUnitCombatProductionModifier(UnitCombatTypes eIndex) const;
	void changeUnitCombatProductionModifier(UnitCombatTypes eIndex, int iChange);

	int getFreePromotionCount(PromotionTypes eIndex) const;
	bool isFreePromotion(PromotionTypes eIndex) const;
	void changeFreePromotionCount(PromotionTypes eIndex, int iChange);
	int getFreeFollowingPromotionCount(PromotionTypes eIndex) const;

	int getTradeRouteDomainRangeModifier(DomainTypes eIndex) const;
	void changeTradeRouteDomainRangeModifier(DomainTypes eIndex, int iChange);

	int getTradeRouteDomainGoldBonus(DomainTypes eIndex) const;
	void changeTradeRouteDomainGoldBonus(DomainTypes eIndex, int iChange);

	int getSpecialistFreeExperience() const;
	void changeSpecialistFreeExperience(int iChange);

	void updateStrengthValue();
	int getStrengthValue(bool bForRangeStrike = false) const;
	int GetPower() const;

	int getDamage() const;
	void setDamage(int iValue, bool noMessage=false);
	void changeDamage(int iChange);

	bool isMadeAttack() const;
	void setMadeAttack(bool bNewValue);

#if defined(MOD_EVENTS_CITY_BOMBARD)
	int getBombardRange(bool& bIndirectFireAllowed) const;
	int getBombardRange() const;
#endif

#if defined(MOD_ROG_CORE)
	int getExtraDamageHealPercent() const;
	void changeExtraDamageHealPercent(int iChange);

	int getExtraDamageHeal() const;
	void changeExtraDamageHeal(int iChange);

	int getExtraBombardRange() const;
	void changeExtraBombardRange(int iChange);

	void changeBombardIndirect(int iChange);
	bool isBombardIndirect() const;

	void ChangeNumTimesAttackedThisTurn(PlayerTypes ePlayer, int iValue);
	int GetNumTimesAttackedThisTurn(PlayerTypes ePlayer) const;

	int getCityBuildingRangeStrikeModifier() const;
	void changeCityBuildingRangeStrikeModifier(int iValue);

	int getResetDamageValue()const;
	void changeResetDamageValue(int iChange);

	int getReduceDamageValue()const;
	void changeReduceDamageValue(int iChange);



	int getWaterTileDamage()const;
	void changeWaterTileDamage(int iChange);

	int getWaterTileMovementReduce()const;
	void changeWaterTileMovementReduce(int iChange);

	int getWaterTileTurnDamage()const;
	void changeWaterTileTurnDamage(int iChange);

	int getLandTileDamage()const;
	void changeLandTileDamage(int iChange);

	int getLandTileMovementReduce()const;
	void changeLandTileMovementReduce(int iChange);

	int getLandTileTurnDamage()const;
	void changeLandTileTurnDamage(int iChange);

#endif

	void changeExtraAttacks(int iChange);


	bool canRangeStrike() const;
	bool CanRangeStrikeNow() const;
	bool IsHasBuildingThatAllowsRangeStrike() const;

	bool canRangeStrikeAt(int iX, int iY) const;
	CityTaskResult rangeStrike(int iX, int iY);
	CvUnit* rangedStrikeTarget(CvPlot* pPlot);
	bool canRangedStrikeTarget(const CvPlot& targetPlot) const;

	int rangeCombatUnitDefense(_In_ const CvUnit* pDefender) const;
	int rangeCombatDamage(const CvUnit* pDefender, CvCity* pCity = NULL, bool bIncludeRand = true) const;

	int GetAirStrikeDefenseDamage(const CvUnit* pAttacker, bool bIncludeRand = true) const;

	void DoNearbyEnemy();

	void IncrementUnitStatCount(CvUnit* pUnit);
	void CheckForAchievementBuilding(BuildingTypes eBuilding);
	bool AreAllUnitsBuilt();

	// Plot acquisition

	bool CanBuyPlot(int iPlotX = -1, int iPlotY = -1, bool bIgnoreCost = false);
	bool CanBuyAnyPlot(void);
	CvPlot* GetNextBuyablePlot();
	void GetBuyablePlotList(std::vector<int>& aiPlotList);
	int GetBuyPlotCost(int iPlotX, int iPlotY) const;
	void BuyPlot(int iPlotX, int iPlotY);
	void DoAcquirePlot(int iPlotX, int iPlotY);
	int GetBuyPlotScore(int& iBestX, int& iBestY);
	int GetIndividualPlotScore(const CvPlot* pPlot) const;

	int GetCheapestPlotInfluence() const;
	void SetCheapestPlotInfluence(int iValue);
	void DoUpdateCheapestPlotInfluence();

	// End plot acquisition

	bool isValidBuildingLocation(BuildingTypes eIndex) const;

	void SetThreatValue(int iThreatValue);
	int getThreatValue(void);

	void clearOrderQueue();
	void pushOrder(OrderTypes eOrder, int iData1, int iData2, bool bSave, bool bPop, bool bAppend, bool bRush=false);
	void popOrder(int iNum, bool bFinish = false, bool bChoose = false);
	void swapOrder(int iNum);
	void startHeadOrder();
	void stopHeadOrder();
	int getOrderQueueLength();
	OrderData* getOrderFromQueue(int iIndex);
	const OrderData* nextOrderQueueNode(const OrderData* pNode) const;
	OrderData* nextOrderQueueNode(OrderData* pNode);
	const OrderData* headOrderQueueNode() const;
	OrderData* headOrderQueueNode();
	const OrderData* tailOrderQueueNode() const;
	bool CleanUpQueue(void);  // remove items in the queue that are no longer valid

	int CreateUnit(UnitTypes eUnitType, bool bIsGold, bool bIsFaith,  UnitAITypes eAIType = NO_UNITAI, bool bUseToSatisfyOperation=true);
	bool CreateBuilding(BuildingTypes eBuildType);
	bool CreateProject(ProjectTypes eProjectType, bool bIsCapture = false);
	void changeProjectCount(ProjectTypes eProject, int iValue);
	int getProjectCount(ProjectTypes eProject) const;

	bool CanPlaceUnitHere(UnitTypes eUnitType);
	bool IsCanPurchase(bool bTestPurchaseCost, bool bTestTrainable, UnitTypes eUnitType, BuildingTypes eBuildingType, ProjectTypes eProjectType, YieldTypes ePurchaseYield);
#if defined(MOD_AI_SMART_V3)	
	bool IsCanGoldPurchase(OrderData* pOrder);
	void PurchaseCurrentOrder();
#endif	
	void Purchase(UnitTypes eUnitType, BuildingTypes eBuildingType, ProjectTypes eProjectType, YieldTypes ePurchaseYield);

	PlayerTypes getLiberationPlayer() const;
	void liberate();

	CvCityStrategyAI* GetCityStrategyAI() const;
	CvCityCitizens* GetCityCitizens() const;
	CvCityReligions* GetCityReligions() const;
	CvCityEmphases* GetCityEmphases() const;
	CvCityEspionage* GetCityEspionage() const;
	CvCityCulture* GetCityCulture() const;

	template<typename City, typename Visitor>
	static void Serialize(City& city, Visitor& visitor);

	void read(FDataStream& kStream);
	void write(FDataStream& kStream) const;

	virtual void AI_init() = 0;
	virtual void AI_reset() = 0;
	virtual void AI_doTurn() = 0;
	virtual void AI_chooseProduction(bool bInterruptWonders) = 0;
	virtual bool AI_isChooseProductionDirty() = 0;
	virtual void AI_setChooseProductionDirty(bool bNewValue) = 0;

	virtual int AI_GetNumPlotsAcquiredByOtherPlayer(PlayerTypes ePlayer) const = 0;
	virtual void AI_ChangeNumPlotsAcquiredByOtherPlayer(PlayerTypes ePlayer, int iChange) = 0;

	void invalidatePopulationRankCache();
	void invalidateYieldRankCache(YieldTypes eYield = NO_YIELD);

	bool CommitToBuildingUnitForOperation();
	UnitTypes GetUnitForOperation();
	virtual bool IsBuildingUnitForOperation() const
	{
		return m_unitBeingBuiltForOperation.IsValid();
	}

	const char* GetCityBombardEffectTag() const;
	uint GetCityBombardEffectTagHash() const;

	int GetExtraHitPoints() const;
	void ChangeExtraHitPoints(int iValue);

	int GetMaxHitPoints() const;
	const FAutoArchive& getSyncArchive() const;
	FAutoArchive& getSyncArchive();
	std::string debugDump(const FAutoVariableBase&) const;
	std::string stackTraceRemark(const FAutoVariableBase&) const;

	bool			IsBusy() const;
	// Combat related
	const CvUnit*	getCombatUnit() const;
	CvUnit*			getCombatUnit();
	void			setCombatUnit(CvUnit* pUnit, bool bAttacking = false);
	void			clearCombat();
	bool			isFighting() const;

#if defined(MOD_API_EXTENSIONS)
	bool HasBelief(BeliefTypes iBeliefType) const;
	bool HasBuilding(BuildingTypes iBuildingType) const;
	bool HasBuildingClass(BuildingClassTypes iBuildingClassType) const;
	int GetNumBuildingClass(BuildingClassTypes iBuildingClassType) const;
	void ChangeNumBuildingClass(BuildingClassTypes iBuildingClassType, int iValue);
	bool HasAnyWonder() const;
	bool HasWonder(BuildingTypes iBuildingType) const;
	bool IsCivilization(CivilizationTypes iCivilizationType) const;
	bool HasFeature(FeatureTypes iFeatureType) const;
	bool HasWorkedFeature(FeatureTypes iFeatureType) const;
	bool HasAnyNaturalWonder() const;
	bool HasNaturalWonder(FeatureTypes iFeatureType) const;
	bool HasImprovement(ImprovementTypes iImprovementType) const;
	bool HasWorkedImprovement(ImprovementTypes iImprovementType) const;
	bool HasPlotType(PlotTypes iPlotType) const;
	bool HasWorkedPlotType(PlotTypes iPlotType) const;
	bool HasAnyReligion() const;
	bool HasReligion(ReligionTypes iReligionType) const;
	bool HasResource(ResourceTypes iResourceType) const;
	bool HasWorkedResource(ResourceTypes iResourceType) const;
	bool IsConnectedToCapital() const;
	bool IsConnectedTo(CvCity* pCity) const;
	bool HasSpecialistSlot(SpecialistTypes iSpecialistType) const;
	bool HasSpecialist(SpecialistTypes iSpecialistType) const;
	bool HasTerrain(TerrainTypes iTerrainType) const;
	bool HasWorkedTerrain(TerrainTypes iTerrainType) const;
	bool HasAnyDomesticTradeRoute() const;
	bool HasAnyInternationalTradeRoute() const;
	bool HasTradeRouteToAnyCity() const;
	bool HasTradeRouteTo(CvCity* pCity) const;
	bool HasTradeRouteFromAnyCity() const;
	bool HasTradeRouteFrom(CvCity* pCity) const;
	bool IsOnFeature(FeatureTypes iFeatureType) const;
	bool IsAdjacentToFeature(FeatureTypes iFeatureType) const;
	bool IsWithinDistanceOfFeature(FeatureTypes iFeatureType, int iDistance) const;
	bool IsOnImprovement(ImprovementTypes iImprovementType) const;
	bool IsAdjacentToImprovement(ImprovementTypes iImprovementType) const;
	bool IsWithinDistanceOfImprovement(ImprovementTypes iImprovementType, int iDistance) const;
	bool IsOnPlotType(PlotTypes iPlotType) const;
	bool IsAdjacentToPlotType(PlotTypes iPlotType) const;
	bool IsWithinDistanceOfPlotType(PlotTypes iPlotType, int iDistance) const;
	bool IsOnResource(ResourceTypes iResourceType) const;
	bool IsAdjacentToResource(ResourceTypes iResourceType) const;
	bool IsWithinDistanceOfResource(ResourceTypes iResourceType, int iDistance) const;
	bool IsOnTerrain(TerrainTypes iTerrainType) const;
	bool IsAdjacentToTerrain(TerrainTypes iTerrainType) const;
	bool IsWithinDistanceOfTerrain(TerrainTypes iTerrainType, int iDistance) const;
	int CountFeature(FeatureTypes iFeatureType) const;
	int CountWorkedFeature(FeatureTypes iFeatureType) const;
	int CountImprovement(ImprovementTypes iImprovementType) const;
	int CountUnPillagedImprovement(ImprovementTypes iImprovementType) const;
	int CountWorkedImprovement(ImprovementTypes iImprovementType) const;
	int CountPlotType(PlotTypes iPlotType) const;
	int CountWorkedPlotType(PlotTypes iPlotType) const;
	int CountResource(ResourceTypes iResourceType) const;
	int CountWorkedResource(ResourceTypes iResourceType) const;
	int CountTerrain(TerrainTypes iTerrainType) const;
	int CountWorkedTerrain(TerrainTypes iTerrainType) const;
#endif

	void DoReligionFounderChange();
#ifdef MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD
	//int GetYieldFromOtherYield(const YieldTypes eInType, const YieldTypes eOutType, const YieldFromYield eConvertType) const;
	bool HasYieldFromOtherYield() const;
	//void ChangeYieldFromOtherYield(const YieldTypes eInType, const YieldTypes eOutType, const YieldFromYield eConvertType, const int iChange);
#endif

#if defined(MOD_NUCLEAR_WINTER_FOR_SP)
	bool IsNoNuclearWinterLocal() const;
	void ChangeNumNoNuclearWinterLocal(int iChange);
#endif
#if defined(MOD_TROOPS_AND_CROPS_FOR_SP)
	bool HasEnableCrops() const;
	void ChangeNumEnableCrops(int iChange);
	bool HasEnableArmee() const;
	void ChangeNumEnableArmee(int iChange);
#endif
#if defined(MOD_INTERNATIONAL_IMMIGRATION_FOR_SP)
	bool IsCanDoImmigration() const;
	void SetCanDoImmigration(bool iValue);
	bool CanImmigrantIn() const;
	bool CanImmigrantOut() const;
	bool CanAllScaleImmigrantIn() const;
	void ChangeNumAllScaleImmigrantIn(int iChange);
#endif
#ifdef MOD_GLOBAL_CITY_SCALES
	CityScaleTypes GetScale() const { return m_eCityScale; }
	CvCityScaleEntry* GetScaleInfo() const { return GC.getCityScaleInfo(m_eCityScale); }
	void SetScale(CityScaleTypes eScale);
	void UpdateScaleBuildings();
	bool CanGrowNormally() const;
	bool CanScaleImmigrantIn() const;
	bool CanScaleImmigrantOut() const;
#endif

#ifdef MOD_PROMOTION_CITY_DESTROYER
	int GetSiegeKillCitizensModifier() const;
	void ChangeSiegeKillCitizensModifier(int iChange);
#endif

	int iScratch; // know the scope of your validity

#ifdef MOD_GLOBAL_CORRUPTION
	int GetCorruptionScore() const;
	CorruptionLevelTypes GetCorruptionLevel() const;
	void UpdateCorruption();

	int CalculateTotalCorruptionScore() const;
	int CalculateCorruptionScoreFromDistance() const;
	int CalculateCorruptionScoreFromCoastalBonus() const;
	int CalculateCorruptionScoreFromResource() const;
	int CalculateCorruptionScoreFromTrait() const;
	int CalculateCorruptionScoreModifierFromSpy() const;
	int CalculateCorruptionScoreModifierFromTrait() const;

	CvCorruptionLevel* DecideCorruptionLevelForNormalCity(const int score) const;

	int GetCorruptionScoreChangeFromBuilding() const;
	void ChangeCorruptionScoreChangeFromBuilding(int value);

	int GetCorruptionLevelChangeFromBuilding() const;
	void ChangeCorruptionLevelChangeFromBuilding(int value);

	int GetCorruptionScoreModifierFromPolicy() const;
	int GetMaxCorruptionLevel() const;
	bool IsCorruptionLevelReduceByOne() const;
#endif

	bool IsSecondCapital() const;
	void SetSecondCapital(bool value);

	int GetAdditionalFood() const;
	void SetAdditionalFood(int iValue);


#if defined(MOD_API_UNIFIED_YIELDS_MORE)
	bool IsColony() const;
	void SetColony(bool bValue);


	int GetOrganizedCrime() const;
	void SetOrganizedCrime(int iValue);
	bool HasOrganizedCrime();

	void ChangeResistanceCounter(int iValue);
	void SetResistanceCounter(int iValue);
	int GetResistanceCounter() const;

	void ChangePlagueCounter(int iValue);
	void SetPlagueCounter(int iValue);
	int GetPlagueCounter() const;

	int GetPlagueTurns() const;
	void ChangePlagueTurns(int iValue); //Set in city::doturn
	void SetPlagueTurns(int iValue);

	int GetPlagueType() const;
	void SetPlagueType(int iValue);
	bool HasPlague();

	void ChangeLoyaltyCounter(int iValue);
	void SetLoyaltyCounter(int iValue);
	int GetLoyaltyCounter() const;

	void ChangeDisloyaltyCounter(int iValue);
	void SetDisloyaltyCounter(int iValue);
	int GetDisloyaltyCounter() const;

	int GetLoyaltyState() const;
	void SetLoyaltyState(int iLoyalty);


	void SetYieldModifierFromHealth(YieldTypes eYield, int iValue);
	int GetYieldModifierFromHealth(YieldTypes eYield) const;

	void SetYieldModifierFromCrime(YieldTypes eYield, int iValue);
	int GetYieldModifierFromCrime(YieldTypes eYield) const;

	void SetYieldFromHappiness(YieldTypes eYield, int iValue);
	int GetYieldFromHappiness(YieldTypes eYield) const;

	void SetYieldFromHealth(YieldTypes eYield, int iValue);
	int GetYieldFromHealth(YieldTypes eYield) const;

	void SetYieldFromCrime(YieldTypes eYield, int iValue);
	int GetYieldFromCrime(YieldTypes eYield) const;
#endif
	int GetHurryModifier(HurryTypes eIndex) const;
	int GetHurryModifierLocal(HurryTypes eIndex) const;
	void ChangeHurryModifierLocal(HurryTypes eIndex, int iChange);

	int GetTradeRouteFromTheCityYields(YieldTypes eIndex) const;
	void ChangeTradeRouteFromTheCityYields(YieldTypes eIndex, int iChange);

	int GetTradeRouteFromTheCityYieldsPerEra(YieldTypes eIndex) const;
	void ChangeTradeRouteFromTheCityYieldsPerEra(YieldTypes eIndex, int iChange);

	int GetLastTurnWorkerDisbanded() const;
	void SetLastTurnWorkerDisbanded(int iValue);

	int GetDefendedAgainstSpreadUntilTurn() const;
	void SetDefendedAgainstSpreadUntilTurn(int iValue);

protected:
	FAutoArchiveClassContainer<CvCity> m_syncArchive;

	FAutoVariable<CvString, CvCity> m_strNameIAmNotSupposedToBeUsedAnyMoreBecauseThisShouldNotBeCheckedAndWeNeedToPreserveSaveGameCompatibility;
	FAutoVariable<PlayerTypes, CvCity> m_eOwner;
	FAutoVariable<int, CvCity> m_iX;
	FAutoVariable<int, CvCity> m_iY;
	FAutoVariable<int, CvCity> m_iID;

	FAutoVariable<int, CvCity> m_iRallyX;
	FAutoVariable<int, CvCity> m_iRallyY;
	FAutoVariable<int, CvCity> m_iGameTurnFounded;
	FAutoVariable<int, CvCity> m_iGameTurnAcquired;
	FAutoVariable<int, CvCity> m_iGameTurnLastExpanded;
	FAutoVariable<int, CvCity> m_iPopulation;
#if defined(MOD_GLOBAL_CITY_AUTOMATON_WORKERS)
	int m_iAutomatons;
#endif
	FAutoVariable<int, CvCity> m_iHighestPopulation;
	int m_iExtraHitPoints;

	FAutoVariable<int, CvCity> m_iNumGreatPeople;
	FAutoVariable<int, CvCity> m_iBaseGreatPeopleRate;
	FAutoVariable<int, CvCity> m_iGreatPeopleRateModifier;
	FAutoVariable<int, CvCity> m_iJONSCultureStored;
	FAutoVariable<int, CvCity> m_iJONSCultureLevel;
	FAutoVariable<int, CvCity> m_iJONSCulturePerTurnFromBuildings;
	FAutoVariable<int, CvCity> m_iJONSCulturePerTurnFromPolicies;
	FAutoVariable<int, CvCity> m_iJONSCulturePerTurnFromSpecialists;
	FAutoVariable<int, CvCity> m_iCultureRateModifier;
	FAutoVariable<int, CvCity> m_iNumWorldWonders;
	FAutoVariable<int, CvCity> m_iNumTeamWonders;
	FAutoVariable<int, CvCity> m_iNumNationalWonders;
	FAutoVariable<int, CvCity> m_iWonderProductionModifier;
	FAutoVariable<int, CvCity> m_iCapturePlunderModifier;
	FAutoVariable<int, CvCity> m_iPlotCultureCostModifier;
	int m_iPlotBuyCostModifier;
	int m_iUnitMaxExperienceLocal;
	int m_iTradeRouteRiverBonusModifier;
	int m_iSecondCapitalsExtraScore;
	int m_iFoodKeptFromPollution;
	int m_iNumAllowsFoodTradeRoutes;
	int m_iNumAllowsProductionTradeRoutes;
#if defined(MOD_BUILDINGS_CITY_WORKING)
	int m_iCityWorkingChange;
#endif
#if defined(MOD_BUILDINGS_CITY_AUTOMATON_WORKERS)
	int m_iCityAutomatonWorkersChange;
#endif
	FAutoVariable<int, CvCity> m_iMaintenance;
	FAutoVariable<int, CvCity> m_iHealRate;
	FAutoVariable<int, CvCity> m_iNoOccupiedUnhappinessCount;
	FAutoVariable<int, CvCity> m_iFood;
	FAutoVariable<int, CvCity> m_iFoodKept;
	FAutoVariable<int, CvCity> m_iMaxFoodKeptPercent;
	FAutoVariable<int, CvCity> m_iOverflowProduction;
	FAutoVariable<int, CvCity> m_iFeatureProduction;
	FAutoVariable<int, CvCity> m_iMilitaryProductionModifier;
	FAutoVariable<int, CvCity> m_iSpaceProductionModifier;
	FAutoVariable<int, CvCity> m_iFreeExperience;
	int m_iNumCanAirlift;
	FAutoVariable<int, CvCity> m_iCurrAirlift; // unused
	FAutoVariable<int, CvCity> m_iMaxAirUnits;
	FAutoVariable<int, CvCity> m_iAirModifier; // unused
	FAutoVariable<int, CvCity> m_iNukeModifier;
	int m_iTradeRouteTargetBonus;
	int m_iTradeRouteRecipientBonus;
	FAutoVariable<int, CvCity> m_iCultureUpdateTimer;
	FAutoVariable<int, CvCity> m_iCitySizeBoost;
	FAutoVariable<int, CvCity> m_iSpecialistFreeExperience;
	FAutoVariable<int, CvCity> m_iStrengthValue;
	FAutoVariable<int, CvCity> m_iDamage;
	FAutoVariable<int, CvCity> m_iThreatValue;
	FAutoVariable<int, CvCity> m_iGarrisonedUnit;  // unused
	FAutoVariable<int, CvCity> m_iResourceDemanded;
	FAutoVariable<int, CvCity> m_iWeLoveTheKingDayCounter;
	FAutoVariable<int, CvCity> m_iLastTurnGarrisonAssigned;
	FAutoVariable<int, CvCity> m_iThingsProduced; // total number of units, buildings, wonders, etc. this city has constructed
	FAutoVariable<int, CvCity> m_iDemandResourceCounter;
	FAutoVariable<int, CvCity> m_iResistanceTurns;
	FAutoVariable<int, CvCity> m_iRazingTurns;
	FAutoVariable<int, CvCity> m_iCountExtraLuxuries;
	FAutoVariable<int, CvCity> m_iCheapestPlotInfluence;
	int m_iEspionageModifier;
#if defined(MOD_RELIGION_CONVERSION_MODIFIERS)
	int m_iConversionModifier;
#endif
#if defined(MOD_VOLCANO_BREAK)
	int m_iImmueVolcanoDamage;
#endif
	int m_iAddsFreshWater;
	FAutoVariable<int, CvCity> m_iExtraAttackOnKill;
	int m_iForbiddenForeignSpyCount;
	int m_iAllowSpaceshipLaunchCount;
#if defined(MOD_ROG_CORE)
	int m_iExtraDamageHealPercent;
	FAutoVariable<int, CvCity> m_iExtraDamageHeal;
	FAutoVariable<int, CvCity> m_iBombardRange;
	int m_iBombardIndirect;
	FAutoVariable<int, CvCity> m_iCityBuildingRangeStrikeModifier;

	FAutoVariable<int, CvCity> m_iResetDamageValue;
	FAutoVariable<int, CvCity> m_iReduceDamageValue;


	FAutoVariable<int, CvCity> m_iWaterTileDamage;
	FAutoVariable<int, CvCity> m_iWaterTileMovementReduce;

	FAutoVariable<int, CvCity> m_iWaterTileTurnDamage;
	FAutoVariable<int, CvCity> m_iLandTileDamage;
	FAutoVariable<int, CvCity> m_iLandTileMovementReduce;
	FAutoVariable<int, CvCity> m_iLandTileTurnDamage;
#endif

	int m_iNukeInterceptionChance;
	FAutoVariable<int, CvCity> m_iNumAttacks;
	FAutoVariable<int, CvCity> m_iAttacksMade;


	OperationSlot m_unitBeingBuiltForOperation;

	FAutoVariable<bool, CvCity> m_bNeverLost;
	FAutoVariable<bool, CvCity> m_bDrafted;
	FAutoVariable<bool, CvCity> m_bAirliftTargeted;   // unused
	FAutoVariable<bool, CvCity> m_bProductionAutomated;
	FAutoVariable<bool, CvCity> m_bLayoutDirty;
	FAutoVariable<bool, CvCity> m_bMadeAttack;
	FAutoVariable<bool, CvCity> m_bOccupied;
	FAutoVariable<bool, CvCity> m_bPuppet;
	bool m_bIgnoreCityForHappiness;

	FAutoVariable<bool, CvCity> m_bEverCapital;
	FAutoVariable<bool, CvCity> m_bIndustrialRouteToCapital;
	FAutoVariable<bool, CvCity> m_bFeatureSurrounded;

	FAutoVariable<PlayerTypes, CvCity> m_ePreviousOwner;
	FAutoVariable<PlayerTypes, CvCity> m_eOriginalOwner;
	FAutoVariable<PlayerTypes, CvCity> m_ePlayersReligion;

	FAutoVariable<std::vector<int>, CvCity> m_aiSeaPlotYield;
	FAutoVariable<std::vector<int>, CvCity> m_aiRiverPlotYield;
	FAutoVariable<std::vector<int>, CvCity> m_aiLakePlotYield;
	FAutoVariable<std::vector<int>, CvCity> m_aiSeaResourceYield;
	FAutoVariable<std::vector<int>, CvCity> m_aiBaseYieldRateFromTerrain;
	FAutoVariable<std::vector<int>, CvCity> m_aiBaseYieldRateFromBuildings;
	FAutoVariable<std::vector<int>, CvCity> m_aiBaseYieldRateFromBuildingsPolicies;
	FAutoVariable<std::vector<int>, CvCity> m_aiBaseYieldRateFromSpecialists;
	FAutoVariable<std::vector<int>, CvCity> m_aiBaseYieldRateFromProjects;
	FAutoVariable<std::vector<int>, CvCity> m_aiBaseYieldRateFromMisc;
	std::vector<int> m_aiBaseYieldRateFromReligion;
	FAutoVariable<std::vector<int>, CvCity> m_aiYieldRateModifier;
	FAutoVariable<std::vector<int>, CvCity> m_aiYieldRateMultiplier;
	FAutoVariable<std::vector<int>, CvCity> m_aiYieldPerPop;
	FAutoVariable<std::vector<int>, CvCity> m_aiYieldPerPopInEmpire;

	std::vector<int> m_aiNumProjects;


#if defined(MOD_ROG_CORE)
	std::vector<int> m_aiNumTimesAttackedThisTurn;
	std::vector<int> m_aiBaseYieldRateFromCSAlliance;
	std::vector<int> m_aiBaseYieldRateFromCSFriendship;
	std::vector<int> m_aiYieldPerAlly;
	std::vector<int> m_aiYieldPerFriend;
	std::vector<int> m_aiYieldPerEspionageSpy;
	std::vector<int> m_aiBaseYieldRateFromEspionageSpy;
	std::vector<int> m_aiYieldFromConstruction;
	std::vector<int> m_aiYieldFromUnitProduction;
	std::vector<int> m_aiYieldFromBirth;
	std::vector<int> m_aiYieldFromBorderGrowth;

	std::vector<int> m_aiYieldFromPillage;

	std::vector<int> m_aiResourceQuantityFromPOP;
	std::vector<int> m_aiSpecialistRateModifier;
#endif

	std::vector<int> m_aiYieldFromInternalTR;
	FAutoVariable<std::vector<int>, CvCity> m_aiYieldFromProcessModifier;


	std::vector<int> m_aiYieldPerReligion;
	std::vector<int> m_aiYieldPerEra;
	std::vector<int> m_aiYieldModifierPerEra;
	std::vector<int> m_aiCityStateTradeRouteYieldModifier;
	FAutoVariable<std::vector<int>, CvCity> m_aiPowerYieldRateModifier;
	FAutoVariable<std::vector<int>, CvCity> m_aiFeatureYieldRateModifier;
	FAutoVariable<std::vector<int>, CvCity> m_aiTerrainYieldRateModifier;
	FAutoVariable<std::vector<int>, CvCity> m_aiImprovementYieldRateModifier;
	FAutoVariable<std::vector<int>, CvCity> m_aiSpecialistYieldRateModifier;
	FAutoVariable<std::vector<int>, CvCity> m_aiResourceYieldRateModifier;
	FAutoVariable<std::vector<int>, CvCity> m_aiExtraSpecialistYield;
	FAutoVariable<std::vector<int>, CvCity> m_aiProductionToYieldModifier;
	FAutoVariable<std::vector<int>, CvCity> m_aiDomainFreeExperience;
	FAutoVariable<std::vector<int>, CvCity> m_aiDomainProductionModifier;
	FAutoVariable<std::vector<int>, CvCity> m_aiDomainFreeExperiencesPerPop;
	FAutoVariable<std::vector<int>, CvCity> m_aiDomainFreeExperiencesPerPopGlobal;
	FAutoVariable<std::vector<int>, CvCity> m_aiDomainFreeExperiencesPerTurn;
	FAutoVariable<std::vector<int>, CvCity> m_aiDomainEnemyCombatModifier;
	FAutoVariable<std::vector<int>, CvCity> m_aiDomainFriendsCombatModifierLocal;

	FAutoVariable<std::vector<bool>, CvCity> m_abEverOwned;
	FAutoVariable<std::vector<bool>, CvCity> m_abRevealed;

	FAutoVariable<CvString, CvCity> m_strScriptData;

	FAutoVariable<std::vector<int>, CvCity> m_paiNoResource;
	FAutoVariable<std::vector<int>, CvCity> m_paiFreeResource;
	FAutoVariable<std::vector<int>, CvCity> m_paiNumResourcesLocal;
	FAutoVariable<std::vector<int>, CvCity> m_paiProjectProduction;
	FAutoVariable<std::vector<int>, CvCity> m_paiSpecialistProduction;
	FAutoVariable<std::vector<int>, CvCity> m_paiUnitProduction;
	FAutoVariable<std::vector<int>, CvCity> m_paiUnitProductionTime;
	FAutoVariable<std::vector<int>, CvCity> m_paiSpecialistCount;
	FAutoVariable<std::vector<int>, CvCity> m_paiMaxSpecialistCount;
	FAutoVariable<std::vector<int>, CvCity> m_paiForceSpecialistCount;
	FAutoVariable<std::vector<int>, CvCity> m_paiFreeSpecialistCount;
	FAutoVariable<std::vector<int>, CvCity> m_paiImprovementFreeSpecialists;
	FAutoVariable<std::vector<int>, CvCity> m_paiUnitCombatFreeExperience;
	FAutoVariable<std::vector<int>, CvCity> m_paiUnitCombatProductionModifier;
	FAutoVariable<std::vector<int>, CvCity> m_paiFreePromotionCount;
	FAutoVariable<std::vector<int>, CvCity> m_viTradeRouteDomainRangeModifier;
	FAutoVariable<std::vector<int>, CvCity> m_viTradeRouteDomainGoldBonus;

	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiYieldModifierFromFeature;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiYieldModifierFromTerrain;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiYieldModifierFromImprovement;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiYieldModifierFromSpecialist;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiYieldModifierFromResource;

	int m_iBaseHappinessFromBuildings;
	int m_iUnmoddedHappinessFromBuildings;

	bool m_bRouteToCapitalConnectedLastTurn;
	bool m_bRouteToCapitalConnectedThisTurn;
	CvString m_strName;

	bool m_bOwedCultureBuilding;

#if defined(MOD_BUGFIX_FREE_FOOD_BUILDING)
	bool m_bOwedFoodBuilding;
#endif

	int m_iNumNearbyMountains;

	mutable FFastSmallFixedList< OrderData, 25, true, c_eCiv5GameplayDLL > m_orderQueue;

	int** m_aaiBuildingSpecialistUpgradeProgresses;

#if defined(MOD_NUCLEAR_WINTER_FOR_SP)
	int m_iNumNoNuclearWinterLocal = 0;
#endif

#if defined(MOD_TROOPS_AND_CROPS_FOR_SP)
	int m_iCityEnableCrops = 0;
	int m_iCityEnableArmee = 0;
#endif

#if defined(MOD_INTERNATIONAL_IMMIGRATION_FOR_SP)
	bool m_bCanDoImmigration = true;
	int m_iNumAllScaleImmigrantIn = 0;
#endif
#ifdef MOD_GLOBAL_CITY_SCALES
	CityScaleTypes m_eCityScale = NO_CITY_SCALE;
#endif

#ifdef MOD_GLOBAL_CORRUPTION
	int m_iCachedCorruptionScore = 0;
	CorruptionLevelTypes m_eCachedCorruptionLevel = INVALID_CORRUPTION;

	int m_iCorruptionScoreChangeFromBuilding = 0;
	int m_iCorruptionLevelChangeFromBuilding = 0;
#endif

	bool m_bIsSecondCapital = false;

#ifdef MOD_PROMOTION_CITY_DESTROYER
	int m_iSiegeKillCitizensModifier = 0;
#endif

	std::vector<int> m_paiNumTerrainWorked;
	std::vector<int> m_paiNumFeaturelessTerrainWorked;
	std::vector<int> m_paiNumFeatureWorked;

	std::vector<int> m_paiNumImprovementWorked;

	vector<SCityExtraYields> m_yieldChanges; //[NUM_YIELD_TYPES]

#if defined(MOD_BUILDING_IMPROVEMENT_RESOURCES)
	std::map<std::pair<int, int>, short> m_ppiResourceFromImprovement;
#endif
	int m_iAdditionalFood;
	int m_iBaseTourism;
	int m_iBaseTourismBeforeModifiers;

#if defined(MOD_API_UNIFIED_YIELDS_MORE)
	bool m_bIsColony;
	int m_iOrganizedCrime;
	int m_iResistanceCounter;
	int m_iPlagueCounter;
	int m_iPlagueTurns;
	int m_iPlagueType;
	int m_iLoyaltyCounter;
	int m_iDisloyaltyCounter;
	int m_iLoyaltyStateType;
	std::vector<int> m_aiYieldModifierFromHealth;
	std::vector<int> m_aiYieldModifierFromCrime;
	std::vector<int> m_aiYieldFromHappiness;
	std::vector<int> m_aiYieldFromHealth;
	std::vector<int> m_aiYieldFromCrime;
	std::vector<int> m_aiStaticCityYield;
#endif

	std::tr1::array<int, YieldTypes::NUM_YIELD_TYPES> m_aTradeRouteFromTheCityYields;
	std::tr1::array<int, YieldTypes::NUM_YIELD_TYPES> m_aTradeRouteFromTheCityYieldsPerEra;

	CvCityBuildings* m_pCityBuildings;
	CvCityStrategyAI* m_pCityStrategyAI;
	CvCityCitizens* m_pCityCitizens;
	CvCityReligions* m_pCityReligions;
	CvCityEmphases* m_pEmphases;
	CvCityEspionage* m_pCityEspionage;
	CvCityCulture* m_pCityCulture;

	mutable int m_bombardCheckTurn;

	// CACHE: cache frequently used values
	mutable int	m_iPopulationRank;
	mutable bool m_bPopulationRankValid;
	FAutoVariable<std::vector<int>, CvCity> m_aiBaseYieldRank;
	FAutoVariable<std::vector<bool>, CvCity> m_abBaseYieldRankValid;
	FAutoVariable<std::vector<int>, CvCity> m_aiYieldRank;
	FAutoVariable<std::vector<bool>, CvCity> m_abYieldRankValid;

	FAutoVariable<std::vector<int>, CvCity> m_paiHurryModifier;

	IDInfo m_combatUnit;		// The unit the city is in combat with

	void doGrowth();
	void doProduction(bool bAllowNoProduction);
	void doProcess();
	void doDecay();
	void doGreatPeople();
	bool doCheckProduction();

	int getExtraProductionDifference(int iExtra, UnitTypes eUnit) const;
	int getExtraProductionDifference(int iExtra, BuildingTypes eBuilding) const;
	int getExtraProductionDifference(int iExtra, ProjectTypes eProject) const;
	int getExtraProductionDifference(int iExtra, int iModifier) const;
	int getHurryCostModifier(HurryTypes eHurry, UnitTypes eUnit, bool bIgnoreNew) const;
	int getHurryCostModifier(HurryTypes eHurry, BuildingTypes eBuilding, bool bIgnoreNew) const;
	int getHurryCostModifier(HurryTypes eHurry, int iBaseModifier, int iProduction, bool bIgnoreNew) const;
	int getHurryCost(HurryTypes eHurry, bool bExtra, UnitTypes eUnit, bool bIgnoreNew) const;
	int getHurryCost(HurryTypes eHurry, bool bExtra, BuildingTypes eBuilding, bool bIgnoreNew) const;
	int getHurryCost(bool bExtra, int iProductionLeft, int iHurryModifier, int iModifier) const;
	int getHurryPopulation(HurryTypes eHurry, int iHurryCost) const;
	int getHurryGold(HurryTypes eHurry, int iHurryCost, int iFullCost) const;
	bool canHurryUnit(HurryTypes eHurry, UnitTypes eUnit, bool bIgnoreNew) const;
	bool canHurryBuilding(HurryTypes eHurry, BuildingTypes eBuilding, bool bIgnoreNew) const;

#ifdef MOD_BUILDINGS_YIELD_FROM_OTHER_YIELD
	std::vector<Firaxis::Array<int, YieldFromYieldStruct::STRUCT_LENGTH>> m_ppiYieldFromOtherYield;
	bool m_bHasYieldFromOtherYield;
#endif
	int m_iLastTurnWorkerDisbanded = 0;
	int m_iDefendedAgainstSpreadUntilTurn = 0;

	std::vector<int> m_paiNumBuildingClasses;
};

namespace FSerialization
{
void SyncCities();
void ClearCityDeltas();
}

#endif


FDataStream& operator>>(FDataStream& loadFrom, SCityExtraYields& writeTo);
FDataStream& operator<<(FDataStream& saveTo, const SCityExtraYields& readFrom);

