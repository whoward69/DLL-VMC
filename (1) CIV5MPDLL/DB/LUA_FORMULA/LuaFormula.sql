create table LuaFormula (
    ID integer primary key autoincrement not null,
    Type text unique,
    Formula text default ''
);