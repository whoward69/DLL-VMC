alter table Technologies add RazeSpeedModifier integer not null default 0;
alter table Technologies add FreePromotionRemoved integer default -1;
alter table Technologies add RemoveCurrentPromotion boolean default 0;
alter table Technologies add RemoveOceanImpassableCivilian boolean default 0;

alter table Technologies add CitySplashDamage boolean default 0;
alter table Technologies add BombardRange integer default 0;
alter table Technologies add BombardIndirect integer default 0;