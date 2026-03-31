--******************** Improvement New Condition ********************--
alter table Improvements add RequiredAdjacentImprovement text references Improvements(Type);
alter table Improvements add RequiredAdjacentCity boolean default 0;
alter table Improvements add RemoveWhenSetNoFuture boolean default 0;
alter table Improvements add NumWaterPlotMakesValid integer default 0;
alter table Improvements add IsFreshWater boolean default 0;
alter table Improvements add NoLake boolean default 0;
alter table Improvements add NoFeature boolean default 0;
alter table Improvements add NoRemove boolean default 0;
alter table Improvements add ExtraScore integer default 0;
alter table Improvements add ForbidSameBuildUnitClasses text references UnitClasses(Type);
create table Improvement_FeaturesNeeded (
    ImprovementType text references Improvements(Type),
    FeatureType text references Features(Type)
);
--******************** Improvement New Effect ********************--
alter table Improvements add NearbyFriendHeal integer default 0;
alter table Improvements add ImprovementResource text references Resources(Type);
alter table Improvements add ImprovementResourceQuantity integer default 0;
alter table Improvements add WonderProductionModifier integer default 0;
-- Need IMPROVEMENT_TRADE_ROUTE_BONUSES
create table Improvement_TradeRouteYieldChanges (
    ImprovementType text references Improvements(Type),
    DomainType text references Domains(Type),
    YieldType text references Yields(Type),
    Yield integer not null default 0
);
-- Need IMPROVEMENTS_UNIT_XP_PER_TURN
create table Improvement_UnitXPPerTurn (
  ImprovementType text references Improvements(Type),
  Value integer not null default 0,
  -- conditional
  UnitType text default '',
  PromotionType text default ''
);
-- Need IMPROVEMENTS_YIELD_CHANGE_PER_UNIT
create table Improvement_YieldChangesPerUnit (
  ImprovementType text references Improvements(Type),
  YieldType text references Yields(Type),
  Yield integer not null default 0,
  -- conditional
  UnitType text default '',
  PromotionType text default ''
);
