/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#pragma once

// player.h

#ifndef CIV5_PLAYER_H
#define CIV5_PLAYER_H

#define SAFE_ESTIMATE_NUM_IMPROVEMENTS 50
#define SAFE_ESTIMATE_NUM_CITIES       64
#define MAX_INCOMING_UNITS	20

#include "CvCityAI.h"
#include "CvUnit.h"
#include "CvArmyAI.h"
#include "LinkedList.h"
#include "FFastVector.h"
#include "CvPreGame.h"
#include "CvAchievementUnlocker.h"
#include "CvUnitCycler.h"


class CvPlayerPolicies;
class CvEconomicAI;
class CvMilitaryAI;
class CvCitySpecializationAI;
class CvWonderProductionAI;
class CvGrandStrategyAI;
class CvDiplomacyAI;
class CvPlayerReligions;
class CvReligionAI;
class CvPlayerTechs;
class CvFlavorManager;
class CvTacticalAI;
class CvHomelandAI;
class CvMinorCivAI;
class CvDealAI;
class CvBuilderTaskingAI;
class CvDangerPlots;
class CvCityConnections;
class CvNotifications;
#if defined(MOD_API_PLAYER_LOGS)
class CvEventLog;
#endif
class CvTreasury;
class CvPlayerTraits;
class CvGameInitialItemsOverrides;
class CvDiplomacyRequests;
class CvPlayerEspionage;
class CvEspionageAI;
class CvPlayerTrade;
class CvTradeAI;
class CvLeagueAI;
class CvPlayerCulture;

typedef std::list<CvPopupInfo*> CvPopupQueue;

typedef std::vector< std::pair<UnitCombatTypes, PromotionTypes> > UnitCombatPromotionArray;
typedef std::vector< std::pair<UnitClassTypes, PromotionTypes> > UnitClassPromotionArray;
typedef std::vector< std::pair<CivilizationTypes, LeaderHeadTypes> > CivLeaderArray;
typedef FStaticVector<int, 180*92, true, c_eCiv5GameplayDLL, 0> CvPlotsVector; // allocate the size of HUGE Terra world just in case (this is max that we ship with)

class CvPlayer : public CvGameObjectExtractable
{
	friend class CvPlayerPolicies;

public:
	typedef std::map<unsigned int, int> TurnData;


	CvPlayer();
	virtual ~CvPlayer();

	void ExtractToArg(BasicArguments* arg);
	static void PushToLua(lua_State* L, BasicArguments* arg);
	static void RegistInstanceFunctions();
	static void RegistStaticFunctions();
	static CvPlayerAI* Provide(PlayerTypes player);

	void init(PlayerTypes eID);
	void setupGraphical();
	void reset(PlayerTypes eID = NO_PLAYER, bool bConstructorCall = false);
	void gameStartInit();
	void uninit();

	void initFreeState(CvGameInitialItemsOverrides& kOverrides);
	void initFreeUnits(CvGameInitialItemsOverrides& kOverrides);
	void addFreeUnitAI(UnitAITypes eUnitAI, int iCount);
	CvPlot* addFreeUnit(UnitTypes eUnit, UnitAITypes eUnitAI = NO_UNITAI);

	CvCity* initCity(int iX, int iY, bool bBumpUnits = true, bool bInitialFounding = true, ReligionTypes eInitialReligion = NO_RELIGION, const char* szName = NULL);
	CvCity* acquireCity(CvCity* pCity, bool bConquest, bool bGift, bool bKeepResources = false, bool bIsMajorCivBuyout = false, bool bNoKillPunishment = false);
	void killCities();
	CvString getNewCityName() const;
	CvString GetBorrowedCityName(CivilizationTypes eCivToBorrowFrom) const;
	void getCivilizationCityName(CvString& szBuffer, CivilizationTypes eCivilization) const;
	bool isCityNameValid(CvString& szName, bool bTestDestroyed = true) const;

#if defined(MOD_GLOBAL_CITY_WORKING)
	int getBuyPlotDistance() const;
	int getWorkPlotDistance() const;
	int GetNumWorkablePlots() const;
#endif


	

	int GetMinorFriendCount(const bool bExcludeNonAlive) const;
	int GetMinorAllyCount(const bool bExcludeNonAlive) const;

	void DoLiberatePlayer(PlayerTypes ePlayer, int iOldCityID);
	bool CanLiberatePlayer(PlayerTypes ePlayer);
	bool CanLiberatePlayerCity(PlayerTypes ePlayer);

	CvUnit* initUnit(UnitTypes eUnit, int iX, int iY, UnitAITypes eUnitAI = NO_UNITAI, DirectionTypes eFacingDirection = NO_DIRECTION, bool bNoMove=false, bool bSetupGraphical=true, int iMapLayer = 0, int iNumGoodyHutsPopped = 0);
	CvUnit* initUnitWithNameOffset(UnitTypes eUnit, int nameOffset, int iX, int iY, UnitAITypes eUnitAI = NO_UNITAI, DirectionTypes eFacingDirection = NO_DIRECTION, bool bNoMove = false, bool bSetupGraphical = true, int iMapLayer = 0, int iNumGoodyHutsPopped = 0);

	void disbandUnit(bool bAnnounce);
	void killUnits();

#if defined(MOD_API_EXTENSIONS) || defined(MOD_BUGFIX_UNITCLASS_NOT_UNIT)
	UnitTypes GetSpecificUnitType(const char* szUnitClass, bool hideAssert = false);
#endif
#if defined(MOD_API_EXTENSIONS) || defined(MOD_BUGFIX_BUILDINGCLASS_NOT_BUILDING)
	BuildingTypes GetSpecificBuildingType(const char* szBuildingClass, bool hideAssert = false);
#endif

	CvPlot *GetGreatAdmiralSpawnPlot (CvUnit *pUnit);

	int GetNumBuilders() const;
	void SetNumBuilders(int iNum);
	void ChangeNumBuilders(int iChange);
	int GetMaxNumBuilders() const;
	void SetMaxNumBuilders(int iNum);
	void ChangeMaxNumBuilders(int iChange);

	int GetNumUnitsWithUnitAI(UnitAITypes eUnitAIType, bool bIncludeBeingTrained = false, bool bIncludeWater = true);
	int GetNumUnitsWithDomain(DomainTypes eDomain, bool bMilitaryOnly);
	int GetNumUnitsWithUnitCombat(UnitCombatTypes eDomain);

	void InitDangerPlots();
	void UpdateDangerPlots();
	void SetDangerPlotsDirty();

	bool isHuman() const;
	bool isObserver() const;
	bool isBarbarian() const;
	void doBarbarianRansom(int iOption, int iUnitID);

	const char* getName() const;
	const char* getNameKey() const;
	const char* const getNickName() const;
	const char* getCivilizationDescription() const;
	const char* getCivilizationDescriptionKey() const;
	const char* getCivilizationShortDescription() const;
	const char* getCivilizationShortDescriptionKey() const;
	const char* getCivilizationAdjective() const;
	const char* getCivilizationAdjectiveKey() const;
	const char* getCivilizationTypeKey() const;
	const char* getLeaderTypeKey() const;

	bool isWhiteFlag() const;
	const char* GetStateReligionName() const;
	CvString GetStateReligionKey() const;
	void SetStateReligionKey(const char* strKey);
	const CvString getWorstEnemyName() const;
	ArtStyleTypes getArtStyleType() const;

	void doTurn();
	void doTurnPostDiplomacy();
	void doTurnUnits();
	void SetAllUnitsUnprocessed();
	void DoUnitReset();
	void DoUnitAttrition();
	void RespositionInvalidUnits();

	void updateYield();
	void updateExtraSpecialistYield();
	void updateCityPlotYield();
	void updateCitySight(bool bIncrement);
	void UpdateNotifications();
	void UpdateReligion();

	void updateTimers();

	bool hasPromotableUnit() const;

	bool hasReadyUnit() const;
	int GetCountReadyUnits() const;
	const CvUnit* GetFirstReadyUnit() const;
	void EndTurnsForReadyUnits();
	bool hasAutoUnit() const;
	bool hasBusyUnit() const;
	const UnitHandle getBusyUnit() const;
	bool hasBusyCity() const;
	bool hasBusyUnitOrCity() const;
	const CvCity* getBusyCity() const;
	void chooseTech(int iDiscover = 0, const char* strTxt=0, TechTypes iTechJustDiscovered=NO_TECH);

	// Civ 5 Score
	int GetScore(bool bFinal = false, bool bVictory = false) const;

	int GetScoreFromCities() const;
	int GetScoreFromPopulation() const;
	int GetScoreFromLand() const;
	int GetScoreFromWonders() const;
	int GetScoreFromPolicies() const;
	int GetScoreFromGreatWorks() const;
	int GetScoreFromReligion() const;
	int GetScoreFromTechs() const;
	int GetScoreFromFutureTech() const;
	void ChangeScoreFromFutureTech(int iChange);
	int GetScoreFromScenario1() const;
	void ChangeScoreFromScenario1(int iChange);
	int GetScoreFromScenario2() const;
	void ChangeScoreFromScenario2(int iChange);
	int GetScoreFromScenario3() const;
	void ChangeScoreFromScenario3(int iChange);
	int GetScoreFromScenario4() const;
	void ChangeScoreFromScenario4(int iChange);
	// End Civ 5 Score

	int countCityFeatures(FeatureTypes eFeature) const;
	int countNumBuildings(BuildingTypes eBuilding, bool bReset = false) const;

	void setNumBuildings(BuildingTypes eBuilding, int iValue);
	int getNumBuildings(BuildingTypes eBuilding) const;
	//int countNumCitiesConnectedToCapital() const;

	int countCitiesFeatureSurrounded() const;

	bool IsCityConnectedToCity(CvCity* pCity1, CvCity* pCity2, RouteTypes eRestrictRouteType = NO_ROUTE, bool bIgnoreHarbors = false);
	bool IsCapitalConnectedToPlayer(PlayerTypes ePlayer, RouteTypes eRestrictRouteType = NO_ROUTE);
	bool IsCapitalConnectedToCity(CvCity* pCity, RouteTypes eRestrictRouteType = NO_ROUTE);
#if defined(MOD_API_EXTENSIONS)
	bool IsPlotConnectedToPlot(CvPlot* pFromPlot, CvPlot* pToPlot, RouteTypes eRestrictRoute = NO_ROUTE, bool bIgnoreHarbors = false);
#endif

	void findNewCapital();

	bool canRaze(CvCity* pCity, bool bIgnoreCapitals = false) const;
	void raze(CvCity* pCity);
	void unraze(CvCity* pCity);
	void disband(CvCity* pCity);

	bool canReceiveGoody(CvPlot* pPlot, GoodyTypes eGoody, CvUnit* pUnit) const;
	void receiveGoody(CvPlot* pPlot, GoodyTypes eGoody, CvUnit* pUnit);
	void doGoody(CvPlot* pPlot, CvUnit* pUnit);

	CvCity* findClosestCity(CvPlot& pPlot);

	void AwardFreeBuildings(CvCity* pCity); // slewis - broken out so that Venice can get free buildings when they purchase something
	bool canFound(int iX, int iY, bool bTestVisible = false) const;

#if defined(MOD_GLOBAL_RELIGIOUS_SETTLERS)
	void found(int iX, int iY, ReligionTypes eReligion = NO_RELIGION);
#else
	void found(int iX, int iY);
#endif

	bool canTrain(UnitTypes eUnit, bool bContinue = false, bool bTestVisible = false, bool bIgnoreCost = false, bool bIgnoreUniqueUnitStatus = false, CvString* toolTipSink = NULL) const;
	bool canConstruct(BuildingTypes eBuilding, bool bContinue = false, bool bTestVisible = false, bool bIgnoreCost = false, CvString* toolTipSink = NULL) const;
	bool canCreate(ProjectTypes eProject, bool bContinue = false, bool bTestVisible = false) const;
	bool canPrepare(SpecialistTypes eSpecialist, bool bContinue = false) const;
	bool canMaintain(ProcessTypes eProcess, bool bContinue = false) const;
	bool IsCanPurchaseAnyCity(bool bTestPurchaseCost, bool bTestTrainable, UnitTypes eUnit, BuildingTypes eBuilding, YieldTypes ePurchaseYield);
	bool isProductionMaxedUnitClass(UnitClassTypes eUnitClass) const;
	bool isProductionMaxedBuildingClass(BuildingClassTypes eBuildingClass, bool bAcquireCity = false) const;
	bool isProductionMaxedProject(ProjectTypes eProject) const;
	int getProductionNeeded(UnitTypes eUnit) const;
	int getProductionNeeded(BuildingTypes eBuilding) const;
	int getProductionNeeded(ProjectTypes eProject) const;
	int getProductionNeeded(SpecialistTypes eSpecialist) const;

#if defined(MOD_PROCESS_STOCKPILE)
	int getMaxStockpile() const;
#endif

	int getProductionModifier(CvString* toolTipSink = NULL) const;
	int getProductionModifier(UnitTypes eUnit, CvString* toolTipSink = NULL) const;
	int getProductionModifier(BuildingTypes eBuilding, CvString* toolTipSink = NULL) const;
	int getProductionModifier(ProjectTypes eProject, CvString* toolTipSink = NULL) const;
	int getProductionModifier(SpecialistTypes eSpecialist, CvString* toolTipSink = NULL) const;
	int getProductionModifier(ProcessTypes eProcess, CvString* toolTipSink = NULL) const;

#if defined(MOD_ROG_CORE)
	int GetWorldWonderYieldChange(int iYield);

	int GetBuildingClassYieldModifier(BuildingClassTypes eBuildingClass, YieldTypes eYieldType);
	int GetBuildingClassYieldModifier(BuildingClassTypes eBuildingClass, YieldTypes eYieldType, const vector<int>& preexistingBuildingsCount);
#endif

	int getBuildingClassPrereqBuilding(BuildingTypes eBuilding, BuildingClassTypes ePrereqBuildingClass, int iExtra = 0) const;
	void removeBuildingClass(BuildingClassTypes eBuildingClass);
	void processBuilding(BuildingTypes eBuilding, int iChange, bool bFirst, CvArea* pArea);
	int GetBuildingClassYieldChange(BuildingClassTypes eBuildingClass, YieldTypes eYieldType);

	bool canBuild(const CvPlot* pPlot, BuildTypes eBuild, bool bTestEra = false, bool bTestVisible = false, bool bTestGold = true, bool bTestPlotOwner = true) const;
	bool IsBuildBlockedByFeature(BuildTypes eBuild, FeatureTypes eFeature) const;
	int getBuildCost(const CvPlot* pPlot, BuildTypes eBuild) const;
	RouteTypes getBestRoute(CvPlot* pPlot = NULL) const;
	int getImprovementUpgradeRate() const;

	int GetAllFeatureProduction() const;
	void ChangeAllFeatureProduction(int iChange);

	int calculateTotalYield(YieldTypes eYield) const;

	int GetUnitProductionMaintenanceMod() const;
	void UpdateUnitProductionMaintenanceMod();
	int calculateUnitProductionMaintenanceMod() const;

	int GetNumUnitsSupplied() const;
	int GetNumUnitsSuppliedByHandicap() const;
	int GetNumUnitsSuppliedByCities() const;
	int GetNumUnitsSuppliedByPopulation() const;

	int GetNumUnitsOutOfSupply() const;

	int calculateUnitCost() const;
	int calculateUnitSupply() const;
	int calculateResearchModifier(TechTypes eTech);
	int calculateGoldRate() const;
	int calculateGoldRateTimes100() const;

	int unitsRequiredForGoldenAge() const;
	int unitsGoldenAgeCapable() const;
	int unitsGoldenAgeReady() const;

	int greatGeneralThreshold() const;
	int greatAdmiralThreshold() const;

	int specialistYield(SpecialistTypes eSpecialist, YieldTypes eYield) const;

	int GetCityYieldChangeTimes100(YieldTypes eYield) const;
	void ChangeCityYieldChangeTimes100(YieldTypes eYield, int iChange);

	int GetCoastalCityYieldChange(YieldTypes eYield) const;
	void ChangeCoastalCityYieldChange(YieldTypes eYield, int iChange);

	int GetCapitalYieldChangeTimes100(YieldTypes eYield) const;
	void ChangeCapitalYieldChangeTimes100(YieldTypes eYield, int iChange);

	int GetCapitalYieldPerPopChange(YieldTypes eYield) const;
	void ChangeCapitalYieldPerPopChange(YieldTypes eYield, int iChange);

	int GetYieldPerPopChange(YieldTypes eYield) const;
	void ChangeYieldPerPopChange(YieldTypes eYield, int iChange);

	int GetGreatWorkYieldChange(YieldTypes eYield) const;
	void ChangeGreatWorkYieldChange(YieldTypes eYield, int iChange);

	CvPlot* getStartingPlot() const;
	void setStartingPlot(CvPlot* pNewValue);

	int getTotalPopulation() const;
	int getAveragePopulation() const;
	void changeTotalPopulation(int iChange);
	long getRealPopulation() const;

	int GetNewCityExtraPopulation() const;
	void ChangeNewCityExtraPopulation(int iChange);

	int GetFreeFoodBox() const;
	void ChangeFreeFoodBox(int iChange);

	int getTotalLand() const;
	void changeTotalLand(int iChange);

	int getTotalLandScored() const;
	void changeTotalLandScored(int iChange);

	int GetHappinessFromTradeRoutes() const;
	void DoUpdateCityConnectionHappiness();

	// Culture

	int GetTotalJONSCulturePerTurn() const;

	int GetJONSCulturePerTurnFromCities() const;

	int GetJONSCulturePerTurnFromExcessHappiness() const;
	int GetJONSCulturePerTurnFromTraits() const;

	int GetJONSCulturePerTurnForFree() const;
	void ChangeJONSCulturePerTurnForFree(int iChange);

	int GetJONSCulturePerTurnFromMinorCivs() const; // DEPRECATED, use GetCulturePerTurnFromMinorCivs() instead
	void ChangeJONSCulturePerTurnFromMinorCivs(int iChange); // DEPRECATED, does nothing
	int GetCulturePerTurnFromMinorCivs() const;
	int GetCulturePerTurnFromMinor(PlayerTypes eMinor) const;

	int GetCulturePerTurnFromReligion() const;

	int GetCulturePerTurnFromBonusTurns() const;

