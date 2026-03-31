ALTER TABLE Traits
  ADD SeaTradeRouteRangeBonus INTEGER DEFAULT 0;

INSERT INTO CustomModDbUpdates(Name, Value) VALUES('TRAITS_TRADE_ROUTE_BONUSES', 1);

create table Trait_SeaTradeRouteYieldPerEraTimes100 (
	TraitType text references Traits(Type),
	YieldType text references Yields(Type),
	Yield int default 0
);
create table Trait_SeaTradeRouteYieldTimes100 (
	TraitType text references Traits(Type),
	YieldType text references Yields(Type),
	Yield int default 0
);
