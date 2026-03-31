create table NuclearWinterLevels (
    ID integer primary key autoincrement not null,
    Type text unique,
    Description text default '',
    Help text default '',
    TriggerThreshold integer default 0
);

create table NuclearWinterLevel_GlobalYieldModifier (
    NuclearWinterLevelType text references NuclearWinterLevels(Type),
    YieldType text references Yields(Type),
    Yield integer default 0
);
create table NuclearWinterLevel_GlobalYield (
    NuclearWinterLevelType text references NuclearWinterLevels(Type),
    YieldType text references Yields(Type),
    Yield integer default 0
);

ALTER TABLE Units ADD 'NuclearWinterProcess' INTEGER DEFAULT 0;
ALTER TABLE Buildings ADD 'NoNuclearWinterLocal' boolean DEFAULT 0;

INSERT INTO NuclearWinterLevels(Type, Description, Help, TriggerThreshold)
SELECT 'NO_NUCLEAR_WINTER', 'TXT_KEY_NUCLEAR_LEVEL_NONE', 'TXT_KEY_NUCLEAR_LEVEL_NONE_HELP', 0;