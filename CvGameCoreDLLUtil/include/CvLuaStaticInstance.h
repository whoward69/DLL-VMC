
#pragma once 
#ifndef CVLUASTATICINSTANCE_H

#include "CvLuaMethodWrapper.h"
#include "NetworkMessageUtil.h"


template<class Derived, class InstanceType>
class CvLuaStaticInstance : public CvLuaMethodWrapper<Derived, InstanceType>
{
public:	
	static void Register(lua_State* L);

	//! Used by CvLuaMethodWrapper to know where first argument is.
	static const int GetStartingArgIndex();

protected:
	static int pRegister(lua_State* L);

	static int lSendAndExecuteLuaFunction(lua_State* L);
	static int lSendAndExecuteLuaFunctionPostpone(lua_State* L);

	//! Called inside of pRegisterMembers to register a method.
	static void RegisterMethod(lua_State* L, lua_CFunction func, const char* funcName);
};

//------------------------------------------------------------------------------
// template implementation
//------------------------------------------------------------------------------
template<class Derived, class InstanceType>
void CvLuaStaticInstance<Derived, InstanceType>::Register(lua_State* L)
{
	FLua::Details::CCallWithErrorHandling(L, pRegister);
}
//------------------------------------------------------------------------------
template<class Derived, class InstanceType>
const int CvLuaStaticInstance<Derived, InstanceType>::GetStartingArgIndex()
{
	return 1;
}
//------------------------------------------------------------------------------
template<class Derived, class InstanceType>
int CvLuaStaticInstance<Derived, InstanceType>::pRegister(lua_State* L)
{
	const char* szInstanceName = Derived::GetInstanceName();
	lua_getglobal(L, szInstanceName);
	if(lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		lua_newtable(L);
		lua_pushvalue(L, -1);
		lua_setglobal(L, szInstanceName);
	}

	Derived::RegisterMembers(L);

	return 0;
}
//------------------------------------------------------------------------------
template<class Derived, class InstanceType>
void CvLuaStaticInstance<Derived, InstanceType>::RegisterMethod(lua_State *L, lua_CFunction func, const char* funcName)
{
	lua_pushcclosure(L, func, 0);
	lua_setfield(L, -2, funcName);
}
//------------------------------------------------------------------------------

template<class Derived, class InstanceType>
int CvLuaStaticInstance<Derived, InstanceType>::lSendAndExecuteLuaFunction(lua_State* L) {
	auto num = lua_gettop(L);
	std::string funcToCall;
	if (num < 1) return 0;
	for (int i = 1; i <= num; i++) {
		auto type = lua_type(L, i);

		BasicArguments* arg;
		
		if (i == 1) {
			
			if (type == LUA_TSTRING) {
				funcToCall = lua_tostring(L, i);
				continue;
			}
			else {
				NetworkMessageUtil::ReceiveLargeArgContainer.Clear();
				return 0;
			}
		}
		arg = NetworkMessageUtil::ReceiveLargeArgContainer.add_args();
		if (type == LUA_TNIL) {
			arg->set_argtype("nil");
		}
		if (type == LUA_TTABLE) {
			auto instance = CvLuaScopedInstance<CvLuaUnit, CvGameObjectExtractable>::GetInstance(L, i);
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
	lua_remove(L, 1); //remove the name of the function you want to execute.
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
int CvLuaStaticInstance<Derived, InstanceType>::lSendAndExecuteLuaFunctionPostpone(lua_State* L) {
	auto num = lua_gettop(L);
	std::string funcToCall;
	if (num < 1) return 0;
	for (int i = 1; i <= num; i++) {
		auto type = lua_type(L, i);
		BasicArguments* arg;
		if (i == 1) {
			if (type == LUA_TSTRING) {
				funcToCall = lua_tostring(L, i);
				continue;
			}
			else {
				NetworkMessageUtil::ReceiveLargeArgContainer.Clear();
				return 0;
			}
		}
		arg = NetworkMessageUtil::ReceiveLargeArgContainer.add_args();
		if (type == LUA_TNIL) {
			arg->set_argtype("nil");
		}
		if (type == LUA_TTABLE) {
			auto instance = CvLuaScopedInstance<CvLuaUnit, CvGameObjectExtractable>::GetInstance(L, i);
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
	lua_remove(L, 1); //remove the name of the function you want to execute.
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

#endif //CVLUASTATICINSTANCE_H