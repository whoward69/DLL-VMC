/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include "CvUnit.h"
#include "CvUnitCombat.h"
#include "CvUnitMission.h"
#include "CvGameCoreUtils.h"
#include "ICvDLLUserInterface.h"
#include "CvDiplomacyAI.h"
#include "CvTypes.h"

#include "CvDllCity.h"
#include "CvDllUnit.h"
#include "CvDllCombatInfo.h"

// include this after all other headers
#include "LintFree.h"

// Maximum damage members for the nuke, units and cities
#define MAX_NUKE_DAMAGE_MEMBERS	64

#define POST_QUICK_COMBAT_DELAY	110
#define POST_COMBAT_DELAY		1

//	---------------------------------------------------------------------------
static int GetPostCombatDelay()
{
	return CvPreGame::quickCombat()?POST_QUICK_COMBAT_DELAY:POST_COMBAT_DELAY;
}

//	---------------------------------------------------------------------------
// Find a object in the combat member array
static CvCombatMemberEntry* FindCombatMember(CvCombatMemberEntry* pkArray, int iMembers, IDInfo kMember, CvCombatMemberEntry::MEMBER_TYPE eType)
{
	if(iMembers > 0)
	{
		while(iMembers--)
		{
			if(pkArray->IsType(eType))
			{
				if(pkArray->GetID() == kMember.iID && pkArray->GetPlayer() == kMember.eOwner)
					return pkArray;
			}
			++pkArray;
		}
	}

	return NULL;
}

//	---------------------------------------------------------------------------
// Add a member to the combat array
static CvCombatMemberEntry* AddCombatMember(CvCombatMemberEntry* pkArray, int* piMembers, int iMaxMembers, IDInfo kMember, CvCombatMemberEntry::MEMBER_TYPE eType, int iX, int iY, EraTypes eEra)
{
	if(*piMembers < iMaxMembers)
	{
		int iCount = *piMembers;
		if(!FindCombatMember(pkArray, iCount, kMember, eType))
		{
			CvCombatMemberEntry& kEntry = pkArray[iCount];
			kEntry.SetPlayer(kMember.eOwner);
			kEntry.SetID(kMember.iID, eType);
			kEntry.SetDamage(0);
			kEntry.SetFinalDamage(0);
			kEntry.SetMaxHitPoints(0);
			kEntry.SetPlot(iX, iY);
			kEntry.SetEra(eEra);
			*piMembers += 1;
			return &(pkArray[iCount]);
		}
	}
	return NULL;
}

//	---------------------------------------------------------------------------
// Add a unit member to the combat array
static CvCombatMemberEntry* AddCombatMember(CvCombatMemberEntry* pkArray, int* piMembers, int iMaxMembers, CvUnit* pkMember)
{
	if(pkMember)
		return AddCombatMember(pkArray, piMembers, iMaxMembers, pkMember->GetIDInfo(), CvCombatMemberEntry::MEMBER_UNIT, pkMember->getX(), pkMember->getY(), GET_PLAYER(pkMember->getOwner()).GetCurrentEra());

	return NULL;
}

//	---------------------------------------------------------------------------
// Add a city member to the combat array
static CvCombatMemberEntry* AddCombatMember(CvCombatMemberEntry* pkArray, int* piMembers, int iMaxMembers, CvCity* pkMember)
{
	if(pkMember)
		return AddCombatMember(pkArray, piMembers, iMaxMembers, pkMember->GetIDInfo(), CvCombatMemberEntry::MEMBER_CITY, pkMember->getX(), pkMember->getY(), GET_PLAYER(pkMember->getOwner()).GetCurrentEra());

	return NULL;
}

//	---------------------------------------------------------------------------
void CvUnitCombat::GenerateMeleeCombatInfo(CvUnit& kAttacker, CvUnit* pkDefender, CvPlot& plot, CvCombatInfo* pkCombatInfo)
{
	BATTLE_STARTED(BATTLE_TYPE_MELEE, plot);
	pkCombatInfo->setBattleType(BATTLE_TYPE_MELEE);
#if defined(MOD_UNITS_MAX_HP)
	int iAttackerMaxHP = kAttacker.GetMaxHitPoints();
#else
	int iMaxHP = GC.getMAX_HIT_POINTS();
#endif

	pkCombatInfo->setUnit(BATTLE_UNIT_ATTACKER, &kAttacker);
	pkCombatInfo->setUnit(BATTLE_UNIT_DEFENDER, pkDefender);
	pkCombatInfo->setPlot(&plot);

	// Attacking a City
	if(plot.isCity())
	{
		// Unit vs. City (non-ranged so the city will retaliate
		CvCity* pkCity = plot.getPlotCity();
		BATTLE_JOINED(pkCity, BATTLE_UNIT_DEFENDER, true);
		int iMaxCityHP = pkCity->GetMaxHitPoints();

		int iAttackerStrength = kAttacker.GetMaxAttackStrength(kAttacker.plot(), &plot, NULL);
		int iDefenderStrength = pkCity->getStrengthValue();

		int iAttackerDamageInflicted = kAttacker.getCombatDamage(iAttackerStrength, iDefenderStrength, kAttacker.getDamage(), /*bIncludeRand*/ true, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ true);
		int iDefenderDamageInflicted = kAttacker.getCombatDamage(iDefenderStrength, iAttackerStrength, pkCity->getDamage(), /*bIncludeRand*/ true, /*bAttackerIsCity*/ true, /*bDefenderIsCity*/ false);

#if defined(MOD_ROG_CORE)
		InflictDamageContext ctx;
		ctx.pDefenderCity = pkCity;
		ctx.pAttackerUnit = &kAttacker;
		ctx.piAttackInflictDamage = &iAttackerDamageInflicted;
		ctx.piDefenseInflictDamage = &iDefenderDamageInflicted;
		ctx.bMelee = true;
		InterveneInflictDamage(&ctx);
#endif

		int iAttackerTotalDamageInflicted = iAttackerDamageInflicted + pkCity->getDamage();
		int iDefenderTotalDamageInflicted = iDefenderDamageInflicted + kAttacker.getDamage();

		// Will both the attacker die, and the city fall? If so, the unit wins
#if defined(MOD_UNITS_MAX_HP)
		if (iAttackerTotalDamageInflicted >= iMaxCityHP && iDefenderTotalDamageInflicted >= iAttackerMaxHP)
		{
			iDefenderDamageInflicted = iAttackerMaxHP - kAttacker.getDamage() - 1;
			iDefenderTotalDamageInflicted = iAttackerMaxHP - 1;
		}
#else
		if (iAttackerTotalDamageInflicted >= iMaxCityHP && iDefenderTotalDamageInflicted >= iMaxHP)
		{
			iDefenderDamageInflicted = iMaxHP - kAttacker.getDamage() - 1;
			iDefenderTotalDamageInflicted = iMaxHP - 1;
		}
#endif

		pkCombatInfo->setFinalDamage(BATTLE_UNIT_ATTACKER, iDefenderTotalDamageInflicted);
		pkCombatInfo->setDamageInflicted(BATTLE_UNIT_ATTACKER, iAttackerDamageInflicted);
		pkCombatInfo->setFinalDamage(BATTLE_UNIT_DEFENDER, iAttackerTotalDamageInflicted);
		pkCombatInfo->setDamageInflicted(BATTLE_UNIT_DEFENDER, iDefenderDamageInflicted);

		int iExperience = /*5*/ GC.getEXPERIENCE_ATTACKING_CITY_MELEE();
#ifdef MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE
		if (MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE)
		{
			iExperience += kAttacker.ExtraAttackXPValue();
		}
#endif
		pkCombatInfo->setExperience(BATTLE_UNIT_ATTACKER, iExperience);
		pkCombatInfo->setMaxExperienceAllowed(BATTLE_UNIT_ATTACKER, MAX_INT);
		pkCombatInfo->setInBorders(BATTLE_UNIT_ATTACKER, plot.getOwner() == pkCity->getOwner());
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS) || defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
		pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, kAttacker.isGGFromBarbarians() || !kAttacker.isBarbarian());
#else
		pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, !kAttacker.isBarbarian());
#endif

		pkCombatInfo->setExperience(BATTLE_UNIT_DEFENDER, 0);
		pkCombatInfo->setMaxExperienceAllowed(BATTLE_UNIT_DEFENDER, kAttacker.maxXPValue());
		pkCombatInfo->setInBorders(BATTLE_UNIT_DEFENDER, plot.getOwner() == kAttacker.getOwner());
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS) || defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
		pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_DEFENDER, !pkCity->isBarbarian()); // Kind of irrelevant, as cities don't get XP!
#else
		pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_DEFENDER, !pkCity->isBarbarian());
#endif

		pkCombatInfo->setAttackIsRanged(false);
		pkCombatInfo->setDefenderRetaliates(true);
	}
	// Attacking a Unit
	else
	{
		// Unit vs. Unit
		CvAssert(pkDefender != NULL);

#if defined(MOD_UNITS_MAX_HP)
		int iDefenderMaxHP = pkDefender->GetMaxHitPoints();
#endif

		int iDefenderStrength = pkDefender->GetMaxDefenseStrength(&plot, &kAttacker);
		int iAttackerStrength = 0;
		if(kAttacker.GetMaxRangedCombatStrength(NULL, /*pCity*/ NULL, true, true) > 0 && kAttacker.getDomainType() == DOMAIN_AIR)
		{
			iAttackerStrength = kAttacker.GetMaxRangedCombatStrength(NULL, /*pCity*/ NULL, true, true);
			if(pkDefender->getDomainType() != DOMAIN_AIR)
			{
				iDefenderStrength /= 2;
			}
		}
		else
		{
			iAttackerStrength = kAttacker.GetMaxAttackStrength(kAttacker.plot(), &plot, pkDefender);
		}

		if (kAttacker.IsCanHeavyCharge() && !pkDefender->CanFallBackFromMelee(kAttacker))
		{
			iAttackerStrength = (iAttackerStrength * 150) / 100;
		}

		int iAttackerDamageInflicted = kAttacker.getCombatDamage(iAttackerStrength, iDefenderStrength, kAttacker.getDamage(), /*bIncludeRand*/ true, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ false);
		int iDefenderDamageInflicted = pkDefender->getCombatDamage(iDefenderStrength, iAttackerStrength, pkDefender->getDamage(), /*bIncludeRand*/ true, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ false);

		if(kAttacker.getDomainType() == DOMAIN_AIR && pkDefender->getDomainType() != DOMAIN_AIR)
		{
			iAttackerDamageInflicted /= 2;
			iDefenderDamageInflicted /= 3;
		}

#if defined(MOD_ROG_CORE)
		InflictDamageContext ctx;
		ctx.pDefenderUnit = pkDefender;
		ctx.pAttackerUnit = &kAttacker;
		ctx.piAttackInflictDamage = &iAttackerDamageInflicted;
		ctx.piDefenseInflictDamage = &iDefenderDamageInflicted;
		ctx.bMelee = true;
		InterveneInflictDamage(&ctx);
#endif

		int iAttackerTotalDamageInflicted = iAttackerDamageInflicted + pkDefender->getDamage();
		int iDefenderTotalDamageInflicted = iDefenderDamageInflicted + kAttacker.getDamage();

		// Will both units be killed by this? :o If so, take drastic corrective measures
#if defined(MOD_UNITS_MAX_HP)
		if (iAttackerTotalDamageInflicted >= iDefenderMaxHP && iDefenderTotalDamageInflicted >= iAttackerMaxHP)
#else
		if (iAttackerTotalDamageInflicted >= iMaxHP && iDefenderTotalDamageInflicted >= iMaxHP)
#endif
		{
			// He who hath the least amount of damage survives with 1 HP left
			if(iAttackerTotalDamageInflicted > iDefenderTotalDamageInflicted)
			{
#if defined(MOD_UNITS_MAX_HP)
				iDefenderDamageInflicted = iAttackerMaxHP - kAttacker.getDamage() - 1;
				iDefenderTotalDamageInflicted = iAttackerMaxHP - 1;
				iAttackerTotalDamageInflicted = iDefenderMaxHP;
#else
				iDefenderDamageInflicted = iMaxHP - kAttacker.getDamage() - 1;
				iDefenderTotalDamageInflicted = iMaxHP - 1;
				iAttackerTotalDamageInflicted = iMaxHP;
#endif
			}
			else
			{
#if defined(MOD_UNITS_MAX_HP)
				iAttackerDamageInflicted = iDefenderMaxHP - pkDefender->getDamage() - 1;
				iAttackerTotalDamageInflicted = iDefenderMaxHP - 1;
				iDefenderTotalDamageInflicted = iAttackerMaxHP;
#else
				iAttackerDamageInflicted = iMaxHP - pkDefender->getDamage() - 1;
				iAttackerTotalDamageInflicted = iMaxHP - 1;
				iDefenderTotalDamageInflicted = iMaxHP;
#endif
			}
		}

		pkCombatInfo->setFinalDamage(BATTLE_UNIT_ATTACKER, iDefenderTotalDamageInflicted);
		pkCombatInfo->setDamageInflicted(BATTLE_UNIT_ATTACKER, iAttackerDamageInflicted);
		pkCombatInfo->setFinalDamage(BATTLE_UNIT_DEFENDER, iAttackerTotalDamageInflicted);
		pkCombatInfo->setDamageInflicted(BATTLE_UNIT_DEFENDER, iDefenderDamageInflicted);

		// Fear Damage
		pkCombatInfo->setFearDamageInflicted(BATTLE_UNIT_ATTACKER, kAttacker.getCombatDamage(iAttackerStrength, iDefenderStrength, kAttacker.getDamage(), true, false, true));
		//	pkCombatInfo->setFearDamageInflicted( BATTLE_UNIT_DEFENDER, getCombatDamage(iDefenderStrength, iAttackerStrength, pDefender->getDamage(), true, false, true) );

#if defined(MOD_UNITS_MAX_HP)
		int iAttackerEffectiveStrength = iAttackerStrength * (iAttackerMaxHP - range(kAttacker.getDamage(), 0, iAttackerMaxHP - 1)) / iAttackerMaxHP;
#else
		int iAttackerEffectiveStrength = iAttackerStrength * (iMaxHP - range(kAttacker.getDamage(), 0, iMaxHP - 1)) / iMaxHP;
#endif
		iAttackerEffectiveStrength = iAttackerEffectiveStrength > 0 ? iAttackerEffectiveStrength : 1;
#if defined(MOD_UNITS_MAX_HP)
		int iDefenderEffectiveStrength = iDefenderStrength * (iDefenderMaxHP - range(pkDefender->getDamage(), 0, iDefenderMaxHP - 1)) / iDefenderMaxHP;
#else
		int iDefenderEffectiveStrength = iDefenderStrength * (iMaxHP - range(pkDefender->getDamage(), 0, iMaxHP - 1)) / iMaxHP;
#endif
		iDefenderEffectiveStrength = iDefenderEffectiveStrength > 0 ? iDefenderEffectiveStrength : 1;

		//int iExperience = kAttacker.defenseXPValue();
		//iExperience = ((iExperience * iAttackerEffectiveStrength) / iDefenderEffectiveStrength); // is this right? looks like more for less [Jon: Yes, it's XP for the defender]
		//iExperience = range(iExperience, GC.getMIN_EXPERIENCE_PER_COMBAT(), GC.getMAX_EXPERIENCE_PER_COMBAT());
		int iExperience = /*4*/ GC.getEXPERIENCE_DEFENDING_UNIT_MELEE();
#ifdef MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE
		if (MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE && pkDefender)
		{
			iExperience += pkDefender->ExtraDefenseXPValue();
		}
#endif
		pkCombatInfo->setExperience(BATTLE_UNIT_DEFENDER, iExperience);
		pkCombatInfo->setMaxExperienceAllowed(BATTLE_UNIT_DEFENDER, kAttacker.maxXPValue());
		pkCombatInfo->setInBorders(BATTLE_UNIT_DEFENDER, plot.getOwner() == pkDefender->getOwner());
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS) || defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
		pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_DEFENDER, pkDefender->isGGFromBarbarians() || !kAttacker.isBarbarian());
#else
		pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_DEFENDER, !kAttacker.isBarbarian());
#endif

		//iExperience = ((iExperience * iDefenderEffectiveStrength) / iAttackerEffectiveStrength);
		//iExperience = range(iExperience, GC.getMIN_EXPERIENCE_PER_COMBAT(), GC.getMAX_EXPERIENCE_PER_COMBAT());
		iExperience = /*6*/ GC.getEXPERIENCE_ATTACKING_UNIT_MELEE();
#ifdef MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE
		if (MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE)
		{
			iExperience += kAttacker.ExtraAttackXPValue();
		}
#endif
		pkCombatInfo->setExperience(BATTLE_UNIT_ATTACKER, iExperience);
		pkCombatInfo->setMaxExperienceAllowed(BATTLE_UNIT_ATTACKER, pkDefender->maxXPValue());
		pkCombatInfo->setInBorders(BATTLE_UNIT_ATTACKER, plot.getOwner() == kAttacker.getOwner());
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS) || defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
		pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, kAttacker.isGGFromBarbarians() || !pkDefender->isBarbarian());
#else
		pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, !pkDefender->isBarbarian());
#endif

		pkCombatInfo->setAttackIsRanged(false);

		bool bAdvance = true;
		if(plot.getNumDefenders(pkDefender->getOwner()) > 1)
		{
			bAdvance = false;
		}
#if defined(MOD_UNITS_MAX_HP)
		else if (iAttackerTotalDamageInflicted >= iDefenderMaxHP && kAttacker.IsCaptureDefeatedEnemy() && kAttacker.AreUnitsOfSameType(*pkDefender))
#else
		else if (iAttackerTotalDamageInflicted >= iMaxHP && kAttacker.IsCaptureDefeatedEnemy() && kAttacker.AreUnitsOfSameType(*pkDefender))
#endif
		{
			int iCaptureRoll = GC.getGame().getJonRandNum(100, "Capture Enemy Roll");

			if (iCaptureRoll < kAttacker.GetCaptureChance(pkDefender))
			{
				bAdvance = false;
				pkCombatInfo->setDefenderCaptured(true);
			}
		}
		else if (kAttacker.IsCanHeavyCharge() && !pkDefender->isDelayedDeath() && (iAttackerDamageInflicted > iDefenderDamageInflicted) )
		{
			bAdvance = true;
		}

#if defined(MOD_GLOBAL_NO_FOLLOWUP_FROM_CITIES)
		// If the attacker is in a city, don't advance
		if (MOD_GLOBAL_NO_FOLLOWUP_FROM_CITIES && kAttacker.plot()->isCity()) {
			CUSTOMLOG("Attacker %s is in a city/fort/citadel at (%i, %i) - they will not follow up", kAttacker.getName().GetCString(), kAttacker.getX(), kAttacker.getY());
			bAdvance = false;
		}
#endif

#if defined(MOD_GLOBAL_NO_FOLLOWUP) || defined(MOD_EVENTS_UNIT_ACTIONS)
		ImprovementTypes eImprovement = kAttacker.plot()->getImprovementType();
		
		if (eImprovement != NO_IMPROVEMENT) {
#if defined(MOD_GLOBAL_NO_FOLLOWUP)
			if (MOD_GLOBAL_NO_FOLLOWUP && GC.getImprovementInfo(eImprovement)->IsNoFollowup()) {
				// The attacker is in a designated improvement (fort, citadel, etc), so don't advance
				CUSTOMLOG("Attacker %s is in a designated improvement (%i) at (%i, %i) - they will not follow up", kAttacker.getName().GetCString(), eImprovement, kAttacker.getX(), kAttacker.getY());
				bAdvance = false;
			}
#endif

#if defined(MOD_EVENTS_UNIT_ACTIONS)
			if (MOD_EVENTS_UNIT_ACTIONS && bAdvance) {
				if (GAMEEVENTINVOKE_TESTALL(GAMEEVENT_UnitCanFollowupFrom, kAttacker.getOwner(), kAttacker.GetID(), eImprovement, kAttacker.getX(), kAttacker.getY(), pkDefender->getX(), pkDefender->getY()) == GAMEEVENTRETURN_FALSE) {
					bAdvance = false;
				}
			}
#endif
		}
#endif

		pkCombatInfo->setAttackerAdvances(bAdvance);
		pkCombatInfo->setDefenderRetaliates(true);
	}

	GC.GetEngineUserInterface()->setDirty(UnitInfo_DIRTY_BIT, true);
}

//	---------------------------------------------------------------------------
void CvUnitCombat::ResolveMeleeCombat(const CvCombatInfo& kCombatInfo, uint uiParentEventID)
{
	// After combat stuff
	CvString strBuffer;
	bool bAttackerDead = false;
	bool bDefenderDead = false;
	int iAttackerDamageDelta = 0;

	CvUnit* pkAttacker = kCombatInfo.getUnit(BATTLE_UNIT_ATTACKER);
	CvUnit* pkDefender = kCombatInfo.getUnit(BATTLE_UNIT_DEFENDER);
	CvPlot* pkTargetPlot = kCombatInfo.getPlot();
	if(!pkTargetPlot && pkDefender)
		pkTargetPlot = pkDefender->plot();

	CvAssert_Debug(pkAttacker && pkDefender && pkTargetPlot);

	int iActivePlayerID = GC.getGame().getActivePlayer();

	bool bAttackerDidMoreDamage = false;

	if(pkAttacker != NULL && pkDefender != NULL && pkTargetPlot != NULL &&
	        pkDefender->IsCanDefend()) 		// Did the defender actually defend?
	{
		// Internal variables
		int iAttackerDamageInflicted = kCombatInfo.getDamageInflicted(BATTLE_UNIT_ATTACKER);
		int iDefenderDamageInflicted = kCombatInfo.getDamageInflicted(BATTLE_UNIT_DEFENDER);
		int iAttackerFearDamageInflicted = 0;//pInfo->getFearDamageInflicted( BATTLE_UNIT_ATTACKER );

		bAttackerDidMoreDamage = iAttackerDamageInflicted > iDefenderDamageInflicted;

#if !defined(NO_ACHIEVEMENTS)
		//One Hit
#if defined(MOD_UNITS_MAX_HP)
		if(pkDefender->GetCurrHitPoints() == pkDefender->GetMaxHitPoints() && iAttackerDamageInflicted >= pkDefender->GetCurrHitPoints()  // Defender at full hit points and will the damage be more than the full hit points?
#else
		if(pkDefender->GetCurrHitPoints() == GC.getMAX_HIT_POINTS() && iAttackerDamageInflicted >= pkDefender->GetCurrHitPoints()  // Defender at full hit points and will the damage be more than the full hit points?
#endif
		        && pkAttacker->isHuman() && !GC.getGame().isGameMultiPlayer())
		{
			gDLL->UnlockAchievement(ACHIEVEMENT_ONEHITKILL);
		}
#endif
#if defined(MOD_PROMOTION_GET_INSTANCE_FROM_ATTACK)
		DoInstantYieldFromCombat(kCombatInfo);
#endif		
#if defined(MOD_API_UNIT_STATS)
		pkDefender->changeDamage(iAttackerDamageInflicted, pkAttacker->getOwner(), pkAttacker->GetID());
		iAttackerDamageDelta = pkAttacker->changeDamage(iDefenderDamageInflicted, pkDefender->getOwner(), pkDefender->GetID(), -1.f);		// Signal that we don't want the popup text.  It will be added later when the unit is at its final location
#else
		pkDefender->changeDamage(iAttackerDamageInflicted, pkAttacker->getOwner());
		iAttackerDamageDelta = pkAttacker->changeDamage(iDefenderDamageInflicted, pkDefender->getOwner(), -1.f);		// Signal that we don't want the popup text.  It will be added later when the unit is at its final location
#endif

		// Update experience for both sides.
#if defined(MOD_UNITS_XP_TIMES_100)
		pkDefender->changeExperienceTimes100(100 *
#else
		pkDefender->changeExperience(
#endif
		    kCombatInfo.getExperience(BATTLE_UNIT_DEFENDER),
		    kCombatInfo.getMaxExperienceAllowed(BATTLE_UNIT_DEFENDER),
		    true,
		    kCombatInfo.getInBorders(BATTLE_UNIT_DEFENDER),
		    kCombatInfo.getUpdateGlobal(BATTLE_UNIT_DEFENDER));

#if defined(MOD_UNITS_XP_TIMES_100)
		pkAttacker->changeExperienceTimes100(100 * 
#else
		pkAttacker->changeExperience(
#endif
		    kCombatInfo.getExperience(BATTLE_UNIT_ATTACKER),
		    kCombatInfo.getMaxExperienceAllowed(BATTLE_UNIT_ATTACKER),
		    true,
		    kCombatInfo.getInBorders(BATTLE_UNIT_ATTACKER),
		    kCombatInfo.getUpdateGlobal(BATTLE_UNIT_ATTACKER));

		// Anyone eat it?
#if defined(MOD_UNITS_MAX_HP)
		bAttackerDead = (pkAttacker->getDamage() >= pkAttacker->GetMaxHitPoints());
		bDefenderDead = (pkDefender->getDamage() >= pkDefender->GetMaxHitPoints());
#else
		bAttackerDead = (pkAttacker->getDamage() >= GC.getMAX_HIT_POINTS());
		bDefenderDead = (pkDefender->getDamage() >= GC.getMAX_HIT_POINTS());
#endif

#if !defined(NO_ACHIEVEMENTS)
		CvPlayerAI& kAttackerOwner = GET_PLAYER(pkAttacker->getOwner());
		kAttackerOwner.GetPlayerAchievements().AttackedUnitWithUnit(pkAttacker, pkDefender);
#endif

		// Attacker died
		if(bAttackerDead)
		{
#if !defined(NO_ACHIEVEMENTS)
			CvPlayerAI& kDefenderOwner = GET_PLAYER(pkDefender->getOwner());
			kDefenderOwner.GetPlayerAchievements().KilledUnitWithUnit(pkDefender, pkAttacker);
#endif

			auto_ptr<ICvUnit1> pAttacker = GC.WrapUnitPointer(pkAttacker);
			gDLL->GameplayUnitDestroyedInCombat(pAttacker.get());
			
			if(iActivePlayerID == pkAttacker->getOwner())
			{
				strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_DIED_ATTACKING", pkAttacker->getNameKey(), pkDefender->getNameKey(), iAttackerDamageInflicted, iAttackerFearDamageInflicted);
				GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitDefeatScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
				MILITARYLOG(pkAttacker->getOwner(), strBuffer.c_str(), pkAttacker->plot(), pkDefender->getOwner());
			}
			if(iActivePlayerID == pkDefender->getOwner())
			{
				strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_KILLED_ENEMY_UNIT", pkDefender->getNameKey(), iAttackerDamageInflicted, iAttackerFearDamageInflicted, pkAttacker->getNameKey(), pkAttacker->getVisualCivAdjective(pkDefender->getTeam()));
				GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitVictoryScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
				MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkAttacker->getOwner());
			}
			pkDefender->testPromotionReady();

			ApplyPostCombatTraitEffects(pkDefender, pkAttacker);

		}
		// Defender died
		else if(bDefenderDead)
		{
#if !defined(NO_ACHIEVEMENTS)
			kAttackerOwner.GetPlayerAchievements().KilledUnitWithUnit(pkAttacker, pkDefender);
#endif

			auto_ptr<ICvUnit1> pDefender = GC.WrapUnitPointer(pkDefender);
			gDLL->GameplayUnitDestroyedInCombat(pDefender.get());

			if(iActivePlayerID == pkAttacker->getOwner())
			{
				strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_DESTROYED_ENEMY", pkAttacker->getNameKey(), iDefenderDamageInflicted, pkDefender->getNameKey());
				GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitVictoryScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
				MILITARYLOG(pkAttacker->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkDefender->getOwner());
			}

			if(pkAttacker->getVisualOwner(pkDefender->getTeam()) != pkAttacker->getOwner())
			{
				strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_WAS_DESTROYED_UNKNOWN", pkDefender->getNameKey(), pkAttacker->getNameKey(), iDefenderDamageInflicted);
			}
			else
			{
				strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_WAS_DESTROYED", pkDefender->getNameKey(), pkAttacker->getNameKey(), pkAttacker->getVisualCivAdjective(pkDefender->getTeam()), iDefenderDamageInflicted);
			}
			if(iActivePlayerID == pkDefender->getOwner())
			{
				GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*,GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitDefeatScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
				MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pkDefender->plot(), ((pkAttacker->getVisualOwner(pkDefender->getTeam()) != pkAttacker->getOwner()) ? NO_PLAYER : pkAttacker->getOwner()));
			}
			CvNotifications* pNotification = GET_PLAYER(pkDefender->getOwner()).GetNotifications();
			if(pNotification)
			{
				Localization::String strSummary = Localization::Lookup("TXT_KEY_UNIT_LOST");
				pNotification->Add(NOTIFICATION_UNIT_DIED, strBuffer, strSummary.toUTF8(), pkDefender->getX(), pkDefender->getY(), (int) pkDefender->getUnitType(), pkDefender->getOwner());
			}

			pkAttacker->testPromotionReady();

			ApplyPostCombatTraitEffects(pkAttacker, pkDefender);

			// If defender captured, mark who captured him
			if (kCombatInfo.getDefenderCaptured())
			{
				pkDefender->setCapturingPlayer(pkAttacker->getOwner());
#ifdef MOD_BATTLE_CAPTURE_NEW_RULE
				pkDefender->setCapturingUnit(pkAttacker);
#endif
				pkDefender->SetCapturedAsIs(true);
			}
		}
		// Nobody died
		else
		{
			if(iActivePlayerID == pkAttacker->getOwner())
			{
				strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_WITHDRAW", pkAttacker->getNameKey(), iDefenderDamageInflicted, pkDefender->getNameKey(), iAttackerDamageInflicted);
				GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, "AS2D_OUR_WITHDRAWL", MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
				MILITARYLOG(pkAttacker->getOwner(), strBuffer.c_str(), pkAttacker->plot(), pkDefender->getOwner());
			}
			if(iActivePlayerID == pkDefender->getOwner())
			{
				strBuffer = GetLocalizedText("TXT_KEY_MISC_ENEMY_UNIT_WITHDRAW", pkAttacker->getNameKey(), iDefenderDamageInflicted, pkDefender->getNameKey(), iAttackerDamageInflicted);
				GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, "AS2D_THEIR_WITHDRAWL", MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
				MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkAttacker->getOwner());
			}

			pkDefender->testPromotionReady();
			pkAttacker->testPromotionReady();

		}

		// Minors want Barbs near them dead
		if(bAttackerDead)
		{
			if(pkAttacker->isBarbarian())
				pkAttacker->DoTestBarbarianThreatToMinorsWithThisUnitsDeath(pkDefender->getOwner());
		}
		else if(bDefenderDead)
		{

#if defined(MOD_ROG_CORE)
			pkDefender->DoAdjacentPlotDamage(pkTargetPlot, pkDefender->getAOEDamageOnKill());
#endif
			if(pkDefender->isBarbarian())
				pkDefender->DoTestBarbarianThreatToMinorsWithThisUnitsDeath(pkAttacker->getOwner());
		}
	}

	if(pkAttacker)
	{
		pkAttacker->setCombatUnit(NULL);
		pkAttacker->ClearMissionQueue(GetPostCombatDelay());
	}
	if(pkDefender)
	{
		pkDefender->setCombatUnit(NULL);
		pkDefender->ClearMissionQueue();
	}
	else
		bDefenderDead = true;

	if(pkAttacker)
	{
		if(pkAttacker->isSuicide())
		{
			pkAttacker->setCombatUnit(NULL);	// Must clear this if doing a delayed kill, should this be part of the kill method?
			pkAttacker->kill(true);
		}
		else
		{
			if(pkTargetPlot)
			{
				if (pkAttacker->IsCanHeavyCharge() && bAttackerDidMoreDamage)
				{
					if (!pkDefender->isDelayedDeath())
						pkDefender->DoFallBackFromMelee(*pkAttacker);
					DoHeavyChargeEffects(pkAttacker, pkDefender, pkTargetPlot);
				}

				bool bCanAdvance = kCombatInfo.getAttackerAdvances() && pkTargetPlot->getNumVisibleEnemyDefenders(pkAttacker) == 0;
				if(bCanAdvance)
				{
					if(kCombatInfo.getAttackerAdvancedVisualization())
						// The combat vis has already 'moved' the unit.  Have the game side just do its movement calculations and pop the unit to the new location.
						pkAttacker->move(*pkTargetPlot, false);
					else
						pkAttacker->UnitMove(pkTargetPlot, true, pkAttacker);

					pkAttacker->PublishQueuedVisualizationMoves();


				}
				else
				{
					pkAttacker->changeMoves(-1 * std::max(GC.getMOVE_DENOMINATOR(), pkTargetPlot->movementCost(pkAttacker, pkAttacker->plot())));

					if(!pkAttacker->canMove() || !pkAttacker->isBlitz())
					{
						if(pkAttacker->IsSelected())
						{
							if(GC.GetEngineUserInterface()->GetLengthSelectionList() > 1)
							{
								auto_ptr<ICvUnit1> pDllAttacker = GC.WrapUnitPointer(pkAttacker);
								GC.GetEngineUserInterface()->RemoveFromSelectionList(pDllAttacker.get());
							}
						}
					}
				}
			}

			// If a Unit loses his moves after attacking, do so
			if(!pkAttacker->canMoveAfterAttacking())
			{
				pkAttacker->finishMoves();
				GC.GetEngineUserInterface()->changeCycleSelectionCounter(1);
			}

			// Now that the attacker is in their final location, show any damage popup
			if (!pkAttacker->IsDead() && iAttackerDamageDelta != 0)
#if defined(SHOW_PLOT_POPUP)
				pkAttacker->ShowDamageDeltaText(iAttackerDamageDelta, pkAttacker->plot());
#else
				CvUnit::ShowDamageDeltaText(iAttackerDamageDelta, pkAttacker->plot());
#endif
		}

		// Report that combat is over in case we want to queue another attack
		GET_PLAYER(pkAttacker->getOwner()).GetTacticalAI()->CombatResolved(pkAttacker, bDefenderDead);
	}

	BATTLE_FINISHED();
	DoNewBattleEffects(kCombatInfo);
}

