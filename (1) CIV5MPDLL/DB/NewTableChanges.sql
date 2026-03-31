INSERT INTO Defines(Name, Value) VALUES('ORIGINAL_CAPITAL_MODMAX', 10);
INSERT INTO Defines(Name, Value) VALUES('BONUS_PER_ADJACENT_FRIEND_RANGED', 0);

INSERT INTO Defines(Name, Value) VALUES('HEALTH_DISEASE_CONNECTION_MOD', 10);
INSERT INTO Defines(Name, Value) VALUES('HEALTH_DISEASE_TRADE_MOD',10);
INSERT INTO Defines(Name, Value) VALUES('FRESH_WATER_HEALTH_YIELD', 2);
INSERT INTO Defines(Name, Value) VALUES('AI_CITIZEN_VALUE_HEALTH', 7);

INSERT INTO Defines(Name, Value) VALUES('VERY_UNHAPPY_DISEASE_PENALTY_PER_UNHAPPY', 2);
INSERT INTO Defines(Name, Value) VALUES('VERY_UNHAPPY_MAX_DISEASE_PENALTY', 100);
INSERT INTO Defines(Name, Value) VALUES('VERY_UNHAPPY_CRIME_PENALTY_PER_UNHAPPY', 2);
INSERT INTO Defines(Name, Value) VALUES('VERY_UNHAPPY_MAX_CRIME_PENALTY', 100);

INSERT INTO Defines(Name, Value) VALUES('CITY_CRIME_SPY_YIELD', 6);
INSERT INTO Defines(Name, Value) VALUES('CITY_CRIME_OPINION_REVOLUTIONARY_WAVE_YIELD', 10);
INSERT INTO Defines(Name, Value) VALUES('CITY_CRIME_OPINION_CIVIL_RESISTANCE_YIELD', 4);
INSERT INTO Defines(Name, Value) VALUES('CITY_CRIME_OPINION_DISSIDENTS_YIELD', 1);
INSERT INTO Defines(Name, Value) VALUES('CITY_CRIME_GOLDEN_AGE_YIELD',-10);
INSERT INTO Defines(Name, Value) VALUES('CITY_LOYALTY_GOLDEN_AGE_YIELD', -10);

alter table Features add PseudoNaturalWonder integer default 0;
alter table Features add Volcano boolean default 0;

alter table HandicapInfos add StrategicResourceMod integer default 100;
alter table HandicapInfos add StrategicResourceModPerEra integer default 0;
-- If AIFirstProphetPercent is positive, it overrides AITrainPercent on AI's First Prophet Cost
alter table HandicapInfos add AIFirstProphetPercent integer default 0;

alter table Worlds add ExtraCityDistance integer default 0;
create table World_HandicapExtraAIStartingUnit (
    WorldType text references Worlds(Type),
    HandicapType text references HandicapInfos(Type),
    ExtraAIStartingUnit integer default 0
);

alter table GameSpeeds add FreePromotion text references UnitPromotions(Type);
alter table GameSpeeds add SetterExtraPercent integer default 0;

alter table Processes add DefenseValue integer default 0;

alter table Projects add FreePromotion text references UnitPromotions(Type);
alter table Projects add NoBroadcast boolean default 0;
alter table Projects add PolicyBranchPrereq text references PolicyBranchTypes(Type);
create table Project_PolicyNeeded (
    ProjectType text references Projects(Type),
    PolicyType text references Policies(Type)
);

alter table Builds add ObsoleteTech text default null;
create table Build_ResourceRemove (
    BuildType text references Builds(Type),
    ResourceType text references Resources(Type)
);
