-- Unit stacking limit in cities
INSERT INTO Defines(Name, Value) VALUES('CITY_UNIT_LIMIT', 1);
-- Additional units allowed by improvements
ALTER TABLE Improvements
  ADD AdditionalUnits INTEGER DEFAULT 0;

INSERT INTO CustomModDbUpdates(Name, Value) VALUES('GLOBAL_STACKING_RULES', 1);

INSERT INTO CustomModDbUpdates(Name, Value) VALUES('API_TRADE_ROUTE_YIELD_RATE', 1);

INSERT INTO CustomModDbUpdates(Name, Value) VALUES('BALANCE_CORE', 1);

INSERT INTO CustomModDbUpdates(Name, Value) VALUES('BUILDINGS_YIELD_FROM_OTHER_YIELD', 1);

INSERT INTO CustomModDbUpdates(Name, Value) VALUES('TRAITS_CAN_FOUND_MOUNTAIN_CITY', 1);

INSERT INTO CustomModDbUpdates(Name, Value) VALUES('TRAITS_CAN_FOUND_COAST_CITY', 1);

INSERT INTO CustomModDbUpdates(Name, Value) VALUES('ERA_EFFECTS_EXTENSIONS', 1);

INSERT INTO CustomModDbUpdates(Name, Value) VALUES('BUILDINGS_GOLDEN_AGE_EXTEND', 1);

INSERT INTO CustomModDbUpdates(Name, Value) VALUES('TRAIT_RELIGION_FOLLOWER_EFFECTS', 1);