//	---------------------------------------------------------------------------
//	Function: GenerateRangedCombatInfo
//	Take the input parameters and fill in a CvCombatInfo definition assuming a
//	ranged combat.
//
//	Parameters:
//		pkDefender   	-	Defending unit.  Can be null, in which case the input plot must have a city
//		plot         	-	The plot of the defending unit/city
//		pkCombatInfo 	-	Output combat info
//	---------------------------------------------------------------------------
void CvUnitCombat::GenerateRangedCombatInfo(CvUnit& kAttacker, CvUnit* pkDefender, CvPlot& plot, CvCombatInfo* pkCombatInfo)
{
	BATTLE_STARTED(BATTLE_TYPE_RANGED, plot);
	pkCombatInfo->setBattleType(BATTLE_TYPE_RANGED);
	pkCombatInfo->setUnit(BATTLE_UNIT_ATTACKER, &kAttacker);
	pkCombatInfo->setUnit(BATTLE_UNIT_DEFENDER, pkDefender);
	pkCombatInfo->setPlot(&plot);

	//////////////////////////////////////////////////////////////////////

	bool bBarbarian = false;
	int iExperience = 0;
	int iMaxXP = 0;
	int iDamage = 0;
	int iTotalDamage = 0;
	PlayerTypes eDefenderOwner;
	if(!plot.isCity())
	{
		CvAssert(pkDefender != NULL);

		eDefenderOwner = pkDefender->getOwner();

		iExperience = /*2*/ GC.getEXPERIENCE_ATTACKING_UNIT_RANGED();
#ifdef MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE
		if (MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE)
		{
			iExperience += kAttacker.ExtraAttackXPValue();
		}
#endif		
		if(pkDefender->isBarbarian())
			bBarbarian = true;
		iMaxXP = pkDefender->maxXPValue();

		//CvAssert(pkDefender->IsCanDefend());

		iDamage = kAttacker.GetRangeCombatDamage(pkDefender, /*pCity*/ NULL, /*bIncludeRand*/ true);

#if defined(MOD_ROG_CORE)
		InflictDamageContext ctx;
		ctx.pDefenderUnit = pkDefender;
		ctx.pAttackerUnit = &kAttacker;
		ctx.piAttackInflictDamage = &iDamage;
		ctx.bRanged = true;
		InterveneInflictDamage(&ctx);
#endif

#if defined(MOD_UNITS_MAX_HP)
		if(iDamage + pkDefender->getDamage() > kAttacker.GetMaxHitPoints())
		{
			//iDamage = kAttacker.GetMaxHitPoints() - pkDefender->getDamage();
		}
#else
		if(iDamage + pkDefender->getDamage() > GC.getMAX_HIT_POINTS())
		{
			iDamage = GC.getMAX_HIT_POINTS() - pkDefender->getDamage();
		}
#endif

		iTotalDamage = std::max(pkDefender->getDamage(), pkDefender->getDamage() + iDamage);
	}
	else // plot.isCity()
	{
		if (kAttacker.isRangedSupportFire()) return; // can't attack cities with this

		CvCity* pCity = plot.getPlotCity();
		CvAssert(pCity != NULL);
		if(!pCity) return;
		BATTLE_JOINED(pCity, BATTLE_UNIT_DEFENDER, true);

		eDefenderOwner = plot.getOwner();
		/*		iDefenderStrength = pCity->getStrengthValue() / 2;
		iOldDamage = pCity->getDamage();*/

		iExperience = /*3*/ GC.getEXPERIENCE_ATTACKING_CITY_RANGED();
#ifdef MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE
		if (MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE)
		{
			iExperience += kAttacker.ExtraAttackXPValue();
		}
#endif
		if(pCity->isBarbarian())
			bBarbarian = true;
		iMaxXP = 1000;

		iDamage = kAttacker.GetRangeCombatDamage(/*pDefender*/ NULL, pCity, /*bIncludeRand*/ true);

#if defined(MOD_ROG_CORE)
		InflictDamageContext ctx;
		ctx.pDefenderCity = pCity;
		ctx.pAttackerUnit = &kAttacker;
		ctx.piAttackInflictDamage = &iDamage;
		ctx.bRanged = true;
		InterveneInflictDamage(&ctx);
#endif

		// Cities can't be knocked to less than 1 HP
		if(iDamage + pCity->getDamage() >= pCity->GetMaxHitPoints())
		{
			iDamage = pCity->GetMaxHitPoints() - pCity->getDamage() - 1;
		}

		iTotalDamage = std::max(pCity->getDamage(), pCity->getDamage() + iDamage);
	}
	//////////////////////////////////////////////////////////////////////

	pkCombatInfo->setFinalDamage(BATTLE_UNIT_ATTACKER, 0);				// Total damage to the unit
	pkCombatInfo->setDamageInflicted(BATTLE_UNIT_ATTACKER, iDamage);		// Damage inflicted this round
	pkCombatInfo->setFinalDamage(BATTLE_UNIT_DEFENDER, iTotalDamage);		// Total damage to the unit
	pkCombatInfo->setDamageInflicted(BATTLE_UNIT_DEFENDER, 0);			// Damage inflicted this round

	// Fear Damage
	pkCombatInfo->setFearDamageInflicted(BATTLE_UNIT_ATTACKER, 0);
	// pkCombatInfo->setFearDamageInflicted( BATTLE_UNIT_DEFENDER, 0 );

	pkCombatInfo->setExperience(BATTLE_UNIT_ATTACKER, iExperience);
	pkCombatInfo->setMaxExperienceAllowed(BATTLE_UNIT_ATTACKER, iMaxXP);
	pkCombatInfo->setInBorders(BATTLE_UNIT_ATTACKER, plot.getOwner() == eDefenderOwner);
#if defined(MOD_BUGFIX_BARB_GP_XP)
	bool bGeneralsXP = !kAttacker.isBarbarian();
	if (MOD_BUGFIX_BARB_GP_XP) {
		if (!plot.isCity()) {
			bGeneralsXP = !pkDefender->isBarbarian();
		} else {
			bGeneralsXP = !plot.getPlotCity()->isBarbarian();
		}
	}
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS) || defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, kAttacker.isGGFromBarbarians() || bGeneralsXP);
#else
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, bGeneralsXP);
#endif
#else
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS) || defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, kAttacker.isGGFromBarbarians() || !kAttacker.isBarbarian());
#else
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, !kAttacker.isBarbarian());
#endif
#endif

	iExperience = /*2*/ GC.getEXPERIENCE_DEFENDING_UNIT_RANGED();
#ifdef MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE
	if (MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE && pkDefender)
	{
		iExperience += pkDefender->ExtraDefenseXPValue();
	}
#endif
	pkCombatInfo->setExperience(BATTLE_UNIT_DEFENDER, iExperience);
	pkCombatInfo->setMaxExperienceAllowed(BATTLE_UNIT_DEFENDER, kAttacker.maxXPValue());
	pkCombatInfo->setInBorders(BATTLE_UNIT_DEFENDER, plot.getOwner() == kAttacker.getOwner());
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS) || defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_DEFENDER, (!plot.isCity() && pkDefender->isGGFromBarbarians()) || (!bBarbarian && !kAttacker.isBarbarian()));
#else
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_DEFENDER, !bBarbarian && !kAttacker.isBarbarian());
#endif

	pkCombatInfo->setAttackIsRanged(true);
	// Defender doesn't retaliate.  We'll keep this separate from the ranged attack flag in case something changes to allow
	// some units to retaliate on a ranged attack (Archer vs. Archers maybe?)
	pkCombatInfo->setDefenderRetaliates(false);

	GC.GetEngineUserInterface()->setDirty(UnitInfo_DIRTY_BIT, true);
}

//	---------------------------------------------------------------------------
//	Function: GenerateRangedCombatInfo
//	Take the input parameters and fill in a CvCombatInfo definition assuming a
//	ranged combat from a city.
//
//	Parameters:
//		kAttacker		-	Attacking city.
//		pkDefender   	-	Defending unit.
//		plot         	-	The plot of the defending unit
//		pkCombatInfo 	-	Output combat info
//	---------------------------------------------------------------------------
void CvUnitCombat::GenerateRangedCombatInfo(CvCity& kAttacker, CvUnit* pkDefender, CvPlot& plot, CvCombatInfo* pkCombatInfo)
{
	BATTLE_STARTED(BATTLE_TYPE_RANGED, plot);
	pkCombatInfo->setBattleType(BATTLE_TYPE_RANGED);
	pkCombatInfo->setCity(BATTLE_UNIT_ATTACKER, &kAttacker);
	pkCombatInfo->setUnit(BATTLE_UNIT_DEFENDER, pkDefender);
	pkCombatInfo->setPlot(&plot);

	//////////////////////////////////////////////////////////////////////

	bool bBarbarian = false;
	int iDamage = 0;
	int iTotalDamage = 0;
	PlayerTypes eDefenderOwner = NO_PLAYER;
	if(!plot.isCity())
	{
		CvAssert(pkDefender != NULL);

		eDefenderOwner = pkDefender->getOwner();

		if(pkDefender->isBarbarian())
			bBarbarian = true;

		//CvAssert(pkDefender->IsCanDefend());

		iDamage = kAttacker.rangeCombatDamage(pkDefender);


#if defined(MOD_ROG_CORE)
		InflictDamageContext ctx;
		ctx.pDefenderUnit = pkDefender;
		ctx.pAttackerCity = &kAttacker;
		ctx.piAttackInflictDamage = &iDamage;
		ctx.bRanged = true;
		InterveneInflictDamage(&ctx);
#endif

#if defined(MOD_UNITS_MAX_HP)
		if(iDamage + pkDefender->getDamage() > pkDefender->GetMaxHitPoints())
		{
			iDamage = pkDefender->GetMaxHitPoints() - pkDefender->getDamage();
		}
#else
		if(iDamage + pkDefender->getDamage() > GC.getMAX_HIT_POINTS())
		{
			iDamage = GC.getMAX_HIT_POINTS() - pkDefender->getDamage();
		}
#endif

		iTotalDamage = std::max(pkDefender->getDamage(), pkDefender->getDamage() + iDamage);
	}
	else
	{
		FAssertMsg(false, "City vs. City not supported.");	// Don't even think about it Jon....
	}


	if (MOD_EVENTS_CITY_RANGE_STRIKE)
	{
		GAMEEVENTINVOKE_HOOK(GAMEEVENT_CityRangedStrike, kAttacker.getOwner(), kAttacker.GetID(), pkDefender->getOwner(), pkDefender->GetID(), plot.getX(), plot.getY());
	}
	else
	{
		ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
		if (pkScriptSystem)
		{
			CvLuaArgsHandle args;

			args->Push(kAttacker.getOwner());
			args->Push(kAttacker.GetID());
			args->Push(pkDefender->getOwner());
			args->Push(pkDefender->GetID());
			args->Push(plot.getX());
			args->Push(plot.getY());

			bool bResult = false;
			LuaSupport::CallHook(pkScriptSystem, "CityRangedStrike", args.get(), bResult);
		}
	}

	//////////////////////////////////////////////////////////////////////

	pkCombatInfo->setFinalDamage(BATTLE_UNIT_ATTACKER, 0);				// Total damage to the unit
	pkCombatInfo->setDamageInflicted(BATTLE_UNIT_ATTACKER, iDamage);		// Damage inflicted this round
	pkCombatInfo->setFinalDamage(BATTLE_UNIT_DEFENDER, iTotalDamage);		// Total damage to the unit
	pkCombatInfo->setDamageInflicted(BATTLE_UNIT_DEFENDER, 0);			// Damage inflicted this round

	// Fear Damage
	pkCombatInfo->setFearDamageInflicted(BATTLE_UNIT_ATTACKER, 0);
	// pkCombatInfo->setFearDamageInflicted( BATTLE_UNIT_DEFENDER, 0 );

	pkCombatInfo->setExperience(BATTLE_UNIT_ATTACKER, 0);
	pkCombatInfo->setMaxExperienceAllowed(BATTLE_UNIT_ATTACKER, 0);
	pkCombatInfo->setInBorders(BATTLE_UNIT_ATTACKER, plot.getOwner() == eDefenderOwner);
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS) || defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, !kAttacker.isBarbarian()); // Kind of irrelevant, as cities don't get XP!
#else
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, !kAttacker.isBarbarian());
#endif

	int iExperience = /*2*/ GC.getEXPERIENCE_DEFENDING_UNIT_RANGED();
#ifdef MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE
	if (MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE  && pkDefender)
	{
		iExperience += pkDefender->ExtraDefenseXPValue();
	}
#endif
	pkCombatInfo->setExperience(BATTLE_UNIT_DEFENDER, iExperience);
	pkCombatInfo->setMaxExperienceAllowed(BATTLE_UNIT_DEFENDER, MAX_INT);
	pkCombatInfo->setInBorders(BATTLE_UNIT_DEFENDER, plot.getOwner() == kAttacker.getOwner());
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS) || defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_DEFENDER, pkDefender->isGGFromBarbarians() || !bBarbarian && !kAttacker.isBarbarian());
#else
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_DEFENDER, !bBarbarian && !kAttacker.isBarbarian());
#endif

	pkCombatInfo->setAttackIsRanged(true);
	// Defender doesn't retaliate.  We'll keep this separate from the ranged attack flag in case something changes to allow
	// some units to retaliate on a ranged attack (Archer vs. Archers maybe?)
	pkCombatInfo->setDefenderRetaliates(false);

	GC.GetEngineUserInterface()->setDirty(UnitInfo_DIRTY_BIT, true);
}

//	---------------------------------------------------------------------------
//	Function: ResolveRangedUnitVsCombat
//	Resolve ranged combat where the attacker is a unit.  This will handle
//  unit vs. unit and unit vs. city
//	---------------------------------------------------------------------------
void CvUnitCombat::ResolveRangedUnitVsCombat(const CvCombatInfo& kCombatInfo, uint uiParentEventID)
{
	bool bTargetDied = false;
	int iDamage = kCombatInfo.getDamageInflicted(BATTLE_UNIT_ATTACKER);
//	int iExperience = kCombatInfo.getExperience(BATTLE_UNIT_ATTACKER);
//	int iMaxXP = kCombatInfo.getMaxExperienceAllowed(BATTLE_UNIT_ATTACKER);
	bool bBarbarian = false;

	CvUnit* pkAttacker = kCombatInfo.getUnit(BATTLE_UNIT_ATTACKER);
	CvAssert_Debug(pkAttacker);

	CvPlot* pkTargetPlot = kCombatInfo.getPlot();
	CvAssert_Debug(pkTargetPlot);

	ICvUserInterface2* pkDLLInterface = GC.GetEngineUserInterface();
	CvString strBuffer;

	CvUnit* pkDefender = nullptr;
	CvCity* pCity = nullptr;
	if(pkTargetPlot)
	{
		if(!pkTargetPlot->isCity())
		{
			// Unit
			pkDefender = kCombatInfo.getUnit(BATTLE_UNIT_DEFENDER);
			CvAssert_Debug(pkDefender != NULL);
			if(pkDefender)
			{
				bBarbarian = pkDefender->isBarbarian();

				if(pkAttacker)
				{
					// Defender died
#if defined(MOD_UNITS_MAX_HP)
					if(iDamage + pkDefender->getDamage() >= pkDefender->GetMaxHitPoints())
#else
					if(iDamage + pkDefender->getDamage() >= GC.getMAX_HIT_POINTS())
#endif
					{
						if(pkAttacker->getOwner() == GC.getGame().getActivePlayer())
						{
							strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_ATTACK_BY_AIR_AND_DEATH", pkAttacker->getNameKey(), pkDefender->getNameKey());
							pkDLLInterface->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
							MILITARYLOG(pkAttacker->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkDefender->getOwner());
						}

						strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_ARE_ATTACKED_BY_AIR_AND_DEATH", pkDefender->getNameKey(), pkAttacker->getNameKey());
						CvNotifications* pNotifications = GET_PLAYER(pkDefender->getOwner()).GetNotifications();
						if(pNotifications)
						{
							Localization::String strSummary = Localization::Lookup("TXT_KEY_UNIT_LOST");
							pNotifications->Add(NOTIFICATION_UNIT_DIED, strBuffer, strSummary.toUTF8(), pkDefender->getX(), pkDefender->getY(), (int) pkDefender->getUnitType(), pkDefender->getOwner());
						}

						bTargetDied = true;


#if defined(MOD_ROG_CORE)
						if (bTargetDied)
						{
							pkDefender->DoAdjacentPlotDamage(pkTargetPlot, pkDefender->getAOEDamageOnKill());
						}
#endif

#if !defined(NO_ACHIEVEMENTS)
						CvPlayerAI& kAttackerOwner = GET_PLAYER(pkAttacker->getOwner());
						kAttackerOwner.GetPlayerAchievements().KilledUnitWithUnit(pkAttacker, pkDefender);
#endif

						ApplyPostCombatTraitEffects(pkAttacker, pkDefender);

						if(bBarbarian)
						{
							pkDefender->DoTestBarbarianThreatToMinorsWithThisUnitsDeath(pkAttacker->getOwner());
						}

#if !defined(NO_ACHIEVEMENTS)
						//One Hit
#if defined(MOD_UNITS_MAX_HP)
						if(pkDefender->GetCurrHitPoints() == pkDefender->GetMaxHitPoints() && pkAttacker->isHuman() && !GC.getGame().isGameMultiPlayer())
#else
						if(pkDefender->GetCurrHitPoints() == GC.getMAX_HIT_POINTS() && pkAttacker->isHuman() && !GC.getGame().isGameMultiPlayer())
#endif
						{
							gDLL->UnlockAchievement(ACHIEVEMENT_ONEHITKILL);
						}
#endif
					}
					// Nobody died
					else
					{
						if(pkAttacker->getOwner() == GC.getGame().getActivePlayer())
						{
							strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_ATTACK_BY_AIR", pkAttacker->getNameKey(), pkDefender->getNameKey(), iDamage);
							pkDLLInterface->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
							MILITARYLOG(pkAttacker->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkDefender->getOwner());
						}
						strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_ARE_ATTACKED_BY_AIR", pkDefender->getNameKey(), pkAttacker->getNameKey(), iDamage);
					}

					//red icon over attacking unit
					if(pkDefender->getOwner() == GC.getGame().getActivePlayer())
					{
						pkDLLInterface->AddMessage(uiParentEventID, pkDefender->getOwner(), false, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkAttacker->m_pUnitInfo->GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkAttacker->getX(), pkAttacker->getY(), true, true*/);
						MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkAttacker->getOwner());
					}
					//white icon over defending unit
					//pkDLLInterface->AddMessage(uiParentEventID, pkDefender->getOwner(), false, 0, ""/*, "AS2D_COMBAT", MESSAGE_TYPE_DISPLAY_ONLY, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"), pkDefender->getX(), pkDefender->getY(), true, true*/);

#if defined(MOD_PROMOTION_GET_INSTANCE_FROM_ATTACK)
					DoInstantYieldFromCombat(kCombatInfo);
#endif	
					//set damage but don't update entity damage visibility
#if defined(MOD_API_UNIT_STATS)
					pkDefender->changeDamage(iDamage, pkAttacker->getOwner(), pkAttacker->GetID());
#else
					pkDefender->changeDamage(iDamage, pkAttacker->getOwner());
#endif

					// Update experience
#if defined(MOD_UNITS_XP_TIMES_100)
					pkDefender->changeExperienceTimes100(100 * 
#else
					pkDefender->changeExperience(
#endif
					    kCombatInfo.getExperience(BATTLE_UNIT_DEFENDER),
					    kCombatInfo.getMaxExperienceAllowed(BATTLE_UNIT_DEFENDER),
					    true,
					    kCombatInfo.getInBorders(BATTLE_UNIT_DEFENDER),
					    kCombatInfo.getUpdateGlobal(BATTLE_UNIT_DEFENDER));
				}

				pkDefender->setCombatUnit(NULL);
				if(!CvUnitMission::IsHeadMission(pkDefender, CvTypes::getMISSION_WAIT_FOR()))		// If the top mission was not a 'wait for', then clear it.
					pkDefender->ClearMissionQueue();
			}
			else
				bTargetDied = true;
		}
		else
		{
			// City
			pCity = pkTargetPlot->getPlotCity();
			CvAssert_Debug(pCity != NULL);
			if(pCity)
			{
				if(pkAttacker)
				{
					bBarbarian = pCity->isBarbarian();
					pCity->changeDamage(iDamage);

					if(pCity->getOwner() == GC.getGame().getActivePlayer())
					{
						strBuffer = GetLocalizedText("TXT_KEY_MISC_YOUR_CITY_ATTACKED_BY_AIR", pCity->getNameKey(), pkAttacker->getNameKey(), iDamage);
						//red icon over attacking unit
						pkDLLInterface->AddMessage(uiParentEventID, pCity->getOwner(), false, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkAttacker->m_pUnitInfo->GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkAttacker->getX(), pkAttacker->getY(), true, true*/);
						MILITARYLOG(pCity->getOwner(), strBuffer.c_str(), pCity->plot(), pkAttacker->getOwner());
					}
				}

				pCity->clearCombat();
			}
			else
				bTargetDied = true;
		}
	}
	else
		bTargetDied = true;

	if(pkAttacker)
	{
		// Unit gains XP for executing a Range Strike
		if(iDamage > 0) // && iDefenderStrength > 0)
		{
#if defined(MOD_UNITS_XP_TIMES_100)
			pkAttacker->changeExperienceTimes100(100 * 
#else
			pkAttacker->changeExperience(
#endif
			    kCombatInfo.getExperience(BATTLE_UNIT_ATTACKER),
			    kCombatInfo.getMaxExperienceAllowed(BATTLE_UNIT_ATTACKER),
			    true,
			    kCombatInfo.getInBorders(BATTLE_UNIT_ATTACKER),
			    kCombatInfo.getUpdateGlobal(BATTLE_UNIT_ATTACKER));
		}

		pkAttacker->testPromotionReady();

		pkAttacker->setCombatUnit(NULL);
		pkAttacker->ClearMissionQueue(GetPostCombatDelay());

		// Report that combat is over in case we want to queue another attack
		GET_PLAYER(pkAttacker->getOwner()).GetTacticalAI()->CombatResolved(pkAttacker, bTargetDied);
	}
	
	BATTLE_FINISHED();
	DoNewBattleEffects(kCombatInfo);
}

//	---------------------------------------------------------------------------
//	Function: ResolveRangedCityVsUnitCombat
//	Resolve ranged combat where the attacker is a city
//	---------------------------------------------------------------------------
void CvUnitCombat::ResolveRangedCityVsUnitCombat(const CvCombatInfo& kCombatInfo, uint uiParentEventID)
{
	bool bTargetDied = false;
	int iDamage = kCombatInfo.getDamageInflicted(BATTLE_UNIT_ATTACKER);
	bool bBarbarian = false;

	CvCity* pkAttacker = kCombatInfo.getCity(BATTLE_UNIT_ATTACKER);
	CvAssert_Debug(pkAttacker);

	if(pkAttacker)
		pkAttacker->clearCombat();

	CvPlot* pkTargetPlot = kCombatInfo.getPlot();
	CvAssert_Debug(pkTargetPlot);

	ICvUserInterface2* pkDLLInterface = GC.GetEngineUserInterface();
	int iActivePlayerID = GC.getGame().getActivePlayer();

	if(pkTargetPlot)
	{
		if(!pkTargetPlot->isCity())
		{
			CvUnit* pkDefender = kCombatInfo.getUnit(BATTLE_UNIT_DEFENDER);
			CvAssert_Debug(pkDefender != NULL);
			if(pkDefender)
			{
				bBarbarian = pkDefender->isBarbarian();

				if(pkAttacker)
				{
					// Info message for the attacking player
					if(iActivePlayerID == pkAttacker->getOwner())
					{
						Localization::String localizedText = Localization::Lookup("TXT_KEY_MISC_YOUR_CITY_RANGE_ATTACK");
						localizedText << pkAttacker->getNameKey() << pkDefender->getNameKey() << iDamage;
						pkDLLInterface->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), localizedText.toUTF8());//, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pPlot->getX(), pPlot->getY());
						MILITARYLOG(pkAttacker->getOwner(), localizedText.toUTF8(), pkAttacker->plot(), pkDefender->getOwner());
					}

					// Red icon over defending unit
					if(iActivePlayerID == pkDefender->getOwner())
					{
						Localization::String localizedText = Localization::Lookup("TXT_KEY_MISC_YOU_ARE_ATTACKED_BY_CITY");
						localizedText << pkDefender->getNameKey() << pkAttacker->getNameKey() << iDamage;
						pkDLLInterface->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), localizedText.toUTF8());//, "AS2D_COMBAT", MESSAGE_TYPE_COMBAT_MESSAGE, pDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pDefender->getX(), pDefender->getY(), true, true);
						MILITARYLOG(pkDefender->getOwner(), localizedText.toUTF8(), pkDefender->plot(), pkAttacker->getOwner());
					}

#if defined(MOD_UNITS_MAX_HP)
					if(iDamage + pkDefender->getDamage() >= pkDefender->GetMaxHitPoints())
#else
					if(iDamage + pkDefender->getDamage() >= GC.getMAX_HIT_POINTS())
#endif
					{
						CvNotifications* pNotifications = GET_PLAYER(pkDefender->getOwner()).GetNotifications();
						if(pNotifications)
						{
							Localization::String localizedText = Localization::Lookup("TXT_KEY_MISC_YOU_ARE_ATTACKED_BY_CITY");
							localizedText << pkDefender->getNameKey() << pkAttacker->getNameKey() << iDamage;
							Localization::String strSummary = Localization::Lookup("TXT_KEY_UNIT_LOST");
							pNotifications->Add(NOTIFICATION_UNIT_DIED, localizedText.toUTF8(), strSummary.toUTF8(), pkDefender->getX(), pkDefender->getY(), (int) pkDefender->getUnitType(), pkDefender->getOwner());
						}
						bTargetDied = true;

						// Earn bonuses for kills?
						CvPlayer& kAttackingPlayer = GET_PLAYER(pkAttacker->getOwner());
#if defined(MOD_API_UNIFIED_YIELDS)
						kAttackingPlayer.DoYieldsFromKill(NULL, pkDefender, pkDefender->getX(), pkDefender->getY(), 0);
#else
						kAttackingPlayer.DoYieldsFromKill(NO_UNIT, pkDefender->getUnitType(), pkDefender->getX(), pkDefender->getY(), pkDefender->isBarbarian(), 0);
#endif
					}

#if defined(MOD_PROMOTION_GET_INSTANCE_FROM_ATTACK)
					DoInstantYieldFromCombat(kCombatInfo);
#endif	
					//set damage but don't update entity damage visibility
#if defined(MOD_API_UNIT_STATS)
					pkDefender->changeDamage(iDamage, pkAttacker->getOwner(), pkAttacker->GetID());
#else
					pkDefender->changeDamage(iDamage, pkAttacker->getOwner());
#endif

					// Update experience
#if defined(MOD_UNITS_XP_TIMES_100)
					pkDefender->changeExperienceTimes100(100 * 
#else
					pkDefender->changeExperience(
#endif
					    kCombatInfo.getExperience(BATTLE_UNIT_DEFENDER),
					    kCombatInfo.getMaxExperienceAllowed(BATTLE_UNIT_DEFENDER),
					    true,
					    kCombatInfo.getInBorders(BATTLE_UNIT_DEFENDER),
					    kCombatInfo.getUpdateGlobal(BATTLE_UNIT_DEFENDER));
				}

				pkDefender->setCombatUnit(NULL);
				if(!CvUnitMission::IsHeadMission(pkDefender, CvTypes::getMISSION_WAIT_FOR()))		// If the top mission was not a 'wait for', then clear it.
					pkDefender->ClearMissionQueue();
			}
			else
				bTargetDied = true;
		}
		else
		{
			CvAssert(false);	// Left as an exercise for the reader
			bTargetDied = true;
		}
	}

	// Report that combat is over in case we want to queue another attack
	if(pkAttacker)
		GET_PLAYER(pkAttacker->getOwner()).GetTacticalAI()->CombatResolved((void*)pkAttacker, bTargetDied, true);
	
	BATTLE_FINISHED();
}

