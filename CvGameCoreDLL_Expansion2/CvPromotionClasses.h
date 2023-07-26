/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#pragma once

#ifndef CIV5_PROMOTION_CLASSES_H
#define CIV5_PROMOTION_CLASSES_H

#include "CvBitfield.h"
#include <unordered_map>
#include "CustomMods.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvPromotionEntry
//!  \brief		A single promotion available in the game
//
//!  Key Attributes:
//!  - Used to be called CvPromotionInfo
//!  - Populated from XML\Units\CIV5UnitPromotions.xml
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvPromotionEntry: public CvHotKeyInfo
{
public:
	CvPromotionEntry(void);
	~CvPromotionEntry(void);

	virtual bool CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility);

	int	GetLayerAnimationPath() const;
	int	GetPrereqPromotion() const;
	void	SetPrereqPromotion(int i);
	int	GetPrereqOrPromotion1() const;
	void	SetPrereqOrPromotion1(int i);
	int	GetPrereqOrPromotion2() const;
	void	SetPrereqOrPromotion2(int i);
	int	GetPrereqOrPromotion3() const;
	void	SetPrereqOrPromotion3(int i);
	int	GetPrereqOrPromotion4() const;
	void	SetPrereqOrPromotion4(int i);
	int	GetPrereqOrPromotion5() const;
	void	SetPrereqOrPromotion5(int i);
	int	GetPrereqOrPromotion6() const;
	void	SetPrereqOrPromotion6(int i);
	int	GetPrereqOrPromotion7() const;
	void	SetPrereqOrPromotion7(int i);
	int	GetPrereqOrPromotion8() const;
	void	SetPrereqOrPromotion8(int i);
	int	GetPrereqOrPromotion9() const;
	void	SetPrereqOrPromotion9(int i);


	int	GetPrereqOrPromotion10() const;
	void	SetPrereqOrPromotion10(int i);
	int	GetPrereqOrPromotion11() const;
	void	SetPrereqOrPromotion11(int i);
	int	GetPrereqOrPromotion12() const;
	void	SetPrereqOrPromotion12(int i);
	int	GetPrereqOrPromotion13() const;
	void	SetPrereqOrPromotion13(int i);

	int  GetTechPrereq() const;
	int  GetInvisibleType() const;
	int  GetSeeInvisibleType() const;
	int  GetVisibilityChange() const;
#if defined(MOD_PROMOTION_FEATURE_INVISIBLE)
	int GetFeatureInvisible() const;
	int GetFeatureInvisible2() const;
#endif
#if defined(MOD_PROMOTION_MULTIPLE_INIT_EXPERENCE)
	int GetMultipleInitExperence() const;
#endif
#if defined(MOD_PROMOTIONS_VARIABLE_RECON)
	int  GetReconChange() const;
#endif
	int  GetMovesChange() const;
	int  GetMoveDiscountChange() const;
	int  GetRangeChange() const;
	int  GetRangedAttackModifier() const;
	int  GetInterceptionCombatModifier() const;
	int  GetInterceptionDefenseDamageModifier() const;
	int  GetAirSweepCombatModifier() const;
	int  GetInterceptChanceChange() const;
	int  GetNumInterceptionChange() const;
	int  GetEvasionChange() const;
	int  GetCargoChange() const;
	int  GetEnemyHealChange() const;
	int  GetNeutralHealChange() const;
	int  GetFriendlyHealChange() const;
	int  GetSameTileHealChange() const;
	int  GetAdjacentTileHealChange() const;
	int  GetEnemyDamageChance() const;
	int  GetNeutralDamageChance() const;
	int  GetEnemyDamage() const;
	int  GetNeutralDamage() const;
	int  GetCombatPercent() const;
	int  GetCityAttackPercent() const;
	int  GetCityDefensePercent() const;
	int  GetRangedDefenseMod() const;
	int  GetHillsAttackPercent() const;
	int  GetHillsDefensePercent() const;
	int  GetOpenAttackPercent() const;
	int  GetOpenRangedAttackMod() const;
	int  GetRoughAttackPercent() const;
	int  GetRoughRangedAttackMod() const;
	int  GetAttackFortifiedMod() const;
	int  GetAttackWoundedMod() const;
	int  GetFlankAttackModifier() const;
	int  GetNearbyEnemyCombatMod() const;
	int  GetNearbyEnemyCombatRange() const;
	int  GetOpenDefensePercent() const;
	int  GetRoughDefensePercent() const;
	int  GetExtraAttacks() const;
	bool IsGreatGeneral() const;
	bool IsGreatAdmiral() const;
#if defined(MOD_PROMOTIONS_AURA_CHANGE)
	int GetAuraRangeChange() const;
	int GetAuraEffectChange() const;
#endif
	int  GetGreatGeneralModifier() const;
	bool IsGreatGeneralReceivesMovement() const;
	int  GetGreatGeneralCombatModifier() const;
	int  GetFriendlyLandsModifier() const;
	int  GetFriendlyLandsAttackModifier() const;
	int  GetOutsideFriendlyLandsModifier() const;
	int  GetCommandType() const;
	void SetCommandType(int iNewType);

#if defined(MOD_UNITS_NO_SUPPLY)
	bool IsNoSupply() const;
#endif

