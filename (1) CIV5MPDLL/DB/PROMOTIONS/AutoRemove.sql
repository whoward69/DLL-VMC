alter table UnitPromotions add RemoveAfterXTurns integer default 0;
alter table UnitPromotions add AutoRemoveDoneTurn boolean default 0;
alter table UnitPromotions add RemoveAfterFullyHeal boolean default 0;
alter table UnitPromotions add RemoveWithLuaCheck boolean default 0;
alter table UnitPromotions add CanActionClear boolean default 0;

alter table Improvements add ClearNegativePromotions integer default 0;