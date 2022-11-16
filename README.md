# CIV5-MPDLL

This repos is based on whoward69's DLL-VMC v97. It fixes out-of-sync problem of playing with mods in multiplayer game by creating lua interface allows to broadcast arbitrary lua-CPP interface operations or CPP functions (in progress) on all the clients in a multiplayer game by providing their names and arguments. 

# Usage
Call *SendAndExecuteLuaFunction* in lua, the first argument is the function name (or the function it self selected by ".") you want to call, and followings are the argument you want to pass. Remember if function names are provided, they should be prefixed with **ClassName::l** (it's actually their name in CPP source). 
*SendAndExecuteLuaFunction* is registered as static method for lua object **Game**, **Map** and as instance method for lua object **Unit**, **Player**, **Plot**, **City**.
For example, if you are executing a custom button function, call: 

``` lua
unit = player:SendAndExecuteLuaFunction("CvLuaPlayer::lInitUnit", GameInfoTypes[unitType], plotX, plotY)
unit:SendAndExecuteLuaFunction("CvLuaUnit::lKill", false)
Game.SendAndExecuteLuaFunction("CvLuaGame::lSetPlotExtraYield", x, y, GameInfoTypes.YIELD_FOOD, 1)
``` 
or
``` lua
unit = player:SendAndExecuteLuaFunction(player.InitUnit, GameInfoTypes[unitType], plotX, plotY)
unit:SendAndExecuteLuaFunction(unit.Kill, false)
Game.SendAndExecuteLuaFunction(Game.SetPlotExtraYield, x, y, GameInfoTypes.YIELD_FOOD, 1)
``` 
instead of 
``` lua
unit = player:InitUnit(GameInfoTypes[unitType], plotX, plotY)
unit:Kill(false) 
Game.SetPlotExtraYield(x, y, GameInfoTypes.YIELD_FOOD, 1)
``` 
This will execute the methods with given arguments on all the clients in a multiplayer game, eliminating out-of-sync problem.


# How to Compile
You **must** have both *VS2008* and *VS2013* tool chain installed. In project configuration, enter the page *VC++ directory*, change executable path to your *VS2013* compiler path (Usually **"\Microsoft Visual Studio 12.0\VC\bin"** and **"\Microsoft Visual Studio 12.0\Common7\IDE"**), the remaining set to *VS2008* tool chain's path.  
If you have them installed in the path **"C:\Program Files (x86)"**, you can open the sln file with *VS2022* and compile without modifiying configurations.
# Caution
Currently, not all the methods can be called in this way, available methods are:
``` c++
CvLuaUnit::lKill();
CvLuaUnit::lDoCommand();
CvLuaUnit::lPushMission();
CvLuaUnit::lJumpToNearestValidPlot();
CvLuaUnit::lEndTrader();
CvLuaUnit::lRecallTrader();
CvLuaUnit::lRangeStrike();

CvLuaPlayer::lInitUnit();
CvLuaPlayer::lInitCity();
CvLuaPlayer::lKillCities();
CvLuaPlayer::lKillUnits();
CvLuaPlayer::lAcquireCity();
CvLuaPlayer::lRaze();
CvLuaCity::lKill();
``` 
And all the lua methods prefixed with ***lSet***, ***lChange*** belongning to class **CvLuaUnit**, **CvLuaCity**, **CvLuaTeam**, **CvLuaTeamTech**,  **CvLuaPlot**, **CvLuaPlayer**, **CvLuaMap**, **CvLuaGame**.   
Currently you can pass ***LUA_TNUMBER***, ***LUA_TSTRING***, ***LUA_TBOOLEAN***, ***LUA_TNILL*** and lua game instances ***unit***, ***player***, ***team***, ***plot***, ***city***, ***teamTech*** which are actually ***LUA_TTABLE*** as arguments to invoke **SendAndExecuteLuaFunction**.   

 
