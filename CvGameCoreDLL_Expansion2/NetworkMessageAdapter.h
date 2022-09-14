#pragma once
#include <string>
#include "ArgContainer.pb.h"
#include "FunctionsRef.h"
#include "CvEnums.h"
#include "CvUnit.h"
#include "CvPlayerAI.h"


#define SERIALIZED_SHIFT_NUM 14; 

namespace NetworkMessageAdapter {
	//Shift the message string or it will be automatically truncated by CIV5 network methods.
	extern void StringShift(char* buffer, std::string& target);
	//Shift the message string reversely for decode.
	extern void StringShiftReverse(char* buffer, const char* target, int msgLength);
}