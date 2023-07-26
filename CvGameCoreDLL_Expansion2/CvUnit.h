/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#pragma once

// unit.h

#ifndef CIV5_UNIT_H
#define CIV5_UNIT_H

#include "CvEnumSerialization.h"
#include "FStlContainerSerialization.h"
#include "FAutoVariable.h"
#include "FAutoVector.h"
#include "FObjectHandle.h"
#include "CvInfos.h"
#include "CvPromotionClasses.h"
#include "CvAStarNode.h"
#include "CvGameObjectExtractable.h"

#define DEFAULT_UNIT_MAP_LAYER 0

#pragma warning( disable: 4251 )		// needs to have dll-interface to be used by clients of class

class CvPlot;
class CvArea;
class CvAStarNode;
class CvArtInfoUnit;
class CvUnitEntry;
class CvUnitReligion;
class CvPathNode;

typedef MissionData MissionQueueNode;

typedef FFastSmallFixedList<MissionQueueNode, 12, true, c_eCiv5GameplayDLL> MissionQueue;

typedef FObjectHandle<CvUnit> UnitHandle;
typedef FStaticVector<CvPlot*, 20, true, c_eCiv5GameplayDLL, 0> UnitMovementQueue;

struct CvUnitCaptureDefinition
{
	PlayerTypes eOriginalOwner;		// Who first created the unit
	PlayerTypes eOldPlayer;			// The previous owner of the unit, no necessarily the original owner
	UnitTypes	eOldType;			// Previous type of the unit, the type can change when capturing
	PlayerTypes eCapturingPlayer;
	UnitTypes	eCaptureUnitType;
	int iX;
	int iY;
	bool bEmbarked;
	bool bAsIs;
#if defined(MOD_API_EXTENSIONS)
	int iScenarioData;
	CvString sName;
#if defined(MOD_GLOBAL_NO_LOST_GREATWORKS)
	CvString sGreatName;
#endif
	GreatWorkType eGreatWork;
	int iTourismBlastStrength;
#endif
	ReligionTypes eReligion;
	int iReligiousStrength;
	int iSpreadsLeft;

#ifdef MOD_BATTLE_CAPTURE_NEW_RULE
	CvUnit* pCapturingUnit = nullptr;
#endif

	CvUnitCaptureDefinition()
		: eOriginalOwner(NO_PLAYER)
		, eOldPlayer(NO_PLAYER)
		, eOldType(NO_UNIT)
		, eCapturingPlayer(NO_PLAYER)
		, eCaptureUnitType(NO_UNIT)
		, iX(-1)
		, iY(-1)
		, bEmbarked(false)
		, bAsIs(false)
#if defined(MOD_API_EXTENSIONS)
		, iScenarioData(0)
		, sName(NULL)
#if defined(MOD_GLOBAL_NO_LOST_GREATWORKS)
		, sGreatName(NULL)
#endif
		, eGreatWork(NO_GREAT_WORK)
		, iTourismBlastStrength(0)
#endif
		, eReligion(NO_RELIGION)
		, iReligiousStrength(0)
		, iSpreadsLeft(0)
#ifdef MOD_BATTLE_CAPTURE_NEW_RULE
		, pCapturingUnit(nullptr)
#endif
		{ }

	inline bool IsValid() const
	{
		return eCapturingPlayer != NO_PLAYER && eCaptureUnitType != NO_UNIT;
	}
};

class CvUnit : public CvGameObjectExtractable
{

	friend class CvUnitMission;
	friend class CvUnitCombat;

public:

	CvUnit();
	~CvUnit();

	enum
	{
	    MOVEFLAG_ATTACK						  = 0x001,
	    MOVEFLAG_DECLARE_WAR				  = 0x002,
	    MOVEFLAG_DESTINATION				  = 0x004,
	    MOVEFLAG_NOT_ATTACKING_THIS_TURN	  = 0x008,
	    MOVEFLAG_IGNORE_STACKING			  = 0x010,
	    MOVEFLAG_PRETEND_EMBARKED			  = 0x020, // so we can check movement as if the unit was embarked
	    MOVEFLAG_PRETEND_UNEMBARKED			  = 0x040, // to check movement as if the unit was unembarked
	    MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE = 0x080, // check to see if the unit can move into the tile in an embarked or unembarked state
		MOVEFLAG_STAY_ON_LAND                 = 0x100, // don't embark, even if you can
	};

	DestructionNotification<UnitHandle>& getDestructionNotification();

	void ExtractToArg(BasicArguments* arg);
	static void PushToLua(lua_State* L, BasicArguments* arg);
	static void RegistInstanceFunctions();
	static void RegistStaticFunctions();
	static CvUnit* Provide(PlayerTypes player, int id);
	

	void init(int iID, UnitTypes eUnit, UnitAITypes eUnitAI, PlayerTypes eOwner, int iX, int iY, DirectionTypes eFacingDirection, bool bNoMove, bool bSetupGraphical=true, int iMapLayer = DEFAULT_UNIT_MAP_LAYER, int iNumGoodyHutsPopped = 0);
	void initWithNameOffset(int iID, UnitTypes eUnit, int iNameOffset, UnitAITypes eUnitAI, PlayerTypes eOwner, int iX, int iY, DirectionTypes eFacingDirection, bool bNoMove, bool bSetupGraphical=true, int iMapLayer = DEFAULT_UNIT_MAP_LAYER, int iNumGoodyHutsPopped = 0);

	
	void uninit();

	void reset(int iID = 0, UnitTypes eUnit = NO_UNIT, PlayerTypes eOwner = NO_PLAYER, bool bConstructorCall = false);
	void setupGraphical();

	void initPromotions();
	void uninitInfos();  // used to uninit arrays that may be reset due to mod changes

	void convert(CvUnit* pUnit, bool bIsUpgrade);
	void kill(bool bDelay, PlayerTypes ePlayer = NO_PLAYER);

	void doTurn();

	bool isActionRecommended(int iAction);

	bool isBetterDefenderThan(const CvUnit* pDefender, const CvUnit* pAttacker) const;

	bool canDoCommand(CommandTypes eCommand, int iData1, int iData2, bool bTestVisible = false, bool bTestBusy = true) const;
	void doCommand(CommandTypes eCommand, int iData1, int iData2);

	bool IsDoingPartialMove() const;

	ActivityTypes GetActivityType() const;
#if defined(MOD_BUGFIX_UNITS_AWAKE_IN_DANGER)
	void SetActivityType(ActivityTypes eNewValue, bool bClearFortify = true);
#else
	void SetActivityType(ActivityTypes eNewValue);
#endif

	AutomateTypes GetAutomateType() const;
	bool IsAutomated() const;
	void SetAutomateType(AutomateTypes eNewValue);
	bool CanAutomate(AutomateTypes eAutomate, bool bTestVisible = false) const;
	void Automate(AutomateTypes eAutomate);

	bool ReadyToSelect() const;
	bool ReadyToMove() const;
	bool ReadyToAuto() const;
	bool IsBusy() const;
#if defined(MOD_BUGFIX_WORKERS_VISIBLE_DANGER) || defined(MOD_BUGFIX_UNITS_AWAKE_IN_DANGER)
	bool SentryAlert(bool bSameDomainOrRanged = false) const;
#else
	bool SentryAlert() const;
#endif

	bool ShowMoves() const;
	bool CanDoInterfaceMode(InterfaceModeTypes eInterfaceMode, bool bTestVisibility = false);

	bool IsDeclareWar() const;

	RouteTypes GetBestBuildRoute(CvPlot* pPlot, BuildTypes* peBestBuild = NULL) const;
	void PlayActionSound();

	bool UnitAttack(int iX, int iY, int iFlags, int iSteps=0);
	bool UnitMove(CvPlot* pPlot, bool bCombat, CvUnit* pCombatUnit, bool bEndMove = false);
	int  UnitPathTo(int iX, int iY, int iFlags, int iPrevETA = -1, bool bBuildingRoute = false); // slewis'd the iPrevETA
	bool UnitRoadTo(int iX, int iY, int iFlags);
	bool UnitBuild(BuildTypes eBuild);
	bool canEnterTerritory(TeamTypes eTeam, bool bIgnoreRightOfPassage = false, bool bIsCity = false, bool bIsDeclareWarMove = false) const;
	bool canEnterTerrain(const CvPlot& pPlot, byte bMoveFlags = 0) const;
	TeamTypes GetDeclareWarMove(const CvPlot& pPlot) const;
	PlayerTypes GetBullyMinorMove(const CvPlot* pPlot) const;
	TeamTypes GetDeclareWarRangeStrike(const CvPlot& pPlot) const;
	bool canMoveInto(const CvPlot& pPlot, byte bMoveFlags = 0) const;
	bool canMoveOrAttackInto(const CvPlot& pPlot, byte bMoveFlags = 0) const;
	bool canMoveThrough(const CvPlot& pPlot, byte bMoveFlags = 0) const;

	bool IsAngerFreeUnit() const;

	int getCombatDamage(int iStrength, int iOpponentStrength, int iCurrentDamage, bool bIncludeRand, bool bAttackerIsCity, bool bDefenderIsCity) const;
	void fightInterceptor(const CvPlot& pPlot);
	void move(CvPlot& pPlot, bool bShow);
	bool jumpToNearestValidPlot();
	bool jumpToNearestValidPlotWithinRange(int iRange);

	bool canScrap(bool bTestVisible = false) const;
	void scrap();
	int GetScrapGold() const;

	bool canGift(bool bTestVisible = false, bool bTestTransport = true) const;
	void gift(bool bTestTransport = true);

	bool CanDistanceGift(PlayerTypes eToPlayer) const;

	// Cargo/transport methods (units inside other units)
	bool canLoadUnit(const CvUnit& pUnit, const CvPlot& pPlot) const;
	void loadUnit(CvUnit& pUnit);
	bool canLoad(const CvPlot& pPlot) const;
	void load();
	bool shouldLoadOnMove(const CvPlot* pPlot) const;
	bool canUnload() const;
	void unload();
	bool canUnloadAll() const;
	void unloadAll();
	const CvUnit* getTransportUnit() const;
	CvUnit* getTransportUnit();
	bool isCargo() const;
	void setTransportUnit(CvUnit* pTransportUnit);

	SpecialUnitTypes specialCargo() const;
	DomainTypes domainCargo() const;
	int cargoSpace() const;
	void changeCargoSpace(int iChange);
	bool isFull() const;
	int cargoSpaceAvailable(SpecialUnitTypes eSpecialCargo = NO_SPECIALUNIT, DomainTypes eDomainCargo = NO_DOMAIN) const;
	bool hasCargo() const;
	bool canCargoAllMove() const;
	int getUnitAICargo(UnitAITypes eUnitAI) const;
	//

	bool canHold(const CvPlot* pPlot) const; // skip turn
	bool canSleep(const CvPlot* pPlot) const;
	bool canFortify(const CvPlot* pPlot) const;
	bool canAirPatrol(const CvPlot* pPlot) const;

	bool IsRangeAttackIgnoreLOS() const;
	int GetRangeAttackIgnoreLOSCount() const;
	void ChangeRangeAttackIgnoreLOSCount(int iChange);

	bool canSetUpForRangedAttack(const CvPlot* pPlot) const;
	bool isSetUpForRangedAttack() const;
	void setSetUpForRangedAttack(bool bValue);

	bool IsCityAttackOnly() const;
	void ChangeCityAttackOnlyCount(int iChange);

	bool IsCaptureDefeatedEnemy() const;
	void ChangeCaptureDefeatedEnemyCount(int iChange);
	int GetCaptureChance(CvUnit *pEnemy);

	bool canEmbark(const CvPlot* pPlot) const;
	bool canDisembark(const CvPlot* pPlot) const;
	bool canEmbarkOnto(const CvPlot& pOriginPlot, const CvPlot& pTargetPlot, bool bOverrideEmbarkedCheck = false, bool bIsDestination = false) const;
	bool canDisembarkOnto(const CvPlot& pOriginPlot, const CvPlot& pTargetPlot, bool bOverrideEmbarkedCheck = false, bool bIsDestination = false) const;
	bool canDisembarkOnto(const CvPlot& pTargetPlot, bool bIsDestination = false) const;
	bool CanEverEmbark() const;  // can this unit ever change into an embarked unit
	void embark(CvPlot* pPlot);
	void disembark(CvPlot* pPlot);
	inline bool isEmbarked() const
	{
		return m_bEmbarked;
	}

	void setEmbarked(bool bValue);

	bool IsHasEmbarkAbility() const;
	int GetEmbarkAbilityCount() const;
	void ChangeEmbarkAbilityCount(int iChange);

	bool canHeal(const CvPlot* pPlot, bool bTestVisible = false) const;
	bool canSentry(const CvPlot* pPlot) const;

