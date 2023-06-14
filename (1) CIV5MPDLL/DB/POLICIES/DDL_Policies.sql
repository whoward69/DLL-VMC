alter table Policies add column `MinorBullyInfluenceLossModifier` int not null default 0;

create table Policy_MinorsTradeRouteYieldRate (
    PolicyType text not null,
    YieldType text not null,
    Rate integer not null
);

create table Policy_InternalTradeRouteDestYieldRate (
    PolicyType text not null,
    YieldType text not null,
    Rate integer not null
);

alter table Policies add column IdeologyPressureModifier integer not null default 0;
alter table Policies add column IdeologyUnhappinessModifier integer not null default 0;

CREATE TABLE Policy_CityWithWorldWonderYieldModifier (
	'PolicyType' text ,
	'YieldType' text ,
	'Yield' integer  not null ,
	foreign key (PolicyType) references Policies(Type),
	foreign key (YieldType) references Yields(Type)
);

CREATE TABLE Policy_TradeRouteCityYieldModifier (
	'PolicyType' text ,
	'YieldType' text ,
	'Yield' integer  not null ,
	foreign key (PolicyType) references Policies(Type),
	foreign key (YieldType) references Yields(Type)
);

alter table Policies add column GlobalHappinessFromFaithPercent integer not null default 0;

alter table Policies add column HappinessInWLTKDCities integer not null default 0;