	int GetJONSCultureCityModifier() const;
	void ChangeJONSCultureCityModifier(int iChange);

	int getJONSCulture() const;
	void setJONSCulture(int iNewValue);
	void changeJONSCulture(int iChange);

	int GetJONSCultureEverGenerated() const;
	void SetJONSCultureEverGenerated(int iNewValue);
	void ChangeJONSCultureEverGenerated(int iChange);

	int GetJONSCulturePerCityPerTurn() const;

	int GetCulturePerWonder() const;
	void ChangeCulturePerWonder(int iChange);

	int GetCultureWonderMultiplier() const;
	void ChangeCultureWonderMultiplier(int iChange);

	int GetCulturePerTechResearched() const;
	void ChangeCulturePerTechResearched(int iChange);

	int GetSpecialistCultureChange() const;
	void ChangeSpecialistCultureChange(int iChange);

	int GetCultureYieldFromPreviousTurns(int iGameTurn, int iNumPreviousTurnsToCount);

	int GetNumCitiesFreeCultureBuilding() const;
	void ChangeNumCitiesFreeCultureBuilding(int iChange);
	int GetNumCitiesFreeFoodBuilding() const;
	void ChangeNumCitiesFreeFoodBuilding(int iChange);

	void DoCombatStrengthChangeFromKill(CvUnit* pAttackingUnit, CvUnit* pDefendingUnit, int iX, int iY, int iExistingDelay);

#if defined(MOD_API_UNIFIED_YIELDS)
	void DoYieldsFromKill(CvUnit* pAttackingUnit, CvUnit* pDefendingUnit, int iX, int iY, int iExistingDelay);
#if defined(MOD_API_EXTENSIONS)
	void DoYieldBonusFromKill(YieldTypes eYield, CvUnit* pAttackingUnit, CvUnit* pKilledUnit, UnitTypes eKilledUnitType, int iX, int iY, bool bWasBarbarian, int &iNumBonuses);
#else
	void DoYieldBonusFromKill(YieldTypes eYield, CvUnit* pAttackingUnit, UnitTypes eKilledUnitType, int iX, int iY, bool bWasBarbarian, int &iNumBonuses);
#endif
#else
	void DoYieldsFromKill(UnitTypes eAttackingUnitType, UnitTypes eKilledUnitType, int iX, int iY, bool bWasBarbarian, int iExistingDelay);
#if defined(MOD_API_EXTENSIONS)
	void DoYieldBonusFromKill(YieldTypes eYield, UnitTypes eAttackingUnitType, CvUnit* pKilledUnit, UnitTypes eKilledUnitType, int iX, int iY, bool bWasBarbarian, int &iNumBonuses);
#else
	void DoYieldBonusFromKill(YieldTypes eYield, UnitTypes eAttackingUnitType, UnitTypes eKilledUnitType, int iX, int iY, bool bWasBarbarian, int &iNumBonuses);
#endif
#endif
#if defined(MOD_API_EXTENSIONS)
	void DoUnresearchedTechBonusFromKill(CvUnit* pKilledUnit, UnitTypes eKilledUnitType, int iX, int iY, int &iNumBonuses);
#else
	void DoUnresearchedTechBonusFromKill(UnitTypes eKilledUnitType, int iX, int iY, int &iNumBonuses);
#endif
	void ReportYieldFromKill(YieldTypes eYield, int iValue, int iX, int iY, int iDelay);

	void DoTechFromCityConquer(CvCity* pConqueredCity);

#if defined(MOD_API_UNIFIED_YIELDS)
	int GetYieldPerTurnFromReligion(YieldTypes eYield) const;
	int GetYieldPerTurnFromTraits(YieldTypes eYield) const;
#endif


#if defined(MOD_ROG_CORE)
	void DoHealGlobal(int iValue);
#endif

	// Faith
	int GetTotalFaithPerTurn() const;
	int GetFaithPerTurnFromCities() const;
	int GetFaithPerTurnFromMinorCivs() const;
	int GetFaithPerTurnFromMinor(PlayerTypes eMinor) const;
	int GetFaithPerTurnFromReligion() const;
	int GetFaith() const;
	void SetFaith(int iNewValue);
	void ChangeFaith(int iChange);
	int GetFaithEverGenerated() const;
	void SetFaithEverGenerated(int iNewValue);
	void ChangeFaithEverGenerated(int iChange);
	int GetCachedTotalFaithPerTurn() const;
	void SetCachedTotalFaithPerTurn(int iValue);

	void DoUpdateAllCityYields();

	// Happiness

	void DoUpdateHappiness();
	int GetHappiness() const;
	void SetHappiness(int iNewValue);

	void DoUpdateTotalHappiness();
	int GetExcessHappiness() const;
	int GetCachedExcessHappiness() const;
	void SetCachedExcessHappiness(int value);
	bool IsEmpireUnhappy() const;
	bool IsEmpireVeryUnhappy() const;
	bool IsEmpireSuperUnhappy() const;

	void DoUpdateUprisings();
	int GetUprisingCounter() const;
	void SetUprisingCounter(int iValue);
	void ChangeUprisingCounter(int iChange);
	void DoResetUprisingCounter(bool bFirstTime);
	void DoUprising();

	void DoUpdateCityRevolts();
	int GetCityRevoltCounter() const;
	void SetCityRevoltCounter(int iValue);
	void ChangeCityRevoltCounter(int iChange);
	void DoResetCityRevoltCounter();
	void DoCityRevolt();
	CvCity *GetMostUnhappyCity();
	PlayerTypes GetMostUnhappyCityRecipient();

	int GetHappinessFromPolicies() const;
	int GetHappinessFromCities() const;
	int GetHappinessFromBuildings() const;

	int GetExtraHappinessPerCity() const;
	void ChangeExtraHappinessPerCity(int iChange);
	int GetExtraHappinessPerXPolicies() const;
	void ChangeExtraHappinessPerXPolicies(int iChange);

	int GetHappinessFromResources() const;
	int GetHappinessFromResourceVariety() const;
	int GetHappinessFromReligion();
	int GetHappinessFromNaturalWonders() const;

	void SetNaturalWonderOwned(FeatureTypes eFeature, bool bValue);

	int GetExtraHappinessPerLuxury() const;
	void ChangeExtraHappinessPerLuxury(int iChange);

	int GetHappinessFromLuxury(ResourceTypes eResource) const;
	int GetAdequateLuxuryKindCount(int threshold) const;
	int GetStrengthModifierFromAlly() const;

	int GetUnhappiness() const;
	void SetUnhappiness(int iNewValue);
	int DoUpdateTotalUnhappiness(CvCity* pAssumeCityAnnexed = NULL, CvCity* pAssumeCityPuppeted = NULL);
	int GetUnhappinessFromCityForUI(CvCity* pCity) const;

	int GetUnhappinessFromCityCount(CvCity* pAssumeCityAnnexed = NULL, CvCity* pAssumeCityPuppeted = NULL) const;
	int GetUnhappinessFromCapturedCityCount(CvCity* pAssumeCityAnnexed = NULL, CvCity* pAssumeCityPuppeted = NULL) const;
	int GetUnhappinessFromCityPopulation(CvCity* pAssumeCityAnnexed = NULL, CvCity* pAssumeCityPuppeted = NULL) const;
	int GetUnhappinessFromCitySpecialists(CvCity* pAssumeCityAnnexed, CvCity* pAssumeCityPuppeted) const;
	int GetUnhappinessFromPuppetCityPopulation() const;
	int GetUnhappinessFromOccupiedCities(CvCity* pAssumeCityAnnexed = NULL, CvCity* pAssumeCityPuppeted = NULL) const;

	int GetUnhappinessFromUnits() const;
	void ChangeUnhappinessFromUnits(int iChange);

	int GetUnhappinessFromUnitsMod() const;
	void ChangeUnhappinessFromUnitsMod(int iChange);

	int GetUnhappinessMod() const;
	void ChangeUnhappinessMod(int iChange);

	int GetCityCountUnhappinessMod() const;
	void ChangeCityCountUnhappinessMod(int iChange);

	int GetOccupiedPopulationUnhappinessMod() const;
	void ChangeOccupiedPopulationUnhappinessMod(int iChange);

	int GetCapitalUnhappinessMod() const;
	void ChangeCapitalUnhappinessMod(int iChange);

	int GetHappinessPerGarrisonedUnit() const;
	void SetHappinessPerGarrisonedUnit(int iValue);
	void ChangeHappinessPerGarrisonedUnit(int iChange);

	int GetHappinessPerTradeRoute() const;
	void SetHappinessPerTradeRoute(int iValue);
	void ChangeHappinessPerTradeRoute(int iChange);

	int GetHappinessPerXPopulation() const;
	void SetHappinessPerXPopulation(int iValue);
	void ChangeHappinessPerXPopulation(int iChange);

	int GetHappinessFromMinorCivs() const;
	int GetHappinessFromMinor(PlayerTypes eMinor) const;

	int GetHappinessFromLeagues() const;
	void SetHappinessFromLeagues(int iValue);
	void ChangeHappinessFromLeagues(int iChange);

	// END Happiness

	// Espionage
	int GetEspionageModifier() const;
	void ChangeEspionageModifier(int iChange);
	int GetEspionageSpeedModifier() const;
	void ChangeEspionageSpeedModifier(int iChange);
	int GetStartingSpyRank() const;
	void ChangeStartingSpyRank(int iChange);
	int GetSpyLevelUpWhenRiggingCount() const;
	// END Espionage

#if defined(MOD_RELIGION_CONVERSION_MODIFIERS)
	int GetConversionModifier() const;
	void ChangeConversionModifier(int iChange);
#endif

	int GetExtraLeagueVotes() const;
	void ChangeExtraLeagueVotes(int iChange);

	int GetWoundedUnitDamageMod() const;
	void SetWoundedUnitDamageMod(int iValue);
	void ChangeWoundedUnitDamageMod(int iChange);

	int GetUnitUpgradeCostMod() const;
	void SetUnitUpgradeCostMod(int iValue);
	void ChangeUnitUpgradeCostMod(int iChange);

	int GetAllowPuppetPurchase() const;
	void ChangeAllowPuppetPurchase(int iChange);
	bool IsAllowPuppetPurchase()const;

	int GetBarbarianCombatBonus() const;
	void SetBarbarianCombatBonus(int iValue);
	void ChangeBarbarianCombatBonus(int iChange);

	bool IsAlwaysSeeBarbCamps() const;
	void SetAlwaysSeeBarbCampsCount(int iValue);
	void ChangeAlwaysSeeBarbCampsCount(int iChange);

#if defined(MOD_API_EXTENSIONS)
	bool grantPolicy(PolicyTypes iPolicy, bool bFree=false);
	bool revokePolicy(PolicyTypes iPolicy);
	bool swapPolicy(PolicyTypes iNewPolicy, PolicyTypes iOldPolicy);
	void setHasPolicy(PolicyTypes eIndex, bool bNewValue, bool bFree=false);
#else
	void setHasPolicy(PolicyTypes eIndex, bool bNewValue);
#endif
	int getNextPolicyCost() const;
	void DoUpdateNextPolicyCost();
	bool canAdoptPolicy(PolicyTypes ePolicy) const;
	void doAdoptPolicy(PolicyTypes ePolicy);
	void DoBuyNewBranch(PolicyBranchTypes eBranch);

	bool IsAnarchy() const;
	int GetAnarchyNumTurns() const;
	void SetAnarchyNumTurns(int iValue);
	void ChangeAnarchyNumTurns(int iChange);

	int getAdvancedStartPoints() const;
	void setAdvancedStartPoints(int iNewValue);
	void changeAdvancedStartPoints(int iChange);

	bool canStealTech(PlayerTypes eTarget, TechTypes eTech) const;
	bool canSpyDestroyUnit(PlayerTypes eTarget, CvUnit& kUnit) const;
	bool canSpyBribeUnit(PlayerTypes eTarget, CvUnit& kUnit) const;
	bool canSpyDestroyBuilding(PlayerTypes eTarget, BuildingTypes eBuilding) const;
	bool canSpyDestroyProject(PlayerTypes eTarget, ProjectTypes eProject) const;

	void doAdvancedStartAction(AdvancedStartActionTypes eAction, int iX, int iY, int iData, bool bAdd);
	int getAdvancedStartUnitCost(UnitTypes eUnit, bool bAdd, CvPlot* pPlot = NULL);
	int getAdvancedStartCityCost(bool bAdd, CvPlot* pPlot = NULL);
	int getAdvancedStartPopCost(bool bAdd, CvCity* pCity = NULL);
	int getAdvancedStartBuildingCost(BuildingTypes eBuilding, bool bAdd, CvCity* pCity = NULL);
	int getAdvancedStartImprovementCost(ImprovementTypes eImprovement, bool bAdd, CvPlot* pPlot = NULL);
	int getAdvancedStartRouteCost(RouteTypes eRoute, bool bAdd, CvPlot* pPlot = NULL);
	int getAdvancedStartTechCost(TechTypes eTech, bool bAdd);
	int getAdvancedStartVisibilityCost(bool bAdd, CvPlot* pPlot = NULL);

	// Temporary Bonuses
	int GetAttackBonusTurns() const;
	void ChangeAttackBonusTurns(int iChange);
	int GetCultureBonusTurns() const;
	void ChangeCultureBonusTurns(int iChange);
	int GetTourismBonusTurns() const;
	void ChangeTourismBonusTurns(int iChange);
	CvString GetInternationalTourismTooltip();

	int GetGoldenAgePointPerTurnFromCitys() const;

#if defined(MOD_API_UNIFIED_YIELDS_MORE)
	void DoChangeGreatGeneralRate();
	void DoChangeGreatAdmiralRate();
#endif

	// Golden Age Stuff

	void DoProcessGoldenAge();

	int GetGoldenAgeProgressThreshold() const;

	int GetGoldenAgeProgressMeter() const;
	void SetGoldenAgeProgressMeter(int iValue);
	void ChangeGoldenAgeProgressMeter(int iChange);

	int GetGoldenAgeMeterMod() const;
	void SetGoldenAgeMeterMod(int iValue);
	void ChangeGoldenAgeMeterMod(int iChange);

	int GetNumGoldenAges() const;
	void SetNumGoldenAges(int iValue);
	void ChangeNumGoldenAges(int iChange);

	int getGoldenAgeTurns() const;
	bool isGoldenAge() const;
	void changeGoldenAgeTurns(int iChange);
	int getGoldenAgeLength() const;

	int getNumUnitGoldenAges() const;
	void changeNumUnitGoldenAges(int iChange);

	int getStrikeTurns() const;
	void changeStrikeTurns(int iChange);

	int getGoldenAgeModifier() const;
	void changeGoldenAgeModifier(int iChange);

#ifdef MOD_BUILDINGS_GOLDEN_AGE_EXTEND
	int CvPlayer::GetGoldenAgeUnitCombatModifier() const;

	void CvPlayer::SetGoldenAgeUnitCombatModifier(int iValue);

	void CvPlayer::ChangeGoldenAgeUnitCombatModifier(int iValue);
#endif

	// Great People Stuff
	void createGreatGeneral(UnitTypes eGreatPersonUnit, int iX, int iY, bool bIsFree);
	void createGreatAdmiral(UnitTypes eGreatPersonUnit, int iX, int iY, bool bIsFree);

	int getGreatPeopleCreated(bool bExcludeFree = MOD_GLOBAL_TRULY_FREE_GP) const;
	void incrementGreatPeopleCreated(bool bIsFree = MOD_GLOBAL_TRULY_FREE_GP);

	int getGreatGeneralsCreated(bool bExcludeFree = MOD_GLOBAL_TRULY_FREE_GP) const;
	void incrementGreatGeneralsCreated(bool bIsFree = MOD_GLOBAL_TRULY_FREE_GP);
	int getGreatAdmiralsCreated(bool bExcludeFree = MOD_GLOBAL_TRULY_FREE_GP) const;
	void incrementGreatAdmiralsCreated(bool bIsFree = MOD_GLOBAL_TRULY_FREE_GP);
#if defined(MOD_GLOBAL_SEPARATE_GP_COUNTERS)
	int getGreatMerchantsCreated(bool bExcludeFree = MOD_GLOBAL_TRULY_FREE_GP) const;
	void incrementGreatMerchantsCreated(bool bIsFree = MOD_GLOBAL_TRULY_FREE_GP);
	int getGreatScientistsCreated(bool bExcludeFree = MOD_GLOBAL_TRULY_FREE_GP) const;
	void incrementGreatScientistsCreated(bool bIsFree = MOD_GLOBAL_TRULY_FREE_GP);
	int getGreatEngineersCreated(bool bExcludeFree = MOD_GLOBAL_TRULY_FREE_GP) const;
	void incrementGreatEngineersCreated(bool bIsFree = MOD_GLOBAL_TRULY_FREE_GP);
#endif
	int getGreatWritersCreated(bool bExcludeFree = MOD_GLOBAL_TRULY_FREE_GP) const;
	void incrementGreatWritersCreated(bool bIsFree = MOD_GLOBAL_TRULY_FREE_GP);
	int getGreatArtistsCreated(bool bExcludeFree = MOD_GLOBAL_TRULY_FREE_GP) const;
	void incrementGreatArtistsCreated(bool bIsFree = MOD_GLOBAL_TRULY_FREE_GP);
	int getGreatMusiciansCreated(bool bExcludeFree = MOD_GLOBAL_TRULY_FREE_GP) const;
	void incrementGreatMusiciansCreated(bool bIsFree = MOD_GLOBAL_TRULY_FREE_GP);

	int getUnitClassesFromFaith(UnitClassTypes eIndex) const;
	void incrementUnitClassesFromFaith(UnitClassTypes eIndex);

	int getGreatPeopleThresholdModifier() const;
	void changeGreatPeopleThresholdModifier(int iChange);

	int getGreatGeneralsThresholdModifier() const;
	void changeGreatGeneralsThresholdModifier(int iChange);
	int getGreatAdmiralsThresholdModifier() const;
	void changeGreatAdmiralsThresholdModifier(int iChange);

	int getPolicyCostModifier() const;
	void recomputePolicyCostModifier();

