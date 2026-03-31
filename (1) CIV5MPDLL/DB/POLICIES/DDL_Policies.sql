--******************** New Yield Bonus ********************--
alter table Policies add ScienceModifierFromRANum integer default 0;
create table Policy_CityNumberCityYieldModifier (
    PolicyType text references Policies(Type),
    YieldType text references Yields(Type),
    Yield integer default 0
);
create table Policy_CityWithWorldWonderYieldModifier (
    PolicyType text references Policies(Type),
    YieldType text references Yields(Type),
    Yield integer default 0
);
create table Policy_YieldModifierPerArtifacts (
    PolicyType text references Policies(Type),
    YieldType text references Yields(Type),
    Yield integer default 0
);
create table Policy_CityLoveKingDayYieldMod (
    PolicyType text references Policies(Type),
    YieldType text references Yields(Type),
    Yield integer default 0
);
create table Policy_TradeRouteCityYieldModifier (
    PolicyType text references Policies(Type),
    YieldType text references Yields(Type),
    Yield integer default 0
);
create table Policy_MinorsTradeRouteYieldRate (
    PolicyType text references Policies(Type),
    YieldType text references Yields(Type),
    Rate integer default 0
);
create table Policy_InternalTradeRouteDestYieldRate (
    PolicyType text references Policies(Type),
    YieldType text references Yields(Type),
    Rate integer default 0
);
create table Policy_HappinessYieldModifier (
    PolicyType text references Policies(Type),
    YieldType text references Yields(Type),
    YieldFormula text references LuaFormula(Type)
);
--******************** New Instant Effect ********************--
alter table Policies add FreePopulation integer default 0;
alter table Policies add FreePopulationCapital integer default 0;
alter table Policies add InstantFoodThresholdPercent integer not null default 0;
alter table Policies add InstantFoodKeptPercent integer not null default 0;
alter table Policies add FreeBuildingClass text default null references BuildingClasses(Type);
alter table Policies add ExtraSpies integer default 0;
--******************** New Effect ********************--
alter table Policies add DefenseBoostAllCities integer default 0;
alter table Policies add AlwaysWeLoveKindDayInGoldenAge boolean default 0;

alter table Policies add MinorLocalBullyScoreModifier integer default 0;
alter table Policies add MinorAllyBullyScoreModifier integer default 0;
alter table Policies add MinorBullyInfluenceLossModifier int default 0;
alter table Policies add RiggingElectionInfluenceModifier integer default 0;
alter table Policies add SpyLevelUpWhenRigging boolean default 0;

alter table Policies add IdeologyPressureModifier integer not null default 0;
alter table Policies add IdeologyUnhappinessModifier integer not null default 0;
alter table Policies add NullifyInfluenceModifier boolean default 0;
alter table Policies add DifferentIdeologyTourismModifier integer default 0;
alter table Policies add TourismModifierPerGPCreation boolean default 0;
alter table Policies add DiplomatPropagandaModifier integer default 0;

alter table Policies add GlobalHappinessFromFaithPercent integer not null default 0;
alter table Policies add HappinessInWLTKDCities integer not null default 0;
alter table Policies add HappinessPerReligionInCity integer default 0;
alter table Policies add HappinessPerPolicy integer default 0;

alter table Policies add NoResistance boolean default 0;
alter table Policies add UpgradeAllTerritory boolean default 0;
alter table Policies add CityCaptureHealGlobal integer default 0;
alter table Policies add ConquestCasualtiesModifier integer default 0;
alter table Policies add OriginalCapitalCaptureTech integer default 0;
alter table Policies add OriginalCapitalCapturePolicy integer default 0;
alter table Policies add OriginalCapitalCaptureGreatPerson integer default 0;
alter table Policies add CaptureCityResistanceTurnsChangeFormula text references LuaFormula(Type);
alter table Policies add NoOccupiedUnhappinessGarrisonedCity boolean default 0;

alter table Policies add FreePromotionRemoved integer default -1;
alter table Policies add RemoveCurrentPromotion boolean default 0;
alter table Policies add RemoveOceanImpassableCombatUnit boolean default 0;

alter table Policies add WaterBuildSpeedModifier integer default 0;
create table Policy_BuildSpeedModifier (
    PolicyType text references Policies(Type),
    BuildType text references Builds(Type),
    Modifier integer not null
);

alter table Policies add SettlerPopConsume boolean default 0;
alter table Policies add SettlerProductionEraModifier integer default 0;
alter table Policies add SettlerProductionStartEra text default null references Eras(Type);

alter table Policies add NumTradeRouteBonus integer default 0;
alter table Policies add CapitalTradeRouteGoldChange integer default 0;
alter table Policies add CapitalTradeRouteRangeChange integer default 0;

alter table Policies add ReligionProductionModifier integer default 0;
alter table Policies add NationalWonderCityCostModifier integer default 0;
alter table Policies add CityExtraProductionCount integer default 0;
alter table Policies add DeepWaterNavalStrengthCultureModifier integer default 0;

alter table Policies add NoTechForWonder boolean default 0;
alter table Policies add NoTechForProject boolean default 0;

alter table Policies add GreatScientistBeakerPolicyModifier integer default 0;
alter table Policies add ProductionBeakerMod integer default 0;
create table Policy_GreatPersonOutputModifierPerGWs (
    PolicyType text references Policies(Type),
    GreatPersonType text references GreatPersons(Type),
    Modifier integer default 0
);
--******************** New Condition ********************--
create table PolicyBranch_CivilizationLocked (
    PolicyBranchType text references PolicyBranchTypes(Type),
    CivilizationType text references Civilizations(Type)
);