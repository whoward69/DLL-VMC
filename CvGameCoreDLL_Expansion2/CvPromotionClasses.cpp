/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include "ICvDLLUserInterface.h"
#include "CvGameCoreUtils.h"
#include "CvUnitClasses.h"
#include "CustomMods.h"

// include this after all other headers!
#include "LintFree.h"

/// Constructor
CvPromotionEntry::CvPromotionEntry():
	m_iLayerAnimationPath(ANIMATIONPATH_NONE),
	m_iPrereqPromotion(NO_PROMOTION),
	m_iPrereqOrPromotion1(NO_PROMOTION),
	m_iPrereqOrPromotion2(NO_PROMOTION),
	m_iPrereqOrPromotion3(NO_PROMOTION),
	m_iPrereqOrPromotion4(NO_PROMOTION),
	m_iPrereqOrPromotion5(NO_PROMOTION),
	m_iPrereqOrPromotion6(NO_PROMOTION),
	m_iPrereqOrPromotion7(NO_PROMOTION),
	m_iPrereqOrPromotion8(NO_PROMOTION),
	m_iPrereqOrPromotion9(NO_PROMOTION),
	m_iPrereqOrPromotion10(NO_PROMOTION),
	m_iPrereqOrPromotion11(NO_PROMOTION),
	m_iPrereqOrPromotion12(NO_PROMOTION),
	m_iPrereqOrPromotion13(NO_PROMOTION),

	m_iTechPrereq(NO_TECH),
	m_iInvisibleType(NO_INVISIBLE),
	m_iSeeInvisibleType(NO_INVISIBLE),
	m_iVisibilityChange(0),
#if defined(MOD_PROMOTIONS_VARIABLE_RECON)
	m_iReconChange(0),
#endif
	m_iMovesChange(0),
	m_iMoveDiscountChange(0),
	m_iRangeChange(0),
	m_iRangedAttackModifier(0),
	m_iInterceptionCombatModifier(0),
	m_iInterceptionDefenseDamageModifier(0),
	m_iAirSweepCombatModifier(0),
	m_iInterceptChanceChange(0),
	m_iNumInterceptionChange(0),
	m_iEvasionChange(0),
	m_iCargoChange(0),
	m_iEnemyHealChange(0),
	m_iNeutralHealChange(0),
	m_iFriendlyHealChange(0),
	m_iSameTileHealChange(0),
	m_iAdjacentTileHealChange(0),
	m_iEnemyDamageChance(0),
	m_iNeutralDamageChance(0),
	m_iEnemyDamage(0),
	m_iNeutralDamage(0),
	m_iCombatPercent(0),
	m_iCityAttackPercent(0),
	m_iCityDefensePercent(0),
	m_iRangedDefenseMod(0),
	m_iHillsAttackPercent(0),
	m_iHillsDefensePercent(0),
	m_iOpenAttackPercent(0),
	m_iOpenRangedAttackMod(0),
	m_iRoughAttackPercent(0),
	m_iRoughRangedAttackMod(0),
	m_iAttackFortifiedMod(0),
	m_iAttackWoundedMod(0),
	m_iFlankAttackModifier(0),
	m_iNearbyEnemyCombatMod(0),
	m_iNearbyEnemyCombatRange(0),
	m_iOpenDefensePercent(0),
	m_iRoughDefensePercent(0),
	m_iExtraAttacks(0),
	m_bGreatGeneral(false),
	m_bGreatAdmiral(false),
#if defined(MOD_PROMOTIONS_AURA_CHANGE)
	m_iAuraRangeChange(0),
	m_iAuraEffectChange(0),
#endif
	m_iGreatGeneralModifier(0),
	m_bGreatGeneralReceivesMovement(false),
	m_iGreatGeneralCombatModifier(0),
	m_iFriendlyLandsModifier(0),
	m_iFriendlyLandsAttackModifier(0),
	m_iOutsideFriendlyLandsModifier(0),
	m_iCommandType(NO_COMMAND),
#if defined(MOD_UNITS_NO_SUPPLY)
	m_bNoSupply(false),
#endif
#if defined(MOD_UNITS_MAX_HP)
	m_iMaxHitPointsChange(0),
	m_iMaxHitPointsModifier(0),
#endif

#if defined(MOD_DEFENSE_MOVES_BONUS)
	m_iMoveLeftDefenseMod(0),
	m_iMoveUsedDefenseMod(0),
#endif

#if defined(MOD_ROG_CORE)
	m_iMoveLfetAttackMod(0),
	m_iMoveUsedAttackMod(0),
	m_iGoldenAgeMod(0),
	m_iRangedSupportFireMod(0),

	m_iMeleeDefenseMod(0),
#endif

#if defined(MOD_ROG_CORE)
	m_iAoEDamageOnMove(0),
	m_iForcedDamageValue(0),
	m_iChangeDamageValue(0),
	m_iAttackFullyHealedMod(0),
	m_iAttackAboveHealthMod(0),
	m_iAttackBelowHealthMod(0),
	m_bStrongerDamaged(false),
	m_bFightWellDamaged(false),
#endif


#if defined(MOD_ROG_CORE)
	m_iNearbyUnitClassBonus(0),
	m_iNearbyUnitClassBonusRange(0),
	m_iCombatBonusFromNearbyUnitClass(NO_UNITCLASS),

	m_iAOEDamageOnKill(0),
	m_iDamageAoEFortified(0),
	m_iWorkRateMod(0),
	m_iBarbarianCombatBonus(0),
#endif

	m_iCaptureDefeatedEnemyChance(0),
	m_bCannotBeCaptured(false),

#if defined(MOD_ROG_CORE)
	m_iHPHealedIfDefeatEnemyGlobal(0),
	m_iNumOriginalCapitalAttackMod(0),
	m_iNumOriginalCapitalDefenseMod(0),
#endif

#if defined(MOD_ROG_CORE)
	m_iOnCapitalLandAttackMod(0),
	m_iOutsideCapitalLandAttackMod(0),
	m_iOnCapitalLandDefenseMod(0),
	m_iOutsideCapitalLandDefenseMod(0),
#endif


#if defined(MOD_ROG_CORE)
	m_iNumSpyDefenseMod(0),
	m_iNumSpyAttackMod(0),

	m_iNumWonderDefenseMod(0),
	m_iNumWonderAttackMod(0), 

	m_iNumWorkDefenseMod(0),
	m_iNumWorkAttackMod(0),

	m_bNoResourcePunishment(false),


	m_iCurrentHitPointAttackMod(0),
	m_iCurrentHitPointDefenseMod(0),
#endif

	m_iUpgradeDiscount(0),
	m_iExperiencePercent(0),
	m_iAdjacentMod(0),
	m_iAttackMod(0),
	m_iDefenseMod(0),
	m_iDropRange(0),
	m_iExtraNavalMoves(0),
	m_iHPHealedIfDefeatEnemy(0),
	m_iGoldenAgeValueFromKills(0),
	m_iExtraWithdrawal(0),
	m_iEmbarkExtraVisibility(0),
	m_iEmbarkDefenseModifier(0),
	m_iCapitalDefenseModifier(0),
	m_iCapitalDefenseFalloff(0),
	m_iCityAttackPlunderModifier(0),
#if defined(MOD_PROMOTION_GET_INSTANCE_FROM_ATTACK)
	m_iUnitAttackFaithBonus(0),
	m_iCityAttackFaithBonus(0),
#endif
	m_iReligiousStrengthLossRivalTerritory(0),
	m_iTradeMissionInfluenceModifier(0),
	m_iTradeMissionGoldModifier(0),
	m_bCannotBeChosen(false),
	m_bLostWithUpgrade(false),
	m_bNotWithUpgrade(false),
	m_bInstaHeal(false),
	m_bLeader(false),
	m_bBlitz(false),
	m_bAmphib(false),
	m_bRiver(false),
	m_bEnemyRoute(false),
	m_bRivalTerritory(false),
	m_bMustSetUpToRangedAttack(false),
	m_bRangedSupportFire(false),
	m_bAlwaysHeal(false),
	m_bHealOutsideFriendly(false),
	m_bHillsDoubleMove(false),
	m_bIgnoreTerrainCost(false),
#if defined(MOD_API_PLOT_BASED_DAMAGE)
	m_bIgnoreTerrainDamage(false),
	m_bIgnoreFeatureDamage(false),
	m_bExtraTerrainDamage(false),
	m_bExtraFeatureDamage(false),
#endif
#if defined(MOD_PROMOTIONS_IMPROVEMENT_BONUS)
	m_iNearbyImprovementCombatBonus(0),
	m_iNearbyImprovementBonusRange(0),
	m_eCombatBonusImprovement(NO_IMPROVEMENT),
#endif
#if defined(MOD_PROMOTIONS_ALLYCITYSTATE_BONUS)
	m_iAllyCityStateCombatModifier(0),
	m_iAllyCityStateCombatModifierMax(0),
#endif
#if defined(MOD_PROMOTIONS_EXTRARES_BONUS)
	m_eExtraResourceType(NO_RESOURCE),
	m_iExtraResourceCombatModifier(0),
	m_iExtraResourceCombatModifierMax(0),
	m_iExtraHappinessCombatModifier(0),
	m_iExtraHappinessCombatModifierMax(0),
#endif
#if defined(MOD_PROMOTIONS_CROSS_MOUNTAINS)
	m_bCanCrossMountains(false),
#endif
#if defined(MOD_PROMOTIONS_CROSS_OCEANS)
	m_bCanCrossOceans(false),
#endif
#if defined(MOD_PROMOTIONS_CROSS_ICE)
	m_bCanCrossIce(false),
#endif
#if defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
	m_bGGFromBarbarians(false),
#endif
	m_bRoughTerrainEndsTurn(false),
	m_bHoveringUnit(false),
	m_bFlatMovementCost(false),
	m_bCanMoveImpassable(false),
	m_bNoCapture(false),
	m_bOnlyDefensive(false),
	m_bNoDefensiveBonus(false),
	m_bNukeImmune(false),
	m_bHiddenNationality(false),
	m_bAlwaysHostile(false),
	m_bNoRevealMap(false),
	m_bRecon(false),
	m_bCanMoveAllTerrain(false),
	m_bCanMoveAfterAttacking(false),
	m_bAirSweepCapable(false),
	m_bAllowsEmbarkation(false),
	m_bRangeAttackIgnoreLOS(false),
	m_bFreePillageMoves(false),
	m_bHealOnPillage(false),
	m_bHealIfDefeatExcludesBarbarians(false),
	m_bEmbarkedAllWater(false),
#if defined(MOD_PROMOTIONS_DEEP_WATER_EMBARKATION)
	m_bEmbarkedDeepWater(false),
#endif
	m_bCityAttackOnly(false),
	m_bCaptureDefeatedEnemy(false),
	m_bIgnoreGreatGeneralBenefit(false),
	m_bIgnoreZOC(false),
	m_bImmueMeleeAttack(false),
	m_bHasPostCombatPromotions(false),
	m_bPostCombatPromotionsExclusive(false),
	m_bSapper(false),
	m_bCanHeavyCharge(false),
	m_piTerrainAttackPercent(NULL),
	m_piTerrainDefensePercent(NULL),
	m_piFeatureAttackPercent(NULL),
	m_piFeatureDefensePercent(NULL),
#if defined(MOD_API_UNIFIED_YIELDS)
	m_piYieldFromKills(NULL),
	m_piYieldFromBarbarianKills(NULL),
#endif
	m_piUnitCombatModifierPercent(NULL),
	m_piUnitClassModifierPercent(NULL),
	m_piDomainModifierPercent(NULL),
	m_piFeaturePassableTech(NULL),
	m_piUnitClassAttackModifier(NULL),
	m_piUnitClassDefenseModifier(NULL),
	m_pbTerrainDoubleMove(NULL),
	m_pbFeatureDoubleMove(NULL),
#if defined(MOD_PROMOTIONS_HALF_MOVE)
	m_pbTerrainHalfMove(NULL),
	m_pbFeatureHalfMove(NULL),
#endif

	m_piDomainAttackPercent(NULL),
	m_piDomainDefensePercent(NULL),


	m_pbTerrainImpassable(NULL),
	m_piTerrainPassableTech(NULL),
	m_pbFeatureImpassable(NULL),
	m_pbUnitCombat(NULL),
	m_pbCivilianUnitType(NULL),


	m_pbUnitType(NULL),



#if defined(MOD_PROMOTIONS_UNIT_NAMING)
	m_pbUnitName(NULL),
#endif
	m_pbPostCombatRandomPromotion(NULL)
{
}

/// Destructor
CvPromotionEntry::~CvPromotionEntry(void)
{
	SAFE_DELETE_ARRAY(m_piTerrainAttackPercent);
	SAFE_DELETE_ARRAY(m_piTerrainDefensePercent);
	SAFE_DELETE_ARRAY(m_piFeatureAttackPercent);
	SAFE_DELETE_ARRAY(m_piFeatureDefensePercent);
#if defined(MOD_API_UNIFIED_YIELDS)
	SAFE_DELETE_ARRAY(m_piYieldFromKills);
	SAFE_DELETE_ARRAY(m_piYieldFromBarbarianKills);
#endif
	SAFE_DELETE_ARRAY(m_piUnitCombatModifierPercent);
	SAFE_DELETE_ARRAY(m_piUnitClassModifierPercent);
	SAFE_DELETE_ARRAY(m_piDomainModifierPercent);
	SAFE_DELETE_ARRAY(m_piFeaturePassableTech);
	SAFE_DELETE_ARRAY(m_piUnitClassAttackModifier);
	SAFE_DELETE_ARRAY(m_piUnitClassDefenseModifier);
	SAFE_DELETE_ARRAY(m_pbTerrainDoubleMove);
	SAFE_DELETE_ARRAY(m_pbFeatureDoubleMove);
#if defined(MOD_PROMOTIONS_HALF_MOVE)
	SAFE_DELETE_ARRAY(m_pbTerrainHalfMove);
	SAFE_DELETE_ARRAY(m_pbFeatureHalfMove);
#endif

	SAFE_DELETE_ARRAY(m_piDomainAttackPercent);
	SAFE_DELETE_ARRAY(m_piDomainDefensePercent);


	SAFE_DELETE_ARRAY(m_pbTerrainImpassable);
	SAFE_DELETE_ARRAY(m_piTerrainPassableTech);
	SAFE_DELETE_ARRAY(m_pbFeatureImpassable);
	SAFE_DELETE_ARRAY(m_pbUnitCombat);
	SAFE_DELETE_ARRAY(m_pbCivilianUnitType);

	SAFE_DELETE_ARRAY(m_pbUnitType);


#if defined(MOD_PROMOTIONS_UNIT_NAMING)
	SAFE_DELETE_ARRAY(m_pbUnitName);
#endif
	SAFE_DELETE_ARRAY(m_pbPostCombatRandomPromotion);
}
//------------------------------------------------------------------------------
bool CvPromotionEntry::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if(!CvHotKeyInfo::CacheResults(kResults, kUtility))
		return false;

	SetSound(kResults.GetText("Sound"));

	//Basic Properties
	m_bCannotBeChosen = kResults.GetBool("CannotBeChosen");
	m_bLostWithUpgrade = kResults.GetBool("LostWithUpgrade");
	m_bNotWithUpgrade = kResults.GetBool("NotWithUpgrade");
	m_bInstaHeal = kResults.GetBool("InstaHeal");
	m_bLeader = kResults.GetBool("Leader");
	m_bBlitz = kResults.GetBool("Blitz");
	m_bAmphib = kResults.GetBool("Amphib");
	m_bRiver = kResults.GetBool("River");
	m_bEnemyRoute = kResults.GetBool("EnemyRoute");
	m_bRivalTerritory = kResults.GetBool("RivalTerritory");
	m_bMustSetUpToRangedAttack = kResults.GetBool("MustSetUpToRangedAttack");
	m_bRangedSupportFire= kResults.GetBool("RangedSupportFire");
	m_bAlwaysHeal = kResults.GetBool("AlwaysHeal");
	m_bHealOutsideFriendly = kResults.GetBool("HealOutsideFriendly");
	m_bHillsDoubleMove = kResults.GetBool("HillsDoubleMove");
	m_bIgnoreTerrainCost = kResults.GetBool("IgnoreTerrainCost");
#if defined(MOD_API_PLOT_BASED_DAMAGE)
	m_bIgnoreTerrainDamage = kResults.GetBool("IgnoreTerrainDamage");
	m_bIgnoreFeatureDamage = kResults.GetBool("IgnoreFeatureDamage");
	m_bExtraTerrainDamage = kResults.GetBool("ExtraTerrainDamage");
	m_bExtraFeatureDamage = kResults.GetBool("ExtraFeatureDamage");
