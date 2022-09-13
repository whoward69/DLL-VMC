#include "CvGameCoreDLLPCH.h"
#include "FunctionsRef.h"
namespace FunctionPointers {
	FunctionPointerContainer functionPointerUtil;
}

std::tr1::unordered_map<std::string, std::pair<void(None::*)(), int>>* FunctionPointerContainer::methods;
