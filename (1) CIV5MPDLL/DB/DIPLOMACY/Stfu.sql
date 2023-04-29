INSERT OR REPLACE INTO Responses(Type)
  SELECT DISTINCT ResponseType FROM Diplomacy_Responses ORDER BY ResponseType;
  
INSERT INTO CustomModDbUpdates(Name, Value) VALUES('DIPLOMACY_STFU_SQL', 1);
