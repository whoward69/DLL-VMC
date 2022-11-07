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
					//auto dbg = 
					lua_Debug ar;
					int res = -1;
					char* srcName = nullptr;
					if (lua_getstack(L, 1, &ar))
					{
						lua_getinfo(L, "sl", &ar);
						res = ar.currentline;
						srcName = ar.short_src;
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
					lua_getinfo(L, "sl", &ar);
					res = ar.currentline;
					srcName = ar.short_src;
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
	lua_remove(L, indexOfFuncName); //remove the name of the function you want to execute.
	lua_settop(L, num - 1);
	ReceiveLargeArgContainer.set_functiontocall(funcToCall);
	return num - 1;
}

LargeArgContainer NetworkMessageUtil::ReceiveLargeArgContainer;
int NetworkMessageUtil::ArgumentsToPass[MAX_INT32_ARGNUM];
std::list<std::pair<std::string, int>> InvokeRecorder::returnValueRecord;
std::map<std::string, list<std::pair<std::string, int>>::iterator> InvokeRecorder::valueMap;

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
		returnValueRecord.push_back(std::make_pair(invoke, 0));
		valueMap.insert(std::make_pair(invoke, --returnValueRecord.end()));
	}
}

bool InvokeRecorder::getInvokeExist(std::string& invoke) {
	auto& iter = valueMap.find(invoke);
	bool rtn = false;
	if (iter != valueMap.end()) {
		rtn = true;
		auto invokeNumber = --(iter->second->second);
		if (invokeNumber == 0) {
			returnValueRecord.erase((*iter).second);
			valueMap.erase(iter);
			
		}
	}
	return rtn;
}

namespace ReturnValueUtil {
	InvokeRecorder container;
}

NetworkMessageCollisionExceptopn::NetworkMessageCollisionExceptopn(const std::string& invoke_msg) {
	this->message = "Collision happened when invoke " + invoke_msg;
}
NetworkMessageCollisionExceptopn::~NetworkMessageCollisionExceptopn() {}
const char* NetworkMessageCollisionExceptopn::what()const throw() {
	return this->message.c_str();
}