#endif
#if defined(MOD_PROMOTIONS_IMPROVEMENT_BONUS)
	if (MOD_PROMOTIONS_IMPROVEMENT_BONUS) {
		m_iNearbyImprovementCombatBonus = kResults.GetInt("NearbyImprovementCombatBonus");
		m_iNearbyImprovementBonusRange = kResults.GetInt("NearbyImprovementBonusRange");
		const char* szTextVal = kResults.GetText("CombatBonusImprovement");
		if (szTextVal) {
			m_eCombatBonusImprovement = (ImprovementTypes)GC.getInfoTypeForString(szTextVal, true);
		}
	}
#endif

#if defined(MOD_PROMOTIONS_ALLYCITYSTATE_BONUS)
	if (MOD_PROMOTIONS_ALLYCITYSTATE_BONUS) {
		m_iAllyCityStateCombatModifier = kResults.GetInt("AllyCityStateCombatModifier");
		m_iAllyCityStateCombatModifierMax = kResults.GetInt("AllyCityStateCombatModifierMax");
	}
#endif

#if defined(MOD_PROMOTIONS_EXTRARES_BONUS)
	if (MOD_PROMOTIONS_EXTRARES_BONUS) {
		const char* szTextVal = kResults.GetText("ExtraResourceType");
		if (szTextVal) {
			m_eExtraResourceType = (ResourceTypes)GC.getInfoTypeForString(szTextVal, true);
		}
		m_iExtraResourceCombatModifier = kResults.GetInt("ExtraResourceCombatModifier");
		m_iExtraResourceCombatModifierMax = kResults.GetInt("ExtraResourceCombatModifierMax");
		m_iExtraHappinessCombatModifier = kResults.GetInt("ExtraHappinessCombatModifier");
		m_iExtraHappinessCombatModifierMax = kResults.GetInt("ExtraHappinessCombatModifierMax");
	}
#endif

#if defined(MOD_DEFENSE_MOVES_BONUS)
	if (MOD_DEFENSE_MOVES_BONUS) {
		m_iMoveLeftDefenseMod = kResults.GetInt("MoveLeftDefenseMod");
		m_iMoveUsedDefenseMod = kResults.GetInt("MoveUsedDefenseMod");
	}
#endif

#if defined(MOD_ROG_CORE)
	if (MOD_ROG_CORE) {
		m_iNearbyUnitClassBonus = kResults.GetInt("NearbyUnitClassBonus");
		m_iNearbyUnitClassBonusRange = kResults.GetInt("NearbyUnitClassBonusRange");
		const char* szTextVal = kResults.GetText("CombatBonusFromNearbyUnitClass");
		if (szTextVal) {
			m_iCombatBonusFromNearbyUnitClass = (UnitClassTypes)GC.getInfoTypeForString(szTextVal, true);
		}
	}
#endif

#if defined(MOD_PROMOTIONS_CROSS_MOUNTAINS)
	if (MOD_PROMOTIONS_CROSS_MOUNTAINS) {
		m_bCanCrossMountains = kResults.GetBool("CanCrossMountains");
	}
#endif
#if defined(MOD_PROMOTIONS_CROSS_OCEANS)
	if (MOD_PROMOTIONS_CROSS_OCEANS) {
		m_bCanCrossOceans = kResults.GetBool("CanCrossOceans");
	}
#endif
#if defined(MOD_PROMOTIONS_CROSS_ICE)
	if (MOD_PROMOTIONS_CROSS_ICE) {
		m_bCanCrossIce = kResults.GetBool("CanCrossIce");
	}
#endif
#if defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
	if (MOD_PROMOTIONS_GG_FROM_BARBARIANS) {
		m_bGGFromBarbarians = kResults.GetBool("GGFromBarbarians");
	}
#endif

#if defined(MOD_ROG_CORE)
	m_iAoEDamageOnMove = kResults.GetInt("AoEDamageOnMove");
	m_iForcedDamageValue = kResults.GetInt("ForcedDamageValue");
	m_iChangeDamageValue = kResults.GetInt("ChangeDamageValue");
	m_iAttackFullyHealedMod = kResults.GetInt("AttackFullyHealedMod");
	m_iAttackAboveHealthMod = kResults.GetInt("AttackAbove50HealthMod");
	m_iAttackBelowHealthMod = kResults.GetInt("AttackBelowEqual50HealthMod");
	m_bStrongerDamaged = kResults.GetBool("StrongerDamaged");
	m_bFightWellDamaged = kResults.GetBool("FightWellDamaged");

	m_iMeleeDefenseMod = kResults.GetInt("MeleeDefenseMod");
#endif

#if defined(MOD_ROG_CORE)
	if (MOD_ROG_CORE) {
		m_iMoveLfetAttackMod = kResults.GetInt("MoveLfetAttackMod");
		m_iMoveUsedAttackMod = kResults.GetInt("MoveUsedAttackMod");
		m_iGoldenAgeMod = kResults.GetInt("GoldenAgeMod");
		m_iRangedSupportFireMod = kResults.GetInt("RangedSupportFireMod");
	}
#endif

#if defined(MOD_ROG_CORE)
     	m_iHPHealedIfDefeatEnemyGlobal = kResults.GetInt("HPHealedIfDestroyEnemyGlobal");
		m_iNumOriginalCapitalAttackMod = kResults.GetInt("NumOriginalCapitalAttackMod");
		m_iNumOriginalCapitalDefenseMod = kResults.GetInt("NumOriginalCapitalDefenseMod");
#endif


#if defined(MOD_ROG_CORE)
		m_iOnCapitalLandAttackMod = kResults.GetInt("OnCapitalLandAttackMod");
		m_iOutsideCapitalLandAttackMod = kResults.GetInt("OutsideCapitalLandAttackMod");
		m_iOnCapitalLandDefenseMod = kResults.GetInt("OnCapitalLandDefenseMod");
		m_iOutsideCapitalLandDefenseMod = kResults.GetInt("OutsideCapitalLandDefenseMod");


		m_iDamageAoEFortified = kResults.GetInt("AoEWhileFortified");
		m_iWorkRateMod = kResults.GetInt("WorkRateMod");
		m_iAOEDamageOnKill = kResults.GetInt("AOEDamageOnKill");

		m_iBarbarianCombatBonus = kResults.GetInt("BarbarianCombatBonus");
#endif

		m_iCaptureDefeatedEnemyChance = kResults.GetInt("CaptureDefeatedEnemyChance");
		m_bCannotBeCaptured = kResults.GetBool("CannotBeCaptured");

#if defined(MOD_ROG_CORE)
			m_iNumSpyDefenseMod = kResults.GetInt("NumSpyDefenseMod");
			m_iNumSpyAttackMod = kResults.GetInt("NumSpyAttackMod");

			m_iNumWonderDefenseMod = kResults.GetInt("NumWonderDefenseMod");
			m_iNumWonderAttackMod = kResults.GetInt("NumWonderAttackMod");

			m_iNumWorkDefenseMod = kResults.GetInt("NumWorkDefenseMod");
			m_iNumWorkAttackMod = kResults.GetInt("NumWorkAttackMod");

			m_bNoResourcePunishment = kResults.GetBool("NoResourcePunishment");

			m_iCurrentHitPointAttackMod = kResults.GetInt("CurrentHitPointAttackMod");
			m_iCurrentHitPointDefenseMod = kResults.GetInt("CurrentHitPointDefenseMod");

			m_iNearNumEnemyAttackMod = kResults.GetInt("NearNumEnemyAttackMod");
			m_iNearNumEnemyDefenseMod = kResults.GetInt("NearNumEnemyDefenseMod");
#endif

	m_bRoughTerrainEndsTurn = kResults.GetBool("RoughTerrainEndsTurn");
	m_bHoveringUnit = kResults.GetBool("HoveringUnit");
	m_bFlatMovementCost = kResults.GetBool("FlatMovementCost");
	m_bCanMoveImpassable = kResults.GetBool("CanMoveImpassable");
	m_bNoCapture = kResults.GetBool("NoCapture");
	m_bOnlyDefensive = kResults.GetBool("OnlyDefensive");
	m_bNoDefensiveBonus = kResults.GetBool("NoDefensiveBonus");
	m_bNukeImmune = kResults.GetBool("NukeImmune");
	m_bHiddenNationality = kResults.GetBool("HiddenNationality");
	m_bAlwaysHostile = kResults.GetBool("AlwaysHostile");
	m_bNoRevealMap = kResults.GetBool("NoRevealMap");
	m_bRecon = kResults.GetBool("Recon");
	m_bCanMoveAllTerrain = kResults.GetBool("CanMoveAllTerrain");
	m_bCanMoveAfterAttacking = kResults.GetBool("CanMoveAfterAttacking");
	m_bAirSweepCapable = kResults.GetBool("AirSweepCapable");
	m_bAllowsEmbarkation = kResults.GetBool("AllowsEmbarkation");
	m_bRangeAttackIgnoreLOS = kResults.GetBool("RangeAttackIgnoreLOS");
	m_bFreePillageMoves = kResults.GetBool("FreePillageMoves");
	m_bHealOnPillage = kResults.GetBool("HealOnPillage");
	m_bHealIfDefeatExcludesBarbarians = kResults.GetBool("HealIfDestroyExcludesBarbarians");
	m_bEmbarkedAllWater = kResults.GetBool("EmbarkedAllWater");
#if defined(MOD_PROMOTIONS_DEEP_WATER_EMBARKATION)
	m_bEmbarkedDeepWater = kResults.GetBool("EmbarkedDeepWater");
#endif
	m_bCityAttackOnly = kResults.GetBool("CityAttackOnly");
	m_bCaptureDefeatedEnemy = kResults.GetBool("CaptureDefeatedEnemy");
	m_bIgnoreGreatGeneralBenefit = kResults.GetBool("IgnoreGreatGeneralBenefit");
	m_bIgnoreZOC = kResults.GetBool("IgnoreZOC");
	m_bImmueMeleeAttack = kResults.GetBool("ImmueMeleeAttack");
	m_bHasPostCombatPromotions = kResults.GetBool("HasPostCombatPromotions");
	m_bPostCombatPromotionsExclusive = kResults.GetBool("PostCombatPromotionsExclusive");
	m_bSapper = kResults.GetBool("Sapper");
	m_bCanHeavyCharge = kResults.GetBool("HeavyCharge");

#if defined(MOD_API_UNIT_CANNOT_BE_RANGED_ATTACKED)
	if (MOD_API_UNIT_CANNOT_BE_RANGED_ATTACKED)
	{
		m_bCannotBeRangedAttacked = kResults.GetBool("CannotBeRangedAttacked");
	}
#endif

	m_iVisibilityChange = kResults.GetInt("VisibilityChange");
#if defined(MOD_PROMOTIONS_VARIABLE_RECON)
	m_iReconChange = kResults.GetInt("ReconChange");
#endif
	m_iMovesChange = kResults.GetInt("MovesChange");
	m_iMoveDiscountChange = kResults.GetInt("MoveDiscountChange");
	m_iRangeChange = kResults.GetInt("RangeChange");
	m_iRangedAttackModifier = kResults.GetInt("RangedAttackModifier");
	m_iInterceptionCombatModifier = kResults.GetInt("InterceptionCombatModifier");
	m_iInterceptionDefenseDamageModifier = kResults.GetInt("InterceptionDefenseDamageModifier");
	m_iAirSweepCombatModifier = kResults.GetInt("AirSweepCombatModifier");
	m_iInterceptChanceChange = kResults.GetInt("InterceptChanceChange");
	m_iNumInterceptionChange = kResults.GetInt("NumInterceptionChange");
	m_iEvasionChange = kResults.GetInt("EvasionChange");
	m_iCargoChange = kResults.GetInt("CargoChange");
	m_iEnemyHealChange = kResults.GetInt("EnemyHealChange");
	m_iNeutralHealChange = kResults.GetInt("NeutralHealChange");
	m_iFriendlyHealChange = kResults.GetInt("FriendlyHealChange");
	m_iSameTileHealChange = kResults.GetInt("SameTileHealChange");
	m_iAdjacentTileHealChange = kResults.GetInt("AdjacentTileHealChange");
	m_iEnemyDamageChance = kResults.GetInt("EnemyDamageChance");
	m_iNeutralDamageChance = kResults.GetInt("NeutralDamageChance");
	m_iEnemyDamage = kResults.GetInt("EnemyDamage");
	m_iNeutralDamage = kResults.GetInt("NeutralDamage");
	m_iCombatPercent = kResults.GetInt("CombatPercent");
	m_iCityAttackPercent = kResults.GetInt("CityAttack");
	m_iCityDefensePercent = kResults.GetInt("CityDefense");
	m_iRangedDefenseMod = kResults.GetInt("RangedDefenseMod");
	m_iHillsAttackPercent = kResults.GetInt("HillsAttack");
	m_iHillsDefensePercent = kResults.GetInt("HillsDefense");
	m_iOpenAttackPercent = kResults.GetInt("OpenAttack");
	m_iOpenRangedAttackMod = kResults.GetInt("OpenRangedAttackMod");
	m_iRoughAttackPercent = kResults.GetInt("RoughAttack");
	m_iRoughRangedAttackMod = kResults.GetInt("RoughRangedAttackMod");
	m_iAttackFortifiedMod = kResults.GetInt("AttackFortifiedMod");
	m_iAttackWoundedMod = kResults.GetInt("AttackWoundedMod");
	m_iFlankAttackModifier = kResults.GetInt("FlankAttackModifier");
	m_iNearbyEnemyCombatMod = kResults.GetInt("NearbyEnemyCombatMod");
	m_iNearbyEnemyCombatRange = kResults.GetInt("NearbyEnemyCombatRange");
	m_iOpenDefensePercent = kResults.GetInt("OpenDefense");
	m_iRoughDefensePercent = kResults.GetInt("RoughDefense");
	m_iExtraAttacks = kResults.GetInt("ExtraAttacks");
	m_bGreatGeneral = kResults.GetBool("GreatGeneral");
	m_bGreatAdmiral = kResults.GetBool("GreatAdmiral");
#if defined(MOD_PROMOTIONS_AURA_CHANGE)
	if (MOD_PROMOTIONS_AURA_CHANGE) {
		m_iAuraRangeChange = kResults.GetInt("AuraRangeChange");
		m_iAuraEffectChange = kResults.GetInt("AuraEffectChange");
	}
#endif
	m_iGreatGeneralModifier = kResults.GetInt("GreatGeneralModifier");
	m_bGreatGeneralReceivesMovement = kResults.GetBool("GreatGeneralReceivesMovement");
	m_iGreatGeneralCombatModifier = kResults.GetInt("GreatGeneralCombatModifier");
	m_iFriendlyLandsModifier = kResults.GetInt("FriendlyLandsModifier");
	m_iFriendlyLandsAttackModifier = kResults.GetInt("FriendlyLandsAttackModifier");
	m_iOutsideFriendlyLandsModifier = kResults.GetInt("OutsideFriendlyLandsModifier");
#if defined(MOD_UNITS_NO_SUPPLY)
	if (MOD_UNITS_NO_SUPPLY) {
		m_bNoSupply = (kResults.GetInt("NoSupply") != 0);
	}
#endif
#if defined(MOD_UNITS_MAX_HP)
	if (MOD_UNITS_MAX_HP) {
		m_iMaxHitPointsChange = kResults.GetInt("MaxHitPointsChange");
		m_iMaxHitPointsModifier = kResults.GetInt("MaxHitPointsModifier");
	}
#endif
	m_iUpgradeDiscount = kResults.GetInt("UpgradeDiscount");
	m_iExperiencePercent = kResults.GetInt("ExperiencePercent");
	m_iAdjacentMod = kResults.GetInt("AdjacentMod");
	m_iAttackMod = kResults.GetInt("AttackMod");
	m_iDefenseMod = kResults.GetInt("DefenseMod");
	m_iDropRange = kResults.GetInt("DropRange");
	m_iExtraNavalMoves = kResults.GetInt("ExtraNavalMovement");
	m_iHPHealedIfDefeatEnemy = kResults.GetInt("HPHealedIfDestroyEnemy");
	m_iGoldenAgeValueFromKills = kResults.GetInt("GoldenAgeValueFromKills");
	m_iExtraWithdrawal = kResults.GetInt("ExtraWithdrawal");
	m_iEmbarkExtraVisibility = kResults.GetInt("EmbarkExtraVisibility");
	m_iEmbarkDefenseModifier = kResults.GetInt("EmbarkDefenseModifier");
	m_iCapitalDefenseModifier = kResults.GetInt("CapitalDefenseModifier");
	m_iCapitalDefenseFalloff = kResults.GetInt("CapitalDefenseFalloff");
	m_iCityAttackPlunderModifier = kResults.GetInt("CityAttackPlunderModifier");