	int healRate(const CvPlot* pPlot) const;
	int healTurns(const CvPlot* pPlot) const;
	void doHeal();
	void DoAttrition();

#if defined(MOD_GLOBAL_RELOCATION)
	const CvPlot* getAirliftFromPlot(const CvPlot* pPlot) const;
	const CvPlot* getAirliftToPlot(const CvPlot* pPlot, bool bIncludeCities) const;
#endif
	bool canAirlift(const CvPlot* pPlot) const;
	bool canAirliftAt(const CvPlot* pPlot, int iX, int iY) const;
	bool airlift(int iX, int iY);

	bool isNukeVictim(const CvPlot* pPlot, TeamTypes eTeam) const;
	bool canNuke(const CvPlot* pPlot) const;
	bool canNukeAt(const CvPlot* pPlot, int iX, int iY) const;

	bool canParadrop(const CvPlot* pPlot, bool bOnlyTestVisibility) const;
	bool canParadropAt(const CvPlot* pPlot, int iX, int iY) const;
	bool paradrop(int iX, int iY);

	bool canMakeTradeRoute(const CvPlot* pPlot) const;
	bool canMakeTradeRouteAt(const CvPlot* pPlot, int iX, int iY, TradeConnectionType eConnectionType) const;
	bool makeTradeRoute(int iX, int iY, TradeConnectionType eConnectionType);

	bool canChangeTradeUnitHomeCity(const CvPlot* pPlot) const;
	bool canChangeTradeUnitHomeCityAt(const CvPlot* pPlot, int iX, int iY) const;
	bool changeTradeUnitHomeCity(int iX, int iY);

	bool canChangeAdmiralPort(const CvPlot* pPlot) const;
	bool canChangeAdmiralPortAt(const CvPlot* pPlot, int iX, int iY) const;
	bool changeAdmiralPort(int iX, int iY);

	bool canPlunderTradeRoute(const CvPlot* pPlot, bool bOnlyTestVisibility = false) const;
	bool plunderTradeRoute();

	bool canCreateGreatWork(const CvPlot* pPlot, bool bOnlyTestVisibility = false) const;
	bool createGreatWork();

	int getNumExoticGoods() const;
	void setNumExoticGoods(int iValue);
	void changeNumExoticGoods(int iChange);
	float calculateExoticGoodsDistanceFactor(const CvPlot* pPlot);
	bool canSellExoticGoods(const CvPlot* pPlot, bool bOnlyTestVisibility = false) const;
	int getExoticGoodsGoldAmount();
	int getExoticGoodsXPAmount();
	bool sellExoticGoods();

	bool canRebase(const CvPlot* pPlot) const;
	bool canRebaseAt(const CvPlot* pPlot, int iX, int iY) const;
	bool rebase(int iX, int iY);

	bool canPillage(const CvPlot* pPlot) const;
	bool pillage();

	bool canFound(const CvPlot* pPlot, bool bTestVisible = false) const;
	bool found();

	bool canJoin(const CvPlot* pPlot, SpecialistTypes eSpecialist) const;
	bool join(SpecialistTypes eSpecialist);

	bool canConstruct(const CvPlot* pPlot, BuildingTypes eBuilding) const;
	bool construct(BuildingTypes eBuilding);

	bool CanFoundReligion(const CvPlot* pPlot) const;
	bool DoFoundReligion();

	bool CanEnhanceReligion(const CvPlot* pPlot) const;
	bool DoEnhanceReligion();

	bool CanSpreadReligion(const CvPlot* pPlot) const;
	bool DoSpreadReligion();

	bool CanRemoveHeresy(const CvPlot* pPlot) const;
	bool DoRemoveHeresy();

	int GetNumFollowersAfterSpread() const;
	ReligionTypes GetMajorityReligionAfterSpread() const;
	CvCity *GetSpreadReligionTargetCity() const;
#if defined(MOD_RELIGION_CONVERSION_MODIFIERS)
	int GetConversionStrength(const CvCity* pCity) const;
#else
	int GetConversionStrength() const;
#endif

#ifdef MOD_BALANCE_CORE
	int GetScaleAmount(int iAmountToScale) const;
#endif

	bool canDiscover(const CvPlot* pPlot, bool bTestVisible = false) const;
	int getDiscoverAmount();
	bool discover();

	bool IsCanRushBuilding(CvCity* pCity, bool bTestVisible) const;
	bool DoRushBuilding();

	int getMaxHurryProduction(CvCity* pCity) const;
	int getHurryProduction(const CvPlot* pPlot) const;
	bool canHurry(const CvPlot* pPlot, bool bTestVisible = false) const;
	bool hurry();

	int getTradeGold(const CvPlot* pPlot) const;
	int getTradeInfluence(const CvPlot* pPlot) const;
	bool canTrade(const CvPlot* pPlot, bool bTestVisible = false) const;
	bool trade();

	bool canBuyCityState(const CvPlot* pPlot, bool bTestVisible = false) const;
	bool buyCityState();

	bool canRepairFleet(const CvPlot *pPlot, bool bTestVisible = false) const;
	bool repairFleet();

	bool CanBuildSpaceship(const CvPlot* pPlot, bool bTestVisible) const;
	bool DoBuildSpaceship();

	bool CanCultureBomb(const CvPlot* pPlot, bool bTestVisible = false) const;
	bool DoCultureBomb();
	void PerformCultureBomb(int iRadius);

	bool canGoldenAge(const CvPlot* pPlot, bool bTestVisible = false) const;
	bool goldenAge();
	int GetGoldenAgeTurns() const;

	bool canGivePolicies(const CvPlot* pPlot, bool bTestVisible = false) const;
	int getGivePoliciesCulture();
	bool givePolicies();

	bool canBlastTourism(const CvPlot* pPlot, bool bTestVisible = false) const;
	int getBlastTourism();
	bool blastTourism();

	bool canBuild(const CvPlot* pPlot, BuildTypes eBuild, bool bTestVisible = false, bool bTestGold = true) const;
	bool build(BuildTypes eBuild);

	bool canPromote(PromotionTypes ePromotion, int iLeaderUnitId) const;
	void promote(PromotionTypes ePromotion, int iLeaderUnitId);

	int canLead(const CvPlot* pPlot, int iUnitId) const;
	bool lead(int iUnitId);

	int canGiveExperience(const CvPlot* pPlot) const;
	bool giveExperience();
	int getStackExperienceToGive(int iNumUnits) const;

	bool isReadyForUpgrade() const;
	bool CanUpgradeRightNow(bool bOnlyTestVisible) const;
#if defined(MOD_API_EXTENSIONS)
	bool CanUpgradeTo(UnitTypes eUpgradeUnitType, bool bOnlyTestVisible) const;
#endif
#if defined(MOD_GLOBAL_CS_UPGRADES)
	bool CanUpgradeInTerritory(bool bOnlyTestVisible) const;
#endif
	UnitTypes GetUpgradeUnitType() const;
	int upgradePrice(UnitTypes eUnit) const;
#if defined(MOD_API_EXTENSIONS)
	CvUnit* DoUpgrade(bool bFree = false);
#else
	CvUnit* DoUpgrade();
#endif
#if defined(MOD_API_EXTENSIONS)
	CvUnit* DoUpgradeTo(UnitTypes eUpgradeUnitType, bool bFree = false);
#endif

#if defined(MOD_API_UNIT_STATS)
	int getStatsTravelled() { return m_iStatsTravelled; };
	void setStatsTravelled(int iDistance) { m_iStatsTravelled = iDistance; };
	int changeStatsTravelled(int iDistance) { m_iStatsTravelled += iDistance; return m_iStatsTravelled; };
	int getStatsKilled() { return m_iStatsKilled; };
	void setStatsKilled(int iCount) { m_iStatsKilled = iCount; };
	int changeStatsKilled(int iCount) { m_iStatsKilled += iCount; return m_iStatsKilled; };
#endif

	HandicapTypes getHandicapType() const;
	CvCivilizationInfo& getCivilizationInfo() const;
	CivilizationTypes getCivilizationType() const;
	const char* getVisualCivAdjective(TeamTypes eForTeam) const;
	SpecialUnitTypes getSpecialUnitType() const;
	bool IsGreatPerson() const;
	UnitTypes getCaptureUnitType(CivilizationTypes eCivilization) const;
	UnitCombatTypes getUnitCombatType() const;
#if defined(MOD_GLOBAL_PROMOTION_CLASSES)
	UnitCombatTypes getUnitPromotionType() const;
#endif
	DomainTypes getDomainType() const;

	int flavorValue(FlavorTypes eFlavor) const;

	bool isBarbarian() const;
	bool isHuman() const;

	void DoTestBarbarianThreatToMinorsWithThisUnitsDeath(PlayerTypes eKillingPlayer);
	bool IsBarbarianUnitThreateningMinor(PlayerTypes eMinor);

	int visibilityRange() const;
#if defined(MOD_PROMOTIONS_VARIABLE_RECON)
	int reconRange() const;
#endif
	bool canChangeVisibility() const;

	int baseMoves(DomainTypes eIntoDomain = NO_DOMAIN) const;
	int maxMoves() const;
	int movesLeft() const;
	bool canMove() const;
	bool hasMoved() const;

	int GetRange() const;
	int GetNukeDamageLevel() const;

	bool canBuildRoute() const;
	BuildTypes getBuildType() const;
	int workRate(bool bMax, BuildTypes eBuild = NO_BUILD) const;

	bool isNoBadGoodies() const;
	bool isRivalTerritory() const;
	int getRivalTerritoryCount() const;
	void changeRivalTerritoryCount(int iChange);
	bool isFound() const;
	bool IsFoundAbroad() const;
	bool IsWork() const;
	bool isGoldenAge() const;
	bool isGivesPolicies() const;
	bool isBlastTourism() const;
	bool canCoexistWithEnemyUnit(TeamTypes eTeam) const;

	bool isMustSetUpToRangedAttack() const;
	int getMustSetUpToRangedAttackCount() const;
	void changeMustSetUpToRangedAttackCount(int iChange);

	bool isRangedSupportFire() const;
	int getRangedSupportFireCount() const;
	void changeRangedSupportFireCount(int iChange);

	bool isFighting() const;
	bool isAttacking() const;
	bool isDefending() const;
	bool isInCombat() const;

	int GetMaxHitPoints() const;
	int GetCurrHitPoints() const;
	bool IsHurt() const;
	bool IsDead() const;

	int GetStrategicResourceCombatPenalty() const;
	int GetUnhappinessCombatPenalty() const;

	void SetBaseCombatStrength(int iCombat);
	int GetBaseCombatStrength(bool bIgnoreEmbarked = false) const;
	int GetBaseCombatStrengthConsideringDamage() const;

	int GetGenericMaxStrengthModifier(const CvUnit* pOtherUnit, const CvPlot* pBattlePlot, bool bIgnoreUnitAdjacency) const;
	int GetMaxAttackStrength(const CvPlot* pFromPlot, const CvPlot* pToPlot, const CvUnit* pDefender) const;
	int GetMaxDefenseStrength(const CvPlot* pInPlot, const CvUnit* pAttacker, bool bFromRangedAttack = false) const;
	int GetEmbarkedUnitDefense() const;

	bool canSiege(TeamTypes eTeam) const;

	int GetBaseRangedCombatStrength() const;
#if defined(MOD_API_EXTENSIONS)
	void SetBaseRangedCombatStrength(int iStrength);
#endif 

#if defined(MOD_ROG_CORE)
	int GetDamageCombatModifier(bool bForDefenseAgainstRanged = false, int iAssumedDamage = 0) const;
#endif 


	int GetMaxRangedCombatStrength(const CvUnit* pOtherUnit, const CvCity* pCity, bool bAttacking, bool bForRangedAttack) const;

	int GetAirCombatDamage(const CvUnit* pDefender, CvCity* pCity, bool bIncludeRand, int iAssumeExtraDamage = 0) const;
	int GetRangeCombatDamage(const CvUnit* pDefender, CvCity* pCity, bool bIncludeRand, int iAssumeExtraDamage = 0) const;

	bool canAirAttack() const;
	bool canAirDefend(const CvPlot* pPlot = NULL) const;

#if defined(MOD_AI_SMART_V3)
	int EnemyScoreAtRange(const CvPlot* pPlot, bool onlyInterceptors) const;
#endif
	int GetAirStrikeDefenseDamage(const CvUnit* pAttacker, bool bIncludeRand = true) const;