//	---------------------------------------------------------------------------
//	Function: ResolveCityMeleeCombat
//
//	Resolves combat between a melee unit and a city.
//  The unit does not have to be a hand-to-hand combat type unit, just a unit doing
//  a non-ranged attack to an adjacent city.  The visualization of the attack will
//	usually appear as if it is ranged, simply because we don't want the unit members
//	running through a city and they wouldn't have anything to attack.
//	This is also the case where a city is able to attack back.
void CvUnitCombat::ResolveCityMeleeCombat(const CvCombatInfo& kCombatInfo, uint uiParentEventID)
{
	CvUnit* pkAttacker = kCombatInfo.getUnit(BATTLE_UNIT_ATTACKER);
	CvCity* pkDefender = kCombatInfo.getCity(BATTLE_UNIT_DEFENDER);

	CvAssert_Debug(pkAttacker && pkDefender);

	CvPlot* pkPlot = kCombatInfo.getPlot();
	if(!pkPlot && pkDefender)
		pkPlot = pkDefender->plot();

	CvAssert_Debug(pkPlot);

	int iAttackerDamageInflicted = kCombatInfo.getDamageInflicted(BATTLE_UNIT_ATTACKER);
	int iDefenderDamageInflicted = kCombatInfo.getDamageInflicted(BATTLE_UNIT_DEFENDER);

	if(pkAttacker && pkDefender)
	{
#if defined(MOD_PROMOTION_GET_INSTANCE_FROM_ATTACK)
		DoInstantYieldFromCombat(kCombatInfo);
#endif	
#if defined(MOD_API_UNIT_STATS)
		pkAttacker->changeDamage(iDefenderDamageInflicted, pkDefender->getOwner(), pkDefender->GetID());
#else
		pkAttacker->changeDamage(iDefenderDamageInflicted, pkDefender->getOwner());
#endif
		pkDefender->changeDamage(iAttackerDamageInflicted);

#if defined(MOD_UNITS_XP_TIMES_100)
		pkAttacker->changeExperienceTimes100(100 * kCombatInfo.getExperience(BATTLE_UNIT_ATTACKER),
#else
		pkAttacker->changeExperience(kCombatInfo.getExperience(BATTLE_UNIT_ATTACKER),
#endif
		                             kCombatInfo.getMaxExperienceAllowed(BATTLE_UNIT_ATTACKER),
		                             true,
		                             false,
		                             kCombatInfo.getUpdateGlobal(BATTLE_UNIT_ATTACKER));
	}

	bool bCityConquered = false;

	if(pkDefender)
		pkDefender->clearCombat();
	else
		bCityConquered = true;

	if(pkAttacker)
	{
		pkAttacker->setCombatUnit(NULL);
		pkAttacker->ClearMissionQueue(GetPostCombatDelay());

		if(pkAttacker->isSuicide())
		{
			pkAttacker->setCombatUnit(NULL);	// Must clear this if doing a delayed kill, should this be part of the kill method?
			pkAttacker->kill(true);
		}
	}

	CvString strBuffer;
	int iActivePlayerID = GC.getGame().getActivePlayer();

	// Barbarians don't capture Cities
	if(pkAttacker && pkDefender)
	{
		if(pkAttacker->isBarbarian() && (pkDefender->getDamage() >= pkDefender->GetMaxHitPoints()))
		{
			// 1 HP left
			pkDefender->setDamage(pkDefender->GetMaxHitPoints() - 1);

			int iNumGoldStolen = GC.getBARBARIAN_CITY_GOLD_RANSOM();	// 200

			if(iNumGoldStolen > GET_PLAYER(pkDefender->getOwner()).GetTreasury()->GetGold())
			{
				iNumGoldStolen = GET_PLAYER(pkDefender->getOwner()).GetTreasury()->GetGold();
			}

			// City is ransomed for Gold
			GET_PLAYER(pkDefender->getOwner()).GetTreasury()->ChangeGold(-iNumGoldStolen);

			if(pkDefender->getOwner() == iActivePlayerID)
			{
				strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_CITY_RANSOMED_BY_BARBARIANS", pkDefender->getNameKey(), iNumGoldStolen);
				GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*,GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitDefeatScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkPlot->getX(), pkPlot->getY()*/);
				MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkAttacker->getOwner());
			}

#if !defined(NO_ACHIEVEMENTS)
			if( pkDefender->GetPlayer()->GetID() == GC.getGame().getActivePlayer() && pkDefender->isHuman() && !GC.getGame().isGameMultiPlayer())
			{
				gDLL->UnlockAchievement(ACHIEVEMENT_REALLY_SUCK);
			}
#endif

			// Barb goes away after ransom
			pkAttacker->kill(true, NO_PLAYER);

			// Treat this as a conquest
			bCityConquered = true;
		}
		// Attacker died
		else if(pkAttacker->IsDead())
		{
			auto_ptr<ICvUnit1> pAttacker = GC.WrapUnitPointer(pkAttacker);
			gDLL->GameplayUnitDestroyedInCombat(pAttacker.get());
			if(pkAttacker->getOwner() == iActivePlayerID)
			{
				strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_DIED_ATTACKING_CITY", pkAttacker->getNameKey(), pkDefender->getNameKey(), iAttackerDamageInflicted);
				GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitDefeatScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkPlot->getX(), pkPlot->getY()*/);
				MILITARYLOG(pkAttacker->getOwner(), strBuffer.c_str(), pkAttacker->plot(), pkDefender->getOwner());
			}
			if(pkDefender->getOwner() == iActivePlayerID)
			{
				strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_KILLED_ENEMY_UNIT_CITY", pkDefender->getNameKey(), iAttackerDamageInflicted, pkAttacker->getNameKey(), pkAttacker->getVisualCivAdjective(pkDefender->getTeam()));
				GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitVictoryScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkPlot->getX(), pkPlot->getY()*/);
				MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkAttacker->getOwner());
			}
		}
		// City conquest
		else if(pkDefender->getDamage() >= pkDefender->GetMaxHitPoints())
		{
			if(!pkAttacker->isNoCapture())
			{
				if(pkAttacker->getOwner() == iActivePlayerID)
				{
					strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_CAPTURED_ENEMY_CITY", pkAttacker->getNameKey(), iDefenderDamageInflicted, pkDefender->getNameKey());
					GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitVictoryScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkPlot->getX(), pkPlot->getY()*/);
					MILITARYLOG(pkAttacker->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkDefender->getOwner());
				}
				if(pkDefender->getOwner() == iActivePlayerID)
				{
					strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_CITY_WAS_CAPTURED", pkDefender->getNameKey(), pkAttacker->getNameKey(), pkAttacker->getVisualCivAdjective(pkDefender->getTeam()), iDefenderDamageInflicted);
					GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*,GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitDefeatScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkPlot->getX(), pkPlot->getY()*/);
					MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkAttacker->getOwner());
				}

				pkAttacker->UnitMove(pkPlot, true, pkAttacker);

				bCityConquered = true;
			}
		}
		// Neither side lost
		else
		{
			if(pkAttacker->getOwner() == iActivePlayerID)
			{
				strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_WITHDRAW_CITY", pkAttacker->getNameKey(), iDefenderDamageInflicted, pkDefender->getNameKey(), iAttackerDamageInflicted);
				GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, "AS2D_OUR_WITHDRAWL", MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkPlot->getX(), pkPlot->getY()*/);
				MILITARYLOG(pkAttacker->getOwner(), strBuffer.c_str(), pkAttacker->plot(), pkDefender->getOwner());
			}
			if(pkDefender->getOwner() == iActivePlayerID)
			{
				strBuffer = GetLocalizedText("TXT_KEY_MISC_ENEMY_UNIT_WITHDRAW_CITY", pkAttacker->getNameKey(), iDefenderDamageInflicted, pkDefender->getNameKey(), iAttackerDamageInflicted);
				GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, "AS2D_THEIR_WITHDRAWL", MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkPlot->getX(), pkPlot->getY()*/);
				MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkAttacker->getOwner());
			}
			pkAttacker->changeMoves(-GC.getMOVE_DENOMINATOR());

			ApplyPostCityCombatEffects(pkAttacker, pkDefender, iAttackerDamageInflicted);
		}
	}

	if(pkAttacker)
	{
		pkAttacker->PublishQueuedVisualizationMoves();

		if(!pkAttacker->canMoveAfterAttacking())
		{
			pkAttacker->finishMoves();
			GC.GetEngineUserInterface()->changeCycleSelectionCounter(1);
		}

		// Report that combat is over in case we want to queue another attack
		GET_PLAYER(pkAttacker->getOwner()).GetTacticalAI()->CombatResolved(pkAttacker, bCityConquered);
	}
	
	BATTLE_FINISHED();
	DoNewBattleEffects(kCombatInfo);
}

//	GenerateAirCombatInfo
//	Function: GenerateRangedCombatInfo
//	Take the input parameters and fill in a CvCombatInfo definition assuming a
//	air bombing mission.
//
//	Parameters:
//		pkDefender   	-	Defending unit.  Can be null, in which case the input plot must have a city
//		plot         	-	The plot of the defending unit/city
//		pkCombatInfo 	-	Output combat info
//	---------------------------------------------------------------------------
void CvUnitCombat::GenerateAirCombatInfo(CvUnit& kAttacker, CvUnit* pkDefender, CvPlot& plot, CvCombatInfo* pkCombatInfo)
{
	BATTLE_STARTED(BATTLE_TYPE_AIR, plot);
	pkCombatInfo->setBattleType(BATTLE_TYPE_AIR);
	int iExperience = 0;

	pkCombatInfo->setUnit(BATTLE_UNIT_ATTACKER, &kAttacker);
	pkCombatInfo->setUnit(BATTLE_UNIT_DEFENDER, pkDefender);
	pkCombatInfo->setPlot(&plot);

	//////////////////////////////////////////////////////////////////////

	// Any interception to be done?
	CvUnit* pInterceptor = kAttacker.GetBestInterceptor(plot, pkDefender);
	int iInterceptionDamage = 0;

	if(pInterceptor != NULL && pInterceptor != pkDefender)
	{
		pkCombatInfo->setUnit(BATTLE_UNIT_INTERCEPTOR, pInterceptor);
		// Does the attacker evade?
		if(GC.getGame().getJonRandNum(100, "Evasion Rand") >= kAttacker.evasionProbability())
		{
			// Is the interception successful?
			if(GC.getGame().getJonRandNum(100, "Intercept Rand (Air)") < pInterceptor->currInterceptionProbability())
			{
				iInterceptionDamage = pInterceptor->GetInterceptionDamage(&kAttacker);
			}
		}
		pkCombatInfo->setDamageInflicted(BATTLE_UNIT_INTERCEPTOR, iInterceptionDamage);		// Damage inflicted this round
	}

	//////////////////////////////////////////////////////////////////////

	bool bBarbarian = false;
	int iMaxXP = 0;

	int iAttackerDamageInflicted;
	int iDefenderDamageInflicted;

	int iAttackerTotalDamageInflicted;
	int iDefenderTotalDamageInflicted;

	PlayerTypes eDefenderOwner;

	// Target is a Unit
	if(!plot.isCity())
	{
		CvAssert(pkDefender != NULL);
		if(!pkDefender)
			return;

		eDefenderOwner = pkDefender->getOwner();

		iExperience = /*4*/ GC.getEXPERIENCE_ATTACKING_UNIT_AIR();
#ifdef MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE
		if (MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE)
		{
			iExperience += kAttacker.ExtraAttackXPValue();
		}
#endif
		if(pkDefender->isBarbarian())
			bBarbarian = true;
		iMaxXP = pkDefender->maxXPValue();

		// Calculate attacker damage
		iAttackerDamageInflicted = kAttacker.GetAirCombatDamage(pkDefender, /*pCity*/ NULL, /*bIncludeRand*/ true, iInterceptionDamage);
		// Calculate defense damage
		iDefenderDamageInflicted = pkDefender->GetAirStrikeDefenseDamage(&kAttacker);

#if defined(MOD_ROG_CORE)
		InflictDamageContext ctx;
		ctx.pDefenderUnit = pkDefender;
		ctx.pAttackerUnit = &kAttacker;
		ctx.piAttackInflictDamage = &iAttackerDamageInflicted;
		ctx.piDefenseInflictDamage = &iDefenderDamageInflicted;
		ctx.bAirCombat = true;
		InterveneInflictDamage(&ctx);
#endif

#if defined(MOD_UNITS_MAX_HP)
		if(iAttackerDamageInflicted + pkDefender->getDamage() > pkDefender->GetMaxHitPoints())
		{
			iAttackerDamageInflicted = pkDefender->GetMaxHitPoints() - pkDefender->getDamage();
		}
#else
		if(iAttackerDamageInflicted + pkDefender->getDamage() > GC.getMAX_HIT_POINTS())
		{
			iAttackerDamageInflicted = GC.getMAX_HIT_POINTS() - pkDefender->getDamage();
		}
#endif

		iAttackerTotalDamageInflicted = std::max(pkDefender->getDamage(), pkDefender->getDamage() + iAttackerDamageInflicted);


#if defined(MOD_UNITS_MAX_HP)
		if(iDefenderDamageInflicted + kAttacker.getDamage() > kAttacker.GetMaxHitPoints())
		{
			iDefenderDamageInflicted = kAttacker.GetMaxHitPoints() - kAttacker.getDamage();
		}
#else
		if(iDefenderDamageInflicted + kAttacker.getDamage() > GC.getMAX_HIT_POINTS())
		{
			iDefenderDamageInflicted = GC.getMAX_HIT_POINTS() - kAttacker.getDamage();
		}
#endif

		iDefenderTotalDamageInflicted = std::max(kAttacker.getDamage(), kAttacker.getDamage() + (iDefenderDamageInflicted + iInterceptionDamage));
	}
	// Target is a City
	else
	{
		CvCity* pCity = plot.getPlotCity();
		CvAssert(pCity != NULL);
		if(!pCity) return;
		BATTLE_JOINED(pCity, BATTLE_UNIT_DEFENDER, true);
		
		CUSTOMLOG("Bombing %s by %s", pCity->getName().GetCString(), kAttacker.getName().GetCString());
		if(pInterceptor != NULL && pInterceptor != pkDefender) {
			CUSTOMLOG("  intercption being done by %s for %i damage", pInterceptor->getName().GetCString(), iInterceptionDamage);
		}

		eDefenderOwner = plot.getOwner();

		/*		iDefenderStrength = pCity->getStrengthValue() / 2;
		iOldDamage = pCity->getDamage();*/

		iExperience = /*4*/ GC.getEXPERIENCE_ATTACKING_CITY_AIR();
#ifdef MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE
		if (MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE)
		{
			iExperience += kAttacker.ExtraAttackXPValue();
		}
#endif
		if(pCity->isBarbarian())
			bBarbarian = true;
		iMaxXP = 1000;

		iAttackerDamageInflicted = kAttacker.GetAirCombatDamage(/*pUnit*/ NULL, pCity, /*bIncludeRand*/ true, iInterceptionDamage);
		// Calculate defense damage
		iDefenderDamageInflicted = pCity->GetAirStrikeDefenseDamage(&kAttacker);

#if defined(MOD_ROG_CORE)
		InflictDamageContext ctx;
		ctx.pDefenderCity = pCity;
		ctx.pAttackerUnit = &kAttacker;
		ctx.piAttackInflictDamage = &iAttackerDamageInflicted;
		ctx.piDefenseInflictDamage = &iDefenderDamageInflicted;
		ctx.bAirCombat = true;
		InterveneInflictDamage(&ctx);
#endif


		// Cities can't be knocked to less than 1 HP
		if(iAttackerDamageInflicted + pCity->getDamage() >= pCity->GetMaxHitPoints())
		{
			iAttackerDamageInflicted = pCity->GetMaxHitPoints() - pCity->getDamage() - 1;
		}

		iAttackerTotalDamageInflicted = std::max(pCity->getDamage(), pCity->getDamage() + iAttackerDamageInflicted);


		if(iDefenderDamageInflicted + kAttacker.getDamage() > pCity->GetMaxHitPoints())
		{
#if defined(MOD_BUGFIX_MINOR)
			// Surely!!!
			iDefenderDamageInflicted = pCity->GetMaxHitPoints() - kAttacker.getDamage();
#else
			iDefenderDamageInflicted = GC.getMAX_HIT_POINTS() - kAttacker.getDamage();
#endif
		}

		iDefenderTotalDamageInflicted = std::max(kAttacker.getDamage(), kAttacker.getDamage() + (iDefenderDamageInflicted + iInterceptionDamage));

#if !defined(NO_ACHIEVEMENTS)
		//Achievement for Washington
		CvUnitEntry* pkUnitInfo = GC.getUnitInfo(kAttacker.getUnitType());
		if(pkUnitInfo)
		{
			if(kAttacker.isHuman() && !GC.getGame().isGameMultiPlayer() && _stricmp(pkUnitInfo->GetType(), "UNIT_AMERICAN_B17") == 0)
			{
				gDLL->UnlockAchievement(ACHIEVEMENT_SPECIAL_B17);
			}
		}
#endif
	}
	//////////////////////////////////////////////////////////////////////

	pkCombatInfo->setFinalDamage(BATTLE_UNIT_ATTACKER, iDefenderTotalDamageInflicted);				// Total damage to the unit
	pkCombatInfo->setDamageInflicted(BATTLE_UNIT_ATTACKER, iAttackerDamageInflicted);		// Damage inflicted this round
	pkCombatInfo->setFinalDamage(BATTLE_UNIT_DEFENDER, iAttackerTotalDamageInflicted);		// Total damage to the unit
	pkCombatInfo->setDamageInflicted(BATTLE_UNIT_DEFENDER, iDefenderDamageInflicted);			// Damage inflicted this round

	// Fear Damage
	pkCombatInfo->setFearDamageInflicted(BATTLE_UNIT_ATTACKER, 0);
	// pkCombatInfo->setFearDamageInflicted( BATTLE_UNIT_DEFENDER, 0 );

	pkCombatInfo->setExperience(BATTLE_UNIT_ATTACKER, iExperience);
	pkCombatInfo->setMaxExperienceAllowed(BATTLE_UNIT_ATTACKER, iMaxXP);
	pkCombatInfo->setInBorders(BATTLE_UNIT_ATTACKER, plot.getOwner() == eDefenderOwner);
#if defined(MOD_BUGFIX_BARB_GP_XP)
	bool bGeneralsXP = !kAttacker.isBarbarian();
	if (MOD_BUGFIX_BARB_GP_XP) {
		if (!plot.isCity()) {
			bGeneralsXP = !pkDefender->isBarbarian();
		} else {
			bGeneralsXP = !plot.getPlotCity()->isBarbarian();
		}
	}
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS) || defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, kAttacker.isGGFromBarbarians() || bGeneralsXP);
#else
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, bGeneralsXP);
#endif
#else
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS) || defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, kAttacker.isGGFromBarbarians() || !kAttacker.isBarbarian());
#else
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, !kAttacker.isBarbarian());
#endif
#endif

	iExperience = /*2*/ GC.getEXPERIENCE_DEFENDING_UNIT_AIR();
#ifdef MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE
	if (MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE  && pkDefender)
	{
		iExperience += pkDefender->ExtraDefenseXPValue();
	}
#endif
	pkCombatInfo->setExperience(BATTLE_UNIT_DEFENDER, iExperience);
	pkCombatInfo->setMaxExperienceAllowed(BATTLE_UNIT_DEFENDER, MAX_INT);
	pkCombatInfo->setInBorders(BATTLE_UNIT_DEFENDER, plot.getOwner() == kAttacker.getOwner());
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS) || defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_DEFENDER, (!plot.isCity() && pkDefender->isGGFromBarbarians()) || !bBarbarian);
#else
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_DEFENDER, !bBarbarian);
#endif

	if (iInterceptionDamage > 0)
	{
		iExperience = /*2*/ GC.getEXPERIENCE_DEFENDING_AIR_SWEEP_GROUND();
#ifdef MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE
		if (MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE && pInterceptor)
		{
			iExperience += pInterceptor->ExtraDefenseXPValue();
		}
#endif
		pkCombatInfo->setExperience( BATTLE_UNIT_INTERCEPTOR, iExperience );
		pkCombatInfo->setMaxExperienceAllowed( BATTLE_UNIT_INTERCEPTOR, MAX_INT );
		pkCombatInfo->setInBorders( BATTLE_UNIT_INTERCEPTOR, plot.getOwner() == kAttacker.getOwner() );
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS) || defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
		pkCombatInfo->setUpdateGlobal( BATTLE_UNIT_INTERCEPTOR, pInterceptor->isGGFromBarbarians() || !bBarbarian );
#else
		pkCombatInfo->setUpdateGlobal( BATTLE_UNIT_INTERCEPTOR, !bBarbarian );
#endif
	}

	pkCombatInfo->setAttackIsBombingMission(true);
	pkCombatInfo->setDefenderRetaliates(true);

	GC.GetEngineUserInterface()->setDirty(UnitInfo_DIRTY_BIT, true);
}

//	ResolveAirUnitVsCombat
//	Function: ResolveRangedUnitVsCombat
//	Resolve air combat where the attacker is a unit.  This will handle
//  unit vs. unit and unit vs. city
//	---------------------------------------------------------------------------
void CvUnitCombat::ResolveAirUnitVsCombat(const CvCombatInfo& kCombatInfo, uint uiParentEventID)
{
	bool bTargetDied = false;
	int iAttackerDamageInflicted = kCombatInfo.getDamageInflicted(BATTLE_UNIT_ATTACKER);
	int iDefenderDamageInflicted = kCombatInfo.getDamageInflicted(BATTLE_UNIT_DEFENDER);

	CvUnit* pkAttacker = kCombatInfo.getUnit(BATTLE_UNIT_ATTACKER);
	CvUnit* pkDefender = nullptr;
	CvCity* pCity = nullptr;

	// If there's no valid attacker, then get out of here
	CvAssert_Debug(pkAttacker);

	// Interception?
	int iInterceptionDamage = kCombatInfo.getDamageInflicted(BATTLE_UNIT_INTERCEPTOR);
	if(iInterceptionDamage > 0)
		iDefenderDamageInflicted += iInterceptionDamage;

	CvUnit* pInterceptor = kCombatInfo.getUnit(BATTLE_UNIT_INTERCEPTOR);
	CvAssert_Debug(pInterceptor);
	if(pInterceptor)
	{
		pInterceptor->setMadeInterception(true);
		pInterceptor->setCombatUnit(NULL);
#if defined(MOD_UNITS_XP_TIMES_100)
		pInterceptor->changeExperienceTimes100(100 * 
#else
		pInterceptor->changeExperience(
#endif
			kCombatInfo.getExperience(BATTLE_UNIT_INTERCEPTOR),
			kCombatInfo.getMaxExperienceAllowed(BATTLE_UNIT_INTERCEPTOR),
			true,
			kCombatInfo.getInBorders(BATTLE_UNIT_INTERCEPTOR),
			kCombatInfo.getUpdateGlobal(BATTLE_UNIT_INTERCEPTOR));
	}

	CvPlot* pkTargetPlot = kCombatInfo.getPlot();
	CvAssert_Debug(pkTargetPlot);

	ICvUserInterface2* pkDLLInterface = GC.GetEngineUserInterface();
	int iActivePlayerID = GC.getGame().getActivePlayer();
	CvString strBuffer;

	if(pkTargetPlot)
	{
		if(!pkTargetPlot->isCity())
		{
			// Target was a Unit
			pkDefender = kCombatInfo.getUnit(BATTLE_UNIT_DEFENDER);
			CvAssert_Debug(pkDefender != NULL);

			if(pkDefender)
			{
				if(pkAttacker)
				{
#if !defined(NO_ACHIEVEMENTS)
					//One Hit
#if defined(MOD_UNITS_MAX_HP)
					if(pkDefender->GetCurrHitPoints() == pkDefender->GetMaxHitPoints() && pkAttacker->isHuman() && !GC.getGame().isGameMultiPlayer())
#else
					if(pkDefender->GetCurrHitPoints() == GC.getMAX_HIT_POINTS() && pkAttacker->isHuman() && !GC.getGame().isGameMultiPlayer())
#endif
					{
						gDLL->UnlockAchievement(ACHIEVEMENT_ONEHITKILL);
					}
#endif

#if defined(MOD_PROMOTION_GET_INSTANCE_FROM_ATTACK)
					DoInstantYieldFromCombat(kCombatInfo);
#endif	
#if defined(MOD_API_UNIT_STATS)
					pkAttacker->changeDamage(iDefenderDamageInflicted, pkDefender->getOwner(), pkDefender->GetID());
					pkDefender->changeDamage(iAttackerDamageInflicted, pkAttacker->getOwner(), pkAttacker->GetID());
#else
					pkAttacker->changeDamage(iDefenderDamageInflicted, pkDefender->getOwner());
					pkDefender->changeDamage(iAttackerDamageInflicted, pkAttacker->getOwner());
#endif

					// Update experience
#if defined(MOD_UNITS_XP_TIMES_100)
					pkDefender->changeExperienceTimes100(100 * 
#else
					pkDefender->changeExperience(
#endif
					    kCombatInfo.getExperience(BATTLE_UNIT_DEFENDER),
					    kCombatInfo.getMaxExperienceAllowed(BATTLE_UNIT_DEFENDER),
					    true,
					    kCombatInfo.getInBorders(BATTLE_UNIT_DEFENDER),
					    kCombatInfo.getUpdateGlobal(BATTLE_UNIT_DEFENDER));

					// Attacker died
					if(pkAttacker->IsDead())
					{
						auto_ptr<ICvUnit1> pAttacker = GC.WrapUnitPointer(pkAttacker);
						gDLL->GameplayUnitDestroyedInCombat(pAttacker.get());

#if !defined(NO_ACHIEVEMENTS)
						CvPlayerAI& kDefenderOwner = GET_PLAYER(pkDefender->getOwner());
						kDefenderOwner.GetPlayerAchievements().KilledUnitWithUnit(pkDefender, pkAttacker);
#endif

						if(iActivePlayerID == pkAttacker->getOwner())
						{
							strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_DIED_ATTACKING", pkAttacker->getNameKey(), pkDefender->getNameKey(), iAttackerDamageInflicted);
							pkDLLInterface->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitDefeatScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
							MILITARYLOG(pkAttacker->getOwner(), strBuffer.c_str(), pkAttacker->plot(), pkDefender->getOwner());
						}
						if(iActivePlayerID == pkDefender->getOwner())
						{
							if (iInterceptionDamage > 0 && pInterceptor)
							{
								strBuffer = GetLocalizedText("TXT_KEY_MISC_ENEMY_AIR_UNIT_DESTROYED", pInterceptor->getNameKey(), pkAttacker->getVisualCivAdjective(pkDefender->getTeam()), pkAttacker->getNameKey(), pkDefender->getNameKey());
								pkDLLInterface->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitVictoryScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
								MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkAttacker->getOwner());
							}
							else
							{
								strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_KILLED_ENEMY_UNIT", pkDefender->getNameKey(), iAttackerDamageInflicted, 0, pkAttacker->getNameKey(), pkAttacker->getVisualCivAdjective(pkDefender->getTeam()));
								pkDLLInterface->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitVictoryScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
								MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkAttacker->getOwner());
							}
						}

						ApplyPostCombatTraitEffects(pkDefender, pkAttacker);
					}
					// Defender died
					else if(pkDefender->IsDead())
					{
#if !defined(NO_ACHIEVEMENTS)
						CvPlayerAI& kAttackerOwner = GET_PLAYER(pkAttacker->getOwner());
						kAttackerOwner.GetPlayerAchievements().KilledUnitWithUnit(pkAttacker, pkDefender);
#endif

						if(iActivePlayerID == pkAttacker->getOwner())
						{
							strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_ATTACK_BY_AIR_AND_DEATH", pkAttacker->getNameKey(), pkDefender->getNameKey());
							pkDLLInterface->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
							MILITARYLOG(pkAttacker->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkDefender->getOwner());
						}
						if(iActivePlayerID == pkDefender->getOwner())
						{
							strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_ARE_ATTACKED_BY_AIR_AND_DEATH", pkDefender->getNameKey(), pkAttacker->getNameKey());
							pkDLLInterface->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
							MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkAttacker->getOwner());
						}

						CvNotifications* pNotifications = GET_PLAYER(pkDefender->getOwner()).GetNotifications();
						if(pNotifications)
						{
							Localization::String strSummary = Localization::Lookup("TXT_KEY_UNIT_LOST");
							pNotifications->Add(NOTIFICATION_UNIT_DIED, strBuffer, strSummary.toUTF8(), pkDefender->getX(), pkDefender->getY(), (int) pkDefender->getUnitType(), pkDefender->getOwner());
						}

#if defined(MOD_ROG_CORE)
						// If a Unit is adjacent to KILL
						pkDefender->DoAdjacentPlotDamage(pkTargetPlot, pkDefender->getAOEDamageOnKill());
#endif

						bTargetDied = true;

						ApplyPostCombatTraitEffects(pkAttacker, pkDefender);

#if defined(MOD_BUGFIX_MINOR)
						// Friendship from barb death via air-strike
						if(pkDefender->isBarbarian())
						{
						 	pkDefender->DoTestBarbarianThreatToMinorsWithThisUnitsDeath(pkAttacker->getOwner());
						}
#endif
					}
					// Nobody died
					else
					{
						if(iActivePlayerID == pkAttacker->getOwner())
						{
							strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_ATTACK_BY_AIR", pkAttacker->getNameKey(), pkDefender->getNameKey(), iDefenderDamageInflicted);
							pkDLLInterface->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
							MILITARYLOG(pkAttacker->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkDefender->getOwner());
						}
						if(iActivePlayerID == pkDefender->getOwner())
						{
							if (iInterceptionDamage > 0 && pInterceptor)
							{
								strBuffer = GetLocalizedText("TXT_KEY_MISC_ENEMY_AIR_UNIT_INTERCEPTED", pInterceptor->getNameKey(), pkAttacker->getVisualCivAdjective(pkDefender->getTeam()), pkAttacker->getNameKey(), iDefenderDamageInflicted, pkDefender->getNameKey());
								pkDLLInterface->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitVictoryScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
								MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pInterceptor->plot(), pkAttacker->getOwner());
							}
							else
							{
								strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_ARE_ATTACKED_BY_AIR", pkDefender->getNameKey(), pkAttacker->getNameKey(), iAttackerDamageInflicted);
								pkDLLInterface->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
								MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkAttacker->getOwner());
							}
						}
					}
				}

				//set damage but don't update entity damage visibility
				//pkDefender->changeDamage(iDamage, pkAttacker->getOwner());

				pkDefender->setCombatUnit(NULL);
				if(!CvUnitMission::IsHeadMission(pkDefender, CvTypes::getMISSION_WAIT_FOR()))		// If the top mission was not a 'wait for', then clear it.
					pkDefender->ClearMissionQueue();
			}
			else
				bTargetDied = true;
		}
		else
		{
			// Target was a City
			pCity = pkTargetPlot->getPlotCity();
			CvAssert_Debug(pCity != NULL);

			if(pCity)
			{
				pCity->clearCombat();
				if(pkAttacker)
				{
					pCity->changeDamage(iAttackerDamageInflicted);
#if defined(MOD_API_UNIT_STATS)
					pkAttacker->changeDamage(iDefenderDamageInflicted, pCity->getOwner(), -1);
#else
					pkAttacker->changeDamage(iDefenderDamageInflicted, pCity->getOwner());
#endif

					//		iUnitDamage = std::max(pCity->getDamage(), pCity->getDamage() + iDamage);

					if(pkAttacker->IsDead())
					{
						if(iActivePlayerID == pkAttacker->getOwner())
						{
							strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_DIED_ATTACKING_CITY", pkAttacker->getNameKey(), pCity->getNameKey(), iAttackerDamageInflicted);
							pkDLLInterface->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitDefeatScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
							MILITARYLOG(pkAttacker->getOwner(), strBuffer.c_str(), pkAttacker->plot(), pCity->getOwner());
						}
					}

					if(pCity->getOwner() == iActivePlayerID)
					{
						strBuffer = GetLocalizedText("TXT_KEY_MISC_YOUR_CITY_ATTACKED_BY_AIR", pCity->getNameKey(), pkAttacker->getNameKey(), iDefenderDamageInflicted);
						//red icon over attacking unit
						pkDLLInterface->AddMessage(uiParentEventID, pCity->getOwner(), false, GC.getEVENT_MESSAGE_TIME(), strBuffer);
						MILITARYLOG(pCity->getOwner(), strBuffer.c_str(), pCity->plot(), pkAttacker->getOwner());
					}
				}
			}
			else
				bTargetDied = true;
		}
	}
	else
		bTargetDied = true;

	// Suicide Unit (e.g. Missiles)
	if(pkAttacker)
	{
		if(pkAttacker->isSuicide())
		{
			pkAttacker->setCombatUnit(NULL);	// Must clear this if doing a delayed kill, should this be part of the kill method?
			pkAttacker->kill(true);
		}
		else
		{
			// Experience
			if(iAttackerDamageInflicted > 0)
			{
#if defined(MOD_UNITS_XP_TIMES_100)
				pkAttacker->changeExperienceTimes100(100 * kCombatInfo.getExperience(BATTLE_UNIT_ATTACKER),
#else
				pkAttacker->changeExperience(kCombatInfo.getExperience(BATTLE_UNIT_ATTACKER),
#endif
				                             kCombatInfo.getMaxExperienceAllowed(BATTLE_UNIT_ATTACKER),
				                             true,
				                             kCombatInfo.getInBorders(BATTLE_UNIT_ATTACKER),
				                             kCombatInfo.getUpdateGlobal(BATTLE_UNIT_ATTACKER));

				// Promotion time?
				pkAttacker->testPromotionReady();

			}

			// Clean up some stuff
			pkAttacker->setCombatUnit(NULL);
			pkAttacker->ClearMissionQueue(GetPostCombatDelay());

			// Spend a move for this attack
			pkAttacker->changeMoves(-GC.getMOVE_DENOMINATOR());

			// Can't move or attack again
			if(!pkAttacker->canMoveAfterAttacking())
			{
				pkAttacker->finishMoves();
			}
		}

		// Report that combat is over in case we want to queue another attack
		GET_PLAYER(pkAttacker->getOwner()).GetTacticalAI()->CombatResolved(pkAttacker, bTargetDied);
	}
	
	BATTLE_FINISHED();
	DoNewBattleEffects(kCombatInfo);
}