	int getGreatPeopleRateModifier() const;
	int getGreatGeneralRateModifier() const;
	int getGreatAdmiralRateModifier() const;
	int getGreatWriterRateModifier() const;
	int getGreatArtistRateModifier() const;
	int getGreatMusicianRateModifier() const;
	int getGreatMerchantRateModifier() const;
	int getGreatScientistRateModifier() const;
	int getGreatEngineerRateModifier() const;
	int getDomesticGreatGeneralRateModifier() const;
	void changeGreatPeopleRateModFromBldgs(int ichange);
	void changeGreatGeneralRateModFromBldgs(int ichange);
	void recomputeGreatPeopleModifiers();
	int GetGreatPeopleRateModFromFriendships() const;

	int GetGreatScientistBeakerPolicyMod() const;
	void SetGreatScientistBeakerPolicyMod(int iValue);
	void ChangeGreatScientistBeakerPolicyMod(int iChange);

	int GetGreatScientistBeakerMod() const;
	void SetGreatScientistBeakerMod(int iValue);
	void ChangeGreatScientistBeakerMod(int iChange);

	int GetProductionBeakerMod() const;
	void SetProductionBeakerMod(int iValue);
	void ChangeProductionBeakerMod(int iChange);

	bool CanAlwaysWeLoveKindDayInGoldenAge() const;
	int GetAlwaysWeLoveKindDayInGoldenAge() const;
	void ChangeAlwaysWeLoveKindDayInGoldenAge(int iValue);

	bool CanNoResistance() const;
	int GetNoResistance() const;
	void ChangeNoResistance(int iValue);

	bool CanUpgradeAllTerritory() const;
	int GetUpgradeAllTerritory() const;
	void ChangeUpgradeAllTerritory(int iValue);

	bool CanNoTechForWonder() const;
	int GetNoTechForWonder() const;
	void ChangeNoTechForWonder(int iChange);
	bool CanNoTechForProject() const;
	int GetNoTechForProject() const;
	void ChangeNoTechForProject(int iChange);

	int getCityCaptureHealGlobal() const;
	void changeCityCaptureHealGlobal(int iChange);
	int getOriginalCapitalCaptureTech() const;
	void changeOriginalCapitalCaptureTech(int iChange);
	int getOriginalCapitalCapturePolicy() const;
	void changeOriginalCapitalCapturePolicy(int iChange);
	int getOriginalCapitalCaptureGreatPerson() const;
	void changeOriginalCapitalCaptureGreatPerson(int iChange);

	int GetGreatGeneralCombatBonus() const;
	void SetGreatGeneralCombatBonus(int iValue);

	// Unit Killed in Combat
	void DoUnitKilledCombat(CvUnit* pKillingUnit, PlayerTypes eKilledPlayer, UnitTypes eUnitType, CvUnit* pKilledUnit);

	// Great People Expenditure
#if defined(MOD_EVENTS_GREAT_PEOPLE)
	void DoGreatPersonExpended(UnitTypes eGreatPersonUnit, CvUnit* pGreatPersonUnit);
#else
	void DoGreatPersonExpended(UnitTypes eGreatPersonUnit);
#endif
	int GetGreatPersonExpendGold() const;
	void ChangeGreatPersonExpendGold(int iChange);

	// Great People Spawning
	void DoSeedGreatPeopleSpawnCounter();
	void DoApplyNewAllyGPBonus();
	int GetGreatPeopleSpawnCounter();
	void SetGreatPeopleSpawnCounter(int iValue);
	void ChangeGreatPeopleSpawnCounter(int iChange);

	void DoSpawnGreatPerson(PlayerTypes eMinor, bool bIsFree = MOD_GLOBAL_TRULY_FREE_GP);
	void DoGreatPeopleSpawnTurn();
	CvCity* GetGreatPersonSpawnCity(UnitTypes eUnit);

	// End Great People Stuff

	int getMaxGlobalBuildingProductionModifier() const;
	void changeMaxGlobalBuildingProductionModifier(int iChange);

	int getMaxTeamBuildingProductionModifier() const;
	void changeMaxTeamBuildingProductionModifier(int iChange);

	int getMaxPlayerBuildingProductionModifier() const;
	void changeMaxPlayerBuildingProductionModifier(int iChange);

	int getFreeExperience() const;
	void changeFreeExperienceFromBldgs(int ichange);
	void changeFreeExperienceFromMinors(int ichange);
	void recomputeFreeExperience();

	void doUpdateBarbarianCampVisibility();

	int getFeatureProductionModifier() const;
	void changeFeatureProductionModifier(int iChange);

	int getWorkerSpeedModifier() const;
	void changeWorkerSpeedModifier(int iChange);

	int getSharedIdeologyTourismModifier() const;
	void changeSharedIdeologyTourismModifier(int iChange);

	bool IsRemoveOceanImpassableCombatUnit() const;
	void ChangeRemoveOceanImpassableCombatUnit(int iChange);

	bool IsRemoveOceanImpassableCivilian() const;
	void ChangeRemoveOceanImpassableCivilian(int iChange);
#if defined(MOD_POLICY_NEW_EFFECT_FOR_SP)
	int getDifferentIdeologyTourismModifier() const;
	void changeDifferentIdeologyTourismModifier(int iChange);

	int getHappinessPerPolicy() const;
	void changeHappinessPerPolicy(int iChange);

	int getWaterBuildSpeedModifier() const;
	void changeWaterBuildSpeedModifier(int iChange);

	int getSettlerProductionEraModifier(EraTypes eEra) const;
	void changeSettlerProductionEraModifier(EraTypes eStartEra, int iChange);

	int getBuildSpeedModifier(BuildTypes eBuild) const;
	void changeBuildSpeedModifier(BuildTypes eBuild, int iChange);
#endif
	bool isNullifyInfluenceModifier() const;
	void changeNullifyInfluenceModifier(int iChange);

	int getNumTradeRouteBonus() const;
	void changeNumTradeRouteBonus(int iChange);

	int getTradeRouteDomainExtraRange(DomainTypes eIndex) const;
	void changeTradeRouteDomainExtraRange(DomainTypes eIndex, int iChange);

#if defined(MOD_BUILDING_NEW_EFFECT_FOR_SP)
	int getCityDefenseModifierGlobal() const;
	void changeCityDefenseModifierGlobal(int iChange);

	int GetCityStateTradeRouteYieldModifierGlobal(YieldTypes eYield) const;
	void ChangeCityStateTradeRouteYieldModifierGlobal(YieldTypes eYield, int iChange);

	int getLandmarksTourismPercentGlobal() const;
	void changeLandmarksTourismPercentGlobal(int iChange);

	int getGreatWorksTourismModifierGlobal() const;
	void changeGreatWorksTourismModifierGlobal(int iChange);

	int getTradeRouteSeaGoldBonusGlobal() const;
	void changeTradeRouteSeaGoldBonusGlobal(int iChange);

	int getTradeRouteLandGoldBonusGlobal() const;
	void changeTradeRouteLandGoldBonusGlobal(int iChange);
#endif

	int getImprovementCostModifier() const;
	void changeImprovementCostModifier(int iChange);

	int getImprovementUpgradeRateModifier() const;
	void changeImprovementUpgradeRateModifier(int iChange);

	int getSpecialistProductionModifier() const;
	void changeSpecialistProductionModifier(int iChange);

	int getMilitaryProductionModifier() const;
	void changeMilitaryProductionModifier(int iChange);

	int getSpaceProductionModifier() const;
	void changeSpaceProductionModifier(int iChange);

	int getCityDefenseModifier() const;
	void changeCityDefenseModifier(int iChange);

	int getUnitFortificationModifier() const;
	void changeUnitFortificationModifier(int iChange);

	int getUnitBaseHealModifier() const;
	void changeUnitBaseHealModifier(int iChange);

	int getWonderProductionModifier() const;
	void changeWonderProductionModifier(int iChange);

	int getSettlerProductionModifier() const;
	void changeSettlerProductionModifier(int iChange);

	int getCapitalSettlerProductionModifier() const;
	void changeCapitalSettlerProductionModifier(int iChange);

	int GetPolicyCostBuildingModifier() const;
	void ChangePolicyCostBuildingModifier(int iChange);

	int GetPolicyCostMinorCivModifier() const;
	void ChangePolicyCostMinorCivModifier(int iChange);

	int GetInfluenceSpreadModifier() const;
	void ChangeInfluenceSpreadModifier(int iChange);

	int GetExtraVotesPerDiplomat() const;
	void ChangeExtraVotesPerDiplomat(int iChange);

	int getNumNukeUnits() const;
	void changeNumNukeUnits(int iChange);

	int getBaseFreeUnits() const;
	void changeBaseFreeUnits(int iChange);

	int getNumOutsideUnits();
	void changeNumOutsideUnits(int iChange);
	int GetVerifiedOutsideUnitCount();

	int getGoldPerUnit() const;
	void changeGoldPerUnit(int iChange);
	int getGoldPerUnitTimes100() const;
	void changeGoldPerUnitTimes100(int iChange);

	int getGoldPerMilitaryUnit() const;
	void changeGoldPerMilitaryUnit(int iChange);

	int GetRouteGoldMaintenanceMod() const;
	void ChangeRouteGoldMaintenanceMod(int iChange);

	int GetBuildingGoldMaintenanceMod() const;
	void ChangeBuildingGoldMaintenanceMod(int iChange);

	int GetUnitGoldMaintenanceMod() const;
	void ChangeUnitGoldMaintenanceMod(int iChange);

	int GetUnitSupplyMod() const;
	void ChangeUnitSupplyMod(int iChange);

	int getExtraUnitCost() const;
	void changeExtraUnitCost(int iChange);

	int GetNumMaintenanceFreeUnits(DomainTypes eDomain = NO_DOMAIN, bool bOnlyCombatUnits = false) const;

	int getNumMilitaryUnits() const;
	void changeNumMilitaryUnits(int iChange);

	int getHappyPerMilitaryUnit() const;
	void changeHappyPerMilitaryUnit(int iChange);

	int getHappinessToCulture() const;
	void changeHappinessToCulture(int iChange);

	int getHappinessToScience() const;
	void changeHappinessToScience(int iChange);

	int getHalfSpecialistUnhappinessCount() const;
	bool isHalfSpecialistUnhappiness() const;
	void changeHalfSpecialistUnhappinessCount(int iChange);

	int getHalfSpecialistFoodCount() const;
	bool isHalfSpecialistFood() const;
	void changeHalfSpecialistFoodCount(int iChange);

	bool IsPlayerMoveAfterCreated() const;
	void ChangePlayerMoveAfterCreated(int iChange);

#if defined(MOD_ROG_CORE)
	void ChangeDomainFreeExperiencePerGreatWorkGlobal(DomainTypes eDomain, int iChange);
	int GetDomainFreeExperiencePerGreatWorkGlobal(DomainTypes eDomain) const;
	
	int GetDomainFreeExperiencesPerPopGlobal(DomainTypes eDomain);
	void ChangeDomainFreeExperiencesPerTurnGlobal(DomainTypes eDomain, int iChange);
	int GetDomainFreeExperiencesPerTurnGlobal(DomainTypes eDomain) const;
	void ChangeDomainEnemyCombatModifierGlobal(DomainTypes eDomain, int iChange);
	int GetDomainEnemyCombatModifierGlobal(DomainTypes eDomain) const;
	void ChangeDomainFreeExperience(DomainTypes eDomain, int iChange);
	int GetDomainFreeExperience(DomainTypes) const;

	void ChangeUnitTypePrmoteHealGlobal(UnitTypes eUnit, int iChange);
	int GetUnitTypePrmoteHealGlobal(UnitTypes) const;
#endif
	void ChangeEraUnitClassMaxInstances(UnitClassTypes eUnitClass, int iChange);
	int GetEraUnitClassMaxInstances(UnitClassTypes eUnitClass) const;

	int getPolicyModifiers(PolicyModifierType eIndex) const;
	void changePolicyModifiers(PolicyModifierType eIndex, int iChange);

	int getMilitaryFoodProductionCount() const;
	bool isMilitaryFoodProduction() const;
	void changeMilitaryFoodProductionCount(int iChange);

	int GetGoldenAgeCultureBonusDisabledCount() const;
	bool IsGoldenAgeCultureBonusDisabled() const;
	void ChangeGoldenAgeCultureBonusDisabledCount(int iChange);

	int GetSecondReligionPantheonCount() const;
	bool IsSecondReligionPantheon() const;
	void ChangeSecondReligionPantheonCount(int iChange);

	int GetEnablesSSPartHurryCount() const;
	bool IsEnablesSSPartHurry() const;
	void ChangeEnablesSSPartHurryCount(int iChange);

	int GetEnablesSSPartPurchaseCount() const;
	bool IsEnablesSSPartPurchase() const;
	void ChangeEnablesSSPartPurchaseCount(int iChange);

	int getHighestUnitLevel() const;
	void setHighestUnitLevel(int iNewValue);

	int getConscriptCount() const;
	void setConscriptCount(int iNewValue);
	void changeConscriptCount(int iChange);

	int getMaxConscript() const;
	void changeMaxConscript(int iChange);


	int GetNumCSAllies() const;
	void SetNumCSAllies(int iValue);

	int GetNumCSFriends() const;
	void SetNumCSFriends(int iValue);

	void RefreshCSAlliesFriends();


	int getOverflowResearch() const;
	void setOverflowResearch(int iNewValue);
	void changeOverflowResearch(int iChange);
	int getOverflowResearchTimes100() const;
	void setOverflowResearchTimes100(int iNewValue);
	void changeOverflowResearchTimes100(long long iChange);

	int getExpModifier() const;
	void changeExpModifier(int iChange);

	int getExpInBorderModifier() const;
	void changeExpInBorderModifier(int iChange);

	int getLevelExperienceModifier() const;
	void changeLevelExperienceModifier(int iChange);

	int getMinorQuestFriendshipMod() const;
	void changeMinorQuestFriendshipMod(int iChange);

	int getMinorGoldFriendshipMod() const;
	void changeMinorGoldFriendshipMod(int iChange);

	int GetMinorFriendshipAnchorMod() const;
	void SetMinorFriendshipAnchorMod(int iValue);
	void ChangeMinorFriendshipAnchorMod(int iChange);

	int GetMinorFriendshipDecayMod() const;
	void changeGetMinorFriendshipDecayMod(int iChange);

	bool IsMinorScienceAllies() const;
	int GetMinorScienceAlliesCount() const;
	void ChangeMinorScienceAlliesCount(int iChange);

	bool IsMinorResourceBonus() const;
	int GetMinorResourceBonusCount() const;
	void ChangeMinorResourceBonusCount(int iChange);

	bool IsAbleToAnnexCityStates() const;
	int GetAbleToAnnexCityStatesCount() const;
	void ChangeAbleToAnnexCityStatesCount(int iChange);

	int getCultureBombTimer() const;
	void setCultureBombTimer(int iNewValue);
	void changeCultureBombTimer(int iChange);

	int getConversionTimer() const;
	void setConversionTimer(int iNewValue);
	void changeConversionTimer(int iChange);

	CvCity* getCapitalCity();
	const CvCity* getCapitalCity() const;
	void setCapitalCity(CvCity* pNewCapitalCity);

	int GetOriginalCapitalX() const;
	int GetOriginalCapitalY() const;



	int GetYieldFromProcessModifierGlobal(YieldTypes eIndex1) const;
	void ChangeYieldFromProcessModifierGlobal(YieldTypes eIndex, int iChange);


	bool IsHasLostCapital() const;
	void SetHasLostCapital(bool bValue, PlayerTypes eConqueror);
#if defined(MOD_GLOBAL_NO_CONQUERED_SPACESHIPS)
	void disassembleSpaceship(CvPlot* pPlot);
#endif
	PlayerTypes GetCapitalConqueror() const;

	int getCitiesLost() const;
	void changeCitiesLost(int iChange);

	int getPower() const;
	int GetMilitaryMight() const;
	int GetEconomicMight() const;
	int calculateMilitaryMight() const;
	int calculateEconomicMight() const;
	int calculateProductionMight() const;

#if defined(MOD_UNITS_XP_TIMES_100)
	int getCombatExperienceTimes100() const;
#else
	int getCombatExperience() const;
#endif
#if defined(MOD_GLOBAL_LOCAL_GENERALS)
#if defined(MOD_UNITS_XP_TIMES_100)
	void setCombatExperienceTimes100(int iExperienceTimes100, CvUnit* pFromUnit = NULL);
	void changeCombatExperienceTimes100(int iChangeTimes100, CvUnit* pFromUnit = NULL);
#else
	void setCombatExperience(int iExperience, CvUnit* pFromUnit = NULL);
	void changeCombatExperience(int iChange, CvUnit* pFromUnit = NULL);
#endif
#else
#if defined(MOD_UNITS_XP_TIMES_100)
	void setCombatExperienceTimes100(int iExperienceTimes100);
	void changeCombatExperienceTimes100(int iChangeTimes100);
#else
	void setCombatExperience(int iExperience);
	void changeCombatExperience(int iChange);
#endif
#endif
#if defined(MOD_UNITS_XP_TIMES_100)
	int getLifetimeCombatExperienceTimes100() const;
	int getNavalCombatExperienceTimes100() const;
#else
	int getLifetimeCombatExperience() const;
	int getNavalCombatExperience() const;
#endif
#if defined(MOD_GLOBAL_LOCAL_GENERALS)
#if defined(MOD_UNITS_XP_TIMES_100)
	void setNavalCombatExperienceTimes100(int iExperienceTimes100, CvUnit* pFromUnit = NULL);
	void changeNavalCombatExperienceTimes100(int iChangeTimes100, CvUnit* pFromUnit = NULL);
#else
	void setNavalCombatExperience(int iExperience, CvUnit* pFromUnit = NULL);
	void changeNavalCombatExperience(int iChange, CvUnit* pFromUnit = NULL);
#endif
#else
#if defined(MOD_UNITS_XP_TIMES_100)
	void setNavalCombatExperienceTimes100(int iExperienceTimes100);
	void changeNavalCombatExperienceTimes100(int iChangeTimes100);
#else
	void setNavalCombatExperience(int iExperience);
	void changeNavalCombatExperience(int iChange);
#endif
#endif

	int getBorderObstacleCount() const;
	bool isBorderObstacle() const;
	void changeBorderObstacleCount(int iChange);

	int getForbiddenForeignSpyGlobalCount() const;
	bool isForbiddenForeignSpyGlobal() const;
	void changeForbiddenForeignSpyGlobalCount(int iChange);

