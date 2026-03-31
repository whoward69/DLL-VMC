INSERT INTO Defines(Name, Value) VALUES('WAR_CASUALTIES_THRESHOLD', 200);
INSERT INTO Defines(Name, Value) VALUES('WAR_CASUALTIES_DELTA_BASE', 100);
INSERT INTO Defines(Name, Value) VALUES('WAR_CASUALTIES_POPULATION_LOSS', 1);

ALTER TABLE UnitPromotions add column WarCasualtiesModifier int not null default 0;
ALTER TABLE Policies add column WarCasualtiesModifier int not null default 0;