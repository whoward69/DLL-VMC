# DLL-VMC

This repos is based on whoward69's DLL-VMC v97. It fixes out-of-sync problem of playing with mods in multiplayer game by creating lua interface allows to execute arbitrary lua-CPP interface methods or CPP functions on all the clients in a multiplayer game by providing their name and arguments. 

## Usage
Call *SendAndExecuteLuaFunction* in lua, the first argument is the function name you want to call. Remember the name should be prefixed with *ClassName::l* (it's actually their name in CPP source).
For example, if you are executing a custom button function, call 

``` lua
unit = player:SendAndExecuteLuaFunction("CvLuaPlayer::lInitUnit", GameInfoTypes[unitType], plotX, plotY)
unit:SendAndExecuteLuaFunction("CvLuaUnit::lKill", false)
``` 
instead of 
``` lua
unit = player:InitUnit(GameInfoTypes[unitType], plotX, plotY)
unit:Kill(false) 
``` 
This will execute the kill and spawn method with given arguments on all the clients, eliminating out-of-sync problem.


## How to Compile
You **must** have *VS2008* and *VS2013* tool chain installed. In project configuration, enter the page *VC++ directory*, change executable path to your *VS2013* compiler path, the remaining set to *VS2008* tool chain's path.

## Caution
Currently, not all the methods can be called in this way, available methods are:
``` c++
CvLuaUnit::lKill();
CvLuaPlayer::lInit();
``` 
The DLL is **NOT** intensively tested.

 