	CvUnit* GetBestInterceptor(const CvPlot& pPlot, CvUnit* pkDefender = NULL, bool bLandInterceptorsOnly=false, bool bVisibleInterceptorsOnly=false) const;
	int GetInterceptorCount(const CvPlot& pPlot, CvUnit* pkDefender = NULL, bool bLandInterceptorsOnly=false, bool bVisibleInterceptorsOnly=false) const;
	int GetInterceptionDamage(const CvUnit* pAttacker, bool bIncludeRand = true) const;
#if defined(MOD_GLOBAL_PARATROOPS_AA_DAMAGE)
	int GetParadropInterceptionDamage(const CvUnit* pAttacker, bool bIncludeRand = true) const;
#endif

	int GetCombatLimit() const;
	int GetRangedCombatLimit() const;

	bool isWaiting() const;
	bool isFortifyable(bool bCanWaitForNextTurn = false) const;
	bool IsEverFortifyable() const;
	int fortifyModifier() const;

	int experienceNeeded() const;
	int attackXPValue() const;
	int defenseXPValue() const;
#ifdef MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE
	int ExtraAttackXPValue() const;
	int ExtraDefenseXPValue() const;
#endif
	int maxXPValue() const;

	int firstStrikes() const;
	int chanceFirstStrikes() const;
	int maxFirstStrikes() const;
	bool isRanged() const;

	bool immuneToFirstStrikes() const;
	bool ignoreBuildingDefense() const;

	bool ignoreTerrainCost() const;
	int getIgnoreTerrainCostCount() const;
	void changeIgnoreTerrainCostCount(int iValue);

#if defined(MOD_API_PLOT_BASED_DAMAGE)
	bool ignoreTerrainDamage() const;
	int getIgnoreTerrainDamageCount() const;
	void changeIgnoreTerrainDamageCount(int iValue);

	bool ignoreFeatureDamage() const;
	int getIgnoreFeatureDamageCount() const;
	void changeIgnoreFeatureDamageCount(int iValue);

	bool extraTerrainDamage() const;
	int getExtraTerrainDamageCount() const;
	void changeExtraTerrainDamageCount(int iValue);

	bool extraFeatureDamage() const;
	int getExtraFeatureDamageCount() const;
	void changeExtraFeatureDamageCount(int iValue);
#endif

#if defined(MOD_PROMOTIONS_IMPROVEMENT_BONUS)
	int GetNearbyImprovementCombatBonus() const;
	void SetNearbyImprovementCombatBonus(int iCombatBonus);
	int GetNearbyImprovementBonusRange() const;
	void SetNearbyImprovementBonusRange(int iBonusRange);
	ImprovementTypes GetCombatBonusImprovement() const;
	void SetCombatBonusImprovement(ImprovementTypes eImprovement);
#endif

#if defined(MOD_PROMOTIONS_ALLYCITYSTATE_BONUS)
	int GetAllyCityStateCombatModifier() const;
	void SetAllyCityStateCombatModifier(int iCombatBonus);
	int GetAllyCityStateCombatModifierMax() const;
	void SetAllyCityStateCombatModifierMax(int iCombatBonusMax);
	int GetStrengthModifierFromAlly() const;
#endif

#if defined(MOD_PROMOTIONS_EXTRARES_BONUS)
	ResourceTypes GetExtraResourceType() const;
	void SetExtraResourceType(ResourceTypes m_eResourceType);
	int GetExtraResourceCombatModifier() const;
	void SetExtraResourceCombatModifier(int iCombatBonus);
	int GetExtraResourceCombatModifierMax() const;
	void SetExtraResourceCombatModifierMax(int iCombatBonusMax);
	int GetStrengthModifierFromExtraResource() const;
	int GetExtraHappinessCombatModifier() const;
	void SetExtraHappinessCombatModifier(int iCombatBonus);
	int GetExtraHappinessCombatModifierMax() const;
	void SetExtraHappinessCombatModifierMax(int iCombatBonusMax);
	int GetStrengthModifierFromExtraHappiness() const;
#endif

#if defined(MOD_ROG_CORE)
	int getNearbyUnitClassBonus() const;
	void SetNearbyUnitClassBonus(int iCombatBonus);
	int getNearbyUnitClassBonusRange() const;
	void SetNearbyUnitClassBonusRange(int iBonusRange);
	UnitClassTypes getCombatBonusFromNearbyUnitClass() const;
	void SetCombatBonusFromNearbyUnitClass(UnitClassTypes eUnitClass);
#endif

#if defined(MOD_PROMOTIONS_CROSS_MOUNTAINS)
	bool canCrossMountains() const;
	int getCanCrossMountainsCount() const;
	void changeCanCrossMountainsCount(int iValue);
#endif

#if defined(MOD_PROMOTIONS_CROSS_OCEANS)
	bool canCrossOceans() const;
	int getCanCrossOceansCount() const;
	void changeCanCrossOceansCount(int iValue);
#endif

#if defined(MOD_PROMOTIONS_CROSS_ICE)
	bool canCrossIce() const;
	int getCanCrossIceCount() const;
	void changeCanCrossIceCount(int iValue);
#endif

#if defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
	bool isGGFromBarbarians() const;
	int getGGFromBarbariansCount() const;
	void changeGGFromBarbariansCount(int iValue);
#endif

	bool IsRoughTerrainEndsTurn() const;
	int GetRoughTerrainEndsTurnCount() const;
	void ChangeRoughTerrainEndsTurnCount(int iValue);

	bool IsHoveringUnit() const;
	int GetHoveringUnitCount() const;
	void ChangeHoveringUnitCount(int iValue);

	bool flatMovementCost() const;
	int getFlatMovementCostCount() const;
	void changeFlatMovementCostCount(int iValue);

	bool canMoveImpassable() const;
	int getCanMoveImpassableCount() const;
	void changeCanMoveImpassableCount(int iValue);

	bool canMoveAllTerrain() const;
	void changeCanMoveAllTerrainCount(int iValue);
	int getCanMoveAllTerrainCount() const;

	bool canMoveAfterAttacking() const;
	void changeCanMoveAfterAttackingCount(int iValue);
	int getCanMoveAfterAttackingCount() const;

	bool hasFreePillageMove() const;
	void changeFreePillageMoveCount(int iValue);
	int getFreePillageMoveCount() const;

	bool hasHealOnPillage() const;
	void changeHealOnPillageCount(int iValue);
	int getHealOnPillageCount() const;

	bool isHiddenNationality() const;
	void changeHiddenNationalityCount(int iValue);
	int getHiddenNationalityCount() const;

	bool isNoRevealMap() const;
	void changeNoRevealMapCount(int iValue);
	int getNoRevealMapCount() const;

	bool isOnlyDefensive() const;
	int getOnlyDefensiveCount() const;
	void changeOnlyDefensiveCount(int iValue);

	bool noDefensiveBonus() const;
	int getNoDefensiveBonusCount() const;
	void changeNoDefensiveBonusCount(int iValue);

	bool isNoCapture() const;
	int getNoCaptureCount() const;
	void changeNoCaptureCount(int iValue);

	bool isNeverInvisible() const;
	bool isInvisible(TeamTypes eTeam, bool bDebug, bool bCheckCargo = true) const;

	bool isNukeImmune() const;
	void changeNukeImmuneCount(int iValue);
	int getNukeImmuneCount() const;

	int maxInterceptionProbability() const;
	int currInterceptionProbability() const;
	int evasionProbability() const;
	int withdrawalProbability() const;

	int GetNumEnemyUnitsAdjacent(const CvUnit* pUnitToExclude = NULL) const;
	bool IsEnemyCityAdjacent() const;
	bool IsEnemyCityAdjacent(const CvCity* pSpecifyCity) const;
	int GetNumSpecificEnemyUnitsAdjacent(const CvUnit* pUnitToExclude = NULL, const CvUnit* pUnitCompare = NULL) const;
	bool IsFriendlyUnitAdjacent(bool bCombatUnit) const;

	int GetAdjacentModifier() const;
	void ChangeAdjacentModifier(int iValue);
	int GetRangedAttackModifier() const;
	void ChangeRangedAttackModifier(int iValue);
	int GetInterceptionCombatModifier() const;
	void ChangeInterceptionCombatModifier(int iValue);
	int GetInterceptionDefenseDamageModifier() const;
	void ChangeInterceptionDefenseDamageModifier(int iValue);
	int GetAirSweepCombatModifier() const;
	void ChangeAirSweepCombatModifier(int iValue);
	int getAttackModifier() const;
	void changeAttackModifier(int iValue);
	int getDefenseModifier() const;
	void changeDefenseModifier(int iValue);

	int cityAttackModifier() const;
	int cityDefenseModifier() const;
	int rangedDefenseModifier() const;
	int hillsAttackModifier() const;
	int hillsDefenseModifier() const;
	int openAttackModifier() const;
	int openRangedAttackModifier() const;
	int roughAttackModifier() const;
	int roughRangedAttackModifier() const;
	int attackFortifiedModifier() const;
	int attackWoundedModifier() const;
	int openDefenseModifier() const;
	int roughDefenseModifier() const;
	int terrainAttackModifier(TerrainTypes eTerrain) const;
	int terrainDefenseModifier(TerrainTypes eTerrain) const;
	int featureAttackModifier(FeatureTypes eFeature) const;
	int featureDefenseModifier(FeatureTypes eFeature) const;
	int unitClassAttackModifier(UnitClassTypes eUnitClass) const;
	int unitClassDefenseModifier(UnitClassTypes eUnitClass) const;
	int unitCombatModifier(UnitCombatTypes eUnitCombat) const;
	int domainModifier(DomainTypes eDomain) const;

	int domainAttack(DomainTypes eDomain) const;
	int domainDefense(DomainTypes eDomain) const;

#if defined(MOD_API_PROMOTION_TO_PROMOTION_MODIFIERS)
	int otherPromotionModifier(PromotionTypes other) const;
	int otherPromotionAttackModifier(PromotionTypes other) const;
	int otherPromotionDefenseModifier(PromotionTypes other) const;

	int otherPromotionModifierByUnit(const CvUnit* otherUnit) const;
	int otherPromotionAttackModifierByUnit(const CvUnit* otherUnit) const;
	int otherPromotionDefenseModifierByUnit(const CvUnit* otherUnit) const;
#endif

	bool IsHasNoValidMove() const;

	inline int GetID() const
	{
		return m_iID;
	}
	int getIndex() const;
	IDInfo GetIDInfo() const;
	void SetID(int iID);

	int getHotKeyNumber();
	void setHotKeyNumber(int iNewValue);

	inline int getX() const
	{
		return m_iX.get();
	}

	inline int getY() const
	{
		return m_iY.get();
	}

	void setXY(int iX, int iY, bool bGroup = false, bool bUpdate = true, bool bShow = false, bool bCheckPlotVisible = false, bool bNoMove = false);
	bool at(int iX, int iY) const;
	bool atPlot(const CvPlot& plot) const;
	CvPlot* plot() const;
	int getArea() const;
	CvArea* area() const;
	bool onMap() const;

	int getLastMoveTurn() const;
	void setLastMoveTurn(int iNewValue);

	int GetCycleOrder() const;
	void SetCycleOrder(int iNewValue);

	int GetDeployFromOperationTurn() const
	{
		return m_iDeployFromOperationTurn;
	};
	void SetDeployFromOperationTurn(int iTurn)
	{
		m_iDeployFromOperationTurn = iTurn;
	};

	bool IsRecon() const;
	int GetReconCount() const;
	void ChangeReconCount(int iChange);

	CvPlot* getReconPlot() const;
	void setReconPlot(CvPlot* pNewValue);

	int getGameTurnCreated() const;
	void setGameTurnCreated(int iNewValue);

	int getDamage() const;
#if defined(MOD_API_UNIT_STATS)
	int setDamage(int iNewValue, PlayerTypes ePlayer = NO_PLAYER, int iUnit = -1, float fAdditionalTextDelay = 0.0f, const CvString* pAppendText = NULL);
	int changeDamage(int iChange, PlayerTypes ePlayer = NO_PLAYER, int iUnit = -1, float fAdditionalTextDelay = 0.0f, const CvString* pAppendText = NULL);
#else
	int setDamage(int iNewValue, PlayerTypes ePlayer = NO_PLAYER, float fAdditionalTextDelay = 0.0f, const CvString* pAppendText = NULL);
	int changeDamage(int iChange, PlayerTypes ePlayer = NO_PLAYER, float fAdditionalTextDelay = 0.0f, const CvString* pAppendText = NULL);
#endif
#if defined(SHOW_PLOT_POPUP)
	void ShowDamageDeltaText(int iDelta, CvPlot* pkPlot, float fAdditionalTextDelay = 0.0f, const CvString* pAppendText = NULL);
#else
	static void ShowDamageDeltaText(int iDelta, CvPlot* pkPlot, float fAdditionalTextDelay = 0.0f, const CvString* pAppendText = NULL);
#endif

	int getMoves() const;
	void setMoves(int iNewValue);
	void changeMoves(int iChange);
	void finishMoves();

