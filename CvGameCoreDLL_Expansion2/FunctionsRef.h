#pragma once

#include <unordered_map>

#define REGIST_INSTANCE_FUNCTION(T) FunctionPointerContainer::RegistFunction(#T, &T);

class None {};
//Why VC9 has no variadic template?
class FunctionPointerContainer {
public:
	static std::tr1::unordered_map<std::string, std::tr1::unordered_map<std::string, std::pair<void(None::*)(void*), int>>>* classesAndMethods;

	static std::tr1::unordered_map<std::string, std::pair<void(None::*)(void*), int>>* methods;
	template<typename ClassType, typename returnType>
	static void RegistFunction(std::string name, returnType(ClassType::* func)()) {
		(*methods)[name].first = (void(None::*)(void*))func;
		(*methods)[name].second = 0;
	}
	template<typename ClassType, typename returnType, typename arg1>
	static void RegistFunction(std::string name, returnType(ClassType::* func)(arg1)) {
		(*methods)[name].first = (void(None::*)(void*))func;
		(*methods)[name].second = 1;
	}
	template<typename ClassType, typename returnType, typename arg1, typename arg2>
	static void RegistFunction(std::string name, returnType(ClassType::* func)(arg1, arg2)) {
		(*methods)[name].first = (void(None::*)(void*))func;
		(*methods)[name].second = 2;
	};
	template<typename ClassType, typename returnType, typename arg1, typename arg2, typename arg3>
	static void RegistFunction(std::string name, returnType(ClassType::* func)(arg1, arg2, arg3)) {
		(*methods)[name].first = (void(None::*)(void*))func;
		(*methods)[name].second = 3;
	};
	template<typename ClassType, typename returnType, typename arg1, typename arg2, typename arg3, typename arg4>
	static void RegistFunction(std::string name, returnType(ClassType::* func)(arg1, arg2, arg3, arg4)) {
		(*methods)[name].first = (void(None::*)(void*))func;
		(*methods)[name].second = 4;
	};
	template<typename ClassType, typename returnType, typename arg1, typename arg2, typename arg3, typename arg4, typename arg5>
	static void RegistFunction(std::string name, returnType(ClassType::* func)(arg1, arg2, arg3, arg4, arg5)) {
		(*methods)[name].first = (void(None::*)(void*))func;
		(*methods)[name].second = 5;
	};
	template<typename ClassType, typename returnType, typename arg1, typename arg2, typename arg3, typename arg4, typename arg5, typename arg6>
	static void RegistFunction(std::string name, returnType(ClassType::* func)(arg1, arg2, arg3, arg4, arg5, arg6)) {
		(*methods)[name].first = (void(None::*)(void*))func;
		(*methods)[name].second = 6;
	};

	template<typename ReturnType, typename ClassType>
	static void ExecuteFunction(ClassType* object, std::string name) {
		if (object == NULL || methods->find(name) == methods->end()) return;
		ReturnType(ClassType:: * func)() = (ReturnType(ClassType::*)())(*methods)[name].first;
		return (object->*func)();
	}

	template<typename ReturnType, typename ClassType, typename arg1>
	static void ExecuteFunction(ClassType* object, std::string name, arg1 a1) {
		if (object == NULL || methods->find(name) == methods->end()) return;
		ReturnType(ClassType:: * func)(arg1) = (ReturnType(ClassType::*)(arg1))(*methods)[name].first;
		return (object->*func)(a1);
	}
	
	template<typename ReturnType, typename ClassType, typename arg1, typename arg2>
	static void ExecuteFunction(ClassType* object, std::string name, arg1 a1, arg2 a2) {
		if (object == NULL || methods->find(name) == methods->end()) return;
		ReturnType(ClassType:: * func)(arg1, arg2) = (ReturnType(ClassType::*)(arg1, arg2))(*methods)[name].first;
		return (object->*func)(a1, a2);
	}