	int getNetID() const;
	void setNetID(int iNetID);
	bool isConnected() const;
	void sendTurnReminder();

	uint getStartTime() const;
	void setStartTime(uint uiStartTime);
	uint getTotalTimePlayed() const;

	bool isMinorCiv() const;
#if defined(MOD_API_EXTENSIONS)
	bool isMajorCiv() const;
	BuildingTypes GetCivBuilding(BuildingClassTypes eBuildingClass) const;
	UnitTypes GetCivUnit(UnitClassTypes eUnitClass, int iFakeSeed = 0) const;
#endif
	bool IsHasBetrayedMinorCiv() const;
	void SetHasBetrayedMinorCiv(bool bValue);

	void setAlive(bool bNewValue, bool bNotify = true);
	void verifyAlive();
	bool isAlive() const
	{
		return m_bAlive;
	}

	bool isEverAlive() const
	{
		return m_bEverAlive;
	}

	void setBeingResurrected(bool bNewValue);
	bool isBeingResurrected() const
	{
		return m_bBeingResurrected;
	}

	bool isTurnActive() const;
	void setTurnActive(bool bNewValue, bool bDoTurn = true);
	bool isSimultaneousTurns() const;
	void setDynamicTurnsSimultMode(bool simultaneousTurns);

	bool isAutoMoves() const;
	void setAutoMoves(bool bNewValue);
	bool hasProcessedAutoMoves() const;
	void setProcessedAutoMoves(bool bNewValue);

	void setTurnActiveForPbem(bool bActive);

	bool isPbemNewTurn() const;
	void setPbemNewTurn(bool bNew);

	bool isEndTurn() const;
	void setEndTurn(bool bNewValue);

	EndTurnBlockingTypes GetEndTurnBlockingType(void) const;
	int GetEndTurnBlockingNotificationIndex(void) const;
	void SetEndTurnBlocking(EndTurnBlockingTypes eBlockingType, int iNotificationIndex);

	bool isTurnDone() const;
	bool isLocalPlayer() const;
	bool isExtendedGame() const;
	void makeExtendedGame();

	bool isFoundedFirstCity() const;
	void setFoundedFirstCity(bool bNewValue);
	int GetNumCitiesFounded() const;
	void ChangeNumCitiesFounded(int iValue);

	// slewis - centralizing code where a player gets whacked by another player
	void CheckForMurder(PlayerTypes ePossibleVictimPlayer); // check to see if we defeated this other player

	bool isStrike() const;
	void setStrike(bool bNewValue);

	bool IsCramped() const;
	void DoUpdateCramped();

	CvHandicapInfo& getHandicapInfo() const;
	HandicapTypes getHandicapType() const;

	CvCivilizationInfo& getCivilizationInfo() const;
	CivilizationTypes getCivilizationType() const;

	CvLeaderHeadInfo& getLeaderInfo() const;
	LeaderHeadTypes getLeaderType() const;
#if defined(MOD_API_EXTENSIONS)
	void setLeaderType(LeaderHeadTypes eNewLeader);
#endif

	LeaderHeadTypes getPersonalityType() const;
	void setPersonalityType(LeaderHeadTypes eNewValue);

	EraTypes GetCurrentEra() const;

	PlayerTypes GetID() const
	{
		return m_eID;
	}

	static TeamTypes getTeam(PlayerTypes ePlayerID)
	{
		return CvPreGame::teamType(ePlayerID);
	}

	TeamTypes getTeam() const
	{
		return CvPreGame::teamType(m_eID);
	}

	void setTeam(TeamTypes eTeam);
	bool IsAITeammateOfHuman() const;

	PlayerColorTypes getPlayerColor() const;
	const CvColorA& getPlayerTextColor() const;

	int getSeaPlotYield(YieldTypes eIndex) const;
	void changeSeaPlotYield(YieldTypes eIndex, int iChange);

	int getRiverPlotYield(YieldTypes eIndex) const;
	void changeRiverPlotYield(YieldTypes eIndex, int iChange);

	int getCityLoveKingDayYieldMod(YieldTypes eIndex) const;
	void changeCityLoveKingDayYieldMod(YieldTypes eIndex, int iChange);

	int getYieldRateModifier(YieldTypes eIndex) const;
	void changeYieldRateModifier(YieldTypes eIndex, int iChange);

	int getGoldenAgeYieldRateModifier(YieldTypes eIndex) const;
	void changeGoldenAgeYieldRateModifier(YieldTypes eIndex, int iChange);

	int getCapitalYieldRateModifier(YieldTypes eIndex) const;
	void changeCapitalYieldRateModifier(YieldTypes eIndex, int iChange);

	int getExtraYieldThreshold(YieldTypes eIndex) const;
	void updateExtraYieldThreshold(YieldTypes eIndex);

#if defined(MOD_ROG_CORE)
	int getWorldWonderCityYieldRateModifier(YieldTypes eIndex) const;
	void changeWorldWonderCityYieldRateModifier(YieldTypes eIndex, int iChange);

	int GetImprovementExtraYield(ImprovementTypes eImprovement, YieldTypes eYield) const;
	void ChangeImprovementExtraYield(ImprovementTypes eImprovement, YieldTypes eYield, int iChange);

	int GetYieldModifierFromSpecialistGlobal(SpecialistTypes eSpecialist, YieldTypes eYield) const;
	void ChangeYieldModifierFromSpecialistGlobal(SpecialistTypes eSpecialist, YieldTypes eYield, int iChange);


	int GetYieldFromPillage(YieldTypes eIndex) const;
	void ChangeYieldFromPillage(YieldTypes eIndex, int iChange);

	int GetYieldFromPillagePlayer(YieldTypes eIndex) const;
	void ChangeYieldFromPillagePlayer(YieldTypes eIndex, int iChange);
#endif

	// Science

	int GetScience() const;
	int GetScienceTimes100(bool bIgnoreFriendships = false) const;

	int GetScienceFromCitiesTimes100(bool bIgnoreTrade) const;
	int GetScienceFromOtherPlayersTimes100() const;
	int GetScienceFromHappinessTimes100(int iScienceFromCities) const;
	int GetScienceFromResearchAgreementsTimes100(int iScienceFromCities) const;
	int GetScienceFromBudgetDeficitTimes100() const;

	int GetScienceYieldFromPreviousTurns(int iGameTurn, int iNumPreviousTurnsToCount);

	bool IsGetsScienceFromPlayer(PlayerTypes ePlayer) const;
	void SetGetsScienceFromPlayer(PlayerTypes ePlayer, bool bValue);

	// END Science

	void DoDeficit();

	int getSpecialistExtraYield(YieldTypes eIndex) const;
	void changeSpecialistExtraYield(YieldTypes eIndex, int iChange);

	PlayerProximityTypes GetProximityToPlayer(PlayerTypes ePlayer) const;
	void SetProximityToPlayer(PlayerTypes ePlayer, PlayerProximityTypes eProximity);
	void DoUpdateProximityToPlayer(PlayerTypes ePlayer);

	void UpdateResearchAgreements(int iValue);
	int GetResearchAgreementCounter(PlayerTypes ePlayer) const;
	void SetResearchAgreementCounter(PlayerTypes ePlayer, int iValue);
	void ChangeResearchAgreementCounter(PlayerTypes ePlayer, int iChange);

	void DoCivilianReturnLogic(bool bReturn, PlayerTypes eToPlayer, int iUnitID);

	// Incoming Units
	void DoIncomingUnits();

	void DoDistanceGift(PlayerTypes eFromPlayer, CvUnit* pUnit);
	void AddIncomingUnit(PlayerTypes eFromPlayer, CvUnit* pUnit);

	UnitTypes GetIncomingUnitType(PlayerTypes eFromPlayer) const;
	void SetIncomingUnitType(PlayerTypes eFromPlayer, UnitTypes eUnitType);

	int GetIncomingUnitCountdown(PlayerTypes eFromPlayer) const;
	void SetIncomingUnitCountdown(PlayerTypes eFromPlayer, int iNumTurns);
	void ChangeIncomingUnitCountdown(PlayerTypes eFromPlayer, int iChange);

	bool isOption(PlayerOptionTypes eIndex) const;
	void setOption(PlayerOptionTypes eIndex, bool bNewValue);

	bool isPlayable() const;
	void setPlayable(bool bNewValue);

	int getNumResourceUsed(ResourceTypes eIndex) const;
	void changeNumResourceUsed(ResourceTypes eIndex, int iChange);
	int getNumResourceTotal(ResourceTypes eIndex, bool bIncludeImport = true) const;
	void changeNumResourceTotal(ResourceTypes eIndex, int iChange, bool bIgnoreResourceWarning = false);

	int getSiphonLuxuryCount(PlayerTypes eFromPlayer) const;
	void changeSiphonLuxuryCount(PlayerTypes eFromPlayer, int iChange);
	
	void UpdateResourcesSiphoned();

	void DoTestOverResourceNotification(ResourceTypes eIndex, bool bIsDoTurn = false);
	void DoTestOverResourceNotificationAll();

	int GetStrategicResourceMod() const;
	void ChangeStrategicResourceMod(int iChange);

	int getNumResourceAvailable(ResourceTypes eIndex, bool bIncludeImport = true) const;

	int getResourceGiftedToMinors(ResourceTypes eIndex) const;
	void changeResourceGiftedToMinors(ResourceTypes eIndex, int iChange);

	int getResourceExport(ResourceTypes eIndex) const;
	void changeResourceExport(ResourceTypes eIndex, int iChange);

	int getResourceImport(ResourceTypes eIndex) const;
	void changeResourceImport(ResourceTypes eIndex, int iChange);

	int getResourceFromMinors(ResourceTypes eIndex) const;
	void changeResourceFromMinors(ResourceTypes eIndex, int iChange);

	int getResourceSiphoned(ResourceTypes eIndex) const;
	void changeResourceSiphoned(ResourceTypes eIndex, int iChange);

#ifdef MOD_SPECIALIST_RESOURCES
	int getResourceFromSpecialists(ResourceTypes eIndex) const;
	void changeResourceFromSpecialists(ResourceTypes eIndex, int iChange);
	void UpdateResourceFromSpecialists();
	bool MeetSpecialistResourceRequirement(const CvSpecialistInfo::ResourceInfo& info) const;
#endif
	//the following func can only get a Fuzzy value, To help AI's resource strategies
	int getNumResourceTotalCache(ResourceTypes eIndex) const;
	void changeNumResourceTotalCache(ResourceTypes eIndex, int iChange);
	void setNumResourceTotalCache(ResourceTypes eIndex, int iValue);
	void updateNumResourceTotalCache();

	int getResourceInOwnedPlots(ResourceTypes eIndex);

	int getTotalImprovementsBuilt() const;
	void changeTotalImprovementsBuilt(int iChange);
	int getImprovementCount(ImprovementTypes eIndex) const;
	void changeImprovementCount(ImprovementTypes eIndex, int iChange);

	int getGreatPersonImprovementCount();

	int getFreeBuildingCount(BuildingTypes eIndex) const;
	bool isBuildingFree(BuildingTypes eIndex) const;
	void changeFreeBuildingCount(BuildingTypes eIndex, int iChange);

	int GetFreePromotionCount(PromotionTypes eIndex) const;
	bool IsFreePromotion(PromotionTypes eIndex) const;
	void ChangeFreePromotionCount(PromotionTypes eIndex, int iChange);
	void RemoveCurrentPromotion(PromotionTypes eIndex);
	void RemoveOceanImpassableCivilian();
	void RemoveOceanImpassableCombatUnit();

	int getUnitCombatProductionModifiers(UnitCombatTypes eIndex) const;
	void changeUnitCombatProductionModifiers(UnitCombatTypes eIndex, int iChange);
	int getUnitCombatFreeExperiences(UnitCombatTypes eIndex) const;
	void changeUnitCombatFreeExperiences(UnitCombatTypes eIndex, int iChange);

	int getUnitClassCount(UnitClassTypes eIndex) const;
	int getUnitCountFromHasPromotion(PromotionTypes ePromotion) const;
	void getUnitsListFromHasPromotion(PromotionTypes ePromotion, std::vector<const CvUnit*>& unitsListFromHasPromotion) const;
	bool isUnitClassMaxedOut(UnitClassTypes eIndex, int iExtra = 0) const;
	void changeUnitClassCount(UnitClassTypes eIndex, int iChange);
	int getUnitClassMaking(UnitClassTypes eIndex) const;
	void changeUnitClassMaking(UnitClassTypes eIndex, int iChange);
	int getUnitClassCountPlusMaking(UnitClassTypes eIndex) const;

	int getBuildingClassCount(BuildingClassTypes eIndex) const;
	bool isBuildingClassMaxedOut(BuildingClassTypes eIndex, int iExtra = 0) const;
	void changeBuildingClassCount(BuildingClassTypes eIndex, int iChange);
	int getBuildingClassMaking(BuildingClassTypes eIndex) const;
	void changeBuildingClassMaking(BuildingClassTypes eIndex, int iChange);
	int getBuildingClassCountPlusMaking(BuildingClassTypes eIndex) const;

	int getProjectMaking(ProjectTypes eIndex) const;
	void changeProjectMaking(ProjectTypes eIndex, int iChange);

	int getHurryCount(HurryTypes eIndex) const;
	bool IsHasAccessToHurry(HurryTypes eIndex) const;

	bool IsCanHurry(HurryTypes eIndex) const;
	int GetHurryGoldCost(HurryTypes eHurry) const;
	void DoHurry(HurryTypes eIndex);

	bool canPopRush();
	void changeHurryCount(HurryTypes eIndex, int iChange);
	int getHurryModifier(HurryTypes eIndex) const;
	void changeHurryModifier(HurryTypes eIndex, int iChange);

	void setResearchingTech(TechTypes eIndex, bool bNewValue);

	int getSpecialistExtraYield(SpecialistTypes eIndex1, YieldTypes eIndex2) const;
	void changeSpecialistExtraYield(SpecialistTypes eIndex1, YieldTypes eIndex2, int iChange);

	int getYieldFromYieldGlobal(YieldTypes eIndex1, YieldTypes eIndex2) const;
	void changeYieldFromYieldGlobal(YieldTypes eIndex1, YieldTypes eIndex2, int iChange);

#if defined(MOD_API_UNIFIED_YIELDS) && defined(MOD_API_PLOT_YIELDS)
	int getPlotYieldChange(PlotTypes eIndex1, YieldTypes eIndex2) const;
	void changePlotYieldChange(PlotTypes eIndex1, YieldTypes eIndex2, int iChange);
#endif

#if defined(MOD_API_UNIFIED_YIELDS)
	int getFeatureYieldChange(FeatureTypes eIndex1, YieldTypes eIndex2) const;
	void changeFeatureYieldChange(FeatureTypes eIndex1, YieldTypes eIndex2, int iChange);

	int getCityYieldFromUnimprovedFeature(FeatureTypes eIndex1, YieldTypes eIndex2) const;
	void changeCityYieldFromUnimprovedFeature(FeatureTypes eIndex1, YieldTypes eIndex2, int iChange);

	int getUnimprovedFeatureYieldChange(FeatureTypes eIndex1, YieldTypes eIndex2) const;
	void changeUnimprovedFeatureYieldChange(FeatureTypes eIndex1, YieldTypes eIndex2, int iChange);

	int getResourceYieldChange(ResourceTypes eIndex1, YieldTypes eIndex2) const;
	void changeResourceYieldChange(ResourceTypes eIndex1, YieldTypes eIndex2, int iChange);

	int getTerrainYieldChange(TerrainTypes eIndex1, YieldTypes eIndex2) const;
	void changeTerrainYieldChange(TerrainTypes eIndex1, YieldTypes eIndex2, int iChange);

	int getTradeRouteYieldChange(DomainTypes eIndex1, YieldTypes eIndex2) const;
	void changeTradeRouteYieldChange(DomainTypes eIndex1, YieldTypes eIndex2, int iChange);

	int getSpecialistYieldChange(SpecialistTypes eIndex1, YieldTypes eIndex2) const;
	void changeSpecialistYieldChange(SpecialistTypes eIndex1, YieldTypes eIndex2, int iChange);

	int getGreatPersonExpendedYield(GreatPersonTypes eIndex1, YieldTypes eIndex2) const;
	void changeGreatPersonExpendedYield(GreatPersonTypes eIndex1, YieldTypes eIndex2, int iChange);

	int getGoldenAgeGreatPersonRateModifier(GreatPersonTypes eIndex1) const;
	void changeGoldenAgeGreatPersonRateModifier(GreatPersonTypes eIndex1, int iChange);

	int GetYieldFromKills(YieldTypes eYield) const;
	void changeYieldFromKills(YieldTypes eYield, int iChange);

	int GetYieldFromBarbarianKills(YieldTypes eYield) const;
	void changeYieldFromBarbarianKills(YieldTypes eYield, int iChange);

	int GetYieldChangeTradeRoute(YieldTypes eYield) const;
	void ChangeYieldChangeTradeRoute(YieldTypes eYield, int iChange);

	int GetYieldChangesNaturalWonder(YieldTypes eYield) const;
	void ChangeYieldChangesNaturalWonder(YieldTypes eYield, int iChange);

	int GetYieldChangeWorldWonder(YieldTypes eYield) const;
	void ChangeYieldChangeWorldWonder(YieldTypes eYield, int iChange);

	int getBuildingClassYieldChange(BuildingClassTypes eIndex1, YieldTypes eIndex2) const;
	void changeBuildingClassYieldChange(BuildingClassTypes eIndex1, YieldTypes eIndex2, int iChange);
#endif

