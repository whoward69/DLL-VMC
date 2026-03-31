create table Era_MountainCityYieldChanges (
	EraType text references Eras(Type),
	YieldType text references Yields(Type),
	Yield integer not null
);
create table Era_CoastCityYieldChanges (
	EraType text references Eras(Type),
	YieldType text references Yields(Type),
	Yield integer not null
);