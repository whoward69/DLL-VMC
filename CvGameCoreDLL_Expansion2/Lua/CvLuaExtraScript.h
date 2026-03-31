#pragma once
#ifndef CVLUAEXTRASCRIPT_H
#define CVLUAEXTRASCRIPT_H
#include "CvString.h"
CvString OptionalUCCode = R"(
    include ("IconSupport");
    include ("MenuUtils");
    include("InfoTooltipInclude");

    -- Hide dialog by default.
    ContextPtr:SetHide(true);
    --==========================================================================================
    -- Variables
    --==========================================================================================
    local g_UCList = {}
    local g_ChosenUCList = {{nil, nil}, {nil, nil}, {nil, nil}, {nil, nil}}
    local UC_UNIT = 0;
    local UC_BUILDING = 1;
    local UC_IMPROVEMENT = 2;
    local t_List = {};

    -- 抽取所有UC
    for row in GameInfo.Civilization_UnitClassOverrides() do
        -- 不能建造的以及本身是默认单位的不要
        -- 海鹞 蛮族?
        if row.UnitType 
        and row.UnitType ~= "UNIT_BARBARIAN_WARRIOR"
        and row.UnitType ~= "UNIT_BARBARIAN_ARCHER"
        and row.UnitType ~= "UNIT_BARBARIAN_AXMAN"
        and row.UnitType ~= "UNIT_BARBARIAN_GALLEY"
        then
            table.insert(t_List, {row.UnitType, UC_UNIT, GameInfoTypes[row.UnitType], Locale.ConvertTextKey(GameInfo.Units[row.UnitType].Description)})
        end
    end
    table.sort(t_List, function(a , b) return Locale.Compare(a[4], b[4]) == -1 end)
    g_UCList = t_List
    t_List = {}
    for row in GameInfo.Civilization_BuildingClassOverrides() do
        -- 不能建造的不要
        -- 总督府 总督宫 宫殿允许(V5)
        if row.BuildingType 
        and (GameInfo.Buildings[row.BuildingType].Cost > 1 
            or row.BuildingType == "BUILDING_SATRAPS_COURT" 
            or row.BuildingType == "BUILDING_PUPPET_GOVERNEMENT_FULL"
            or GameInfo.Buildings[row.BuildingType].BuildingClass == "BUILDINGCLASS_PALACE") 
        then
            table.insert(t_List, {row.BuildingType, UC_BUILDING, GameInfoTypes[row.BuildingType], Locale.ConvertTextKey(GameInfo.Buildings[row.BuildingType].Description)})
        end
    end
    table.sort(t_List, function(a , b) return Locale.Compare(a[4], b[4]) == -1 end)
    for k, v in pairs(t_List) do table.insert(g_UCList, v) end
    t_List = {}
    for row in GameInfo.Improvements() do
        if row.Type and row.CivilizationType then
            table.insert(t_List, {row.Type, UC_IMPROVEMENT, row.ID, Locale.ConvertTextKey(row.Description)})
        end
    end
    table.sort(t_List, function(a , b) return Locale.Compare(a[4], b[4]) == -1 end)
    for k, v in pairs(t_List) do table.insert(g_UCList, v) end
    t_List = {}

    local activePlayerID = Game.GetActivePlayer()
    local activePlayer = Players[activePlayerID]

    --==========================================================================================
    -- Main Functions
    --==========================================================================================
    -- Initializes All Components.
    function initializeDialog()

        local pPlayer = activePlayer;	
        local leader = GameInfo.Leaders[pPlayer:GetLeaderType()];
        local activeCivID = pPlayer:GetCivilizationType();
        local activeCiv = GameInfo.Civilizations[activeCivID];
        g_ChosenUCList = {{nil, nil}, {nil, nil}, {nil, nil}, {nil, nil}}

        if leader then
            print("initializeDialog: Leader Found: " .. Locale.ConvertTextKey(leader.Description))
            -- Build CIV LEADER Potrair
            IconHookup(leader.PortraitIndex, 128, leader.IconAtlas, Controls.MercenaryUnitLeaderPortrait)
        else
            print("Leader not found")
        end

        IconHookup(11, 256, "EXPANSION2_PROMOTION_ATLAS", Controls.UCPPortrait1)
        IconHookup(11, 256, "EXPANSION2_PROMOTION_ATLAS", Controls.UCPPortrait2)
        IconHookup(11, 256, "EXPANSION2_PROMOTION_ATLAS", Controls.UCPPortrait3)
        IconHookup(11, 256, "EXPANSION2_PROMOTION_ATLAS", Controls.UCPPortrait4)
        
        Controls.UCSelectList1:GetButton():LocalizeAndSetText("TXT_KEY_OPTIONAL_UC_CHOSE")
        Controls.UCSelectList2:GetButton():LocalizeAndSetText("TXT_KEY_OPTIONAL_UC_CHOSE")
        Controls.UCSelectList3:GetButton():LocalizeAndSetText("TXT_KEY_OPTIONAL_UC_CHOSE")
        Controls.UCSelectList4:GetButton():LocalizeAndSetText("TXT_KEY_OPTIONAL_UC_CHOSE")
    end

    function OnUCSelected1(UCID, UCType)
        if UCType == UC_UNIT then
            local unit = GameInfo.Units[UCID]
            Controls.UCSelectList1:GetButton():SetText(Locale.ConvertTextKey(unit.Description or 0))
            Controls.UCIconButton1:SetToolTipString(GetHelpTextForUnit(UCID))
            IconHookup(unit.PortraitIndex, 256, unit.IconAtlas, Controls.UCPPortrait1);
        elseif UCType == UC_BUILDING then
            local building = GameInfo.Buildings[UCID]
            Controls.UCSelectList1:GetButton():SetText(Locale.ConvertTextKey(building.Description or 0))
            Controls.UCIconButton1:SetToolTipString(GetHelpTextForBuilding(UCID))
            IconHookup(building.PortraitIndex, 256, building.IconAtlas, Controls.UCPPortrait1);
        elseif UCType == UC_IMPROVEMENT then
            local improvement = GameInfo.Improvements[UCID]
            Controls.UCSelectList1:GetButton():SetText(Locale.ConvertTextKey(improvement.Description or 0))
            Controls.UCIconButton1:SetToolTipString(GetHelpTextForImprovement(UCID))
            IconHookup(improvement.PortraitIndex, 256, improvement.IconAtlas, Controls.UCPPortrait1);
        end
        g_ChosenUCList[1] = {UCID, UCType}
    end

    function OnUCSelected2(UCID, UCType)
        if UCType == UC_UNIT then
            local unit = GameInfo.Units[UCID]
            Controls.UCSelectList2:GetButton():SetText(Locale.ConvertTextKey(unit.Description or 0))
            Controls.UCIconButton2:SetToolTipString(GetHelpTextForUnit(UCID))
            IconHookup(unit.PortraitIndex, 256, unit.IconAtlas, Controls.UCPPortrait2);
        elseif UCType == UC_BUILDING then
            local building = GameInfo.Buildings[UCID]
            Controls.UCSelectList2:GetButton():SetText(Locale.ConvertTextKey(building.Description or 0))
            Controls.UCIconButton2:SetToolTipString(GetHelpTextForBuilding(UCID))
            IconHookup(building.PortraitIndex, 256, building.IconAtlas, Controls.UCPPortrait2);
        elseif UCType == UC_IMPROVEMENT then
            local improvement = GameInfo.Improvements[UCID]
            Controls.UCSelectList2:GetButton():SetText(Locale.ConvertTextKey(improvement.Description or 0))
            Controls.UCIconButton2:SetToolTipString(GetHelpTextForImprovement(UCID))
            IconHookup(improvement.PortraitIndex, 256, improvement.IconAtlas, Controls.UCPPortrait2);
        end
        g_ChosenUCList[2] = {UCID, UCType}
    end

    function OnUCSelected3(UCID, UCType)
        if UCType == UC_UNIT then
            local unit = GameInfo.Units[UCID]
            Controls.UCSelectList3:GetButton():SetText(Locale.ConvertTextKey(unit.Description or 0))
            Controls.UCIconButton3:SetToolTipString(GetHelpTextForUnit(UCID))
            IconHookup(unit.PortraitIndex, 256, unit.IconAtlas, Controls.UCPPortrait3);
        elseif UCType == UC_BUILDING then
            local building = GameInfo.Buildings[UCID]
            Controls.UCSelectList3:GetButton():SetText(Locale.ConvertTextKey(building.Description or 0))
            Controls.UCIconButton3:SetToolTipString(GetHelpTextForBuilding(UCID))
            IconHookup(building.PortraitIndex, 256, building.IconAtlas, Controls.UCPPortrait3);
        elseif UCType == UC_IMPROVEMENT then
            local improvement = GameInfo.Improvements[UCID]
            Controls.UCSelectList3:GetButton():SetText(Locale.ConvertTextKey(improvement.Description or 0))
            Controls.UCIconButton3:SetToolTipString(GetHelpTextForImprovement(UCID))
            IconHookup(improvement.PortraitIndex, 256, improvement.IconAtlas, Controls.UCPPortrait3);
        end
        g_ChosenUCList[3] = {UCID, UCType}
    end

    function OnUCSelected4(UCID, UCType)
        if UCType == UC_UNIT then
            local unit = GameInfo.Units[UCID]
            Controls.UCSelectList4:GetButton():SetText(Locale.ConvertTextKey(unit.Description or 0))
            Controls.UCIconButton4:SetToolTipString(GetHelpTextForUnit(UCID))
            IconHookup(unit.PortraitIndex, 256, unit.IconAtlas, Controls.UCPPortrait4);
        elseif UCType == UC_BUILDING then
            local building = GameInfo.Buildings[UCID]
            Controls.UCSelectList4:GetButton():SetText(Locale.ConvertTextKey(building.Description or 0))
            Controls.UCIconButton4:SetToolTipString(GetHelpTextForBuilding(UCID))
            IconHookup(building.PortraitIndex, 256, building.IconAtlas, Controls.UCPPortrait4);
        elseif UCType == UC_IMPROVEMENT then
            local improvement = GameInfo.Improvements[UCID]
            Controls.UCSelectList4:GetButton():SetText(Locale.ConvertTextKey(improvement.Description or 0))
            Controls.UCIconButton4:SetToolTipString(GetHelpTextForImprovement(UCID))
            IconHookup(improvement.PortraitIndex, 256, improvement.IconAtlas, Controls.UCPPortrait4);
        end
        g_ChosenUCList[4] = {UCID, UCType}
    end

    -- Left pulldown list update.
    function UpdateUCList(ConUCSelectList, OnUCSelectedFuc)
        ConUCSelectList:ClearEntries()
        for k, v in pairs(g_UCList) do
            local entry = {}
            ConUCSelectList:BuildEntry("InstanceOne", entry)
            local UCPoint = nil;
            if v[2] == UC_UNIT then
                UCPoint = GameInfo.Units[v[3]]
            elseif v[2] == UC_BUILDING then
                UCPoint = GameInfo.Buildings[v[3]]
            elseif v[2] == UC_IMPROVEMENT then
                UCPoint = GameInfo.Improvements[v[3]]
            end
            -- RegisterSelectionCallback 传参 ID UCType 
            entry.Button:SetVoid1(v[3])
            entry.Button:SetVoid2(v[2])
            entry.Button:SetText(Locale.ConvertTextKey(UCPoint.Description))
        end

        ConUCSelectList:GetButton():LocalizeAndSetText("TXT_KEY_OPTIONAL_UC_CHOSE")	
        ConUCSelectList:CalculateInternals()
        ConUCSelectList:RegisterSelectionCallback(OnUCSelectedFuc)
    end


    -- Handle the Apply Button
    function onApplyButton()
        for k, v in pairs(g_ChosenUCList) do
            if v[1] and v[2] then
                if v[2] == UC_UNIT then
                    local unit = GameInfo.Units[v[1]]
                    --activePlayer:SendAndExecuteLuaFunction("CvLuaPlayer::lChangeUUFromExtra", unit.ID)
                    activePlayer:ChangeUUFromExtra(unit.ID)
                    --禁用默认
                    --if unit.Special ~= "SPECIALUNIT_PEOPLE" then end
                    activePlayer:ChangeUUFromExtra(GameInfoTypes[GameInfo.UnitClasses[unit.Class].DefaultUnit])
                elseif v[2] == UC_BUILDING then
                    local building = GameInfo.Buildings[v[1]]
                    --activePlayer:SendAndExecuteLuaFunction("CvLuaPlayer::lChangeUBFromExtra", building.ID)
                    activePlayer:ChangeUBFromExtra(building.ID)
                    --禁用默认
                    activePlayer:ChangeUBFromExtra(GameInfoTypes[GameInfo.BuildingClasses[building.BuildingClass].DefaultBuilding])
                elseif v[2] == UC_IMPROVEMENT then
                    local improvement = GameInfo.Improvements[v[1]]
                    --activePlayer:SendAndExecuteLuaFunction("CvLuaPlayer::lChangeUIFromExtra", improvement.ID)
                    activePlayer:ChangeUIFromExtra(improvement.ID)
                end
            end
        end
        --activePlayer:SendAndExecuteLuaFunction("CvLuaPlayer::lSetLostUC", true)
        activePlayer:SetLostUC(true)
        addOptionalUCNotification()
        changeGreatWorkBuildings()
        hideDialog()
    end
    Controls.OKButton:RegisterCallback(Mouse.eLClick, onApplyButton)

    --==========================================================================================
    -- Smaller Functions
    --==========================================================================================
    -- Show function
    function showDialog()
        -- Show panel
        ContextPtr:SetHide(false)
        -- Initialize
        initializeDialog()
        UpdateUCList(Controls.UCSelectList1, OnUCSelected1)
        UpdateUCList(Controls.UCSelectList2, OnUCSelected2)
        UpdateUCList(Controls.UCSelectList3, OnUCSelected3)
        UpdateUCList(Controls.UCSelectList4, OnUCSelected4)
    end

    -- Hide function
    function hideDialog()
        ContextPtr:SetHide(true)
    end

    --==========================================================================================
    -- Game Procession Functions
    --==========================================================================================
    function updateChosenUCList()
        for row in GameInfo.Civilization_UnitClassOverrides() do
            if row.UnitType and (activePlayer:GetUUFromExtra(GameInfoTypes[row.UnitType]) > 0) then
                table.insert(g_ChosenUCList,{GameInfoTypes[row.UnitType], UC_UNIT})
            end
        end
        for row in GameInfo.Civilization_BuildingClassOverrides() do
            if row.BuildingType and (activePlayer:GetUBFromExtra(GameInfoTypes[row.BuildingType]) > 0)  then
                table.insert(g_ChosenUCList,{GameInfoTypes[row.BuildingType], UC_BUILDING})
            end
        end
        for row in GameInfo.Improvements() do
            if row.Type and (activePlayer:GetUIFromExtra(GameInfoTypes[row.Type]) > 0)  then
                table.insert(g_ChosenUCList,{GameInfoTypes[row.Type], UC_IMPROVEMENT})
            end
        end
    end
    function addOptionalUCNotification()
        local heading = Locale.ConvertTextKey("TXT_KEY_OPTIONAL_UC_NOTIFICATION_HEAD")
        local text = "";
        local UCDescript = "";
        for k, v in pairs(g_ChosenUCList) do
            if v[1] and v[2] then
                if v[2] == UC_UNIT then
                    text = text .. "[NEWLINE]" .. Locale.ConvertTextKey(GameInfo.Units[v[1]].Description)
                elseif v[2] == UC_BUILDING then
                    text = text .. "[NEWLINE]" .. Locale.ConvertTextKey(GameInfo.Buildings[v[1]].Description)
                elseif v[2] == UC_IMPROVEMENT then
                    text = text .. "[NEWLINE]" .. Locale.ConvertTextKey(GameInfo.Improvements[v[1]].Description)
                end
            end
        end
        if text ~= "" then
            text = Locale.ConvertTextKey("TXT_KEY_OPTIONAL_UC_NOTIFICATION_TEXT") .. text
        else
            text = Locale.ConvertTextKey("TXT_KEY_OPTIONAL_UC_NOTIFICATION_TEXT2")
        end
        activePlayer:AddNotification(NotificationTypes.NOTIFICATION_GENERIC, text, heading, -1, -1);
    end
    function changeGreatWorkBuildings()
        if activePlayer:IsLostUC() then
            for row in GameInfo.Civilization_BuildingClassOverrides() do
                if row.BuildingType
                and row.CivilizationType == GameInfo.Civilizations[activePlayer:GetCivilizationType()].Type 
                then
                    LuaEvents.GreatWorkBuildingsChanges(GameInfo.BuildingClasses[GameInfo.Buildings[row.BuildingType].BuildingClass].DefaultBuilding)
                    print(GameInfo.BuildingClasses[GameInfo.Buildings[row.BuildingType].BuildingClass].DefaultBuilding)
                end
            end
        end
        for k, v in pairs(g_ChosenUCList) do
            if v[1] and v[2] then
                if v[2] == UC_BUILDING then
                    LuaEvents.GreatWorkBuildingsChanges(activePlayer:GetCivBuilding(GameInfoTypes[GameInfo.Buildings[v[1]].BuildingClass]))
                    print(GameInfo.Buildings[activePlayer:GetCivBuilding(GameInfoTypes[GameInfo.Buildings[v[1]].BuildingClass])].Type)
                end
            end
        end
    end
    function showDialogOnGameStart()
        activePlayerID = Game.GetActivePlayer()
        activePlayer = Players[activePlayerID]
        if not activePlayer:IsLostUC() then
            showDialog()
        else
            updateChosenUCList()
            addOptionalUCNotification()
            changeGreatWorkBuildings()
        end
    end
    Events.SequenceGameInitComplete.Add(showDialogOnGameStart)
    print("UC Selection Loaded")
)";
#endif