//	---------------------------------------------------------------------------
void CvUnitCombat::GenerateAirSweepCombatInfo(CvUnit& kAttacker, CvUnit* pkDefender, CvPlot& plot, CvCombatInfo* pkCombatInfo)
{
	BATTLE_STARTED(BATTLE_TYPE_SWEEP, plot);
	pkCombatInfo->setBattleType(BATTLE_TYPE_SWEEP);
#if defined(MOD_UNITS_MAX_HP)
	int iAttackerMaxHP = kAttacker.GetMaxHitPoints();
#else
	int iMaxHP = GC.getMAX_HIT_POINTS();
#endif

	pkCombatInfo->setUnit(BATTLE_UNIT_ATTACKER, &kAttacker);
	pkCombatInfo->setUnit(BATTLE_UNIT_DEFENDER, pkDefender);
	pkCombatInfo->setPlot(&plot);

	// Unit vs. Unit
	CvAssert(pkDefender != NULL);

	int iAttackerStrength = kAttacker.GetMaxRangedCombatStrength(pkDefender, /*pCity*/ NULL, true, false);

	// Mod to air sweep strength
	iAttackerStrength *= (100 + kAttacker.GetAirSweepCombatModifier());
	iAttackerStrength /= 100;

	int iDefenderStrength = 0;

	int iDefenderExperience = 0;

	// Ground AA interceptor
	if(pkDefender->getDomainType() != DOMAIN_AIR)
	{
		int iInterceptionDamage = pkDefender->GetInterceptionDamage(&kAttacker);

		// Reduce damage for performing a sweep
		iInterceptionDamage *= /*75*/ GC.getAIR_SWEEP_INTERCEPTION_DAMAGE_MOD();
		iInterceptionDamage /= 100;

		//this code is no work! Ground interceptor's experence is counted elsewhere
		iDefenderExperience = /*2*/ GC.getEXPERIENCE_DEFENDING_AIR_SWEEP_GROUND();

		pkCombatInfo->setDamageInflicted(BATTLE_UNIT_DEFENDER, iInterceptionDamage);		// Damage inflicted this round
	}
	// Air interceptor
	else
	{
		iDefenderStrength = pkDefender->GetMaxRangedCombatStrength(&kAttacker, /*pCity*/ NULL, false, false);
#if defined(MOD_UNITS_MAX_HP)
		int iDefenderMaxHP = pkDefender->GetMaxHitPoints();
#endif

		int iAttackerDamageInflicted = kAttacker.getCombatDamage(iAttackerStrength, iDefenderStrength, kAttacker.getDamage(), /*bIncludeRand*/ true, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ false);
		int iDefenderDamageInflicted = pkDefender->getCombatDamage(iDefenderStrength, iAttackerStrength, pkDefender->getDamage(), /*bIncludeRand*/ true, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ false);

		int iAttackerTotalDamageInflicted = iAttackerDamageInflicted + pkDefender->getDamage();
		int iDefenderTotalDamageInflicted = iDefenderDamageInflicted + kAttacker.getDamage();

		// Will both units be killed by this? :o If so, take drastic corrective measures
#if defined(MOD_UNITS_MAX_HP)
		if (iAttackerTotalDamageInflicted >= iDefenderMaxHP && iDefenderTotalDamageInflicted >= iAttackerMaxHP)
#else
		if (iAttackerTotalDamageInflicted >= iMaxHP && iDefenderTotalDamageInflicted >= iMaxHP)
#endif
		{
			// He who hath the least amount of damage survives with 1 HP left
			if(iAttackerTotalDamageInflicted > iDefenderTotalDamageInflicted)
			{
#if defined(MOD_UNITS_MAX_HP)
				iDefenderDamageInflicted = iAttackerMaxHP - kAttacker.getDamage() - 1;
				iDefenderTotalDamageInflicted = iAttackerMaxHP - 1;
				iAttackerTotalDamageInflicted = iDefenderMaxHP;
#else
				iDefenderDamageInflicted = iMaxHP - kAttacker.getDamage() - 1;
				iDefenderTotalDamageInflicted = iMaxHP - 1;
				iAttackerTotalDamageInflicted = iMaxHP;
#endif
			}
			else
			{
#if defined(MOD_UNITS_MAX_HP)
				iAttackerDamageInflicted = iDefenderMaxHP - pkDefender->getDamage() - 1;
				iAttackerTotalDamageInflicted = iDefenderMaxHP - 1;
				iDefenderTotalDamageInflicted = iAttackerMaxHP;
#else
				iAttackerDamageInflicted = iMaxHP - pkDefender->getDamage() - 1;
				iAttackerTotalDamageInflicted = iMaxHP - 1;
				iDefenderTotalDamageInflicted = iMaxHP;
#endif
			}
		}

		iDefenderExperience = /*6*/ GC.getEXPERIENCE_DEFENDING_AIR_SWEEP_AIR();
#ifdef MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE
		if (MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE && pkDefender)
		{
			iDefenderExperience += pkDefender->ExtraDefenseXPValue();
		}
#endif

		pkCombatInfo->setFinalDamage(BATTLE_UNIT_ATTACKER, iDefenderTotalDamageInflicted);
		pkCombatInfo->setDamageInflicted(BATTLE_UNIT_ATTACKER, iAttackerDamageInflicted);
		pkCombatInfo->setFinalDamage(BATTLE_UNIT_DEFENDER, iAttackerTotalDamageInflicted);
		pkCombatInfo->setDamageInflicted(BATTLE_UNIT_DEFENDER, iDefenderDamageInflicted);

		// Fear Damage
		//pkCombatInfo->setFearDamageInflicted( BATTLE_UNIT_ATTACKER, kAttacker.getCombatDamage(iAttackerStrength, iDefenderStrength, kAttacker.getDamage(), true, false, true) );
		//	pkCombatInfo->setFearDamageInflicted( BATTLE_UNIT_DEFENDER, getCombatDamage(iDefenderStrength, iAttackerStrength, pDefender->getDamage(), true, false, true) );

#if defined(MOD_UNITS_MAX_HP)
		int iAttackerEffectiveStrength = iAttackerStrength * (iAttackerMaxHP - range(kAttacker.getDamage(), 0, iAttackerMaxHP - 1)) / iAttackerMaxHP;
#else
		int iAttackerEffectiveStrength = iAttackerStrength * (iMaxHP - range(kAttacker.getDamage(), 0, iMaxHP - 1)) / iMaxHP;
#endif
		iAttackerEffectiveStrength = iAttackerEffectiveStrength > 0 ? iAttackerEffectiveStrength : 1;
#if defined(MOD_UNITS_MAX_HP)
		int iDefenderEffectiveStrength = iDefenderStrength * (iDefenderMaxHP - range(pkDefender->getDamage(), 0, iDefenderMaxHP - 1)) / iDefenderMaxHP;
#else
		int iDefenderEffectiveStrength = iDefenderStrength * (iMaxHP - range(pkDefender->getDamage(), 0, iMaxHP - 1)) / iMaxHP;
#endif
		iDefenderEffectiveStrength = iDefenderEffectiveStrength > 0 ? iDefenderEffectiveStrength : 1;

		//int iExperience = kAttacker.defenseXPValue();
		//iExperience = ((iExperience * iAttackerEffectiveStrength) / iDefenderEffectiveStrength); // is this right? looks like more for less [Jon: Yes, it's XP for the defender]
		//iExperience = range(iExperience, GC.getMIN_EXPERIENCE_PER_COMBAT(), GC.getMAX_EXPERIENCE_PER_COMBAT());
		pkCombatInfo->setExperience(BATTLE_UNIT_DEFENDER, iDefenderExperience);
		pkCombatInfo->setMaxExperienceAllowed(BATTLE_UNIT_ATTACKER, pkDefender->maxXPValue());
		pkCombatInfo->setInBorders(BATTLE_UNIT_ATTACKER, plot.getOwner() == pkDefender->getOwner());
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS) || defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
		pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, kAttacker.isGGFromBarbarians() || !kAttacker.isBarbarian());
#else
		pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, !kAttacker.isBarbarian());
#endif

		//iExperience = ((iExperience * iDefenderEffectiveStrength) / iAttackerEffectiveStrength);
		//iExperience = range(iExperience, GC.getMIN_EXPERIENCE_PER_COMBAT(), GC.getMAX_EXPERIENCE_PER_COMBAT());
		int iExperience = /*6*/ GC.getEXPERIENCE_ATTACKING_AIR_SWEEP();
#ifdef MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE
		if (MOD_GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE)
		{
			iExperience += kAttacker.ExtraAttackXPValue();
		}
#endif
		pkCombatInfo->setExperience(BATTLE_UNIT_ATTACKER, iExperience);
		pkCombatInfo->setMaxExperienceAllowed(BATTLE_UNIT_DEFENDER, kAttacker.maxXPValue());
		pkCombatInfo->setInBorders(BATTLE_UNIT_DEFENDER, plot.getOwner() == kAttacker.getOwner());
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS) || defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
		pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_DEFENDER, pkDefender->isGGFromBarbarians() || !pkDefender->isBarbarian());
#else
		pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_DEFENDER, !pkDefender->isBarbarian());
#endif
	}

	pkCombatInfo->setAttackIsRanged(false);
	pkCombatInfo->setAttackIsAirSweep(true);
	pkCombatInfo->setAttackerAdvances(false);
	pkCombatInfo->setDefenderRetaliates(true);

	GC.GetEngineUserInterface()->setDirty(UnitInfo_DIRTY_BIT, true);
}

