alter table Improvements add column EnableXP boolean not null default 0;

alter table Improvements add column EnableUpgrade boolean not null default 0;

alter table Improvements add column UpgradeXP integer not null default -1;
alter table Improvements add column UpgradeImprovementType text not null default '';

alter table Improvements add column EnableDowngrade boolean not null default 0;
alter table Improvements add column DowngradeImprovementType text not null default '';

alter table Improvements add column HideXPInfo boolean not null default 0;