#if defined(MOD_UNITS_MAX_HP)
	int  GetMaxHitPointsChange() const;
	int  GetMaxHitPointsModifier() const;
#endif

#if defined(MOD_DEFENSE_MOVES_BONUS)
	int GetMoveLeftDefenseMod() const;
	int GetMoveUsedDefenseMod() const;
#endif

#if defined(MOD_ROG_CORE)
	int GetMoveLfetAttackMod() const;
	int GetMoveUsedAttackMod() const;
	int GetGoldenAgeMod() const;
	int GetRangedSupportFireMod() const;

	int GetMeleeDefenseMod() const;
#endif

#if defined(MOD_ROG_CORE)	
	int GetAoEDamageOnMove() const;
	int ForcedDamageValue() const;
	int ChangeDamageValue() const;
	int  GetAttackFullyHealedMod() const;
	int  GetAttackAboveHealthMod() const;
	int  GetAttackBelowHealthMod() const;
	bool IsStrongerDamaged() const;
	bool IsFightWellDamaged() const;
#endif

#if defined(MOD_ROG_CORE)
	int GetHPHealedIfDefeatEnemyGlobal() const;
	int GetNumOriginalCapitalAttackMod() const;
	int GetNumOriginalCapitalDefenseMod() const;
#endif

#if defined(MOD_ROG_CORE)
	UnitClassTypes GetCombatBonusFromNearbyUnitClass() const;
	int GetNearbyUnitClassBonusRange() const;
	int GetNearbyUnitClassBonus() const;
#endif



	int GetDomainAttackPercent(int i) const;
	int GetDomainDefensePercent(int i) const;


#if defined(MOD_ROG_CORE)
	int GetOnCapitalLandAttackMod() const;
	int GetOutsideCapitalLandAttackMod() const;
	int GetOnCapitalLandDefenseMod() const;
	int GetOutsideCapitalLandDefenseMod() const;

	int GetBarbarianCombatBonus() const;
	int GetAOEDamageOnKill() const;
	int GetDamageAoEFortified() const;
	int GetWorkRateMod() const;
#endif

	bool CannotBeCaptured() const;
	int GetCaptureDefeatedEnemyChance() const;


#if defined(MOD_ROG_CORE)
	int GetNumSpyDefenseMod() const;
	int GetNumSpyAttackMod() const;

	int GetNumWonderDefenseMod() const;
	int GetNumWonderAttackMod() const;

	int GetNumWorkDefenseMod() const;
	int GetNumWorkAttackMod() const;

	bool IsNoResourcePunishment() const;

	int GetCurrentHitPointAttackMod() const;
	int GetCurrentHitPointDefenseMod() const;


	int GetNearNumEnemyAttackMod() const;
	int GetNearNumEnemyDefenseMod() const;
#endif


	int GetUpgradeDiscount() const;
	int GetExperiencePercent() const;
	int GetAdjacentMod() const;
	int GetAttackMod() const;
	int GetDefenseMod() const;

	int GetDropRange() const;
	int GetExtraNavalMoves() const;
	int GetHPHealedIfDefeatEnemy() const;
	int GetGoldenAgeValueFromKills() const;
	int GetExtraWithdrawal() const;
	int GetEmbarkExtraVisibility() const;
	int GetEmbarkDefenseModifier() const;
	int GetCapitalDefenseModifier() const;
	int GetCapitalDefenseFalloff() const;
	int GetCityAttackPlunderModifier() const;
#if defined(MOD_PROMOTION_GET_INSTANCE_FROM_ATTACK)
	int GetUnitAttackFaithBonus() const;
	int GetCityAttackFaithBonus() const;
#endif
#if defined(MOD_PROMOTION_REMOVE_PROMOTION_UPGRADE)
	int GetRemovePromotionUpgrade() const;
#endif
	int GetReligiousStrengthLossRivalTerritory() const;
	
	int GetTradeMissionInfluenceModifier() const;
	int GetTradeMissionGoldModifier() const;

	bool IsCannotBeChosen() const;
	bool IsLostWithUpgrade() const;
	bool IsNotWithUpgrade() const;
	bool IsInstaHeal() const;
	bool IsLeader() const;
	bool IsBlitz() const;
	bool IsAmphib() const;
	bool IsRiver() const;
	bool IsEnemyRoute() const;
	bool IsRivalTerritory() const;
	bool IsMustSetUpToRangedAttack() const;
	bool IsRangedSupportFire() const;
	bool IsAlwaysHeal() const;
	bool IsHealOutsideFriendly() const;
	bool IsHillsDoubleMove() const;

	bool IsIgnoreTerrainCost() const;
#if defined(MOD_API_PLOT_BASED_DAMAGE)
	bool IsIgnoreTerrainDamage() const;
	bool IsIgnoreFeatureDamage() const;
	bool IsExtraTerrainDamage() const;
	bool IsExtraFeatureDamage() const;
#endif
#if defined(MOD_PROMOTIONS_IMPROVEMENT_BONUS)
	int GetNearbyImprovementCombatBonus() const;
	int GetNearbyImprovementBonusRange() const;
	ImprovementTypes GetCombatBonusImprovement() const;
#endif
#if defined(MOD_PROMOTIONS_ALLYCITYSTATE_BONUS)
	int GetAllyCityStateCombatModifier() const;
	int GetAllyCityStateCombatModifierMax() const;
