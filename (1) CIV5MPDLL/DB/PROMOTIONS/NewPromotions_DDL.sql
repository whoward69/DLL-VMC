--******************** New Combat Bonus ********************--
alter table UnitPromotions add BarbarianCombatBonus integer default 0;
alter table UnitPromotions add MultiAttackBonus integer default 0;
alter table UnitPromotions add AntiHigherPopMod integer default 0;
alter table UnitPromotions add GoldenAgeMod integer default 0;
alter table UnitPromotions add WoundedMod integer default 0;

alter table UnitPromotions add MoveLfetAttackMod integer default 0;
alter table UnitPromotions add MoveLeftDefenseMod integer default 0;
alter table UnitPromotions add MoveUsedAttackMod integer default 0;
alter table UnitPromotions add MoveUsedDefenseMod integer default 0;
alter table UnitPromotions add NumOriginalCapitalAttackMod integer default 0;
alter table UnitPromotions add NumOriginalCapitalDefenseMod integer default 0;
alter table UnitPromotions add DoFallBackAttackMod integer default 0;
alter table UnitPromotions add BeFallBackDefenseMod integer default 0;
alter table UnitPromotions add NumSpyAttackMod integer default 0;
alter table UnitPromotions add NumSpyDefenseMod integer default 0;
alter table UnitPromotions add NumWonderAttackMod integer default 0;
alter table UnitPromotions add NumWonderDefenseMod integer default 0;
alter table UnitPromotions add NumWorkAttackMod integer default 0;
alter table UnitPromotions add NumWorkDefenseMod integer default 0;
alter table UnitPromotions add OnCapitalLandAttackMod integer default 0;
alter table UnitPromotions add OnCapitalLandDefenseMod integer default 0;
alter table UnitPromotions add OutsideCapitalLandAttackMod integer default 0;
alter table UnitPromotions add OutsideCapitalLandDefenseMod integer default 0;
alter table UnitPromotions add CurrentHitPointAttackMod integer default 0;
alter table UnitPromotions add CurrentHitPointDefenseMod integer default 0;
alter table UnitPromotions add NearNumEnemyAttackMod integer default 0;
alter table UnitPromotions add NearNumEnemyDefenseMod integer default 0;
alter table UnitPromotions add NumSpyStayAttackMod integer default 0;
alter table UnitPromotions add NumSpyStayDefenseMod integer default 0;
alter table UnitPromotions add MeleeAttackModifier integer default 0;
alter table UnitPromotions add MeleeDefenseMod integer default 0;

alter table UnitPromotions add NumAttacksMadeThisTurnAttackMod integer default 0;
alter table UnitPromotions add AttackFullyHealedMod integer default 0;
alter table UnitPromotions add AttackAbove50HealthMod integer default 0;
alter table UnitPromotions add AttackBelowEqual50HealthMod integer default 0;
alter table UnitPromotions add RangedSupportFireMod integer default 0;
alter table UnitPromotions add RangedFlankAttackModifier integer default 0;
alter table UnitPromotions add RangedFlankAttackModifierPercent integer default 0;
alter table UnitPromotions add MoraleBreakChance integer default 0;
alter table UnitPromotions add RangeSuppressModifier integer default 0;
alter table UnitPromotions add HeightModPerX integer default 0;
alter table UnitPromotions add HeightModLimited integer default 0;

-- InterceptionDamageMod / AirSweepDamageMod(only for Lua)
alter table UnitPromotions add InterceptionDamageMod integer default 0;
alter table UnitPromotions add AirSweepDamageMod integer default 0;

alter table UnitPromotions_Domains add Attack integer default 0;
alter table UnitPromotions_Domains add Defense integer default 0;
create table UnitPromotions_CombatModPerAdjacentUnitCombat (
    PromotionType text references UnitPromotions(Type),
    UnitCombatType text references UnitCombatInfos(Type),
    Modifier integer default 0,
    Attack integer default 0,
    Defense integer default 0
);
create table UnitPromotions_PromotionModifiers (
    PromotionType text references UnitPromotions(Type),
    OtherPromotionType text references UnitPromotions(Type),
    Modifier integer default 0,
    Attack integer default 0,
    Defense integer default 0
);
--******************** New Abilities ********************--
alter table UnitPromotions add IsCanParadropUnLimit boolean default 0;
alter table UnitPromotions add CanParadropAnyWhere boolean default 0;
alter table UnitPromotions add FeatureInvisible text references Features(Type);
alter table UnitPromotions add FeatureInvisible2 text references Features(Type);

