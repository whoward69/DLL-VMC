#include <CvGameCoreDLLPCH.h>
#include "NetworkMessageUtil.h"
#include "CvLuaScopedInstance.h"



void NetworkMessageUtil::IClear(int* buffer) {
	for (int i = 0; i < 16; i++) {
		buffer[i] = 0;
	}
}

int NetworkMessageUtil::ProcessLuaArgForReflection(lua_State* L, int indexOfFuncName) {
	auto num = lua_gettop(L);
	std::string funcToCall;
	if (num < indexOfFuncName) {
		ReceiveLargeArgContainer.Clear();
		return -1;
	}
	/*
	* For broadcasting lua function, maybe we can directly send function pointers.
	* For broadcasting CPP function, reflection is needed.
	*/
	for (int i = 1; i <= num; i++) {
		auto type = lua_type(L, i);
		if (i == indexOfFuncName) {
			if (type == LUA_TSTRING) {
				funcToCall = lua_tostring(L, i);
				continue;
			}
			else if (type == LUA_TFUNCTION) {
				try {
					auto funcPtr = lua_tocfunction(L, i);
					funcToCall = StaticFunctionReflector::GetFunctionPointerName(funcPtr);
				}
				catch (NoSuchMethodException e) {
					ReceiveLargeArgContainer.Clear();
					lua_Debug ar;
					int res = -1;
					char* srcName = nullptr;
					if (lua_getstack(L, 1, &ar))
					{
						lua_getinfo(L, "Sl", &ar);
						res = ar.currentline;
						srcName = (char*)ar.source;
					}
					CUSTOMLOG("An unkwown function pointer passed to SendAndExecuteLuaFunction at %s : line %d", srcName ? srcName : "NULL", res);
					return -1;
				}
				continue;
			}
			else {
				ReceiveLargeArgContainer.Clear();
				lua_Debug ar;
				int res = -1;
				char* srcName = nullptr;
				if (lua_getstack(L, 1, &ar))
				{
					lua_getinfo(L, "Sl", &ar);
					res = ar.currentline;
					srcName = (char*)ar.source;
				}
				CUSTOMLOG("First argument passed to SendAndExecuteLuaFunction must be either string or function you want to call, at %s : line %d", srcName ? srcName : "NULL", res);
				
				return -1;
			}
		}
		auto arg = ReceiveLargeArgContainer.add_args();
		if (type == LUA_TNIL) {
			arg->set_argtype("nil");
		}
		if (type == LUA_TTABLE) {
			auto instance = CvLuaScopedInstance<CvGameObjectExtractable, CvGameObjectExtractable>::GetInstance(L, i);
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
#ifdef LUA_NETWORKMSG_DEBUG
	auto debugArg = ReceiveLargeArgContainer.add_args(); 
	lua_Debug ar;
	int res = -1;
	char* srcName = nullptr;
	if (lua_getstack(L, 1, &ar))
	{
		lua_getinfo(L, "Sl", &ar);
		res = ar.currentline;
		srcName = (char*)ar.source;
	}
	debugArg->set_argtype("LuaNetworkDebugMsg");
	debugArg->set_identifier1(res);
	debugArg->set_longmessage(srcName ? srcName : "Unknown");
#endif // LUA_NETWORKMSG_DEBUG

	lua_remove(L, indexOfFuncName); //remove the name of the function you want to execute.
	lua_settop(L, num - 1);
	ReceiveLargeArgContainer.set_functiontocall(funcToCall);
	return num - 1;
}

LargeArgContainer NetworkMessageUtil::ReceiveLargeArgContainer;
int NetworkMessageUtil::ArgumentsToPass[MAX_INT32_ARGNUM];
std::list<std::pair<std::string, int>> InvokeRecorder::returnValueRecord;
std::tr1::unordered_map<std::string, list<std::pair<std::string, int>>::iterator> InvokeRecorder::valueMap;

void InvokeRecorder::clear() {
	returnValueRecord.clear();
	valueMap.clear();
}


void InvokeRecorder::pushInvoke(std::string& invoke) {
	auto& targetEntry = valueMap.find(invoke);
	if (targetEntry != valueMap.end()) {
		//throw NetworkMessageCollisionExceptopn(invoke);
		targetEntry->second->second++;
	}
	else {
		if (returnValueRecord.size() >= MaxSize) {
			valueMap.erase(returnValueRecord.front().first);
			returnValueRecord.pop_front();
			CUSTOMLOG("MaxSize exceeded")
		}
		returnValueRecord.push_back(std::make_pair(invoke, 1));
		valueMap.insert(std::make_pair(invoke, --returnValueRecord.end()));
	}
}

bool InvokeRecorder::getInvokeExist(std::string& invoke) {
	auto& iter = valueMap.find(invoke);
	bool rtn = false;
	if (iter != valueMap.end()) {
		rtn = true;
		auto invokeNumber = --(iter->second->second);
		if (invokeNumber <= 0) {
			returnValueRecord.erase((*iter).second);
			valueMap.erase(iter);
		}
	}
	return rtn;
}

namespace ReturnValueUtil {
	InvokeRecorder container;
}

NetworkMessageNullPointerExceptopn::NetworkMessageNullPointerExceptopn(const std::string& typeName, int id1, int id2) {
	this->message = "A game object not exists locally passed to us, Type is: " + typeName;
	this->message += ", Identifiers are: ";
	char buffer[1024] = { 0 };
	_itoa_s(id1, buffer, 10);
	this->message += buffer;
	_itoa_s(id2, buffer, 10);
	this->message += ", ";
	this->message += buffer;
}
NetworkMessageNullPointerExceptopn::~NetworkMessageNullPointerExceptopn() {}
const char* NetworkMessageNullPointerExceptopn::what()const throw() {
	return this->message.c_str();
}