#if defined(MOD_PROMOTION_GET_INSTANCE_FROM_ATTACK)
	m_iUnitAttackFaithBonus = kResults.GetInt("UnitAttackFaithBonus");
	m_iCityAttackFaithBonus = kResults.GetInt("CityAttackFaithBonus");
#endif
#if defined(MOD_PROMOTION_REMOVE_PROMOTION_UPGRADE)
	const char* szRemovePromotionUpgrade = kResults.GetText("RemovePromotionUpgrade");
	m_iRemovePromotionUpgrade = GC.getInfoTypeForString(szRemovePromotionUpgrade, true);
#endif
	m_iReligiousStrengthLossRivalTerritory = kResults.GetInt("ReligiousStrengthLossRivalTerritory");
	m_iTradeMissionInfluenceModifier = kResults.GetInt("TradeMissionInfluenceModifier");
	m_iTradeMissionGoldModifier = kResults.GetInt("TradeMissionGoldModifier");

#ifdef MOD_GLOBAL_WAR_CASUALTIES
	m_iWarCasualtiesModifier = kResults.GetInt("WarCasualtiesModifier");
#endif

#ifdef MOD_PROMOTION_SPLASH_DAMAGE
	m_iSplashDamageRadius = kResults.GetInt("SplashDamageRadius");
	m_iSplashDamagePercent = kResults.GetInt("SplashDamagePercent");
	m_iSplashDamageFixed = kResults.GetInt("SplashDamageFixed");
	m_iSplashDamagePlotUnitLimit = kResults.GetInt("SplashDamagePlotUnitLimit");
	m_iSplashDamageImmune = kResults.GetBool("SplashDamageImmune");
	m_iSplashXP = kResults.GetInt("SplashXP");
#endif

#ifdef MOD_PROMOTION_COLLATERAL_DAMAGE
	m_iCollateralDamagePercent = kResults.GetInt("CollateralDamagePercent");
	m_iCollateralDamageFixed = kResults.GetInt("CollateralDamageFixed");
	m_iCollateralDamagePlotUnitLimit = kResults.GetInt("CollateralDamagePlotUnitLimit");
	m_iCollateralDamageImmune = kResults.GetBool("CollateralDamageImmune");
	m_iCollateralXP = kResults.GetInt("CollateralXP");
	m_bCollateralOnlyCity = kResults.GetBool("CollateralOnlyCity");
	m_bCollateralOnlyUnit = kResults.GetBool("CollateralOnlyUnit");
#endif

#ifdef MOD_PROMOTION_ADD_ENEMY_PROMOTIONS
	m_bAddEnemyPromotionImmune = kResults.GetBool("AddEnemyPromotionImmune");
#endif

#ifdef MOD_GLOBAL_PROMOTIONS_REMOVAL
	m_iRemoveAfterXTurns = kResults.GetInt("RemoveAfterXTurns");
	m_bRemoveAfterFullyHeal = kResults.GetBool("RemoveAfterFullyHeal");
	m_bRemoveWithLuaCheck = kResults.GetBool("RemoveWithLuaCheck");
	m_bCanActionClear = kResults.GetBool("CanActionClear");
#endif

#ifdef MOD_PROMOTION_CITY_DESTROYER
	m_iDestroyBuildingProbability =  kResults.GetInt("DestroyBuildingProbability");
	m_iDestroyBuildingNumLimit = kResults.GetInt("DestroyBuildingNumLimit");
	const char* strDestroyBuildingCollection = kResults.GetText("DestroyBuildingCollection");
	if (strDestroyBuildingCollection != nullptr)
	{
		int iLen = strlen(strDestroyBuildingCollection);
		if (iLen > 0)
		{
			std::string sqlKey = "UnitPromotions_DestroyBuildingCollection";
			Database::Results* pResults = kUtility.GetResults(sqlKey);
			if (pResults == NULL)
			{
				const char* szSQL = "select ID from BuildingClassCollections where Type = ?;";
				pResults = kUtility.PrepareResults(sqlKey, szSQL);
			}

			pResults->Bind(1, strDestroyBuildingCollection, iLen, false);
			if (pResults->Step())
			{
				int id = pResults->GetInt(0);
				m_iDestroyBuildingCollection = (BuildingClassCollectionsTypes)id;
			}
		}
	}

	m_iSiegeKillCitizensFixed = kResults.GetInt("SiegeKillCitizensFixed");
	m_iSiegeKillCitizensPercent = kResults.GetInt("SiegeKillCitizensPercent");
#endif

	//References
	const char* szLayerAnimationPath = kResults.GetText("LayerAnimationPath");
	m_iLayerAnimationPath = GC.getInfoTypeForString(szLayerAnimationPath, true);

	const char* szTechPrereq = kResults.GetText("TechPrereq");
	m_iTechPrereq = GC.getInfoTypeForString(szTechPrereq, true);

	const char* szInvisible = kResults.GetText("Invisible");
	m_iInvisibleType = GC.getInfoTypeForString(szInvisible, true);

	const char* szSeeInvisible = kResults.GetText("SeeInvisible");
	m_iSeeInvisibleType = GC.getInfoTypeForString(szSeeInvisible, true);

#if defined(MOD_PROMOTION_FEATURE_INVISIBLE)
	const char* szFeatureInvisible = kResults.GetText("FeatureInvisible");
	m_iFeatureInvisible = GC.getInfoTypeForString(szFeatureInvisible, true);
	const char* szFeatureInvisible2 = kResults.GetText("FeatureInvisible2");
	m_iFeatureInvisible2 = GC.getInfoTypeForString(szFeatureInvisible2, true);
#endif
#if defined(MOD_PROMOTION_MULTIPLE_INIT_EXPERENCE)
	m_iMultipleInitExperence = kResults.GetInt("MultipleInitExperence");
#endif

	m_iAttackInflictDamageChange = kResults.GetInt("AttackInflictDamageChange");
	m_iAttackInflictDamageChangeMaxHPPercent = kResults.GetInt("AttackInflictDamageChangeMaxHPPercent");

	m_iDefenseInflictDamageChange = kResults.GetInt("DefenseInflictDamageChange");
	m_iDefenseInflictDamageChangeMaxHPPercent = kResults.GetInt("DefenseInflictDamageChangeMaxHPPercent");

	m_iSiegeInflictDamageChange = kResults.GetInt("SiegeInflictDamageChange");
	m_iSiegeInflictDamageChangeMaxHPPercent = kResults.GetInt("SiegeInflictDamageChangeMaxHPPercent");

	m_iHeavyChargeAddMoves = kResults.GetInt("HeavyChargeAddMoves");
	m_iHeavyChargeExtraDamage = kResults.GetInt("HeavyChargeExtraDamage");
	m_iHeavyChargeCollateralFixed = kResults.GetInt("HeavyChargeCollateralFixed");
	m_iHeavyChargeCollateralPercent = kResults.GetInt("HeavyChargeCollateralPercent");

	const char* szPromotionPrereq = kResults.GetText("PromotionPrereq");
	m_iPrereqPromotion = GC.getInfoTypeForString(szPromotionPrereq, true);

	const char* szPromotionPrereqOr1 = kResults.GetText("PromotionPrereqOr1");
	m_iPrereqOrPromotion1 = GC.getInfoTypeForString(szPromotionPrereqOr1, true);

	const char* szPromotionPrereqOr2 = kResults.GetText("PromotionPrereqOr2");
	m_iPrereqOrPromotion2 = GC.getInfoTypeForString(szPromotionPrereqOr2, true);

	const char* szPromotionPrereqOr3 = kResults.GetText("PromotionPrereqOr3");
	m_iPrereqOrPromotion3 = GC.getInfoTypeForString(szPromotionPrereqOr3, true);

	const char* szPromotionPrereqOr4 = kResults.GetText("PromotionPrereqOr4");
	m_iPrereqOrPromotion4 = GC.getInfoTypeForString(szPromotionPrereqOr4, true);

	const char* szPromotionPrereqOr5 = kResults.GetText("PromotionPrereqOr5");
	m_iPrereqOrPromotion5 = GC.getInfoTypeForString(szPromotionPrereqOr5, true);

	const char* szPromotionPrereqOr6 = kResults.GetText("PromotionPrereqOr6");
	m_iPrereqOrPromotion6 = GC.getInfoTypeForString(szPromotionPrereqOr6, true);

	const char* szPromotionPrereqOr7 = kResults.GetText("PromotionPrereqOr7");
	m_iPrereqOrPromotion7 = GC.getInfoTypeForString(szPromotionPrereqOr7, true);

	const char* szPromotionPrereqOr8 = kResults.GetText("PromotionPrereqOr8");
	m_iPrereqOrPromotion8 = GC.getInfoTypeForString(szPromotionPrereqOr8, true);

	const char* szPromotionPrereqOr9 = kResults.GetText("PromotionPrereqOr9");
	m_iPrereqOrPromotion9 = GC.getInfoTypeForString(szPromotionPrereqOr9, true);

	const char* szPromotionPrereqOr10 = kResults.GetText("PromotionPrereqOr10");
	m_iPrereqOrPromotion10 = GC.getInfoTypeForString(szPromotionPrereqOr10, true);

	const char* szPromotionPrereqOr11 = kResults.GetText("PromotionPrereqOr11");
	m_iPrereqOrPromotion11 = GC.getInfoTypeForString(szPromotionPrereqOr11, true);

	const char* szPromotionPrereqOr12 = kResults.GetText("PromotionPrereqOr12");
	m_iPrereqOrPromotion12 = GC.getInfoTypeForString(szPromotionPrereqOr12, true);

	const char* szPromotionPrereqOr13 = kResults.GetText("PromotionPrereqOr13");
	m_iPrereqOrPromotion13 = GC.getInfoTypeForString(szPromotionPrereqOr13, true);

	//Arrays
	const int iNumUnitClasses = kUtility.MaxRows("UnitClasses");
	const int iNumTerrains = GC.getNumTerrainInfos();
	const int iNumFeatures = GC.getNumFeatureInfos();
	const int iNumDomains = kUtility.MaxRows("Domains");
	DEBUG_VARIABLE(iNumDomains);
	const int iNumUnitCombatClasses = kUtility.MaxRows("UnitCombatInfos");
	const int iNumUnitTypes = kUtility.MaxRows("Units");

	const char* szPromotionType = GetType();

	//UnitPromotions_Terrains
	{
		kUtility.InitializeArray(m_piTerrainAttackPercent, iNumTerrains, 0);
		kUtility.InitializeArray(m_piTerrainDefensePercent, iNumTerrains, 0);
		kUtility.InitializeArray(m_piTerrainPassableTech, iNumTerrains, NO_TECH);
		kUtility.InitializeArray(m_pbTerrainDoubleMove, iNumTerrains, false);
#if defined(MOD_PROMOTIONS_HALF_MOVE)
		kUtility.InitializeArray(m_pbTerrainHalfMove, iNumTerrains, false);
#endif
		kUtility.InitializeArray(m_pbTerrainImpassable, iNumTerrains, false);

		std::string sqlKey = "UnitPromotions_Terrains";
		Database::Results* pResults = kUtility.GetResults(sqlKey);
		if(pResults == NULL)
		{
			const char* szSQL = "select Terrains.ID as TerrainID, coalesce(Technologies.ID, -1) as PassableTechID, UnitPromotions_Terrains.* from UnitPromotions_Terrains inner join Terrains on TerrainType = Terrains.Type left join Technologies on PassableTech = Technologies.Type where PromotionType = ?";
			pResults = kUtility.PrepareResults(sqlKey, szSQL);
		}

		CvAssert(pResults);
		if(!pResults) return false;

		pResults->Bind(1, szPromotionType);

		while(pResults->Step())
		{
			const int iTerrainID = pResults->GetInt("TerrainID");
			CvAssert(iTerrainID > -1 && iTerrainID < iNumTerrains);

			const int iTerrainAttack = pResults->GetInt("Attack");
			m_piTerrainAttackPercent[iTerrainID] = iTerrainAttack;

			const int iTerrainDefense = pResults->GetInt("Defense");
			m_piTerrainDefensePercent[iTerrainID] = iTerrainDefense;

			const bool bDoubleMove = pResults->GetBool("DoubleMove");
			m_pbTerrainDoubleMove[iTerrainID] = bDoubleMove;

#if defined(MOD_PROMOTIONS_HALF_MOVE)
			const bool bHalfMove = pResults->GetBool("HalfMove");
			m_pbTerrainHalfMove[iTerrainID] = bHalfMove;
#endif

			const bool bImpassable = pResults->GetBool("Impassable");
			m_pbTerrainImpassable[iTerrainID] = bImpassable;

			const int iPassableTechID = pResults->GetInt("PassableTechID");
			m_piTerrainPassableTech[iTerrainID] = iPassableTechID;
		}
	}

	//UnitPromotions_Features
	{
		kUtility.InitializeArray(m_piFeatureAttackPercent, iNumFeatures, 0);
		kUtility.InitializeArray(m_piFeatureDefensePercent, iNumFeatures, 0);
		kUtility.InitializeArray(m_piFeaturePassableTech, iNumFeatures, NO_TECH);
		kUtility.InitializeArray(m_pbFeatureDoubleMove, iNumFeatures, false);
#if defined(MOD_PROMOTIONS_HALF_MOVE)
		kUtility.InitializeArray(m_pbFeatureHalfMove, iNumFeatures, false);
#endif
		kUtility.InitializeArray(m_pbFeatureImpassable, iNumFeatures, false);

		std::string sqlKey = "UnitPromotions_Features";
		Database::Results* pResults = kUtility.GetResults(sqlKey);
		if(pResults == NULL)
		{
			const char* szSQL = "select Features.ID as FeatureID, coalesce(Technologies.ID, -1) as PassableTechID, UnitPromotions_Features.* from UnitPromotions_Features inner join Features on FeatureType = Features.Type left join Technologies on PassableTech = Technologies.Type where PromotionType = ?";
			pResults = kUtility.PrepareResults(sqlKey, szSQL);
		}

		CvAssert(pResults);
		if(!pResults) return false;

		pResults->Bind(1, szPromotionType);

		while(pResults->Step())
		{
			const int iFeatureID = pResults->GetInt("FeatureID");
			CvAssert(iFeatureID > -1 && iFeatureID < iNumFeatures);

			const int iFeatureAttack = pResults->GetInt("Attack");
			m_piFeatureAttackPercent[iFeatureID] = iFeatureAttack;

			const int iFeatureDefense = pResults->GetInt("Defense");
			m_piFeatureDefensePercent[iFeatureID] = iFeatureDefense;

			const bool bDoubleMove = pResults->GetBool("DoubleMove");
			m_pbFeatureDoubleMove[iFeatureID] = bDoubleMove;

#if defined(MOD_PROMOTIONS_HALF_MOVE)
			const bool bHalfMove = pResults->GetBool("HalfMove");
			m_pbFeatureHalfMove[iFeatureID] = bHalfMove;
#endif

			const bool bImpassable = pResults->GetBool("Impassable");
			m_pbFeatureImpassable[iFeatureID] = bImpassable;

			const int iPassableTech = pResults->GetInt("PassableTechID");
			m_piFeaturePassableTech[iFeatureID] = iPassableTech;
		}
	}

#if defined(MOD_API_UNIFIED_YIELDS)
	//UnitPromotions_YieldFromKills
	{
		kUtility.InitializeArray(m_piYieldFromKills, NUM_YIELD_TYPES, 0);

		std::string sqlKey = "UnitPromotions_YieldFromKills";
		Database::Results* pResults = kUtility.GetResults(sqlKey);
		if(pResults == NULL)
		{
			const char* szSQL = "select Yields.ID as YieldID, UnitPromotions_YieldFromKills.* from UnitPromotions_YieldFromKills inner join Yields on YieldType = Yields.Type where PromotionType = ?";
			pResults = kUtility.PrepareResults(sqlKey, szSQL);
		}

		CvAssert(pResults);
		if(!pResults) return false;

		pResults->Bind(1, szPromotionType);

		while(pResults->Step())
		{
			const int iYieldID = pResults->GetInt("YieldID");
			CvAssert(iYieldID > -1 && iYieldID < iNumYields);

			const int iYield = pResults->GetInt("Yield");
			m_piYieldFromKills[iYieldID] = iYield;
		}
	}

	//UnitPromotions_YieldFromBarbarianKills
	{
		kUtility.InitializeArray(m_piYieldFromBarbarianKills, NUM_YIELD_TYPES, 0);

		std::string sqlKey = "UnitPromotions_YieldFromBarbarianKills";
		Database::Results* pResults = kUtility.GetResults(sqlKey);
		if(pResults == NULL)
		{
			const char* szSQL = "select Yields.ID as YieldID, UnitPromotions_YieldFromBarbarianKills.* from UnitPromotions_YieldFromBarbarianKills inner join Yields on YieldType = Yields.Type where PromotionType = ?";
			pResults = kUtility.PrepareResults(sqlKey, szSQL);
		}

		CvAssert(pResults);
		if(!pResults) return false;

		pResults->Bind(1, szPromotionType);

		while(pResults->Step())
		{
			const int iYieldID = pResults->GetInt("YieldID");
			CvAssert(iYieldID > -1 && iYieldID < iNumYields);

			const int iYield = pResults->GetInt("Yield");
			m_piYieldFromBarbarianKills[iYieldID] = iYield;
		}
	}
