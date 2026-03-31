Events.LoadScreenClose.Add(function()
	if not Game.CheckDllVersion(Modding.GetActivatedModVersion("a2969a0a-e4da-41cc-9e0b-19b29aa971d8")) then
		local str = Locale.ConvertTextKey("TXT_KEY_MPDLL_ERROR_VERSION")
		Events.SerialEventGameMessagePopup( { Type = ButtonPopupTypes.BUTTONPOPUP_TEXT, Data1 = 800, Option1 = true, Text = str } ) 
	end
end)