	int getImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2) const;
	void changeImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2, int iChange);

	CvUnitCycler& GetUnitCycler() { return m_UnitCycle; };

	bool removeFromArmy(int iArmyID, int iID);
	bool removeFromArmy(int iID);

	int findPathLength(TechTypes eTech, bool bCost = true) const;
	int getQueuePosition(TechTypes eTech) const;
	void clearResearchQueue();
	bool pushResearch(TechTypes eTech, bool bClear = false);
	void popResearch(TechTypes eTech);
	int getLengthResearchQueue() const;
	CLLNode<TechTypes>* nextResearchQueueNode(CLLNode<TechTypes>* pNode);
	const CLLNode<TechTypes>* nextResearchQueueNode(const CLLNode<TechTypes>* pNode) const;
	CLLNode<TechTypes>* headResearchQueueNode();
	const CLLNode<TechTypes>* headResearchQueueNode() const;
	CLLNode<TechTypes>* tailResearchQueueNode();

	void addCityName(const CvString& szName);
	int getNumCityNames() const;
	CvString getCityName(int iIndex) const;
	CLLNode<CvString>* nextCityNameNode(CLLNode<CvString>* pNode);
	const CLLNode<CvString>* nextCityNameNode(const CLLNode<CvString>* pNode) const;
	CLLNode<CvString>* headCityNameNode();
	const CLLNode<CvString>* headCityNameNode() const;

	// city iteration
	const CvCity* firstCity(int* pIterIdx, bool bRev=false) const;
	CvCity* firstCity(int* pIterIdx, bool bRev=false);
	const CvCity* nextCity(int* pIterIdx, bool bRev=false) const;
	CvCity* nextCity(int* pIterIdx, bool bRev=false);
	int getNumCities() const;
	CvCity* getCity(int iID);
	const CvCity* getCity(int iID) const;
	CvCity* addCity();
	void deleteCity(int iID);
	CvCity* GetFirstCityWithBuildingClass(BuildingClassTypes eBuildingClass);

	// unit iteration
	int getNumUnits() const;
	const CvUnit* firstUnit(int* pIterIdx, bool bRev=false) const;
	const CvUnit* nextUnit(int* pIterIdx, bool bRev=false) const;
	const CvUnit* getUnit(int iID) const;
	CvUnit* firstUnit(int* pIterIdx, bool bRev=false);
	CvUnit* nextUnit(int* pIterIdx, bool bRev=false);
	CvUnit* getUnit(int iID);
	CvUnit* addUnit();
	void deleteUnit(int iID);
	void UpdateCityStrength();

	// army iteration
	const CvArmyAI* firstArmyAI(int* pIterIdx, bool bRev=false) const;
	const CvArmyAI* nextArmyAI(int* pIterIdx, bool bRev=false) const;
	CvArmyAI* firstArmyAI(int* pIterIdx, bool bRev=false);
	CvArmyAI* nextArmyAI(int* pIterIdx, bool bRev=false);
	int getNumArmyAIs() const;
	const CvArmyAI* getArmyAI(int iID) const;
	CvArmyAI* getArmyAI(int iID);
	CvArmyAI* addArmyAI();
	void deleteArmyAI(int iID);

	// operations
	CvAIOperation* getFirstAIOperation();
	CvAIOperation* getNextAIOperation();
	CvAIOperation* getAIOperation(int iID);
	const CvAIOperation* getAIOperation(int iID) const;
	CvAIOperation* addAIOperation(int iOperationType, PlayerTypes eEnemy=NO_PLAYER, int iArea=-1, CvCity* pTarget=NULL, CvCity* pMuster=NULL);
	void deleteAIOperation(int iID);
	bool haveAIOperationOfType(int iOperationType, int* piID=NULL, PlayerTypes eTargetPlayer = NO_PLAYER, CvPlot* pTargetPlot=NULL);
	int numOperationsOfType(int iOperationType);
	bool IsCityAlreadyTargeted(CvCity* pCity, DomainTypes eDomain=NO_DOMAIN, int iPercentToTarget=100, int iIgnoreOperationID=-1) const;
	bool IsPlotTargetedForCity(CvPlot *pPlot) const;
	void GatherPerTurnReplayStats(int iGameTurn);
	unsigned int getNumReplayDataSets() const;
	const char* getReplayDataSetName(unsigned int uiDataSet) const;
	unsigned int getReplayDataSetIndex(const char* szDataSetName);
	int getReplayDataValue(unsigned int uiDataSet, unsigned int uiTurn) const;
	void setReplayDataValue(unsigned int uiDataSet, unsigned int uiTurn, int iValue);
	TurnData getReplayDataHistory(unsigned int uiDataSet) const;

	// Arbitrary Script Data
	std::string getScriptData() const;
	void setScriptData(std::string szNewValue);

	const CvString& getPbemEmailAddress() const;
	void setPbemEmailAddress(const char* szAddress);

	int getUnitExtraCost(UnitClassTypes eUnitClass) const;
	void setUnitExtraCost(UnitClassTypes eUnitClass, int iCost);

	void launch(VictoryTypes victoryType);

	void invalidatePopulationRankCache();
	void invalidateYieldRankCache(YieldTypes eYield = NO_YIELD);

	PlayerTypes pickConqueredCityOwner(const CvCity& kCity) const;

	int getNewCityProductionValue() const;

	int getGrowthThreshold(int iPopulation) const;

	int GetCityStrengthMod() const;
	void SetCityStrengthMod(int iValue);
	void ChangeCityStrengthMod(int iChange);

#if defined(MOD_ROG_CORE)
	int GetGlobalCityStrengthMod() const;
	void SetGlobalCityStrengthMod(int iValue);
	void ChangeGlobalCityStrengthMod(int iChange);

	int GetGlobalRangedStrikeModifier() const;
	void SetGlobalRangedStrikeModifier(int iValue);
	void ChangeGlobalRangedStrikeModifier(int iChange);

	int GetResearchTotalCostModifier() const;
	void ChangeResearchTotalCostModifier(int iChange);
	int GetResearchTotalCostModifierGoldenAge() const;
	void ChangeResearchTotalCostModifierGoldenAge(int iChange);

	int GetLiberatedInfluence() const;
	void SetLiberatedInfluence(int iValue);
	void ChangeLiberatedInfluence(int iChange);

	int GetExtraUnitPlayerInstances() const;
	void SetExtraUnitPlayerInstances(int iValue);
	void ChangeExtraUnitPlayerInstances(int iChange);

	int GetConquestCasualtiesModifier() const;
	void ChangeConquestCasualtiesModifier(int iChange);

	int GetWaterTileDamageGlobal() const;
	void SetWaterTileDamageGlobal(int iValue);
	void ChangeWaterTileDamageGlobal(int iChange);

	int GetWaterTileMovementReduceGlobal() const;
	void SetWaterTileMovementReduceGlobal(int iValue);
	void ChangeWaterTileMovementReduceGlobal(int iChange);

	int GetWaterTileTurnDamageGlobal() const;
	void SetWaterTileTurnDamageGlobal(int iValue);
	void ChangeWaterTileTurnDamageGlobal(int iChange);

	int GetLandTileDamageGlobal() const;
	void SetLandTileDamageGlobal(int iValue);
	void ChangeLandTileDamageGlobal(int iChange);

	int GetLandTileMovementReduceGlobal() const;
	void SetLandTileMovementReduceGlobal(int iValue);
	void ChangeLandTileMovementReduceGlobal(int iChange);

	int GetLandTileTurnDamageGlobal() const;
	void SetLandTileTurnDamageGlobal(int iValue);
	void ChangeLandTileTurnDamageGlobal(int iChange);
#endif

	int GetCityGrowthMod() const;
	void SetCityGrowthMod(int iValue);
	void ChangeCityGrowthMod(int iChange);

	int GetCapitalGrowthMod() const;
	void SetCapitalGrowthMod(int iValue);
	void ChangeCapitalGrowthMod(int iChange);

	void InitPlots();  // this needs to be called after the map is inited. It makes the list of how many plots the player controls
	void UpdatePlots();  // Modifies the list of plots and sets which ones the player owns
	void AddAPlot(CvPlot* pPlot); // adds a plot at the end of the list
	CvPlotsVector& GetPlots();  // gets the list of plots the player owns
	void AddPlotsToList(std::list<CvPlot*>& lTargetList);
	int GetNumPlots() const;

	int GetNumPlotsBought() const;
	void SetNumPlotsBought(int iValue);
	void ChangeNumPlotsBought(int iChange);

	int GetBuyPlotCost() const;

	int GetPlotGoldCostMod() const;
	void ChangePlotGoldCostMod(int iChange);
#if defined(MOD_TRAITS_CITY_WORKING) || defined(MOD_BUILDINGS_CITY_WORKING) || defined(MOD_POLICIES_CITY_WORKING) || defined(MOD_TECHS_CITY_WORKING)
	int GetCityWorkingChange() const;
	void ChangeCityWorkingChange(int iChange);
#endif

#if defined(MOD_TRAITS_CITY_AUTOMATON_WORKERS) || defined(MOD_BUILDINGS_CITY_AUTOMATON_WORKERS) || defined(MOD_POLICIES_CITY_AUTOMATON_WORKERS) || defined(MOD_TECHS_CITY_AUTOMATON_WORKERS)
	int GetCityAutomatonWorkersChange() const;
	void ChangeCityAutomatonWorkersChange(int iChange);
#endif

	int GetPlotCultureCostModifier() const;
	void ChangePlotCultureCostModifier(int iChange);
	int GetPlotCultureExponentModifier() const;
	void ChangePlotCultureExponentModifier(int iChange);

	int GetNumCitiesPolicyCostDiscount() const;
	void ChangeNumCitiesPolicyCostDiscount(int iChange);

	int GetGarrisonedCityRangeStrikeModifier() const;
	void ChangeGarrisonedCityRangeStrikeModifier(int iChange);
	bool IsGarrisonFreeMaintenance() const;
	void ChangeGarrisonFreeMaintenanceCount(int iChange);

	int GetUnitPurchaseCostModifier() const;
	void ChangeUnitPurchaseCostModifier(int iChange);

	int GetPlotDanger(CvPlot& Plot) const;
	bool IsPlotUnderImmediateThreat(CvPlot& Plot) const;
	CvCity* GetClosestFriendlyCity(CvPlot& plot, int iSearchRadius);

	int GetNumPuppetCities() const;
	int GetMaxEffectiveCities(bool bIncludePuppets = false);

	int GetNumNaturalWondersDiscoveredInArea() const;
	void SetNumNaturalWondersDiscoveredInArea(int iValue);
	void ChangeNumNaturalWondersDiscoveredInArea(int iChange);

	int GetNumNaturalWondersInOwnedPlots();

	int GetTurnsSinceSettledLastCity() const;
	void SetTurnsSinceSettledLastCity(int iValue);
	void ChangeTurnsSinceSettledLastCity(int iChange);

	int GetBestSettleAreas(int iMinScore, int& iFirstArea, int& iSecondArea);
	CvPlot* GetBestSettlePlot(CvUnit* pUnit, bool bEscorted, int iArea = -1) const;

	// New Victory Stuff
	int GetNumWonders() const;
	void ChangeNumWonders(int iValue);
	int GetNumPolicies() const;
	void ChangeNumPolicies(int iValue);
	int GetNumGreatPeople() const;
	void ChangeNumGreatPeople(int iValue);
	// End New Victory Stuff

	void DoAdoptedGreatPersonCityStatePolicy();
	bool IsAlliesGreatPersonBiasApplied() const;
	void SetAlliesGreatPersonBiasApplied(bool bValue);

	// New Religion Stuff
	bool IsHasAdoptedStateReligion() const;
	void SetHasAdoptedStateReligion(bool bValue);

	int GetNumCitiesWithStateReligion();

	// the following 2 functions seem to have no use
	CvCity* GetHolyCity();
	void SetHolyCity(int iCityID);

	PromotionTypes GetEmbarkationPromotion() const;
#if defined(MOD_PROMOTIONS_DEEP_WATER_EMBARKATION)
	PromotionTypes GetDeepWaterEmbarkationPromotion() const;
#endif

	void DoAnnounceReligionAdoption();
	void processReligion(ReligionTypes eReligion, int iChange);
	void processBelief(BeliefTypes eBelief, int iChange, bool bFirst = false);
	// End New Religion Stuff

#if defined(MOD_ROG_CORE)
	std::vector<int> GetTotalBuildingCount(bool bIncludePuppets = false) const;
#endif

	int GetNumFreeTechs() const;
	void SetNumFreeTechs(int iValue);
	void ChooseFreeTechs(int iNumTech, CvString strBuffer = "");

	int GetMedianTechPercentage() const;
	void ChangeMedianTechPercentage(int iValue);

	int GetNumFreeGreatPeople() const;
	void SetNumFreeGreatPeople(int iValue);
	void ChangeNumFreeGreatPeople(int iChange);

	int GetNumMayaBoosts() const;
	void SetNumMayaBoosts(int iValue);
	void ChangeNumMayaBoosts(int iChange);

	int GetNumFaithGreatPeople() const;
	void SetNumFaithGreatPeople(int iValue);
	void ChangeNumFaithGreatPeople(int iChange);

	int GetNumArchaeologyChoices() const;
	void SetNumArchaeologyChoices(int iValue);

	FaithPurchaseTypes GetFaithPurchaseType() const;
	void SetFaithPurchaseType(FaithPurchaseTypes eType);
	int GetFaithPurchaseIndex() const;
	void SetFaithPurchaseIndex(int iIndex);

	int GetNumFreePolicies() const;
	void SetNumFreePolicies(int iValue);
	void ChangeNumFreePolicies(int iChange);

	int GetNumFreePoliciesEver() const;
	void SetNumFreePoliciesEver(int iValue);
	void ChangeNumFreePoliciesEver(int iChange);

	int GetNumFreeTenets() const;
	void SetNumFreeTenets(int iValue, bool bCountAsFreePolicies);
	void ChangeNumFreeTenets(int iChange, bool bCountAsFreePolicies);

	int GetLastSliceMoved() const;
	void SetLastSliceMoved(int iValue);

	void SetEverConqueredBy(PlayerTypes ePlayer, bool bValue);
	bool IsEverConqueredBy(PlayerTypes ePlayer);

	// slewis Tutorial functions
	bool GetEverPoppedGoody(void);  // has this player ever popped a goody hut
	CvPlot* GetClosestGoodyPlot(bool bStopAfterFindingFirst = false);  // find the goody plot that has the closest unit that can reach it, null means none could be found
	bool GetPlotHasOrder(CvPlot* Plot);  // are any of the player's units directed to this plot?
	bool GetAnyUnitHasOrderToGoody(void);
	bool GetEverTrainedBuilder(void);
	// end Tutorial functions

	// International Trade
	bool IsAllowedToTradeWith(PlayerTypes eOtherPlayer);
	// end International Trade

	CvPlayerPolicies* GetPlayerPolicies() const;
	CvPlayerTraits* GetPlayerTraits() const;
	CvEconomicAI* GetEconomicAI() const;
	CvMilitaryAI* GetMilitaryAI() const;
	CvCitySpecializationAI* GetCitySpecializationAI() const;
	CvWonderProductionAI* GetWonderProductionAI() const;
	CvGrandStrategyAI* GetGrandStrategyAI() const;
	CvDiplomacyAI* GetDiplomacyAI() const;
	CvPlayerReligions* GetReligions() const;
	CvPlayerCulture* GetCulture() const;
	CvReligionAI* GetReligionAI() const;
	CvPlayerTechs* GetPlayerTechs() const;
	CvFlavorManager* GetFlavorManager() const;
	CvTacticalAI* GetTacticalAI() const;
	CvHomelandAI* GetHomelandAI() const;
	CvMinorCivAI* GetMinorCivAI() const;
	CvDealAI* GetDealAI() const;
	CvBuilderTaskingAI* GetBuilderTaskingAI() const;
	CvCityConnections* GetCityConnections() const;
	CvPlayerEspionage* GetEspionage() const;
	CvEspionageAI* GetEspionageAI() const;
	CvPlayerTrade* GetTrade() const;
	CvTradeAI* GetTradeAI() const;
	CvLeagueAI* GetLeagueAI() const;
	CvNotifications* GetNotifications() const;
#if defined(MOD_API_EXTENSIONS)
	int AddNotification(NotificationTypes eNotificationType, const char* sMessage, const char* sSummary, CvPlot* pPlot = NULL, int iGameDataIndex = -1, int iExtraGameData = -1);
	int AddNotification(NotificationTypes eNotificationType, const char* sMessage, const char* sSummary, int iGameDataIndex, int iExtraGameData = -1);
#endif
#if defined(MOD_API_PLAYER_LOGS)
	CvEventLog* GetDiplomacyLog() const;
	bool AddDiplomacyEvent(const char* sMessage, PlayerTypes eFromPlayer, int iData1 = -1, int iData2 = -1, int iData3 = -1, int iData4 = -1);

	CvEventLog* GetMilitaryLog() const;
	bool AddMilitaryEvent(const char* sMessage, CvPlot* pPlot, PlayerTypes eOtherPlayer, int iData1 = -1, int iData2 = -1, int iData3 = -1, int iData4 = -1);
#endif
	CvDiplomacyRequests* GetDiplomacyRequests() const;
	bool HasActiveDiplomacyRequests() const;

	CvTreasury* GetTreasury() const;

	int GetCityDistanceHighwaterMark() const;
	void SetCityDistanceHighwaterMark(int iNewValue);

	void SetEmbarkedGraphicOverride(CvString szGraphicName)
	{
		m_strEmbarkedGraphicOverride = szGraphicName;
	};
	const CvString& GetEmbarkedGraphicOverride() const
	{
		return m_strEmbarkedGraphicOverride;
	};

