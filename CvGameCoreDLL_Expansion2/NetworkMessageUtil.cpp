#include <CvGameCoreDLLPCH.h>
#include "NetworkMessageUtil.h"
#include "CvLuaScopedInstance.h"

void NetworkMessageUtil::IClear(int* buffer) {
	for (int i = 0; i < 16; i++) {
		buffer[i] = 0;
	}
}

int NetworkMessageUtil::checkNum() {
	auto checkNum = 0;
	auto base = 1;
	for (int i = 0; i < ReceiveLargeArgContainer.args_size(); i++) {
		if (ReceiveLargeArgContainer.args(i).has_identifier1()) {
			checkNum += ReceiveLargeArgContainer.args(i).identifier1() * base;
		}
		if (ReceiveLargeArgContainer.args(i).has_identifier2()) {
			checkNum += 65001 * ReceiveLargeArgContainer.args(i).identifier2() * base;
		}
		base *= 107;
	}
	return checkNum;
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
					return -1;
				}
				continue;
			}
			else {
				ReceiveLargeArgContainer.Clear();
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
std::list<std::string> InvokeRecorder::returnValueRecord;
std::map<std::string, list<std::string>::iterator> InvokeRecorder::valueMap;

void InvokeRecorder::clear() {
	returnValueRecord.clear();
	valueMap.clear();
}


void InvokeRecorder::pushInvoke(std::string& invoke) {
	if (valueMap.find(invoke) != valueMap.end()) {
		CUSTOMLOG("Collision where invoke = %s", invoke);
	}
	if (returnValueRecord.size() >= MaxSize) {
		valueMap.erase(returnValueRecord.front());
		returnValueRecord.pop_front();
	}
	returnValueRecord.push_back(invoke);
	valueMap.insert(std::pair<std::string, list<std::string>::iterator>(invoke, --returnValueRecord.end()));
}

bool InvokeRecorder::getInvokeExist(std::string& invoke) {
	auto& iter = valueMap.find(invoke);
	bool rtn = false;
	if (iter != valueMap.end()) {
		rtn = true;
		returnValueRecord.erase((*iter).second);
		valueMap.erase(iter);
	}
	return rtn;
}

namespace ReturnValueUtil {
	InvokeRecorder container;
}
