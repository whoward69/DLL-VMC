alter table UnitPromotions add AttackInflictDamageChange integer default 0;
alter table UnitPromotions add AttackInflictDamageChangeMaxHPPercent integer default 0;

alter table UnitPromotions add DefenseInflictDamageChange integer default 0;
alter table UnitPromotions add DefenseInflictDamageChangeMaxHPPercent integer default 0;

alter table UnitPromotions add SiegeInflictDamageChange integer default 0;
alter table UnitPromotions add SiegeInflictDamageChangeMaxHPPercent integer default 0;

alter table UnitPromotions add OutsideFriendlyLandsInflictDamageChange integer default 0;

alter table UnitPromotions add OriginalCapitalDamageFix integer default 0;
alter table UnitPromotions add OriginalCapitalSpecialDamageFix integer default 0;