	bool IsImmobile() const;
	void SetImmobile(bool bValue);

	bool IsInFriendlyTerritory() const;
	bool IsUnderEnemyRangedAttack() const;

#if defined(MOD_UNITS_XP_TIMES_100)
	int getExperienceTimes100() const;
	void setExperienceTimes100(int iNewValueTimes100, int iMax = -1);
	void changeExperienceTimes100(int iChangeTimes100, int iMax = -1, bool bFromCombat = false, bool bInBorders = false, bool bUpdateGlobal = false);
#else
	int getExperience() const;
	void setExperience(int iNewValue, int iMax = -1);
	void changeExperience(int iChange, int iMax = -1, bool bFromCombat = false, bool bInBorders = false, bool bUpdateGlobal = false);
#endif

	int getLevel() const;
	void setLevel(int iNewValue);
	void changeLevel(int iChange);

	int getCargo() const;
	void changeCargo(int iChange);

	CvPlot* getAttackPlot() const;
	void setAttackPlot(const CvPlot* pNewValue, bool bAirCombat);
	bool isAirCombat() const;

	int getCombatTimer() const;
	void setCombatTimer(int iNewValue);
	void changeCombatTimer(int iChange);

	int getCombatFirstStrikes() const;
	void setCombatFirstStrikes(int iNewValue);
	void changeCombatFirstStrikes(int iChange);

	bool IsGarrisoned(void) const;
	CvCity* GetGarrisonedCity();
	int getFortifyTurns() const;
	void setFortifyTurns(int iNewValue);
	void changeFortifyTurns(int iChange);
	bool IsFortifiedThisTurn() const;
	void SetFortifiedThisTurn(bool bValue);

	int getBlitzCount() const;
	bool isBlitz() const;
	void changeBlitzCount(int iChange);

	void DoAdjacentPlotDamage(CvPlot* pWhere, int iValue);

	void MoveToEnemyPlotDamage(CvPlot* pWhere);

#if defined(MOD_ROG_CORE)
	int getMeleeDefenseModifier() const;
	void changeMeleeDefenseModifier(int iValue);

	int attackFullyHealedModifier() const;
	int attackAbove50HealthModifier() const;
	int attackBelow50HealthModifier() const;

	int getForcedDamageValue();
	void ChangeForcedDamageValue(int iChange);

	int getChangeDamageValue();
	void ChangeChangeDamageValue(int iChange);

	int getExtraAttackFullyHealedMod() const;
	void changeExtraAttackFullyHealedMod(int iChange);

	int getExtraAttackAboveHealthMod() const;
	void changeExtraAttackAboveHealthMod(int iChange);

	int getExtraAttackBelowHealthMod() const;
	void changeExtraAttackBelowHealthMod(int iChange);
#endif

#if defined(MOD_ROG_CORE)
	int getAoEDamageOnMove() const;
	void changeAoEDamageOnMove(int iChange);
	bool IsStrongerDamaged() const;
	void ChangeIsStrongerDamaged(int iChange);
	bool IsFightWellDamaged() const;
	void ChangeIsFightWellDamaged(int iChange);
#endif

	bool IsImmueMeleeAttack() const;
	void ChangeImmueMeleeAttackCount(int iChange);

#if defined(MOD_ROG_CORE)
	int getHPHealedIfDefeatEnemyGlobal() const;
	void changeHPHealedIfDefeatEnemyGlobal(int iValue);

	int getNumOriginalCapitalAttackMod() const;
	void changeNumOriginalCapitalAttackMod(int iValue);

	int getNumOriginalCapitalDefenseMod() const;
	void changeNumOriginalCapitalDefenseMod(int iValue);
#endif


#if defined(MOD_ROG_CORE)
	int getOnCapitalLandAttackMod() const;
	void changeOnCapitalLandAttackMod(int iValue);

	int getOutsideCapitalLandAttackMod() const;
	void changeOutsideCapitalLandAttackMod(int iValue);

	int getOnCapitalLandDefenseMod() const;
	void changeOnCapitalLandDefenseMod(int iValue);

	int getOutsideCapitalLandDefenseMod() const;
	void changeOutsideCapitalLandDefenseMod(int iValue);
#endif

	int GetAttackInflictDamageChange() const;
	int GetAttackInflictDamageChangeMaxHPPercent() const;
	void ChangeAttackInflictDamageChange(int iChange);
	void ChangeAttackInflictDamageChangeMaxHPPercent(int iChange);

	int GetDefenseInflictDamageChange() const;
	int GetDefenseInflictDamageChangeMaxHPPercent() const;
	void ChangeDefenseInflictDamageChange(int iChange);
	void ChangeDefenseInflictDamageChangeMaxHPPercent(int iChange);

	int GetSiegeInflictDamageChange() const;
	int GetSiegeInflictDamageChangeMaxHPPercent() const;
	void ChangeSiegeInflictDamageChange(int iChange);
	void ChangeSiegeInflictDamageChangeMaxHPPercent(int iChange);

	int GetHeavyChargeAddMoves() const;
	int GetHeavyChargeExtraDamage() const;
	int GetHeavyChargeCollateralFixed() const;
	int GetHeavyChargeCollateralPercent() const;
	void ChangeHeavyChargeAddMoves(int iChange);
	void ChangeHeavyChargeExtraDamage(int iChange);
	void ChangeHeavyChargeCollateralFixed(int iChange);
	void ChangeHeavyChargeCollateralPercent(int iChange);

	int getAmphibCount() const;
	bool isAmphib() const;
	void changeAmphibCount(int iChange);

	int getRiverCrossingNoPenaltyCount() const;
	bool isRiverCrossingNoPenalty() const;
	void changeRiverCrossingNoPenaltyCount(int iChange);

	int getEnemyRouteCount() const;
	bool isEnemyRoute() const;
	void changeEnemyRouteCount(int iChange);

	int getAlwaysHealCount() const;
	bool isAlwaysHeal() const;
	void changeAlwaysHealCount(int iChange);

	int getHealOutsideFriendlyCount() const;
	bool isHealOutsideFriendly() const;
	void changeHealOutsideFriendlyCount(int iChange);

	int getHillsDoubleMoveCount() const;
	bool isHillsDoubleMove() const;
	void changeHillsDoubleMoveCount(int iChange);

	int getImmuneToFirstStrikesCount() const;
	void changeImmuneToFirstStrikesCount(int iChange);

	int getExtraVisibilityRange() const;
	void changeExtraVisibilityRange(int iChange);

#if defined(MOD_PROMOTIONS_VARIABLE_RECON)
	int getExtraReconRange() const;
	void changeExtraReconRange(int iChange);
#endif

	int getExtraMoves() const;
	void changeExtraMoves(int iChange);

	int getExtraMoveDiscount() const;
	void changeExtraMoveDiscount(int iChange);

	int getExtraNavalMoves() const;
	void changeExtraNavalMoves(int iChange);

	int getHPHealedIfDefeatEnemy() const;
	void changeHPHealedIfDefeatEnemy(int iValue);
	bool IsHealIfDefeatExcludeBarbarians() const;
	int GetHealIfDefeatExcludeBarbariansCount() const;
	void ChangeHealIfDefeatExcludeBarbariansCount(int iValue);

	int GetGoldenAgeValueFromKills() const;
	void ChangeGoldenAgeValueFromKills(int iValue);

	int getExtraRange() const;
	void changeExtraRange(int iChange);

	int getExtraIntercept() const;
	void changeExtraIntercept(int iChange);

	int getExtraEvasion() const;
	void changeExtraEvasion(int iChange);

	int getExtraFirstStrikes() const;
	void changeExtraFirstStrikes(int iChange);

	int getExtraChanceFirstStrikes() const;
	void changeExtraChanceFirstStrikes(int iChange);

	int getExtraWithdrawal() const;
	void changeExtraWithdrawal(int iChange);

	int getExtraEnemyHeal() const;
	void changeExtraEnemyHeal(int iChange);

	int getExtraNeutralHeal() const;
	void changeExtraNeutralHeal(int iChange);

	int getExtraFriendlyHeal() const;
	void changeExtraFriendlyHeal(int iChange);

	int getSameTileHeal() const;
	void changeSameTileHeal(int iChange);

	int getAdjacentTileHeal() const;
	void changeAdjacentTileHeal(int iChange);

	int getEnemyDamageChance() const;
	void changeEnemyDamageChance(int iChange);

	int getNeutralDamageChance() const;
	void changeNeutralDamageChance(int iChange);

	int getEnemyDamage() const;
	void changeEnemyDamage(int iChange);

	int getNeutralDamage() const;
	void changeNeutralDamage(int iChange);

	int getNearbyEnemyCombatMod() const;
	void changeNearbyEnemyCombatMod(int iChange);

	int getNearbyEnemyCombatRange() const;
	void changeNearbyEnemyCombatRange(int iChange);

	int getExtraCombatPercent() const;
	void changeExtraCombatPercent(int iChange);

	int getExtraCityAttackPercent() const;
	void changeExtraCityAttackPercent(int iChange);

	int getExtraCityDefensePercent() const;
	void changeExtraCityDefensePercent(int iChange);

	int getExtraRangedDefenseModifier() const;
	void changeExtraRangedDefenseModifier(int iChange);

	int getExtraHillsAttackPercent() const;
	void changeExtraHillsAttackPercent(int iChange);

	int getExtraHillsDefensePercent() const;
	void changeExtraHillsDefensePercent(int iChange);

	int getExtraOpenAttackPercent() const;
	void changeExtraOpenAttackPercent(int iChange);

	int getExtraOpenRangedAttackMod() const;
	void changeExtraOpenRangedAttackMod(int iChange);

	int getExtraRoughAttackPercent() const;
	void changeExtraRoughAttackPercent(int iChange);

	int getExtraRoughRangedAttackMod() const;
	void changeExtraRoughRangedAttackMod(int iChange);

	int getExtraAttackFortifiedMod() const;
	void changeExtraAttackFortifiedMod(int iChange);

	int getExtraAttackWoundedMod() const;
	void changeExtraAttackWoundedMod(int iChange);

	int GetFlankAttackModifier() const;
	void ChangeFlankAttackModifier(int iChange);

	int getExtraOpenDefensePercent() const;
	void changeExtraOpenDefensePercent(int iChange);

	int getExtraRoughDefensePercent() const;
	void changeExtraRoughDefensePercent(int iChange);

	void changeExtraAttacks(int iChange);

	// Citadel
	bool IsNearEnemyCitadel(int& iCitadelDamage);

#if defined(MOD_ROG_CORE)
	bool IsNearOurCitadel(int& iCitadelHeal);
#endif

	// Great General Stuff
#if defined(MOD_PROMOTIONS_AURA_CHANGE)
	bool IsNearGreatGeneral(int& iAuraEffectChange) const;
#else
	bool IsNearGreatGeneral() const;
#endif
	bool IsStackedGreatGeneral() const;
	int GetGreatGeneralStackMovement() const;
	int GetReverseGreatGeneralModifier() const;
	int GetNearbyImprovementModifier() const;
#if defined(MOD_PROMOTIONS_IMPROVEMENT_BONUS)
	int GetNearbyImprovementModifierFromTraits() const;
	int GetNearbyImprovementModifierFromPromotions() const;
	int GetNearbyImprovementModifier(ImprovementTypes eBonusImprovement, int iImprovementRange, int iImprovementModifier) const;
#endif

#if defined(MOD_ROG_CORE)
	int GetNearbyUnitClassModifierFromUnitClass() const;
	int GetNearbyUnitClassModifier(UnitClassTypes eUnitClass, int iUnitClassRange, int iUnitClassModifierconst) const;
#endif

	bool IsGreatGeneral() const;
	int GetGreatGeneralCount() const;
	void ChangeGreatGeneralCount(int iChange);
	bool IsGreatAdmiral() const;
	int GetGreatAdmiralCount() const;
	void ChangeGreatAdmiralCount(int iChange);

#if defined(MOD_PROMOTIONS_AURA_CHANGE)
	int GetAuraRangeChange() const;
	void ChangeAuraRangeChange(int iChange);
	int GetAuraEffectChange() const;
	void ChangeAuraEffectChange(int iChange);
#endif

	int getGreatGeneralModifier() const;
	void changeGreatGeneralModifier(int iChange);

	bool IsGreatGeneralReceivesMovement() const;
	void ChangeGreatGeneralReceivesMovementCount(int iChange);
	int GetGreatGeneralCombatModifier() const;
	void ChangeGreatGeneralCombatModifier(int iChange);

	bool IsIgnoreGreatGeneralBenefit() const;
	void ChangeIgnoreGreatGeneralBenefitCount(int iChange);
	// END Great General Stuff

#if defined(MOD_UNITS_NO_SUPPLY)
	bool isNoSupply() const;
	void changeNoSupply(int iChange);
#endif

#if defined(MOD_UNITS_MAX_HP)
	int getMaxHitPointsBase() const;
	void setMaxHitPointsBase(int iMaxHitPoints);
	void changeMaxHitPointsBase(int iChange);
	
