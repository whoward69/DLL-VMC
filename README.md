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
 
