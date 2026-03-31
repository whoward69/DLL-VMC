CREATE TABLE Building_DomainTroops(
    'BuildingType' TEXT REFERENCES Buildings(Type),
    'DomainType' TEXT REFERENCES Domains(Type),
    'NumTroop' INTEGER NOT NULL DEFAULT 0
);

ALTER TABLE Units ADD 'NoTroopConsume' BOOLEAN DEFAULT 0;
ALTER TABLE Units ADD 'CannotBeEstablishedCorps' BOOLEAN DEFAULT 0;
--ALTER TABLE UnitPromotions ADD 'NoTroopConsume' INTEGER DEFAULT 0;

--24 for base amount, 4 per city scale
INSERT INTO Defines(Name, Value) VALUES('TROOP_RATE_TIMES100_LOW', 50);
INSERT INTO Defines(Name, Value) VALUES('TROOP_RATE_TIMES100_DEFAULT', 100);
INSERT INTO Defines(Name, Value) VALUES('TROOP_RATE_TIMES100_HIGH', 150);
INSERT INTO Defines(Name, Value) VALUES('TROOP_NUM_BASE', 24);

ALTER TABLE Buildings ADD 'NumCrops' INTEGER DEFAULT 0;
ALTER TABLE Buildings ADD 'NumArmee' INTEGER DEFAULT 0;

ALTER TABLE UnitPromotions ADD COLUMN 'IsCrops' BOOLEAN DEFAULT 0;
ALTER TABLE UnitPromotions ADD COLUMN 'IsArmee' BOOLEAN DEFAULT 0;

ALTER TABLE UnitPromotions ADD COLUMN 'NumEstablishCorps' INTEGER DEFAULT 0;
ALTER TABLE UnitPromotions ADD COLUMN 'CannotBeEstablishedCorps' BOOLEAN DEFAULT 0;

ALTER TABLE Buildings ADD 'EnableCrops' boolean DEFAULT 0;
ALTER TABLE Buildings ADD 'EnableArmee' boolean DEFAULT 0;