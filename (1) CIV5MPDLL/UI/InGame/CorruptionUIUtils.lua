function getAntiCorruptionBuildingID(player, corruptionLevel)
    local levelInfo = GameInfo.CorruptionLevels {ID = corruptionLevel}();
    if levelInfo == nil then return -1; end

    local antiCorruptionBuildingClass = levelInfo.PublicSecurityBuildingClass;
    local antiCorruptionBuildingClassID = GameInfoTypes[antiCorruptionBuildingClass];
    return player:GetCivBuilding(antiCorruptionBuildingClassID or -1) or -1;
end

function getCorruptionLevelIconString(corruptionLevel)
    local levelInfo = GameInfo.CorruptionLevels {ID = corruptionLevel}();
    if levelInfo == nil then return ""; end

    return levelInfo.IconString or "";
end

function getCorruptionScoreReport(player, city)
	local result = Locale.ConvertTextKey("TXT_KEY_CITYBANNER_CORRUPTION_REPORT");
	result = result .. "[NEWLINE][NEWLINE]";

	-- score detail
	local tmp = 0;
	tmp = city:CalculateCorruptionScoreFromDistance();
	if tmp ~= 0 then
		result = result .. "[ICON_BULLET]" .. Locale.ConvertTextKey("TXT_KEY_CITYBANNER_CORRUPTION_SCORE_FROM_DISTANCE", tmp) .. "[NEWLINE]";
	end
	tmp = city:CalculateCorruptionScoreFromCoastalBonus();
	if tmp ~= 0 then
		result = result .. "[ICON_BULLET]" .. Locale.ConvertTextKey("TXT_KEY_CITYBANNER_CORRUPTION_SCORE_FROM_COASTAL_BONUS", tmp) .. "[NEWLINE]";
	end
	tmp = city:GetCorruptionScoreChangeFromBuilding();
	if tmp ~= 0 then
		result = result .. "[ICON_BULLET]" .. Locale.ConvertTextKey("TXT_KEY_CITYBANNER_CORRUPTION_SCORE_FROM_BUILDING", tmp) .. "[NEWLINE]";
	end
	tmp = city:CalculateCorruptionScoreFromResource();
	if tmp ~= 0 then
		result = result .. "[ICON_BULLET]" .. Locale.ConvertTextKey("TXT_KEY_CITYBANNER_CORRUPTION_SCORE_FROM_RESOURCE", tmp) .. "[NEWLINE]";
	end
	tmp = city:CalculateCorruptionScoreFromTrait();
	if tmp ~= 0 then
		result = result .. "[ICON_BULLET]" .. Locale.ConvertTextKey("TXT_KEY_CITYBANNER_CORRUPTION_SCORE_FROM_TRAIT", tmp) .. "[NEWLINE]";
	end
	tmp = city:CalculateCorruptionScoreModifierFromSpy();
	if tmp ~= 0 then
		result = result .. "[ICON_BULLET]" .. Locale.ConvertTextKey("TXT_KEY_CITYBANNER_CORRUPTION_SCORE_MOD_FROM_SPY", tmp) .. "[NEWLINE]";
	end
	tmp = city:CalculateCorruptionScoreModifierFromTrait();
	if tmp ~= 0 then
		result = result .. "[ICON_BULLET]" .. Locale.ConvertTextKey("TXT_KEY_CITYBANNER_CORRUPTION_SCORE_MOD_FROM_TRAIT", tmp) .. "[NEWLINE]";
	end
	tmp = city:GetCorruptionScoreModifierFromPolicy();
	if tmp ~= 0 then
		result = result .. "[ICON_BULLET]" .. Locale.ConvertTextKey("TXT_KEY_CITYBANNER_CORRUPTION_SCORE_MOD_FROM_POLICY", tmp) .. "[NEWLINE]";
	end
	-- total score
	result = result .. "[ICON_BULLET]" .. Locale.ConvertTextKey("TXT_KEY_CITYBANNER_CORRUPTION_SCORE", city:GetCorruptionScore()) .. "[NEWLINE][NEWLINE]";

	-- level change detail
	local bLevelReduceByOne = city:IsCorruptionLevelReduceByOne();
	if bLevelReduceByOne then
		result = result .. "[ICON_BULLET]" .. Locale.ConvertTextKey("TXT_KEY_CITYBANNER_CORRUPTION_LEVEL_REDUCE_BY_ONE_GLOBAL") .. "[NEWLINE]";
	end
	tmp = city:GetCorruptionLevelChangeFromBuilding();
	if tmp ~= 0 then
		result = result .. "[ICON_BULLET]" .. Locale.ConvertTextKey("TXT_KEY_CITYBANNER_CORRUPTION_LEVEL_CHANGE_FROM_BUILDING", tmp) .. "[NEWLINE]";
	end
	tmp = city:GetMaxCorruptionLevel();
	if tmp > 0 then
		result = result .. "[ICON_BULLET]" .. Locale.ConvertTextKey("TXT_KEY_CITYBANNER_CORRUPTION_LEVEL_MAX_TRAIT", tmp) .. "[NEWLINE]";
	end

	result = result .. "------------------------------------[NEWLINE]"
	return result;
end