	int getMaxHitPointsChange() const;
	void changeMaxHitPointsChange(int iChange);
	int getMaxHitPointsModifier() const;
	void changeMaxHitPointsModifier(int iChange);
#endif

	bool IsIgnoreZOC() const;
	void ChangeIgnoreZOCCount(int iChange);

	bool IsSapper() const;
	void ChangeSapperCount(int iChange);
	bool IsSappingCity(const CvCity* pTargetCity) const;
	bool IsNearSapper(const CvCity* pTargetCity) const;

	bool IsCanHeavyCharge() const;
	void ChangeCanHeavyChargeCount(int iChange);

	int getFriendlyLandsModifier() const;
	void changeFriendlyLandsModifier(int iChange);

	int getFriendlyLandsAttackModifier() const;
	void changeFriendlyLandsAttackModifier(int iChange);

	int getOutsideFriendlyLandsModifier() const;
	void changeOutsideFriendlyLandsModifier(int iChange);

	int getPillageChange() const;
	void changePillageChange(int iChange);

	int getUpgradeDiscount() const;
	void changeUpgradeDiscount(int iChange);

	int getExperiencePercent() const;
	void changeExperiencePercent(int iChange);

	int getKamikazePercent() const;
	void changeKamikazePercent(int iChange);

	DirectionTypes getFacingDirection(bool checkLineOfSightProperty) const;
	void setFacingDirection(DirectionTypes facingDirection);
	void rotateFacingDirectionClockwise();
	void rotateFacingDirectionCounterClockwise();

	bool isSuicide() const;
	bool isTrade() const;

	int getDropRange() const;
	void changeDropRange(int iChange);

	bool isOutOfAttacks() const;
	void setMadeAttack(bool bNewValue);

	int GetNumInterceptions() const;
	void ChangeNumInterceptions(int iChange);

	bool isOutOfInterceptions() const;
	int getMadeInterceptionCount() const;
	void setMadeInterception(bool bNewValue);

	bool TurnProcessed() const;
	void SetTurnProcessed(bool bValue);

#if defined(MOD_API_UNIT_CANNOT_BE_RANGED_ATTACKED)
	bool IsCannotBeRangedAttacked() const;
	void SetCannotBeRangedAttacked(bool bNewValue);
#endif

	bool isPromotionReady() const;
	void setPromotionReady(bool bNewValue);
	void testPromotionReady();

	bool isDelayedDeath() const;
	bool isDelayedDeathExported() const;
	void startDelayedDeath();
	bool doDelayedDeath();

	bool isCombatFocus() const;

	bool isInfoBarDirty() const;
	void setInfoBarDirty(bool bNewValue);

	bool IsNotConverting() const;
	void SetNotConverting(bool bNewValue);

	inline PlayerTypes getOwner() const
	{
		return m_eOwner.get();
	}
	PlayerTypes getVisualOwner(TeamTypes eForTeam = NO_TEAM) const;
	PlayerTypes getCombatOwner(TeamTypes eForTeam, const CvPlot& pPlot) const;
	TeamTypes getTeam() const;

	PlayerTypes GetOriginalOwner() const;
	void SetOriginalOwner(PlayerTypes ePlayer);

	PlayerTypes getCapturingPlayer() const;
	void setCapturingPlayer(PlayerTypes eNewValue);
	bool IsCapturedAsIs() const;
	void SetCapturedAsIs(bool bSetValue);

#ifdef MOD_BATTLE_CAPTURE_NEW_RULE
	CvUnit* getCapturingUnit() const;
	void setCapturingUnit(CvUnit* unit);
#endif

	const UnitTypes getUnitType() const;
	CvUnitEntry& getUnitInfo() const;
	UnitClassTypes getUnitClassType() const;

	const UnitTypes getLeaderUnitType() const;
	void setLeaderUnitType(UnitTypes leaderUnitType);

	const InvisibleTypes getInvisibleType() const;
	void setInvisibleType(InvisibleTypes InvisibleType);

	const InvisibleTypes getSeeInvisibleType() const;
	void setSeeInvisibleType(InvisibleTypes InvisibleType);
#if defined(MOD_PROMOTION_FEATURE_INVISIBLE)
	const int GetFeatureInvisible() const;
	const int GetFeatureInvisible2() const;
	void setFeatureInvisible(int FeatureInvisible, int FeatureInvisible2);
	bool IsInvisibleInvalid() const;
#endif
#if defined(MOD_PROMOTION_MULTIPLE_INIT_EXPERENCE)
	const int GetMultipleInitExperence() const;
	void ChangeMultipleInitExperence(int iValue);
#endif

	const CvUnit* getCombatUnit() const;
	CvUnit* getCombatUnit();
	void setCombatUnit(CvUnit* pUnit, bool bAttacking = false);

	const CvCity* getCombatCity() const;
	CvCity* getCombatCity();
	void setCombatCity(CvCity* pCity);

	void clearCombat();

	int getExtraDomainModifier(DomainTypes eIndex) const;
	void changeExtraDomainModifier(DomainTypes eIndex, int iChange);

	const CvString getName() const;
	const char* getNameKey() const;
#if defined(MOD_PROMOTIONS_UNIT_NAMING)
	const CvString getUnitName() const;
	void setUnitName(const CvString strNewValue);
#endif
	const CvString getNameNoDesc() const;
	void setName(const CvString strNewValue);
#if defined(MOD_GLOBAL_NO_LOST_GREATWORKS)
	const CvString getGreatName() const;
	void setGreatName(CvString strName);
#endif
	GreatWorkType GetGreatWork() const;
	void SetGreatWork(GreatWorkType eGreatWork);
#if defined(MOD_API_EXTENSIONS)
	bool HasGreatWork() const;
	bool HasUnusedGreatWork() const;
#endif
	int GetTourismBlastStrength() const;
	void SetTourismBlastStrength(int iValue);

#ifdef MOD_BATTLE_CAPTURE_NEW_RULE
	bool GetIsNewCapture() const;
	void SetIsNewCapture(bool value);
#endif

	// Arbitrary Script Data
	std::string getScriptData() const;
	void setScriptData(std::string szNewValue);
	int getScenarioData() const;
	void setScenarioData(int iNewValue);

	int getTerrainDoubleMoveCount(TerrainTypes eIndex) const;
	bool isTerrainDoubleMove(TerrainTypes eIndex) const;
	void changeTerrainDoubleMoveCount(TerrainTypes eIndex, int iChange);

	int getFeatureDoubleMoveCount(FeatureTypes eIndex) const;
	bool isFeatureDoubleMove(FeatureTypes eIndex) const;
	void changeFeatureDoubleMoveCount(FeatureTypes eIndex, int iChange);

#if defined(MOD_PROMOTIONS_HALF_MOVE)
	int getTerrainHalfMoveCount(TerrainTypes eIndex) const;
	bool isTerrainHalfMove(TerrainTypes eIndex) const;
	void changeTerrainHalfMoveCount(TerrainTypes eIndex, int iChange);

	int getFeatureHalfMoveCount(FeatureTypes eIndex) const;
	bool isFeatureHalfMove(FeatureTypes eIndex) const;
	void changeFeatureHalfMoveCount(FeatureTypes eIndex, int iChange);
#endif

	int getImpassableCount() const;

	int getTerrainImpassableCount(TerrainTypes eIndex) const;
	void changeTerrainImpassableCount(TerrainTypes eIndex, int iChange);

	int getFeatureImpassableCount(FeatureTypes eIndex) const;
	void changeFeatureImpassableCount(FeatureTypes eIndex, int iChange);

	bool isTerrainImpassable(TerrainTypes eIndex) const
	{
		return m_terrainImpassableCount[eIndex] > 0;
	}

	bool isFeatureImpassable(FeatureTypes eIndex) const
	{
		return m_featureImpassableCount[eIndex] > 0;
	}

	int getExtraTerrainAttackPercent(TerrainTypes eIndex) const;
	void changeExtraTerrainAttackPercent(TerrainTypes eIndex, int iChange);
	int getExtraTerrainDefensePercent(TerrainTypes eIndex) const;
	void changeExtraTerrainDefensePercent(TerrainTypes eIndex, int iChange);
	int getExtraFeatureAttackPercent(FeatureTypes eIndex) const;
	void changeExtraFeatureAttackPercent(FeatureTypes eIndex, int iChange);
	int getExtraFeatureDefensePercent(FeatureTypes eIndex) const;
	void changeExtraFeatureDefensePercent(FeatureTypes eIndex, int iChange);

#if defined(MOD_API_UNIFIED_YIELDS)
	int getYieldFromKills(YieldTypes eIndex) const;
	void changeYieldFromKills(YieldTypes eIndex, int iChange);
	int getYieldFromBarbarianKills(YieldTypes eIndex) const;
	void changeYieldFromBarbarianKills(YieldTypes eIndex, int iChange);
#endif

	int getExtraUnitCombatModifier(UnitCombatTypes eIndex) const;
	void changeExtraUnitCombatModifier(UnitCombatTypes eIndex, int iChange);

	int getUnitClassModifier(UnitClassTypes eIndex) const;
	void changeUnitClassModifier(UnitClassTypes eIndex, int iChange);

	bool canAcquirePromotion(PromotionTypes ePromotion) const;
	bool canAcquirePromotionAny() const;
	bool isPromotionValid(PromotionTypes ePromotion) const;
	bool isHasPromotion(PromotionTypes eIndex) const;
	void setHasPromotion(PromotionTypes eIndex, bool bNewValue);

	int getSubUnitCount() const;
	int getSubUnitsAlive() const;
	int getSubUnitsAlive(int iDamage) const;

	bool isEnemy(TeamTypes eTeam, const CvPlot* pPlot = NULL) const;
	bool isPotentialEnemy(TeamTypes eTeam, const CvPlot* pPlot = NULL) const;

	bool canRangeStrike() const;
#if defined(MOD_AI_SMART_V3)
	int GetRangePlusMoveToshot() const;
	void GetMovablePlotListOpt(vector<CvPlot*>& plotData, CvPlot* plotTarget, bool exitOnFound, bool bIgnoreFriendlyUnits = false);
#endif
#if defined(MOD_AI_SMART_V3)
	bool canEverRangeStrikeAt(int iX, int iY, const CvPlot* pSourcePlot = NULL) const;
#else
	bool canEverRangeStrikeAt(int iX, int iY) const;
#endif
	bool canRangeStrikeAt(int iX, int iY, bool bNeedWar = true, bool bNoncombatAllowed = true) const;

	bool IsAirSweepCapable() const;
	int GetAirSweepCapableCount() const;
	void ChangeAirSweepCapableCount(int iChange);

	bool canAirSweep() const;
	bool canAirSweepAt(int iX, int iY) const;
	bool airSweep(int iX, int iY);

	bool potentialWarAction(const CvPlot* pPlot) const;
	bool willRevealByMove(const CvPlot& pPlot) const;

	bool isAlwaysHostile(const CvPlot& pPlot) const;
	void changeAlwaysHostileCount(int iValue);
	int getAlwaysHostileCount() const;

	int getArmyID() const;
	void setArmyID(int iNewArmyID) ;

	bool isUnderTacticalControl() const;
	void setTacticalMove(TacticalAIMoveTypes eMove);
	TacticalAIMoveTypes getTacticalMove() const;
	bool canRecruitFromTacticalAI() const;
	void SetTacticalAIPlot(CvPlot* pPlot);
	CvPlot* GetTacticalAIPlot() const;

	void LogWorkerEvent(BuildTypes eBuildType, bool bStartingConstruction);

	int GetPower() const;

	bool AreUnitsOfSameType(const CvUnit& pUnit2, const bool bPretendEmbarked = false) const;
	bool CanSwapWithUnitHere(CvPlot& pPlot) const;

	void read(FDataStream& kStream);
	void write(FDataStream& kStream) const;

	void AI_promote();
	UnitAITypes AI_getUnitAIType() const;
	void AI_setUnitAIType(UnitAITypes eNewValue);
#if defined(MOD_AI_SMART_V3)
	int GetPromotionValue(int promotionBonus, int unitExtraValue, int matchFlavorValue, int baseValue);
#endif
	int AI_promotionValue(PromotionTypes ePromotion);

	GreatPeopleDirectiveTypes GetGreatPeopleDirective() const;
	void SetGreatPeopleDirective(GreatPeopleDirectiveTypes eDirective);

	bool IsSelected() const;
	bool IsFirstTimeSelected() const;
	void IncrementFirstTimeSelected();

	void SetPosition(CvPlot* pkPlot);

