create table Specialist_Resources (
    SpecialistType		text references Specialists(Type),
    ResourceType		text references Resources(Type),
    Quantity			integer not null default 0,

    RequiredPolicyType text null, -- references Policies(Type)
    RequiredTechType   text null  -- references Technologies(Type)
);