#if defined(MOD_API_EXTENSIONS)
	bool HasBelief(BeliefTypes iBeliefType) const;
	bool HasBuilding(BuildingTypes iBuildingType);
	bool HasBuildingClass(BuildingClassTypes iBuildingClassType);
	bool HasAnyWonder();
	bool HasWonder(BuildingTypes iBuildingType);
	bool IsCivilization(CivilizationTypes iCivilizationType) const;
	bool IsInEra(EraTypes iEraType) const;
	bool HasReachedEra(EraTypes iEraType) const;
	bool HasAnyNaturalWonder();
	bool HasNaturalWonder(FeatureTypes iFeatureType);
	bool HasPolicy(PolicyTypes iPolicyType) const;
	bool HasTenet(PolicyTypes iPolicyType) const;
	bool HasPolicyBranch(PolicyBranchTypes iPolicyBranchType) const;
	bool HasIdeology(PolicyBranchTypes iPolicyBranchType) const;
	bool HasProject(ProjectTypes iProjectType) const;
	bool IsAtPeace() const;
	bool IsAtPeaceAllMajors() const;
	bool IsAtPeaceAllMinors() const;
	bool IsAtPeaceWith(PlayerTypes iPlayer) const;
	bool IsAtWar() const;
	bool IsAtWarAnyMajor() const;
	bool IsAtWarAnyMinor() const;
	bool IsAtWarWith(PlayerTypes iPlayer) const;
	bool HasPantheon() const;
	bool HasAnyReligion() const;
	bool HasReligion(ReligionTypes iReligionType) const;
	bool HasEnhancedReligion() const;
	bool IsConnectedTo(PlayerTypes iPlayer);
	bool HasSpecialistSlot(SpecialistTypes iSpecialistType);
	bool HasSpecialist(SpecialistTypes iSpecialistType);
	bool HasTech(TechTypes iTechType) const;
	bool HasAnyDomesticTradeRoute() const;
	bool HasAnyInternationalTradeRoute() const;
	bool HasAnyTradeRoute() const;
	bool HasAnyTradeRouteWith(PlayerTypes iPlayer) const;
	bool HasUnit(UnitTypes iUnitType);
	bool HasUnitClass(UnitClassTypes iUnitClassType);

	bool HasTrait(TraitTypes eTrait) const;
	bool HasAnyHolyCity();
	bool HasHolyCity(ReligionTypes eReligion);
	bool HasCapturedHolyCity(ReligionTypes eReligion);
	bool HasEmbassyWith(PlayerTypes eOtherPlayer) const;
	void DoForceDefPact(PlayerTypes eOtherPlayer);

	int CountAllFeature(FeatureTypes iFeatureType);
	int CountAllWorkedFeature(FeatureTypes iFeatureType);
	int CountAllImprovement(ImprovementTypes iImprovementType);
	int CountAllWorkedImprovement(ImprovementTypes iImprovementType);
	int CountAllPlotType(PlotTypes iPlotType);
	int CountAllWorkedPlotType(PlotTypes iPlotType);
	int CountAllResource(ResourceTypes iResourceType);
	int CountAllWorkedResource(ResourceTypes iResourceType);
	int CountAllTerrain(TerrainTypes iTerrainType);
	int CountAllWorkedTerrain(TerrainTypes iTerrainType);
#endif

#if defined(MOD_ROG_CORE)
	int CountAllOriginalCapitalCity();
#endif
	int getYieldFromNonSpecialistCitizens(YieldTypes eIndex)	const;
	void changeYieldFromNonSpecialistCitizens(YieldTypes eIndex, int iChange);
	int GetYieldChangesPerReligionTimes100(YieldTypes eYield) const;
	void ChangeYieldChangesPerReligionTimes100(YieldTypes eYield, int iChange);
	void ChangeUnitClassProductionModifier(UnitClassTypes eUnitClass, int iValue);
	int GetUnitClassProductionModifier(UnitClassTypes eUnitClass) const;

	int getYieldModifierFromActiveSpies(YieldTypes eIndex) const;
	void changeYieldModifierFromActiveSpies(YieldTypes eIndex, int iChange);

	int getYieldModifierPerArtifacts(YieldTypes eIndex)	const;
	void changeYieldModifierPerArtifacts(YieldTypes eIndex, int iChange);

	int GetGreatPersonOutputModifierPerGWs(GreatPersonTypes eGreatPerson) const;
	void ChangeGreatPersonOutputModifierPerGWs(GreatPersonTypes eGreatPerson, int iChange);

#if defined(MOD_ROG_CORE)
	int GetNumWorldWonders();
#endif
#ifdef MOD_TRAIT_RELIGION_FOLLOWER_EFFECTS
	void SetPerMajorReligionFollowerYieldModifierTimes100(const YieldTypes eYieldType, const int iValue);
	void ChangePerMajorReligionFollowerYieldModifierTimes100(const YieldTypes eYieldType, const int iChange);
	int GetPerMajorReligionFollowerYieldModifierTimes100(const YieldTypes eYieldType) const;
#endif

	// for serialization
	virtual void Read(FDataStream& kStream);
	virtual void Write(FDataStream& kStream) const;

	virtual void AI_init() = 0;
	virtual void AI_reset() = 0;
	virtual void AI_doTurnPre() = 0;
	virtual void AI_doTurnPost() = 0;
	virtual void AI_doTurnUnitsPre() = 0;
	virtual void AI_doTurnUnitsPost() = 0;
	virtual void AI_updateFoundValues(bool bStartingLoc = false) = 0;
	virtual void AI_unitUpdate() = 0;
	virtual void AI_conquerCity(CvCity* pCity, PlayerTypes eOldOwner) = 0;
	virtual int AI_foundValue(int iX, int iY, int iMinUnitRange = -1, bool bStartingLoc = false) = 0;
	virtual void AI_chooseFreeGreatPerson() = 0;
	virtual void AI_chooseFreeTech() = 0;
	virtual void AI_chooseResearch() = 0;
	virtual int AI_plotTargetMissionAIs(CvPlot* pPlot, MissionAITypes eMissionAI, int iRange = 0) = 0;
	virtual void AI_launch(VictoryTypes eVictory) = 0;

	virtual OperationSlot PeekAtNextUnitToBuildForOperationSlot(int iAreaID) = 0;
	virtual OperationSlot CityCommitToBuildUnitForOperationSlot(int iAreaID, int iTurns, CvCity* pCity) = 0;
	virtual void CityUncommitToBuildUnitForOperationSlot(OperationSlot thisSlot) = 0;
	virtual void CityFinishedBuildingUnitForOperationSlot(OperationSlot thisSlot, CvUnit* pThisUnit) = 0;
	virtual int GetNumUnitsNeededToBeBuilt() = 0;
	const FAutoArchive& getSyncArchive() const;
	FAutoArchive& getSyncArchive();
	void disconnected();
	void reconnected();
	bool hasBusyUnitUpdatesRemaining() const;
	void setBusyUnitUpdatesRemaining(int iUpdateCount);
	bool hasUnitsThatNeedAIUpdate() const;

	void checkInitialTurnAIProcessed();
	void checkRunAutoMovesForEveryone();
	std::string debugDump(const FAutoVariableBase&) const;
	std::string stackTraceRemark(const FAutoVariableBase&) const;

#ifdef MOD_API_TRADE_ROUTE_YIELD_RATE
	int GetMinorsTradeRouteYieldRate(const YieldTypes eYieldType) const;
	void ChangeMinorsTradeRouteYieldRate(const YieldTypes eYieldType, const int iChange);

	int GetInternalTradeRouteDestYieldRate(const YieldTypes eYieldType) const;
	void ChangeInternalTradeRouteDestYieldRate(const YieldTypes eYieldType, const int iChange);
#endif

#if !defined(NO_ACHIEVEMENTS)
	CvPlayerAchievements& GetPlayerAchievements(){return m_kPlayerAchievements;}
#endif

	bool hasTurnTimerExpired();

#ifdef MOD_TRAITS_CAN_FOUND_MOUNTAIN_CITY
	bool GetCanFoundMountainCity() const;
#endif

#ifdef MOD_TRAITS_CAN_FOUND_COAST_CITY
	bool GetCanFoundCoastCity() const;
#endif

	int GetDishonestyCounter() const;
	void ChangeDishonestyCounter(const int iChange);
#ifdef MOD_GLOBAL_WAR_CASUALTIES
	int GetWarCasualtiesCounter() const;
	void ChangeWarCasualtiesCounter(const int iChange);
	void SetWarCasualtiesCounter(const int iValue);
	bool CheckAndUpdateWarCasualtiesCounter();

	int GetWarCasualtiesModifier() const;
	void SetWarCasualtiesModifier(const int iValue);
	void ChangeWarCasualtiesModifier(const int iChange);
#endif

#ifdef MOD_POLICIY_PUBLIC_OPTION
	int GetIdeologyPressureModifier() const;
	int GetIdeologyUnhappinessModifier() const;
	void ChangeIdeologyPressureModifier(int iChange);
	void ChangeIdeologyUnhappinessModifier(int iChange);
#endif

	std::vector<PolicyYieldInfo>& GetCityWithWorldWonderYieldModifier();
	std::vector<PolicyYieldInfo>& GetTradeRouteCityYieldModifier();
	std::vector<PolicyYieldInfo>& GetCityNumberCityYieldModifier();
	std::vector<PolicyYieldInfo>& GetHappinessYieldModifier();

	std::vector<PolicyResourceInfo>& GetCityResourcesFromPolicy();
	const std::vector<PolicyResourceInfo>& GetCityResourcesFromPolicy() const;
	int GetGlobalHappinessFromFaithPercent() const;
	void ChangeGlobalHappinessFromFaithPercent(int iChange);

	int GetHappinessInWLTKDCities() const;
	void ChangeHappinessInWLTKDCities(int iChange);

	int GetHappinessFromFaith() const;

	LuaFormulaTypes GetCaptureCityResistanceTurnsChangeFormula() const;
	void SetCaptureCityResistanceTurnsChangeFormula(LuaFormulaTypes value);

	int GetCaptureCityResistanceTurnsChange(CvCity* city, int originalResistanceTurn, bool originalOwnerLostCaptal) const;

	CvCity* CvPlayer::GetRandomCity();

	int GetRazeSpeedModifier() const;

#ifdef MOD_RESOURCE_EXTRA_BUFF
	int GetUnhappinessModFromResource() const;
	int CalculateUnhappinessModFromResource(CvResourceInfo* pInfo, int num) const;

	int GetCityConnectionTradeRouteGoldModifierFromResource() const;
	int CalculateCityConnectionTradeRouteGoldModifierFromResource(CvResourceInfo* pInfo, int num) const;

	int GetHurryModifierFromResource(HurryTypes eIndex) const;
	int CalculateGoldHurryModFromResource(CvResourceInfo* pInfo, int num) const;

	int GetGlobalYieldModifierFromResource(YieldTypes eYield) const;
	int CalculateGlobalYieldModifierFromResource(CvResourceInfo* pInfo, int num, YieldTypes eYield) const;

	// modifiers from policies ...
	int GetResourceUnhappinessModifier() const;
	void ChangeResourceUnhappinessModifier(int value);
	int GetResourceCityConnectionTradeRouteGoldModifier() const;
	void ChangeResourceCityConnectionTradeRouteGoldModifier(int value);
#endif

	int GetYieldModifierFromHappiness(CvYieldInfo* info) const;
	int GetYieldModifierFromHappinessPolicy(CvYieldInfo* info) const;

	int GetYieldModifierFromNumGreakWork(CvYieldInfo* info) const;
	int GetYieldModifierFromNumArtifact(CvYieldInfo* info) const;

#ifdef MOD_TRAITS_COMBAT_BONUS_FROM_CAPTURED_HOLY_CITY
	int GetCachedCapturedHolyCity() const;
	void UpdateCachedCapturedHolyCity();
#endif

#ifdef MOD_GLOBAL_CORRUPTION
	bool EnableCorruption() const;

	int GetCorruptionScoreModifierFromPolicy() const;
	void ChangeCorruptionScoreModifierFromPolicy(int change);
	int GetCorruptionLevelReduceByOneRC() const;
	bool IsCorruptionLevelReduceByOne() const;
	void ChangeCorruptionLevelReduceByOneRC(int change);

	int GetCorruptionPolicyCostModifier() const;
	void ChangeCorruptionPolicyCostModifier(int change);

	int GetCorruptionLevelPolicyCostModifier(CorruptionLevelTypes level) const;
	void ChangeCorruptionLevelPolicyCostModifier(CorruptionLevelTypes level, int change);
#endif

	int GetProductionNeededUnitModifier() const;
	void ChangeProductionNeededUnitModifier(int change);
	int GetProductionNeededBuildingModifier() const;
	void ChangeProductionNeededBuildingModifier(int change);
	int GetProductionNeededProjectModifier() const;
	void ChangeProductionNeededProjectModifier(int change);
	int GetProductionNeededUnitMax() const;
	void ChangeProductionNeededUnitMax(int change);
	int GetProductionNeededBuildingMax() const;
	void ChangeProductionNeededBuildingMax(int change);
	int GetProductionNeededProjectMax() const;
	void ChangeProductionNeededProjectMax(int change);

#if defined(MOD_PROMOTION_AURA_PROMOTION)
	const std::multimap<PromotionTypes, int>& GetAuraPromotionUnits() const;
	void AddUnitAuraPromotion(int iUnitID, PromotionTypes ePromotion);
	void RemoveUnitAuraPromotion(int iUnitID, PromotionTypes ePromotion);
	void RemoveAuraUnit(int iUnitID);
#endif
#if defined(MOD_TROOPS_AND_CROPS_FOR_SP)
	int GetDomainTroopsTotalTimes100(DomainTypes eDomain = DOMAIN_SEA) const;
	void ChangeDomainTroopsTotalTimes100(int iChange, DomainTypes eDomain = DOMAIN_SEA);
	void SetDomainTroopsTotalTimes100(int iValue, DomainTypes eDomain = DOMAIN_SEA);

	int GetDomainTroopsUsed(DomainTypes eDomain = DOMAIN_SEA) const;
	void ChangeDomainTroopsUsed(int iChange, DomainTypes eDomain = DOMAIN_SEA);
	void SetDomainTroopsUsed(int iValue, DomainTypes eDomain = DOMAIN_SEA);

	int GetTroopsRateTimes100() const;
	int GetDomainTroopsTotal(DomainTypes eDomain = DOMAIN_SEA) const;
	bool IsLackingTroops(DomainTypes eDomain = DOMAIN_SEA) const;
	int GetDomainTroopsActive(DomainTypes eDomain = DOMAIN_SEA) const;

	int GetNumCropsTotalTimes100() const;
	void ChangeNumCropsTotalTimes100(int change);
	int GetNumCropsUsed() const;
	void ChangeNumCropsUsed(int change);
	int GetNumCropsTotal() const;
	bool IsCanEstablishCrops() const;

	int GetNumArmeeTotalTimes100() const;
	void ChangeNumArmeeTotalTimes100(int change);
	int GetNumArmeeUsed() const;
	void ChangeNumArmeeUsed(int change);
	int GetNumArmeeTotal() const;
	bool IsCanEstablishArmee() const;
#endif

#if defined(MOD_INTERNATIONAL_IMMIGRATION_FOR_SP)
	int GetImmigrationCounter(int iIndex) const;
	void ChangeImmigrationCounter(int iIndex, int iChange);
	void SetImmigrationCounter(int iIndex, int iValue);
	int GetImmigrationRate(PlayerTypes eTargetPlayer) const;
#endif
	int GetNegateWarmongerTurn(int iIndex) const;
	void SetNegateWarmongerTurn(int iIndex, int iValue);

	void UpdateUCsFromCapturedOriginalCapitals();
	std::tr1::unordered_set<UnitTypes>& GetCanTrainUnitsFromCapturedOriginalCapitals();
	std::tr1::unordered_set<BuildingTypes>& GetCanConstructBuildingsFromCapturedOriginalCapitals();
	std::tr1::unordered_set<ImprovementTypes>& GetCanBuildImprovementsFromCapturedOriginalCapitals();

	std::tr1::unordered_set<UnitTypes>& GetUUFromDualEmpire();
	std::tr1::unordered_set<BuildingTypes>& GetUBFromDualEmpire();
	std::tr1::unordered_set<ImprovementTypes>& GetUIFromDualEmpire();

	std::tr1::unordered_set<UnitTypes>& GetUUFromExtra();
	std::tr1::unordered_set<BuildingTypes>& GetUBFromExtra();
	std::tr1::unordered_set<ImprovementTypes>& GetUIFromExtra();
	void ChangeUUFromExtra(UnitTypes eUnitTypes, bool bIsAdd = true);
	void ChangeUBFromExtra(BuildingTypes eBuildingTypes, bool bIsAdd = true);
	void ChangeUIFromExtra(ImprovementTypes eImprovementTypes, bool bIsAdd = true);

	static void GetUCTypesFromPlayer(const CvPlayer& player,
		std::tr1::unordered_set<UnitTypes>* m_sUU,
		std::tr1::unordered_set<BuildingTypes>* m_sUB,
		std::tr1::unordered_set<ImprovementTypes>* m_sUI);

	bool CanAllUc() const {
		return isHuman() && GC.getGame().isOption(GAMEOPTION_HUMAN_ALL_UC);
	}

	unsigned long long GetScienceTimes100FromFriendships() const;
	unsigned long long GetScienceTimes100FromOneFriend(PlayerTypes ePlayer) const;
	void UpdateScienceTimes100FromFriendships();

	void SetInstantResearchFromFriendlyGreatScientist(int value);
	void ChangeInstantResearchFromFriendlyGreatScientist(int change);
	int GetInstantResearchFromFriendlyGreatScientist() const;

	void DoInstantResearchFromFriendlyGreatScientist(CvUnit* pUnit, int iX, int iY);

	int GetGlobalGrowthFoodNeededModifier() const;
	void ChangeGlobalGrowthFoodNeededModifier(int iChange);

	const std::vector<int>& GetSecondCapitals() const;
	void AddSecondCapital(int iNewSecondCapitalID);
	void RemoveSecondCapital(int iSecondCapitalID);

	int GetBossLevel() const;
	void ChangeBossLevel(int iChange);
	void SetBossLevel(int iValue);

	bool IsLostUC() const;
	void SetLostUC(bool bValue);

	int GetNumGreatPersonSincePolicy() const;
	void ChangeNumGreatPersonSincePolicy(int iChange);

	int GetNumSpaceshipPartPurchased() const;
	void ChangeNumSpaceshipPartPurchased(int iChange);

	void doInstantYield(YieldTypes iYield, int iValue);
	UnitTypes GetCivUnitWithDefault(UnitClassTypes eUnitClass) const;