	const FAutoArchive& getSyncArchive() const;
	FAutoArchive& getSyncArchive();

	// Mission routines
	void PushMission(MissionTypes eMission, int iData1 = -1, int iData2 = -1, int iFlags = 0, bool bAppend = false, bool bManual = false, MissionAITypes eMissionAI = NO_MISSIONAI, CvPlot* pMissionAIPlot = NULL, CvUnit* pMissionAIUnit = NULL);
	void PopMission();
	void AutoMission();
	void UpdateMission();
	CvPlot* LastMissionPlot();
	bool CanStartMission(int iMission, int iData1, int iData2, CvPlot* pPlot = NULL, bool bTestVisible = false);
	int GetMissionTimer() const;
	void SetMissionTimer(int iNewValue);
	void ChangeMissionTimer(int iChange);
	void ClearMissionQueue(int iUnitCycleTimer = 1);
	int GetLengthMissionQueue() const;
	const MissionData* GetHeadMissionData();
	const MissionData* GetMissionData(int iIndex);
	CvPlot* GetMissionAIPlot();
	MissionAITypes GetMissionAIType();
	void SetMissionAI(MissionAITypes eNewMissionAI, CvPlot* pNewPlot, CvUnit* pNewUnit);
	CvUnit* GetMissionAIUnit();

#if defined(MOD_API_EXTENSIONS) || defined(MOD_GLOBAL_BREAK_CIVILIAN_RESTRICTIONS)
	inline bool IsCivilianUnit() const
	{
		return !(IsCombatUnit() || isRanged());
	}

	inline bool IsMinorCivUnit() const
	{
		return (m_eOwner >= MAX_MAJOR_CIVS);
	}

	inline bool IsBarbarianUnit() const
	{
		return (m_eOwner == BARBARIAN_PLAYER);
	}

	inline bool IsCityStateUnit() const
	{
		return IsMinorCivUnit() && !IsBarbarianUnit();
	}
#endif

	// Combat eligibility routines
	inline bool IsCombatUnit() const
	{
		return (m_iBaseCombat > 0);
	}

	bool IsCanAttackWithMove() const;
	bool IsCanAttackRanged() const;
	bool IsCanAttack() const;
	bool IsCanAttackWithMoveNow() const;
	bool IsCanDefend(const CvPlot* pPlot = NULL) const;
	bool IsEnemyInMovementRange(bool bOnlyFortified = false, bool bOnlyCities = false);

	// Path-finding routines
	bool GeneratePath(const CvPlot* pToPlot, int iFlags = 0, bool bReuse = false, int* piPathTurns = NULL) const;
	void ResetPath();
	CvPlot* GetPathFirstPlot() const;
	CvPlot* GetPathLastPlot() const;
	const CvPathNodeArray& GetPathNodeArray() const;
	CvPlot* GetPathEndTurnPlot() const;

	bool isBusyMoving() const;
	void setBusyMoving(bool bState);

	bool IsIgnoringDangerWakeup() const;
	void SetIgnoreDangerWakeup(bool bState);

	bool IsNotCivilianIfEmbarked() const;
	void ChangeNotCivilianIfEmbarkedCount(int iValue);
	int GetNotCivilianIfEmbarkedCount() const;

	bool IsEmbarkAllWater() const;
	void ChangeEmbarkAllWaterCount(int iValue);
	int GetEmbarkAllWaterCount() const;

#if defined(MOD_PROMOTIONS_DEEP_WATER_EMBARKATION)
	bool IsEmbarkDeepWater() const;
	void ChangeEmbarkDeepWaterCount(int iValue);
	int GetEmbarkDeepWaterCount() const;
#endif

	void ChangeEmbarkExtraVisibility(int iValue);
	int GetEmbarkExtraVisibility() const;

	void ChangeEmbarkDefensiveModifier(int iValue);
	int GetEmbarkDefensiveModifier() const;

	void ChangeCapitalDefenseModifier(int iValue);
	int GetCapitalDefenseModifier() const;
	void ChangeCapitalDefenseFalloff(int iValue);
	int GetCapitalDefenseFalloff() const;

#if defined(MOD_DEFENSE_MOVES_BONUS)
	void ChangeMoveLeftDefenseMod(int iValue);
	int GetMoveLeftDefenseMod() const;

	void ChangeMoveUsedDefenseMod(int iValue);
	int GetMoveUsedDefenseMod() const;
#endif

#if defined(MOD_ROG_CORE)
	void ChangeMoveLfetAttackMod(int iValue);
	int GetMoveLfetAttackMod() const;

	void ChangeMoveUsedAttackMod(int iValue);
	int GetMoveUsedAttackMod() const;

	void ChangeGoldenAgeMod(int iValue);
	int GetGoldenAgeMod() const;

	void ChangeRangedSupportFireMod(int iValue);
	int GetRangedSupportFireMod() const;


	int GetDamageAoEFortified() const;
	void ChangeDamageAoEFortified(int iChange);

	int GetWorkRateMod() const;
	void ChangeWorkRateMod(int iChange);

	int getAOEDamageOnKill() const;
	void changeAOEDamageOnKill(int iChange);

	int GetBarbarianCombatBonus() const;
	void ChangeBarbarianCombatBonus(int iValue);
#endif

	int GetCaptureDefeatedEnemyChance() const;
	void ChangeCaptureDefeatedEnemyChance(int iValue);
	void ChangeCannotBeCapturedCount(int iChange);
	bool GetCannotBeCaptured();


#if defined(MOD_ROG_CORE)
	void ChangeNumSpyAttackMod(int iValue);
	int GetNumSpyAttackMod() const;

	void ChangeNumWonderAttackMod(int iValue);
	int GetNumWonderAttackMod() const;

	void ChangeNumWorkAttackMod(int iValue);
	int GetNumWorkAttackMod() const;


	void ChangeNumSpyDefenseMod(int iValue);
	int GetNumSpyDefenseMod() const;

	void ChangeNumWonderDefenseMod(int iValue);
	int GetNumWonderDefenseMod() const;

	void ChangeNumWorkDefenseMod(int iValue);
	int GetNumWorkDefenseMod() const;


	bool IsNoResourcePunishment() const;
	void ChangeIsNoResourcePunishment(int iChange);

	void ChangeCurrentHitPointAttackMod(int iValue);
	int GetCurrentHitPointAttackMod() const;

	void ChangeCurrentHitPointDefenseMod(int iValue);
	int GetCurrentHitPointDefenseMod() const;


	void ChangeNearNumEnemyAttackMod(int iValue);
	int GetNearNumEnemyAttackMod() const;

	void ChangeNearNumEnemyDefenseMod(int iValue);
	int GetNearNumEnemyDefenseMod() const;


	int GetNumEnemyAdjacent() const;

#endif


	void ChangeCityAttackPlunderModifier(int iValue);
	int GetCityAttackPlunderModifier() const;

#if defined(MOD_PROMOTION_GET_INSTANCE_FROM_ATTACK)
	void ChangeUnitAttackFaithBonus(int iValue);
	void ChangeCityAttackFaithBonus(int iValue);
	int GetUnitAttackFaithBonus() const;
	int GetCityAttackFaithBonus() const;
#endif
#if defined(MOD_PROMOTION_REMOVE_PROMOTION_UPGRADE)
	void setRemovePromotionUpgrade(int iValue);
	int GetRemovePromotionUpgrade() const;
#endif

	void ChangeReligiousStrengthLossRivalTerritory(int iValue);
	int GetReligiousStrengthLossRivalTerritory() const;

	void ChangeTradeMissionInfluenceModifier(int iValue);
	int GetTradeMissionInfluenceModifier() const;

	void ChangeTradeMissionGoldModifier(int iValue);
	int GetTradeMissionGoldModifier() const;

	bool IsHasBeenPromotedFromGoody() const;
	void SetBeenPromotedFromGoody(bool bBeenPromoted);

	bool IsHigherTechThan(UnitTypes otherUnit) const;
	bool IsLargerCivThan(const CvUnit* pOtherUnit) const;

	int GetNumGoodyHutsPopped() const;
	void ChangeNumGoodyHutsPopped(int iValue);

	// Ported in from old CvUnitAI class
	int SearchRange(int iRange) const;
#if defined(MOD_AI_SECONDARY_WORKERS)
	bool PlotValid(CvPlot* pPlot, byte bMoveFlags = 0) const;
#else
	bool PlotValid(CvPlot* pPlot) const;
#endif

	CvUnitReligion* GetReligionData() const
	{
		return m_pReligion;
	};

	static void dispatchingNetMessage(bool dispatching);
	static bool dispatchingNetMessage();

	std::string debugDump(const FAutoVariableBase&) const;
	std::string stackTraceRemark(const FAutoVariableBase&) const;

#if defined(MOD_API_EXTENSIONS)
	bool IsCivilization(CivilizationTypes iCivilizationType) const;
	bool HasPromotion(PromotionTypes iPromotionType) const;
	bool IsUnit(UnitTypes iUnitType) const;
	bool IsUnitClass(UnitClassTypes iUnitClassType) const;
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
#endif

#ifdef MOD_GLOBAL_WAR_CASUALTIES
	int GetWarCasualtiesModifier() const;
	void ChangeWarCasualtiesModifier(int iChange);
	void SetWarCasualtiesModifier(int iValue);
#endif

#ifdef MOD_PROMOTION_SPLASH_DAMAGE
	std::vector<SplashInfo>& GetSplashInfoVec();

	int GetSplashImmuneRC() const;
	void ChangeSplashImmuneRC(int iChange);
	void SetSplashImmuneRC(int iValue);

	int GetSplashXP() const;
	void ChangeSplashXP(int iChange);
	void SetSplashXP(int iValue);
#endif

#ifdef MOD_PROMOTION_COLLECTIONS
	std::tr1::unordered_map<PromotionCollectionsTypes, int>& GetPromotionCollections();
#endif

#ifdef MOD_PROMOTION_ADD_ENEMY_PROMOTIONS
	int GetAddEnemyPromotionImmuneRC() const;
	bool IsImmuneNegtivePromotions() const;
	void ChangeAddEnemyPromotionImmuneRC(int iChange);
#endif

#ifdef MOD_GLOBAL_PROMOTIONS_REMOVAL
	void ClearSamePlotPromotions();
	std::tr1::unordered_set<PromotionTypes>& GetPromotionsThatCanBeActionCleared();
	void RemoveDebuffWhenDoTurn();
	bool CanRemoveDebuff(const AutoRemoveInfo& kAutoRemoveInfo) const;
#endif

#ifdef MOD_PROMOTION_CITY_DESTROYER
	std::tr1::unordered_map<PromotionTypes, DestroyBuildingsInfo>& GetDestroyBuildings();

	int GetSiegeKillCitizensPercent() const;
	int GetSiegeKillCitizensFixed() const;
	void ChangeSiegeKillCitizensPercent(int iChange);
	void ChangeSiegeKillCitizensFixed(int iChange);
	bool CanSiegeKillCitizens() const;
#endif

#ifdef MOD_PROMOTION_COLLATERAL_DAMAGE
	std::vector<CollateralInfo>& GetCollateralInfoVec();

	int GetCollateralImmuneRC() const;
	void ChangeCollateralImmuneRC(int iChange);
	void SetCollateralImmuneRC(int iValue);

	int GetCollateralXP() const;
	void ChangeCollateralXP(int iChange);
	void SetCollateralXP(int iValue);
#endif

protected:
	const MissionQueueNode* HeadMissionQueueNode() const;
	MissionQueueNode* HeadMissionQueueNode();

	bool getCaptureDefinition(CvUnitCaptureDefinition* pkCaptureDef, PlayerTypes eCapturingPlayer = NO_PLAYER, CvUnit* pCapturingUnit = nullptr);
	static CvUnit* createCaptureUnit(const CvUnitCaptureDefinition& kCaptureDef);

	void	ClearPathCache();
	bool	UpdatePathCache(CvPlot* pDestPlot, int iFlags);

	void QueueMoveForVisualization(CvPlot* pkPlot);
	void PublishQueuedVisualizationMoves();

	typedef enum Flags
	{
	    UNITFLAG_EVALUATING_MISSION = 0x1,
	    UNITFLAG_ALREADY_GOT_GOODY_UPGRADE = 0x2
	};

	FAutoArchiveClassContainer<CvUnit> m_syncArchive;

	FAutoVariable<PlayerTypes, CvUnit> m_eOwner;
	FAutoVariable<PlayerTypes, CvUnit> m_eOriginalOwner;
	FAutoVariable<UnitTypes, CvUnit> m_eUnitType;
	FAutoVariable<int, CvUnit> m_iX;
	FAutoVariable<int, CvUnit> m_iY;
	FAutoVariable<int, CvUnit> m_iID;

