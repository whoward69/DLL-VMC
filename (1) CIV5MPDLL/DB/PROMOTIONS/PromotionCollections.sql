create table PromotionCollections (
    ID integer primary key autoincrement not null,
    Type text unique
);

create table PromotionCollections_Entries (
    CollectionType text references PromotionCollections(Type),
    PromotionType text references UnitPromotions(Type),
    PromotionIndex int not null default 0,
    
    TriggerMeleeAttack  boolean not null default 0,
    TriggerRangedAttack  boolean not null default 0,
    TriggerMeleeDefense boolean not null default 0,
    TriggerRangedDefense boolean not null default 0,
    TriggerHPFixed integer not null default 0,
    TriggerHPPercent integer not null default 0,
    TriggerLuaCheck boolean not null default 0,
    TriggerLuaHook boolean not null default 0
);