#endif

#if defined(MOD_PROMOTIONS_EXTRARES_BONUS)
	ResourceTypes GetExtraResourceType() const;
	int GetExtraResourceCombatModifier() const;
	int GetExtraResourceCombatModifierMax() const;
	int GetExtraHappinessCombatModifier() const;
	int GetExtraHappinessCombatModifierMax() const;
#endif

#if defined(MOD_PROMOTIONS_CROSS_MOUNTAINS)
	bool CanCrossMountains() const;
#endif
#if defined(MOD_PROMOTIONS_CROSS_OCEANS)
	bool CanCrossOceans() const;
#endif
#if defined(MOD_PROMOTIONS_CROSS_ICE)
	bool CanCrossIce() const;
#endif
#if defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
	bool IsGGFromBarbarians() const;
#endif
	bool IsRoughTerrainEndsTurn() const;
	bool IsHoveringUnit() const;
	bool IsFlatMovementCost() const;
	bool IsCanMoveImpassable() const;
	bool IsNoCapture() const;
	bool IsOnlyDefensive() const;
	bool IsNoDefensiveBonus() const;
	bool IsNukeImmune() const;
	bool IsHiddenNationality() const;
	bool IsAlwaysHostile() const;
	bool IsNoRevealMap() const;
	bool IsRecon() const;
	bool CanMoveAllTerrain() const;
	bool IsCanMoveAfterAttacking() const;
	bool IsAirSweepCapable() const;
	bool IsAllowsEmbarkation() const;
	bool IsRangeAttackIgnoreLOS() const;
	bool IsFreePillageMoves() const;
	bool IsHealOnPillage() const;
	bool IsHealIfDefeatExcludeBarbarians() const;
	bool IsEmbarkedAllWater() const;
#if defined(MOD_PROMOTIONS_DEEP_WATER_EMBARKATION)
	bool IsEmbarkedDeepWater() const;
#endif
	bool IsCityAttackOnly() const;
	bool IsCaptureDefeatedEnemy() const;
	bool IsIgnoreGreatGeneralBenefit() const;
	bool IsIgnoreZOC() const;
	bool IsImmueMeleeAttack() const;
	bool IsSapper() const;
	bool IsCanHeavyCharge() const;
	bool HasPostCombatPromotions() const;
	bool ArePostCombatPromotionsExclusive() const;

	const char* GetSound() const;
	void SetSound(const char* szVal);

	// Arrays
	int GetTerrainAttackPercent(int i) const;
	int GetTerrainDefensePercent(int i) const;
	int GetFeatureAttackPercent(int i) const;
	int GetFeatureDefensePercent(int i) const;
#if defined(MOD_API_UNIFIED_YIELDS)
	int GetYieldFromKills(int i) const;
	int GetYieldFromBarbarianKills(int i) const;
#endif
	int GetUnitCombatModifierPercent(int i) const;
	int GetUnitClassModifierPercent(int i) const;
	int GetDomainModifierPercent(int i) const;
	int GetFeaturePassableTech(int i) const;
	int GetUnitClassAttackModifier(int i) const;
	int GetUnitClassDefenseModifier(int i) const;

	bool GetTerrainDoubleMove(int i) const;
	bool GetFeatureDoubleMove(int i) const;
#if defined(MOD_PROMOTIONS_HALF_MOVE)
	bool GetTerrainHalfMove(int i) const;
	bool GetFeatureHalfMove(int i) const;
#endif
	bool GetTerrainImpassable(int i) const;
	int  GetTerrainPassableTech(int i) const;
	bool GetFeatureImpassable(int i) const;
	bool GetUnitCombatClass(int i) const;
	bool GetCivilianUnitType(int i) const;

#if defined(MOD_ROG_CORE)
	bool GetUnitType(int i) const;
#endif

#if defined(MOD_PROMOTIONS_UNIT_NAMING)
	bool IsUnitNaming(int i) const;
	void GetUnitName(UnitTypes eUnit, CvString& sUnitName) const;
#endif
	bool IsPostCombatRandomPromotion(int i) const;

#if defined(MOD_API_PROMOTION_TO_PROMOTION_MODIFIERS)
	int GetOtherPromotionModifier(PromotionTypes other) const;
	int GetOtherPromotionAttackModifier(PromotionTypes other) const;
	int GetOtherPromotionDefenseModifier(PromotionTypes other) const;
	bool HasOtherPromotionModifier() const;
	std::tr1::unordered_map<PromotionTypes, int>& GetOtherPromotionModifierMap();
	std::tr1::unordered_map<PromotionTypes, int>& GetOtherPromotionAttackModifierMap();
	std::tr1::unordered_map<PromotionTypes, int>& GetOtherPromotionDefenseModifierMap();
#endif

#if defined(MOD_API_UNIT_CANNOT_BE_RANGED_ATTACKED)
	bool IsCannotBeRangedAttacked() const;
#endif

 #ifdef MOD_GLOBAL_WAR_CASUALTIES
	int GetWarCasualtiesModifier() const;
 #endif

 #ifdef MOD_PROMOTION_ADD_ENEMY_PROMOTIONS
	bool GetAddEnemyPromotionImmune() const;
 #endif

