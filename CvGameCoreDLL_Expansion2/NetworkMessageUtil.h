#pragma once
#include <string>
#include "FunctionsRef.h"
#include "CvEnums.h"
#include "CvUnit.h"
#include "CvPlayerAI.h"
#include "LargeArgContainer.pb.h"
#include <FCriticalSection.h>

#define MAX_INT32_ARGNUM 16
#define SERIALIZED_SHIFT_NUM 14

namespace NetworkMessageUtil {
	//Shift the message string or it will be automatically truncated by CIV5 network methods.
	extern void StringShift(char* buffer, std::string& target);
	//Shift the message string reversely for decode.
	extern void StringShiftReverse(char* buffer, const char* target, int msgLength);
	extern void CClear(char* buffer, int msgLength);
	extern void IClear(int* buffer);
	extern char ReceiveBuffer[1024];
	extern LargeArgContainer ReceiveLargeArgContainer;
	extern int ArgumentsToPass[MAX_INT32_ARGNUM];
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

class InvokeRecorder {
public:
	static const int MaxSize = 65536;

	InvokeRecorder() {
	}
	~InvokeRecorder() {
		//returnValueRecord.clear();
		//valueMap.clear();
	}
	static std::list<int> returnValueRecord;
	static std::map<int, std::list<int>::iterator> valueMap;
	static FCriticalSection m_Locker;
	static void pushReturnValue(int time);
	static bool getReturnValueExist(int time);
};
namespace ReturnValueUtil {
	extern InvokeRecorder container;
}