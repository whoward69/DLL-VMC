# DLL-VMC

This repos fixes out-of-sync problem of playing with mods in multiplayer game by creating network syncronized varients of set methods. 

## Usage
Call *MethodSync* in lua instead of calling *Method*. For example, call 

``` lua
unit:KillSync() 
``` 
instead of 
``` lua
unit:Kill() 
``` 
Or create a row named *API_FORCE_SYNC_VER* in your GlobalDefines.xml and set its value to 1, then the setter methods calls in lua will be replaced by calling their sync versions.

## Caution
The DLL is **NOT** intensively tested.

 
