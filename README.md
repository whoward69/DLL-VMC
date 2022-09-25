# CIV5-MPDLL

This repos is based on whoward69's DLL-VMC v97. It fixes out-of-sync problem of playing with mods in multiplayer game by creating lua interface allows to execute arbitrary lua-CPP interface methods or CPP functions (in progress) on all the clients in a multiplayer game by providing their name and arguments. 

# Usage
Call *SendAndExecuteLuaFunction* in lua, the first argument is the function name you want to call. Remember the name should be prefixed with ***ClassName::l*** (it's actually their name in CPP source).
For example, if you are executing a custom button function, call: 

``` lua
unit = player:SendAndExecuteLuaFunction("CvLuaPlayer::lInitUnit", GameInfoTypes[unitType], plotX, plotY)
unit:SendAndExecuteLuaFunction("CvLuaUnit::lKill", false)
Game.SendAndExecuteLuaFunction("CvLuaGame::lSetPlotExtraYield", x, y, GameInfoTypes.YIELD_FOOD, 1)
``` 
instead of 
``` lua
unit = player:InitUnit(GameInfoTypes[unitType], plotX, plotY)
unit:Kill(false) 
Game.SetPlotExtraYield(x, y, GameInfoTypes.YIELD_FOOD, 1)
``` 
This will execute the kill and spawn method with given arguments on all the clients, eliminating out-of-sync problem.


# How to Compile
You **must** have *VS2008* and *VS2013* tool chain installed. In project configuration, enter the page *VC++ directory*, change executable path to your *VS2013* compiler path (Usually **"\Microsoft Visual Studio 12.0\VC\bin"** and **"\Microsoft Visual Studio 12.0\Common7\IDE"**), the remaining set to *VS2008* tool chain's path.  
If you have them installed in the path **"C:\Program Files (x86)"**, you can just open the sln file with *VS2022* and compile.
# Caution
Currently, not all the methods can be called in this way, available methods are:
``` c++
CvLuaUnit::lKill();
CvLuaUnit::lDoCommand();
CvLuaUnit::lPushMission();
CvLuaPlayer::lInit();
CvLuaUnit::lJumpToNearestValidPlot();
``` 
And all the lua methods prefixed with ***lSet***, ***lChange***.   
Currently you can pass ***LUA_TNUMBER***, ***LUA_TSTRING***, ***LUA_TBOOLEAN***, ***LUA_TNILL*** and lua game instances ***unit***, ***player***, ***team***, ***plot***, ***city*** which are actually ***LUA_TTABLE*** as arguments to invoke **SendAndExecuteLuaFunction**.   

  
The DLL is **NOT** intensively tested.

 