#endif

	//UnitPromotions_UnitClasses
	{
		kUtility.InitializeArray(m_piUnitClassModifierPercent, iNumUnitClasses, 0);
		kUtility.InitializeArray(m_piUnitClassAttackModifier, iNumUnitClasses, 0);
		kUtility.InitializeArray(m_piUnitClassDefenseModifier, iNumUnitClasses, 0);

		std::string sqlKey = "UnitPromotions_UnitClasses";
		Database::Results* pResults = kUtility.GetResults(sqlKey);
		if(pResults == NULL)
		{
			const char* szSQL = "select UnitClasses.ID, Modifier, Attack, Defense from UnitPromotions_UnitClasses inner join UnitClasses on UnitClassType = UnitClasses.Type where PromotionType = ?";
			pResults = kUtility.PrepareResults(sqlKey, szSQL);
		}

		CvAssert(pResults);
		if(!pResults) return false;

		pResults->Bind(1, szPromotionType);

		while(pResults->Step())
		{
			const int iUnitClassID = pResults->GetInt(0);
			CvAssert(iUnitClassID > -1 && iUnitClassID  < iNumUnitClasses);

			const int iModifier = pResults->GetInt("Modifier");
			m_piUnitClassModifierPercent[iUnitClassID] = iModifier;

			const int iAttack = pResults->GetInt("Attack");
			m_piUnitClassAttackModifier[iUnitClassID] = iAttack;

			const int iDefense = pResults->GetInt("Defense");
			m_piUnitClassDefenseModifier[iUnitClassID] = iDefense;
		}

		pResults->Reset();
	}


	//UnitPromotions_Domains
	{
		kUtility.InitializeArray(m_piDomainModifierPercent, NUM_DOMAIN_TYPES, 0);
		kUtility.InitializeArray(m_piDomainAttackPercent, NUM_DOMAIN_TYPES, 0);
		kUtility.InitializeArray(m_piDomainDefensePercent, NUM_DOMAIN_TYPES, 0);

		std::string sqlKey = "UnitPromotions_Domains";
		Database::Results* pResults = kUtility.GetResults(sqlKey);
		if (pResults == NULL)
		{
			const char* szSQL = "select Domains.ID, Modifier, Attack, Defense from UnitPromotions_Domains inner join Domains on DomainType = Domains.Type where PromotionType = ?;";
			pResults = kUtility.PrepareResults(sqlKey, szSQL);
		}

		CvAssert(pResults);
		if (!pResults) return false;

		pResults->Bind(1, szPromotionType);

		while (pResults->Step())
		{
			const int iDomainID = pResults->GetInt(0);
			CvAssert(iDomainID > -1 && iDomainID < iNumDomains);

			const int iDomainMod = pResults->GetInt("Modifier");
			m_piDomainModifierPercent[iDomainID] = iDomainMod;

			const int iAttack = pResults->GetInt("Attack");
			m_piDomainAttackPercent[iDomainID] = iAttack;

			const int iDefense = pResults->GetInt("Defense");
			m_piDomainDefensePercent[iDomainID] = iDefense;
		}

		pResults->Reset();
	}



	//UnitPromotions_UnitCombatMods
	{
		kUtility.InitializeArray(m_piUnitCombatModifierPercent, iNumUnitCombatClasses, 0);

		std::string sqlKey = "m_piUnitCombatModifierPercent";
		Database::Results* pResults = kUtility.GetResults(sqlKey);
		if(pResults == NULL)
		{
			const char* szSQL = "select UnitCombatInfos.ID, Modifier from UnitPromotions_UnitCombatMods inner join UnitCombatInfos on UnitCombatInfos.Type = UnitCombatType where PromotionType = ?;";
			pResults = kUtility.PrepareResults(sqlKey, szSQL);
		}

		CvAssert(pResults);
		if(!pResults) return false;

		pResults->Bind(1, szPromotionType);

		while(pResults->Step())
		{
			const int iUnitCombatID = pResults->GetInt(0);
			CvAssert(iUnitCombatID > -1 && iUnitCombatID < iNumUnitCombatClasses);

			const int iUnitCombatMod = pResults->GetInt(1);
			m_piUnitCombatModifierPercent[iUnitCombatID] = iUnitCombatMod;
		}

		pResults->Reset();
	}

	//UnitPromotions_UnitCombats
	{
		kUtility.InitializeArray(m_pbUnitCombat, iNumUnitCombatClasses, false);

		std::string sqlKey = "m_pbUnitCombat";
		Database::Results* pResults = kUtility.GetResults(sqlKey);
		if(pResults == NULL)
		{
			const char* szSQL = "select UnitCombatInfos.ID from UnitPromotions_UnitCombats inner join UnitCombatInfos On UnitCombatInfos.Type = UnitCombatType where PromotionType = ?";
			pResults = kUtility.PrepareResults(sqlKey, szSQL);
		}

		CvAssert(pResults);
		if(!pResults) return false;

		pResults->Bind(1, szPromotionType);

		while(pResults->Step())
		{
			const int iUnitCombatInfoID = pResults->GetInt(0);
			CvAssert(iUnitCombatInfoID < iNumUnitCombatClasses);

			m_pbUnitCombat[iUnitCombatInfoID] = true;
		}

		pResults->Reset();
	}

	//UnitPromotions_CivilianUnitType
	{
		kUtility.InitializeArray(m_pbCivilianUnitType, iNumUnitTypes, false);

		std::string sqlKey = "m_pbCivilianUnitType";
		Database::Results* pResults = kUtility.GetResults(sqlKey);
		if(pResults == NULL)
		{
			const char* szSQL = "select Units.ID from UnitPromotions_CivilianUnitType inner join Units On Units.Type = UnitType where PromotionType = ?";
			pResults = kUtility.PrepareResults(sqlKey, szSQL);
		}

		CvAssert(pResults);
		if(!pResults) return false;

		pResults->Bind(1, szPromotionType);

		while(pResults->Step())
		{
			const int iUnit = (UnitTypes)pResults->GetInt(0);
			CvAssert(iUnit < iNumUnitTypes);

			m_pbCivilianUnitType[iUnit] = true;
		}

		pResults->Reset();
	}


	//UnitPromotions_UnitType
	{
		kUtility.InitializeArray(m_pbUnitType, iNumUnitTypes, false);

		std::string sqlKey = "m_pbUnitType";
		Database::Results* pResults = kUtility.GetResults(sqlKey);
		if (pResults == NULL)
		{
			const char* szSQL = "select Units.ID from UnitPromotions_UnitType inner join Units On Units.Type = UnitType where PromotionType = ?";
			pResults = kUtility.PrepareResults(sqlKey, szSQL);
		}

		CvAssert(pResults);
		if (!pResults) return false;

		pResults->Bind(1, szPromotionType);

		while (pResults->Step())
		{
			const int iUnit = (UnitTypes)pResults->GetInt(0);
			CvAssert(iUnit < iNumUnitTypes);

			m_pbUnitType[iUnit] = true;
		}

		pResults->Reset();
	}



#if defined(MOD_PROMOTIONS_UNIT_NAMING)
	if (MOD_PROMOTIONS_UNIT_NAMING)
	{
		// We don't store the unit names, only if the promotion gives a name, we will look-up the actual name as it's needed
		kUtility.PopulateArrayByExistence(m_pbUnitName,
			"Units",
			"UnitPromotions_UnitName",
			"UnitType",
			"PromotionType",
			szPromotionType);
	}
#endif

#if defined(MOD_API_PROMOTION_TO_PROMOTION_MODIFIERS)
	if (MOD_API_PROMOTION_TO_PROMOTION_MODIFIERS)
	{
		// UnitPromotions_PromotionModifiers
		std::string sqlKey = "UnitPromotions_PromotionModifiers";
		Database::Results* pResults = kUtility.GetResults(sqlKey);
		if (pResults == nullptr)
		{
			const char* sql = "select t2.ID, t1.Modifier, t1.Attack, t1.Defense from UnitPromotions_PromotionModifiers as t1 left join UnitPromotions t2 on t1.OtherPromotionType = t2.`Type` where t1.PromotionType = ?";
			pResults = kUtility.PrepareResults(sqlKey, sql);
		}

		CvAssert(pResults);
		if (pResults == nullptr)
		{
			return false;
		}

		pResults->Bind(1, szPromotionType);

		while (pResults->Step())
		{
			const PromotionTypes iOtherPromotionType = static_cast<PromotionTypes>(pResults->GetInt(0));
			CvAssert(iOtherPromotionType > -1 && iOtherPromotionType < GC.getNumPromotionInfos());

			const int iModifier = pResults->GetInt("Modifier");
			if (iModifier != 0)
				m_pPromotionModifiers[iOtherPromotionType] += iModifier;

			const int iAttack = pResults->GetInt("Attack");
			if (iAttack != 0)
				m_pPromotionAttackModifiers[iOtherPromotionType] += iAttack;

			const int iDefense = pResults->GetInt("Defense");
			if (iDefense != 0)
				m_pPromotionDefenseModifiers[iOtherPromotionType] += iDefense;
		}

		pResults->Reset();
	}
#endif

	kUtility.PopulateArrayByExistence(m_pbPostCombatRandomPromotion,
		"UnitPromotions",
		"UnitPromotions_PostCombatRandomPromotion",
		"NewPromotion",
		"PromotionType",
		szPromotionType);

	return true;
}

/// Accessor: The granny animation index used to indicate that a unit has leveled up
int CvPromotionEntry::GetLayerAnimationPath() const
{
	return m_iLayerAnimationPath;
}

/// Accessor: The promotion required before this promotion is available
int CvPromotionEntry::GetPrereqPromotion() const
{
	return m_iPrereqPromotion;
}

/// Set the promotion required before this promotion is available
void CvPromotionEntry::SetPrereqPromotion(int i)
{
	m_iPrereqPromotion = i;
}

/// Accessor: Gets promotion 1 of an either/or promotion prerequisite.
int CvPromotionEntry::GetPrereqOrPromotion1() const
{
	return m_iPrereqOrPromotion1;
}

/// Accessor: Sets promotion 1 of an either/or promotion prerequisite.
void CvPromotionEntry::SetPrereqOrPromotion1(int i)
{
	m_iPrereqOrPromotion1 = i;
}

/// Accessor: Gets promotion 2 of an either/or promotion prerequisite.
int CvPromotionEntry::GetPrereqOrPromotion2() const
{
	return m_iPrereqOrPromotion2;
}

/// Accessor: Sets promotion 2 of an either/or promotion prerequisite.
void CvPromotionEntry::SetPrereqOrPromotion2(int i)
{
	m_iPrereqOrPromotion2 = i;
}

/// Accessor: Gets promotion 3 of an either/or promotion prerequisite.
int CvPromotionEntry::GetPrereqOrPromotion3() const
{
	return m_iPrereqOrPromotion3;
}

/// Accessor: Sets promotion 3 of an either/or promotion prerequisite.
void CvPromotionEntry::SetPrereqOrPromotion3(int i)
{
	m_iPrereqOrPromotion3 = i;
}

/// Accessor: Gets promotion 4 of an either/or promotion prerequisite.
int CvPromotionEntry::GetPrereqOrPromotion4() const
{
	return m_iPrereqOrPromotion4;
}

/// Accessor: Sets promotion 4 of an either/or promotion prerequisite.
void CvPromotionEntry::SetPrereqOrPromotion4(int i)
{
	m_iPrereqOrPromotion4 = i;
}

/// Accessor: Gets promotion 5 of an either/or promotion prerequisite.
int CvPromotionEntry::GetPrereqOrPromotion5() const
{
	return m_iPrereqOrPromotion5;
}

/// Accessor: Sets promotion 5 of an either/or promotion prerequisite.
void CvPromotionEntry::SetPrereqOrPromotion5(int i)
{
	m_iPrereqOrPromotion5 = i;
}

/// Accessor: Gets promotion 6 of an either/or promotion prerequisite.
int CvPromotionEntry::GetPrereqOrPromotion6() const
{
	return m_iPrereqOrPromotion6;
}

/// Accessor: Sets promotion 6 of an either/or promotion prerequisite.
void CvPromotionEntry::SetPrereqOrPromotion6(int i)
{
	m_iPrereqOrPromotion6 = i;
}

/// Accessor: Gets promotion 7 of an either/or promotion prerequisite.
int CvPromotionEntry::GetPrereqOrPromotion7() const
{
	return m_iPrereqOrPromotion7;
}

/// Accessor: Sets promotion 7 of an either/or promotion prerequisite.
void CvPromotionEntry::SetPrereqOrPromotion7(int i)
{
	m_iPrereqOrPromotion7 = i;
}

/// Accessor: Gets promotion 8 of an either/or promotion prerequisite.
int CvPromotionEntry::GetPrereqOrPromotion8() const
{
	return m_iPrereqOrPromotion8;
}

/// Accessor: Sets promotion 8 of an either/or promotion prerequisite.
void CvPromotionEntry::SetPrereqOrPromotion8(int i)
{
	m_iPrereqOrPromotion8 = i;
}

/// Accessor: Gets promotion 9 of an either/or promotion prerequisite.
int CvPromotionEntry::GetPrereqOrPromotion9() const
{
	return m_iPrereqOrPromotion9;
}

/// Accessor: Sets promotion 9 of an either/or promotion prerequisite.
void CvPromotionEntry::SetPrereqOrPromotion9(int i)
{
	m_iPrereqOrPromotion9 = i;
}

/// Accessor: Gets promotion 10 of an either/or promotion prerequisite.
int CvPromotionEntry::GetPrereqOrPromotion10() const
{
	return m_iPrereqOrPromotion10;
}

/// Accessor: Sets promotion 10 of an either/or promotion prerequisite.
void CvPromotionEntry::SetPrereqOrPromotion10(int i)
{
	m_iPrereqOrPromotion10 = i;
}

/// Accessor: Gets promotion 10 of an either/or promotion prerequisite.
int CvPromotionEntry::GetPrereqOrPromotion11() const
{
	return m_iPrereqOrPromotion11;
}

/// Accessor: Sets promotion 11 of an either/or promotion prerequisite.
void CvPromotionEntry::SetPrereqOrPromotion11(int i)
{
	m_iPrereqOrPromotion11 = i;
}


/// Accessor: Gets promotion 11 of an either/or promotion prerequisite.
int CvPromotionEntry::GetPrereqOrPromotion12() const
{
	return m_iPrereqOrPromotion12;
}

/// Accessor: Sets promotion 10 of an either/or promotion prerequisite.
void CvPromotionEntry::SetPrereqOrPromotion12(int i)
{
	m_iPrereqOrPromotion12 = i;
}


/// Accessor: Gets promotion 11 of an either/or promotion prerequisite.
int CvPromotionEntry::GetPrereqOrPromotion13() const
{
	return m_iPrereqOrPromotion13;
}

/// Accessor: Sets promotion 10 of an either/or promotion prerequisite.
void CvPromotionEntry::SetPrereqOrPromotion13(int i)
{
	m_iPrereqOrPromotion13 = i;
}



/// Accessor: Gets the tech prerequisite for this promotion
int CvPromotionEntry::GetTechPrereq() const
{
	return m_iTechPrereq;
}

/// Accessor: What type of invisible this unit is
int CvPromotionEntry::GetInvisibleType() const
{
	return m_iInvisibleType;
}

/// Accessor: What type of invisible this unit can see
int CvPromotionEntry::GetSeeInvisibleType() const
{
	return m_iSeeInvisibleType;
}

#if defined(MOD_PROMOTION_FEATURE_INVISIBLE)
int CvPromotionEntry::GetFeatureInvisible() const
{
	return m_iFeatureInvisible;
}
int CvPromotionEntry::GetFeatureInvisible2() const
{
	return m_iFeatureInvisible2;
}
#endif

#if defined(MOD_PROMOTION_MULTIPLE_INIT_EXPERENCE)
int CvPromotionEntry::GetMultipleInitExperence() const
{
	return m_iMultipleInitExperence;
}
#endif

/// Accessor: How many additional tiles this promotion allows a unit to see (can be negative)
int CvPromotionEntry::GetVisibilityChange() const
{
	return m_iVisibilityChange;
}

