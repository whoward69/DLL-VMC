alter table UnitPromotions add column RemoveAfterXTurns integer default 0 not null;
alter table UnitPromotions add column RemoveAfterFullyHeal boolean default 0 not null;
alter table UnitPromotions add column RemoveWithLuaCheck boolean default 0 not null;
alter table UnitPromotions add column CanActionClear boolean default 0 not null;