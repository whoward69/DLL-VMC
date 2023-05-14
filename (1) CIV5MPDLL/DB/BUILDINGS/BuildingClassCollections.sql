create table BuildingClassCollections (
    ID integer primary key autoincrement not null,
    Type text not null
);

create table BuildingClassCollections_Entries (
    CollectionType text not null references BuildingClassCollections(Type),
    BuildingClassIndex int not null default 0,
    BuildingClassType text not null references BuildingClasses(Type)
);