	FAutoVariable<int, CvUnit> m_iHotKeyNumber;
	FAutoVariable<int, CvUnit> m_iDeployFromOperationTurn;
	int m_iLastMoveTurn;
	short m_iCycleOrder;
#if defined(MOD_API_UNIT_STATS)
	int m_iStatsTravelled;
	int m_iStatsKilled;
#endif
	FAutoVariable<int, CvUnit> m_iReconX;
	FAutoVariable<int, CvUnit> m_iReconY;
	FAutoVariable<int, CvUnit> m_iReconCount;
	FAutoVariable<int, CvUnit> m_iGameTurnCreated;
	FAutoVariable<int, CvUnit> m_iDamage;
	FAutoVariable<int, CvUnit> m_iMoves;
	FAutoVariable<bool, CvUnit> m_bImmobile;
	FAutoVariable<int, CvUnit> m_iExperience;
#if defined(MOD_UNITS_XP_TIMES_100)
	int m_iExperienceTimes100;
#endif
	FAutoVariable<int, CvUnit> m_iLevel;
	FAutoVariable<int, CvUnit> m_iCargo;
	FAutoVariable<int, CvUnit> m_iCargoCapacity;
	FAutoVariable<int, CvUnit> m_iAttackPlotX;
	FAutoVariable<int, CvUnit> m_iAttackPlotY;
	FAutoVariable<int, CvUnit> m_iCombatTimer;
	FAutoVariable<int, CvUnit> m_iCombatFirstStrikes;
	FAutoVariable<int, CvUnit> m_iCombatDamage;
	FAutoVariable<int, CvUnit> m_iFortifyTurns;
	FAutoVariable<bool, CvUnit> m_bFortifiedThisTurn;
	FAutoVariable<int, CvUnit> m_iBlitzCount;
	FAutoVariable<int, CvUnit> m_iAmphibCount;
	FAutoVariable<int, CvUnit> m_iRiverCrossingNoPenaltyCount;
	FAutoVariable<int, CvUnit> m_iEnemyRouteCount;
	FAutoVariable<int, CvUnit> m_iRivalTerritoryCount;
	FAutoVariable<int, CvUnit> m_iMustSetUpToRangedAttackCount;
	FAutoVariable<int, CvUnit> m_iRangeAttackIgnoreLOSCount;
	int m_iCityAttackOnlyCount;
	int m_iCaptureDefeatedEnemyCount;
	FAutoVariable<int, CvUnit> m_iRangedSupportFireCount;
	FAutoVariable<int, CvUnit> m_iAlwaysHealCount;
	FAutoVariable<int, CvUnit> m_iHealOutsideFriendlyCount;
	FAutoVariable<int, CvUnit> m_iHillsDoubleMoveCount;
	FAutoVariable<int, CvUnit> m_iImmuneToFirstStrikesCount;
	FAutoVariable<int, CvUnit> m_iExtraVisibilityRange;
#if defined(MOD_PROMOTIONS_VARIABLE_RECON)
	FAutoVariable<int, CvUnit> m_iExtraReconRange;
#endif
	FAutoVariable<int, CvUnit> m_iExtraMoves;
	FAutoVariable<int, CvUnit> m_iExtraMoveDiscount;
	FAutoVariable<int, CvUnit> m_iExtraRange;
	FAutoVariable<int, CvUnit> m_iExtraIntercept;
	FAutoVariable<int, CvUnit> m_iExtraEvasion;
	FAutoVariable<int, CvUnit> m_iExtraFirstStrikes;
	FAutoVariable<int, CvUnit> m_iExtraChanceFirstStrikes;
	FAutoVariable<int, CvUnit> m_iExtraWithdrawal;
	FAutoVariable<int, CvUnit> m_iExtraEnemyHeal;
	FAutoVariable<int, CvUnit> m_iExtraNeutralHeal;
	FAutoVariable<int, CvUnit> m_iExtraFriendlyHeal;
	int m_iEnemyDamageChance;
	int m_iNeutralDamageChance;
	int m_iEnemyDamage;
	int m_iNeutralDamage;
	int m_iNearbyEnemyCombatMod;
	int m_iNearbyEnemyCombatRange;
	FAutoVariable<int, CvUnit> m_iSameTileHeal;
	FAutoVariable<int, CvUnit> m_iAdjacentTileHeal;
	FAutoVariable<int, CvUnit> m_iAdjacentModifier;
	FAutoVariable<int, CvUnit> m_iRangedAttackModifier;
	FAutoVariable<int, CvUnit> m_iInterceptionCombatModifier;
	FAutoVariable<int, CvUnit> m_iInterceptionDefenseDamageModifier;
	FAutoVariable<int, CvUnit> m_iAirSweepCombatModifier;
	FAutoVariable<int, CvUnit> m_iAttackModifier;
	FAutoVariable<int, CvUnit> m_iDefenseModifier;
	FAutoVariable<int, CvUnit> m_iExtraCombatPercent;
	FAutoVariable<int, CvUnit> m_iExtraCityAttackPercent;
	FAutoVariable<int, CvUnit> m_iExtraCityDefensePercent;
	FAutoVariable<int, CvUnit> m_iExtraRangedDefenseModifier;
	FAutoVariable<int, CvUnit> m_iExtraHillsAttackPercent;
	FAutoVariable<int, CvUnit> m_iExtraHillsDefensePercent;
	FAutoVariable<int, CvUnit> m_iExtraOpenAttackPercent;
	FAutoVariable<int, CvUnit> m_iExtraOpenRangedAttackMod;
	FAutoVariable<int, CvUnit> m_iExtraRoughAttackPercent;
	FAutoVariable<int, CvUnit> m_iExtraRoughRangedAttackMod;
	FAutoVariable<int, CvUnit> m_iExtraAttackFortifiedMod;
	FAutoVariable<int, CvUnit> m_iExtraAttackWoundedMod;
	int m_iFlankAttackModifier;
	FAutoVariable<int, CvUnit> m_iExtraOpenDefensePercent;
	FAutoVariable<int, CvUnit> m_iExtraRoughDefensePercent;
	FAutoVariable<int, CvUnit> m_iPillageChange;
	FAutoVariable<int, CvUnit> m_iUpgradeDiscount;
	FAutoVariable<int, CvUnit> m_iExperiencePercent;
	FAutoVariable<int, CvUnit> m_iDropRange;
	FAutoVariable<int, CvUnit> m_iAirSweepCapableCount;
	FAutoVariable<int, CvUnit> m_iExtraNavalMoves;
	FAutoVariable<int, CvUnit> m_iKamikazePercent;
	FAutoVariable<int, CvUnit> m_iBaseCombat;
#if defined(MOD_API_EXTENSIONS)
	int m_iBaseRangedCombat;
#endif
	FAutoVariable<DirectionTypes, CvUnit> m_eFacingDirection;
	FAutoVariable<int, CvUnit> m_iArmyId;

	FAutoVariable<int, CvUnit> m_iIgnoreTerrainCostCount;
#if defined(MOD_API_PLOT_BASED_DAMAGE)
	FAutoVariable<int, CvUnit> m_iIgnoreTerrainDamageCount;
	FAutoVariable<int, CvUnit> m_iIgnoreFeatureDamageCount;
	FAutoVariable<int, CvUnit> m_iExtraTerrainDamageCount;
	FAutoVariable<int, CvUnit> m_iExtraFeatureDamageCount;
#endif
#if defined(MOD_PROMOTIONS_IMPROVEMENT_BONUS)
	FAutoVariable<int, CvUnit> m_iNearbyImprovementCombatBonus;
	FAutoVariable<int, CvUnit> m_iNearbyImprovementBonusRange;
	FAutoVariable<ImprovementTypes, CvUnit> m_eCombatBonusImprovement;
#endif

#if defined(MOD_PROMOTIONS_ALLYCITYSTATE_BONUS)
	FAutoVariable<int, CvUnit> m_iAllyCityStateCombatModifier;
	FAutoVariable<int, CvUnit> m_iAllyCityStateCombatModifierMax;
#endif

#if defined(MOD_PROMOTIONS_EXTRARES_BONUS)
	FAutoVariable<ResourceTypes, CvUnit> m_eExtraResourceType;
	FAutoVariable<int, CvUnit> m_iExtraResourceCombatModifier;
	FAutoVariable<int, CvUnit> m_iExtraResourceCombatModifierMax;
	FAutoVariable<int, CvUnit> m_iExtraHappinessCombatModifier;
	FAutoVariable<int, CvUnit> m_iExtraHappinessCombatModifierMax;
#endif

#if defined(MOD_ROG_CORE)
	FAutoVariable<int, CvUnit> m_iAoEDamageOnMove;
	FAutoVariable<int, CvUnit> m_iForcedDamage;
	FAutoVariable<int, CvUnit> m_iChangeDamage;
	FAutoVariable<int, CvUnit> m_iExtraFullyHealedMod;
	FAutoVariable<int, CvUnit> m_iExtraAttackAboveHealthMod;
	FAutoVariable<int, CvUnit> m_iExtraAttackBelowHealthMod;
	FAutoVariable<int, CvUnit> m_iFightWellDamaged;
	FAutoVariable<int, CvUnit> m_iStrongerDamaged;
#endif

#if defined(MOD_ROG_CORE)
	FAutoVariable<int, CvUnit> m_iMeleeDefenseModifier;

	FAutoVariable<int, CvUnit> m_iNearbyUnitClassBonus;
	FAutoVariable<int, CvUnit> m_iNearbyUnitClassBonusRange;
	FAutoVariable<UnitClassTypes, CvUnit>  m_iCombatBonusFromNearbyUnitClass;
#endif

#if defined(MOD_ROG_CORE)
	FAutoVariable<int, CvUnit> m_iNumOriginalCapitalAttackMod;
	FAutoVariable<int, CvUnit> m_iNumOriginalCapitalDefenseMod;
	FAutoVariable<int, CvUnit> m_iHPHealedIfDefeatEnemyGlobal;
#endif


#if defined(MOD_ROG_CORE)
	FAutoVariable<int, CvUnit> m_iOnCapitalLandAttackMod;
	FAutoVariable<int, CvUnit> m_iOutsideCapitalLandAttackMod;
	FAutoVariable<int, CvUnit> m_iOnCapitalLandDefenseMod;
	FAutoVariable<int, CvUnit> m_iOutsideCapitalLandDefenseMod;
#endif



#if defined(MOD_ROG_CORE)
	FAutoVariable<int, CvUnit> m_iNumSpyDefenseMod;
	FAutoVariable<int, CvUnit> m_iNumSpyAttackMod;
	FAutoVariable<int, CvUnit> m_iNumWonderDefenseMod;
	FAutoVariable<int, CvUnit> m_iNumWonderAttackMod;
	FAutoVariable<int, CvUnit> m_iNumWorkDefenseMod;
	FAutoVariable<int, CvUnit> m_iNumWorkAttackMod;

	FAutoVariable<int, CvUnit> m_iNoResourcePunishment;

	FAutoVariable<int, CvUnit> m_iCurrentHitPointAttackMod;
	FAutoVariable<int, CvUnit> m_iCurrentHitPointDefenseMod;

	FAutoVariable<int, CvUnit> m_iNearNumEnemyAttackMod;
	FAutoVariable<int, CvUnit> m_iNearNumEnemyDefenseMod;
#endif

#if defined(MOD_PROMOTIONS_CROSS_MOUNTAINS)
	FAutoVariable<int, CvUnit> m_iCanCrossMountainsCount;
#endif
#if defined(MOD_PROMOTIONS_CROSS_OCEANS)
	FAutoVariable<int, CvUnit> m_iCanCrossOceansCount;
#endif
#if defined(MOD_PROMOTIONS_CROSS_ICE)
	FAutoVariable<int, CvUnit> m_iCanCrossIceCount;
#endif
#if defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
	FAutoVariable<int, CvUnit> m_iGGFromBarbariansCount;
#endif
	FAutoVariable<int, CvUnit> m_iRoughTerrainEndsTurnCount;
	FAutoVariable<int, CvUnit> m_iEmbarkAbilityCount;
	FAutoVariable<int, CvUnit> m_iHoveringUnitCount;
	FAutoVariable<int, CvUnit> m_iFlatMovementCostCount;
	FAutoVariable<int, CvUnit> m_iCanMoveImpassableCount;
	FAutoVariable<int, CvUnit> m_iOnlyDefensiveCount;
	FAutoVariable<int, CvUnit> m_iNoDefensiveBonusCount;
	FAutoVariable<int, CvUnit> m_iNoCaptureCount;
	FAutoVariable<int, CvUnit> m_iNukeImmuneCount;
	FAutoVariable<int, CvUnit> m_iHiddenNationalityCount;
	FAutoVariable<int, CvUnit> m_iAlwaysHostileCount;
	FAutoVariable<int, CvUnit> m_iNoRevealMapCount;
	FAutoVariable<int, CvUnit> m_iCanMoveAllTerrainCount;
	FAutoVariable<int, CvUnit> m_iCanMoveAfterAttackingCount;
	FAutoVariable<int, CvUnit> m_iFreePillageMoveCount;
	int m_iHealOnPillageCount;
	FAutoVariable<int, CvUnit> m_iHPHealedIfDefeatEnemy;
	int m_iGoldenAgeValueFromKills;
	FAutoVariable<int, CvUnit> m_iTacticalAIPlotX;
	FAutoVariable<int, CvUnit> m_iTacticalAIPlotY;
	FAutoVariable<int, CvUnit> m_iGarrisonCityID;   // unused
	FAutoVariable<int, CvUnit> m_iFlags;
	FAutoVariable<int, CvUnit> m_iNumAttacks;
	FAutoVariable<int, CvUnit> m_iAttacksMade;
	FAutoVariable<int, CvUnit> m_iGreatGeneralCount;
	int m_iGreatAdmiralCount;
#if defined(MOD_PROMOTIONS_AURA_CHANGE)
	int m_iAuraRangeChange;
	int m_iAuraEffectChange;
#endif
	FAutoVariable<int, CvUnit> m_iGreatGeneralModifier;
	int m_iGreatGeneralReceivesMovementCount;
	int m_iGreatGeneralCombatModifier;
	int m_iIgnoreGreatGeneralBenefit;
	int m_iIgnoreZOC;

