create table CityScales (
    ID integer primary key autoincrement not null,
    Type text unique,
    MinPopulation integer not null,

    NeedGrowthBuilding boolean not null default 0,
    CanImmigrantIn boolean not null default 1,
    CanImmigrantOut boolean not null default 1
);

create table CityScale_FreeBuildingClass (
    CityScaleType text references CityScales(Type),
    BuildingClassType text references BuildingClasses(Type),
    NumBuildings integer not null default 1,

    RequiredTraitType text null,
    RequiredPolicyType text null
);

alter table Buildings add column 'EnableCityScaleGrowth' text null;
alter table Buildings add column 'EnableAllCityScaleGrowth' boolean not null default 0;