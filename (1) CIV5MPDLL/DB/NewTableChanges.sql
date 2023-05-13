ALTER TABLE Processes ADD COLUMN 'DefenseValue' INTEGER DEFAULT 0;

ALTER TABLE Builds ADD COLUMN 'ObsoleteTech' TEXT DEFAULT NULL;

ALTER TABLE UnitPromotions ADD COLUMN 'CannotBeRangedAttacked' BOOLEAN DEFAULT 0 NOT NULL; 
ALTER TABLE UnitPromotions ADD COLUMN 'AoEDamageOnMove' INTEGER DEFAULT 0; 
ALTER TABLE UnitPromotions ADD COLUMN 'ForcedDamageValue' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD COLUMN 'ChangeDamageValue' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD COLUMN 'AttackFullyHealedMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD COLUMN 'AttackAbove50HealthMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD COLUMN 'AttackBelowEqual50HealthMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD COLUMN 'StrongerDamaged' BOOLEAN DEFAULT 0;  
ALTER TABLE UnitPromotions ADD COLUMN 'FightWellDamaged' BOOLEAN DEFAULT 0; 
ALTER TABLE UnitPromotions ADD COLUMN 'NearbyUnitClassBonus' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD COLUMN 'NearbyUnitClassBonusRange' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD COLUMN 'CombatBonusFromNearbyUnitClass' INTEGER DEFAULT -1;
ALTER TABLE UnitPromotions ADD COLUMN 'GoldenAgeMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD COLUMN 'RangedSupportFireMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD COLUMN 'MeleeDefenseMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD COLUMN 'MoveLfetAttackMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD COLUMN 'MoveUsedAttackMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD 'HPHealedIfDestroyEnemyGlobal' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD 'NumOriginalCapitalAttackMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD 'NumOriginalCapitalDefenseMod' INTEGER DEFAULT 0;

ALTER TABLE UnitPromotions_Domains ADD COLUMN 'Attack' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions_Domains ADD COLUMN 'Defense' INTEGER DEFAULT 0;

ALTER TABLE Buildings ADD COLUMN 'PopulationChange' INTEGER DEFAULT 0;
ALTER TABLE Buildings ADD COLUMN 'RangedStrikeModifier' INTEGER DEFAULT 0;
ALTER TABLE Buildings ADD COLUMN 'ResetDamageValue' INTEGER DEFAULT 0;
ALTER TABLE Buildings ADD COLUMN 'ReduceDamageValue' INTEGER DEFAULT 0;
ALTER TABLE Buildings ADD COLUMN 'GlobalCityStrengthMod' INTEGER DEFAULT 0;
ALTER TABLE Buildings ADD COLUMN 'GlobalRangedStrikeModifier' INTEGER DEFAULT 0;
ALTER TABLE Buildings ADD COLUMN 'NukeInterceptionChance' INTEGER DEFAULT 0;
ALTER TABLE Buildings ADD COLUMN 'ExtraDamageHeal' INTEGER DEFAULT 0;
ALTER TABLE Buildings ADD COLUMN 'ExtraAttacks' INTEGER DEFAULT 0;

ALTER TABLE Improvements ADD COLUMN 'NearbyFriendHeal' INTEGER DEFAULT 0;
ALTER TABLE Improvements ADD COLUMN 'ImprovementResource' TEXT DEFAULT NULL;
ALTER TABLE Improvements ADD COLUMN 'ImprovementResourceQuantity' INTEGER DEFAULT 0;

INSERT INTO Defines(Name, Value) VALUES('ORIGINAL_CAPITAL_MODMAX', 10);

CREATE TABLE IF NOT EXISTS UnitPromotions_PromotionModifiers (
    `PromotionType` TEXT NOT NULL,
    `OtherPromotionType` TEXT NOT NULL,
    `Modifier` integer default 0 NOT NULL,
    `Attack` integer default 0 NOT NULL,
    `Defense` integer default 0 NOT NULL
);

ALTER TABLE UnitPromotions ADD 'NumSpyDefenseMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD 'NumSpyAttackMod' INTEGER DEFAULT 0;

ALTER TABLE UnitPromotions ADD 'NumWonderDefenseMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD 'NumWonderAttackMod' INTEGER DEFAULT 0;

ALTER TABLE UnitPromotions ADD 'NumWorkDefenseMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD 'NumWorkAttackMod' INTEGER DEFAULT 0;

ALTER TABLE UnitPromotions ADD COLUMN 'NoResourcePunishment' BOOLEAN DEFAULT 0;

ALTER TABLE UnitPromotions ADD 'OnCapitalLandAttackMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD 'OutsideCapitalLandAttackMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD 'OnCapitalLandDefenseMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD 'OutsideCapitalLandDefenseMod' INTEGER DEFAULT 0;



ALTER TABLE UnitPromotions ADD 'CurrentHitPointAttackMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD 'CurrentHitPointDefenseMod' INTEGER DEFAULT 0;

ALTER TABLE UnitPromotions ADD 'NearNumEnemyAttackMod' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD 'NearNumEnemyDefenseMod' INTEGER DEFAULT 0;

ALTER TABLE Units ADD 'ExtraXPValueAttack' INTEGER DEFAULT 0;
ALTER TABLE Units ADD 'ExtraXPValueDefense' INTEGER DEFAULT 0;