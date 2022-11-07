
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
	auto fault = NetworkMessageUtil::ProcessLuaArgForReflection(L, 1) < 0;
	if (fault) return 0;
	int time = GetTickCount();
	lua_Debug dbg;
	string errWhere = "";
	int line = 0;
	if (lua_getstack(L, 1, &dbg))
	{
		lua_getinfo(L, "sl", &dbg);
		line = dbg.currentline;
		errWhere += dbg.short_src;
	}

	NetworkMessageUtil::ReceiveLargeArgContainer.set_invokestamp(time + line);
	auto str = NetworkMessageUtil::ReceiveLargeArgContainer.SerializeAsString();
	try {
		InvokeRecorder::pushInvoke(str);
	}
	catch (NetworkMessageCollisionExceptopn e) {
		//lua_getinfo(L, "Sl", &dbg);
		char buf[1024] = { 0 };
		_itoa_s(line, buf, 10);
		string collisionMsg = "Collision happened: File name: ";
		collisionMsg += errWhere;
		collisionMsg += " at line: ";
		collisionMsg += buf;
		collisionMsg += " with call: ";
		collisionMsg += str;
		CUSTOMLOG(collisionMsg);
	}
	gDLL->SendRenameCity(-str.length(), str);
	auto rtn = 0;
	try {
		rtn = StaticFunctionReflector::ExecuteFunction<int>(NetworkMessageUtil::ReceiveLargeArgContainer.functiontocall(), L);
	}
	catch (NoSuchMethodException e) {
		CUSTOMLOG(e.what());
	}
	NetworkMessageUtil::ReceiveLargeArgContainer.Clear();
	return rtn;
}

template<class Derived, class InstanceType>
int CvLuaStaticInstance<Derived, InstanceType>::lSendAndExecuteLuaFunctionPostpone(lua_State* L) {
	auto fault = NetworkMessageUtil::ProcessLuaArgForReflection(L, 2) < 0;
	if (fault) return 0;
	int time = GetTickCount();
	NetworkMessageUtil::ReceiveLargeArgContainer.set_invokestamp(time);
	auto str = NetworkMessageUtil::ReceiveLargeArgContainer.SerializeAsString();
	gDLL->SendRenameCity(-str.length(), str);
	NetworkMessageUtil::ReceiveLargeArgContainer.Clear();
	return 0;
}

#endif //CVLUASTATICINSTANCE_H