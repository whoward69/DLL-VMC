create table Policy_CityResources (
    PolicyType text references Policies(Type),
    ResourceType text references Resources(Type),
    Quantity integer not null default 0,
    
    -- optional conditions
    CityScaleType text null references CityScales(Type),
    LargerScaleValid boolean not null default 0,
    MustCoastal boolean not null default 0
);