//	---------------------------------------------------------------------------
void CvUnitCombat::ResolveAirSweep(const CvCombatInfo& kCombatInfo, uint uiParentEventID)
{
	// After combat stuff
	CvString strBuffer;
	bool bAttackerDead = false;
	bool bDefenderDead = false;

	CvUnit* pkAttacker = kCombatInfo.getUnit(BATTLE_UNIT_ATTACKER);
	CvUnit* pkDefender = kCombatInfo.getUnit(BATTLE_UNIT_DEFENDER);
	CvPlot* pkTargetPlot = kCombatInfo.getPlot();
	if(!pkTargetPlot && pkDefender)
		pkTargetPlot = pkDefender->plot();

	CvAssert_Debug(pkAttacker && pkDefender && pkTargetPlot);

	// Internal variables
	int iAttackerDamageInflicted = kCombatInfo.getDamageInflicted(BATTLE_UNIT_ATTACKER);
	int iDefenderDamageInflicted = kCombatInfo.getDamageInflicted(BATTLE_UNIT_DEFENDER);

	// Made interception!
	if(pkDefender)
	{
		pkDefender->setMadeInterception(true);
		if(pkAttacker && pkTargetPlot)
		{
#if !defined(NO_ACHIEVEMENTS)
			//One Hit
#if defined(MOD_UNITS_MAX_HP)
			if(pkDefender->GetCurrHitPoints() == pkDefender->GetMaxHitPoints() && pkAttacker->isHuman() && !GC.getGame().isGameMultiPlayer())
#else
			if(pkDefender->GetCurrHitPoints() == GC.getMAX_HIT_POINTS() && pkAttacker->isHuman() && !GC.getGame().isGameMultiPlayer())
#endif
			{
				gDLL->UnlockAchievement(ACHIEVEMENT_ONEHITKILL);
			}
#endif
#if defined(MOD_PROMOTION_GET_INSTANCE_FROM_ATTACK)
			DoInstantYieldFromCombat(kCombatInfo);
#endif	

#if defined(MOD_API_UNIT_STATS)
			pkDefender->changeDamage(iAttackerDamageInflicted, pkAttacker->getOwner(), pkAttacker->GetID());
			pkAttacker->changeDamage(iDefenderDamageInflicted, pkDefender->getOwner(), pkDefender->GetID());
#else
			pkDefender->changeDamage(iAttackerDamageInflicted, pkAttacker->getOwner());
			pkAttacker->changeDamage(iDefenderDamageInflicted, pkDefender->getOwner());
#endif

			// Update experience for both sides.
#if defined(MOD_UNITS_XP_TIMES_100)
			pkDefender->changeExperienceTimes100(100 *
#else
			pkDefender->changeExperience(
#endif
			    kCombatInfo.getExperience(BATTLE_UNIT_DEFENDER),
			    kCombatInfo.getMaxExperienceAllowed(BATTLE_UNIT_DEFENDER),
			    true,
			    kCombatInfo.getInBorders(BATTLE_UNIT_DEFENDER),
			    kCombatInfo.getUpdateGlobal(BATTLE_UNIT_DEFENDER));

#if defined(MOD_UNITS_XP_TIMES_100)
			pkAttacker->changeExperienceTimes100(100 * 
#else
			pkAttacker->changeExperience(
#endif
			    kCombatInfo.getExperience(BATTLE_UNIT_ATTACKER),
			    kCombatInfo.getMaxExperienceAllowed(BATTLE_UNIT_ATTACKER),
			    true,
			    kCombatInfo.getInBorders(BATTLE_UNIT_ATTACKER),
			    kCombatInfo.getUpdateGlobal(BATTLE_UNIT_ATTACKER));

			// Anyone eat it?
#if defined(MOD_UNITS_MAX_HP)
			bAttackerDead = (pkAttacker->getDamage() >= pkAttacker->GetMaxHitPoints());
			bDefenderDead = (pkDefender->getDamage() >= pkDefender->GetMaxHitPoints());
#else
			bAttackerDead = (pkAttacker->getDamage() >= GC.getMAX_HIT_POINTS());
			bDefenderDead = (pkDefender->getDamage() >= GC.getMAX_HIT_POINTS());
#endif

			int iActivePlayerID = GC.getGame().getActivePlayer();

			//////////////////////////////////////////////////////////////////////////

			// Ground AA interceptor
			if(pkDefender->getDomainType() != DOMAIN_AIR)
			{
				// Attacker died
				if(bAttackerDead)
				{
					auto_ptr<ICvUnit1> pAttacker = GC.WrapUnitPointer(pkAttacker);
					gDLL->GameplayUnitDestroyedInCombat(pAttacker.get());

					if(iActivePlayerID == pkAttacker->getOwner())
					{
						strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_DIED_ATTACKING", pkAttacker->getNameKey(), pkDefender->getNameKey(), iAttackerDamageInflicted, 0);
						GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitDefeatScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
						MILITARYLOG(pkAttacker->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkDefender->getOwner());
					}
					if(iActivePlayerID == pkDefender->getOwner())
					{
						strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_KILLED_ENEMY_UNIT", pkDefender->getNameKey(), iAttackerDamageInflicted, 0, pkAttacker->getNameKey(), pkAttacker->getVisualCivAdjective(pkDefender->getTeam()));
						GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitVictoryScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
						MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkAttacker->getOwner());
					}
					pkDefender->testPromotionReady();
				}
			}
			// Air AA interceptor
			else
			{
				// Attacker died
				if(bAttackerDead)
				{
					auto_ptr<ICvUnit1> pAttacker = GC.WrapUnitPointer(pkAttacker);
					gDLL->GameplayUnitDestroyedInCombat(pAttacker.get());

					if(iActivePlayerID == pkAttacker->getOwner())
					{
						strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_DIED_ATTACKING", pkAttacker->getNameKey(), pkDefender->getNameKey(), iAttackerDamageInflicted, 0);
						GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitDefeatScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
						MILITARYLOG(pkAttacker->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkDefender->getOwner());
					}
					if(iActivePlayerID == pkDefender->getOwner())
					{
						strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_KILLED_ENEMY_UNIT", pkDefender->getNameKey(), iAttackerDamageInflicted, 0, pkAttacker->getNameKey(), pkAttacker->getVisualCivAdjective(pkDefender->getTeam()));
						GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitVictoryScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
						MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkAttacker->getOwner());
					}

					pkDefender->testPromotionReady();

					ApplyPostCombatTraitEffects(pkDefender, pkAttacker);
				}
				// Defender died
				else if(bDefenderDead)
				{
					auto_ptr<ICvUnit1> pDefender = GC.WrapUnitPointer(pkDefender);
					gDLL->GameplayUnitDestroyedInCombat(pDefender.get());

					if(iActivePlayerID == pkAttacker->getOwner())
					{
						strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_DESTROYED_ENEMY", pkAttacker->getNameKey(), iDefenderDamageInflicted, pkDefender->getNameKey());
						GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitVictoryScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
						MILITARYLOG(pkAttacker->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkDefender->getOwner());
					}
					if(iActivePlayerID == pkDefender->getOwner())
					{
						if(pkAttacker->getVisualOwner(pkDefender->getTeam()) != pkAttacker->getOwner())
						{
							strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_WAS_DESTROYED_UNKNOWN", pkDefender->getNameKey(), pkAttacker->getNameKey(), iDefenderDamageInflicted);
						}
						else
						{
							strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_WAS_DESTROYED", pkDefender->getNameKey(), pkAttacker->getNameKey(), pkAttacker->getVisualCivAdjective(pkDefender->getTeam()), iDefenderDamageInflicted);
						}
						GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*,GC.getEraInfo(GC.getGame().getCurrentEra())->getAudioUnitDefeatScript(), MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
						MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pkDefender->plot(), ((pkAttacker->getVisualOwner(pkDefender->getTeam()) != pkAttacker->getOwner()) ? NO_PLAYER : pkAttacker->getOwner()));
					}

					CvNotifications* pNotification = GET_PLAYER(pkDefender->getOwner()).GetNotifications();
					if(pNotification)
					{
						if(pkAttacker->getVisualOwner(pkDefender->getTeam()) != pkAttacker->getOwner())
						{
							strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_WAS_DESTROYED_UNKNOWN", pkDefender->getNameKey(), pkAttacker->getNameKey(), iDefenderDamageInflicted);
						}
						else
						{
							strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_WAS_DESTROYED", pkDefender->getNameKey(), pkAttacker->getNameKey(), pkAttacker->getVisualCivAdjective(pkDefender->getTeam()), iDefenderDamageInflicted);
						}
						Localization::String strSummary = Localization::Lookup("TXT_KEY_UNIT_LOST");
						pNotification->Add(NOTIFICATION_UNIT_DIED, strBuffer, strSummary.toUTF8(), pkDefender->getX(), pkDefender->getY(), (int) pkDefender->getUnitType(), pkDefender->getOwner());
					}

					pkAttacker->testPromotionReady();

					ApplyPostCombatTraitEffects(pkAttacker, pkDefender);
				}
				// Nobody died
				else
				{
					if(iActivePlayerID == pkAttacker->getOwner())
					{
						strBuffer = GetLocalizedText("TXT_KEY_MISC_YOU_UNIT_WITHDRAW", pkAttacker->getNameKey(), iDefenderDamageInflicted, pkDefender->getNameKey(), iAttackerDamageInflicted);
						GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkAttacker->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, "AS2D_OUR_WITHDRAWL", MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
						MILITARYLOG(pkAttacker->getOwner(), strBuffer.c_str(), pkAttacker->plot(), pkDefender->getOwner());
					}
					if(iActivePlayerID == pkDefender->getOwner())
					{
						strBuffer = GetLocalizedText("TXT_KEY_MISC_ENEMY_UNIT_WITHDRAW", pkAttacker->getNameKey(), iDefenderDamageInflicted, pkDefender->getNameKey(), iAttackerDamageInflicted);
						GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pkDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer/*, "AS2D_THEIR_WITHDRAWL", MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
						MILITARYLOG(pkDefender->getOwner(), strBuffer.c_str(), pkDefender->plot(), pkAttacker->getOwner());
					}

					pkDefender->testPromotionReady();
					pkAttacker->testPromotionReady();
				}
			}
		}
	}
	else
		bDefenderDead = true;

	// Clean up some stuff
	if(pkDefender)
	{
		pkDefender->setCombatUnit(NULL);
		pkDefender->ClearMissionQueue();
	}

	if(pkAttacker)
	{
		pkAttacker->setCombatUnit(NULL);
		pkAttacker->ClearMissionQueue(GetPostCombatDelay());

		// Spend a move for this attack
		pkAttacker->changeMoves(-GC.getMOVE_DENOMINATOR());

		// Can't move or attack again
		if(!pkAttacker->canMoveAfterAttacking())
		{
			pkAttacker->finishMoves();
		}

		// Report that combat is over in case we want to queue another attack
		GET_PLAYER(pkAttacker->getOwner()).GetTacticalAI()->CombatResolved(pkAttacker, bDefenderDead);
	}
	
	BATTLE_FINISHED();
	DoNewBattleEffects(kCombatInfo);
}

//	GenerateNuclearCombatInfo
//	Function: GenerateNuclearCombatInfo
//	Take the input parameters and fill in a CvCombatInfo definition assuming a
//	mission to do a nuclear attack.
//
//	Parameters:
//		pkDefender   	-	Defending unit.  Can be null, in which case the input plot must have a city
//		plot         	-	The plot of the defending unit/city
//		pkCombatInfo 	-	Output combat info
//	---------------------------------------------------------------------------
void CvUnitCombat::GenerateNuclearCombatInfo(CvUnit& kAttacker, CvPlot& plot, CvCombatInfo* pkCombatInfo)
{
	BATTLE_STARTED(BATTLE_TYPE_NUKE, plot);
	pkCombatInfo->setBattleType(BATTLE_TYPE_NUKE);
	pkCombatInfo->setUnit(BATTLE_UNIT_ATTACKER, &kAttacker);
	pkCombatInfo->setUnit(BATTLE_UNIT_DEFENDER, NULL);
	pkCombatInfo->setPlot(&plot);

#if defined(MOD_EVENTS_BATTLES)
	if (plot.isCity())
	{
		BATTLE_JOINED(plot.getPlotCity(), BATTLE_UNIT_DEFENDER, true);
	}
	else
	{
		CvUnit* pBestDefender = plot.getBestDefender(NO_PLAYER, kAttacker.getOwner()).pointer();
		BATTLE_JOINED(pBestDefender, BATTLE_UNIT_DEFENDER, false);
	}
#endif




#if defined(MOD_ROG_CORE)

	// Any interception to be done?
	CvCity* pInterceptionCity = plot.GetNukeInterceptor(kAttacker.getOwner());
	bool bInterceptionSuccess = false, bPartialInterception = false;
	CvPlot* pInterceptionCityPlot = NULL;

	if (pInterceptionCity != NULL)
	{
		pInterceptionCityPlot = pInterceptionCity->plot();
		if (pInterceptionCityPlot)
		{
			BATTLE_JOINED(plot.getPlotCity(), BATTLE_UNIT_DEFENDER, true);
			pkCombatInfo->setCity(BATTLE_UNIT_DEFENDER, pInterceptionCity);

			if (GC.getGame().getSmallFakeRandNum(100, plot.GetPlotIndex() + kAttacker.GetID()) <= pInterceptionCity->getNukeInterceptionChance())
			{
				bInterceptionSuccess = true;
			}
			if (bInterceptionSuccess)
			{
				if (kAttacker.GetNukeDamageLevel() <= 2) // Atomic Bombs are destroyed outright
				{
					pkCombatInfo->setDamageInflicted(BATTLE_UNIT_INTERCEPTOR, kAttacker.GetCurrHitPoints());
					kAttacker.kill(true, pInterceptionCity->getOwner());
				}

				else if (kAttacker.GetNukeDamageLevel() == 3) // Icbm Missiles are converted to Atomic Bombs
				{
					bPartialInterception = true;
				}

				else // anti Missiles are immue
				{
					bInterceptionSuccess = false;
				}
			}
		}
	}
#endif


	//////////////////////////////////////////////////////////////////////

	CvString strBuffer;
	bool abTeamsAffected[MAX_TEAMS];
	int iI;
	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		abTeamsAffected[iI] = kAttacker.isNukeVictim(&plot, ((TeamTypes)iI));
	}

	int iPlotTeam = plot.getTeam();
	bool bWar = false;
	bool bBystander = false;

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (abTeamsAffected[iI])
		{
			if (!kAttacker.isEnemy((TeamTypes)iI))
			{
#if defined(MOD_EVENTS_WAR_AND_PEACE)
				GET_TEAM(kAttacker.getTeam()).declareWar(((TeamTypes)iI), false, kAttacker.getOwner());
#else
				GET_TEAM(kAttacker.getTeam()).declareWar(((TeamTypes)iI));
#endif

				if (iPlotTeam == iI)
				{
					bWar = true;
				}
				else
				{
					bBystander = true;
				}
			}
		}
	}

#if defined(MOD_EVENTS_NUCLEAR_DETONATION)
	if (MOD_EVENTS_NUCLEAR_DETONATION) {
		GAMEEVENTINVOKE_HOOK(GAMEEVENT_NuclearDetonation, kAttacker.getOwner(), kAttacker.GetID(), plot.getX(), plot.getY(), bWar, bBystander);
	}
	else {
#endif
		ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
		if (pkScriptSystem)
		{
			CvLuaArgsHandle args;

			args->Push(kAttacker.getOwner());
			args->Push(kAttacker.GetID());
			args->Push(plot.getX());
			args->Push(plot.getY());
			args->Push(bWar);
			args->Push(bBystander);


			bool bResult;
			LuaSupport::CallHook(pkScriptSystem, "NuclearDetonation", args.get(), bResult);
		}
#if defined(MOD_EVENTS_NUCLEAR_DETONATION)
	}
#endif

	kAttacker.setReconPlot(&plot);

	//////////////////////////////////////////////////////////////////////

	pkCombatInfo->setFinalDamage(BATTLE_UNIT_ATTACKER, 0);		// Total damage to the unit
	pkCombatInfo->setDamageInflicted(BATTLE_UNIT_ATTACKER, 0);	// Damage inflicted this round
	pkCombatInfo->setFinalDamage(BATTLE_UNIT_DEFENDER, 0);		// Total damage to the unit
	pkCombatInfo->setDamageInflicted(BATTLE_UNIT_DEFENDER, 0);	// Damage inflicted this round

	pkCombatInfo->setFearDamageInflicted(BATTLE_UNIT_ATTACKER, 0);

	pkCombatInfo->setExperience(BATTLE_UNIT_ATTACKER, 0);
	pkCombatInfo->setMaxExperienceAllowed(BATTLE_UNIT_ATTACKER, 0);
	pkCombatInfo->setInBorders(BATTLE_UNIT_ATTACKER, plot.getOwner() != kAttacker.getOwner());	// Not really correct
#if defined(MOD_TRAITS_GG_FROM_BARBARIANS) || defined(MOD_PROMOTIONS_GG_FROM_BARBARIANS)
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, kAttacker.isGGFromBarbarians() || !kAttacker.isBarbarian());
#else
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_ATTACKER, !kAttacker.isBarbarian());
#endif

	pkCombatInfo->setExperience(BATTLE_UNIT_DEFENDER, 0);
	pkCombatInfo->setMaxExperienceAllowed(BATTLE_UNIT_DEFENDER, 0);
	pkCombatInfo->setInBorders(BATTLE_UNIT_DEFENDER, plot.getOwner() == kAttacker.getOwner());
	pkCombatInfo->setUpdateGlobal(BATTLE_UNIT_DEFENDER, false);



	if (bInterceptionSuccess)
	{
		// Send out notifications to the world
		for (int iPlayerLoop = 0; iPlayerLoop < MAX_PLAYERS; iPlayerLoop++)
		{
			PlayerTypes eLoopPlayer = (PlayerTypes)iPlayerLoop;

			if (GET_PLAYER(eLoopPlayer).isObserver() || (GET_PLAYER(eLoopPlayer).isHuman() && GET_PLAYER(eLoopPlayer).isAlive()))
			{
				if (!GET_PLAYER(eLoopPlayer).isObserver())
				{
					if (!GET_TEAM(GET_PLAYER(eLoopPlayer).getTeam()).isHasMet(GET_PLAYER(pInterceptionCity->getOwner()).getTeam()))
						continue;

					if (!GET_TEAM(GET_PLAYER(eLoopPlayer).getTeam()).isHasMet(GET_PLAYER(kAttacker.getOwner()).getTeam()))
						continue;
				}

				CvNotifications* pNotifications = GET_PLAYER(eLoopPlayer).GetNotifications();
				if (pNotifications)
				{
					if (!bPartialInterception)
					{
						if (eLoopPlayer == kAttacker.getOwner())
						{
							Localization::String strSummary = Localization::Lookup("TXT_KEY_NUKE_INTERCEPTED_US_S");
							Localization::String strBuffer = Localization::Lookup("TXT_KEY_NUKE_INTERCEPTED_US");
							strBuffer << pInterceptionCity->getNameKey();
							pNotifications->Add(NOTIFICATION_UNIT_DIED, strBuffer.toUTF8(), strSummary.toUTF8(), pInterceptionCity->getX(), pInterceptionCity->getY(), (int)kAttacker.getUnitType(), pInterceptionCity->getOwner());
						}
						else if (GET_PLAYER(eLoopPlayer).isObserver() || pInterceptionCityPlot->isRevealed(GET_PLAYER(eLoopPlayer).getTeam()))
						{
							Localization::String strSummary = Localization::Lookup("TXT_KEY_NUKE_INTERCEPTED_S");
							Localization::String strBuffer = Localization::Lookup("TXT_KEY_NUKE_INTERCEPTED");
							strBuffer << GET_PLAYER(kAttacker.getOwner()).getCivilizationAdjectiveKey();
							strBuffer << pInterceptionCity->getNameKey();
							pNotifications->Add(NOTIFICATION_UNIT_DIED, strBuffer.toUTF8(), strSummary.toUTF8(), pInterceptionCity->getX(), pInterceptionCity->getY(), (int)kAttacker.getUnitType(), pInterceptionCity->getOwner());
						}
						else
						{
							Localization::String strSummary = Localization::Lookup("TXT_KEY_NUKE_INTERCEPTED_S");
							Localization::String strBuffer = Localization::Lookup("TXT_KEY_NUKE_INTERCEPTED");
							strBuffer << GET_PLAYER(kAttacker.getOwner()).getCivilizationShortDescription();
							strBuffer << pInterceptionCity->getNameKey();
							pNotifications->Add(NOTIFICATION_UNIT_DIED, strBuffer.toUTF8(), strSummary.toUTF8(), -1, -1, (int)kAttacker.getUnitType(), pInterceptionCity->getOwner());
						}
						}
					else
					{
						if (eLoopPlayer == kAttacker.getOwner())
						{
							Localization::String strSummary = Localization::Lookup("TXT_KEY_NUKE_INTERCEPTED_US_S");
							Localization::String strBuffer = Localization::Lookup("TXT_KEY_NUKE_INTERCEPTED_US");
							strBuffer << pInterceptionCity->getNameKey();
							pNotifications->Add(NOTIFICATION_UNIT_DIED, strBuffer.toUTF8(), strSummary.toUTF8(), pInterceptionCity->getX(), pInterceptionCity->getY(), (int)kAttacker.getUnitType(), pInterceptionCity->getOwner());
						}
						else if (GET_PLAYER(eLoopPlayer).isObserver() || pInterceptionCityPlot->isRevealed(GET_PLAYER(eLoopPlayer).getTeam()))
						{
							Localization::String strSummary = Localization::Lookup("TXT_KEY_NUKE_INTERCEPTED_S");
							Localization::String strBuffer = Localization::Lookup("TXT_KEY_NUKE_INTERCEPTED");
							strBuffer << GET_PLAYER(kAttacker.getOwner()).getCivilizationAdjectiveKey();
							strBuffer << pInterceptionCity->getNameKey();
							pNotifications->Add(NOTIFICATION_UNIT_DIED, strBuffer.toUTF8(), strSummary.toUTF8(), pInterceptionCity->getX(), pInterceptionCity->getY(), (int)kAttacker.getUnitType(), pInterceptionCity->getOwner());
						}
						else
						{
							Localization::String strSummary = Localization::Lookup("TXT_KEY_NUKE_INTERCEPTED_S");
							Localization::String strBuffer = Localization::Lookup("TXT_KEY_NUKE_INTERCEPTED");
							strBuffer << GET_PLAYER(kAttacker.getOwner()).getCivilizationShortDescription();
							strBuffer << pInterceptionCity->getNameKey();
							pNotifications->Add(NOTIFICATION_UNIT_DIED, strBuffer.toUTF8(), strSummary.toUTF8(), -1, -1, (int)kAttacker.getUnitType(), pInterceptionCity->getOwner());
						}
					}
					}
				}
			}

		// If completely intercepted, apply diplomacy penalty and end here.
		if (!bPartialInterception)
		{
			if (GET_PLAYER(pInterceptionCity->getOwner()).isMajorCiv() && GET_PLAYER(kAttacker.getOwner()).isMajorCiv())
				GET_PLAYER(pInterceptionCity->getOwner()).GetDiplomacyAI()->ChangeNumTimesNuked(kAttacker.getOwner(), 1);

			return;
		}
		}

	int iNukeDamageLevel = bPartialInterception ? 1 : kAttacker.GetNukeDamageLevel();








	pkCombatInfo->setAttackIsBombingMission(true);
	pkCombatInfo->setDefenderRetaliates(false);
	pkCombatInfo->setAttackNuclearLevel(iNukeDamageLevel + 1);
	//pkCombatInfo->setAttackNuclearLevel(kAttacker.GetNukeDamageLevel() + 1);


	// Set all of the units in the blast radius to defenders and calculate their damage
	int iDamageMembers = 0;
	GenerateNuclearExplosionDamage(&plot, iNukeDamageLevel, &kAttacker, pkCombatInfo->getDamageMembers(), &iDamageMembers, pkCombatInfo->getMaxDamageMemberCount());
	//GenerateNuclearExplosionDamage(&plot, kAttacker.GetNukeDamageLevel(), &kAttacker, pkCombatInfo->getDamageMembers(), &iDamageMembers, pkCombatInfo->getMaxDamageMemberCount());
	pkCombatInfo->setDamageMemberCount(iDamageMembers);

	GC.GetEngineUserInterface()->setDirty(UnitInfo_DIRTY_BIT, true);
	}

//	-------------------------------------------------------------------------------------
uint CvUnitCombat::ApplyNuclearExplosionDamage(CvPlot* pkTargetPlot, int iDamageLevel, CvUnit* /* pkAttacker = NULL*/)
{
	CvCombatMemberEntry kDamageMembers[MAX_NUKE_DAMAGE_MEMBERS];
	int iDamageMembers = 0;
	GenerateNuclearExplosionDamage(pkTargetPlot, iDamageLevel, NULL, &kDamageMembers[0], &iDamageMembers, MAX_NUKE_DAMAGE_MEMBERS);
	return ApplyNuclearExplosionDamage(&kDamageMembers[0], iDamageMembers, NULL, pkTargetPlot, iDamageLevel);
}

//	-------------------------------------------------------------------------------------
uint CvUnitCombat::ApplyNuclearExplosionDamage(const CvCombatMemberEntry* pkDamageArray, int iDamageMembers, CvUnit* pkAttacker, CvPlot* pkTargetPlot, int iDamageLevel)
{
	uint uiOpposingDamageCount = 0;
	PlayerTypes eAttackerOwner = pkAttacker ? pkAttacker->getOwner() : NO_PLAYER;

	// Do all the units first
	for (int i = 0; i < iDamageMembers; ++i)
	{
		const CvCombatMemberEntry& kEntry = pkDamageArray[i];
		if (kEntry.IsUnit())
		{
			CvUnit* pkUnit = GET_PLAYER(kEntry.GetPlayer()).getUnit(kEntry.GetUnitID());
			if (pkUnit)
			{
				// Apply the damage
				pkUnit->setCombatUnit(NULL);
				pkUnit->ClearMissionQueue();
				pkUnit->SetAutomateType(NO_AUTOMATE); // kick unit out of automation

				if ((eAttackerOwner == NO_PLAYER || pkUnit->getOwner() != eAttackerOwner) && !pkUnit->isBarbarian())
					uiOpposingDamageCount++;	// Count the number of non-barbarian opposing units

				if (pkUnit->IsCombatUnit() || pkUnit->IsCanAttackRanged())
				{
#if defined(MOD_API_UNIT_STATS)
					pkUnit->changeDamage(kEntry.GetDamage(), eAttackerOwner, pkAttacker ? pkAttacker->GetID() : -1);
#else
					pkUnit->changeDamage(kEntry.GetDamage(), eAttackerOwner);
#endif
				}
				else if (kEntry.GetDamage() >= /*6*/ GC.getNUKE_NON_COMBAT_DEATH_THRESHOLD())
				{
					pkUnit->kill(false, eAttackerOwner);
				}

				GET_PLAYER(kEntry.GetPlayer()).GetDiplomacyAI()->ChangeNumTimesNuked(pkAttacker->getOwner(), 1);
			}
		}
	}

	// Then the terrain effects
	int iBlastRadius = GC.getNUKE_BLAST_RADIUS();

	for (int iDX = -(iBlastRadius); iDX <= iBlastRadius; iDX++)
	{
		for (int iDY = -(iBlastRadius); iDY <= iBlastRadius; iDY++)
		{
			CvPlot* pLoopPlot = plotXYWithRangeCheck(pkTargetPlot->getX(), pkTargetPlot->getY(), iDX, iDY, iBlastRadius);

			if (pLoopPlot != NULL)
			{
				// if we remove roads, don't remove them on the city... XXX
				CvCity* pLoopCity = pLoopPlot->getPlotCity();

				if (pLoopCity == NULL)
				{
					if (!(pLoopPlot->isWater()) && !(pLoopPlot->isImpassable()))
					{
						if (pLoopPlot->getFeatureType() != NO_FEATURE)
						{
							CvFeatureInfo* pkFeatureInfo = GC.getFeatureInfo(pLoopPlot->getFeatureType());
							if (pkFeatureInfo && !pkFeatureInfo->isNukeImmune())
							{
								if (pLoopPlot == pkTargetPlot || GC.getGame().getSmallFakeRandNum(100, *pLoopPlot) < GC.getNUKE_FALLOUT_PROB())
									///if(pLoopPlot == pkTargetPlot || GC.getGame().getJonRandNum(100, "Nuke Fallout") < GC.getNUKE_FALLOUT_PROB())
								{
									if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT)
									{
										pLoopPlot->SetImprovementPillaged(true);
									}
									pLoopPlot->setFeatureType((FeatureTypes)(GC.getNUKE_FEATURE()));
								}
							}
						}
						else
						{
							if (pLoopPlot == pkTargetPlot || GC.getGame().getSmallFakeRandNum(100, *pLoopPlot) < GC.getNUKE_FALLOUT_PROB())
								//if(pLoopPlot == pkTargetPlot || GC.getGame().getJonRandNum(100, "Nuke Fallout") < GC.getNUKE_FALLOUT_PROB())
							{
								if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT)
								{
									pLoopPlot->SetImprovementPillaged(true);
								}
								pLoopPlot->setFeatureType((FeatureTypes)(GC.getNUKE_FEATURE()));
							}
						}
#if defined(MOD_GLOBAL_NUKES_MELT_ICE)
					}
					else if (MOD_GLOBAL_NUKES_MELT_ICE && pLoopPlot->getFeatureType() == FEATURE_ICE) {
						if (pLoopPlot == pkTargetPlot || GC.getGame().getSmallFakeRandNum(100, *pLoopPlot) < GC.getNUKE_FALLOUT_PROB()) {
							///if (pLoopPlot == pkTargetPlot || GC.getGame().getJonRandNum(100, "Nuke Fallout") < GC.getNUKE_FALLOUT_PROB()) {
							pLoopPlot->setFeatureType(NO_FEATURE);
						}
#endif
					}
				}
			}
		}
	}

	// Then the cities
	for (int i = 0; i < iDamageMembers; ++i)
	{
		const CvCombatMemberEntry& kEntry = pkDamageArray[i];
		if (kEntry.IsCity())
		{
			CvCity* pkCity = GET_PLAYER(kEntry.GetPlayer()).getCity(kEntry.GetCityID());
			if (pkCity)
			{
				pkCity->setCombatUnit(NULL);

				if (eAttackerOwner == NO_PLAYER || pkCity->getOwner() != eAttackerOwner)
					uiOpposingDamageCount++;

				if (kEntry.GetFinalDamage() >= pkCity->GetMaxHitPoints() && !pkCity->IsOriginalCapital())
				{
					auto_ptr<ICvCity1> pkDllCity(new CvDllCity(pkCity));
					gDLL->GameplayCitySetDamage(pkDllCity.get(), 0, pkCity->getDamage()); // to stop the fires
					gDLL->GameplayCityDestroyed(pkDllCity.get(), NO_PLAYER);

					PlayerTypes eOldOwner = pkCity->getOwner();
					pkCity->kill();

					// slewis - check for killing a player
					GET_PLAYER(pkAttacker->getOwner()).CheckForMurder(eOldOwner);
				}
				else
				{
					// Unlike the city hit points, the population damage is calculated when the pre-calculated damage is applied.
					// This is simply to save space in the damage array, since the combat visualization does not need it.
					// It can be moved into the pre-calculated damage array if needed.
					int iBaseDamage, iRandDamage1, iRandDamage2;
					// How much destruction is unleashed on nearby Cities?
					if (iDamageLevel == 1)
					{
						iBaseDamage = /*30*/ GC.getNUKE_LEVEL1_POPULATION_DEATH_BASE();
						iRandDamage1 = GC.getGame().getSmallFakeRandNum(/*20*/ GD_INT_GET(NUKE_LEVEL1_POPULATION_DEATH_RAND_1), pkCity->getPopulation() + i);
						iRandDamage2 = GC.getGame().getSmallFakeRandNum(/*20*/ GD_INT_GET(NUKE_LEVEL1_POPULATION_DEATH_RAND_2), pkCity->GetPower() + i);
						//iRandDamage1 = GC.getGame().getJonRandNum(/*20*/ GC.getNUKE_LEVEL1_POPULATION_DEATH_RAND_1(), "Population Nuked 1");
						//iRandDamage2 = GC.getGame().getJonRandNum(/*20*/ GC.getNUKE_LEVEL1_POPULATION_DEATH_RAND_2(), "Population Nuked 2");
					}
					else
					{
						iBaseDamage = /*60*/ GC.getNUKE_LEVEL2_POPULATION_DEATH_BASE();
						iRandDamage1 = GC.getGame().getSmallFakeRandNum(/*10*/ GD_INT_GET(NUKE_LEVEL2_POPULATION_DEATH_RAND_1), pkCity->getPopulation() + i);
						iRandDamage2 = GC.getGame().getSmallFakeRandNum(/*10*/ GD_INT_GET(NUKE_LEVEL2_POPULATION_DEATH_RAND_2), pkCity->GetPower() + i);
						//iRandDamage1 = GC.getGame().getJonRandNum(/*10*/ GC.getNUKE_LEVEL2_POPULATION_DEATH_RAND_1(), "Population Nuked 1");
						//iRandDamage2 = GC.getGame().getJonRandNum(/*10*/ GC.getNUKE_LEVEL2_POPULATION_DEATH_RAND_2(), "Population Nuked 2");
					}

					int iNukedPopulation = pkCity->getPopulation() * (iBaseDamage + iRandDamage1 + iRandDamage2) / 100;

					iNukedPopulation *= std::max(0, (pkCity->getNukeModifier() + 100));
					iNukedPopulation /= 100;

					pkCity->changePopulation(-(std::min((pkCity->getPopulation() - 1), iNukedPopulation)));

					// Add damage to the city
					pkCity->setDamage(kEntry.GetFinalDamage());

					GET_PLAYER(pkCity->getOwner()).GetDiplomacyAI()->ChangeNumTimesNuked(pkAttacker->getOwner(), 1);
					}
				}
			}
		}
	return uiOpposingDamageCount;
	}

//	-------------------------------------------------------------------------------------
//	Generate nuclear explosion damage for all the units and cities in the radius of the specified plot.
//	The attacker is optional, this is also called for a meltdown
void CvUnitCombat::GenerateNuclearExplosionDamage(CvPlot* pkTargetPlot, int iDamageLevel, CvUnit* pkAttacker, CvCombatMemberEntry* pkDamageArray, int* piDamageMembers, int iMaxDamageMembers)
{
	int iBlastRadius = GC.getNUKE_BLAST_RADIUS();

#if defined(MOD_EVENTS_BATTLES)
	CvCity* pDefenderCity = NULL;
	CvUnit* pDefenderUnit = NULL;

	if (pkTargetPlot->isCity())
	{
		pDefenderCity = pkTargetPlot->getPlotCity();
	}
	else
	{
		pDefenderUnit = pkTargetPlot->getBestDefender(NO_PLAYER, pkAttacker->getOwner()).pointer();
	}
#endif

	* piDamageMembers = 0;

	for (int iDX = -(iBlastRadius); iDX <= iBlastRadius; iDX++)
	{
		for (int iDY = -(iBlastRadius); iDY <= iBlastRadius; iDY++)
		{
			CvPlot* pLoopPlot = plotXYWithRangeCheck(pkTargetPlot->getX(), pkTargetPlot->getY(), iDX, iDY, iBlastRadius);

			if (pLoopPlot != NULL)
			{
				CvCity* pLoopCity = pLoopPlot->getPlotCity();

				FFastSmallFixedList<IDInfo, 25, true, c_eCiv5GameplayDLL > oldUnits;
				IDInfo* pUnitNode = pLoopPlot->headUnitNode();

				while (pUnitNode != NULL)
				{
					oldUnits.insertAtEnd(pUnitNode);
					pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);
				}

				pUnitNode = oldUnits.head();

				while (pUnitNode != NULL)
				{
					CvUnit* pLoopUnit = ::getUnit(*pUnitNode);
					pUnitNode = oldUnits.next(pUnitNode);

					if (pLoopUnit != NULL)
					{
						if (pLoopUnit != pkAttacker)
						{
							if (!pLoopUnit->isNukeImmune() && !pLoopUnit->isDelayedDeath())
							{
								int iNukeDamage;
								// How much destruction is unleashed on nearby Units?
								if (iDamageLevel == 1 && pLoopPlot != pkTargetPlot)	// Nuke level 1, but NOT the plot that got hit directly (units there are killed)
								{
									iNukeDamage = (/*3*/ GC.getNUKE_UNIT_DAMAGE_BASE() + /*4*/ GC.getGame().getJonRandNum(GC.getNUKE_UNIT_DAMAGE_RAND_1(), "Nuke Damage 1") + /*4*/ GC.getGame().getJonRandNum(GC.getNUKE_UNIT_DAMAGE_RAND_2(), "Nuke Damage 2"));
								}
								// Wipe everything out
								else
								{
#if defined(MOD_UNITS_MAX_HP)
									iNukeDamage = pLoopUnit->GetMaxHitPoints();
#else
									iNukeDamage = GC.getMAX_HIT_POINTS();
#endif
								}

								if (pLoopCity != NULL)
								{
									iNukeDamage *= std::max(0, (pLoopCity->getNukeModifier() + 100));
									iNukeDamage /= 100;
								}

								CvCombatMemberEntry* pkDamageEntry = AddCombatMember(pkDamageArray, piDamageMembers, iMaxDamageMembers, pLoopUnit);
								if (pkDamageEntry)
								{
#if defined(MOD_EVENTS_BATTLES)
									if (pLoopUnit != pDefenderUnit)
									{
										BATTLE_JOINED(pLoopUnit, BATTLE_UNIT_COUNT, false); // Bit of a fudge, as BATTLE_UNIT_COUNT happens to correspond to BATTLEUNIT_BYSTANDER
									}
#endif
									pkDamageEntry->SetDamage(iNukeDamage);
#if defined(MOD_UNITS_MAX_HP)
									pkDamageEntry->SetFinalDamage(std::min(iNukeDamage + pLoopUnit->getDamage(), pLoopUnit->GetMaxHitPoints()));
									pkDamageEntry->SetMaxHitPoints(pLoopUnit->GetMaxHitPoints());
#else
									pkDamageEntry->SetFinalDamage(std::min(iNukeDamage + pLoopUnit->getDamage(), GC.getMAX_HIT_POINTS()));
									pkDamageEntry->SetMaxHitPoints(GC.getMAX_HIT_POINTS());
#endif
									if (pkAttacker)
										pLoopUnit->setCombatUnit(pkAttacker);
								}
								else
								{
									CvAssertMsg(*piDamageMembers < iMaxDamageMembers, "Ran out of entries for the nuclear damage array");
								}
									}
								}
							}
						}

				if (pLoopCity != NULL)
				{
					bool bKillCity = false;

					// Is the city wiped out? - no capitals!
					if (!pLoopCity->IsOriginalCapital())
					{
						if (iDamageLevel > 2)
						{
							bKillCity = true;
						}
						else if (iDamageLevel > 1)
						{
							if (pLoopCity->getPopulation() < /*5*/ GC.getNUKE_LEVEL2_ELIM_POPULATION_THRESHOLD())
							{
								bKillCity = true;
							}
						}
					}

					int iTotalDamage;
					if (bKillCity)
					{
						iTotalDamage = pLoopCity->GetMaxHitPoints();
					}
					else
					{
						// Add damage to the city
						iTotalDamage = (pLoopCity->GetMaxHitPoints() - pLoopCity->getDamage()) * /*50*/ GC.getNUKE_CITY_HIT_POINT_DAMAGE();
						iTotalDamage /= 100;

						iTotalDamage += pLoopCity->getDamage();

						// Can't bring a city below 1 HP
						iTotalDamage = min(iTotalDamage, pLoopCity->GetMaxHitPoints() - 1);
					}

					CvCombatMemberEntry* pkDamageEntry = AddCombatMember(pkDamageArray, piDamageMembers, iMaxDamageMembers, pLoopCity);
					if (pkDamageEntry)
					{
#if defined(MOD_EVENTS_BATTLES)
						if (pLoopCity != pDefenderCity)
						{
							BATTLE_JOINED(pLoopCity, BATTLE_UNIT_COUNT, true); // Bit of a fudge, as BATTLE_UNIT_COUNT happens to correspond to BATTLEUNIT_BYSTANDER
						}
#endif
						pkDamageEntry->SetDamage(iTotalDamage - pLoopCity->getDamage());
						pkDamageEntry->SetFinalDamage(iTotalDamage);
						pkDamageEntry->SetMaxHitPoints(pLoopCity->GetMaxHitPoints());

						if (pkAttacker)
							pLoopCity->setCombatUnit(pkAttacker);
					}
					else
					{
						CvAssertMsg(*piDamageMembers < iMaxDamageMembers, "Ran out of entries for the nuclear damage array");
					}
				}
					}
				}
			}
		}

//	---------------------------------------------------------------------------
//	Function: ResolveNuclearCombat
//	Resolve combat from a nuclear attack.
void CvUnitCombat::ResolveNuclearCombat(const CvCombatInfo& kCombatInfo, uint uiParentEventID)
{
	UNREFERENCED_PARAMETER(uiParentEventID);

	CvUnit* pkAttacker = kCombatInfo.getUnit(BATTLE_UNIT_ATTACKER);
	CvAssert_Debug(pkAttacker);

	CvPlot* pkTargetPlot = kCombatInfo.getPlot();
	CvAssert_Debug(pkTargetPlot);

	CvString strBuffer;

	GC.getGame().changeNukesExploded(1);

	if (pkAttacker && !pkAttacker->isDelayedDeath())
	{
		// Make sure we are disconnected from any unit transporting the attacker (i.e. its a missile)
		pkAttacker->setTransportUnit(NULL);

		if (pkTargetPlot)
		{
			if (ApplyNuclearExplosionDamage(kCombatInfo.getDamageMembers(), kCombatInfo.getDamageMemberCount(), pkAttacker, pkTargetPlot, kCombatInfo.getAttackNuclearLevel() - 1) > 0)
			{
#if !defined(NO_ACHIEVEMENTS)
				if (pkAttacker->getOwner() == GC.getGame().getActivePlayer())
				{
					// Must damage someone to get the achievement.
					gDLL->UnlockAchievement(ACHIEVEMENT_DROP_NUKE);

					if (GC.getGame().getGameTurnYear() == 2012)
					{
						CvPlayerAI& kPlayer = GET_PLAYER(GC.getGame().getActivePlayer());
						if (strncmp(kPlayer.getCivilizationTypeKey(), "CIVILIZATION_MAYA", 32) == 0)
						{
							gDLL->UnlockAchievement(ACHIEVEMENT_XP1_36);
						}

					}

				}
#endif

#if defined(MOD_EVENTS_NUCLEAR_DETONATION)
				// While we should really send the NuclearDetonation event here, we don't still have all the info at this point
				// so we send it while calculating the combat info, just after we declare war (if appropriate) from firing one
#endif
			}
		}

		// Suicide Unit (currently all nuclear attackers are)
		if (pkAttacker->isSuicide())
		{
			pkAttacker->setCombatUnit(NULL);	// Must clear this if doing a delayed kill, should this be part of the kill method?
			pkAttacker->setAttackPlot(NULL, false);
			pkAttacker->kill(true);
		}
		else
		{
			CvAssertMsg(pkAttacker->isSuicide(), "A nuke unit that is not a one time use?");

			// Clean up some stuff
			pkAttacker->setCombatUnit(NULL);
			pkAttacker->ClearMissionQueue(GetPostCombatDelay());
			pkAttacker->SetAutomateType(NO_AUTOMATE); // kick unit out of automation

			// Spend a move for this attack
			pkAttacker->changeMoves(-GC.getMOVE_DENOMINATOR());

			// Can't move or attack again
			if (!pkAttacker->canMoveAfterAttacking())
			{
				pkAttacker->finishMoves();
			}
		}

		// Report that combat is over in case we want to queue another attack
		GET_PLAYER(pkAttacker->getOwner()).GetTacticalAI()->CombatResolved(pkAttacker, true);
	}

	BATTLE_FINISHED();
	DoNewBattleEffects(kCombatInfo);
}

#if defined(MOD_GLOBAL_PARATROOPS_AA_DAMAGE)
//	---------------------------------------------------------------------------
bool CvUnitCombat::ParadropIntercept(CvUnit& pParaUnit, CvPlot& pDropPlot) {
	CvAssertMsg(!pParaUnit.isDelayedDeath(), "Trying to paradrop and the unit is already dead!");
	CvAssert(pParaUnit.getCombatTimer() == 0);
	
	// Any interception to be done?
	CvUnit* pInterceptor = pParaUnit.GetBestInterceptor(pDropPlot, NULL);
	if (pInterceptor) {
		uint uiParentEventID = 0;
		int iInterceptionDamage = 0;

		// Is the interception successful?
		if(GC.getGame().getJonRandNum(100, "Intercept Rand (Paradrop)") < pInterceptor->currInterceptionProbability())
		{
			iInterceptionDamage = pInterceptor->GetParadropInterceptionDamage(&pParaUnit);
		}
	
		if (iInterceptionDamage > 0) {
#if defined(MOD_EVENTS_BATTLES)
			if (MOD_EVENTS_BATTLES) {
				BATTLE_STARTED(BATTLE_TYPE_PARADROP, pDropPlot);
				BATTLE_JOINED(&pParaUnit, BATTLE_UNIT_ATTACKER, false);
				BATTLE_JOINED(pInterceptor, BATTLE_UNIT_INTERCEPTOR, false);

				if (MOD_EVENTS_BATTLES_DAMAGE) {
					int iValue = 0;
					if (GAMEEVENTINVOKE_VALUE(iValue, GAMEEVENT_BattleDamageDelta, BATTLE_UNIT_INTERCEPTOR, iInterceptionDamage) == GAMEEVENTRETURN_VALUE) {
						if (iValue != 0) {
							if (iValue < 0) {
								// Decreasing the amount of damage, in which case it can't be more than the amount inflicted (as that's called 'healing'!)
								if (iInterceptionDamage + iValue < 0) {
									iValue = -iInterceptionDamage;
								}
							} else {
								// Increasing the amount of damage, in which case we can't exceed unit's hit points
								if (iInterceptionDamage + iValue > pParaUnit.GetCurrHitPoints()) {
									iValue = pParaUnit.GetCurrHitPoints() - iInterceptionDamage;
								}
							}
				
							iInterceptionDamage += iValue;
						}
					}
				}
			}
#endif

			CUSTOMLOG("Paradrop: hostile AA did %i damage", iInterceptionDamage);
			// Play the AA animations here ... but without an air attacker it's just not possible!!!
			// if (!CvPreGame::quickCombat()) {
				// Center camera here (pDropPlot MUST be visible or couldn't drop onto it!)
				// auto_ptr<ICvPlot1> pDllTargetPlot = GC.WrapPlotPointer(&pDropPlot);
				// GC.GetEngineUserInterface()->lookAt(pDllTargetPlot.get(), CAMERALOOKAT_NORMAL);
				// kCombatInfo.setVisualizeCombat(true);

				// auto_ptr<ICvCombatInfo1> pDllCombatInfo(new CvDllCombatInfo(&kCombatInfo));
				// uiParentEventID = gDLL->GameplayUnitCombat(pDllCombatInfo.get());

				// Set the combat units so that other missions do not continue until combat is over.
				// pInterceptor->setCombatUnit(&pParaUnit, false);
			// }

			pInterceptor->setMadeInterception(true);
			pInterceptor->setCombatUnit(NULL);

			// Killing the unit during the drop is a really bad idea, the game crashes at random after the drop
			int iHealth = pParaUnit.GetMaxHitPoints() - pParaUnit.GetCurrHitPoints();
#if defined(MOD_API_UNIT_STATS)
			pParaUnit.changeDamage(std::min(iHealth - 1, iInterceptionDamage), pInterceptor->getOwner(), pInterceptor->GetID());
#else
			pParaUnit.changeDamage(std::min(iHealth - 1, iInterceptionDamage), pInterceptor->getOwner());
#endif

			if (GC.getGame().getActivePlayer() == pParaUnit.getOwner()) {
				CvString strBuffer;
				if (pParaUnit.IsDead()) {
					strBuffer = GetLocalizedText("TXT_KEY_PARADROP_AA_KILLED", pParaUnit.getNameKey(), pInterceptor->getNameKey());
				} else {
					strBuffer = GetLocalizedText("TXT_KEY_PARADROP_AA_DAMAGED", pParaUnit.getNameKey(), pInterceptor->getNameKey(), iInterceptionDamage);
				}
				GC.GetEngineUserInterface()->AddMessage(uiParentEventID, pParaUnit.getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer);
				MILITARYLOG(pParaUnit.getOwner(), strBuffer.c_str(), pParaUnit.plot(), pInterceptor->getOwner());
			}
			
			BATTLE_FINISHED();
		}
	}
	
	return pParaUnit.IsDead();
}
#endif

//	---------------------------------------------------------------------------
void CvUnitCombat::ResolveCombat(const CvCombatInfo& kInfo, uint uiParentEventID /* = 0 */)
{
	PlayerTypes eAttackingPlayer = NO_PLAYER;
	// Restore visibility
	CvUnit* pAttacker = kInfo.getUnit(BATTLE_UNIT_ATTACKER);

	const TeamTypes eActiveTeam = GC.getGame().getActiveTeam();

	if(pAttacker)
	{
		eAttackingPlayer = pAttacker->getOwner();
		auto_ptr<ICvUnit1> pDllUnit(new CvDllUnit(pAttacker));
		gDLL->GameplayUnitVisibility(pDllUnit.get(), !pAttacker->isInvisible(eActiveTeam, false));
	}

	CvUnit* pDefender = kInfo.getUnit(BATTLE_UNIT_DEFENDER);
	if(pDefender)
	{
		auto_ptr<ICvUnit1> pDllUnit(new CvDllUnit(pDefender));
		gDLL->GameplayUnitVisibility(pDllUnit.get(), !pDefender->isInvisible(eActiveTeam, false));
	}

	CvUnit* pDefenderSupport = kInfo.getUnit(BATTLE_UNIT_INTERCEPTOR);
	if(pDefenderSupport)
	{
		auto_ptr<ICvUnit1> pDllUnit(new CvDllUnit(pDefenderSupport));
		gDLL->GameplayUnitVisibility(pDllUnit.get(), !pDefenderSupport->isInvisible(eActiveTeam, false));
	}

	// Nuclear Mission
	if(kInfo.getAttackIsNuclear())
	{
		ResolveNuclearCombat(kInfo, uiParentEventID);
	}

	// Bombing Mission
	else if(kInfo.getAttackIsBombingMission())
	{
		ResolveAirUnitVsCombat(kInfo, uiParentEventID);
	}

	// Air Sweep Mission
	else if(kInfo.getAttackIsAirSweep())
	{
		ResolveAirSweep(kInfo, uiParentEventID);
	}

	// Ranged Attack
	else if(kInfo.getAttackIsRanged())
	{
		if(kInfo.getUnit(BATTLE_UNIT_ATTACKER))
		{
			ResolveRangedUnitVsCombat(kInfo, uiParentEventID);
			CvPlot *pPlot = kInfo.getPlot();
			if (kInfo.getUnit(BATTLE_UNIT_ATTACKER) && kInfo.getUnit(BATTLE_UNIT_DEFENDER) && pPlot)
			{
				pPlot->AddArchaeologicalRecord(CvTypes::getARTIFACT_BATTLE_RANGED(), kInfo.getUnit(BATTLE_UNIT_ATTACKER)->getOwner(), kInfo.getUnit(BATTLE_UNIT_DEFENDER)->getOwner());
			}
		}
		else
		{
			ResolveRangedCityVsUnitCombat(kInfo, uiParentEventID);
			CvPlot *pPlot = kInfo.getPlot();
			if (kInfo.getCity(BATTLE_UNIT_ATTACKER) && kInfo.getUnit(BATTLE_UNIT_DEFENDER) && pPlot)
			{
				pPlot->AddArchaeologicalRecord(CvTypes::getARTIFACT_BATTLE_RANGED(), kInfo.getCity(BATTLE_UNIT_ATTACKER)->getOwner(), kInfo.getUnit(BATTLE_UNIT_DEFENDER)->getOwner());
			}
		}
	}

	// Melee Attack
	else
	{
		if(kInfo.getCity(BATTLE_UNIT_DEFENDER))
		{
			ResolveCityMeleeCombat(kInfo, uiParentEventID);
		}
		else
		{
			ResolveMeleeCombat(kInfo, uiParentEventID);
			CvPlot *pPlot = kInfo.getPlot();
			if (kInfo.getUnit(BATTLE_UNIT_ATTACKER) && kInfo.getUnit(BATTLE_UNIT_DEFENDER) && pPlot)
			{
				pPlot->AddArchaeologicalRecord(CvTypes::getARTIFACT_BATTLE_MELEE(), kInfo.getUnit(BATTLE_UNIT_ATTACKER)->getOwner(), kInfo.getUnit(BATTLE_UNIT_DEFENDER)->getOwner());
			}
		}
	}

	// Clear popup blocking after combat resolves
	if(eAttackingPlayer == GC.getGame().getActivePlayer())
	{
		GC.GetEngineUserInterface()->SetDontShowPopups(false);
	}
}

//	----------------------------------------------------------------------------
CvUnitCombat::ATTACK_RESULT CvUnitCombat::Attack(CvUnit& kAttacker, CvPlot& targetPlot, ATTACK_OPTION eOption)
{
	CvString strBuffer;

	//VALIDATE_OBJECT
	CvAssert(kAttacker.canMoveInto(targetPlot, CvUnit::MOVEFLAG_ATTACK | CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE));
	CvAssert(kAttacker.getCombatTimer() == 0);

	CvUnitCombat::ATTACK_RESULT eResult = CvUnitCombat::ATTACK_ABORTED;

	CvAssert(kAttacker.getCombatTimer() == 0);
	//	CvAssert(pDefender != NULL);
	CvAssert(!kAttacker.isFighting());

	// Unit that attacks loses his Fort bonus
	kAttacker.setFortifyTurns(0);

	UnitHandle pDefender;
	pDefender = targetPlot.getBestDefender(NO_PLAYER, kAttacker.getOwner(), &kAttacker, true);

	// JAR - without pDefender, nothing in here is going to work, just crash
	if(!pDefender)
	{
		return eResult;
	}

	kAttacker.SetAutomateType(NO_AUTOMATE);
	pDefender->SetAutomateType(NO_AUTOMATE);
#if defined(MOD_BUGFIX_UNITS_AWAKE_IN_DANGER)
	// We want to "wake up" a unit that is being attacked by a hidden enemy (probably being bombed, indirect naval fire,
	// or sneaky long-range archers) so the player can consider what to do with them, but without removing any fortification bonus!
	if (MOD_BUGFIX_UNITS_AWAKE_IN_DANGER) {
		pDefender->SetActivityType(ACTIVITY_AWAKE, false); 
	}
#endif

#if !defined(NO_TUTORIALS)
	// slewis - tutorial'd
	if(kAttacker.getOwner() == GC.getGame().getActivePlayer())
	{
		GC.getGame().SetEverAttackedTutorial(true);
	}
	// end tutorial'd
#endif

	// handle the Zulu special thrown spear first attack
	ATTACK_RESULT eFireSupportResult = ATTACK_ABORTED;
	if (kAttacker.isRangedSupportFire() && pDefender->IsCanDefend())
	{
		eFireSupportResult = AttackRanged(kAttacker, pDefender->getX(), pDefender->getY(), CvUnitCombat::ATTACK_OPTION_NO_DEFENSIVE_SUPPORT);
		if (pDefender->isDelayedDeath())
		{
			// Killed him, move to the plot if we can.
			if(targetPlot.getNumVisibleEnemyDefenders(&kAttacker) == 0)
			{
				if (kAttacker.UnitMove(&targetPlot, true, &kAttacker, true))
					kAttacker.finishMoves();	// Burn all the moves we have
			}
			return eFireSupportResult;
		}
	}

	CvCombatInfo kCombatInfo;
	GenerateMeleeCombatInfo(kAttacker, pDefender.pointer(), targetPlot, &kCombatInfo);

	CvAssertMsg(!kAttacker.isDelayedDeath() && !pDefender->isDelayedDeath(), "Trying to battle and one of the units is already dead!");

	if(pDefender->getExtraWithdrawal() > 0 && pDefender->CanWithdrawFromMelee(kAttacker))
	{
		pDefender->DoWithdrawFromMelee(kAttacker);

		if(kAttacker.getOwner() == GC.getGame().getActivePlayer())
		{
			strBuffer = GetLocalizedText("TXT_KEY_MISC_ENEMY_UNIT_WITHDREW", pDefender->getNameKey());
			GC.GetEngineUserInterface()->AddMessage(0, kAttacker.getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer);
			MILITARYLOG(kAttacker.getOwner(), strBuffer.c_str(), kAttacker.plot(), pDefender->getOwner());
		}
		else if(pDefender->getOwner() == GC.getGame().getActivePlayer())
		{
			strBuffer = GetLocalizedText("TXT_KEY_MISC_FRIENDLY_UNIT_WITHDREW", pDefender->getNameKey());
			GC.GetEngineUserInterface()->AddMessage(0, pDefender->getOwner(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer);
			MILITARYLOG(pDefender->getOwner(), strBuffer.c_str(), pDefender->plot(), kAttacker.getOwner());
		}

		// Move forward
		if(targetPlot.getNumVisibleEnemyDefenders(&kAttacker) == 0)
		{
			kAttacker.UnitMove(&targetPlot, true, &kAttacker);
		}

//		kAttacker.setMadeAttack(true);   /* EFB: Doesn't work, causes tactical AI to not dequeue this attack; but we've decided you don't lose your attack anyway */
		eResult = ATTACK_COMPLETED;
	}

	else if(!pDefender->IsCanDefend())
	{
		CvMissionDefinition kMission;
		kMission.setMissionTime(kAttacker.getCombatTimer() * gDLL->getSecsPerTurn());
		kMission.setMissionType(CvTypes::getMISSION_SURRENDER());
		kMission.setUnit(BATTLE_UNIT_ATTACKER, &kAttacker);
		kMission.setUnit(BATTLE_UNIT_DEFENDER, pDefender.pointer());
		kMission.setPlot(&targetPlot);

		// Surrender mission
		CvMissionInfo* pkSurrenderMission = GC.getMissionInfo(CvTypes::getMISSION_SURRENDER());
		if(pkSurrenderMission == NULL)
		{
			CvAssert(false);
		}
		else
		{
			kAttacker.setCombatTimer(pkSurrenderMission->getTime());
		}

		// Kill them!
#if defined(MOD_UNITS_MAX_HP)
		pDefender->setDamage(pDefender->GetMaxHitPoints());
#else
		pDefender->setDamage(GC.getMAX_HIT_POINTS());
#endif

		Localization::String strMessage;
		Localization::String strSummary;

		// Some units can't capture civilians. Embarked units are also not captured, they're simply killed. And some aren't a type that gets captured.
		if(!kAttacker.isNoCapture() && (!pDefender->isEmbarked() || pDefender->getUnitInfo().IsCaptureWhileEmbarked()) && pDefender->getCaptureUnitType(GET_PLAYER(pDefender->getOwner()).getCivilizationType()) != NO_UNIT)
		{
			pDefender->setCapturingPlayer(kAttacker.getOwner());
#ifdef MOD_BATTLE_CAPTURE_NEW_RULE
			pDefender->setCapturingUnit(&kAttacker);
#endif

			if(kAttacker.isBarbarian())
			{
				strMessage = Localization::Lookup("TXT_KEY_UNIT_CAPTURED_BARBS_DETAILED");
				strMessage << pDefender->getUnitInfo().GetTextKey() << GET_PLAYER(kAttacker.getOwner()).getNameKey();
				strSummary = Localization::Lookup("TXT_KEY_UNIT_CAPTURED_BARBS");
			}
			else
			{
				strMessage = Localization::Lookup("TXT_KEY_UNIT_CAPTURED_DETAILED");
				strMessage << pDefender->getUnitInfo().GetTextKey();
				strSummary = Localization::Lookup("TXT_KEY_UNIT_CAPTURED");
			}
		}
		// Unit was killed instead
		else
		{
			strMessage = Localization::Lookup("TXT_KEY_UNIT_LOST");
			strSummary = strMessage;
		}

		CvNotifications* pNotification = GET_PLAYER(pDefender->getOwner()).GetNotifications();
		if(pNotification)
			pNotification->Add(NOTIFICATION_UNIT_DIED, strMessage.toUTF8(), strSummary.toUTF8(), pDefender->getX(), pDefender->getY(), (int) pDefender->getUnitType(), pDefender->getOwner());

		bool bAdvance;
		bAdvance = kAttacker.canAdvance(targetPlot, ((pDefender->IsCanDefend()) ? 1 : 0));

		// Move forward
		if(targetPlot.getNumVisibleEnemyDefenders(&kAttacker) == 0)
		{
			kAttacker.UnitMove(&targetPlot, true, ((bAdvance) ? &kAttacker : NULL));
		}

		// KWG: Should this be called? The defender is killed above and the unit.
		//      If anything, the above code should be put in the ResolveCombat method.
		ResolveCombat(kCombatInfo);
		eResult = ATTACK_COMPLETED;
	}
	else
	{
		ATTACK_RESULT eSupportResult = ATTACK_ABORTED;
		if(eOption != ATTACK_OPTION_NO_DEFENSIVE_SUPPORT)
		{
			// Ranged fire support from artillery units
			CvUnit* pFireSupportUnit = GetFireSupportUnit(pDefender->getOwner(), pDefender->getX(), pDefender->getY(), kAttacker.getX(), kAttacker.getY());
			if(pFireSupportUnit != NULL)
			{
				CvAssertMsg(!pFireSupportUnit->isDelayedDeath(), "Supporting battle unit is already dead!");
				eSupportResult = AttackRanged(*pFireSupportUnit, kAttacker.getX(), kAttacker.getY(), CvUnitCombat::ATTACK_OPTION_NO_DEFENSIVE_SUPPORT);
				// Turn off Fortify Turns, as this is the trigger for whether or not a ranged Unit can provide support fire (in addition to hasMadeAttack)
				pFireSupportUnit->setFortifyTurns(0);
			}

			if(eSupportResult == ATTACK_QUEUED)
			{
				// The supporting unit has queued their attack (against the attacker), we must have the attacker queue its attack.
				// Also, flag the current mission that the next time through, the defender doesn't get any defensive support.
				const_cast<MissionData*>(kAttacker.GetHeadMissionData())->iFlags |= MISSION_MODIFIER_NO_DEFENSIVE_SUPPORT;
				CvUnitMission::WaitFor(&kAttacker, pFireSupportUnit);
				eResult = ATTACK_QUEUED;
			}
		}

		if(eResult != ATTACK_QUEUED)
		{
			kAttacker.setMadeAttack(true);

			uint uiParentEventID = 0;
			// Send the combat message if the target plot is visible.
			bool isTargetVisibleToActivePlayer = targetPlot.isActiveVisible(false);
			bool quickCombat = CvPreGame::quickCombat();
			if(!quickCombat)
			{
				// Center camera here!
				if(isTargetVisibleToActivePlayer)
				{
					auto_ptr<ICvPlot1> pDefenderPlot = GC.WrapPlotPointer(pDefender->plot());
					GC.GetEngineUserInterface()->lookAt(pDefenderPlot.get(), CAMERALOOKAT_NORMAL);
				}
				kCombatInfo.setVisualizeCombat(isTargetVisibleToActivePlayer);

				auto_ptr<ICvCombatInfo1> pDllCombatInfo(new CvDllCombatInfo(&kCombatInfo));
				uiParentEventID = gDLL->GameplayUnitCombat(pDllCombatInfo.get());

				// Set the combat units so that other missions do not continue until combat is over.
				kAttacker.setCombatUnit(pDefender.pointer(), true);
				pDefender->setCombatUnit(&kAttacker, false);

				eResult = ATTACK_QUEUED;
			}
			else
				eResult = ATTACK_COMPLETED;

			// Resolve combat here.
			ResolveCombat(kCombatInfo, uiParentEventID);

		}
	}

	return eResult;
}

//	---------------------------------------------------------------------------
CvUnitCombat::ATTACK_RESULT CvUnitCombat::AttackRanged(CvUnit& kAttacker, int iX, int iY, CvUnitCombat::ATTACK_OPTION /* eOption */)
{
	//VALIDATE_OBJECT
	CvPlot* pPlot = GC.getMap().plot(iX, iY);
	ATTACK_RESULT eResult = ATTACK_ABORTED;

	CvAssertMsg(kAttacker.getDomainType() != DOMAIN_AIR, "Air units should not use AttackRanged, they should just MoveTo the target");

	if(NULL == pPlot)
	{
		return eResult;
	}

	if (!kAttacker.isRangedSupportFire())
	{
		if(!kAttacker.canRangeStrikeAt(iX, iY))
		{
			return eResult;
		}

		if(GC.getRANGED_ATTACKS_USE_MOVES() == 0)
		{
			kAttacker.setMadeAttack(true);
		}
		kAttacker.changeMoves(-GC.getMOVE_DENOMINATOR());
	}

	// Unit that attacks loses his Fort bonus
	kAttacker.setFortifyTurns(0);

	// New test feature - attacking/range striking uses up all moves for most Units
	if(!kAttacker.canMoveAfterAttacking() && !kAttacker.isRangedSupportFire())
	{
		kAttacker.finishMoves();
		GC.GetEngineUserInterface()->changeCycleSelectionCounter(1);
	}

	kAttacker.SetAutomateType(NO_AUTOMATE);

	bool bDoImmediate = CvPreGame::quickCombat();
	// Range-striking a Unit
	if(!pPlot->isCity())
	{
		CvUnit* pDefender = kAttacker.airStrikeTarget(*pPlot, true);

#if defined(MOD_EVENTS_UNIT_RANGEATTACK)
		if (!pDefender) {
			if (MOD_EVENTS_UNIT_RANGEATTACK) {
				int iValue = 0;
				if (GAMEEVENTINVOKE_VALUE(iValue, GAMEEVENT_UnitRangeAttackAt, kAttacker.getOwner(), kAttacker.GetID(), iX, iY) == GAMEEVENTRETURN_VALUE) {
					if (iValue) {
						return CvUnitCombat::ATTACK_COMPLETED;
					}

					return CvUnitCombat::ATTACK_ABORTED;
				}
			}
		}
#endif

		CvAssert(pDefender != NULL);
		if(!pDefender) return ATTACK_ABORTED;

		pDefender->SetAutomateType(NO_AUTOMATE);
#if defined(MOD_BUGFIX_UNITS_AWAKE_IN_DANGER)
		if (MOD_BUGFIX_UNITS_AWAKE_IN_DANGER) {
			pDefender->SetActivityType(ACTIVITY_AWAKE, false); 
		}
#endif

		CvCombatInfo kCombatInfo;
		CvUnitCombat::GenerateRangedCombatInfo(kAttacker, pDefender, *pPlot, &kCombatInfo);
		CvAssertMsg(!kAttacker.isDelayedDeath() && !pDefender->isDelayedDeath(), "Trying to battle and one of the units is already dead!");

		uint uiParentEventID = 0;
		if(!bDoImmediate)
		{
			// Center camera here!
			bool isTargetVisibleToActivePlayer = pPlot->isActiveVisible(false);
			if(isTargetVisibleToActivePlayer)
			{
				auto_ptr<ICvPlot1> pDllPlot = GC.WrapPlotPointer(pPlot);
				GC.GetEngineUserInterface()->lookAt(pDllPlot.get(), CAMERALOOKAT_NORMAL);
			}
			kCombatInfo.setVisualizeCombat(isTargetVisibleToActivePlayer);

			auto_ptr<ICvCombatInfo1> pDllCombatInfo(new CvDllCombatInfo(&kCombatInfo));
			uiParentEventID = gDLL->GameplayUnitCombat(pDllCombatInfo.get());

			// Set the combat units so that other missions do not continue until combat is over.
			kAttacker.setCombatUnit(pDefender, true);
			pDefender->setCombatUnit(&kAttacker, false);
			eResult = ATTACK_QUEUED;
		}
		else
			eResult = ATTACK_COMPLETED;

		ResolveCombat(kCombatInfo, uiParentEventID);
	}
	// Range-striking a City
	else
	{
		if (kAttacker.isRangedSupportFire())
		{
			return eResult;
		}

		CvCombatInfo kCombatInfo;
		GenerateRangedCombatInfo(kAttacker, NULL, *pPlot, &kCombatInfo);
		CvAssertMsg(!kAttacker.isDelayedDeath(), "Trying to battle and the attacker is already dead!");

		uint uiParentEventID = 0;
		if(!bDoImmediate)
		{
			// Center camera here!
			bool isTargetVisibleToActivePlayer = pPlot->isActiveVisible(false);
			if(isTargetVisibleToActivePlayer)
			{
				auto_ptr<ICvPlot1> pDllPlot = GC.WrapPlotPointer(pPlot);
				GC.GetEngineUserInterface()->lookAt(pDllPlot.get(), CAMERALOOKAT_NORMAL);
			}

			kCombatInfo.setVisualizeCombat(isTargetVisibleToActivePlayer);

			auto_ptr<ICvCombatInfo1> pDllCombatInfo(new CvDllCombatInfo(&kCombatInfo));
			uiParentEventID = gDLL->GameplayCityCombat(pDllCombatInfo.get());

			CvCity* pkDefender = pPlot->getPlotCity();
			kAttacker.setCombatCity(pkDefender);
			if(pkDefender)
				pkDefender->setCombatUnit(&kAttacker);
			eResult = ATTACK_QUEUED;
		}
		else
			eResult = ATTACK_COMPLETED;

		ResolveCombat(kCombatInfo, uiParentEventID);
	}

	return eResult;
}

//	----------------------------------------------------------------------------
CvUnitCombat::ATTACK_RESULT CvUnitCombat::AttackAir(CvUnit& kAttacker, CvPlot& targetPlot, ATTACK_OPTION /* eOption */)
{
	//VALIDATE_OBJECT
	CvAssert(kAttacker.getCombatTimer() == 0);

	CvUnitCombat::ATTACK_RESULT eResult = CvUnitCombat::ATTACK_ABORTED;

	// Can we actually hit the target?
	if(!kAttacker.canRangeStrikeAt(targetPlot.getX(), targetPlot.getY()))
	{
		return eResult;
	}

	bool bDoImmediate = CvPreGame::quickCombat();
	kAttacker.SetAutomateType(NO_AUTOMATE);
	kAttacker.setMadeAttack(true);

	// Bombing a Unit
	if(!targetPlot.isCity())
	{
		CvUnit* pDefender = kAttacker.airStrikeTarget(targetPlot, true);

#if defined(MOD_EVENTS_UNIT_RANGEATTACK)
		if (!pDefender) {
			if (MOD_EVENTS_UNIT_RANGEATTACK) {
				int iValue = 0;
				if (GAMEEVENTINVOKE_VALUE(iValue, GAMEEVENT_UnitRangeAttackAt, kAttacker.getOwner(), kAttacker.GetID(), targetPlot.getX(), targetPlot.getY()) == GAMEEVENTRETURN_VALUE) {
					if (iValue) {
						return CvUnitCombat::ATTACK_COMPLETED;
					}

					return CvUnitCombat::ATTACK_ABORTED;
				}
			}
		}
#endif

		CvAssert(pDefender != NULL);
		if(!pDefender) return CvUnitCombat::ATTACK_ABORTED;

		pDefender->SetAutomateType(NO_AUTOMATE);
#if defined(MOD_BUGFIX_UNITS_AWAKE_IN_DANGER)
		if (MOD_BUGFIX_UNITS_AWAKE_IN_DANGER) {
			pDefender->SetActivityType(ACTIVITY_AWAKE, false); 
		}
#endif

		CvCombatInfo kCombatInfo;
		CvUnitCombat::GenerateAirCombatInfo(kAttacker, pDefender, targetPlot, &kCombatInfo);
		CvAssertMsg(!kAttacker.isDelayedDeath() && !pDefender->isDelayedDeath(), "Trying to battle and one of the units is already dead!");

		uint uiParentEventID = 0;
		if(!bDoImmediate)
		{
			// Center camera here!
			bool isTargetVisibleToActivePlayer = targetPlot.isActiveVisible(false);
			if(isTargetVisibleToActivePlayer)
			{
				auto_ptr<ICvPlot1> pDllTargetPlot = GC.WrapPlotPointer(&targetPlot);
				GC.GetEngineUserInterface()->lookAt(pDllTargetPlot.get(), CAMERALOOKAT_NORMAL);
			}
			kCombatInfo.setVisualizeCombat(isTargetVisibleToActivePlayer);

			auto_ptr<ICvCombatInfo1> pDllCombatInfo(new CvDllCombatInfo(&kCombatInfo));
			uiParentEventID = gDLL->GameplayUnitCombat(pDllCombatInfo.get());

			// Set the combat units so that other missions do not continue until combat is over.
			kAttacker.setCombatUnit(pDefender, true);
			pDefender->setCombatUnit(&kAttacker, false);
			CvUnit* pDefenderSupport = kCombatInfo.getUnit(BATTLE_UNIT_INTERCEPTOR);
			if(pDefenderSupport)
				pDefenderSupport->setCombatUnit(&kAttacker, false);

			eResult = ATTACK_QUEUED;
		}
		else
			eResult = ATTACK_COMPLETED;

		ResolveCombat(kCombatInfo, uiParentEventID);
	}
	// Bombing a City
	else
	{
		CvCombatInfo kCombatInfo;
		GenerateAirCombatInfo(kAttacker, NULL, targetPlot, &kCombatInfo);
		CvAssertMsg(!kAttacker.isDelayedDeath(), "Trying to battle and the attacker is already dead!");

		uint uiParentEventID = 0;
		if(!bDoImmediate)
		{
			// Center camera here!
			bool isTargetVisibleToActivePlayer = targetPlot.isActiveVisible(false);
			if(isTargetVisibleToActivePlayer)
			{
				auto_ptr<ICvPlot1> pDllTargetPlot = GC.WrapPlotPointer(&targetPlot);
				GC.GetEngineUserInterface()->lookAt(pDllTargetPlot.get(), CAMERALOOKAT_NORMAL);
			}

			kCombatInfo.setVisualizeCombat(isTargetVisibleToActivePlayer);
			auto_ptr<ICvCombatInfo1> pDllCombatInfo(new CvDllCombatInfo(&kCombatInfo));
			uiParentEventID = gDLL->GameplayCityCombat(pDllCombatInfo.get());

			CvCity* pkDefender = targetPlot.getPlotCity();
			kAttacker.setCombatCity(pkDefender);
			if(pkDefender)
				pkDefender->setCombatUnit(&kAttacker);
			CvUnit* pDefenderSupport = kCombatInfo.getUnit(BATTLE_UNIT_INTERCEPTOR);
			if(pDefenderSupport)
				pDefenderSupport->setCombatUnit(&kAttacker, false);
			eResult = ATTACK_QUEUED;
		}
		else
			eResult = ATTACK_COMPLETED;

		ResolveCombat(kCombatInfo, uiParentEventID);
	}

	return eResult;
}

//	----------------------------------------------------------------------------
CvUnitCombat::ATTACK_RESULT CvUnitCombat::AttackAirSweep(CvUnit& kAttacker, CvPlot& targetPlot, ATTACK_OPTION /* eOption */)
{
	//VALIDATE_OBJECT
	CvAssert(kAttacker.getCombatTimer() == 0);

	CvUnitCombat::ATTACK_RESULT eResult = CvUnitCombat::ATTACK_ABORTED;

	// Can we actually hit the target?
	if(!kAttacker.canAirSweepAt(targetPlot.getX(), targetPlot.getY()))
	{
		return eResult;
	}

	CvUnit* pInterceptor = kAttacker.GetBestInterceptor(targetPlot);
	kAttacker.SetAutomateType(NO_AUTOMATE);

	// Any interceptor to sweep for?
	if(pInterceptor != NULL)
	{
		// CUSTOMLOG("AttackAirSweep: At (%i, %i) Attacker: %i-%i Interceptor: %i-%i", targetPlot.getX(), targetPlot.getY(), kAttacker.getOwner(), kAttacker.GetID(), pInterceptor->getOwner(), pInterceptor->GetID());
		kAttacker.setMadeAttack(true);
		CvCombatInfo kCombatInfo;
		CvUnitCombat::GenerateAirSweepCombatInfo(kAttacker, pInterceptor, targetPlot, &kCombatInfo);
		CvUnit* pkDefender = kCombatInfo.getUnit(BATTLE_UNIT_DEFENDER);
		pkDefender->SetAutomateType(NO_AUTOMATE);
#if defined(MOD_BUGFIX_UNITS_AWAKE_IN_DANGER)
		if (MOD_BUGFIX_UNITS_AWAKE_IN_DANGER) {
			pkDefender->SetActivityType(ACTIVITY_AWAKE, false); 
		}
#endif
		CvAssertMsg(!kAttacker.isDelayedDeath() && !pkDefender->isDelayedDeath(), "Trying to battle and one of the units is already dead!");

		uint uiParentEventID = 0;
		bool bDoImmediate = CvPreGame::quickCombat();
		if(!bDoImmediate)
		{
			// Center camera here!
			bool isTargetVisibleToActivePlayer = targetPlot.isActiveVisible(false);
			if(isTargetVisibleToActivePlayer)
			{
				auto_ptr<ICvPlot1> pDllTargetPlot = GC.WrapPlotPointer(&targetPlot);
				GC.GetEngineUserInterface()->lookAt(pDllTargetPlot.get(), CAMERALOOKAT_NORMAL);
			}
			kCombatInfo.setVisualizeCombat(isTargetVisibleToActivePlayer);

			auto_ptr<ICvCombatInfo1> pDllCombatInfo(new CvDllCombatInfo(&kCombatInfo));
			uiParentEventID = gDLL->GameplayUnitCombat(pDllCombatInfo.get());

			// Set the combat units so that other missions do not continue until combat is over.
			kAttacker.setCombatUnit(pInterceptor, true);
			pInterceptor->setCombatUnit(&kAttacker, false);
			eResult = ATTACK_QUEUED;
		}
		else
			eResult = ATTACK_COMPLETED;

		ResolveCombat(kCombatInfo, uiParentEventID);
	}
	else
	{
		// attempted to do a sweep in a plot that had no interceptors
		// consume the movement and finish its moves
		if(kAttacker.getOwner() == GC.getGame().getActivePlayer())
		{
			Localization::String localizedText = Localization::Lookup("TXT_KEY_AIR_PATROL_FOUND_NOTHING");
			localizedText << kAttacker.getUnitInfo().GetTextKey();
			GC.GetEngineUserInterface()->AddMessage(0, kAttacker.getOwner(), false, GC.getEVENT_MESSAGE_TIME(), localizedText.toUTF8());
			MILITARYLOG(kAttacker.getOwner(), localizedText.toUTF8(), kAttacker.plot(), kAttacker.getOwner());
		}

		// Spend a move for this attack
		kAttacker.changeMoves(-GC.getMOVE_DENOMINATOR());

		// Can't move or attack again
		if(!kAttacker.canMoveAfterAttacking())
		{
			kAttacker.finishMoves();
		}
	}

	return eResult;
}

//	---------------------------------------------------------------------------
CvUnitCombat::ATTACK_RESULT CvUnitCombat::AttackCity(CvUnit& kAttacker, CvPlot& plot, CvUnitCombat::ATTACK_OPTION eOption)
{
	//VALIDATE_OBJECT

	ATTACK_RESULT eResult = ATTACK_ABORTED;
	CvCity* pCity = plot.getPlotCity();
	CvAssertMsg(pCity != NULL, "If this unit is attacking a NULL city then something funky is goin' down");
	if(!pCity) return eResult;

	kAttacker.SetAutomateType(NO_AUTOMATE);

	if(eOption != ATTACK_OPTION_NO_DEFENSIVE_SUPPORT)
	{
		// See if the city has some supporting fire to fend off the attacker
		CvUnit* pFireSupportUnit = GetFireSupportUnit(pCity->getOwner(), pCity->getX(), pCity->getY(), kAttacker.getX(), kAttacker.getY());

		ATTACK_RESULT eSupportResult = ATTACK_ABORTED;
		if(pFireSupportUnit)
		{
			eSupportResult = AttackRanged(*pFireSupportUnit, kAttacker.getX(), kAttacker.getY(), CvUnitCombat::ATTACK_OPTION_NO_DEFENSIVE_SUPPORT);
			// Turn off Fortify Turns, as this is the trigger for whether or not a ranged Unit can provide support fire (in addition to hasMadeAttack)
			pFireSupportUnit->setFortifyTurns(0);
		}

		if(eSupportResult == ATTACK_QUEUED)
		{
			// The supporting unit has queued their attack (against the attacker), we must have the attacker queue its attack.
			// Also, flag the current mission that the next time through, the defender doesn't get any defensive support.
			const_cast<MissionData*>(kAttacker.GetHeadMissionData())->iFlags |= MISSION_MODIFIER_NO_DEFENSIVE_SUPPORT;
			CvUnitMission::WaitFor(&kAttacker, pFireSupportUnit);
			eResult = ATTACK_QUEUED;
		}
	}

	if(eResult != ATTACK_QUEUED)
	{
		kAttacker.setMadeAttack(true);

		// We are doing a non-ranged attack on a city
		CvCombatInfo kCombatInfo;
		GenerateMeleeCombatInfo(kAttacker, NULL, plot, &kCombatInfo);
		CvAssertMsg(!kAttacker.isDelayedDeath(), "Trying to battle and the attacker is already dead!");

		// Send the combat message if the target plot is visible.
		bool isTargetVisibleToActivePlayer = plot.isActiveVisible(false);

		uint uiParentEventID = 0;
		bool bDoImmediate = CvPreGame::quickCombat();
		if(!bDoImmediate)
		{
			// Center camera here!
			if(isTargetVisibleToActivePlayer)
			{
				auto_ptr<ICvPlot1> pDllPlot = GC.WrapPlotPointer(&plot);
				GC.GetEngineUserInterface()->lookAt(pDllPlot.get(), CAMERALOOKAT_NORMAL);
			}
			kCombatInfo.setVisualizeCombat(isTargetVisibleToActivePlayer);

			auto_ptr<ICvCombatInfo1> pDllCombatInfo(new CvDllCombatInfo(&kCombatInfo));
			uiParentEventID = gDLL->GameplayCityCombat(pDllCombatInfo.get());

			CvCity* pkDefender = plot.getPlotCity();
			kAttacker.setCombatCity(pkDefender);
			if(pkDefender)
				pkDefender->setCombatUnit(&kAttacker);
			eResult = ATTACK_QUEUED;
		}
		else
			eResult = ATTACK_COMPLETED;

		ResolveCombat(kCombatInfo, uiParentEventID);
	}
	return eResult;
}

//	-------------------------------------------------------------------------------------------
//	Return a ranged unit that will defend the supplied location against the attacker at the specified location.
CvUnit* CvUnitCombat::GetFireSupportUnit(PlayerTypes eDefender, int iDefendX, int iDefendY, int iAttackX, int iAttackY)
{
	VALIDATE_OBJECT

	if(GC.getFIRE_SUPPORT_DISABLED() == 1)
		return NULL;

	CvPlot* pAdjacentPlot = NULL;
	CvPlot* pPlot = GC.getMap().plot(iDefendX, iDefendY);

	for(int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		pAdjacentPlot = plotDirection(pPlot->getX(), pPlot->getY(), ((DirectionTypes)iI));

		if(pAdjacentPlot != NULL)
		{
			for(int iUnitLoop = 0; iUnitLoop < pAdjacentPlot->getNumUnits(); iUnitLoop++)
			{
				CvUnit* pLoopUnit = pAdjacentPlot->getUnitByIndex(iUnitLoop);

				// Unit owned by same player?
				if(pLoopUnit->getOwner() == eDefender)
				{
					// Can this unit perform a ranged strike on the attacker's plot?
					if(pLoopUnit->canRangeStrikeAt(iAttackX, iAttackY))
					{
						// Range strike would be calculated here, so get the estimated damage
						return pLoopUnit;
					}
				}
			}
		}
	}

	return NULL;
}

//	----------------------------------------------------------------------------
CvUnitCombat::ATTACK_RESULT CvUnitCombat::AttackNuclear(CvUnit& kAttacker, int iX, int iY, ATTACK_OPTION /* eOption */)
{
	ATTACK_RESULT eResult = ATTACK_ABORTED;

	CvPlot* pPlot = GC.getMap().plot(iX, iY);
	if(NULL == pPlot)
		return eResult;

	bool bDoImmediate = CvPreGame::quickCombat();
	CvCombatInfo kCombatInfo;
	CvUnitCombat::GenerateNuclearCombatInfo(kAttacker, *pPlot, &kCombatInfo);
	CvAssertMsg(!kAttacker.isDelayedDeath(), "Trying to battle and the attacker is already dead!");
	kAttacker.setMadeAttack(true);
	uint uiParentEventID = 0;
	if(!bDoImmediate)
	{
		// Nuclear attacks are different in that you can target a plot you can't see, so check to see if the active player
		// is involved in the combat
		TeamTypes eActiveTeam = GC.getGame().getActiveTeam();

		bool isTargetVisibleToActivePlayer = pPlot->isActiveVisible(false);
		if(!isTargetVisibleToActivePlayer)
		{
			// Is the attacker part of the local team?
			isTargetVisibleToActivePlayer = (kAttacker.getTeam() != NO_TEAM && eActiveTeam == kAttacker.getTeam());

			if(!isTargetVisibleToActivePlayer)
			{
				// Are any of the teams effected by the blast in the local team?
				for(int i = 0; i < MAX_TEAMS && !isTargetVisibleToActivePlayer; ++i)
				{
					if(kAttacker.isNukeVictim(pPlot, ((TeamTypes)i)))
					{
						isTargetVisibleToActivePlayer = eActiveTeam == ((TeamTypes)i);
					}
				}
			}
		}

		if(isTargetVisibleToActivePlayer)
		{
			auto_ptr<ICvPlot1> pDllPlot = GC.WrapPlotPointer(pPlot);
			GC.GetEngineUserInterface()->lookAt(pDllPlot.get(), CAMERALOOKAT_NORMAL);
		}
		kCombatInfo.setVisualizeCombat(isTargetVisibleToActivePlayer);

		// Set a combat unit/city.  Not really needed for the combat since we are killing everyone, but it is currently the only way a unit is marked that it is 'in-combat'
		if(pPlot->getPlotCity())
			kAttacker.setCombatCity(pPlot->getPlotCity());
		else
		{
			if(pPlot->getNumUnits())
				kAttacker.setCombatUnit(pPlot->getUnitByIndex(0), true);
			else
				kAttacker.setAttackPlot(pPlot, false);
		}

		auto_ptr<ICvCombatInfo1> pDllCombatInfo(new CvDllCombatInfo(&kCombatInfo));
		uiParentEventID = gDLL->GameplayUnitCombat(pDllCombatInfo.get());

		eResult = ATTACK_QUEUED;
	}
	else
	{
		eResult = ATTACK_COMPLETED;
		// Set the plot, just so the unit is marked as 'in-combat'
		kAttacker.setAttackPlot(pPlot, false);
	}

	ResolveCombat(kCombatInfo,  uiParentEventID);

	return eResult;
}

//	---------------------------------------------------------------------------
void CvUnitCombat::ApplyPostCombatTraitEffects(CvUnit* pkWinner, CvUnit* pkLoser)
{
	int iExistingDelay = 0;

	CvPlayer& kPlayer = GET_PLAYER(pkWinner->getOwner());

	// "Heal if defeat enemy" promotion; doesn't apply if defeat a barbarian
	if(pkWinner->getHPHealedIfDefeatEnemy() > 0 && (pkLoser->getOwner() != BARBARIAN_PLAYER || !(pkWinner->IsHealIfDefeatExcludeBarbarians())))
	{
		if(pkWinner->getHPHealedIfDefeatEnemy() > pkWinner->getDamage())
		{
			pkWinner->changeDamage(-pkWinner->getDamage());
		}
		else
		{
			pkWinner->changeDamage(-pkWinner->getHPHealedIfDefeatEnemy());
		}
	}
#if defined(MOD_BUGFIX_MINOR)
	// If the modder wants the healing to be negative (ie additional damage), then let it be
	else if(pkWinner->getHPHealedIfDefeatEnemy() < 0 && (pkLoser->getOwner() != BARBARIAN_PLAYER || !(pkWinner->IsHealIfDefeatExcludeBarbarians())))
	{
		if(pkWinner->getHPHealedIfDefeatEnemy() <= (pkWinner->getDamage() - pkWinner->GetMaxHitPoints()))
		{
			// The graphics engine expects the unit to be alive here, so do NOT inflict enough damage to kill it!
			pkWinner->setDamage(pkWinner->GetMaxHitPoints()-1);
		}
		else
		{
			pkWinner->changeDamage(-pkWinner->getHPHealedIfDefeatEnemy());
		}
	}
#endif

#if defined(MOD_ROG_CORE)

	// All units heal from defeat a UNIT?
	if (pkWinner->getHPHealedIfDefeatEnemyGlobal() > 0)
	{
		kPlayer.DoHealGlobal(pkWinner->getHPHealedIfDefeatEnemyGlobal());
	}
#endif


	
	if (pkWinner->GetGoldenAgeValueFromKills() > 0)
	{
#if defined(MOD_API_EXTENSIONS)
		int iCombatStrength = max(pkLoser->GetBaseCombatStrength(), pkLoser->GetBaseRangedCombatStrength());
#else
		int iCombatStrength = max(pkLoser->getUnitInfo().GetCombat(), pkLoser->getUnitInfo().GetRangedCombat());
#endif
		if(iCombatStrength > 0)
		{
			int iValue = iCombatStrength * pkWinner->GetGoldenAgeValueFromKills() / 100;
			kPlayer.ChangeGoldenAgeProgressMeter(iValue);

#if defined(MOD_API_UNIFIED_YIELDS_GOLDEN_AGE)
			CvYieldInfo* pYieldInfo = GC.getYieldInfo(YIELD_GOLDEN_AGE_POINTS);
			CvString yieldString;
			yieldString.Format("%s+%%d[ENDCOLOR]%s", pYieldInfo->getColorString(), pYieldInfo->getIconString());
#else
			CvString yieldString = "[COLOR_WHITE]+%d[ENDCOLOR][ICON_GOLDEN_AGE]";
#endif

			if(pkWinner->getOwner() == GC.getGame().getActivePlayer())
			{
				char text[256] = {0};
#if !defined(SHOW_PLOT_POPUP)
				float fDelay = GC.getPOST_COMBAT_TEXT_DELAY() * 1.5f;
#endif
				sprintf_s(text, yieldString, iValue);
#if defined(SHOW_PLOT_POPUP)
				SHOW_PLOT_POPUP(pkLoser->plot(), pkWinner->getOwner(), text, 0.0);
#else
				GC.GetEngineUserInterface()->AddPopupText(pkLoser->getX(), pkLoser->getY(), text, fDelay);
#endif

				iExistingDelay++;
			}
		}
	}

	// Earn bonuses for kills?
#if defined(MOD_API_UNIFIED_YIELDS)
	kPlayer.DoYieldsFromKill(pkWinner, pkLoser, pkLoser->getX(), pkLoser->getY(), iExistingDelay);
#else
	kPlayer.DoYieldsFromKill(pkWinner->getUnitType(), pkLoser->getUnitType(), pkLoser->getX(), pkLoser->getY(), pkLoser->isBarbarian(), iExistingDelay);
#endif

#if !defined(NO_ACHIEVEMENTS)
	//Achievements and Stats
	if(pkWinner->isHuman() && !GC.getGame().isGameMultiPlayer())
	{
		CvString szUnitType;
		CvUnitEntry* pkUnitInfo = GC.getUnitInfo(pkWinner->getUnitType());
		if(pkUnitInfo)
			szUnitType = pkUnitInfo->GetType();

		//Elizabeth Special Achievement
		if((CvString)kPlayer.getLeaderTypeKey() == "LEADER_ELIZABETH" && pkLoser->getDomainType() == DOMAIN_SEA)
		{
			gDLL->IncrementSteamStatAndUnlock(ESTEAMSTAT_BRITISHNAVY, 357, ACHIEVEMENT_SPECIAL_ARMADA);
		}
		//Ramkang's Special Achievement
		if(szUnitType == "UNIT_SIAMESE_WARELEPHANT")
		{
			//CvString szUnitTypeLoser = (CvString) GC.getUnitInfo(pkLoser->getUnitType())->GetType();
		}

		//Oda's Special Achievement
		if((CvString)kPlayer.getLeaderTypeKey() == "LEADER_ODA_NOBUNAGA" && (pkWinner->GetMaxHitPoints() - pkWinner->getDamage() == 1))
		{
			gDLL->UnlockAchievement(ACHIEVEMENT_SPECIAL_KAMIKAZE);
		}
		//Napoleon's Special Achievement
		if(szUnitType == "UNIT_FRENCH_MUSKETEER")
		{
			if(pkLoser->GetNumSpecificEnemyUnitsAdjacent(pkLoser, pkWinner) >=3)
			{
				gDLL->UnlockAchievement(ACHIEVEMENT_SPECIAL_MUSKETEERS);
			}
		}

		//DLC_05 Sejong's Turtle Boat Achievement
		if(szUnitType == "UNIT_KOREAN_TURTLE_SHIP")
		{
			CvString szLoserUnitType;
			CvUnitEntry* pkLoserUnitInfo = GC.getUnitInfo(pkLoser->getUnitType());
			if(pkLoserUnitInfo)
			{
				szLoserUnitType = pkLoserUnitInfo->GetType();
			}
			if(szLoserUnitType == "UNIT_IRONCLAD")
			{
				gDLL->UnlockAchievement(ACHIEVEMENT_SPECIAL_IRONCLAD_TURTLE);
			}
		}

		//DLC_05 Sejong's Hwacha Achievement
		if(szUnitType == "UNIT_KOREAN_HWACHA")
		{
			gDLL->IncrementSteamStatAndUnlock(ESTEAMSTAT_HWACHAKILLS, 99, ACHIEVEMENT_SPECIAL_HWATCH_OUT);
		}

	}
#endif
}

void CvUnitCombat::ApplyPostCityCombatEffects(CvUnit* pkAttacker, CvCity* pkDefender, int iAttackerDamageInflicted)
{
	CvString colorString;
	int iPlunderModifier;
#if !defined(SHOW_PLOT_POPUP)
	float fDelay = GC.getPOST_COMBAT_TEXT_DELAY() * 3;
#endif
	iPlunderModifier = pkAttacker->GetCityAttackPlunderModifier();
	if(iPlunderModifier > 0)
	{
		int iGoldPlundered = iAttackerDamageInflicted * iPlunderModifier;
		iGoldPlundered /= 100;

		if(iGoldPlundered > 0)
		{
			GET_PLAYER(pkAttacker->getOwner()).GetTreasury()->ChangeGold(iGoldPlundered);

			CvPlayer& kCityPlayer = GET_PLAYER(pkDefender->getOwner());
			int iDeduction = min(iGoldPlundered, kCityPlayer.GetTreasury()->GetGold());
			kCityPlayer.GetTreasury()->ChangeGold(-iDeduction);

			if(pkAttacker->getOwner() == GC.getGame().getActivePlayer())
			{
				char text[256] = {0};
				colorString = "[COLOR_YELLOW]+%d[ENDCOLOR][ICON_GOLD]";
				sprintf_s(text, colorString, iGoldPlundered);
#if defined(SHOW_PLOT_POPUP)
				SHOW_PLOT_POPUP(pkAttacker->plot(), pkAttacker->getOwner(), text, 0.0);
#else
				GC.GetEngineUserInterface()->AddPopupText(pkAttacker->getX(), pkAttacker->getY(), text, fDelay);
#endif
			}
		}
	}
#if defined(MOD_PROMOTION_GET_INSTANCE_FROM_ATTACK)
	int iCityAttackFaithBonus;
#if !defined(SHOW_PLOT_POPUP)
	float fDelay = GC.getPOST_COMBAT_TEXT_DELAY() * 3;
#endif
	iCityAttackFaithBonus = pkAttacker->GetCityAttackFaithBonus();
	if(iCityAttackFaithBonus > 0 && MOD_PROMOTION_GET_INSTANCE_FROM_ATTACK)
	{
		int iFaithBonus = iAttackerDamageInflicted * iCityAttackFaithBonus;
		iFaithBonus /= 100;

		if(iFaithBonus > 0)
		{
			GET_PLAYER(pkAttacker->getOwner()).ChangeFaith(iFaithBonus);
			CvPlayer& kCityPlayer = GET_PLAYER(pkDefender->getOwner());
			int iDeduction = min(iFaithBonus, kCityPlayer.GetFaith());
			kCityPlayer.ChangeFaith(-iDeduction);

			if(pkAttacker->getOwner() == GC.getGame().getActivePlayer())
			{
				char text[256] = {0};
				colorString = "[COLOR_YELLOW]+%d[ENDCOLOR][ICON_PEACE]";
				sprintf_s(text, colorString, iFaithBonus);
#if defined(SHOW_PLOT_POPUP)
				SHOW_PLOT_POPUP(pkAttacker->plot(), pkAttacker->getOwner(), text, 0.0);
#else
				GC.GetEngineUserInterface()->AddPopupText(pkAttacker->getX(), pkAttacker->getY(), text, fDelay);
#endif
			}
		}
	}	
#endif
}

#ifdef MOD_NEW_BATTLE_EFFECTS
inline static CvPlayerAI& getAttackerPlayer(const CvCombatInfo& kCombatInfo)
{
	CvUnit* pAttackerUnit = kCombatInfo.getUnit(BATTLE_UNIT_ATTACKER);
	CvCity* pAttackerCity = kCombatInfo.getCity(BATTLE_UNIT_ATTACKER);
	return GET_PLAYER(pAttackerUnit ? pAttackerUnit->getOwner() : pAttackerCity->getOwner());
}
inline static CvPlayerAI& getDefenderPlayer(const CvCombatInfo& kCombatInfo)
{
	CvUnit* pDefenderUnit = kCombatInfo.getUnit(BATTLE_UNIT_DEFENDER);
	CvCity* pDefenderCity = kCombatInfo.getCity(BATTLE_UNIT_DEFENDER);
	return GET_PLAYER(pDefenderUnit ? pDefenderUnit->getOwner() : pDefenderCity->getOwner());
}

#ifdef MOD_ROG_CORE
void UnitDamageChangeInterveneNoCondition(CvUnit* thisUnit, int* enemyInflictDamage)
{
	if (!thisUnit || !enemyInflictDamage) return;
	if (thisUnit->getForcedDamageValue() != 0)
	{
		*enemyInflictDamage = thisUnit->getForcedDamageValue();
	}
	if (thisUnit->getChangeDamageValue() != 0)
	{
		*enemyInflictDamage += thisUnit->getChangeDamageValue();
	}
}

void UnitDamageChangeIntervene(InflictDamageContext* ctx)
{
	UnitDamageChangeInterveneNoCondition(ctx->pAttackerUnit, ctx->piDefenseInflictDamage);
	UnitDamageChangeInterveneNoCondition(ctx->pDefenderUnit, ctx->piAttackInflictDamage);
}

void CityDamageChangeInterveneNoCondition(CvCity* thisCity, int* enemyInflictDamage)
{
	if (!thisCity || !enemyInflictDamage) return;

	if (thisCity->getResetDamageValue() != 0)
	{
		*enemyInflictDamage = thisCity->getResetDamageValue();
	}
	if (thisCity->getReduceDamageValue() != 0)
	{
		*enemyInflictDamage += thisCity->getReduceDamageValue();
	}
}

void CityDamageChangeIntervene(InflictDamageContext* ctx)
{
	CityDamageChangeInterveneNoCondition(ctx->pDefenderCity, ctx->piAttackInflictDamage);
}

void UnitAttackInflictDamageIntervene(InflictDamageContext* ctx)
{
	// Unit VS Unit
	if (ctx->pAttackerUnit != nullptr && ctx->piAttackInflictDamage != nullptr && ctx->pDefenderCity == nullptr)
	{
		*ctx->piAttackInflictDamage += ctx->pAttackerUnit->GetAttackInflictDamageChange();
		if (ctx->pDefenderUnit != nullptr)
		{
			*ctx->piAttackInflictDamage += ctx->pAttackerUnit->GetAttackInflictDamageChangeMaxHPPercent() * ctx->pDefenderUnit->GetMaxHitPoints() / 100;
		}
	}
}

void UnitDefenseInflictDamageIntervene(InflictDamageContext* ctx)
{
	// Unit VS Unit
	if (ctx->pDefenderUnit != nullptr && ctx->piDefenseInflictDamage != nullptr && ctx->pAttackerCity == nullptr)
	{
		*ctx->piDefenseInflictDamage += ctx->pDefenderUnit->GetDefenseInflictDamageChange();
		if (ctx->pAttackerUnit != nullptr)
		{
			*ctx->piDefenseInflictDamage += ctx->pDefenderUnit->GetDefenseInflictDamageChangeMaxHPPercent() * ctx->pAttackerUnit->GetMaxHitPoints() / 100;
		}
	}
}

void SiegeInflictDamageIntervene(InflictDamageContext* ctx)
{
	// Unit VS City
	if (ctx->pAttackerUnit != nullptr && ctx->piAttackInflictDamage != nullptr && ctx->pDefenderCity != nullptr)
	{
		*ctx->piAttackInflictDamage += ctx->pAttackerUnit->GetSiegeInflictDamageChange();
		*ctx->piAttackInflictDamage += ctx->pAttackerUnit->GetSiegeInflictDamageChangeMaxHPPercent() * ctx->pDefenderCity->GetMaxHitPoints() / 100;
	}
}

void CvUnitCombat::InterveneInflictDamage(InflictDamageContext* ctx)
{
	if (ctx == nullptr) return;
	UnitDamageChangeIntervene(ctx);
	CityDamageChangeIntervene(ctx);
	UnitAttackInflictDamageIntervene(ctx);
	UnitDefenseInflictDamageIntervene(ctx);
	SiegeInflictDamageIntervene(ctx);

	if (ctx->piAttackInflictDamage && *ctx->piAttackInflictDamage <= 0)
	{
		*ctx->piAttackInflictDamage = 0;
	}
	if (ctx->piDefenseInflictDamage && *ctx->piDefenseInflictDamage <= 0)
	{
		*ctx->piDefenseInflictDamage = 0;
	}
}
#endif

static int calcDamage(CvUnit* pAttacker, CvPlot* pFromPlot, CvUnit* pDefender, CvPlot* pTargetPlot, bool bRangedAttack)
{
	if (bRangedAttack) {
		return pAttacker->GetRangeCombatDamage(pDefender, nullptr, false); ;
	}
	else {
		int iAttackStrength = pAttacker->GetMaxAttackStrength(pFromPlot, pDefender->plot(), pDefender);
		int iDefenseStrength = pDefender->GetMaxDefenseStrength(pDefender->plot(), pAttacker, false);
		return pAttacker->getCombatDamage(iAttackStrength, iDefenseStrength, pAttacker->getDamage(), false, false, false);
	}
}

static int calcOrCacheDamage(CvUnit* pAttacker, CvPlot* pFromPlot, CvUnit* pDefender, CvPlot* pTargetPlot, bool bRangedAttack, std::tr1::unordered_map<CvUnit*, int>& mUnitDamageBaseMap)
{
	int result = 0;
	auto iterCachedDamageBase = mUnitDamageBaseMap.find(pDefender);
	if (iterCachedDamageBase != mUnitDamageBaseMap.end()) // hit cache
	{
		result = iterCachedDamageBase->second;
	}
	else
	{
		result = calcDamage(pAttacker, pFromPlot, pDefender, pTargetPlot, bRangedAttack);
		mUnitDamageBaseMap[pDefender] = result;
	}

	return result;
}

void CvUnitCombat::DoNewBattleEffects(const CvCombatInfo& kCombatInfo)
{
	if (!ShouldDoNewBattleEffects(kCombatInfo))
		return;
	DoSplashDamage(kCombatInfo);
	DoCollateralDamage(kCombatInfo);
	DoAddEnemyPromotions(kCombatInfo);
	DoDestroyBuildings(kCombatInfo);
	DoKillCitizens(kCombatInfo);
	DoStackingFightBack(kCombatInfo);
	DoStopAttacker(kCombatInfo);
}

bool CvUnitCombat::ShouldDoNewBattleEffects(const CvCombatInfo& kCombatInfo)
{
	if (kCombatInfo.getAttackIsNuclear()) return false;

	CvPlayerAI& kAttackPlayer = getAttackerPlayer(kCombatInfo);
	CvPlayerAI& kDefensePlayer = getDefenderPlayer(kCombatInfo);

	// Only do this for human players.
	// May provide GameOption to enable for AI later.
	return kAttackPlayer.isHuman() || kDefensePlayer.isHuman();
}

#ifdef MOD_PROMOTION_SPLASH_DAMAGE


// AOE damage for units with the splash damage promotion
void CvUnitCombat::DoSplashDamage(const CvCombatInfo& kCombatInfo)
{
	if (!MOD_PROMOTION_SPLASH_DAMAGE) {
		return;
	}

	CvUnit* pAttackerUnit = kCombatInfo.getUnit(BATTLE_UNIT_ATTACKER);
	CvCity* pAttackerCity = kCombatInfo.getCity(BATTLE_UNIT_ATTACKER);
	CvUnit* pDefenderUnit = kCombatInfo.getUnit(BATTLE_UNIT_DEFENDER);
	CvCity* pDefenderCity = kCombatInfo.getCity(BATTLE_UNIT_DEFENDER);

	// Can do splash damage?
	// TODO: allow city to do splash damage.
	if (pAttackerUnit == nullptr) return;
	auto& vSplashInfoVec = pAttackerUnit->GetSplashInfoVec();
	if (vSplashInfoVec.empty()) return;

	CvPlayerAI& kAttackPlayer = getAttackerPlayer(kCombatInfo);
	CvPlayerAI& kDefensePlayer = getDefenderPlayer(kCombatInfo);
	CvPlot* pFromPlot = pAttackerUnit ? pAttackerUnit->plot() : pAttackerCity->plot();
	CvPlot* pTargetPlot = kCombatInfo.getPlot();
	bool bRangedAttack = kCombatInfo.getAttackIsRanged() || kCombatInfo.getAttackIsBombingMission();

	int iX = pTargetPlot->getX();
	int iY = pTargetPlot->getY();

	std::tr1::unordered_map<CvUnit*, int> mUnitDamageSumMap;
	std::tr1::unordered_map<CvUnit*, int> mUnitDamageBaseMap;
	for (const auto& sSplashInfo : vSplashInfoVec)
	{
		int iRadius = sSplashInfo.iRadius;
		int iDamageRateTimes100 = sSplashInfo.iPercent;
		int iUnitLimitPerTile = sSplashInfo.iPlotUnitLimit;
		int iFixed = sSplashInfo.iFixed;

		std::tr1::unordered_set<CvUnit*> dedupSet;
		for (int i = -iRadius; i <= iRadius; ++i) {
			for (int j = -iRadius; j <= iRadius; ++j) {
				CvPlot* pLoopPlot = ::plotXYWithRangeCheck(iX, iY, i, j, iRadius);
				if (pLoopPlot == nullptr || (pLoopPlot->getX() == iX && pLoopPlot->getY() == iY))
					continue;

				int iAffectedCounter = 0;
				for (int iUnitIndex = 0; iUnitIndex < pLoopPlot->getNumUnits(); iUnitIndex++)
				{
					CvUnit* pAOEUnit = pLoopPlot->getUnitByIndex(iUnitIndex);
					bool bAOEImmune = pAOEUnit->GetSplashImmuneRC() > 0;

					if (bAOEImmune) continue;
					if (pAOEUnit->getDomainType() != DOMAIN_LAND && pAOEUnit->getDomainType() != DOMAIN_SEA) continue;
					if (!kAttackPlayer.IsAtWarWith(pAOEUnit->getOwner())) continue;
					if (dedupSet.count(pAOEUnit) > 0) continue;
					dedupSet.insert(pAOEUnit);

					int iAOEDamageBase = calcOrCacheDamage(pAttackerUnit, pFromPlot, pAOEUnit, pLoopPlot, bRangedAttack, mUnitDamageBaseMap);
					int iAOEDamage = (int64)iAOEDamageBase * (int64) iDamageRateTimes100 / 100;
					mUnitDamageSumMap[pAOEUnit] += iAOEDamage + iFixed;

					iAffectedCounter++;
					if (iAffectedCounter >= iUnitLimitPerTile)
					{
						break;
					}
				}
			}
		}
	}

	ICvUserInterface2* pkDLLInterface = GC.GetEngineUserInterface();
	for (auto iter = mUnitDamageSumMap.begin(); iter != mUnitDamageSumMap.end(); iter++)
	{
		CvUnit* pAOEUnit = iter->first;
		int iAOEDamage = iter->second;
		if (iAOEDamage == 0)
		{
			continue;
		}

		bool bAOEKill = iAOEDamage >= pAOEUnit->GetCurrHitPoints();
		pAOEUnit->changeDamage(iAOEDamage, kAttackPlayer.GetID(), pAttackerUnit->GetID());
		pAOEUnit->ShowDamageDeltaText(iAOEDamage, pAOEUnit->plot());

		if (pAttackerUnit->GetSplashXP() != 0)
		{
			pAttackerUnit->changeExperienceTimes100(pAttackerUnit->GetSplashXP() * 100);
		}

		if (kAttackPlayer.isHuman())
		{
			if (bAOEKill)
			{
				CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_SPLASH_DAMAGE_ENEMY_DEATH", pAttackerUnit->getNameKey(), pAOEUnit->getNameKey());
				pkDLLInterface->AddMessage(0, kAttackPlayer.GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer /*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
			}
			else
			{
				CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_SPLASH_DAMAGE_ENEMY", pAttackerUnit->getNameKey(), pAOEUnit->getNameKey(), iAOEDamage);
				pkDLLInterface->AddMessage(0, kAttackPlayer.GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer /*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
			}
		}
		if (kDefensePlayer.isHuman())
		{
			if (bAOEKill)
			{
				CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_SPLASH_DAMAGE_DEATH", pAttackerUnit->getNameKey(), pAOEUnit->getNameKey());
				pkDLLInterface->AddMessage(0, kDefensePlayer.GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer /*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
			}
			else
			{
				CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_SPLASH_DAMAGE_ENEMY", pAttackerUnit->getNameKey(), pAOEUnit->getNameKey(), iAOEDamage);
				pkDLLInterface->AddMessage(0, kDefensePlayer.GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer /*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
			}
		}
	}
}
#endif

#ifdef MOD_PROMOTION_COLLATERAL_DAMAGE
void CvUnitCombat::DoCollateralDamage(const CvCombatInfo& kCombatInfo)
{
	if (!MOD_PROMOTION_COLLATERAL_DAMAGE) return;

	CvUnit* pAttackerUnit = kCombatInfo.getUnit(BATTLE_UNIT_ATTACKER);
	CvCity* pAttackerCity = kCombatInfo.getCity(BATTLE_UNIT_ATTACKER);
	CvUnit* pDefenderUnit = kCombatInfo.getUnit(BATTLE_UNIT_DEFENDER);
	CvCity* pDefenderCity = kCombatInfo.getCity(BATTLE_UNIT_DEFENDER);

	// Only works if the attacker is a unit.
	if (pAttackerCity != nullptr) return;

	CvPlayerAI& kAttackPlayer = getAttackerPlayer(kCombatInfo);
	CvPlayerAI& kDefensePlayer = getDefenderPlayer(kCombatInfo);
	CvPlot* pFromPlot = pAttackerUnit ? pAttackerUnit->plot() : pAttackerCity->plot();
	CvPlot* pTargetPlot = kCombatInfo.getPlot();
	if (pTargetPlot == nullptr) return;

	bool bRangedAttack = kCombatInfo.getAttackIsRanged() || kCombatInfo.getAttackIsBombingMission();

	std::tr1::unordered_map<CvUnit*, int> mUnitDamageSumMap;
	std::tr1::unordered_map<CvUnit*, int> mUnitDamageBaseMap;

	std::vector<CollateralInfo>& vCollateralInfo = pAttackerUnit->GetCollateralInfoVec();
	if (vCollateralInfo.empty()) return;

	for (const auto& sCollateralInfo : vCollateralInfo)
	{
		int iDamageRateTimes100 = sCollateralInfo.iPercent;
		int iUnitLimitPerTile = sCollateralInfo.iPlotUnitLimit;
		int iFixed = sCollateralInfo.iFixed;

		bool bOnlyCity = sCollateralInfo.bOnlyCity;
		if (bOnlyCity)
		{
			if (pDefenderCity == nullptr) continue;
			else pDefenderUnit = nullptr;
		}

		bool bOnlyUnit = sCollateralInfo.bOnlyUnit;
		if (bOnlyUnit && pTargetPlot->isCity()) continue;

		std::tr1::unordered_set<CvUnit*> dedupSet;

		int iAffectedCounter = 0;
		for (int iUnitIndex = 0; iUnitIndex < pTargetPlot->getNumUnits(); iUnitIndex++)
		{
			CvUnit* pAffectedUnit = pTargetPlot->getUnitByIndex(iUnitIndex);
			bool bImmune = pAffectedUnit->GetCollateralImmuneRC() > 0;
			if (pAffectedUnit == pDefenderUnit) continue;
			if (bImmune) continue;
			if (pAffectedUnit->getDomainType() != DOMAIN_LAND && pAffectedUnit->getDomainType() != DOMAIN_SEA) continue;
			if (!kAttackPlayer.IsAtWarWith(pAffectedUnit->getOwner())) continue;
			if (dedupSet.count(pAffectedUnit) > 0) continue;
			dedupSet.insert(pAffectedUnit);

			int iDamageBase = calcOrCacheDamage(pAttackerUnit, pFromPlot, pAffectedUnit, pTargetPlot, bRangedAttack, mUnitDamageBaseMap);
			int iDamage = (int64)iDamageBase * (int64)iDamageRateTimes100 / 100;
			mUnitDamageSumMap[pAffectedUnit] += iDamage + iFixed;

			iAffectedCounter++;
			if (iAffectedCounter >= iUnitLimitPerTile)
			{
				break;
			}
		}
	}

	ICvUserInterface2* pkDLLInterface = GC.GetEngineUserInterface();
	for (auto iter = mUnitDamageSumMap.begin(); iter != mUnitDamageSumMap.end(); iter++)
	{
		CvUnit* pAffectedUnit = iter->first;
		int iDamage = iter->second;
		if (iDamage == 0)
		{
			continue;
		}

		bool bKill = iDamage >= pAffectedUnit->GetCurrHitPoints();
		pAffectedUnit->changeDamage(iDamage, kAttackPlayer.GetID(), pAttackerUnit->GetID());
		pAffectedUnit->ShowDamageDeltaText(iDamage, pAffectedUnit->plot());

		if (pAttackerUnit->GetCollateralXP() != 0)
		{
			pAttackerUnit->changeExperienceTimes100(pAttackerUnit->GetCollateralXP() * 100);
		}

		if (kAttackPlayer.isHuman())
		{
			if (bKill)
			{
				CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_COLL_DAMAGE_ENEMY_DEATH", pAttackerUnit->getNameKey(), pAffectedUnit->getNameKey());
				pkDLLInterface->AddMessage(0, kAttackPlayer.GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer /*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
			}
			else
			{
				CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_COLL_DAMAGE_ENEMY", pAttackerUnit->getNameKey(), pAffectedUnit->getNameKey(), iDamage);
				pkDLLInterface->AddMessage(0, kAttackPlayer.GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer /*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
			}
		}
		if (kDefensePlayer.isHuman())
		{
			if (bKill)
			{
				CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_COLL_DAMAGE_DEATH", pAttackerUnit->getNameKey(), pAffectedUnit->getNameKey());
				pkDLLInterface->AddMessage(0, kDefensePlayer.GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer /*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
			}
			else
			{
				CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_COLL_DAMAGE", pAttackerUnit->getNameKey(), pAffectedUnit->getNameKey(), iDamage);
				pkDLLInterface->AddMessage(0, kDefensePlayer.GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer /*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
			}
		}
	}
}
#endif

#ifdef MOD_PROMOTION_ADD_ENEMY_PROMOTIONS
static void DoAddEnemyPromotionsInner(CvUnit* thisUnit, CvUnit* thatUnit, BattleUnitTypes thisBattleType, const CvCombatInfo& kCombatInfo)
{
	if (thisUnit == nullptr || thisUnit->GetPromotionCollections().empty()) return;
	if (thatUnit == nullptr || thatUnit->isDelayedDeath() || thatUnit->IsDead() || thatUnit->GetAddEnemyPromotionImmuneRC() > 0) return;

	bool ranged = kCombatInfo.getAttackIsRanged() || kCombatInfo.getAttackIsBombingMission();
	bool melee = !ranged;
	bool attack = thisBattleType == BATTLE_UNIT_ATTACKER;
	bool defense = thisBattleType == BATTLE_UNIT_DEFENDER;
	bool rangedAttack = ranged && attack;
	bool meleeAttack = melee && attack;
	bool rangedDefense = ranged && defense;
	bool meleeDefense = melee && defense;

	auto& collections = thisUnit->GetPromotionCollections();

	for (auto collectionIter = collections.begin(); collectionIter != collections.end(); collectionIter++)
	{
		if (collectionIter->second <= 0) continue;

		auto collectionType = collectionIter->first;
		auto* collection = GC.GetPromotionCollection(collectionType);
		if (!collection->CanAddEnemyPromotions()) continue;

		bool breakPromotionLoop = false;
		for (auto promotionIter = collection->GetPromotions().rbegin(); !breakPromotionLoop && promotionIter != collection->GetPromotions().rend(); promotionIter++)
		{
			if (!thisUnit->HasPromotion(promotionIter->m_ePromotionType)) continue;
			breakPromotionLoop = true;

			auto& triggerInfo = promotionIter->m_kTriggerInfo;
			bool combatTypeOK = ((rangedAttack && triggerInfo.m_bRangedAttack)
				|| (rangedDefense && triggerInfo.m_bRangedDefense)
				|| (meleeAttack && triggerInfo.m_bMeleeAttack)
				|| (meleeDefense && triggerInfo.m_bMeleeDefense));
			if (!combatTypeOK) continue;

			bool isTrigger = false;
			if (triggerInfo.m_bLuaCheck)
			{
				isTrigger = GAMEEVENTINVOKE_TESTANY(GAMEEVENT_CanAddEnemyPromotion, promotionIter->m_ePromotionType, collectionType, 
					thisBattleType, thisUnit->getOwner(), thisUnit->GetID(), thatUnit->getOwner(), thatUnit->GetID()) == GAMEEVENTRETURN_TRUE;
			}
			if (!isTrigger)
			{
				int thatHP = thatUnit->GetCurrHitPoints();
				if (thatHP < triggerInfo.m_iHPFixed + triggerInfo.m_iHPPercent * thatUnit->GetMaxHitPoints() / 100)
				{
					isTrigger = true;
				}
			}
			if (!isTrigger) continue;

			for (auto collectionToAdd : collection->GetAddEnemyPromotionPools())
			{
				auto* pCollectionToAdd = GC.GetPromotionCollection(collectionToAdd);
				PromotionTypes thatPromotion = NO_PROMOTION;
				for (auto& promotionToAdd : pCollectionToAdd->GetPromotions())
				{
					if (thatUnit->HasPromotion(promotionToAdd.m_ePromotionType)) continue;

					thatUnit->setHasPromotion(promotionToAdd.m_ePromotionType, true);
					thatPromotion = promotionToAdd.m_ePromotionType;
					break;
				}
				if (triggerInfo.m_bLuaHook)
				{
					GAMEEVENTINVOKE_HOOK(GAMEEVENT_OnTriggerAddEnemyPromotion, promotionIter->m_ePromotionType, collectionType, thisBattleType, thisUnit->getOwner(), thisUnit->GetID(),
					thisUnit->getUnitInfo().GetID(), thatPromotion, pCollectionToAdd->GetID(), thatUnit->getOwner(), thatUnit->GetID(), thatUnit->getUnitInfo().GetID());
				}
			}
		}
	}
}

void CvUnitCombat::DoAddEnemyPromotions(const CvCombatInfo& kCombatInfo)
{
	if (!MOD_PROMOTION_ADD_ENEMY_PROMOTIONS) {
		return;
	}

	CvUnit* pAttackerUnit = kCombatInfo.getUnit(BATTLE_UNIT_ATTACKER);
	CvCity* pAttackerCity = kCombatInfo.getCity(BATTLE_UNIT_ATTACKER);
	CvUnit* pDefenderUnit = kCombatInfo.getUnit(BATTLE_UNIT_DEFENDER);
	CvCity* pDefenderCity = kCombatInfo.getCity(BATTLE_UNIT_DEFENDER);

	if (pAttackerCity || pDefenderCity) return; // no city combat
	if (kCombatInfo.getAttackIsNuclear() || kCombatInfo.getAttackIsAirSweep()) return;

	DoAddEnemyPromotionsInner(pAttackerUnit, pDefenderUnit, BATTLE_UNIT_ATTACKER, kCombatInfo);
	DoAddEnemyPromotionsInner(pDefenderUnit, pAttackerUnit, BATTLE_UNIT_DEFENDER, kCombatInfo);
}
#endif

#ifdef MOD_PROMOTION_CITY_DESTROYER
void CvUnitCombat::DoDestroyBuildings(const CvCombatInfo& kInfo)
{
	if (!MOD_PROMOTION_CITY_DESTROYER) return;

	CvUnit* pAttackerUnit = kInfo.getUnit(BATTLE_UNIT_ATTACKER);
	CvCity* pAttackerCity = kInfo.getCity(BATTLE_UNIT_ATTACKER);
	CvUnit* pDefenderUnit = kInfo.getUnit(BATTLE_UNIT_DEFENDER);
	CvCity* pDefenderCity = kInfo.getCity(BATTLE_UNIT_DEFENDER);

	if (pAttackerUnit == nullptr || pDefenderCity == nullptr) return; // only work for unit attacking a city

	auto& destroyInfo = pAttackerUnit->GetDestroyBuildings();
	if (destroyInfo.empty()) return;

	CvPlayerAI& kAttackPlayer = getAttackerPlayer(kInfo);
	CvPlayerAI& kDefensePlayer = getDefenderPlayer(kInfo);
	int iCount = 0;
	for (auto iter = destroyInfo.begin(); iter != destroyInfo.end(); iter++)
	{
		for (int i = 0; i < iter->second.m_iDestroyBuildingNumLimit; i++)
		{
			int iRand = GC.getGame().getJonRandNum(100, "destroy chance");
			if (iRand >= iter->second.m_iDestroyBuildingProbability) continue;

			auto* collection = GC.GetBuildingClassCollection(iter->second.m_iDestroyBuildingCollection);
			for (auto& entry : collection->GetBuildingClasses())
			{
				BuildingTypes eBuilding = kDefensePlayer.GetCivBuilding(entry.eBuildingClass);
				if (pDefenderCity->HasBuilding(eBuilding))
				{
					pDefenderCity->GetCityBuildings()->SetNumRealBuilding(eBuilding, 0);
					iCount++;
					break;
				}
			}
		}
	}

	if (iCount > 0)
	{
		ICvUserInterface2* pkDLLInterface = GC.GetEngineUserInterface();
		if (kAttackPlayer.isHuman())
		{
			CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_BOMBER_CITY_BUILDING_DESTROYED_ATTACKING", iCount);
			pkDLLInterface->AddMessage(0, kAttackPlayer.GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer /*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
		}
		if (kDefensePlayer.isHuman())
		{
			CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_BOMBER_CITY_BUILDING_DESTROYED_ATTACKED", iCount, pDefenderCity->getNameKey());
			pkDLLInterface->AddMessage(0, kDefensePlayer.GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer /*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
		}
	}
}

void CvUnitCombat::DoKillCitizens(const CvCombatInfo& kInfo)
{
	if (!MOD_PROMOTION_CITY_DESTROYER) return;

	CvUnit* pAttackerUnit = kInfo.getUnit(BATTLE_UNIT_ATTACKER);
	CvCity* pAttackerCity = kInfo.getCity(BATTLE_UNIT_ATTACKER);
	CvUnit* pDefenderUnit = kInfo.getUnit(BATTLE_UNIT_DEFENDER);
	CvCity* pDefenderCity = kInfo.getCity(BATTLE_UNIT_DEFENDER);

	if (pAttackerUnit == nullptr || pDefenderCity == nullptr) return; // only work for unit attacking a city
	if (!pAttackerUnit->CanSiegeKillCitizens()) return;

	CvPlayerAI& kAttackPlayer = getAttackerPlayer(kInfo);
	CvPlayerAI& kDefensePlayer = getDefenderPlayer(kInfo);

	int iKillNum = pDefenderCity->getPopulation() * pAttackerUnit->GetSiegeKillCitizensPercent() / 100 + pAttackerUnit->GetSiegeKillCitizensFixed();
	iKillNum = (pDefenderCity->GetSiegeKillCitizensModifier() + 100) * iKillNum / 100;
	if (iKillNum >= pDefenderCity->getPopulation())
	{
		iKillNum = pDefenderCity->getPopulation() - 1;
	}
	if (iKillNum <= 0) return;

	pDefenderCity->changePopulation(-iKillNum, true);

	ICvUserInterface2* pkDLLInterface = GC.GetEngineUserInterface();
	if (kAttackPlayer.isHuman())
	{
		CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_BOMBER_CITY_KILL_POPULATION_ATTACKING", iKillNum);
		pkDLLInterface->AddMessage(0, kAttackPlayer.GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer /*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
	}
	if (kDefensePlayer.isHuman())
	{
		CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_BOMBER_CITY_KILL_POPULATION_ATTACKED", iKillNum, pDefenderCity->getNameKey());
		pkDLLInterface->AddMessage(0, kDefensePlayer.GetID(), true, GC.getEVENT_MESSAGE_TIME(), strBuffer /*, "AS2D_COMBAT", MESSAGE_TYPE_INFO, pkDefender->getUnitInfo().GetButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pkTargetPlot->getX(), pkTargetPlot->getY()*/);
	}
}

#endif

void CvUnitCombat::DoStackingFightBack(const CvCombatInfo & kCombatInfo)
{
	CvUnit *pAttackerUnit = kCombatInfo.getUnit(BATTLE_UNIT_ATTACKER);
	CvUnit *pDefenderUnit = kCombatInfo.getUnit(BATTLE_UNIT_DEFENDER);
	if (pAttackerUnit == nullptr || pDefenderUnit == nullptr)
		return;
	if (pAttackerUnit->IsDead() || pAttackerUnit->isDelayedDeath() || pDefenderUnit->IsDead() || pDefenderUnit->isDelayedDeath() || !pDefenderUnit->IsCombatUnit())
		return;
	CvPlot *pTargetPlot = kCombatInfo.getPlot();
	if (pTargetPlot == nullptr || pAttackerUnit->plot()->isCity())
		return;

	bool ranged = kCombatInfo.getAttackIsRanged();
	bool melee = !ranged;

	for (int iUnitIndex = 0; iUnitIndex < pTargetPlot->getNumUnits(); iUnitIndex++)
	{
		CvUnit *pFoundUnit = pTargetPlot->getUnitByIndex(iUnitIndex);
		if (pFoundUnit == nullptr || pFoundUnit == pDefenderUnit)
			continue;
		if (!pFoundUnit->canRangeStrike())
			continue;

		auto &collections = pFoundUnit->GetPromotionCollections();
		if (collections.empty()) continue;

		for (auto it = collections.begin(); it != collections.end(); it++)
		{
			if (it->second <= 0)
				continue;

			auto collectionType = it->first;
			auto *collection = GC.GetPromotionCollection(collectionType);
			if (collection == nullptr)
				continue;
			if (!collection->GetStackingFightBack())
				continue;

			auto &promotions = collection->GetPromotions();
			PromotionTypes activatePromotion = NO_PROMOTION;
			bool canMelee = false;
			bool canRanged = false;
			for (auto it2 = promotions.rbegin(); it2 != promotions.rend(); it2++)
			{
				if (pFoundUnit->HasPromotion(it2->m_ePromotionType))
				{
					canMelee = it2->m_kTriggerInfo.m_bMeleeDefense;
					canRanged = it2->m_kTriggerInfo.m_bRangedDefense;
					activatePromotion = it2->m_ePromotionType;
					break;
				}
			}

			if (activatePromotion == NO_PROMOTION)
				continue;
			if (!((melee && canMelee) || (ranged && canRanged)))
				continue;

			auto movesLeft = pFoundUnit->movesLeft();
			CvUnitCombat::AttackRanged(*pFoundUnit, pAttackerUnit->getX(), pAttackerUnit->getY(), CvUnitCombat::ATTACK_OPTION_NONE);
			pFoundUnit->setMadeAttack(false);
			pFoundUnit->setMoves(movesLeft);
		}
	}
}

void CvUnitCombat::DoStopAttacker(const CvCombatInfo& kCombatInfo)
{
	CvUnit *pAttackerUnit = kCombatInfo.getUnit(BATTLE_UNIT_ATTACKER);
	CvUnit *pDefenderUnit = kCombatInfo.getUnit(BATTLE_UNIT_DEFENDER);
	if (pAttackerUnit == nullptr || pDefenderUnit == nullptr)
		return;
	if (pAttackerUnit->IsDead() || pAttackerUnit->isDelayedDeath() || pDefenderUnit->IsDead() || pDefenderUnit->isDelayedDeath() || !pDefenderUnit->IsCombatUnit())
		return;
	if (pDefenderUnit->IsGarrisoned())
		return;
	if (pDefenderUnit->getDomainType() != pAttackerUnit->getDomainType())
		return;
	if (pAttackerUnit->IsImmuneNegtivePromotions())
		return;

	bool ranged = kCombatInfo.getAttackIsRanged();
	bool melee = !ranged;

	auto &collections = pDefenderUnit->GetPromotionCollections();
	if (collections.empty()) return;

	for (auto it = collections.begin(); it != collections.end(); it++)
	{
		if (it->second <= 0)
			continue;

		auto collectionType = it->first;
		auto *collection = GC.GetPromotionCollection(collectionType);
		if (collection == nullptr)
			continue;
		if (!collection->GetStopAttacker())
			continue;

		auto &promotions = collection->GetPromotions();
		PromotionTypes activatePromotion = NO_PROMOTION;
		bool canMelee = false;
		bool canRanged = false;
		int triggerHPFixed = 0;
		int triggerHPPercent = 0;
		for (auto it2 = promotions.rbegin(); it2 != promotions.rend(); it2++)
		{
			if (pDefenderUnit->HasPromotion(it2->m_ePromotionType))
			{
				canMelee = it2->m_kTriggerInfo.m_bMeleeDefense;
				canRanged = it2->m_kTriggerInfo.m_bRangedDefense;
				triggerHPFixed = it2->m_kTriggerInfo.m_iHPFixed;
				triggerHPPercent = it2->m_kTriggerInfo.m_iHPPercent;
				activatePromotion = it2->m_ePromotionType;
				break;
			}
		}

		if (activatePromotion == NO_PROMOTION)
			continue;
		if (!((melee && canMelee) || (ranged && canRanged)))
			continue;
		if (triggerHPFixed <= 0 && triggerHPPercent <= 0)
			continue;
		if (pAttackerUnit->GetCurrHitPoints() >= triggerHPFixed + triggerHPPercent * pAttackerUnit->GetMaxHitPoints() / 100)
			continue;

		pAttackerUnit->setMoves(0);
		return;
	}
}

void CvUnitCombat::DoHeavyChargeEffects(CvUnit* attacker, CvUnit* defender, CvPlot* battlePlot)
{
	if (attacker == nullptr || attacker->IsDead() || attacker->isDelayedDeath()) return;

	int addMoves = attacker->GetHeavyChargeAddMoves();
	if (addMoves > 0)
	{
		attacker->changeMoves(addMoves);
	}

	auto& kAttackPlayer = GET_PLAYER(attacker->getOwner());
	int collateralFixed = attacker->GetHeavyChargeCollateralFixed(), collateralPercent = attacker->GetHeavyChargeCollateralPercent();
	if (collateralFixed > 0 || collateralPercent > 0)
	{
		for (int iUnitIndex = 0; iUnitIndex < battlePlot->getNumUnits(); iUnitIndex++)
		{
			CvUnit* pAffectedUnit = battlePlot->getUnitByIndex(iUnitIndex);
			if (pAffectedUnit == defender) continue;
			if (pAffectedUnit->getDomainType() != DOMAIN_LAND && pAffectedUnit->getDomainType() != DOMAIN_SEA) continue;
			if (!kAttackPlayer.IsAtWarWith(pAffectedUnit->getOwner())) continue;

			int iDamageBase = calcDamage(attacker, attacker->plot(), pAffectedUnit, battlePlot, false);
			int iDamage = (int64)iDamageBase * (int64)collateralPercent / 100 + collateralFixed;
			pAffectedUnit->changeDamage(iDamage, attacker->getOwner(), attacker->GetID());
		}
	}

	int extraDamage = attacker->GetHeavyChargeExtraDamage();
	if (defender && !defender->isDelayedDeath() && extraDamage > 0)
	{
		if (!defender->IsDead() && !defender->isDelayedDeath())
		{
			defender->changeDamage(extraDamage, attacker->getOwner(), attacker->GetID());
		}
	}
}

#endif

#if defined(MOD_PROMOTION_GET_INSTANCE_FROM_ATTACK)
void CvUnitCombat::DoInstantYieldFromCombat(const CvCombatInfo & kCombatInfo)
{
	if (!MOD_PROMOTION_GET_INSTANCE_FROM_ATTACK) return;
#if !defined(SHOW_PLOT_POPUP)
	float fDelay = GC.getPOST_COMBAT_TEXT_DELAY() * 3;
#endif
	CvUnit* pAttackerUnit = kCombatInfo.getUnit(BATTLE_UNIT_ATTACKER);
	CvUnit* pDefenderUnit = kCombatInfo.getUnit(BATTLE_UNIT_DEFENDER);
	// Only work when unit vs unit
	if (pAttackerUnit == nullptr || pDefenderUnit == nullptr) return;
	int iUnitAttackFaithBonus = pAttackerUnit->GetUnitAttackFaithBonus();
	if(iUnitAttackFaithBonus <= 0) return;

	CvString colorString;
	CvPlayerAI& kAttackPlayer = getAttackerPlayer(kCombatInfo);
	int iAttackDamage = kCombatInfo.getDamageInflicted(BATTLE_UNIT_ATTACKER);
	iAttackDamage = iAttackDamage < pDefenderUnit->GetCurrHitPoints() ? iAttackDamage : pDefenderUnit->GetCurrHitPoints();
	int iFaithBonus = iAttackDamage * iUnitAttackFaithBonus /100;
	
	kAttackPlayer.ChangeFaith(iFaithBonus);
	if (kAttackPlayer.GetID() == GC.getGame().getActivePlayer())
	{
		char text[256] = {0};
		colorString = "[COLOR_YELLOW]+%d[ENDCOLOR][ICON_PEACE]";
		sprintf_s(text, colorString, iFaithBonus);
#if defined(SHOW_PLOT_POPUP)
		SHOW_PLOT_POPUP(pAttackerUnit->plot(), pAttackerUnit->getOwner(), text, 0.0);
#else
		GC.GetEngineUserInterface()->AddPopupText(pAttackerUnit->getX(), pAttackerUnit->getY(), text, fDelay);
#endif
	}
}
#endif