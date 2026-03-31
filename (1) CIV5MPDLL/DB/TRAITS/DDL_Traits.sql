--******************** New Yield Bonus ********************--
alter table Traits add TradeRouteLandGoldBonus int default 0;
alter table Traits add TradeRouteSeaGoldBonus int default 0;
alter table Traits add GreatWorksTourism int default 0;
alter table Traits add ShareAllyResearchPercent int default 0;
alter table Traits add CultureBonusUnitStrengthModify int default 0;
alter table Traits add OthersTradeBonusModifier int default 0;
create table Trait_CityYieldPerAdjacentFeature (
    TraitType text references Traits(Type),
    FeatureType text references Features(Type),
    YieldType text references Yields(Type),
    MaxValue int default 0
);
create table Trait_RiverPlotYieldChanges (
	TraitType text references Traits(Type),
	YieldType text not null references Yields(Type),
	Yield integer default 0
);
create table Trait_PerMajorReligionFollowerYieldModifier (
    TraitType text references Traits(Type),
    YieldType text references Yields(Type),
    Yield int default 0
);
create table Trait_PerMajorReligionFollowerYieldModifierTimes100 (
    TraitType text references Traits(Type),
    YieldType text references Yields(Type),
    Yield int default 0
);
create table Trait_PerMajorReligionFollowerYieldModifierMax (
    TraitType text references Traits(Type),
    YieldType text references Yields(Type),
    Max int default 0
);
create table Trait_CityYieldModifierFromAdjacentFeature (
    TraitType text references Traits(Type),
    FeatureType text references Features(Type),
    YieldType text references Yields(Type),
    Yield int default 0
);
create table Trait_GoldenAgeYieldModifiers (
    TraitType text references Traits(Type),
    YieldType text references Yields(Type),
    Yield integer default 0
);
--******************** New Unit/Combat Bonus ********************--
alter table Traits add TrainedAll boolean default 0;
alter table Traits add NoDoDeficit boolean default 0;
alter table Traits add FreeGreatPeoplePerEra integer default 0;
alter table Traits add OwnedReligionUnitCultureExtraTurns int default 0;
alter table Traits add InfluenceFromGreatPeopleBirth integer default 0;
alter table Traits add ExtraUnitPlayerInstances integer default 0;
alter table Traits add ArtistGoldenAgeTechBoost boolean default 0;
alter table Traits add GoodyUnitUpgradeFirst boolean default 0;

alter table Traits add UnitMaxHitPointChangePerRazedCityPop int default 0;
alter table Traits add UnitMaxHitPointChangePerRazedCityPopLimit int default 0;

alter table Traits add AllyCityStateCombatModifier int default 0;
alter table Traits add AllyCityStateCombatModifierMax int default -1;
alter table Traits add AttackBonusAdjacentWhenUnitKilled integer default 0;
alter table Traits add KilledAttackBonusDecreasePerTurn integer default 0;
alter table Traits add AwayFromCapitalCombatModifier integer default 0;
alter table Traits add AwayFromCapitalCombatModifierMax integer default 0;

alter table Traits add PromotionWhenKilledUnit text default null;
alter table Traits add PromotionRadiusWhenKilledUnit integer default 0;
alter table Traits add CiviliansFreePromotion text default null;
create table Trait_FreePromotionUnitClasses(
    TraitType text references Traits(Type),
    UnitClassType text references UnitClasses(Type),
    PromotionType text references UnitPromotions(Type)
);
--******************** New War Bonus ********************--
alter table Traits add NoResistance boolean default 0;
alter table Traits add GoldenAgeOnWar boolean default 0;
alter table Traits add CanConquerUC boolean default 0;
alter table Traits add FreePolicyWhenFirstConquerMajorCapital int default 0;
alter table Traits add InstantTourismBombWhenFirstConquerMajorCapital int default 0; -- apply tourism (x turn) pressure to all civs
alter table Traits add ConquestCasualtiesModifier int default 0;

--******************** Other Bonus ********************--
alter table Traits add AbleToDualEmpire boolean default 0;
alter table Traits add CanDiplomaticMarriage boolean default 0;
alter table Traits add BuyOwnedTiles boolean default 0;
alter table Traits add CanFoundMountainCity boolean default 0;
alter table Traits add CanFoundCoastCity boolean default 0;
alter table Traits add CanPurchaseWonderInGoldenAge boolean default 0;
INSERT INTO Defines(Name, Value) VALUES('WONDER_GOLDEN_AGE_PURCHASE_MODIFIER', 200);
alter table Traits add NumFreeWorldWonderPerCity integer default 0;
alter table Traits add TriggersIdeologyTech text default null;
alter table Traits add ExceedingHappinessImmigrationModifier integer default 0;

alter table Traits add GoldenAgeResearchTotalCostModifier int default 0;
alter table Traits add GoldenAgeResearchCityCountCostModifier int default 0;
alter table Traits add GoldenAgeGrowThresholdModifier int default 0;
alter table Traits add GoldenAgeMinorPerTurnInfluence int default 0;
alter table Traits add AdequateLuxuryCompleteQuestInfluenceModifier int default 0;
alter table Traits add AdequateLuxuryCompleteQuestInfluenceModifierMax int default -1;

alter table Traits add WLKDLengthChangeModifier integer default 0;
alter table Traits add WLKDCityNoResearchCost boolean default 0;

create table Trait_BuildingClassFaithCost (
    TraitType text references Traits(Type),
    BuildingClassType text references BuildingClasses(Type),
    Cost int default 0
);

alter table Civilizations add SpecialGAText text default 'TXT_KEY_GOLDEN_AGE_ANNOUNCE';
alter table Civilizations add SpecialGAHelpText text default 'TXT_KEY_TP_GOLDEN_AGE_EFFECT';