#ifdef MOD_GLOBAL_PROMOTIONS_REMOVAL
	bool CanAutoRemove() const;
	int GetRemoveAfterXTurns() const;
	bool GetRemoveAfterFullyHeal() const;
	bool GetRemoveWithLuaCheck() const;
	bool GetCanActionClear() const;
#endif

#ifdef MOD_PROMOTION_CITY_DESTROYER
	BuildingClassCollectionsTypes GetDestroyBuildingCollection() const;
	int GetDestroyBuildingProbability() const;
	int GetDestroyBuildingNumLimit() const;
	bool CanDestroyBuildings() const;

	int GetSiegeKillCitizensPercent() const;
	int GetSiegeKillCitizensFixed() const;
#endif

 #ifdef MOD_PROMOTION_SPLASH_DAMAGE
	int GetSplashDamageRadius() const;
	int GetSplashDamagePercent() const;
	int GetSplashDamageFixed() const;
	int GetSplashDamagePlotUnitLimit() const;
	bool GetSplashDamageImmune() const;
	int GetSplashXP() const;
#endif

#ifdef MOD_PROMOTION_COLLATERAL_DAMAGE
	int GetCollateralDamagePercent() const;
	int GetCollateralDamageFixed() const;
	int GetCollateralDamagePlotUnitLimit() const;
	bool GetCollateralDamageImmune() const;
	int GetCollateralXP() const;
	bool GetCollateralOnlyCity() const;
	bool GetCollateralOnlyUnit() const;
#endif

	int GetAttackInflictDamageChange() const;
	int GetAttackInflictDamageChangeMaxHPPercent() const;

	int GetDefenseInflictDamageChange() const;
	int GetDefenseInflictDamageChangeMaxHPPercent() const;

	int GetSiegeInflictDamageChange() const;
	int GetSiegeInflictDamageChangeMaxHPPercent() const;

	int GetHeavyChargeAddMoves() const;
	int GetHeavyChargeExtraDamage() const;
	int GetHeavyChargeCollateralFixed() const;
	int GetHeavyChargeCollateralPercent() const;

protected:
	int m_iLayerAnimationPath;
	int m_iPrereqPromotion;
	int m_iPrereqOrPromotion1;
	int m_iPrereqOrPromotion2;
	int m_iPrereqOrPromotion3;
	int m_iPrereqOrPromotion4;
	int m_iPrereqOrPromotion5;
	int m_iPrereqOrPromotion6;
	int m_iPrereqOrPromotion7;
	int m_iPrereqOrPromotion8;
	int m_iPrereqOrPromotion9;
	int m_iPrereqOrPromotion10;
	int m_iPrereqOrPromotion11;
	int m_iPrereqOrPromotion12;
	int m_iPrereqOrPromotion13;

	int m_iTechPrereq;
	int m_iInvisibleType;
	int m_iSeeInvisibleType;
#if defined(MOD_PROMOTION_FEATURE_INVISIBLE)
	int m_iFeatureInvisible;
	int m_iFeatureInvisible2;
#endif
#if defined(MOD_PROMOTION_MULTIPLE_INIT_EXPERENCE)
	int m_iMultipleInitExperence;
#endif
	int m_iVisibilityChange;
#if defined(MOD_PROMOTIONS_VARIABLE_RECON)
	int m_iReconChange;
#endif
	int m_iMovesChange;
	int m_iMoveDiscountChange;
	int m_iRangeChange;
	int m_iRangedAttackModifier;
	int m_iInterceptionCombatModifier;
	int m_iInterceptionDefenseDamageModifier;
	int m_iAirSweepCombatModifier;
	int m_iInterceptChanceChange;
	int m_iNumInterceptionChange;
	int m_iEvasionChange;
	int m_iCargoChange;
	int m_iEnemyHealChange;
	int m_iNeutralHealChange;
	int m_iFriendlyHealChange;
	int m_iSameTileHealChange;
	int m_iAdjacentTileHealChange;
	int m_iEnemyDamageChance;
	int m_iNeutralDamageChance;
	int m_iEnemyDamage;
	int m_iNeutralDamage;
	int m_iCombatPercent;
	int m_iCityAttackPercent;
	int m_iCityDefensePercent;
	int m_iRangedDefenseMod;
	int m_iHillsAttackPercent;
	int m_iHillsDefensePercent;
	int m_iOpenAttackPercent;
	int m_iOpenRangedAttackMod;
	int m_iRoughAttackPercent;
	int m_iRoughRangedAttackMod;
	int m_iAttackFortifiedMod;
	int m_iAttackWoundedMod;
	int m_iFlankAttackModifier;
	int m_iNearbyEnemyCombatMod;
	int m_iNearbyEnemyCombatRange;
	int m_iOpenDefensePercent;
	int m_iRoughDefensePercent;
	int m_iExtraAttacks;
	bool m_bGreatGeneral;
	bool m_bGreatAdmiral;
#if defined(MOD_PROMOTIONS_AURA_CHANGE)
	int m_iAuraRangeChange;
	int m_iAuraEffectChange;
#endif
	int m_iGreatGeneralModifier;
	bool m_bGreatGeneralReceivesMovement;
	int m_iGreatGeneralCombatModifier;
	int m_iFriendlyLandsModifier;
	int m_iFriendlyLandsAttackModifier;
	int m_iOutsideFriendlyLandsModifier;
	int m_iCommandType;