	template<typename ReturnType, typename ClassType, typename arg1, typename arg2, typename arg3>
	static void ExecuteFunction(ClassType* object, std::string name, arg1 a1, arg2 a2, arg3 a3) {
		if (object == NULL || methods->find(name) == methods->end()) return;
		ReturnType(ClassType:: * func)(arg1, arg2, arg3) = (ReturnType(ClassType::*)(arg1, arg2, arg3))(*methods)[name].first;
		return (object->*func)(a1, a2, a3);
	}

	template<typename ReturnType, typename ClassType, typename arg1, typename arg2, typename arg3, typename arg4>
	static void ExecuteFunction(ClassType* object, std::string name, arg1 a1, arg2 a2, arg3 a3, arg4 a4) {
		if (object == NULL || methods->find(name) == methods->end()) return;
		ReturnType(ClassType:: * func)(arg1, arg2, arg3, arg4) = (ReturnType(ClassType::*)(arg1, arg2, arg3, arg4))(*methods)[name].first;
		return (object->*func)(a1, a2, a3, a4);
	}

	template<typename ReturnType, typename ClassType, typename arg1, typename arg2, typename arg3, typename arg4, typename arg5>
	static void ExecuteFunction(ClassType* object, std::string name, arg1 a1, arg2 a2, arg3 a3, arg4 a4, arg5 a5) {
		if (object == NULL || methods->find(name) == methods->end()) return;
		ReturnType(ClassType:: * func)(arg1, arg2, arg3, arg4, arg5) = (ReturnType(ClassType::*)(arg1, arg2, arg3, arg4, arg5))(*methods)[name].first;
		return (object->*func)(a1, a2, a3, a4, a5);
	}

	template<typename ReturnType, typename ClassType, typename arg1, typename arg2, typename arg3, typename arg4, typename arg5, typename arg6>
	static void ExecuteFunction(ClassType* object, std::string name, arg1 a1, arg2 a2, arg3 a3, arg4 a4, arg5 a5, arg6 a6) {
		if (object == NULL || methods->find(name) == methods->end()) return;
		ReturnType(ClassType:: * func)(arg1, arg2, arg3, arg4, arg5, arg6) = (ReturnType(ClassType::*)(arg1, arg2, arg3, arg4, arg5, arg6))(*methods)[name].first;
		return (object->*func)(a1, a2, a3, a4, a5, a6);
	}

	template<typename ReturnType, typename ClassType, typename arg1, typename arg2, typename arg3, typename arg4, typename arg5, typename arg6>
	static void ExecuteFunctionWrap(ClassType* object, std::string name, arg1 a1, arg2 a2, arg3 a3, arg4 a4, arg5 a5, arg6 a6) {
		if (object == NULL || methods->find(name) == methods->end()) return;
		int argNum = (*methods)[name].second;
		switch (argNum)
		{
		case 0:
			return ExecuteFunction<ReturnType>(object, name);
		case 1:
			return ExecuteFunction<ReturnType>(object, name, a1);
		case 2:
			return ExecuteFunction<ReturnType>(object, name, a1, a2);
		case 3:
			return ExecuteFunction<ReturnType>(object, name, a1, a2, a3);
		case 4:
			return ExecuteFunction<ReturnType>(object, name, a1, a2, a3, a4);
		case 5:
			return ExecuteFunction<ReturnType>(object, name, a1, a2, a3, a4, a5);
		case 6:
			return ExecuteFunction<ReturnType>(object, name, a1, a2, a3, a4, a5, a6);
		default:
			break;
		}
	}

	FunctionPointerContainer() {
		methods = new std::tr1::unordered_map<std::string, std::pair<void(None::*)(void*), int>>();
		REGIST_INSTANCE_FUNCTION(CvUnit::kill);
		//RegistFunction("CvUnit::kill", &CvUnit::kill);
		RegistFunction("CvUnit::doCommand", &CvUnit::doCommand);
		RegistFunction("CvUnit::jumpToNearestValidPlot", &CvUnit::jumpToNearestValidPlot);
		RegistFunction("CvUnit::setEmbarked", &CvUnit::setEmbarked);
	}	
	~FunctionPointerContainer() {
		methods->clear();
		delete methods;
	}
};
namespace FunctionPointers {
	extern FunctionPointerContainer functionPointerUtil;
}