#if defined(MOD_PROMOTIONS_VARIABLE_RECON)
/// Accessor: How many additional tiles this promotion allows a unit to recon (can be negative)
int CvPromotionEntry::GetReconChange() const
{
	return m_iReconChange;
}
#endif

/// Accessor: How many additional tiles a unit may move
int CvPromotionEntry::GetMovesChange() const
{
	return m_iMovesChange;
}

/// Accessor: How much movement is discounted
int CvPromotionEntry::GetMoveDiscountChange() const
{
	return m_iMoveDiscountChange;
}

/// Accessor: How much the air range of the unit is modified
int CvPromotionEntry::GetRangeChange() const
{
	return m_iRangeChange;
}

/// Accessor: How much the ranged strength of the unit is modified
int CvPromotionEntry::GetRangedAttackModifier() const
{
	return m_iRangedAttackModifier;
}

/// Accessor: How much the strength of the unit is modified when intercepting
int CvPromotionEntry::GetInterceptionCombatModifier() const
{
	return m_iInterceptionCombatModifier;
}

/// Accessor: Mod to Damage taken if intercepted
int CvPromotionEntry::GetInterceptionDefenseDamageModifier() const
{
	return m_iInterceptionDefenseDamageModifier;
}

/// Accessor: How much the strength of the unit is modified when air sweeping
int CvPromotionEntry::GetAirSweepCombatModifier() const
{
	return m_iAirSweepCombatModifier;
}

/// Accessor: How much the intercept chance changes for an air unit
int CvPromotionEntry::GetInterceptChanceChange() const
{
	return m_iInterceptChanceChange;
}

/// Accessor: Numbe rof extra interceptions that may be made on one turn
int CvPromotionEntry::GetNumInterceptionChange() const
{
	return m_iNumInterceptionChange;
}

/// Accessor: How well an air unit can evade interception
int CvPromotionEntry::GetEvasionChange() const
{
	return m_iEvasionChange;
}

/// Accessor: The amount of cargo a unit can carry
int CvPromotionEntry::GetCargoChange() const
{
	return m_iCargoChange;
}

/// Accessor: Damage/health bonus per turn in enemy lands
int CvPromotionEntry::GetEnemyHealChange() const
{
	return m_iEnemyHealChange;
}

/// Accessor: Damage/health bonus per turn in neutral lands
int CvPromotionEntry::GetNeutralHealChange() const
{
	return m_iNeutralHealChange;
}

/// Accessor: Damage/health bonus per turn in friendly lands
int CvPromotionEntry::GetFriendlyHealChange() const
{
	return m_iFriendlyHealChange;
}

/// Accessor: Heals units in the same tile
int CvPromotionEntry::GetSameTileHealChange() const
{
	return m_iSameTileHealChange;
}

/// Accessor: Heals units in adjacent tiles
int CvPromotionEntry::GetAdjacentTileHealChange() const
{
	return m_iAdjacentTileHealChange;
}

/// Accessor: Chance per turn to take damage in enemy lands
int CvPromotionEntry::GetEnemyDamageChance() const
{
	return m_iEnemyDamageChance;
}

/// Accessor: Chance per turn to take damage in neutral lands
int CvPromotionEntry::GetNeutralDamageChance() const
{
	return m_iNeutralDamageChance;
}

/// Accessor: Amount of damage taken by being in enemy lands
int CvPromotionEntry::GetEnemyDamage() const
{
	return m_iEnemyDamage;
}

/// Accessor: Amount of damage taken by being in neutral lands
int CvPromotionEntry::GetNeutralDamage() const
{
	return m_iNeutralDamage;
}

/// Accessor: Bonus strength percent
int CvPromotionEntry::GetCombatPercent() const
{
	return m_iCombatPercent;
}

/// Accessor: Bonus city attack percent
int CvPromotionEntry::GetCityAttackPercent() const
{
	return m_iCityAttackPercent;
}

/// Accessor: Bonus city defense percent
int CvPromotionEntry::GetCityDefensePercent() const
{
	return m_iCityDefensePercent;
}

/// Accessor: Bonus ranged defense percent
int CvPromotionEntry::GetRangedDefenseMod() const
{
	return m_iRangedDefenseMod;
}

/// Accessor: Bonus hills attack percent
int CvPromotionEntry::GetHillsAttackPercent() const
{
	return m_iHillsAttackPercent;
}

/// Accessor: Bonus hills defense percent
int CvPromotionEntry::GetHillsDefensePercent() const
{
	return m_iHillsDefensePercent;
}

/// Accessor: Bonus open terrain attack percent
int CvPromotionEntry::GetOpenAttackPercent() const
{
	return m_iOpenAttackPercent;
}

/// Accessor: Bonus open terrain ranged attack mod
int CvPromotionEntry::GetOpenRangedAttackMod() const
{
	return m_iOpenRangedAttackMod;
}

/// Accessor: Bonus open terrain attack percent
int CvPromotionEntry::GetRoughAttackPercent() const
{
	return m_iRoughAttackPercent;
}

/// Accessor: Bonus rough terrain ranged attack mod
int CvPromotionEntry::GetRoughRangedAttackMod() const
{
	return m_iRoughRangedAttackMod;
}

/// Accessor: Bonus when attacking fortified units
int CvPromotionEntry::GetAttackFortifiedMod() const
{
	return m_iAttackFortifiedMod;
}

/// Accessor: Bonus when attacking wounded units
int CvPromotionEntry::GetAttackWoundedMod() const
{
	return m_iAttackWoundedMod;
}

/// Accessor: Bonus when making a flank attack
int CvPromotionEntry::GetFlankAttackModifier() const
{
	return m_iFlankAttackModifier;
}

/// Accessor: Modifier on nearby enemy combat units
int CvPromotionEntry::GetNearbyEnemyCombatMod() const
{
	return m_iNearbyEnemyCombatMod;
}

/// Accessor: Range of modifier on nearby enemy combat units
int CvPromotionEntry::GetNearbyEnemyCombatRange() const
{
	return m_iNearbyEnemyCombatRange;
}

/// Accessor: Bonus open terrain defense percent
int CvPromotionEntry::GetOpenDefensePercent() const
{
	return m_iOpenDefensePercent;
}

/// Accessor: Bonus open terrain defense percent
int CvPromotionEntry::GetRoughDefensePercent() const
{
	return m_iRoughDefensePercent;
}

/// Accessor: Does the unit have more than 1 attack?
int CvPromotionEntry::GetExtraAttacks() const
{
	return m_iExtraAttacks;
}

/// Accessor: Does this Promotion make a Unit a Great General?
bool CvPromotionEntry::IsGreatGeneral() const
{
	return m_bGreatGeneral;
}

/// Accessor: Does this Promotion make a Unit a Great Admiral?
bool CvPromotionEntry::IsGreatAdmiral() const
{
	return m_bGreatAdmiral;
}


#if defined(MOD_DEFENSE_MOVES_BONUS)
int CvPromotionEntry::GetMoveLeftDefenseMod() const
{
	return m_iMoveLeftDefenseMod;
}

int CvPromotionEntry::GetMoveUsedDefenseMod() const
{
	return m_iMoveUsedDefenseMod;
}
#endif

#if defined(MOD_PROMOTIONS_AURA_CHANGE)
/// Accessor: Does this Promotion change the range of the aura of a Great General or Great Admiral?
int CvPromotionEntry::GetAuraRangeChange() const
{
	return m_iAuraRangeChange;
}

/// Accessor: Does this Promotion change the effect of the aura of a Great General or Great Admiral?
int CvPromotionEntry::GetAuraEffectChange() const
{
	return m_iAuraEffectChange;
}
#endif

#if defined(MOD_ROG_CORE)
int CvPromotionEntry::GetHPHealedIfDefeatEnemyGlobal() const
{
	return m_iHPHealedIfDefeatEnemyGlobal;
}

int CvPromotionEntry::GetNumOriginalCapitalAttackMod() const
{
	return m_iNumOriginalCapitalAttackMod;
}

int CvPromotionEntry::GetNumOriginalCapitalDefenseMod() const
{
	return m_iNumOriginalCapitalDefenseMod;
}
#endif


#if defined(MOD_ROG_CORE)
int CvPromotionEntry::GetOnCapitalLandAttackMod() const
{
	return m_iOnCapitalLandAttackMod;
}

int CvPromotionEntry::GetOutsideCapitalLandAttackMod() const
{
	return m_iOutsideCapitalLandAttackMod;
}

int CvPromotionEntry::GetOnCapitalLandDefenseMod() const
{
	return m_iOnCapitalLandDefenseMod;
}

int CvPromotionEntry::GetOutsideCapitalLandDefenseMod() const
{
	return m_iOutsideCapitalLandDefenseMod;
}
#endif



#if defined(MOD_ROG_CORE)
int CvPromotionEntry::GetMeleeDefenseMod() const
{
	return m_iMeleeDefenseMod;
}

int CvPromotionEntry::GetAttackFullyHealedMod() const
{
	return m_iAttackFullyHealedMod;
}

int CvPromotionEntry::GetAttackAboveHealthMod() const
{
	return m_iAttackAboveHealthMod;
}
int CvPromotionEntry::GetAttackBelowHealthMod() const
{
	return m_iAttackBelowHealthMod;
}
#endif

#if defined(MOD_ROG_CORE)
bool CvPromotionEntry::IsStrongerDamaged() const
{
	return m_bStrongerDamaged;
}
bool CvPromotionEntry::IsFightWellDamaged() const
{
	return m_bFightWellDamaged;
}

int CvPromotionEntry::GetAoEDamageOnMove() const
{
	return m_iAoEDamageOnMove;
}
int CvPromotionEntry::ForcedDamageValue() const
{
	return m_iForcedDamageValue;
}
int CvPromotionEntry::ChangeDamageValue() const
{
	return m_iChangeDamageValue;
}

#endif

#if defined(MOD_ROG_CORE)

int CvPromotionEntry::GetMoveLfetAttackMod() const
{
	return m_iMoveLfetAttackMod;
}


int CvPromotionEntry::GetMoveUsedAttackMod() const
{
	return m_iMoveUsedAttackMod;
}

int CvPromotionEntry::GetGoldenAgeMod() const
{
	return m_iGoldenAgeMod;
}

int CvPromotionEntry::GetRangedSupportFireMod() const
{
	return m_iRangedSupportFireMod;
}

/// Accessor: Can this Promotion grant bonuses v. barbarians?
int CvPromotionEntry::GetBarbarianCombatBonus() const
{
	return m_iBarbarianCombatBonus;
}

int CvPromotionEntry::GetAOEDamageOnKill() const
{
	return m_iAOEDamageOnKill;
}

int CvPromotionEntry::GetDamageAoEFortified() const
{
	return m_iDamageAoEFortified;
}
int CvPromotionEntry::GetWorkRateMod() const
{
	return m_iWorkRateMod;
}
#endif

int CvPromotionEntry::GetCaptureDefeatedEnemyChance() const
{
	return m_iCaptureDefeatedEnemyChance;
}

//Cannot be captured
bool CvPromotionEntry::CannotBeCaptured() const
{
	return m_bCannotBeCaptured;
}




#if defined(MOD_ROG_CORE)

int CvPromotionEntry::GetNumSpyAttackMod() const
{
	return m_iNumSpyAttackMod;
}

int CvPromotionEntry::GetNumSpyDefenseMod() const
{
	return m_iNumSpyDefenseMod;
}

int CvPromotionEntry::GetNumWonderAttackMod() const
{
	return m_iNumWonderAttackMod;
}

int CvPromotionEntry::GetNumWonderDefenseMod() const
{
	return m_iNumWonderDefenseMod;
}

int CvPromotionEntry::GetNumWorkAttackMod() const
{
	return m_iNumWorkAttackMod;
}

int CvPromotionEntry::GetNumWorkDefenseMod() const
{
	return m_iNumWorkDefenseMod;
}

bool CvPromotionEntry::IsNoResourcePunishment() const
{
	return m_bNoResourcePunishment;
}


int CvPromotionEntry::GetCurrentHitPointAttackMod() const
{
	return m_iCurrentHitPointAttackMod;
}

int CvPromotionEntry::GetCurrentHitPointDefenseMod() const
{
	return m_iCurrentHitPointDefenseMod;
}



int CvPromotionEntry::GetNearNumEnemyAttackMod() const
{
	return m_iNearNumEnemyAttackMod;
}

int CvPromotionEntry::GetNearNumEnemyDefenseMod() const
{
	return m_iNearNumEnemyDefenseMod;
}
#endif



#if defined(MOD_ROG_CORE)
/// Get the UnitClass we want to receive the bonus from.
UnitClassTypes CvPromotionEntry::GetCombatBonusFromNearbyUnitClass() const
{
	return m_iCombatBonusFromNearbyUnitClass;
}
/// Distance from this UnitClass
int CvPromotionEntry::GetNearbyUnitClassBonusRange() const
{
	return m_iNearbyUnitClassBonusRange;
}
/// Bonus from this UnitClass
int CvPromotionEntry::GetNearbyUnitClassBonus() const
{
	return m_iNearbyUnitClassBonus;
}
#endif


/// Accessor: Increase in rate of great general creation
int CvPromotionEntry::GetGreatGeneralModifier() const
{
	return m_iGreatGeneralModifier;
}

/// Accessor: Does this Promotion make a Great General move at this unit's rate?
bool CvPromotionEntry::IsGreatGeneralReceivesMovement() const
{
	return m_bGreatGeneralReceivesMovement;
}

/// Accessor: Combat bonus when stacked with Great General
int CvPromotionEntry::GetGreatGeneralCombatModifier() const
{
	return m_iGreatGeneralCombatModifier;
}

/// Accessor: Combat mod for fighting in friendly territory
int CvPromotionEntry::GetFriendlyLandsModifier() const
{
	return m_iFriendlyLandsModifier;
}

/// Accessor: Attack mod for fighting in friendly territory
int CvPromotionEntry::GetFriendlyLandsAttackModifier() const
{
	return m_iFriendlyLandsAttackModifier;
}

/// Accessor: Combat mod for fighting outside friendly territory
int CvPromotionEntry::GetOutsideFriendlyLandsModifier() const
{
	return m_iOutsideFriendlyLandsModifier;
}

/// Accessor: Returns the command type for this HotKeyInfo class (which would be COMMAND_PROMOTION)
int CvPromotionEntry::GetCommandType() const
{
	return m_iCommandType;
}

/// Sets the command type for this class.
void CvPromotionEntry::SetCommandType(int iNewType)
{
	m_iCommandType = iNewType;
}

#if defined(MOD_UNITS_NO_SUPPLY)
/// Accessor: Unit has no supply cost
bool CvPromotionEntry::IsNoSupply() const
{
	return m_bNoSupply;
}
#endif

#if defined(MOD_UNITS_MAX_HP)
/// Accessor: Absolute change of max hit points
int CvPromotionEntry::GetMaxHitPointsChange() const
{
	return m_iMaxHitPointsChange;
}

/// Accessor: Percentage modifier of base max hit points
int CvPromotionEntry::GetMaxHitPointsModifier() const
{
	return m_iMaxHitPointsModifier;
}
#endif

/// Accessor: How much upgrading this unit is discounted
int CvPromotionEntry::GetUpgradeDiscount() const
{
	return m_iUpgradeDiscount;
}

/// Accessor: Increased rate of gaining experience
int CvPromotionEntry::GetExperiencePercent() const
{
	return m_iExperiencePercent;
}

/// Accessor: Bonus when adjacent to a friendly Unit
int CvPromotionEntry::GetAdjacentMod() const
{
	return m_iAdjacentMod;
}

/// Accessor: Percent attack change
int CvPromotionEntry::GetAttackMod() const
{
	return m_iAttackMod;
}

/// Accessor: Percent defense change
int CvPromotionEntry::GetDefenseMod() const
{
	return m_iDefenseMod;
}

/// Accessor: Number of tiles away a unit may paradrop
int CvPromotionEntry::GetDropRange() const
{
	return m_iDropRange;
}

/// Accessor: Number of extra tiles of movement for naval vessels
int CvPromotionEntry::GetExtraNavalMoves() const
{
	return m_iExtraNavalMoves;
}

/// Accessor: HP healed after defeating enemy (up to 10 for full heal)
int CvPromotionEntry::GetHPHealedIfDefeatEnemy() const
{
	return m_iHPHealedIfDefeatEnemy;
}

/// Accessor: percent of defeated enemy strenth awarded as points toward next Golden Age
int CvPromotionEntry::GetGoldenAgeValueFromKills() const
{
	return m_iGoldenAgeValueFromKills;
}

/// Accessor: maximum chance to withdraw before melee
int CvPromotionEntry::GetExtraWithdrawal() const
{
	return m_iExtraWithdrawal;
}

