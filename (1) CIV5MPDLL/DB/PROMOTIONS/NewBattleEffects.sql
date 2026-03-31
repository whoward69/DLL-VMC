alter table UnitPromotions add SplashDamagePercent integer default 0;
alter table UnitPromotions add SplashDamageFixed integer default 0;
alter table UnitPromotions add SplashDamageRadius integer default 0;
alter table UnitPromotions add SplashDamagePlotUnitLimit integer default 0;
alter table UnitPromotions add SplashDamageImmune boolean default 0;
alter table UnitPromotions add SplashXP int default 0;
alter table UnitPromotions add TriggerSplashFinish boolean default 0;

alter table UnitPromotions add CollateralDamagePercent integer default 0;
alter table UnitPromotions add CollateralDamageFixed integer default 0;
alter table UnitPromotions add CollateralDamagePlotUnitLimit integer default 0;
alter table UnitPromotions add CollateralDamageImmune boolean default 0;
alter table UnitPromotions add CollateralXP int default 0;
alter table UnitPromotions add CollateralOnlyCity boolean default 0;
alter table UnitPromotions add CollateralOnlyUnit boolean default 1;

create table PromotionCollections_AddEnemyPromotions (
    CollectionType text references PromotionCollections(Type),
    OtherCollectionType text references PromotionCollections(Type)
);
alter table UnitPromotions add AddEnemyPromotionImmune boolean default 0;

alter table UnitPromotions add DestroyBuildingCollection text default '';
alter table UnitPromotions add DestroyBuildingProbability int default 0;
alter table UnitPromotions add DestroyBuildingNumLimit int default 0;

alter table UnitPromotions add SiegeKillCitizensPercent integer default 0;
alter table UnitPromotions add SiegeKillCitizensFixed integer default 0;
alter table Buildings add SiegeKillCitizensModifier integer default 0;

alter table PromotionCollections add StackingFightBack boolean default 0;
alter table PromotionCollections add StopAttacker boolean default 0;
alter table UnitPromotions add RangeBackWhenDefense boolean default 0;

alter table UnitPromotions add HeavyChargeAddMoves integer default 0;
alter table UnitPromotions add HeavyChargeExtraDamage integer default 0;
alter table UnitPromotions add HeavyChargeCollateralFixed integer default 0;
alter table UnitPromotions add HeavyChargeCollateralPercent integer default 0;