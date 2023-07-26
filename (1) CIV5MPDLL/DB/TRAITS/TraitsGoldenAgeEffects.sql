CREATE TABLE Trait_GoldenAgeYieldModifiers (
    'TraitType' text,
    'YieldType' text,
    'Yield' integer,
    foreign key (TraitType) references Traits(Type),
    foreign key (YieldType) references Yields(Type)
);