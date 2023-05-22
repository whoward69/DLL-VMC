alter table UnitPromotions add column 'AttackInflictDamageChange' integer not null default 0;
alter table UnitPromotions add column 'AttackInflictDamageChangeMaxHPPercent' integer not null default 0;

alter table UnitPromotions add column 'DefenseInflictDamageChange' integer not null default 0;
alter table UnitPromotions add column 'DefenseInflictDamageChangeMaxHPPercent' integer not null default 0;

alter table UnitPromotions add column 'SiegeInflictDamageChange' integer not null default 0;
alter table UnitPromotions add column 'SiegeInflictDamageChangeMaxHPPercent' integer not null default 0;