	int m_iImmueMeleeAttack;
#if defined(MOD_UNITS_NO_SUPPLY)
	int m_iNoSupply;
#endif
#if defined(MOD_UNITS_MAX_HP)
	int m_iMaxHitPointsBase;
	int m_iMaxHitPointsChange;
	int m_iMaxHitPointsModifier;
#endif
	FAutoVariable<int, CvUnit> m_iFriendlyLandsModifier;
	FAutoVariable<int, CvUnit> m_iFriendlyLandsAttackModifier;
	FAutoVariable<int, CvUnit> m_iOutsideFriendlyLandsModifier;
	FAutoVariable<int, CvUnit> m_iHealIfDefeatExcludeBarbariansCount;
	FAutoVariable<int, CvUnit> m_iNumInterceptions;
	FAutoVariable<int, CvUnit> m_iMadeInterceptionCount;
	int m_iEverSelectedCount;
	int m_iSapperCount;
	int m_iCanHeavyCharge;
	int m_iNumExoticGoods;

	FAutoVariable<bool, CvUnit> m_bPromotionReady;
	FAutoVariable<bool, CvUnit> m_bDeathDelay;
	FAutoVariable<bool, CvUnit> m_bCombatFocus;
	FAutoVariable<bool, CvUnit> m_bInfoBarDirty;
	FAutoVariable<bool, CvUnit> m_bNotConverting;
	FAutoVariable<bool, CvUnit> m_bAirCombat;
	FAutoVariable<bool, CvUnit> m_bSetUpForRangedAttack;
	FAutoVariable<bool, CvUnit> m_bEmbarked;
	FAutoVariable<bool, CvUnit> m_bAITurnProcessed;
#if defined(MOD_API_UNIT_CANNOT_BE_RANGED_ATTACKED)
	FAutoVariable<bool, CvUnit> m_bCannotBeRangedAttacked;
#endif

	FAutoVariable<TacticalAIMoveTypes, CvUnit> m_eTacticalMove;
	FAutoVariable<PlayerTypes, CvUnit> m_eCapturingPlayer;
#ifdef MOD_BATTLE_CAPTURE_NEW_RULE
	CvUnit* m_pCapturingUnit;
#endif

	bool m_bCapturedAsIs;
	FAutoVariable<UnitTypes, CvUnit> m_eLeaderUnitType;
	FAutoVariable<InvisibleTypes, CvUnit> m_eInvisibleType;
	FAutoVariable<InvisibleTypes, CvUnit> m_eSeeInvisibleType;
#if defined(MOD_PROMOTION_FEATURE_INVISIBLE)
	FAutoVariable<int, CvUnit> m_eFeatureInvisible;
	FAutoVariable<int, CvUnit> m_eFeatureInvisible2;
#endif
#if defined(MOD_PROMOTION_MULTIPLE_INIT_EXPERENCE)
	FAutoVariable<int, CvUnit> m_eMultipleInitExperence;
#endif

	FAutoVariable<GreatPeopleDirectiveTypes, CvUnit> m_eGreatPeopleDirectiveType;
	CvUnitEntry* m_pUnitInfo;

	bool m_bWaitingForMove;			///< If true, the unit is busy visualizing its move.

	IDInfo m_combatUnit;
	IDInfo m_combatCity;
	IDInfo m_transportUnit;

	std::vector<int> m_extraDomainModifiers;

	FAutoVariable<CvString, CvUnit> m_strNameIAmNotSupposedToBeUsedAnyMoreBecauseThisShouldNotBeCheckedAndWeNeedToPreserveSaveGameCompatibility;
	FAutoVariable<CvString, CvUnit> m_strScriptData;
	int m_iScenarioData;

	CvUnitPromotions  m_Promotions;
	CvUnitReligion* m_pReligion;

	FAutoVariable<std::vector<int>, CvUnit> m_terrainDoubleMoveCount;
	FAutoVariable<std::vector<int>, CvUnit> m_featureDoubleMoveCount;
#if defined(MOD_PROMOTIONS_HALF_MOVE)
	FAutoVariable<std::vector<int>, CvUnit> m_terrainHalfMoveCount;
	FAutoVariable<std::vector<int>, CvUnit> m_featureHalfMoveCount;
#endif
	FAutoVariable<std::vector<int>, CvUnit> m_terrainImpassableCount;
	FAutoVariable<std::vector<int>, CvUnit> m_featureImpassableCount;
	FAutoVariable<std::vector<int>, CvUnit> m_extraTerrainAttackPercent;
	FAutoVariable<std::vector<int>, CvUnit> m_extraTerrainDefensePercent;
	FAutoVariable<std::vector<int>, CvUnit> m_extraFeatureAttackPercent;
	FAutoVariable<std::vector<int>, CvUnit> m_extraFeatureDefensePercent;
#if defined(MOD_API_UNIFIED_YIELDS)
	FAutoVariable<std::vector<int>, CvUnit> m_yieldFromKills;
	FAutoVariable<std::vector<int>, CvUnit> m_yieldFromBarbarianKills;
#endif
	FAutoVariable<std::vector<int>, CvUnit> m_extraUnitCombatModifier;
	FAutoVariable<std::vector<int>, CvUnit> m_unitClassModifier;
	int m_iMissionTimer;
	FAutoVariable<int, CvUnit> m_iMissionAIX;
	FAutoVariable<int, CvUnit> m_iMissionAIY;
	FAutoVariable<MissionAITypes, CvUnit> m_eMissionAIType;
	IDInfo m_missionAIUnit;
	FAutoVariable<ActivityTypes, CvUnit> m_eActivityType;
	FAutoVariable<AutomateTypes, CvUnit> m_eAutomateType;
	FAutoVariable<UnitAITypes, CvUnit> m_eUnitAIType;
	DestructionNotification<UnitHandle> m_destructionNotification;

	UnitHandle  m_thisHandle;

	UnitMovementQueue m_unitMoveLocs;

	bool m_bIgnoreDangerWakeup; // slewis - make this an autovariable when saved games are broken
	int m_iEmbarkedAllWaterCount;
#if defined(MOD_PROMOTIONS_DEEP_WATER_EMBARKATION)
	int m_iEmbarkedDeepWaterCount;
#endif

#if defined(MOD_DEFENSE_MOVES_BONUS)
	int m_iMoveLeftDefenseMod;
	int m_iMoveUsedDefenseMod;
#endif

#if defined(MOD_ROG_CORE)
	int m_iMoveLfetAttackMod;
	int m_iMoveUsedAttackMod;
	int m_iGoldenAgeMod;
	int m_iRangedSupportFireMod;

	int m_iBarbCombatBonus;
	int m_iDamageAoEFortified;
	int m_iWorkRateMod;
	int m_iAOEDamageOnKill;
#endif

	int m_iCannotBeCapturedCount;
	int m_iCaptureDefeatedEnemyChance;


#ifdef MOD_PROMOTION_SPLASH_DAMAGE
	std::vector<SplashInfo> m_asSplashInfoVec = {};

	int m_iSplashImmuneRC = 0;
	int m_iSplashXP = 0;
#endif

#ifdef MOD_PROMOTION_COLLATERAL_DAMAGE
	std::vector<CollateralInfo> m_asCollateralInfoVec = {};

	int m_iCollateralImmuneRC = 0;
	int m_iCollateralXP = 0;
#endif

#ifdef MOD_PROMOTION_COLLECTIONS
	std::tr1::unordered_map<PromotionCollectionsTypes, int> m_sPromotionCollections;
#endif

 #ifdef MOD_PROMOTION_ADD_ENEMY_PROMOTIONS
	int m_iAddEnemyPromotionImmuneRC = 0;
 #endif

#ifdef MOD_GLOBAL_PROMOTIONS_REMOVAL
	std::tr1::unordered_map<PromotionTypes, AutoRemoveInfo> m_mapAutoRemovePromotions;
	std::tr1::unordered_set<PromotionTypes> m_sPromotionsThatCanBeActionCleared;
#endif

#ifdef MOD_PROMOTION_CITY_DESTROYER
	std::tr1::unordered_map<PromotionTypes, DestroyBuildingsInfo> m_mapDestroyBuildings;

	int m_iSiegeKillCitizensPercent = 0;
	int m_iSiegeKillCitizensFixed = 0;
#endif

	int m_iEmbarkExtraVisibility;
	int m_iEmbarkDefensiveModifier;
	int m_iCapitalDefenseModifier;
	int m_iCapitalDefenseFalloff;
	int m_iCityAttackPlunderModifier;
#if defined(MOD_PROMOTION_GET_INSTANCE_FROM_ATTACK)
	int m_iUnitAttackFaithBonus;
	int m_iCityAttackFaithBonus;
#endif	
#if defined(MOD_PROMOTION_REMOVE_PROMOTION_UPGRADE)
	int m_iRemovePromotionUpgrade;
#endif
	int m_iReligiousStrengthLossRivalTerritory;
	int m_iTradeMissionInfluenceModifier;
	int m_iTradeMissionGoldModifier;
	int m_iMapLayer;		// Which layer does the unit reside on for pathing/stacking/etc.
	int m_iNumGoodyHutsPopped;
	int m_iLastGameTurnAtFullHealth;

	int m_iAttackInflictDamageChange = 0;
	int m_iAttackInflictDamageChangeMaxHPPercent = 0;

	int m_iDefenseInflictDamageChange = 0;
	int m_iDefenseInflictDamageChangeMaxHPPercent = 0;

	int m_iSiegeInflictDamageChange = 0;
	int m_iSiegeInflictDamageChangeMaxHPPercent = 0;

	int m_iHeavyChargeAddMoves = 0;
	int m_iHeavyChargeExtraDamage = 0;
	int m_iHeavyChargeCollateralFixed = 0;
	int m_iHeavyChargeCollateralPercent = 0;
	
#if defined(MOD_PROMOTIONS_UNIT_NAMING)
	CvString m_strUnitName;
#endif
	CvString m_strName;
#if defined(MOD_GLOBAL_NO_LOST_GREATWORKS)
	CvString m_strGreatName;
#endif
	GreatWorkType m_eGreatWork;
	int m_iTourismBlastStrength;

#ifdef MOD_BATTLE_CAPTURE_NEW_RULE
	bool m_bIsNewCapture = false;
#endif

	mutable CvPathNodeArray m_kLastPath;
	mutable uint m_uiLastPathCacheDest;

	bool canAdvance(const CvPlot& pPlot, int iThreshold) const;

	CvUnit* airStrikeTarget(CvPlot& pPlot, bool bNoncombatAllowed) const;

	bool CanWithdrawFromMelee(CvUnit& pAttacker);
	bool DoWithdrawFromMelee(CvUnit& pAttacker);

	// these are do to a unit using Heavy Charge against you
	bool CanFallBackFromMelee(CvUnit& pAttacker);
	bool DoFallBackFromMelee(CvUnit& pAttacker);

 #ifdef MOD_GLOBAL_WAR_CASUALTIES
	int m_iWarCasualtiesModifier = 0;
 #endif

private:

	mutable MissionQueue m_missionQueue;
};

FDataStream& operator<<(FDataStream&, const CvUnit&);
FDataStream& operator>>(FDataStream&, CvUnit&);

namespace FSerialization
{
void SyncUnits();
void ClearUnitDeltas();
}

#endif