#if defined(MOD_UNITS_NO_SUPPLY)
	bool m_bNoSupply;
#endif
#if defined(MOD_UNITS_MAX_HP)
	int m_iMaxHitPointsChange;
	int m_iMaxHitPointsModifier;
#endif
	int m_iUpgradeDiscount;
	int m_iExperiencePercent;
	int m_iAdjacentMod;
	int m_iAttackMod;
	int m_iDefenseMod;
	int m_iDropRange;
	int m_iExtraNavalMoves;
	int m_iHPHealedIfDefeatEnemy;
	int m_iGoldenAgeValueFromKills;
	int m_iExtraWithdrawal;
	int m_iEmbarkExtraVisibility;
	int m_iEmbarkDefenseModifier;
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

#if defined(MOD_DEFENSE_MOVES_BONUS)
	int m_iMoveLeftDefenseMod;
	int m_iMoveUsedDefenseMod;
#endif

#if defined(MOD_ROG_CORE)
	int m_iMoveLfetAttackMod;
	int m_iMoveUsedAttackMod;
	int m_iGoldenAgeMod;

	int m_iRangedSupportFireMod;
	int m_iMeleeDefenseMod;
#endif

#ifdef MOD_PROMOTION_SPLASH_DAMAGE
	int m_iSplashDamagePercent = 0;
	int m_iSplashDamageRadius = 0;
	int m_iSplashDamageFixed = 0;
	int m_iSplashDamagePlotUnitLimit = 0;
	bool m_iSplashDamageImmune = 0;
	int m_iSplashXP = 0;
#endif

#ifdef MOD_PROMOTION_COLLATERAL_DAMAGE
	int m_iCollateralDamagePercent = 0;
	int m_iCollateralDamageFixed = 0;
	int m_iCollateralDamagePlotUnitLimit = 0;
	bool m_iCollateralDamageImmune = 0;
	int m_iCollateralXP = 0;
	bool m_bCollateralOnlyCity = false;
	bool m_bCollateralOnlyUnit = true;
#endif

#if defined(MOD_ROG_CORE)
	int m_iNumSpyDefenseMod;
	int m_iNumSpyAttackMod;

	int m_iNumWonderDefenseMod;
	int m_iNumWonderAttackMod;

	int m_iNumWorkDefenseMod;
	int m_iNumWorkAttackMod;

	bool m_bNoResourcePunishment;

	int m_iCurrentHitPointAttackMod;
	int m_iCurrentHitPointDefenseMod;

	int m_iNearNumEnemyAttackMod;
	int m_iNearNumEnemyDefenseMod;
#endif

#if defined(MOD_ROG_CORE)
	int m_iNearbyUnitClassBonus;
	int m_iNearbyUnitClassBonusRange;
	UnitClassTypes m_iCombatBonusFromNearbyUnitClass;


	int m_iAOEDamageOnKill;
	int m_iDamageAoEFortified;
	int m_iWorkRateMod;
	int m_iBarbarianCombatBonus;
#endif

	int m_iCaptureDefeatedEnemyChance;
	bool m_bCannotBeCaptured;

#if defined(MOD_ROG_CORE)	
	int m_iAoEDamageOnMove;
	int m_iForcedDamageValue;
	int m_iChangeDamageValue;
	int m_iAttackFullyHealedMod;
	int m_iAttackAboveHealthMod;
	int m_iAttackBelowHealthMod;
	bool m_bStrongerDamaged;
	bool m_bFightWellDamaged;
#endif



	int* m_piDomainAttackPercent;
	int* m_piDomainDefensePercent;

#if defined(MOD_ROG_CORE)
	int m_iHPHealedIfDefeatEnemyGlobal;
	int m_iNumOriginalCapitalAttackMod;
	int m_iNumOriginalCapitalDefenseMod;
#endif



#if defined(MOD_ROG_CORE)
	int m_iOnCapitalLandAttackMod;
	int m_iOutsideCapitalLandAttackMod;
	int m_iOnCapitalLandDefenseMod;
	int m_iOutsideCapitalLandDefenseMod;
#endif


	bool m_bCannotBeChosen;
	bool m_bLostWithUpgrade;
	bool m_bNotWithUpgrade;
	bool m_bInstaHeal;
	bool m_bLeader;
	bool m_bBlitz;
	bool m_bAmphib;
	bool m_bRiver;
	bool m_bEnemyRoute;
	bool m_bRivalTerritory;
	bool m_bMustSetUpToRangedAttack;
	bool m_bRangedSupportFire;
	bool m_bAlwaysHeal;
	bool m_bHealOutsideFriendly;
	bool m_bHillsDoubleMove;
	bool m_bIgnoreTerrainCost;
#if defined(MOD_API_PLOT_BASED_DAMAGE)
	bool m_bIgnoreTerrainDamage;
	bool m_bIgnoreFeatureDamage;
	bool m_bExtraTerrainDamage;
	bool m_bExtraFeatureDamage;
#endif
#if defined(MOD_PROMOTIONS_IMPROVEMENT_BONUS)
	int m_iNearbyImprovementCombatBonus;
	int m_iNearbyImprovementBonusRange;
	ImprovementTypes m_eCombatBonusImprovement;
