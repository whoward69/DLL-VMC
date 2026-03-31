INSERT INTO Defines(Name, Value) VALUES('IMMIGRATION_BASE_RATE', 30);

ALTER TABLE Policies ADD 'ImmigrationInModifier' INTEGER DEFAULT 0;
ALTER TABLE Policies ADD 'ImmigrationOutModifier' INTEGER DEFAULT 0;
alter table Buildings add column 'EnableAlwaysImmigrantIn' boolean not null default 0;