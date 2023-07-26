create table CityScales (
    ID integer primary key autoincrement not null,
    Type text not null unique,
    MinPopulation integer not null,

    NeedGrowthBuilding boolean not null default 0
);

create table CityScale_FreeBuildingClass (
    CityScaleType text not null references CityScales(Type),
    BuildingClassType text not null references BuildingClasses(Type),
    NumBuildings integer not null default 1,

    RequiredTraitType text null,
    RequiredPolicyType text null
);

alter table Buildings add column 'EnableCityScaleGrowth' text null;
alter table Buildings add column 'EnableAllCityScaleGrowth' boolean not null default 0;