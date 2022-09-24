/*	-------------------------------------------------------------------------------------------------------
	?1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */

#pragma once
#ifndef CVLUASCOPEDNSTANCE_H

#include "CvLuaMethodWrapper.h"
#include "NetworkMessageUtil.h"

template<class Derived, class InstanceType>
class CvLuaScopedInstance : public CvLuaMethodWrapper<Derived, InstanceType>
{
public:
	static void Push(lua_State* L, InstanceType* pkType);

/**
Simply push instance pointer without involving information like method table into a lua state.
Only for GetInstance() to correctly retrieve the game object.
**/
	static void PushLtwt(lua_State* L, InstanceType* pkType);
	static void Push(lua_State* L, FObjectHandle<InstanceType> handle)
	{
		Push(L, handle.pointer());
	}
	static InstanceType* GetInstance(lua_State* L, int idx = 1, bool bErrorOnFail = true);
	static int lSendAndExecuteLuaFunction(lua_State* L);
	static int lSendAndExecuteLuaFunctionPostpone(lua_State* L);

	//! Used by CvLuaMethodWrapper to know where first argument is.
	static const int GetStartingArgIndex();

protected:
	static void DefaultHandleMissingInstance(lua_State* L);
};



//------------------------------------------------------------------------------
// template members
//------------------------------------------------------------------------------
template<class Derived, class InstanceType>
void CvLuaScopedInstance<Derived, InstanceType>::Push(lua_State* L, InstanceType* pkType)
{
	//Pushing an instance involves more than just actually pushing a pointer into the
	//Lua stack.  There are some caching optimizations that are done as well as some
	//checks.
	//The first step is to load or create a global table <Typename> to store all member
	//methods and all pushed instances.  This conserves memory and offers faster pushing
	//speed.
	//If <Typename>.__instances[pkType] is not nil, return that value.
	//otherwise push a new instance and assign it to __instances.

	//NOTE: Raw gets and sets are used as an optimization over using lua_[get,set]field
	if(pkType)
	{
		//const int t = lua_gettop(L);

		lua_getglobal(L, Derived::GetTypeName());
		if(lua_isnil(L, -1))
		{
			//Typename wasn't found, time to build it.
			lua_pop(L, 1);
			lua_newtable(L);

			//Create weak __instances table.
			lua_pushstring(L, "__instances");
			lua_newtable(L);

			//Create __instances.mt
			lua_newtable(L);
			lua_pushstring(L, "__mode");
			lua_pushstring(L, "v");
			lua_rawset(L, -3);				// mt.__mode = "v";
			lua_setmetatable(L, -2);

			lua_rawset(L, -3);				//type.__instances = t;


			lua_pushvalue(L, -1);
			lua_setglobal(L, Derived::GetTypeName());

			Derived::PushMethods(L, lua_gettop(L));
		}
		const int type_index = lua_gettop(L);

		lua_pushstring(L, "__instances");
		lua_rawget(L, -2);

		const int instances_index = lua_gettop(L);

		lua_pushlightuserdata(L, pkType);

		lua_rawget(L, -2);					//retrieve type.__instances[pkType]

		if(lua_isnil(L, -1))
		{
			lua_pop(L, 1);

			//Push new instance
			lua_createtable(L, 0, 1);
			lua_pushlightuserdata(L, pkType);
			lua_setfield(L, -2, "__instance");
			lua_createtable(L, 0, 1);			// create mt
			lua_pushstring(L, "__index");
			lua_pushvalue(L, type_index);
			lua_rawset(L, -3);					// mt.__index = Type
			lua_setmetatable(L, -2);

			//Assign it in instances
			lua_pushlightuserdata(L, pkType);
			lua_pushvalue(L, -2);
			lua_rawset(L, instances_index);				//__instances[pkType] = t;
		}

		//VERIFY(instances_index > type_index);
		lua_remove(L, instances_index);
		lua_remove(L, type_index);

		//const int dt = lua_gettop(L);
		//VERIFY(dt == t + 1)
	}
	else
	{
		lua_pushnil(L);
	}
}


template<class Derived, class InstanceType>
void CvLuaScopedInstance<Derived, InstanceType>::PushLtwt(lua_State* L, InstanceType* pkType)
{
	if (pkType)
	{

		lua_createtable(L, 0, 1);
		lua_pushlightuserdata(L, pkType);
		lua_setfield(L, -2, "__instance");
		const int size = lua_gettop(L);
		int x = 0;
	}
	else
	{
		lua_pushnil(L);
	}
}
//------------------------------------------------------------------------------
template<class Derived, class InstanceType>
InstanceType* CvLuaScopedInstance<Derived, InstanceType>::GetInstance(lua_State* L, int idx, bool bErrorOnFail)
{
	const int stack_size = lua_gettop(L);
	bool bFail = true;

	InstanceType* pkInstance = NULL;
	if(lua_type(L, idx) == LUA_TTABLE)
	{
		lua_getfield(L, idx, "__instance");
		if(lua_type(L, -1) == LUA_TLIGHTUSERDATA)
		{
			pkInstance = static_cast<InstanceType*>(lua_touserdata(L, -1));
			if(pkInstance)
			{
				bFail = false;
			}
		}
	}

	lua_settop(L, stack_size);

	if(bFail && bErrorOnFail)
	{
		if(idx == 1)
			luaL_error(L, "Not a valid instance.  Either the instance is NULL or you used '.' instead of ':'.");
		Derived::HandleMissingInstance(L);
	}
	return pkInstance;
}
//------------------------------------------------------------------------------
template<class Derived, class InstanceType>
const int CvLuaScopedInstance<Derived, InstanceType>::GetStartingArgIndex()
{
	return 2;
}
//------------------------------------------------------------------------------
template<class Derived, class InstanceType>
void CvLuaScopedInstance<Derived, InstanceType>::DefaultHandleMissingInstance(lua_State* L)
{
	luaL_error(L, "Instance does not exist.");
}

