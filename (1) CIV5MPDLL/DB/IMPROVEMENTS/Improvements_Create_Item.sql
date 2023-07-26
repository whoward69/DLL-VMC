--enable create item or set new a improvement 

--get the items generate mod 0 = disable, 1 = only improvement, 2 = only feature, 3 = enable all
ALTER TABLE Improvements ADD COLUMN 'CreatedItemMod' INTEGER DEFAULT 0;
--set quantity > 0 to Generating fixed quantity and set quantity < 0 to Generating random quantity
ALTER TABLE Improvements ADD COLUMN 'CreatedResourceQuantity' INTEGER DEFAULT 0;
--enable to set a new Improvement or clear itself when built
ALTER TABLE Improvements ADD COLUMN 'SetNewImprovement' TEXT DEFAULT NULL;
ALTER TABLE Improvements ADD COLUMN 'SetNewFeature' TEXT DEFAULT NULL;

--add a column to set Generation Class for CreatedItemMod, 0 = disable to Create
--ALTER TABLE Resources ADD COLUMN 'ResourcesGenerationClass' INTEGER DEFAULT 0;

CREATE TABLE Improvements_Create_Collection (
    ImprovementType TEXT DEFAULT NULL REFERENCES Improvements(Type),
    TerrainType TEXT DEFAULT NULL REFERENCES Terrains(Type),
    TerrainOnly BOOLEAN DEFAULT 0,
    FeatureType TEXT DEFAULT NULL REFERENCES Features(Type),
    FeatureOnly BOOLEAN DEFAULT 0,
    ResourceType TEXT DEFAULT NULL REFERENCES Resources(Type)
);