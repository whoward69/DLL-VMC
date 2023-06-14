CREATE TABLE Improvement_TradeRouteYieldChanges(
    'ImprovementType' text not null references Improvements(Type),
    'DomainType' text not null references Domains(Type),
    'YieldType' text not null references Yields(Type),
    'Yield' integer not null default 0
);