create table Unit_ScalingFromOwnedImprovements (
	UnitType text references Units(Type),
	ImprovementType text references Improvements(Type),
	Amount integer default 0
);