#endif
#if defined(MOD_PROMOTIONS_ALLYCITYSTATE_BONUS)
	int m_iAllyCityStateCombatModifier;
	int m_iAllyCityStateCombatModifierMax;
#endif
#if defined(MOD_PROMOTIONS_EXTRARES_BONUS)
	ResourceTypes m_eExtraResourceType;
	int m_iExtraResourceCombatModifier;
	int m_iExtraResourceCombatModifierMax;
	int m_iExtraHappinessCombatModifier;
	int m_iExtraHappinessCombatModifierMax;
#endif
#if defined(MOD_PROMOTIONS_CROSS_MOUNTAINS)
	bool m_bCanCrossMountains;
#endif
#if defined(MOD_PROMOTIONS_CROSS_OCEANS)
	bool m_bCanCrossOceans;
#endif
#if defined(MOD_PROMOTIONS_CROSS_ICE)
	bool m_bCanCrossIce;
#endif
#if defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
	bool m_bGGFromBarbarians;
#endif
	bool m_bRoughTerrainEndsTurn;
	bool m_bHoveringUnit;
	bool m_bFlatMovementCost;
	bool m_bCanMoveImpassable;
	bool m_bNoCapture;
	bool m_bOnlyDefensive;
	bool m_bNoDefensiveBonus;
	bool m_bNukeImmune;
	bool m_bHiddenNationality;
	bool m_bAlwaysHostile;
	bool m_bNoRevealMap;
	bool m_bRecon;
	bool m_bCanMoveAllTerrain;
	bool m_bCanMoveAfterAttacking;
	bool m_bAirSweepCapable;
	bool m_bAllowsEmbarkation;
	bool m_bRangeAttackIgnoreLOS;
	bool m_bFreePillageMoves;
	bool m_bHealOnPillage;
	bool m_bHealIfDefeatExcludesBarbarians;
	bool m_bEmbarkedAllWater;
#if defined(MOD_PROMOTIONS_DEEP_WATER_EMBARKATION)
	bool m_bEmbarkedDeepWater;
#endif
	bool m_bCityAttackOnly;
	bool m_bCaptureDefeatedEnemy;
	bool m_bIgnoreGreatGeneralBenefit;
	bool m_bIgnoreZOC;
	bool m_bImmueMeleeAttack;
	bool m_bHasPostCombatPromotions;
	bool m_bPostCombatPromotionsExclusive;
	bool m_bSapper;
	bool m_bCanHeavyCharge;

	CvString m_strSound;

	// Arrays
	int* m_piTerrainAttackPercent;
	int* m_piTerrainDefensePercent;
	int* m_piFeatureAttackPercent;
	int* m_piFeatureDefensePercent;
#if defined(MOD_API_UNIFIED_YIELDS)
	int* m_piYieldFromKills;
	int* m_piYieldFromBarbarianKills;
#endif
	int* m_piUnitCombatModifierPercent;
	int* m_piUnitClassModifierPercent;
	int* m_piDomainModifierPercent;

	int* m_piUnitClassAttackModifier;
	int* m_piUnitClassDefenseModifier;

	int* m_piTerrainPassableTech;
	int* m_piFeaturePassableTech;

	bool* m_pbTerrainDoubleMove;
	bool* m_pbFeatureDoubleMove;
#if defined(MOD_PROMOTIONS_HALF_MOVE)
	bool* m_pbTerrainHalfMove;
	bool* m_pbFeatureHalfMove;
#endif
	bool* m_pbTerrainImpassable;
	bool* m_pbFeatureImpassable;
	bool* m_pbUnitCombat;
	bool* m_pbCivilianUnitType;


	bool* m_pbUnitType;


#if defined(MOD_PROMOTIONS_UNIT_NAMING)
	bool* m_pbUnitName;
#endif
	bool* m_pbPostCombatRandomPromotion;

 #if defined(MOD_API_PROMOTION_TO_PROMOTION_MODIFIERS)
	std::tr1::unordered_map<PromotionTypes, int> m_pPromotionModifiers; // key: other promotion type, value: modifier * 100
	std::tr1::unordered_map<PromotionTypes, int> m_pPromotionAttackModifiers; // key: other promotion type, value: attack modifier * 100
	std::tr1::unordered_map<PromotionTypes, int> m_pPromotionDefenseModifiers; // key: other promotion type, value: defense modifier * 100
 #endif

 #if defined(MOD_API_UNIT_CANNOT_BE_RANGED_ATTACKED)
	bool m_bCannotBeRangedAttacked;
 #endif

 #ifdef MOD_GLOBAL_WAR_CASUALTIES
	int m_iWarCasualtiesModifier = 0;
 #endif

#ifdef MOD_PROMOTION_ADD_ENEMY_PROMOTIONS
	bool m_bAddEnemyPromotionImmune = 0;
#endif

#ifdef MOD_GLOBAL_PROMOTIONS_REMOVAL
	int m_iRemoveAfterXTurns = 0;
	bool m_bRemoveAfterFullyHeal = 0;
	bool m_bRemoveWithLuaCheck = 0;
	bool m_bCanActionClear = 0;
#endif

