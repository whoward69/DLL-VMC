alter table Resources add column UnHappinessModifierFormula text null references LuaFormula(Type);
alter table Resources add column CityConnectionTradeRouteGoldModifierFormula text null references LuaFormula(Type);
alter table Resources add column GoldHurryCostModifierFormula text null references LuaFormula(Type);

alter table Policies add column ResourceCityConnectionTradeRouteGoldModifier integer not null default 0;
alter table Policies add column ResourceUnhappinessModifier integer not null default 0;

create table Resource_GlobalYieldModifiers (
    ResourceType text references Resources(Type),
    YieldType text references Yields(Type),
    YieldFormula text references LuaFormula(Type),
    StartEra text references Eras(Type),
    EndEra text references Eras(Type)
);

alter table Resources add column NotificationTurn integer not null default 0;
alter table Resources add column NoDefaultNotification boolean not null default 0;