protected:
	class ConqueredByBoolField
	{
	public:
		enum { eCount = 4, eSize = 32 };
		DWORD m_dwBits[eCount];

		bool GetBit(const uint uiEntry) const
		{
			const uint uiOffset = uiEntry/eSize;
			return m_dwBits[uiOffset] & 1<<(uiEntry-(eSize*uiOffset));
		}
		void SetBit(const uint uiEntry)
		{
			const uint uiOffset = uiEntry/eSize;
			m_dwBits[uiOffset] |= 1<<(uiEntry-(eSize*uiOffset));
		}
		void ClearBit(const uint uiEntry)
		{
			const uint uiOffset = uiEntry/eSize;
			m_dwBits[uiOffset] &= ~(1<<(uiEntry-(eSize*uiOffset)));
		}
		void ToggleBit(const uint uiEntry)
		{
			const uint uiOffset = uiEntry/eSize;
			m_dwBits[uiOffset] ^= 1<<(uiEntry-(eSize*uiOffset));
		}
		void ClearAll()
		{
			for(uint i = 0; i <eCount; ++i)
			{
				m_dwBits[i] = 0;
			}
		}

		bool ValidateFromBoolArray(const bool* pBools, uint uiCount) const
		{
			for(uint i = 0; i < uiCount; ++i)
				if(GetBit(i) != pBools[i]) return false;

			return true;
		}

		void InitFromBoolArray(bool* pBools, uint uiCount)
		{
			for(uint i = 0; i < uiCount; ++i)
				if(GetBit(i) != pBools[i]) ToggleBit(i);
		}
	};

	FAutoArchiveClassContainer<CvPlayer> m_syncArchive;

	FAutoVariable<PlayerTypes, CvPlayer> m_eID;
	FAutoVariable<LeaderHeadTypes, CvPlayer> m_ePersonalityType;

	FAutoVariable<int, CvPlayer> m_iStartingX;
	FAutoVariable<int, CvPlayer> m_iStartingY;
	FAutoVariable<int, CvPlayer> m_iTotalPopulation;
	FAutoVariable<int, CvPlayer> m_iTotalLand;
	FAutoVariable<int, CvPlayer> m_iTotalLandScored;
	FAutoVariable<int, CvPlayer> m_iJONSCulturePerTurnForFree;
	FAutoVariable<int, CvPlayer> m_iJONSCulturePerTurnFromMinorCivs;
	FAutoVariable<int, CvPlayer> m_iJONSCultureCityModifier;
	FAutoVariable<int, CvPlayer> m_iJONSCulture;
	FAutoVariable<int, CvPlayer> m_iJONSCultureEverGenerated;
	FAutoVariable<int, CvPlayer> m_iCulturePerWonder;
	FAutoVariable<int, CvPlayer> m_iCultureWonderMultiplier;
	FAutoVariable<int, CvPlayer> m_iCulturePerTechResearched;
	int m_iFaith;
	int m_iFaithEverGenerated;
	int m_iCachedTotalFaithPerTurn;
	FAutoVariable<int, CvPlayer> m_iHappiness;
	FAutoVariable<int, CvPlayer> m_iUnhappiness;
	int m_iCachedExcessHappiness = 0;
	FAutoVariable<int, CvPlayer> m_iUprisingCounter;
	FAutoVariable<int, CvPlayer> m_iExtraHappinessPerLuxury;
	FAutoVariable<int, CvPlayer> m_iUnhappinessFromUnits;
	FAutoVariable<int, CvPlayer> m_iUnhappinessFromUnitsMod;
	FAutoVariable<int, CvPlayer> m_iUnhappinessMod;
	FAutoVariable<int, CvPlayer> m_iCityCountUnhappinessMod;
	FAutoVariable<int, CvPlayer> m_iOccupiedPopulationUnhappinessMod;
	FAutoVariable<int, CvPlayer> m_iCapitalUnhappinessMod;
	FAutoVariable<int, CvPlayer> m_iCityRevoltCounter;
	FAutoVariable<int, CvPlayer> m_iHappinessPerGarrisonedUnitCount;
	FAutoVariable<int, CvPlayer> m_iHappinessPerTradeRouteCount;
	int m_iHappinessPerXPopulation;
	int m_iHappinessFromLeagues;
	FAutoVariable<int, CvPlayer> m_iSpecialPolicyBuildingHappiness;  //unused
	FAutoVariable<int, CvPlayer> m_iWoundedUnitDamageMod;
	FAutoVariable<int, CvPlayer> m_iUnitUpgradeCostMod;
	FAutoVariable<int, CvPlayer> m_iAllowPuppetPurchase;
	FAutoVariable<int, CvPlayer> m_iBarbarianCombatBonus;
	FAutoVariable<int, CvPlayer> m_iAlwaysSeeBarbCampsCount;
	FAutoVariable<int, CvPlayer> m_iHappinessFromBuildings;
	FAutoVariable<int, CvPlayer> m_iHappinessPerCity;
	int m_iHappinessPerXPolicies;
	int m_iEspionageModifier;
	int m_iEspionageSpeedModifier;
	int m_iSpyStartingRank;
#if defined(MOD_RELIGION_CONVERSION_MODIFIERS)
	int m_iConversionModifier;
#endif
	int m_iExtraLeagueVotes;
	FAutoVariable<int, CvPlayer> m_iAdvancedStartPoints;
	FAutoVariable<int, CvPlayer> m_iAttackBonusTurns;
	int m_iCultureBonusTurns;
	int m_iTourismBonusTurns;
	FAutoVariable<int, CvPlayer> m_iGoldenAgeProgressMeter;
	FAutoVariable<int, CvPlayer> m_iGoldenAgeUnitCombatModifier;
	FAutoVariable<int, CvPlayer> m_iGoldenAgeMeterMod;
	FAutoVariable<int, CvPlayer> m_iNumGoldenAges;
	FAutoVariable<int, CvPlayer> m_iGoldenAgeTurns;
	FAutoVariable<int, CvPlayer> m_iNumUnitGoldenAges;
	FAutoVariable<int, CvPlayer> m_iStrikeTurns;
	FAutoVariable<int, CvPlayer> m_iGoldenAgeModifier;

	int m_iFreeGreatPeopleCreated;
	int m_iFreeGreatGeneralsCreated;
	int m_iFreeGreatAdmiralsCreated;
#if defined(MOD_GLOBAL_SEPARATE_GP_COUNTERS)
	int m_iFreeGreatMerchantsCreated;
	int m_iFreeGreatScientistsCreated;
	int m_iFreeGreatEngineersCreated;
#endif
	int m_iFreeGreatWritersCreated;
	int m_iFreeGreatArtistsCreated;
	int m_iFreeGreatMusiciansCreated;

	int m_iGreatPeopleCreated;
	int m_iGreatGeneralsCreated;
	int m_iGreatAdmiralsCreated;
#if defined(MOD_GLOBAL_SEPARATE_GP_COUNTERS)
	int m_iGreatMerchantsCreated;
	int m_iGreatScientistsCreated;
	int m_iGreatEngineersCreated;
#endif
	int m_iGreatWritersCreated;
	int m_iGreatArtistsCreated;
	int m_iGreatMusiciansCreated;
	std::tr1::unordered_map<UnitClassTypes, int> m_mUnitClassesFromFaith;
	FAutoVariable<int, CvPlayer> m_iGreatPeopleThresholdModifier;
	FAutoVariable<int, CvPlayer> m_iGreatGeneralsThresholdModifier;
	int m_iGreatAdmiralsThresholdModifier;
	int m_iAlwaysWeLoveKindDayInGoldenAge;
	int m_iNoResistance;
	int m_iUpgradeAllTerritory;
	int m_iNoTechForWonder;
	int m_iNoTechForProject;
	int m_iCityCaptureHealGlobal;
	int m_iOriginalCapitalCaptureTech;
	int m_iOriginalCapitalCapturePolicy;
	int m_iOriginalCapitalCaptureGreatPerson;
	int m_iGreatGeneralCombatBonus;
	FAutoVariable<int, CvPlayer> m_iAnarchyNumTurns;
	FAutoVariable<int, CvPlayer> m_iPolicyCostModifier;
	FAutoVariable<int, CvPlayer> m_iGreatPeopleRateModifier;
	FAutoVariable<int, CvPlayer> m_iGreatPeopleRateModFromBldgs;
	FAutoVariable<int, CvPlayer> m_iGreatGeneralRateModifier;
	FAutoVariable<int, CvPlayer> m_iGreatGeneralRateModFromBldgs;
	FAutoVariable<int, CvPlayer> m_iDomesticGreatGeneralRateModifier;
	FAutoVariable<int, CvPlayer> m_iDomesticGreatGeneralRateModFromBldgs;
	int m_iGreatAdmiralRateModifier;
	int m_iGreatWriterRateModifier;
	int m_iGreatArtistRateModifier;
	int m_iGreatMusicianRateModifier;
	int m_iGreatMerchantRateModifier;
	int m_iGreatScientistRateModifier;
	int m_iGreatScientistBeakerModifier;
	int m_iGreatScientistBeakerPolicyModifier;
	int m_iProductionBeakerMod;
	int m_iGreatEngineerRateModifier;
	int m_iGreatPersonExpendGold;
	FAutoVariable<int, CvPlayer> m_iMaxGlobalBuildingProductionModifier;
	FAutoVariable<int, CvPlayer> m_iMaxTeamBuildingProductionModifier;
	FAutoVariable<int, CvPlayer> m_iMaxPlayerBuildingProductionModifier;
	FAutoVariable<int, CvPlayer> m_iFreeExperience;
	FAutoVariable<int, CvPlayer> m_iFreeExperienceFromBldgs;
	FAutoVariable<int, CvPlayer> m_iFreeExperienceFromMinors;
	FAutoVariable<int, CvPlayer> m_iFeatureProductionModifier;
	FAutoVariable<int, CvPlayer> m_iWorkerSpeedModifier;
	int m_iSharedIdeologyTourismModifier;
	int m_iRemoveOceanImpassableCombatUnit;
	int m_iRemoveOceanImpassableCivilian;
#if defined(MOD_POLICY_NEW_EFFECT_FOR_SP)
	int m_iDifferentIdeologyTourismModifier;
	int m_iHappinessPerPolicy;
	int m_iWaterBuildSpeedModifier;
	std::vector<int> m_vSettlerProductionEraModifier;
	std::vector<int> m_vBuildSpeedModifier;
#endif
	int m_iNullifyInfluenceModifier;
	int m_iNumTradeRouteBonus;
	FAutoVariable<std::vector<int>, CvPlayer> m_viTradeRouteDomainExtraRange;
#if defined(MOD_BUILDING_NEW_EFFECT_FOR_SP)
	int m_iCityDefenseModifierGlobal;
	int m_iLandmarksTourismPercentGlobal;
	int m_iGreatWorksTourismModifierGlobal;
	int m_iTradeRouteSeaGoldBonusGlobal;
	int m_iTradeRouteLandGoldBonusGlobal;
	std::vector<int> m_vCityStateTradeRouteYieldModifierGlobal;
#endif
	FAutoVariable<int, CvPlayer> m_iImprovementCostModifier;
	FAutoVariable<int, CvPlayer> m_iImprovementUpgradeRateModifier;
	FAutoVariable<int, CvPlayer> m_iSpecialistProductionModifier;
	FAutoVariable<int, CvPlayer> m_iMilitaryProductionModifier;
	FAutoVariable<int, CvPlayer> m_iSpaceProductionModifier;
	FAutoVariable<int, CvPlayer> m_iCityDefenseModifier;
	FAutoVariable<int, CvPlayer> m_iUnitFortificationModifier;
	FAutoVariable<int, CvPlayer> m_iUnitBaseHealModifier;
	FAutoVariable<int, CvPlayer> m_iWonderProductionModifier;
	FAutoVariable<int, CvPlayer> m_iSettlerProductionModifier;
	FAutoVariable<int, CvPlayer> m_iCapitalSettlerProductionModifier;
	FAutoVariable<int, CvPlayer> m_iUnitProductionMaintenanceMod;
	FAutoVariable<int, CvPlayer> m_iPolicyCostBuildingModifier;
	FAutoVariable<int, CvPlayer> m_iPolicyCostMinorCivModifier;
	int m_iInfluenceSpreadModifier;
	int m_iExtraVotesPerDiplomat;
	FAutoVariable<int, CvPlayer> m_iNumNukeUnits;
	FAutoVariable<int, CvPlayer> m_iNumOutsideUnits;
	FAutoVariable<int, CvPlayer> m_iBaseFreeUnits;
	FAutoVariable<int, CvPlayer> m_iBaseFreeMilitaryUnits;
	FAutoVariable<int, CvPlayer> m_iFreeUnitsPopulationPercent;
	FAutoVariable<int, CvPlayer> m_iFreeMilitaryUnitsPopulationPercent;
	FAutoVariable<int, CvPlayer> m_iGoldPerUnit;
	FAutoVariable<int, CvPlayer> m_iGoldPerMilitaryUnit;
	FAutoVariable<int, CvPlayer> m_iRouteGoldMaintenanceMod;
	FAutoVariable<int, CvPlayer> m_iBuildingGoldMaintenanceMod;
	FAutoVariable<int, CvPlayer> m_iUnitGoldMaintenanceMod;
	FAutoVariable<int, CvPlayer> m_iUnitSupplyMod;
	FAutoVariable<int, CvPlayer> m_iExtraUnitCost;
	FAutoVariable<int, CvPlayer> m_iNumMilitaryUnits;
	FAutoVariable<int, CvPlayer> m_iHappyPerMilitaryUnit;
	FAutoVariable<int, CvPlayer> m_iHappinessToCulture;
	FAutoVariable<int, CvPlayer> m_iHappinessToScience;
	FAutoVariable<int, CvPlayer> m_iHalfSpecialistUnhappinessCount;
	FAutoVariable<int, CvPlayer> m_iHalfSpecialistFoodCount;
	FAutoVariable<int, CvPlayer> m_iMilitaryFoodProductionCount;
	int m_iGoldenAgeCultureBonusDisabledCount;
	int m_iSecondReligionPantheonCount;
	int m_iEnablesSSPartHurryCount;
	int m_iEnablesSSPartPurchaseCount;
	FAutoVariable<int, CvPlayer> m_iConscriptCount;
	FAutoVariable<int, CvPlayer> m_iMaxConscript;
	FAutoVariable<int, CvPlayer> m_iHighestUnitLevel;
	FAutoVariable<int, CvPlayer> m_iOverflowResearch;
	FAutoVariable<int, CvPlayer> m_iExpModifier;
	FAutoVariable<int, CvPlayer> m_iExpInBorderModifier;
	FAutoVariable<int, CvPlayer> m_iLevelExperienceModifier;
	FAutoVariable<int, CvPlayer> m_iMinorQuestFriendshipMod;
	FAutoVariable<int, CvPlayer> m_iMinorGoldFriendshipMod;
	FAutoVariable<int, CvPlayer> m_iMinorFriendshipMinimum;
	FAutoVariable<int, CvPlayer> m_iMinorFriendshipDecayMod;
	FAutoVariable<int, CvPlayer> m_iMinorScienceAlliesCount;
	FAutoVariable<int, CvPlayer> m_iMinorResourceBonusCount;
	int m_iAbleToAnnexCityStatesCount;
	FAutoVariable<int, CvPlayer> m_iFreeSpecialist;
	FAutoVariable<int, CvPlayer> m_iCultureBombTimer;
	FAutoVariable<int, CvPlayer> m_iConversionTimer;
	FAutoVariable<int, CvPlayer> m_iCapitalCityID;
	FAutoVariable<int, CvPlayer> m_iCitiesLost;
	FAutoVariable<int, CvPlayer> m_iMilitaryMight;
	FAutoVariable<int, CvPlayer> m_iEconomicMight;
	FAutoVariable<int, CvPlayer> m_iTurnMightRecomputed;
	FAutoVariable<int, CvPlayer> m_iNewCityExtraPopulation;
	FAutoVariable<int, CvPlayer> m_iFreeFoodBox;
	FAutoVariable<int, CvPlayer> m_iScenarioScore1;
	FAutoVariable<int, CvPlayer> m_iScenarioScore2;
	FAutoVariable<int, CvPlayer> m_iScenarioScore3;
	FAutoVariable<int, CvPlayer> m_iScenarioScore4;
	FAutoVariable<int, CvPlayer> m_iScoreFromFutureTech;
	FAutoVariable<int, CvPlayer> m_iCombatExperience;

	int m_iCSAllies;
	int m_iCSFriends;

	std::vector<int> m_piNumBuildings;

	int m_iLifetimeCombatExperience;
	int m_iNavalCombatExperience;
#if defined(MOD_UNITS_XP_TIMES_100)
	int m_iCombatExperienceTimes100;
	int m_iLifetimeCombatExperienceTimes100;
	int m_iNavalCombatExperienceTimes100;
#endif
	int m_iBorderObstacleCount;
	int m_iForbiddenForeignSpyGlobalCount;
	FAutoVariable<int, CvPlayer> m_iPopRushHurryCount;
	FAutoVariable<int, CvPlayer> m_iTotalImprovementsBuilt;
	FAutoVariable<int, CvPlayer> m_iNextOperationID;
	FAutoVariable<int, CvPlayer> m_iCostNextPolicy;
	FAutoVariable<int, CvPlayer> m_iNumBuilders;
	FAutoVariable<int, CvPlayer> m_iMaxNumBuilders;
	FAutoVariable<int, CvPlayer> m_iCityStrengthMod;
	FAutoVariable<int, CvPlayer> m_iCityGrowthMod;
	FAutoVariable<int, CvPlayer> m_iCapitalGrowthMod;
	FAutoVariable<int, CvPlayer> m_iNumPlotsBought;
	FAutoVariable<int, CvPlayer> m_iPlotGoldCostMod;

#if defined(MOD_ROG_CORE)
	int m_iGlobalCityStrengthMod;
	int m_iGlobalRangedStrikeModifier;
	int m_iResearchTotalCostModifier;
	int m_iResearchTotalCostModifierGoldenAge;
	int m_iLiberatedInfluence;
	int m_iExtraUnitPlayerInstances;
	int m_iConquestCasualtiesModifier;
	int m_iWaterTileDamageGlobal;
	int m_iWaterTileMovementReduceGlobal;
	int m_iWaterTileTurnDamageGlobal;
	int m_iLandTileDamageGlobal;
	int m_iLandTileMovementReduceGlobal;
	int m_iLandTileTurnDamageGlobal;
#endif

#if defined(MOD_TRAITS_CITY_WORKING) || defined(MOD_BUILDINGS_CITY_WORKING) || defined(MOD_POLICIES_CITY_WORKING) || defined(MOD_TECHS_CITY_WORKING)
	int m_iCityWorkingChange;
#endif
#if defined(MOD_TRAITS_CITY_AUTOMATON_WORKERS) || defined(MOD_BUILDINGS_CITY_AUTOMATON_WORKERS) || defined(MOD_POLICIES_CITY_AUTOMATON_WORKERS) || defined(MOD_TECHS_CITY_AUTOMATON_WORKERS)
	int m_iCityAutomatonWorkersChange;