/// Accessor: extra sight range when embarked
int CvPromotionEntry::GetEmbarkExtraVisibility() const
{
	return m_iEmbarkExtraVisibility;
}

/// Accessor: extra defense when embarked
int CvPromotionEntry::GetEmbarkDefenseModifier() const
{
	return m_iEmbarkDefenseModifier;
}

/// Accessor: bonus defending near capital
int CvPromotionEntry::GetCapitalDefenseModifier() const
{
	return m_iCapitalDefenseModifier;
}

/// Accessor: drop per hex away from capital of CapitalDefenseModifier
int CvPromotionEntry::GetCapitalDefenseFalloff() const
{
	return m_iCapitalDefenseFalloff;
}

/// Accessor: gold earned from damage on an attacked city
int CvPromotionEntry::GetCityAttackPlunderModifier() const
{
	return m_iCityAttackPlunderModifier;
}

#if defined(MOD_PROMOTION_GET_INSTANCE_FROM_ATTACK)
/// Accessor: faith earned from damage on an attacked unit
int CvPromotionEntry::GetUnitAttackFaithBonus() const
{
	return m_iUnitAttackFaithBonus;
}
/// Accessor: faith earned from damage on an attacked city
int CvPromotionEntry::GetCityAttackFaithBonus() const
{
	return m_iCityAttackFaithBonus;
}
#endif

#if defined(MOD_PROMOTION_REMOVE_PROMOTION_UPGRADE)
int CvPromotionEntry::GetRemovePromotionUpgrade() const
{
	return m_iRemovePromotionUpgrade;
}
#endif

/// Accessor: loss of strength at end of each turn in rival territory without open borders
int CvPromotionEntry::GetReligiousStrengthLossRivalTerritory() const
{
	return m_iReligiousStrengthLossRivalTerritory;
}

/// Accessor: This is for great merchants for when they complete their mission in a city state.
int CvPromotionEntry::GetTradeMissionInfluenceModifier() const
{
	return m_iTradeMissionInfluenceModifier;
}

/// Accessor: This is for great merchants for when they complete their mission in a city state.
int CvPromotionEntry::GetTradeMissionGoldModifier() const
{
	return m_iTradeMissionGoldModifier;
}

/// Accessor: Can this Promotion be earned through normal leveling?
bool CvPromotionEntry::IsCannotBeChosen() const
{
	return m_bCannotBeChosen;
}

/// Accessor: Is this lost with upgrading?
bool CvPromotionEntry::IsLostWithUpgrade() const
{
	return m_bLostWithUpgrade;
}

/// Accessor: Is this a promotion that is not given out when a unit is upgrading?
bool CvPromotionEntry::IsNotWithUpgrade() const
{
	return m_bNotWithUpgrade;
}

/// Accessor: Does this promotion instantly heal a Unit?
bool CvPromotionEntry::IsInstaHeal() const
{
	return m_bInstaHeal;
}

/// Accessor: Does this unit have a warlord attached to it?
bool CvPromotionEntry::IsLeader() const
{
	return m_bLeader;
}

/// Accessor: Can this unit attack multiple times a turn?
bool CvPromotionEntry::IsBlitz() const
{
	return m_bBlitz;
}

/// Accessor: Can this unit attack from the sea without penalty?
bool CvPromotionEntry::IsAmphib() const
{
	return m_bAmphib;
}

/// Accessor: Can this unit attack across rivers without penalty?
bool CvPromotionEntry::IsRiver() const
{
	return m_bRiver;
}

/// Accessor: Can this unit use enemy roads?
bool CvPromotionEntry::IsEnemyRoute() const
{
	return m_bEnemyRoute;
}

/// Accessor: Can enter rival territory
bool CvPromotionEntry::IsRivalTerritory() const
{
	return m_bRivalTerritory;
}

/// Accessor: Must this unit set up multiple turns for a ranged attack
bool CvPromotionEntry::IsMustSetUpToRangedAttack() const
{
	return m_bMustSetUpToRangedAttack;
}

/// Accessor: Can this Unit provide Ranged Support Fire?
bool CvPromotionEntry::IsRangedSupportFire() const
{
	return m_bRangedSupportFire;
}

/// Accessor: Unit can heal while moving
bool CvPromotionEntry::IsAlwaysHeal() const
{
	return m_bAlwaysHeal;
}

/// Accessor: Unit can heal outside friendly territory (naval units)
bool CvPromotionEntry::IsHealOutsideFriendly() const
{
	return m_bHealOutsideFriendly;
}

/// Accessor: Double movement in hills
bool CvPromotionEntry::IsHillsDoubleMove() const
{
	return m_bHillsDoubleMove;
}

/// Accessor: Ignores terrain movement penalties
bool CvPromotionEntry::IsIgnoreTerrainCost() const
{
	return m_bIgnoreTerrainCost;
}

#if defined(MOD_API_PLOT_BASED_DAMAGE)
/// Accessor: Ignores terrain damage
bool CvPromotionEntry::IsIgnoreTerrainDamage() const
{
	return m_bIgnoreTerrainDamage;
}

/// Accessor: Ignores feature damage
bool CvPromotionEntry::IsIgnoreFeatureDamage() const
{
	return m_bIgnoreFeatureDamage;
}

/// Accessor: Takes extra terrain damage
bool CvPromotionEntry::IsExtraTerrainDamage() const
{
	return m_bExtraTerrainDamage;
}

/// Accessor: Takes extra feature damage
bool CvPromotionEntry::IsExtraFeatureDamage() const
{
	return m_bExtraFeatureDamage;
}
#endif

#if defined(MOD_PROMOTIONS_IMPROVEMENT_BONUS)
/// Accessor: Can cross mountains (but we'd rather they left them nice and straight!)
int CvPromotionEntry::GetNearbyImprovementCombatBonus() const
{
	return m_iNearbyImprovementCombatBonus;
}
int CvPromotionEntry::GetNearbyImprovementBonusRange() const
{
	return m_iNearbyImprovementBonusRange;
}
ImprovementTypes CvPromotionEntry::GetCombatBonusImprovement() const
{
	return m_eCombatBonusImprovement;
}
#endif

#if defined(MOD_PROMOTIONS_ALLYCITYSTATE_BONUS)
/// Accessor: Permits units to receive a combat bonus from Ally City States
int CvPromotionEntry::GetAllyCityStateCombatModifier() const
{
	return m_iAllyCityStateCombatModifier;
}
int CvPromotionEntry::GetAllyCityStateCombatModifierMax() const
{
	return m_iAllyCityStateCombatModifierMax;
}
#endif

#if defined(MOD_PROMOTIONS_EXTRARES_BONUS)
// Permits units to receive a combat bonus from Extra Resourses/Hapiness
ResourceTypes CvPromotionEntry::GetExtraResourceType() const
{
	return m_eExtraResourceType;
}
int CvPromotionEntry::GetExtraResourceCombatModifier() const
{
	return m_iExtraResourceCombatModifier;
}
int CvPromotionEntry::GetExtraResourceCombatModifierMax() const
{
	return m_iExtraResourceCombatModifierMax;
}
int CvPromotionEntry::GetExtraHappinessCombatModifier() const
{
	return m_iExtraHappinessCombatModifier;
}
int CvPromotionEntry::GetExtraHappinessCombatModifierMax() const
{
	return m_iExtraHappinessCombatModifierMax;
}
#endif

#if defined(MOD_PROMOTIONS_CROSS_MOUNTAINS)
/// Accessor: Can cross mountains (but we'd rather they left them nice and straight!)
bool CvPromotionEntry::CanCrossMountains() const
{
	return m_bCanCrossMountains;
}
#endif

#if defined(MOD_PROMOTIONS_CROSS_OCEANS)
/// Accessor: Can cross oceans (but that may make them angry!)
bool CvPromotionEntry::CanCrossOceans() const
{
	return m_bCanCrossOceans;
}
#endif

#if defined(MOD_PROMOTIONS_CROSS_ICE)
/// Accessor: Can cross ice (with what? Fire?)
bool CvPromotionEntry::CanCrossIce() const
{
	return m_bCanCrossIce;
}
#endif

#if defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
/// Accessor: Gets GG/GA points from barbarians
bool CvPromotionEntry::IsGGFromBarbarians() const
{
	return m_bGGFromBarbarians;
}
#endif

/// Accessor: Entering rough terrain uses up a unit's movement? (regardless of how many moves he has)
bool CvPromotionEntry::IsRoughTerrainEndsTurn() const
{
	return m_bRoughTerrainEndsTurn;
}

/// Accessor: Unit may pass over coast and Mountains
bool CvPromotionEntry::IsHoveringUnit() const
{
	return m_bHoveringUnit;
}

/// Accessor: All terrain costs 1 to enter
bool CvPromotionEntry::IsFlatMovementCost() const
{
	return m_bFlatMovementCost;
}

/// Accessor: May enter ice tiles
bool CvPromotionEntry::IsCanMoveImpassable() const
{
	return m_bCanMoveImpassable;
}

/// Accessor: Unable to capture cities
bool CvPromotionEntry::IsNoCapture() const
{
	return m_bNoCapture;
}

/// Accessor: Unable to attack enemy units
bool CvPromotionEntry::IsOnlyDefensive() const
{
	return m_bOnlyDefensive;
}

/// Accessor: No defensive bonuses
bool CvPromotionEntry::IsNoDefensiveBonus() const
{
	return m_bNoDefensiveBonus;
}

/// Accessor: Cannot be hurt by nukes?
bool CvPromotionEntry::IsNukeImmune() const
{
	return m_bNukeImmune;
}

/// Accessor: Nationality is hidden from other players
bool CvPromotionEntry::IsHiddenNationality() const
{
	return m_bHiddenNationality;
}

/// Accessor: Can attack without declaring war
bool CvPromotionEntry::IsAlwaysHostile() const
{
	return m_bAlwaysHostile;
}

/// Accessor: Cannot reveal terrain unless it is inside a player's territory
bool CvPromotionEntry::IsNoRevealMap() const
{
	return m_bNoRevealMap;
}

/// Accessor: Is this a Recon unit? (free sight all around it)
bool CvPromotionEntry::IsRecon() const
{
	return m_bRecon;
}

/// Accessor: Can move through all terrain?
bool CvPromotionEntry::CanMoveAllTerrain() const
{
	return m_bCanMoveAllTerrain;
}

/// Accessor: Can move after attacking another Unit?
bool CvPromotionEntry::IsCanMoveAfterAttacking() const
{
	return m_bCanMoveAfterAttacking;
}

/// Accessor: Can this unit air sweep?
bool CvPromotionEntry::IsAirSweepCapable() const
{
	return m_bAirSweepCapable;
}

/// Accessor: Can this unit embark?
bool CvPromotionEntry::IsAllowsEmbarkation() const
{
	return m_bAllowsEmbarkation;
}

/// Accessor: Can this unit shoot over obstacles?
bool CvPromotionEntry::IsRangeAttackIgnoreLOS() const
{
	return m_bRangeAttackIgnoreLOS;
}

/// Accessor: Lose no movement when pillaging?
bool CvPromotionEntry::IsFreePillageMoves() const
{
	return m_bFreePillageMoves;
}

/// Accessor: When pillaging, does the unit heal?
bool CvPromotionEntry::IsHealOnPillage() const
{
	return m_bHealOnPillage;
}

/// Accessor: Do we only get healed after a combat win if fighting a real civ or minor?
bool CvPromotionEntry::IsHealIfDefeatExcludeBarbarians() const
{
	return m_bHealIfDefeatExcludesBarbarians;
}

/// Accessor: Do we not get run over by naval units when embarked at sea?
bool CvPromotionEntry::IsEmbarkedAllWater() const
{
	return m_bEmbarkedAllWater;
}

#if defined(MOD_PROMOTIONS_DEEP_WATER_EMBARKATION)
/// Accessor: Do we not get run over by naval units when embarked at sea?
bool CvPromotionEntry::IsEmbarkedDeepWater() const
{
	return m_bEmbarkedDeepWater;
}
#endif

/// Accessor: Does this unit only attack cities
bool CvPromotionEntry::IsCityAttackOnly() const
{
	return m_bCityAttackOnly;
}

/// Accessor: Is a defeated enemy captured?
bool CvPromotionEntry::IsCaptureDefeatedEnemy() const
{
	return m_bCaptureDefeatedEnemy;
}

/// Accessor: Should this infantry ignore benefits from great generals
bool CvPromotionEntry::IsIgnoreGreatGeneralBenefit() const
{
	return m_bIgnoreGreatGeneralBenefit;
}


bool CvPromotionEntry::IsImmueMeleeAttack() const
{
	return m_bImmueMeleeAttack;
}

/// Accessor: Can this unit ignore ZOC when moving?
bool CvPromotionEntry::IsIgnoreZOC() const
{
	return m_bIgnoreZOC;
}

/// Accessor: Is this a Sapper unit (when next to a city, provides city attack bonus to nearby units)?
bool CvPromotionEntry::IsSapper() const
{
	return m_bSapper;
}

/// Accessor: Can this unit doa heavy charge (which either force an enemy to retreat or take extra damage)
bool CvPromotionEntry::IsCanHeavyCharge() const
{
	return m_bCanHeavyCharge;
}

/// Accessor: Does this promotion change into another after combat?
bool CvPromotionEntry::HasPostCombatPromotions() const
{
	return m_bHasPostCombatPromotions;
}

/// Accessor: Do we have to pick a post-combat promotion that hasn't been used yet?
bool CvPromotionEntry::ArePostCombatPromotionsExclusive() const
{
	return m_bPostCombatPromotionsExclusive;
}

#if defined(MOD_API_UNIT_CANNOT_BE_RANGED_ATTACKED)
bool CvPromotionEntry::IsCannotBeRangedAttacked() const
{
	return m_bCannotBeRangedAttacked;
}
#endif

/// Accessor: Sound to play when the promotion is gained
const char* CvPromotionEntry::GetSound() const
{
	return m_strSound;
}

/// Set the sound to be played when the promotion is gained
void CvPromotionEntry::SetSound(const char* szVal)
{
	m_strSound = szVal;
}

// ARRAYS

/// Returns an array of bonuses when attacking a tile of a given terrain
int CvPromotionEntry::GetTerrainAttackPercent(int i) const
{
	CvAssertMsg(i < GC.getNumTerrainInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumTerrainInfos() && m_piTerrainAttackPercent)
	{
		return m_piTerrainAttackPercent[i];
	}

	return -1;
}

/// Returns an array of bonuses when defending a tile of a given terrain
int CvPromotionEntry::GetTerrainDefensePercent(int i) const
{
	CvAssertMsg(i < GC.getNumTerrainInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumTerrainInfos() && m_piTerrainDefensePercent)
	{
		return m_piTerrainDefensePercent[i];
	}

	return-1;
}

/// Returns an array of bonuses when attacking a tile with a terrain feature
int CvPromotionEntry::GetFeatureAttackPercent(int i) const
{
	CvAssertMsg(i < GC.getNumFeatureInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumFeatureInfos() && m_piFeatureAttackPercent)
	{
		return m_piFeatureAttackPercent[i];
	}

	return -1;
}

/// Returns an array of bonuses when defending a tile with a terrain feature
int CvPromotionEntry::GetFeatureDefensePercent(int i) const
{
	CvAssertMsg(i < GC.getNumFeatureInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumFeatureInfos() && m_piFeatureDefensePercent)
	{
		return m_piFeatureDefensePercent[i];
	}

	return -1;
}

#if defined(MOD_API_UNIFIED_YIELDS)
int CvPromotionEntry::GetYieldFromKills(int i) const
{
	CvAssertMsg(i < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < NUM_YIELD_TYPES && m_piYieldFromKills)
	{
		return m_piYieldFromKills[i];
	}

	return 0;
}

int CvPromotionEntry::GetYieldFromBarbarianKills(int i) const
{
	CvAssertMsg(i < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < NUM_YIELD_TYPES && m_piYieldFromBarbarianKills)
	{
		return m_piYieldFromBarbarianKills[i];
	}

	return 0;
}
#endif

/// Returns an array of bonuses when fighting against a certain unit
int CvPromotionEntry::GetUnitCombatModifierPercent(int i) const
{
	CvAssertMsg(i < GC.getNumUnitCombatClassInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumUnitCombatClassInfos() && m_piUnitCombatModifierPercent)
	{
		return m_piUnitCombatModifierPercent[i];
	}

	return -1;
}

/// Returns an array of bonuses when fighting against a type of unit
int CvPromotionEntry::GetUnitClassModifierPercent(int i) const
{
	CvAssertMsg(i < GC.getNumUnitClassInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumUnitClassInfos() && m_piUnitClassModifierPercent)
	{
		return m_piUnitClassModifierPercent[i];
	}

	return -1;
}

