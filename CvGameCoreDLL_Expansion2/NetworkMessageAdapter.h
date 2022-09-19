#pragma once
#include <string>
#include "ArgContainer.pb.h"
#include "FunctionsRef.h"
#include "CvEnums.h"
#include "CvUnit.h"
#include "CvPlayerAI.h"
#include "LargeArgContainer.pb.h"


#define SERIALIZED_SHIFT_NUM 14; 

namespace NetworkMessageAdapter {
	//Shift the message string or it will be automatically truncated by CIV5 network methods.
	extern void StringShift(char* buffer, std::string& target);
	//Shift the message string reversely for decode.
	extern void StringShiftReverse(char* buffer, const char* target, int msgLength);
	extern void Clear(char* buffer, int msgLength);
	extern void SetArguments(ArgContainer& args, const std::string& name, 
		int arg1 = 0,
		int arg2 = 0,
		int arg3 = 0,
		int arg4 = 0,
		int arg5 = 0,
		int arg6 = 0,
		int arg7 = 0,
		int arg8 = 0,
		int arg9 = 0
	);

	extern char ReceiveBuffer[1024];
	extern ArgContainer ReceiveArgContainer;
	extern LargeArgContainer ReceivrLargeArgContainer;
}