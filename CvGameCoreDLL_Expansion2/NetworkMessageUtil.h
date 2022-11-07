#pragma once
#include <string>
#include "FunctionsRef.h"
#include "CvEnums.h"
#include "CvUnit.h"
#include "CvPlayerAI.h"
#include "LargeArgContainer.pb.h"
#include <FCriticalSection.h>

#define MAX_INT32_ARGNUM 16

namespace NetworkMessageUtil {
	extern void IClear(int* buffer);
	extern LargeArgContainer ReceiveLargeArgContainer;
	extern int ArgumentsToPass[MAX_INT32_ARGNUM];
	extern int ProcessLuaArgForReflection(lua_State* L, int indexOfFuncName);
	//template<typename InstanceType>
	/*void InstanceArrExecute(InstanceType& reference, ArgContainer* args) {
		for (int i = 0; i < args->args_size(); i++) {
			ArgumentsToPass[i] = args->args().Get(i);
		}
		auto index = make_index_sequence<MAX_INT32_ARGNUM>{};
		InstanceFunctionReflector::ExecuteFunctionWrapsWithIntegerArray<void>(reference, args->functiontocall(),
			ArgumentsToPass, index);
		IClear(ArgumentsToPass);
	}*/
}

struct NetworkMessageCollisionExceptopn :public std::exception
{
public:
	explicit NetworkMessageCollisionExceptopn(const std::string& method_name);
	virtual ~NetworkMessageCollisionExceptopn()throw();
	virtual const char* what()const throw();
protected:
	std::string message;
};


class InvokeRecorder {
public:
	static const int MaxSize = 262144;

	InvokeRecorder() {
	}
	~InvokeRecorder() {
		clear();
	}
	static void clear();
	static std::list<std::pair<std::string, int>> returnValueRecord;
	static std::map<std::string, std::list<std::pair<std::string, int>>::iterator> valueMap;
	static void pushInvoke(std::string& invoke);
	static bool getInvokeExist(std::string& invoke);
};
namespace ReturnValueUtil {
	extern InvokeRecorder container;
}