--******************** New Conditions ********************--
alter table Buildings add PurchaseOnly boolean default 0;
alter table Buildings add AllowsPuppetPurchase boolean default 0;
alter table Buildings add HumanOnly boolean default 0;
alter table Buildings add AnyWater boolean default 0;
alter table Buildings add RiverOrCoastal boolean default 0;
alter table Buildings add MinNumReligions int not null default 0;
alter table Buildings add NoPuppet boolean default 0;
alter table Buildings add CapitalOnly boolean default 0;
alter table Buildings add OriginalCapitalOnly boolean default 0;
alter table Buildings add NotNeedOccupied boolean default 0;
alter table Buildings add PolicyNeededType text default null references Policies(Type);
alter table Buildings add TechNoPrereqClasses text default null references Technologies(Type);
create table Building_BuildingsNeededInCity (
	BuildingType text not null references Buildings(Type),
	PreBuildingType text not null references Buildings(Type)
);
create table Building_BuildingsNeededGlobal (
	BuildingType text not null references Buildings(Type),
	PreBuildingType text not null references Buildings(Type)
);
create table Building_ClassesNeededGlobal (
	BuildingType text not null references Buildings(Type),
	BuildingClassType text not null references BuildingClasses(Type)
);
create table Building_LocalPlotAnds (
	BuildingType text not null references Buildings(Type),
	PlotType text not null references Plots(Type)
);
create table Building_LocalFeatureOrs (
	BuildingType text not null references Buildings(Type),
	FeatureType text not null references Features(Type)
);
create table Building_LocalFeatureAnds (
	BuildingType text not null references Buildings(Type),
	FeatureType text not null references Features(Type)
);
create table Building_EmpireResourceAnds (
	BuildingType text not null references Buildings(Type),
	ResourceType text not null references Resources(Type)
);
create table Building_EmpireResourceOrs (
	BuildingType text not null references Buildings(Type),
	ResourceType text not null references Resources(Type)
);
--******************** New Instant Yield ********************--
alter table Buildings add InstantResearchFromFriendlyGreatScientist integer default 0;
alter table Buildings add AllowInstantYield boolean default 0; -- only for Building_InstantYield
create table Building_InstantYield (
	BuildingType text not null references Buildings(Type),
	YieldType integer not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_YieldFromBirth (
	BuildingType text not null references Buildings(Type),
	YieldType integer not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_YieldFromConstruction (
	BuildingType text not null references Buildings(Type),
	YieldType integer not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_YieldFromUnitProduction (
	BuildingType text not null references Buildings(Type),
	YieldType integer not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_YieldFromBorderGrowth (
	BuildingType text not null references Buildings(Type),
	YieldType integer not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_YieldFromPillage (
	BuildingType text not null references Buildings(Type),
	YieldType integer not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_YieldFromPillageGlobal (
	BuildingType text not null references Buildings(Type),
	YieldType integer not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_YieldFromPillageGlobalPlayer (
	BuildingType text not null references Buildings(Type),
	YieldType integer not null references Yields(Type),
	Yield integer not null default 0
);
--******************** New Yield ********************--
create table Building_YieldFromOtherYield (
	BuildingType text not null references Buildings(Type),
	InYieldType text not null references Yields(Type),
	InYieldValue integer not null default 0,
	OutYieldType text not null references Yields(Type),
	OutYieldValue integer not null default 0
);
create table Building_YieldFromYieldPercentGlobal (
	BuildingType text not null references Buildings(Type),
	YieldOut integer not null references Yields(Type),
	YieldIn integer not null references Yields(Type),
	Value integer not null default 0
);
create table Building_YieldChangesPerEra (
	BuildingType text not null references Buildings(Type),
	YieldType text not null references Yields(Type),
	Yield integer default 0
);
create table Building_YieldFromInternalTR (
	BuildingType text not null references Buildings(Type),
	YieldType integer not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_PlotYieldChanges (
	BuildingType text not null references Buildings(Type),
	PlotType text not null references Plots(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_RiverPlotYieldChangesGlobal (
	BuildingType text not null references Buildings(Type),
	YieldType text not null references Yields(Type),
	Yield integer default 0
);
create table Building_ResourceYieldChangesGlobal (
	BuildingType text not null references Buildings(Type),
	ResourceType text not null references Resources(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_YieldPerXFeatureTimes100 (
	BuildingType text not null references Buildings(Type),
	FeatureType text not null references Features(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_FeatureYieldChangesGlobal (
	BuildingType text not null references Buildings(Type),
	FeatureType text not null references Features(Type),
	YieldType integer not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_ImprovementYieldChanges (
	BuildingType text not null references Buildings(Type),
	ImprovementType text not null references Improvements(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_ImprovementYieldChangesGlobal (
	BuildingType text not null references Buildings(Type),
	ImprovementType text not null references Improvements(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_YieldPerXTerrainTimes100 (
	BuildingType text not null references Buildings(Type),
	TerrainType text not null references Terrains(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_TerrainYieldChangesGlobal (
	BuildingType text not null references Buildings(Type),
	TerrainType text not null references Terrains(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_YieldChangeWorldWonder (
	BuildingType text not null references Buildings(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_YieldChangeWorldWonderGlobal (
	BuildingType text not null references Buildings(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_SpecialistYieldChangesLocal (
	BuildingType text not null references Buildings(Type),
	SpecialistType text not null references Specialists(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_GreatWorkYieldChanges (
	BuildingType text not null references Buildings(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_YieldPerEspionageSpy (
	BuildingType text not null references Buildings(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_YieldPerFriend (
	BuildingType text not null references Buildings(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_YieldPerAlly (
	BuildingType text not null references Buildings(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
-- This is a percent, eg 100 will give the same number of yields.
create table Building_YieldChangesPerPopInEmpire (
	BuildingType text not null references Buildings(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_BuildingClassLocalYieldChanges (
	BuildingType text not null references Buildings(Type),
	BuildingClassType text not null references BuildingClasses(Type),
	YieldType integer not null references Yields(Type),
	YieldChange integer not null default 0
);
--******************** Trade Yiled ********************--
create table Building_TradeRouteFromTheCityYields (
	BuildingType text not null references Buildings(Type),
	YieldType text not null references Yields(Type),
	YieldValue integer default 0
);
create table Building_TradeRouteFromTheCityYieldsPerEra (
	BuildingType text not null references Buildings(Type),
	YieldType text not null references Yields(Type),
	YieldValue integer default 0
);
--******************** New Modifier ********************--
alter table Buildings add ResearchTotalCostModifierGoldenAge integer default 0;
alter table Buildings add ResearchTotalCostModifier integer default 0;
alter table Buildings add TradeRouteRiverBonusModifier integer default 0;
alter table Buildings add TradeRouteSeaGoldBonusGlobal integer default 0;
alter table Buildings add TradeRouteLandGoldBonusGlobal integer default 0;
alter table Buildings add LandmarksTourismPercentGlobal integer default 0;
alter table Buildings add GreatWorksTourismModifierGlobal integer default 0;
alter table Buildings add CityStateTradeRouteProductionModifierGlobal integer default 0;

alter table Buildings add GlobalGrowthFoodNeededModifier integer default 0;

alter table Buildings add GlobalProductionNeededUnitModifier integer default 0;
alter table Buildings add GlobalProductionNeededBuildingModifier integer default 0;
alter table Buildings add GlobalProductionNeededProjectModifier integer default 0;
alter table Buildings add GlobalProductionNeededUnitMax integer default 0;
alter table Buildings add GlobalProductionNeededBuildingMax integer default 0;
alter table Buildings add GlobalProductionNeededProjectMax integer default 0;

create table Building_YieldModifiersChangesPerEra (
	BuildingType text not null references Buildings(Type),
	YieldType text not null references Yields(Type),
	Yield integer default 0
);
create table Building_CityStateTradeRouteYieldModifiers (
	BuildingType text not null references Buildings(Type),
	YieldType text not null references Yields(Type),
	Yield integer default 0
);
create table Building_CityStateTradeRouteYieldModifiersGlobal (
	BuildingType text not null references Buildings(Type),
	YieldType text not null references Yields(Type),
	Yield integer default 0
);
create table Building_FeatureYieldModifiers (
	BuildingType text not null references Buildings(Type),
	FeatureType text not null references Features(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_ImprovementYieldModifiers (
	BuildingType text not null references Buildings(Type),
	ImprovementType text not null references Improvements(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_TerrainYieldModifier (
	BuildingType text not null references Buildings(Type),
	TerrainType text not null references Terrains(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_BuildingClassYieldModifiers (
	BuildingType text not null references Buildings(Type),
	BuildingClassType text not null references BuildingClasses(Type),
	YieldType integer not null references Yields(Type),
	Modifier integer not null default 0
);
create table Building_SpecialistYieldModifiers (
	BuildingType text not null references Buildings(Type),
	SpecialistType text not null references Specialists(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_SpecialistYieldModifiersGlobal (
	BuildingType text not null references Buildings(Type),
	SpecialistType text not null references Specialists(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_YieldFromProcessModifier (
	BuildingType text not null references Buildings(Type),
	YieldType integer not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_YieldFromProcessModifierGlobal (
	BuildingType text not null references Buildings(Type),
	YieldType integer not null references Yields(Type),
	Yield integer not null default 0
);
create table Building_CityWithWorldWonderYieldModifierGlobal (
	BuildingType text not null references Buildings(Type),
	YieldType text not null references Yields(Type),
	Yield integer not null default 0
);
--******************** New Multiplier ********************--
create table Building_YieldMultiplier (
	BuildingType text not null references Buildings(Type),
	YieldType integer not null references Yields(Type),
	Yield integer not null default 0
);
--******************** New Unit Effect ********************--
alter table Buildings add FreePromotion2 text default null;
alter table Buildings add FreePromotion3 text default null;
alter table Buildings add MoveAfterCreated integer default 0;
alter table Buildings add ExtraUnitPlayerInstances integer default 0;
alter table Buildings add GoldenAgeUnitCombatModifier int not null default 0;
alter table Buildings add UnitMaxExperienceLocal integer default 0;
alter table Buildings add AllowSpaceshipLaunch boolean default 0;

create table Building_DomainFreeExperiencesGlobal (
	BuildingType text not null references Buildings(Type),
	DomainType text not null references Domains(Type),
	Experience integer not null default 0
);
create table Building_DomainFreeExperiencesPerPop (
	BuildingType text not null references Buildings(Type),
	DomainType text not null references Domains(Type),
	Modifier integer default 0
);
create table Building_DomainFreeExperiencesPerPopGlobal (
	BuildingType text not null references Buildings(Type),
	DomainType text not null references Domains(Type),
	Modifier integer default 0
);
create table Building_DomainFreeExperiencePerGreatWorkGlobal (
	BuildingType text not null references Buildings(Type),
	DomainType text not null references Domains(Type),
	Experience integer not null default 0
);
create table Building_DomainFreeExperiencesPerTurn (
	BuildingType text not null references Buildings(Type),
	DomainType text not null references Domains(Type),
	Value integer default 0
);
create table Building_DomainFreeExperiencesPerTurnGlobal (
	BuildingType text not null references Buildings(Type),
	DomainType text not null references Domains(Type),
	Value integer default 0
);

create table Building_DomainEnemyCombatModifier (
	BuildingType text not null references Buildings(Type),
	DomainType text not null references Domains(Type),
	Modifier integer default 0
);
create table Building_DomainEnemyCombatModifierGlobal (
	BuildingType text not null references Buildings(Type),
	DomainType text not null references Domains(Type),
	Modifier integer default 0
);
create table Building_DomainFriendsCombatModifierLocal (
	BuildingType text not null references Buildings(Type),
	DomainType text not null references Domains(Type),
	Modifier integer default 0
);
create table Building_UnitTypePrmoteHealGlobal (
	BuildingType text not null references Buildings(Type),
	UnitType text not null references Units(Type),
	Heal integer not null default 0
);
create table Building_UnitClassMaxInstances (
	BuildingType text not null references Buildings(Type),
	UnitClassType text not null references UnitClasses(Type),
	ExtraMax integer default 0
);
--******************** New City Defense ********************--
alter table Buildings add ExtraAttacks integer default 0;
alter table Buildings add BombardRange integer default 0;
alter table Buildings add BombardIndirect integer default 0;
alter table Buildings add CityCollateralDamage boolean default 0;
alter table Buildings add RangedStrikeModifier integer default 0;
alter table Buildings add GlobalRangedStrikeModifier integer default 0;
alter table Buildings add WaterTileDamage integer default 0;
alter table Buildings add WaterTileMovementReduce integer default 0;
alter table Buildings add WaterTileTurnDamage integer default 0;
alter table Buildings add LandTileDamage integer default 0;
alter table Buildings add LandTileMovementReduce integer default 0;
alter table Buildings add LandTileTurnDamage integer default 0;
alter table Buildings add WaterTileDamageGlobal integer default 0;
alter table Buildings add WaterTileMovementReduceGlobal integer default 0;
alter table Buildings add WaterTileTurnDamageGlobal integer default 0;
alter table Buildings add LandTileDamageGlobal integer default 0;
alter table Buildings add LandTileMovementReduceGlobal integer default 0;
alter table Buildings add LandTileTurnDamageGlobal integer default 0;

alter table Buildings add CityDefenseModifierGlobal integer default 0;
alter table Buildings add GlobalCityStrengthMod integer default 0;
alter table Buildings add ResetDamageValue integer default 0;
alter table Buildings add ReduceDamageValue integer default 0;
alter table Buildings add NukeInterceptionChance integer default 0;
alter table Buildings add ExtraDamageHeal integer default 0;
alter table Buildings add ExtraDamageHealPercent integer default 0;
alter table Buildings add ForbiddenForeignSpyGlobal boolean default 0;
alter table Buildings add ForbiddenForeignSpy boolean default 0;
alter table Buildings add ImmueVolcanoDamage integer default 0;
--******************** New Other Effect ********************--
alter table Buildings add DummyBuilding boolean default 0;
alter table Buildings add ExtraSellRefund integer default 0;
alter table Buildings add ExtraSellRefundModifierPerEra integer default 0;

alter table Buildings add PuppetPurchaseOverride boolean default 0;
alter table Buildings add AddsFreshWater integer default 0;
alter table Buildings add PopulationChange integer default 0;
alter table Buildings add FoodKeptFromPollution integer default 0;
alter table Buildings add GoldenAgeMeterMod int not null default 0;
alter table Buildings add GlobalEspionageSpeedModifier integer default 0;

alter table Buildings add MinorFriendshipAnchorChange int not null default 0;
alter table Buildings add MinorQuestFriendshipMod int not null default 0;
alter table Buildings add MinorCivFriendship integer default 0;
alter table Buildings add LiberatedInfluence integer default 0;

-- For World Power
alter table Buildings add PlagueMod integer default 0;
alter table Buildings add PlagueModGlobal integer default 0;

create table Building_EnableUnitPurchase (
	BuildingType text not null references Buildings(Type),
	UnitClassType text not null references UnitClasses(Type),
	YieldType text not null references Yields(Type),
	CostModifier integer not null default -1
);
create table Building_FreeSpecUnits (
	BuildingType text not null references Buildings(Type),
	UnitType text not null references Units(Type),
	NumUnits integer not null default 0
);
create table Building_ResourceQuantityFromPOP (
	BuildingType text not null references Buildings(Type),
	ResourceType text not null references Resources(Type),
	Modifier integer not null default 0
);
create table Building_ResourceFromImprovement (
	BuildingType text not null references Buildings(Type),
	ResourceType text not null references Resources(Type),
	ImprovementType text not null references Improvements(Type),
	Value integer not null default 0
);
create table Building_HurryModifiersLocal (
	BuildingType text not null references Buildings(Type),
	HurryType text not null references HurryInfos(Type),
	HurryCostModifier integer not null default 0
);
create table Building_SpecificGreatPersonRateModifier (
	BuildingType text not null references Buildings(Type),
	SpecialistType text not null references Specialists(Type),
	Modifier integer not null default 0
);