template<class Derived, class InstanceType>
int CvLuaScopedInstance<Derived, InstanceType>::lSendAndExecuteLuaFunction(lua_State* L) {
	auto num = lua_gettop(L);
	std::string funcToCall;
	if (num < 2) return 0;
	for (int i = 1; i <= num; i++) {
		auto type = lua_type(L, i);
		BasicArguments* arg;
		if (i == 2) {
			if (type == LUA_TSTRING) {
				funcToCall = lua_tostring(L, i);
				continue;
			}
			else return 0;
		}
		arg = NetworkMessageUtil::ReceiveLargeArgContainer.add_args();
		if (type == LUA_TNIL) {
			arg->set_argtype("nil");
		}
		if (type == LUA_TTABLE) {
			auto instance = CvLuaScopedInstance<Derived, CvGameObjectExtractable>::GetInstance(L, i);
			instance->ExtractToArg(arg);
		}
		else if (type == LUA_TNUMBER) {
			auto number = (int)lua_tointeger(L, i);
			arg->set_argtype("int");
			arg->set_identifier1(number);
		}

		else if (type == LUA_TSTRING) {
			auto str = lua_tostring(L, i);
			arg->set_argtype("string");
			arg->set_longmessage(str);
		}
		else if (type == LUA_TBOOLEAN) {
			auto tf = lua_toboolean(L, i);
			arg->set_argtype("bool");
			arg->set_identifier1(tf);
		}
	}
	lua_remove(L, 2); //remove the name of the function you want to execute.
	lua_settop(L, num - 1);
	auto checkSum = 0;
	for (int i = 0; i < NetworkMessageUtil::ReceiveLargeArgContainer.args_size(); i++) {
		if (NetworkMessageUtil::ReceiveLargeArgContainer.args(i).has_identifier1()) {
			checkSum += NetworkMessageUtil::ReceiveLargeArgContainer.args(i).identifier1();
		}
		if (NetworkMessageUtil::ReceiveLargeArgContainer.args(i).has_identifier2()) {
			checkSum += 65001 * NetworkMessageUtil::ReceiveLargeArgContainer.args(i).identifier2();
		}
	}
	int time = GetTickCount() + rand() + checkSum;
	InvokeRecorder::pushReturnValue(time);
	NetworkMessageUtil::ReceiveLargeArgContainer.set_invokestamp(time);
	NetworkMessageUtil::ReceiveLargeArgContainer.set_functiontocall(funcToCall);
	auto str = NetworkMessageUtil::ReceiveLargeArgContainer.SerializeAsString();
	gDLL->SendRenameCity(-str.length(), str);
	NetworkMessageUtil::ReceiveLargeArgContainer.Clear();
	return StaticFunctionReflector::ExecuteFunction<int>(funcToCall, L);
}


template<class Derived, class InstanceType>
int CvLuaScopedInstance<Derived, InstanceType>::lSendAndExecuteLuaFunctionPostpone(lua_State* L) {
	auto num = lua_gettop(L);
	std::string funcToCall;
	if (num < 2) return 0;
	for (int i = 1; i <= num; i++) {
		auto type = lua_type(L, i);
		BasicArguments* arg;
		if (i == 2) {
			if (type == LUA_TSTRING) {
				funcToCall = lua_tostring(L, i);
				continue;
			}
			else return 0;
		}
		arg = NetworkMessageUtil::ReceiveLargeArgContainer.add_args();
		if (type == LUA_TNIL) {
			arg->set_argtype("nil");
		}
		if (type == LUA_TTABLE) {
			auto instance = CvLuaScopedInstance<Derived, CvGameObjectExtractable>::GetInstance(L, i);
			instance->ExtractToArg(arg);
		}
		else if (type == LUA_TNUMBER) {
			auto number = (int)lua_tointeger(L, i);
			arg->set_argtype("int");
			arg->set_identifier1(number);
		}

		else if (type == LUA_TSTRING) {
			auto str = lua_tostring(L, i);
			arg->set_argtype("string");
			arg->set_longmessage(str);
		}
		else if (type == LUA_TBOOLEAN) {
			auto tf = lua_toboolean(L, i);
			arg->set_argtype("bool");
			arg->set_identifier1(tf);
		}
	}
	lua_remove(L, 2); //remove the name of the function you want to execute.
	lua_settop(L, num - 1);
	int time = GetTickCount() + rand();
	//InvokeRecorder::pushReturnValue(time);
	NetworkMessageUtil::ReceiveLargeArgContainer.set_invokestamp(time);
	NetworkMessageUtil::ReceiveLargeArgContainer.set_functiontocall(funcToCall);
	auto str = NetworkMessageUtil::ReceiveLargeArgContainer.SerializeAsString();
	gDLL->SendRenameCity(-str.length(), str);
	NetworkMessageUtil::ReceiveLargeArgContainer.Clear();
	return 0;
}


#endif //CVLUASCOPEDNSTANCE_H