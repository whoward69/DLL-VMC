create table BuildingClassCollections (
    ID integer primary key autoincrement not null,
    Type text default ''
);

create table BuildingClassCollections_Entries (
    CollectionType text references BuildingClassCollections(Type),
    BuildingClassIndex int not null default 0,
    BuildingClassType text references BuildingClasses(Type)
);
