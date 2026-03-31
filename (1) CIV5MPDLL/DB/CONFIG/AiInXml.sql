-- Number of melee units per AA unit before the AI considers that they have enough, default value is 4
INSERT INTO Defines(Name, Value) VALUES('AI_CONFIG_MILITARY_MELEE_PER_AA', 4);
-- Number of aircraft per "carrier slot" before the AI considers that they have enough, default value is 1,
--  ie the AI will build carriers until it can embark EVERY aircraft, 2 or 3 would be sufficient
INSERT INTO Defines(Name, Value) VALUES('AI_CONFIG_MILITARY_AIRCRAFT_PER_CARRIER_SPACE', 1);
-- Number of water tiles per ship for (small?) bodies of water, the default is 5, but 7 or 8 would be sufficient
INSERT INTO Defines(Name, Value) VALUES('AI_CONFIG_MILITARY_TILES_PER_SHIP', 5);

-- Warmonger weight for capturing a major civ's city
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_MAJOR_CITY_WEIGHT', 1000);
-- Warmonger weight for capturing a minor civ's city
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_MINOR_CITY_WEIGHT', 1000);
-- Warmonger modifier for the capital
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_CAPITAL_CITY_PERCENT', 100);
-- Warmonger modifiers for knowing the attacker/defender
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_KNOWS_ATTACKER_PERCENT', 100);
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_KNOWS_DEFENDER_PERCENT', 0);
-- Warmonger modifier for being the aggrieved
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_AGGRIEVED_PERCENT', 100);
-- Warmonger modifier for joint wars
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_COOP_WAR_PERCENT', 100);
-- Warmonger enabled for def pact consideration
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_DEF_PACT_ENABLED', 0);
-- Warmonger enabled for city size consideration
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_CITY_SIZE_ENABLED', 0);
-- Warmonger approach modifiers
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_APPROACH_PERCENT_HOSTILE', 0);
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_APPROACH_PERCENT_AFRAID', 0);
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_APPROACH_PERCENT_GUARDED', 0);
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_APPROACH_PERCENT_NEUTRAL', 0);
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_APPROACH_PERCENT_FRIENDLY', 0);
-- Warmonger decay modifiers
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_APPROACH_DECAY_PERCENT_HOSTILE', 100);
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_APPROACH_DECAY_PERCENT_AFRAID', 100);
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_APPROACH_DECAY_PERCENT_GUARDED', 100);
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_APPROACH_DECAY_PERCENT_NEUTRAL', 100);
INSERT INTO Defines(Name, Value) VALUES('WARMONGER_THREAT_APPROACH_DECAY_PERCENT_FRIENDLY', 100);

INSERT INTO CustomModDbUpdates(Name, Value) VALUES('CONFIG_AI_IN_XML', 1);