/// Returns an array of bonuses when I have no idea
int CvPromotionEntry::GetDomainModifierPercent(int i) const
{
	CvAssertMsg(i < NUM_DOMAIN_TYPES, "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < NUM_DOMAIN_TYPES && m_piDomainModifierPercent)
	{
		return m_piDomainModifierPercent[i];
	}

	return -1;
}



int CvPromotionEntry::GetDomainAttackPercent(int i) const
{
	CvAssertMsg(i < NUM_DOMAIN_TYPES, "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if (i > -1 && i < NUM_DOMAIN_TYPES && m_piDomainAttackPercent)
	{
		return m_piDomainAttackPercent[i];
	}

	return -1;
}


int CvPromotionEntry::GetDomainDefensePercent(int i) const
{
	CvAssertMsg(i < NUM_DOMAIN_TYPES, "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if (i > -1 && i < NUM_DOMAIN_TYPES && m_piDomainDefensePercent)
	{
		return m_piDomainDefensePercent[i];
	}

	return -1;
}

/// Percentage bonus when attacking a specific unit class
int CvPromotionEntry::GetUnitClassAttackModifier(int i) const
{
	CvAssertMsg(i < GC.getNumUnitClassInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumUnitClassInfos() && m_piUnitClassAttackModifier)
	{
		return m_piUnitClassAttackModifier[i];
	}

	return -1;
}

/// Percentage bonus when defending against a specific unit class
int CvPromotionEntry::GetUnitClassDefenseModifier(int i) const
{
	CvAssertMsg(i < GC.getNumUnitClassInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumUnitClassInfos() && m_piUnitClassDefenseModifier)
	{
		return m_piUnitClassDefenseModifier[i];
	}

	return -1;
}

/// Returns an array that indicates if a feature type is traversable by the unit
int CvPromotionEntry::GetFeaturePassableTech(int i) const
{
	CvAssertMsg(i < GC.getNumFeatureInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumFeatureInfos() && m_piFeaturePassableTech)
	{
		return m_piFeaturePassableTech[i];
	}

	return -1;
}

/// Returns an array that indicates if a unit can move twice in a type of terrain
bool CvPromotionEntry::GetTerrainDoubleMove(int i) const
{
	CvAssertMsg(i < GC.getNumTerrainInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumTerrainInfos() && m_pbTerrainDoubleMove)
	{
		return m_pbTerrainDoubleMove[i];
	}

	return false;
}

/// Returns an array that indicates if a unit can move twice in a type of terrain feature
bool CvPromotionEntry::GetFeatureDoubleMove(int i) const
{
	CvAssertMsg(i < GC.getNumFeatureInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumFeatureInfos() && m_pbFeatureDoubleMove)
	{
		return m_pbFeatureDoubleMove[i];
	}

	return false;
}

#if defined(MOD_PROMOTIONS_HALF_MOVE)
/// Returns an array that indicates if a unit can move half as fast in a type of terrain
bool CvPromotionEntry::GetTerrainHalfMove(int i) const
{
	CvAssertMsg(i < GC.getNumTerrainInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumTerrainInfos() && m_pbTerrainHalfMove)
	{
		return m_pbTerrainHalfMove[i];
	}

	return false;
}

/// Returns an array that indicates if a unit can move half as fast in a type of terrain feature
bool CvPromotionEntry::GetFeatureHalfMove(int i) const
{
	CvAssertMsg(i < GC.getNumFeatureInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumFeatureInfos() && m_pbFeatureHalfMove)
	{
		return m_pbFeatureHalfMove[i];
	}

	return false;
}
#endif

/// Returns an array that indicates if a terrain type is impassable
bool CvPromotionEntry::GetTerrainImpassable(int i) const
{
	CvAssertMsg(i < GC.getNumTerrainInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumTerrainInfos() && m_pbTerrainImpassable)
	{
		return m_pbTerrainImpassable[i];
	}

	return false;
}

/// Returns an array that indicates what tech is needed to pass through a terrain type
int CvPromotionEntry::GetTerrainPassableTech(int i) const
{
	CvAssertMsg(i < GC.getNumTerrainInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumTerrainInfos() && m_piTerrainPassableTech)
	{
		return m_piTerrainPassableTech[i];
	}

	return -1;
}

/// Returns an array that indicates what tech is needed to pass through a terrain feature type
bool CvPromotionEntry::GetFeatureImpassable(int i) const
{
	CvAssertMsg(i < GC.getNumFeatureInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumFeatureInfos() && m_pbFeatureImpassable)
	{
		return m_pbFeatureImpassable[i];
	}

	return false;
}






/// Returns the combat classes that this promotion is available for
bool CvPromotionEntry::GetUnitCombatClass(int i) const
{
	CvAssertMsg(i < GC.getNumUnitCombatClassInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumUnitCombatClassInfos() && m_pbUnitCombat)
	{
		return m_pbUnitCombat[i];
	}

	return false;
}



/// Returns the  unit type that this promotion is available for
bool CvPromotionEntry::GetUnitType(int i) const
{
	CvAssertMsg(i < GC.getNumUnitInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if (i > -1 && i < GC.getNumUnitInfos() && m_pbUnitType)
	{
		return m_pbUnitType[i];
	}

	return false;
}


/// Returns the civilian unit type that this promotion is available for
bool CvPromotionEntry::GetCivilianUnitType(int i) const
{
	CvAssertMsg(i < GC.getNumUnitInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");

	if(i > -1 && i < GC.getNumUnitInfos() && m_pbCivilianUnitType)
	{
		return m_pbCivilianUnitType[i];
	}

	return false;
}

#if defined(MOD_PROMOTIONS_UNIT_NAMING)
/// If this a promotion that names a unit
bool CvPromotionEntry::IsUnitNaming(int i) const
{
	CvAssertMsg(i < GC.getNumPromotionInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	return m_pbUnitName ? m_pbUnitName[i] : false;
}

void CvPromotionEntry::GetUnitName(UnitTypes eUnit, CvString& sUnitName) const
{
	Database::Results kDBResults;
	char szQuery[512] = {0};
	sprintf_s(szQuery, "select Name from UnitPromotions_UnitName n, UnitPromotions p, Units u where n.PromotionType = p.Type and n.UnitType = u.Type and p.ID = %i and u.ID = %i;", GetID(), eUnit);
	if(DB.Execute(kDBResults, szQuery))
	{
		while(kDBResults.Step())
		{
			sUnitName = kDBResults.GetText("Name");
		}
	}
}
#endif

/// If this a promotion that can randomly turn into other c
bool CvPromotionEntry::IsPostCombatRandomPromotion(int i) const
{
	CvAssertMsg(i < GC.getNumPromotionInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	return m_pbPostCombatRandomPromotion ? m_pbPostCombatRandomPromotion[i] : false;
}

#if defined(MOD_API_PROMOTION_TO_PROMOTION_MODIFIERS)
int CvPromotionEntry::GetOtherPromotionModifier(PromotionTypes other) const
{
	CvAssertMsg(other < GC.getNumPromotionInfos(), "GetOtherPromotionModifier: upper bound");
	CvAssertMsg(other > -1, "GetOtherPromotionModifier: lower bound");

	if (other <= -1 || other >= GC.getNumPromotionInfos())
	{
		return -1;
	}

	auto iterator = m_pPromotionModifiers.find(other);
	if (iterator == m_pPromotionModifiers.end())
	{
		return 0;
	}

	return iterator->second;
}
int CvPromotionEntry::GetOtherPromotionAttackModifier(PromotionTypes other) const
{
	CvAssertMsg(other < GC.getNumPromotionInfos(), "GetOtherPromotionAttackModifier: upper bound");
	CvAssertMsg(other > -1, "GetOtherPromotionAttackModifier: lower bound");

	if (other <= -1 || other >= GC.getNumPromotionInfos())
	{
		return -1;
	}

	auto iterator = m_pPromotionAttackModifiers.find(other);
	if (iterator == m_pPromotionAttackModifiers.end())
	{
		return 0;
	}

	return iterator->second;
}
int CvPromotionEntry::GetOtherPromotionDefenseModifier(PromotionTypes other) const
{
	CvAssertMsg(other < GC.getNumPromotionInfos(), "GetOtherPromotionDefenseModifier: upper bound");
	CvAssertMsg(other > -1, "GetOtherPromotionDefenseModifier: lower bound");

	if (other <= -1 || other >= GC.getNumPromotionInfos())
	{
		return -1;
	}

	auto iterator = m_pPromotionDefenseModifiers.find(other);
	if (iterator == m_pPromotionDefenseModifiers.end())
	{
		return 0;
	}

	return iterator->second;
}
bool CvPromotionEntry::HasOtherPromotionModifier() const
{
	return !m_pPromotionModifiers.empty() || !m_pPromotionAttackModifiers.empty() || !m_pPromotionDefenseModifiers.empty();
}
std::tr1::unordered_map<PromotionTypes, int>& CvPromotionEntry::GetOtherPromotionModifierMap()
{
	return m_pPromotionModifiers;
}
std::tr1::unordered_map<PromotionTypes, int>& CvPromotionEntry::GetOtherPromotionAttackModifierMap()
{
	return m_pPromotionAttackModifiers;
}
std::tr1::unordered_map<PromotionTypes, int>& CvPromotionEntry::GetOtherPromotionDefenseModifierMap()
{
	return m_pPromotionDefenseModifiers;
}
#endif

#ifdef MOD_GLOBAL_WAR_CASUALTIES
int CvPromotionEntry::GetWarCasualtiesModifier() const
{
	return this->m_iWarCasualtiesModifier;
}
#endif

#ifdef MOD_PROMOTION_SPLASH_DAMAGE
int CvPromotionEntry::GetSplashDamageRadius() const
{
	return m_iSplashDamageRadius;
}
int CvPromotionEntry::GetSplashDamagePercent() const
{
	return m_iSplashDamagePercent;
}
int CvPromotionEntry::GetSplashDamageFixed() const
{
	return m_iSplashDamageFixed;
}
int CvPromotionEntry::GetSplashDamagePlotUnitLimit() const
{
	return m_iSplashDamagePlotUnitLimit;
}
bool CvPromotionEntry::GetSplashDamageImmune() const
{
	return m_iSplashDamageImmune;
}
int CvPromotionEntry::GetSplashXP() const
{
	return m_iSplashXP;
}
#endif

#ifdef MOD_PROMOTION_COLLATERAL_DAMAGE
int CvPromotionEntry::GetCollateralDamagePercent() const
{
	return m_iCollateralDamagePercent;
}
int CvPromotionEntry::GetCollateralDamageFixed() const
{
	return m_iCollateralDamageFixed;
}
int CvPromotionEntry::GetCollateralDamagePlotUnitLimit() const
{
	return m_iCollateralDamagePlotUnitLimit;
}
bool CvPromotionEntry::GetCollateralDamageImmune() const
{
	return m_iCollateralDamageImmune;
}
int CvPromotionEntry::GetCollateralXP() const
{
	return m_iCollateralXP;
}
bool CvPromotionEntry::GetCollateralOnlyCity() const
{
	return m_bCollateralOnlyCity;
}
bool CvPromotionEntry::GetCollateralOnlyUnit() const
{
	return m_bCollateralOnlyUnit;
}
#endif

int CvPromotionEntry::GetAttackInflictDamageChange() const
{
	return m_iAttackInflictDamageChange;
}
int CvPromotionEntry::GetAttackInflictDamageChangeMaxHPPercent() const
{
	return m_iAttackInflictDamageChangeMaxHPPercent;
}

int CvPromotionEntry::GetDefenseInflictDamageChange() const
{
	return m_iDefenseInflictDamageChange;
}
int CvPromotionEntry::GetDefenseInflictDamageChangeMaxHPPercent() const
{
	return m_iDefenseInflictDamageChangeMaxHPPercent;
}

int CvPromotionEntry::GetSiegeInflictDamageChange() const
{
	return m_iSiegeInflictDamageChange;
}
int CvPromotionEntry::GetSiegeInflictDamageChangeMaxHPPercent() const
{
	return m_iSiegeInflictDamageChangeMaxHPPercent;
}

int CvPromotionEntry::GetHeavyChargeAddMoves() const
{
	return m_iHeavyChargeAddMoves;
}
int CvPromotionEntry::GetHeavyChargeExtraDamage() const
{
	return m_iHeavyChargeExtraDamage;
}
int CvPromotionEntry::GetHeavyChargeCollateralFixed() const
{
	return m_iHeavyChargeCollateralFixed;
}
int CvPromotionEntry::GetHeavyChargeCollateralPercent() const
{
	return m_iHeavyChargeCollateralPercent;
}

#ifdef MOD_PROMOTION_ADD_ENEMY_PROMOTIONS
bool CvPromotionEntry::GetAddEnemyPromotionImmune() const
{
	return m_bAddEnemyPromotionImmune;
}
#endif

#ifdef MOD_GLOBAL_PROMOTIONS_REMOVAL
bool CvPromotionEntry::CanAutoRemove() const{
	return m_iRemoveAfterXTurns > 0 || m_bRemoveAfterFullyHeal || m_bRemoveWithLuaCheck;
}

int CvPromotionEntry::GetRemoveAfterXTurns() const{
	return m_iRemoveAfterXTurns;
}

bool CvPromotionEntry::GetRemoveAfterFullyHeal() const{
	return m_bRemoveAfterFullyHeal;
}

bool CvPromotionEntry::GetRemoveWithLuaCheck() const{
	return m_bRemoveWithLuaCheck;
}

bool CvPromotionEntry::GetCanActionClear() const{
	return m_bCanActionClear;
}
#endif

#ifdef MOD_PROMOTION_CITY_DESTROYER
BuildingClassCollectionsTypes CvPromotionEntry::GetDestroyBuildingCollection() const
{
	return m_iDestroyBuildingCollection;
}
int CvPromotionEntry::GetDestroyBuildingProbability() const
{
	return m_iDestroyBuildingProbability;
}
int CvPromotionEntry::GetDestroyBuildingNumLimit() const
{
	return m_iDestroyBuildingNumLimit;
}
bool CvPromotionEntry::CanDestroyBuildings() const
{
	return m_iDestroyBuildingCollection != NO_BUILDINGCLASS_COLLECTION && m_iDestroyBuildingProbability > 0 && m_iDestroyBuildingNumLimit > 0;
}

int CvPromotionEntry::GetSiegeKillCitizensPercent() const
{
	return m_iSiegeKillCitizensPercent;
}
int CvPromotionEntry::GetSiegeKillCitizensFixed() const
{
	return m_iSiegeKillCitizensFixed;
}
#endif

//=====================================
// CvPromotionEntryXMLEntries
//=====================================
/// Constructor
CvPromotionXMLEntries::CvPromotionXMLEntries(void)
{

}

/// Destructor
CvPromotionXMLEntries::~CvPromotionXMLEntries(void)
{
	DeleteArray();
}

/// Returns vector of promotion entries
std::vector<CvPromotionEntry*>& CvPromotionXMLEntries::GetPromotionEntries()
{
	return m_paPromotionEntries;
}

/// Number of defined projects
int CvPromotionXMLEntries::GetNumPromotions()
{
	return m_paPromotionEntries.size();
}

/// Get a specific entry
CvPromotionEntry* CvPromotionXMLEntries::GetEntry(int index)
{
	return m_paPromotionEntries[index];
}

/// Clear promotion entries
void CvPromotionXMLEntries::DeleteArray()
{
	for(std::vector<CvPromotionEntry*>::iterator it = m_paPromotionEntries.begin(); it != m_paPromotionEntries.end(); ++it)
	{
		SAFE_DELETE(*it);
	}

	m_paPromotionEntries.clear();
}

//=====================================
// CvUnitPromotions
//=====================================
/// Constructor
CvUnitPromotions::CvUnitPromotions():
	m_pPromotions(NULL),
	m_pUnit(NULL)
{
}

/// Destructor
CvUnitPromotions::~CvUnitPromotions(void)
{
}

/// Initialize
void CvUnitPromotions::Init(CvPromotionXMLEntries* pPromotions, CvUnit* pUnit)
{
	CvAssert(pPromotions);
	if(!pPromotions)
		return;

	CvAssert(pUnit);
	if(!pUnit)
		return;

	m_pPromotions = pPromotions;
	m_pUnit = pUnit;

	Reset();
}

/// Deallocate memory created in initialize
void CvUnitPromotions::Uninit()
{
	m_kHasPromotion.SetSize(0);
}

/// Reset unit promotion array to all false
void CvUnitPromotions::Reset()
{
	m_kHasPromotion.SetSize(0);
}

#ifdef MOD_API_PROMOTION_TO_PROMOTION_MODIFIERS
inline FDataStream& operator<<(FDataStream& os, const std::tr1::unordered_map<PromotionTypes, int>& promotionIntMap)
{
	os << promotionIntMap.size();
	for (auto iter = promotionIntMap.begin(); iter != promotionIntMap.end(); ++iter)
	{
		os << (int)iter->first;
		os << iter->second;
	}
	return os;
}
inline FDataStream& operator>>(FDataStream& is, std::tr1::unordered_map<PromotionTypes, int>& promotionIntMap)
{
	int iSize;
	is >> iSize;
	for (int i = 0; i < iSize; i++)
	{
		PromotionTypes ePromotion;
		int iValue;
		is >> (int&)ePromotion;
		is >> iValue;
		promotionIntMap[ePromotion] = iValue;
	}
	return is;
}
#endif

/// Serialization read
void CvUnitPromotions::Read(FDataStream& kStream)
{
	Reset();

	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;
	MOD_SERIALIZE_INIT_READ(kStream);

	// Read number of promotions
	int iNumPromotions;
	kStream >> iNumPromotions;
	CvAssertMsg(m_pPromotions != NULL && m_pPromotions->GetNumPromotions() > 0, "Number of promotions to serialize is expected to greater than 0");

#ifdef MOD_API_PROMOTION_TO_PROMOTION_MODIFIERS
	kStream >> m_pPromotionModifiers;
	kStream >> m_pPromotionAttackModifiers;
	kStream >> m_pPromotionDefenseModifiers;
#endif

	PromotionArrayHelpers::Read(kStream, m_kHasPromotion);
}

/// Serialization write
void CvUnitPromotions::Write(FDataStream& kStream) const
{
	// Current version number
	uint uiVersion = 1;
	kStream << uiVersion;
	MOD_SERIALIZE_INIT_WRITE(kStream);

	// Write out number of promotions to save
	int iNumPromotions = m_pPromotions->GetNumPromotions();
	kStream << iNumPromotions;
	CvAssertMsg(iNumPromotions > 0, "Number of promotions to serialize is expected to greater than 0");

#ifdef MOD_API_PROMOTION_TO_PROMOTION_MODIFIERS
	kStream << m_pPromotionModifiers;
	kStream << m_pPromotionAttackModifiers;
	kStream << m_pPromotionDefenseModifiers;
#endif

	PromotionArrayHelpers::Write(kStream, m_kHasPromotion, iNumPromotions);
}

/// Accessor: Unit object
CvUnit* CvUnitPromotions::GetUnit()
{
	return m_pUnit;
}

/// Accessor: Does the unit have a certain promotion
bool CvUnitPromotions::HasPromotion(PromotionTypes eIndex) const
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumPromotionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(eIndex >= 0 && eIndex < GC.getNumPromotionInfos())
	{
		return m_kHasPromotion.GetBit(eIndex);
	}

	return false;
}

static void UpdatePromotionToPromotionModifierMap(std::tr1::unordered_map<PromotionTypes, int>& dest, std::tr1::unordered_map<PromotionTypes, int>& src, int iChange)
{
	for (auto iter = src.begin(); iter != src.end(); iter++)
	{
		PromotionTypes eOtherPromotion = (PromotionTypes)iter->first;
		int iModifier = iter->second;
		dest[eOtherPromotion] += iModifier * iChange;
		if (dest[eOtherPromotion] == 0)
		{
			dest.erase(eOtherPromotion);
		}
	}
}

/// Sets the promotion to a certain value
void CvUnitPromotions::SetPromotion(PromotionTypes eIndex, bool bValue)
{
	CvAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	CvAssertMsg(eIndex < GC.getNumPromotionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(eIndex >= 0 && eIndex < GC.getNumPromotionInfos())
	{
		m_kHasPromotion.SetBit(eIndex, bValue);

#ifdef MOD_API_PROMOTION_TO_PROMOTION_MODIFIERS
		CvPromotionEntry* thisPromotion = GC.getPromotionInfo(eIndex);
		if (MOD_API_PROMOTION_TO_PROMOTION_MODIFIERS && thisPromotion->HasOtherPromotionModifier())
		{
			if (!thisPromotion->GetOtherPromotionModifierMap().empty())
				UpdatePromotionToPromotionModifierMap(m_pPromotionModifiers, thisPromotion->GetOtherPromotionModifierMap(), bValue ? 1 : -1);
			if (!thisPromotion->GetOtherPromotionAttackModifierMap().empty())
				UpdatePromotionToPromotionModifierMap(m_pPromotionAttackModifiers, thisPromotion->GetOtherPromotionAttackModifierMap(), bValue ? 1 : -1);
			if (!thisPromotion->GetOtherPromotionDefenseModifierMap().empty())
				UpdatePromotionToPromotionModifierMap(m_pPromotionDefenseModifiers, thisPromotion->GetOtherPromotionDefenseModifierMap(), bValue ? 1 : -1);
		}
#endif
	}
}

/// determines if the terrain feature is passable given the unit's current promotions
bool CvUnitPromotions::GetAllowFeaturePassable(FeatureTypes eFeatureType) const
{
	CvTeamTechs* teamTechs = GET_TEAM(m_pUnit->getTeam()).GetTeamTechs();
	CvAssert(teamTechs);
	if(!teamTechs) return false;

	int iNumPromos = GC.getNumPromotionInfos();
	for(int iLoop = 0; iLoop < iNumPromos; iLoop++)
	{
		PromotionTypes ePromotion = (PromotionTypes) iLoop;
		if(m_kHasPromotion.GetBit(ePromotion))
		{
			CvPromotionEntry* promotion = GC.getPromotionInfo(ePromotion);
			if(promotion)
			{
				TechTypes eTech = (TechTypes) promotion->GetFeaturePassableTech(eFeatureType);
				if(eTech != NO_TECH && teamTechs->HasTech(eTech))
				{
					return true;
				}
			}
		}
	}

	return false;
}

/// determines if the terrain type is passable given the unit's current promotions
bool CvUnitPromotions::GetAllowTerrainPassable(TerrainTypes eTerrainType) const
{
	CvTeamTechs* teamTechs = GET_TEAM(m_pUnit->getTeam()).GetTeamTechs();
	CvAssert(teamTechs);
	if(!teamTechs) return false;

	int iNumPromos = GC.getNumPromotionInfos();
	for(int iLoop = 0; iLoop < iNumPromos; iLoop++)
	{
		PromotionTypes ePromotion = (PromotionTypes) iLoop;
		if(m_kHasPromotion.GetBit(ePromotion))
		{
			CvPromotionEntry* promotion = GC.getPromotionInfo(ePromotion);

			if(promotion)
			{
				TechTypes eTech = (TechTypes) promotion->GetTerrainPassableTech(eTerrainType);
				if(eTech != NO_TECH && teamTechs->HasTech(eTech))
				{
					return true;
				}
			}
		}
	}
	return false;
}

/// returns the advantage percent when attacking the specified unit class
int CvUnitPromotions::GetUnitClassAttackMod(UnitClassTypes eUnitClass) const
{
	int iSum = 0;
	for(int iLoop = 0; iLoop < GC.getNumPromotionInfos(); iLoop++)
	{
		PromotionTypes ePromotion = (PromotionTypes)iLoop;
		CvPromotionEntry* promotion = GC.getPromotionInfo(ePromotion);
		if(promotion && HasPromotion(ePromotion))
		{
			iSum += promotion->GetUnitClassAttackModifier(eUnitClass);
		}
	}
	return iSum;
}

/// returns the advantage percent when defending against the specified unit class
int CvUnitPromotions::GetUnitClassDefenseMod(UnitClassTypes eUnitClass) const
{
	int iSum = 0;
	for(int iLoop = 0; iLoop < GC.getNumPromotionInfos(); iLoop++)
	{
		PromotionTypes ePromotion = (PromotionTypes)iLoop;
		CvPromotionEntry* promotion = GC.getPromotionInfo(ePromotion);
		if(promotion && HasPromotion(ePromotion))
		{
			iSum += promotion->GetUnitClassDefenseModifier(eUnitClass);
		}
	}
	return iSum;
}

/// returns the advantage percent when attacking the specified unit Domain
int CvUnitPromotions::GetDomainAttackPercentMod(DomainTypes eDomain) const
{
	int iSum = 0;
	for (int iLoop = 0; iLoop < GC.getNumPromotionInfos(); iLoop++)
	{
		PromotionTypes ePromotion = (PromotionTypes)iLoop;
		CvPromotionEntry* promotion = GC.getPromotionInfo(ePromotion);
		if (promotion && HasPromotion(ePromotion))
		{
			iSum += promotion->GetDomainAttackPercent(eDomain);
		}
	}
	return iSum;
}

/// returns the advantage percent when defending against the specified unit Domain
int CvUnitPromotions::GetDomainDefensePercentMod(DomainTypes eDomain) const
{
	int iSum = 0;
	for (int iLoop = 0; iLoop < GC.getNumPromotionInfos(); iLoop++)
	{
		PromotionTypes ePromotion = (PromotionTypes)iLoop;
		CvPromotionEntry* promotion = GC.getPromotionInfo(ePromotion);
		if (promotion && HasPromotion(ePromotion))
		{
			iSum += promotion->GetDomainDefensePercent(eDomain);
		}
	}
	return iSum;
}


// Swap to a new promotion after a combat - returns new promotion we switched to
PromotionTypes CvUnitPromotions::ChangePromotionAfterCombat(PromotionTypes eIndex)
{
	std::vector<int> aPossiblePromotions;

	for (int iI = 0; iI < m_pPromotions->GetNumPromotions(); iI++)
	{
		CvPromotionEntry *pkEntry = m_pPromotions->GetEntry(eIndex);
		if (pkEntry && pkEntry->IsPostCombatRandomPromotion(iI))
		{
			if (!pkEntry->ArePostCombatPromotionsExclusive() || !IsInUseByPlayer((PromotionTypes)iI, m_pUnit->getOwner()))
			{
#if defined(MOD_EVENTS_UNIT_UPGRADES)
				if (MOD_EVENTS_UNIT_UPGRADES) {
					if (GAMEEVENTINVOKE_TESTALL(GAMEEVENT_CanHavePromotion, m_pUnit->getOwner(), m_pUnit->GetID(), iI) == GAMEEVENTRETURN_FALSE) {
						continue;
					}

					if (GAMEEVENTINVOKE_TESTALL(GAMEEVENT_UnitCanHavePromotion, m_pUnit->getOwner(), m_pUnit->GetID(), iI) == GAMEEVENTRETURN_FALSE) {
						continue;
					}
				}
#endif
				aPossiblePromotions.push_back(iI);
			}
		}
	}

	int iNumChoices = aPossiblePromotions.size();
	if (iNumChoices > 0)
	{
		int iChoice = GC.getGame().getJonRandNum(iNumChoices, "Random Promotion Pick");
		return (PromotionTypes)aPossiblePromotions[iChoice];
	}

	return NO_PROMOTION;
}

#if defined(MOD_API_PROMOTION_TO_PROMOTION_MODIFIERS)
int CvUnitPromotions::GetOtherPromotionModifier(PromotionTypes other)
{
	return m_pPromotionModifiers[other];
}
int CvUnitPromotions::GetOtherPromotionAttackModifier(PromotionTypes other)
{
	return m_pPromotionAttackModifiers[other];
}
int CvUnitPromotions::GetOtherPromotionDefenseModifier(PromotionTypes other)
{
	return m_pPromotionDefenseModifiers[other];
}
std::tr1::unordered_map<PromotionTypes, int>& CvUnitPromotions::GetOtherPromotionModifierMap()
{
	return m_pPromotionModifiers;
}
std::tr1::unordered_map<PromotionTypes, int>& CvUnitPromotions::GetOtherPromotionAttackModifierMap()
{
	return m_pPromotionAttackModifiers;
}
std::tr1::unordered_map<PromotionTypes, int>& CvUnitPromotions::GetOtherPromotionDefenseModifierMap()
{
	return m_pPromotionDefenseModifiers;
}
#endif

#if defined(MOD_API_UNIT_CANNOT_BE_RANGED_ATTACKED)
bool CvUnitPromotions::IsCannotBeRangedAttacked() const
{
	for (int iLoop = 0; iLoop < GC.getNumPromotionInfos(); iLoop++)
	{
		PromotionTypes thisPromotionType = (PromotionTypes)iLoop;
		CvPromotionEntry* thisPromotion = GC.getPromotionInfo(thisPromotionType);
		if (thisPromotion == nullptr || !HasPromotion(thisPromotionType))
		{
			continue;
		}

		if (thisPromotion->IsCannotBeRangedAttacked())
		{
			return true;
		}
	}
	return false;
}
#endif

// PRIVATE METHODS

// Is this (post-combat) promotion already present for some unit of this player?
bool CvUnitPromotions::IsInUseByPlayer(PromotionTypes eIndex, PlayerTypes ePlayer)
{
	bool bRtnValue = false;
	CvPlayer &kPlayer = GET_PLAYER(ePlayer);

	CvUnit *pLoopUnit;
	int iValueLoop;
	for(pLoopUnit = kPlayer.firstUnit(&iValueLoop); pLoopUnit != NULL; pLoopUnit = kPlayer.nextUnit(&iValueLoop))
	{
		if (pLoopUnit->isHasPromotion(eIndex))
		{
			return true;
		}
	}

	return bRtnValue;
}

// Read the saved promotions.  Entries are saved as string values, all entries are saved.
void PromotionArrayHelpers::ReadV3(FDataStream& kStream, CvBitfield& kPromotions)
{
	int iNumEntries;
	FStringFixedBuffer(sTemp, 64);
	int iType;

	kStream >> iNumEntries;

	kPromotions.SetSize( iNumEntries );

	for(int iI = 0; iI < iNumEntries; iI++)
	{
		kStream >> sTemp;
		if(sTemp == "PROMOTION_OLIGARCHY")
		{
			bool bTemp;
			kStream >> bTemp;
		}
		else
		{
			iType = GC.getInfoTypeForString(sTemp);
			if(iType != -1)
			{
				bool bValue;
				kStream >> bValue;
				kPromotions.SetBit(iType, bValue);
			}
			else
			{
				CvString szError;
				szError.Format("LOAD ERROR: Promotion Type not found: %s", sTemp);
				GC.LogMessage(szError.GetCString());
				CvAssertMsg(false, szError);
				bool bDummy;
				kStream >> bDummy;
			}
		}
	}
}

// Read the saved promotions.  Entries are saved as hash values
void PromotionArrayHelpers::Read(FDataStream& kStream, CvBitfield& kPromotions)
{
	int iNumEntries;
	int iType;

	kStream >> iNumEntries;

	kPromotions.SetSize( iNumEntries );

	uint uiHashTemp;
	uint uiOligarchyHash = FString::Hash("PROMOTION_OLIGARCHY");
	for(int iI = 0; iI < iNumEntries; iI++)
	{
		kStream >> uiHashTemp;
		if(uiHashTemp == uiOligarchyHash)
		{
			bool bTemp;
			kStream >> bTemp;
		}
		else
		{
			iType = GC.getInfoTypeForHash(uiHashTemp);
			if(iType != -1)
			{
				bool bValue;
				kStream >> bValue;
				kPromotions.SetBit(iType, bValue);
			}
			else
			{
				CvString szError;
				szError.Format("LOAD ERROR: Promotion Type not found for hash: %u", uiHashTemp);
				GC.LogMessage(szError.GetCString());
				CvAssertMsg(false, szError);
				bool bDummy;
				kStream >> bDummy;
			}
		}
	}
}

// Save the promotions.  Entries are saved as hash values and only the entries that are 'on' are saved
void PromotionArrayHelpers::Write(FDataStream& kStream, const CvBitfield& kPromotions, int iArraySize)
{
	// We are only going to save the 'on' bit, so we have to count them
	int iCount = 0;

	for(int iI = 0; iI < iArraySize; iI++)
	{
		const PromotionTypes ePromotion = static_cast<PromotionTypes>(iI);
		if (kPromotions.GetBit(ePromotion) && GC.getPromotionInfo(ePromotion) != NULL)
			++iCount;
	}

	kStream << iCount;

	for(int iI = 0; iI < iArraySize; iI++)
	{
		const PromotionTypes ePromotion = static_cast<PromotionTypes>(iI);
		bool bValue = kPromotions.GetBit(ePromotion);
		if (bValue)
		{
			CvPromotionEntry* pkPromotionInfo = GC.getPromotionInfo(ePromotion);
			if(pkPromotionInfo)
			{
				uint uiHash = FString::Hash( pkPromotionInfo->GetType() );		// Save just the hash
				kStream << uiHash;
				kStream << bValue;
			}
		}
	}
}