alter table UnitPromotions add CanDoFallBackDamage boolean default 0; -- Splash to Defender
alter table UnitPromotions add CanDoNukeDamage boolean default 0;
alter table UnitPromotions add AoEWhileFortified integer default 0;
alter table UnitPromotions add AOEDamageOnKill integer default 0;
alter table UnitPromotions add AoEDamageOnMove integer default 0;
alter table UnitPromotions add AOEDamageOnPillage integer default 0;
alter table UnitPromotions add NearbyEnemyDamage integer default 0;
alter table UnitPromotions add TurnDamagePercent integer default 0;
alter table UnitPromotions add TurnDamage integer default 0;
alter table UnitPromotions add InsightEnemyDamageModifier integer default 0;
alter table UnitPromotions add AttackChanceFromAttackDamage text references LuaFormula(Type);
alter table UnitPromotions add MovementFromAttackDamage text references LuaFormula(Type);
alter table UnitPromotions add HealPercentFromAttackDamage text references LuaFormula(Type);

alter table UnitPromotions add StrongerDamaged boolean default 0;
alter table UnitPromotions add FightWellDamaged boolean default 0;
alter table UnitPromotions add NoResourcePunishment boolean default 0;

alter table UnitPromotions add ImmueMeleeAttack boolean default 0;
alter table UnitPromotions add CannotBeRangedAttacked boolean default 0;
alter table UnitPromotions add IgnoreDamageChance integer default 0;
alter table UnitPromotions add ForcedDamageValue integer default 0;
alter table UnitPromotions add ChangeDamageValue integer default 0;
alter table UnitPromotions add HPHealedIfDestroyEnemyGlobal integer default 0;

alter table UnitPromotions add GetGroundAttackRange integer default 0;
alter table UnitPromotions add GetGroundAttackDamage integer default 0;
alter table UnitPromotions add AirInterceptRangeChange integer default 0;

alter table UnitPromotions add MultipleInitExperence integer default 0;
alter table UnitPromotions add FreeExpPerTurn integer default 0;
alter table UnitPromotions add CarrierEXPGivenModifier integer default 0;
alter table UnitPromotions add StayCSExpPerTurn integer default 0;
alter table UnitPromotions add StayCSInfluencePerTurn integer default 0;

alter table UnitPromotions add CannotBeCaptured boolean default 0;
alter table UnitPromotions add CaptureDefeatedEnemyChance integer default 0; --fixed Value
alter table UnitPromotions add CaptureEmenyPercent integer default 0;
alter table UnitPromotions add CaptureEmenyExtraMax integer default 0;
alter table UnitPromotions add AdjacentEnemySapMovement integer default 0;
alter table UnitPromotions add AdjacentFriendlySapMovement integer default 0;
alter table UnitPromotions add AdjacentSapExperience integer default 0;

alter table UnitPromotions add Immobile boolean default 0;
alter table UnitPromotions add LostAllMovesAttackCity integer default 0;
alter table UnitPromotions add RiverDoubleMove boolean default 0;
alter table UnitPromotions add ExtraMoveTimesXX integer default 0;
alter table UnitPromotions add RangeAttackCostModifier integer default 0;
alter table UnitPromotions add SetUpCostModifier integer default 0;

alter table UnitPromotions add UnitAttackFaithBonus integer default 0;
alter table UnitPromotions add CityAttackFaithBonus integer default 0;
alter table UnitPromotions add MovePercentCaptureCity integer default 0;
alter table UnitPromotions add HealPercentCaptureCity integer default 0;
alter table UnitPromotions add RemovePromotionUpgrade text default null;

alter table UnitPromotions add CanPlunderWithoutWar boolean default 0; -- Pillage Without War
alter table UnitPromotions add PillageReplenishMoves integer default 0;
alter table UnitPromotions add PillageReplenishAttck boolean default 0;
alter table UnitPromotions add PillageReplenishHealth integer default 0;

alter table UnitPromotions add PlagueImmune boolean default 0; 
alter table UnitPromotions add PlagueChance integer default 0;
alter table UnitPromotions add PlaguePromotion text default null references UnitPromotions(Type);
alter table UnitPromotions add PlagueID integer default -1;
alter table UnitPromotions add PlaguePriority integer default 0;
alter table UnitPromotions add PlagueIDImmunity integer default -1;

alter table UnitPromotions add MaintenanceCost integer default 0;
alter table UnitPromotions add WorkRateMod integer default 0;
alter table UnitPromotions add MilitaryMightMod integer default 0;
alter table UnitPromotions add ShowInUnitPanel integer default 1;
alter table UnitPromotions add ShowInTooltip integer default 1;
alter table UnitPromotions add ShowInPedia integer default 1;

