#include "CvGameCoreDLLPCH.h"
#include "FunctionsRef.h"
namespace FunctionPointers {
	InstanceFunctionReflector instanceFunctions;
	StaticFunctionReflector staticFunctions;
}

NoSuchMethodException::NoSuchMethodException(const std::string& method_name){
	this->message = "Method " + method_name + "does not exist in dictionry.";
}
NoSuchMethodException::~NoSuchMethodException(){}
const char* NoSuchMethodException::what()const throw(){
	return this->message.c_str();
}

std::tr1::unordered_map<std::string, std::pair<void(None::*)(), int>>* InstanceFunctionReflector::methods;
std::tr1::unordered_map<std::string, std::pair<void(*)(), int>>* StaticFunctionReflector::methods;