#endif
	FAutoVariable<int, CvPlayer> m_iPlotCultureCostModifier;
	int m_iPlotCultureExponentModifier;
	int m_iNumCitiesPolicyCostDiscount;
	int m_iGarrisonedCityRangeStrikeModifier;
	int m_iGarrisonFreeMaintenanceCount;
	int m_iNumCitiesFreeCultureBuilding;
	int m_iNumCitiesFreeFoodBuilding;
	FAutoVariable<int, CvPlayer> m_iUnitPurchaseCostModifier;
	FAutoVariable<int, CvPlayer> m_iAllFeatureProduction;
	FAutoVariable<int, CvPlayer> m_iCityDistanceHighwaterMark; // this is used to determine camera zoom
	FAutoVariable<int, CvPlayer> m_iOriginalCapitalX;
	FAutoVariable<int, CvPlayer> m_iOriginalCapitalY;
	FAutoVariable<int, CvPlayer> m_iNumWonders;
	FAutoVariable<int, CvPlayer> m_iNumPolicies;
	FAutoVariable<int, CvPlayer> m_iNumGreatPeople;
	FAutoVariable<int, CvPlayer> m_iCityConnectionHappiness;
	FAutoVariable<int, CvPlayer> m_iHolyCityID;
	FAutoVariable<int, CvPlayer> m_iTurnsSinceSettledLastCity;
	FAutoVariable<int, CvPlayer> m_iNumNaturalWondersDiscoveredInArea;
	FAutoVariable<int, CvPlayer> m_iStrategicResourceMod;
	FAutoVariable<int, CvPlayer> m_iSpecialistCultureChange;
	FAutoVariable<int, CvPlayer> m_iGreatPeopleSpawnCounter;

	FAutoVariable<int, CvPlayer> m_iFreeTechCount;
	int m_iMedianTechPercentage;
	FAutoVariable<int, CvPlayer> m_iNumFreePolicies;
	FAutoVariable<int, CvPlayer> m_iNumFreePoliciesEver; 



	int m_iNumFreeTenets;
    int m_iMaxEffectiveCities;

	int m_iLastSliceMoved;

	FAutoVariable<uint, CvPlayer> m_uiStartTime;  // XXX save these?

	FAutoVariable<bool, CvPlayer> m_bHasBetrayedMinorCiv;
	FAutoVariable<bool, CvPlayer> m_bAlive;
	FAutoVariable<bool, CvPlayer> m_bEverAlive;
	bool m_bBeingResurrected;
	FAutoVariable<bool, CvPlayer> m_bTurnActive;
	FAutoVariable<bool, CvPlayer> m_bAutoMoves;					// Signal that we can process the auto moves when ready.
	bool						  m_bProcessedAutoMoves;		// Signal that we have processed the auto moves
	FAutoVariable<bool, CvPlayer> m_bEndTurn;					// Signal that the player has completed their turn.  The turn will still be active until the auto-moves have been processed.
	bool						  m_bDynamicTurnsSimultMode;
	FAutoVariable<bool, CvPlayer> m_bPbemNewTurn;
	FAutoVariable<bool, CvPlayer> m_bExtendedGame;
	FAutoVariable<bool, CvPlayer> m_bFoundedFirstCity;
	int m_iNumCitiesFounded;
	FAutoVariable<bool, CvPlayer> m_bStrike;
	FAutoVariable<bool, CvPlayer> m_bCramped;
	FAutoVariable<bool, CvPlayer> m_bLostCapital;
	PlayerTypes m_eConqueror;
	FAutoVariable<bool, CvPlayer> m_bHasAdoptedStateReligion;
	FAutoVariable<bool, CvPlayer> m_bAlliesGreatPersonBiasApplied;

	FAutoVariable<std::vector<int>, CvPlayer> m_aiCityYieldChange;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiCoastalCityYieldChange;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiCapitalYieldChange;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiCapitalYieldPerPopChange;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiYieldPerPopChange;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiSeaPlotYield;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiRiverPlotYield;

	FAutoVariable<std::vector<int>, CvPlayer> m_aiYieldFromProcessModifierGlobal;

	FAutoVariable<std::vector<int>, CvPlayer> m_aiCityLoveKingDayYieldMod;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiYieldRateModifier;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiCapitalYieldRateModifier;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiExtraYieldThreshold;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiSpecialistExtraYield;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiProximityToPlayer;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiResearchAgreementCounter;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiIncomingUnitTypes;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiIncomingUnitCountdowns;
	FAutoVariable<std::vector<int>, CvPlayer> m_aiMinorFriendshipAnchors; // DEPRECATED
	std::vector<int> m_aiSiphonLuxuryCount;
	std::vector<int> m_aiGreatWorkYieldChange;

	std::vector<int> m_aiGoldenAgeYieldRateModifier;

	int m_iMoveAfterCreated;

#if defined(MOD_ROG_CORE)
	std::vector<int> m_aiWorldWonderCityYieldRateModifier;

	std::vector<int> m_aiDomainFreeExperiencePerGreatWorkGlobal;
	std::vector<int> m_aiDomainFreeExperiencesPerTurnGlobal;
	std::vector<int> m_aiDomainEnemyCombatModifierGlobal;

	std::map<int, int> m_piDomainFreeExperience;
	std::tr1::unordered_map<int, int> m_piUnitTypePrmoteHealGlobal;
#endif
	std::tr1::unordered_map<int, int> m_mapEraUnitClassMaxInstances;

	FAutoVariable<std::vector<int>, CvPlayer> m_aiPolicyModifiers;

	std::vector<int> m_aiYieldFromNonSpecialistCitizens;
	std::vector<int> m_piYieldChangesPerReligion;
	std::vector<int> m_paiUnitClassProductionModifiers;

	std::vector<int> m_aiYieldModifierFromActiveSpies;
	std::vector<int> m_aiYieldModifierPerArtifacts;
	std::vector<int> m_aiGreatPersonOutputModifierPerGWs;

	std::vector<FeatureTypes> m_ownedNaturalWonders;

	typedef std::pair<uint, int> PlayerOptionEntry;
	typedef std::vector< PlayerOptionEntry > PlayerOptionsVector;
	FAutoVariable<PlayerOptionsVector, CvPlayer> m_aOptions;

	FAutoVariable<CvString, CvPlayer> m_strReligionKey;
	FAutoVariable<CvString, CvPlayer> m_strScriptData;

	CvString m_strEmbarkedGraphicOverride;

	FAutoVariable<std::vector<int>, CvPlayer> m_paiNumResourceUsed;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiNumResourceTotal;
	std::vector<int> m_paiNumResourceAvailableCache;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiResourceGiftedToMinors;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiResourceExport;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiResourceImport;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiResourceFromMinors;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiResourcesSiphoned;
#ifdef MOD_SPECIALIST_RESOURCES
	std::vector<int> m_paiResourcesFromSpecialists;
#endif
	std::vector<int> m_paiNumResourceTotalCache;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiImprovementCount;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiFreeBuildingCount;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiFreePromotionCount;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiUnitCombatProductionModifiers;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiUnitCombatFreeExperiences;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiUnitClassCount;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiUnitClassMaking;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiBuildingClassCount;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiBuildingClassMaking;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiProjectMaking;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiHurryCount;
	FAutoVariable<std::vector<int>, CvPlayer> m_paiHurryModifier;

	FAutoVariable<std::vector<bool>, CvPlayer> m_pabLoyalMember;

	FAutoVariable<std::vector<bool>, CvPlayer> m_pabGetsScienceFromPlayer;

	FAutoVariable< std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > >, CvPlayer> m_ppaaiSpecialistExtraYield;
	FAutoVariable< std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > >, CvPlayer> m_ppiYieldFromYieldGlobal;
#if defined(MOD_API_UNIFIED_YIELDS) && defined(MOD_API_PLOT_YIELDS)
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiPlotYieldChange;
#endif

#if defined(MOD_ROG_CORE)
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiImprovementYieldChange;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiSpecialistYieldModifierGlobal;
	std::vector<int> m_aiYieldFromPillageGlobal;
	std::vector<int> m_aiYieldFromPillage;
#endif

#if defined(MOD_API_UNIFIED_YIELDS)
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiFeatureYieldChange;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiResourceYieldChange;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiTerrainYieldChange;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiTradeRouteYieldChange;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiSpecialistYieldChange;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiGreatPersonExpendedYield;
	std::vector<int> m_piGoldenAgeGreatPersonRateModifier;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiUnimprovedFeatureYieldChange;
	std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > > m_ppiCityYieldFromUnimprovedFeature;
	std::vector<int> m_piYieldFromKills;
	std::vector<int> m_piYieldFromBarbarianKills;
	std::vector<int> m_piYieldChangeTradeRoute;
	std::vector<int> m_piYieldChangesNaturalWonder;
	std::vector<int> m_piYieldChangeWorldWonder;
	FAutoVariable< std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > >, CvPlayer> m_ppiBuildingClassYieldChange;
#endif
	FAutoVariable< std::vector< Firaxis::Array<int, NUM_YIELD_TYPES > >, CvPlayer> m_ppaaiImprovementYieldChange;

#ifdef MOD_API_TRADE_ROUTE_YIELD_RATE
	Firaxis::Array<int, YieldTypes::NUM_YIELD_TYPES> m_piMinorsTradeRouteYieldRate;
	Firaxis::Array<int, YieldTypes::NUM_YIELD_TYPES> m_piInternalTradeRouteDestYieldRate;
#endif

#ifdef MOD_GLOBAL_WAR_CASUALTIES
	int m_iWarCasualtiesModifier = 0;
#endif

#ifdef MOD_POLICIY_PUBLIC_OPTION
	int m_iIdeologyPressureModifier = 0;
	int m_iIdeologyUnhappinessModifier = 0;
#endif

	std::vector<PolicyYieldInfo> m_vCityWithWorldWonderYieldModifier;
	std::vector<PolicyYieldInfo> m_vTradeRouteCityYieldModifier;
	std::vector<PolicyYieldInfo> m_vCityNumberCityYieldModifier;
	std::vector<PolicyYieldInfo> m_vHappinessYieldModifier;

	std::vector<PolicyResourceInfo> m_vCityResourcesFromPolicy;
	int m_iGlobalHappinessFromFaithPercent = 0;
	int m_iHappinessInWLTKDCities = 0;

	// Obsolete: only used to read old saves
	FAutoVariable< std::vector< Firaxis::Array< int, NUM_YIELD_TYPES > >, CvPlayer> m_ppaaiBuildingClassYieldMod;

	CvUnitCycler	m_UnitCycle;	

	// slewis's tutorial variables!
	FAutoVariable<bool, CvPlayer> m_bEverPoppedGoody;
	FAutoVariable<bool, CvPlayer> m_bEverTrainedBuilder;
	// end slewis's tutorial variables

	EndTurnBlockingTypes  m_eEndTurnBlockingType;
	int  m_iEndTurnBlockingNotificationIndex;

	CLinkList<TechTypes> m_researchQueue;

	CLinkList<CvString> m_cityNames;

	FFreeListTrashArray<CvCityAI> m_cities;

	FFreeListTrashArray<CvUnit> m_units;

	FFreeListTrashArray<CvArmyAI> m_armyAIs;

	std::map<int, CvAIOperation*> m_AIOperations;
	std::map<int, CvAIOperation*>::iterator m_CurrentOperation;

	std::vector< std::pair<int, PlayerVoteTypes> > m_aVote;
	std::vector< std::pair<UnitClassTypes, int> > m_aUnitExtraCosts;

	std::vector<CvString> m_ReplayDataSets;
	std::vector< TurnData > m_ReplayDataSetValues;

	void doResearch();
	void doWarnings();

	// Danger plots!
	CvDangerPlots* m_pDangerPlots;

	// Policies
	CvPlayerPolicies* m_pPlayerPolicies;
	void processPolicies(PolicyTypes ePolicy, int iChange);

	// AI Strategies
	CvEconomicAI* m_pEconomicAI;
	CvMilitaryAI* m_pMilitaryAI;
	CvCitySpecializationAI* m_pCitySpecializationAI;
	CvWonderProductionAI* m_pWonderProductionAI;

	// AI Grand Strategies
	CvGrandStrategyAI* m_pGrandStrategyAI;

	// Diplomacy AI
	CvDiplomacyAI* m_pDiplomacyAI;

	// Religion AI
	CvPlayerReligions* m_pReligions;
	CvReligionAI* m_pReligionAI;

	// AI Tactics
	CvTacticalAI* m_pTacticalAI;
	CvHomelandAI* m_pHomelandAI;

	// Techs
	CvPlayerTechs* m_pPlayerTechs;

	// Flavor Manager
	CvFlavorManager* m_pFlavorManager;

	// Minor Civ AI
	CvMinorCivAI* m_pMinorCivAI;

	// Deal AI
	CvDealAI* m_pDealAI;

	// Builder Tasking AI
	CvBuilderTaskingAI* m_pBuilderTaskingAI;

	// City Connections
	CvCityConnections* m_pCityConnections;

	// Espionage
	CvPlayerEspionage* m_pEspionage;
	CvEspionageAI* m_pEspionageAI;

	// Trade
	CvPlayerTrade* m_pTrade;
	CvTradeAI* m_pTradeAI;

	// League AI
	CvLeagueAI* m_pLeagueAI;

	// Culture
	CvPlayerCulture* m_pCulture;

	CvNotifications* m_pNotifications;
#if defined(MOD_API_PLAYER_LOGS)
	CvEventLog* m_pDiplomacyLog;
	CvEventLog* m_pMilitaryLog;
#endif
	CvDiplomacyRequests* m_pDiplomacyRequests;

	CvPlotsVector m_aiPlots;

	// Treasury
	CvTreasury* m_pTreasury;

	CvPlayerTraits* m_pTraits;

	// human player wanted to end turn processing but hasn't received
	// the net turn complete message
	bool m_activeWaitingForEndTurnMessage;
	int  m_endTurnBusyUnitUpdatesLeft;

	int m_lastGameTurnInitialAIProcessed;

	ConqueredByBoolField m_bfEverConqueredBy;

	int m_iNumFreeGreatPeople;
	int m_iNumMayaBoosts;
	int m_iNumFaithGreatPeople;
    int m_iNumArchaeologyChoices;

	FaithPurchaseTypes m_eFaithPurchaseType;
	int m_iFaithPurchaseIndex;

	void doUpdateCacheOnTurn();

	void doArmySize();

	friend class CvPlayerManager;
	friend CvUnit* GetPlayerUnit(IDInfo& unit);
	friend const CvUnit* GetPlayerUnit(const IDInfo& unit);

	CvPlayerAchievements m_kPlayerAchievements;

	int m_iDishonestyCounter = 0;
#ifdef MOD_GLOBAL_WAR_CASUALTIES
	int m_iWarCasualtiesCounter = 0;
#endif

#ifdef MOD_TRAIT_RELIGION_FOLLOWER_EFFECTS
	int m_piPerMajorReligionFollowerYieldModifierTimes100[NUM_YIELD_TYPES];
#endif

#ifdef MOD_RESOURCE_EXTRA_BUFF
	int m_iResourceUnhappinessModifier = 0;
	int m_iResourceCityConnectionTradeRouteGoldModifier = 0;
#endif

	LuaFormulaTypes m_iCaptureCityResistanceTurnsChangeFormula = NO_LUA_FORMULA;

#ifdef MOD_TRAITS_COMBAT_BONUS_FROM_CAPTURED_HOLY_CITY
  	int m_iCachedCapturedHolyCity = 0;
#endif

#ifdef MOD_GLOBAL_CORRUPTION
	int m_iCorruptionScoreModifierFromPolicy = 0;
	int m_iCorruptionLevelReduceByOneRC = 0;
	int m_iCorruptionPolicyCostModifier = 0;

	std::vector<int> m_paiCorruptionLevelPolicyCostModifier;
#endif

	int m_iProductionNeededUnitModifier = 0;
	int m_iProductionNeededBuildingModifier = 0;
	int m_iProductionNeededProjectModifier = 0;
	int m_iProductionNeededUnitMax = -20;
	int m_iProductionNeededBuildingMax = -20;
	int m_iProductionNeededProjectMax = -20;

#if defined(MOD_PROMOTION_AURA_PROMOTION)
	std::multimap<PromotionTypes, int> m_mAuraPromotionUnits;
#endif
#if defined(MOD_TROOPS_AND_CROPS_FOR_SP)
	std::vector<int> m_aiDomainTroopsTotal;
	std::vector<int> m_aiDomainTroopsUsed;

	int m_iNumCropsTotal = 0;
	int m_iNumCropsUsed = 0;
	int m_iNumArmeeTotal = 0;
	int m_iNumArmeeUsed = 0;
#endif

#if defined(MOD_INTERNATIONAL_IMMIGRATION_FOR_SP)
	std::vector<int> m_aiImmigrationCounter;
#endif
	std::vector<int> m_aiNegateWarmongerTurn;

	std::vector<int> m_viSecondCapitals;

	std::tr1::unordered_set<UnitTypes> m_sCanTrainUnitsFromCapturedOriginalCapitals;
	std::tr1::unordered_set<BuildingTypes> m_sCanConstructBuildingsFromCapturedOriginalCapitals;
	std::tr1::unordered_set<ImprovementTypes> m_sCanBuildImprovementsFromCapturedOriginalCapitals;

	std::tr1::unordered_set<UnitTypes> m_sUUFromDualEmpire;
	std::tr1::unordered_set<BuildingTypes> m_sUBFromDualEmpire;
	std::tr1::unordered_set<ImprovementTypes> m_sUIFromDualEmpire;

	std::tr1::unordered_set<UnitTypes> m_sUUFromExtra;
	std::tr1::unordered_set<BuildingTypes> m_sUBFromExtra;
	std::tr1::unordered_set<ImprovementTypes> m_sUIFromExtra;

	bool m_bLostUC = false;

	std::tr1::array<unsigned long long, MAX_MAJOR_CIVS> m_aScienceTimes100FromMajorFriends; // length = MAX_MAJOR_CIVS

	int m_iInstantResearchFromFriendlyGreatScientist = 0;

	int m_iGlobalGrowthFoodNeededModifier = 0;

	int m_iBossLevel = 0;
	int m_iNumGreatPersonSincePolicy = 0;
	int m_iNumSpaceshipPartPurchased = 0;
};

extern bool CancelActivePlayerEndTurn();

namespace FSerialization
{
void SyncPlayer();
void ClearPlayerDeltas();
}



#endif

