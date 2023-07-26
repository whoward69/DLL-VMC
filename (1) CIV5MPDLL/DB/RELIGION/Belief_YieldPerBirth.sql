CREATE TABLE Belief_YieldPerBirth (
    BeliefType TEXT NOT NULL REFERENCES Beliefs(Type),
    YieldType TEXT NOT NULL REFERENCES Yields(Type),
    Yield NOT NULL DEFAULT 0
);

ALTER TABLE Beliefs ADD COLUMN 'AllowYieldPerBirth' BOOLEAN DEFAULT 0;