#ifdef MOD_PROMOTION_CITY_DESTROYER
	BuildingClassCollectionsTypes m_iDestroyBuildingCollection = NO_BUILDINGCLASS_COLLECTION;
	int m_iDestroyBuildingProbability = 0;
	int m_iDestroyBuildingNumLimit = 0;

	int m_iSiegeKillCitizensPercent = 0;
	int m_iSiegeKillCitizensFixed = 0;
#endif

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
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvPromotionXMLEntries
//!  \brief		Game-wide information about promotions
//
//! Key Attributes:
//! - Plan is it will be contained in CvGameRules object within CvGame class
//! - Populated from XML\GameInfo\CIV5PromotionInfo.xml
//! - Contains an array of CvPromotionEntry from the above XML file
//! - One instance for the entire game
//! - Accessed heavily by the [what stores info on projects built?] class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvPromotionXMLEntries
{
public:
	CvPromotionXMLEntries(void);
	~CvPromotionXMLEntries(void);

	// Accessor functions
	std::vector<CvPromotionEntry*>& GetPromotionEntries();
	int GetNumPromotions();
	_Ret_maybenull_ CvPromotionEntry* GetEntry(int index);

	// Binary cache functions
	void DeleteArray();

private:
	std::vector<CvPromotionEntry*> m_paPromotionEntries;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvUnitPromotions
//!  \brief		Information about the promotions of a single unit
//
//!  Key Attributes:
//!  - Plan is it will be contained in CvPlayerState object within CvUnit class
//!  - One instance for each unit
//!  - Accessed by any class that needs to check promotions
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvUnitPromotions
{
public:
	CvUnitPromotions(void);
	~CvUnitPromotions(void);
	void Init(CvPromotionXMLEntries* pPromotions, CvUnit* pUnit);
	void Uninit();
	void Reset();
	void Read(FDataStream& kStream);
	void Write(FDataStream& kStream) const;

	CvUnit* GetUnit();

	bool HasPromotion(PromotionTypes eIndex) const;
	void SetPromotion(PromotionTypes eIndex, bool bValue);

	bool GetAllowFeaturePassable(FeatureTypes eFeatureType) const;
	bool GetAllowTerrainPassable(TerrainTypes eTerrainType) const ;

	int GetUnitClassAttackMod(UnitClassTypes eUnitClass) const;
	int GetUnitClassDefenseMod(UnitClassTypes eUnitClass) const;

	PromotionTypes ChangePromotionAfterCombat(PromotionTypes eIndex);

	int GetDomainAttackPercentMod(DomainTypes eDomain) const;
	int GetDomainDefensePercentMod(DomainTypes eDomain) const;

#if defined(MOD_API_PROMOTION_TO_PROMOTION_MODIFIERS)
	int GetOtherPromotionModifier(PromotionTypes other);
	int GetOtherPromotionAttackModifier(PromotionTypes other);
	int GetOtherPromotionDefenseModifier(PromotionTypes other);
	std::tr1::unordered_map<PromotionTypes, int>& CvUnitPromotions::GetOtherPromotionModifierMap();
	std::tr1::unordered_map<PromotionTypes, int>& CvUnitPromotions::GetOtherPromotionAttackModifierMap();
	std::tr1::unordered_map<PromotionTypes, int>& CvUnitPromotions::GetOtherPromotionDefenseModifierMap();
#endif

#if defined(MOD_API_UNIT_CANNOT_BE_RANGED_ATTACKED)
	bool IsCannotBeRangedAttacked() const;
#endif

private:

#if defined(MOD_API_PROMOTION_TO_PROMOTION_MODIFIERS)
	std::tr1::unordered_map<PromotionTypes, int> m_pPromotionModifiers; // key: other promotion type, value: modifier * 100
	std::tr1::unordered_map<PromotionTypes, int> m_pPromotionAttackModifiers; // key: other promotion type, value: attack modifier * 100
	std::tr1::unordered_map<PromotionTypes, int> m_pPromotionDefenseModifiers; // key: other promotion type, value: defense modifier * 100
#endif

	bool IsInUseByPlayer(PromotionTypes eIndex, PlayerTypes ePlayer); 

	CvBitfield m_kHasPromotion;
	CvPromotionXMLEntries* m_pPromotions;
	CvUnit* m_pUnit;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Helper Functions to serialize arrays of variable length (based on number of promotions defined in game)
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
namespace PromotionArrayHelpers
{
void ReadV3(FDataStream& kStream, CvBitfield& kPromotions);
void Read(FDataStream& kStream, CvBitfield& kPromotions);
void Write(FDataStream& kStream, const CvBitfield& kPromotions, int iArraySize);
}

#endif //CIV5_PROMOTION_CLASSES_H

#ifdef MOD_PROMOTION_SPLASH_DAMAGE
struct SplashInfo {
	PromotionTypes ePromotion;

	int iRadius;
	int iPercent;
	int iFixed;
	int iPlotUnitLimit;

	SplashInfo() = default;

	SplashInfo(const CvPromotionEntry& promotion) :
		ePromotion{ (PromotionTypes)promotion.GetID()}, 
		iRadius{promotion.GetSplashDamageRadius()}, 
		iPercent{ promotion.GetSplashDamagePercent() }, 
		iFixed{ promotion.GetSplashDamageFixed() }, 
		iPlotUnitLimit{ promotion.GetSplashDamagePlotUnitLimit() } {}

	inline void read(FDataStream& kStream) {
		int iPromotion;
		kStream >> iPromotion;
		ePromotion = (PromotionTypes)iPromotion;
		kStream >> iRadius;
		kStream >> iPercent;
		kStream >> iFixed;
		kStream >> iPlotUnitLimit;
	}

	inline void write(FDataStream& kStream) const {
		int iPromotion = (int)ePromotion;
		kStream << iPromotion;
		kStream << iRadius;
		kStream << iPercent;
		kStream << iFixed;
		kStream << iPlotUnitLimit;
	}
};
#endif

#ifdef MOD_PROMOTION_COLLATERAL_DAMAGE
struct CollateralInfo {
	PromotionTypes ePromotion;

	int iPercent = 0;
	int iFixed = 0;
	int iPlotUnitLimit = 0;
	bool bOnlyUnit = true;
	bool bOnlyCity = false;

	CollateralInfo() = default;

	CollateralInfo(const CvPromotionEntry& promotion) :
		ePromotion{ (PromotionTypes)promotion.GetID() },
		iPercent{ promotion.GetCollateralDamagePercent() },
		iFixed{ promotion.GetCollateralDamageFixed() },
		iPlotUnitLimit{ promotion.GetCollateralDamagePlotUnitLimit() },
		bOnlyCity {promotion.GetCollateralOnlyCity()},
		bOnlyUnit {promotion.GetCollateralOnlyUnit()} {}

	inline void read(FDataStream& kStream) {
		int iPromotion;
		kStream >> iPromotion;
		ePromotion = (PromotionTypes)iPromotion;
		kStream >> iPercent;
		kStream >> iFixed;
		kStream >> iPlotUnitLimit;
		kStream >> bOnlyUnit;
		kStream >> bOnlyCity;
	}

	inline void write(FDataStream& kStream) const {
		int iPromotion = (int)ePromotion;
		kStream << iPromotion;
		kStream << iPercent;
		kStream << iFixed;
		kStream << iPlotUnitLimit;
		kStream << bOnlyUnit;
		kStream << bOnlyCity;
	}
};
#endif

#ifdef MOD_GLOBAL_PROMOTIONS_REMOVAL
struct AutoRemoveInfo {
	PromotionTypes m_ePromotion = NO_PROMOTION;

	int m_iTurnToRemove = -1;
	bool m_bRemoveAfterFullyHeal = false;
	bool m_bRemoveLuaCheck = false;

	AutoRemoveInfo() = default;

	AutoRemoveInfo(const CvPromotionEntry& entry, int iTurnToRemove):
		m_ePromotion((PromotionTypes)entry.GetID()),
		m_iTurnToRemove(iTurnToRemove),
		m_bRemoveAfterFullyHeal(entry.GetRemoveAfterFullyHeal()),
		m_bRemoveLuaCheck(entry.GetRemoveWithLuaCheck()) {}
};

inline FDataStream& operator<<(FDataStream& os, const AutoRemoveInfo& info) {
	os << (int)info.m_ePromotion;
	os << info.m_iTurnToRemove;
	os << info.m_bRemoveAfterFullyHeal;
	os << info.m_bRemoveLuaCheck;

	return os;
}

inline FDataStream& operator>>(FDataStream& is, AutoRemoveInfo& info) {
	int iPromotion = -1;
	is >> iPromotion;
	info.m_ePromotion = (PromotionTypes)iPromotion;
	is >> info.m_iTurnToRemove;
	is >> info.m_bRemoveAfterFullyHeal;
	is >> info.m_bRemoveLuaCheck;

	return is;
}
#endif

#ifdef MOD_PROMOTION_CITY_DESTROYER
struct DestroyBuildingsInfo {
	PromotionTypes ePromotion;

	BuildingClassCollectionsTypes m_iDestroyBuildingCollection = NO_BUILDINGCLASS_COLLECTION;
	int m_iDestroyBuildingProbability = 0;
	int m_iDestroyBuildingNumLimit = 0;

	DestroyBuildingsInfo() = default;
	DestroyBuildingsInfo(const CvPromotionEntry& entry):
		ePromotion{ (PromotionTypes)entry.GetID() },
		m_iDestroyBuildingCollection{ entry.GetDestroyBuildingCollection() },
		m_iDestroyBuildingProbability{ entry.GetDestroyBuildingProbability() },
		m_iDestroyBuildingNumLimit{ entry.GetDestroyBuildingNumLimit() } {}
};

inline FDataStream& operator<<(FDataStream& os, const DestroyBuildingsInfo& info) {
	os << (int)info.ePromotion;
	os << (int)info.m_iDestroyBuildingCollection;
	os << info.m_iDestroyBuildingProbability;
	os << info.m_iDestroyBuildingNumLimit;

	return os;
}

inline FDataStream& operator>>(FDataStream& is, DestroyBuildingsInfo& info) {
	int iPromotion = -1;
	int iBuildingClassType = -1;
	is >> iPromotion;
	info.ePromotion = (PromotionTypes)iPromotion;
	is >> iBuildingClassType;
	info.m_iDestroyBuildingCollection = (BuildingClassCollectionsTypes)iBuildingClassType;
	is >> info.m_iDestroyBuildingProbability;
	is >> info.m_iDestroyBuildingNumLimit;

	return is;
}

#endif