-- Allow build from promotion
create table Promotion_Builds (
	PromotionType text references UnitPromotions(Type),
	BuildType text references Builds(Type)
);
create table Promotion_RouteMovementChanges (
	PromotionType text no null references UnitPromotions(Type),
	RouteType text no null references Routes(Type),
	MovementChange integer no null
);
create table UnitPromotions_InstantYieldPerReligionFollowerConverted (
    PromotionType text references UnitPromotions(Type),
    YieldType text references Yields(Type),
    Yield integer default 0
);
--******************** Independent Promotions ********************--
alter table UnitPromotions add AllyCityStateCombatModifier integer default 0;
alter table UnitPromotions add AllyCityStateCombatModifierMax integer default -1;

alter table UnitPromotions add ExtraResourceType text references Resources(Type);
alter table UnitPromotions add ExtraResourceCombatModifier integer default 0;
alter table UnitPromotions add ExtraResourceCombatModifierMax integer default -1;

alter table UnitPromotions add ExtraHappinessCombatModifier integer default 0;
alter table UnitPromotions add ExtraHappinessCombatModifierMax integer default -1;

alter table UnitPromotions add NearbyUnitPromotionBonus integer default 0;
alter table UnitPromotions add NearbyUnitPromotionBonusRange integer default 0;
-- Zero means once, if NearbyUnitPromotionBonusMax = 0, result max = NearbyUnitPromotionBonus
alter table UnitPromotions add NearbyUnitPromotionBonusMax integer default 0;
alter table UnitPromotions add CombatBonusFromNearbyUnitPromotion text references UnitPromotions(Type);
--******************** Aura Promotion ********************--
-- Add Promotion to other Units in Range, need CustomModOptions PROMOTION_AURA_PROMOTION to make it valid
alter table UnitPromotions add AuraPromotionType text default null references UnitPromotions(Type);
alter table UnitPromotions add AuraPromotionRange integer default 0;
alter table UnitPromotions add AuraPromotionRangeAIBonus integer default 0;
alter table UnitPromotions add AuraPromotionNoSelf boolean default 0;
-- Only Valid Domain Units can get Promotions
create table Promotion_AuraPromotionDomains (
    PromotionType text references UnitPromotions(Type),
    DomainType text references Domains(Type)
);
-- Can be Empty
create table Promotion_AuraPromotionPrePromotionOr (
    PromotionType text references UnitPromotions(Type),
    PrePromotionType text references UnitPromotions(Type)
);
-- Add Promotions when providers that have a complex number, at least 1
create table Promotion_AuraPromotionProviderNum (
    PromotionType text references UnitPromotions(Type),
    AuraPromotionType text references UnitPromotions(Type),
    ProviderNum integer default 0
);
--******************** Promotion New Condition ********************--
alter table UnitPromotions add PromotionPrereqOr10 text default null;
alter table UnitPromotions add PromotionPrereqOr11 text default null;
alter table UnitPromotions add PromotionPrereqOr12 text default null;
alter table UnitPromotions add PromotionPrereqOr13 text default null;
alter table UnitPromotions add MutuallyExclusiveGroup integer default -1;
create table UnitPromotions_Promotions (
    FreePromotionType text references UnitPromotions(Type),
    PrePromotionType text references UnitPromotions(Type)
);
--Must have all needed promotions to unlock Promotion
create table Promotion_PromotionPrereqAnds (
	PromotionType text references UnitPromotions(Type),
	PrereqPromotionType text references UnitPromotions(Type)
);
--Must any Exclusion promotions will lock Promotion，not two-way
create table Promotion_PromotionExclusionAny (
	PromotionType text references UnitPromotions(Type),
	ExclusionPromotionType text references UnitPromotions(Type)
);
-- Only for check promotion valid
create table Promotion_UnitCombatsPromotionValid (
	PromotionType text references UnitPromotions(Type),
	UnitCombatType text references UnitCombatInfos(Type)
);
create table UnitPromotions_PromotionUpgrade (
    PromotionType text references UnitPromotions(Type),
    JudgePromotionType text references UnitPromotions(Type),
    NewPromotionType text references UnitPromotions(Type)
);
--Have any needed promotions will unlock Promotion
create table Promotion_PromotionPrereqOrs (
	PromotionType text references UnitPromotions(Type),
	PrereqPromotionType text references UnitPromotions(Type)
);
