#pragma once
#include <string>
#include "ArgContainer.pb.h"
#include "FunctionsRef.h"
#include "CvEnums.h"
#include "CvUnit.h"
#include "CvPlayerAI.h"


#define SERIALIZED_SHIFT_NUM 14; 

namespace NetworkMessageAdapter {
	extern void StringShift(char* buffer, std::string& target);
	extern void StringShiftReverse(char* buffer, const char* target, int msgLength);
}