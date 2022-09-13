#pragma once

#include <unordered_map>

#define REGIST_INSTANCE_FUNCTION(T) FunctionPointerContainer::RegistFunction(#T, &T);

#define REGIST_INSTANCE_EXECUTER_FUNCTION(TYPE, EXECUTER) FunctionPointerContainer::RegistFunction(#TYPE, &EXECUTER);

#define EXECUTE_FUNC_WITH_ARGS(POINTER, ARGS) FunctionPointers::functionPointerUtil.ExecuteFunctionWrap<void>(POINTER, ARGS->functiontocall(),\
ARGS->args1(),\
ARGS->args2(),\
ARGS->args3(),\
ARGS->args4(),\
ARGS->args5(),\
ARGS->args6(),\
ARGS->args7(),\
ARGS->args8(),\
ARGS->args9()\
);\

class None {};

//Why VC9 has no variadic template?
class FunctionPointerContainer {
public:
	template<typename ClassType, typename returnType, typename... Args>
	static void RegistFunction(std::string name, returnType(ClassType::* func)(Args...)) {
		(*methods)[name] = std::make_pair((void(None::*)())func, 0);
	}

	template<typename ReturnType, typename ClassType, typename... Args>
	static ReturnType ExecuteFunction(ClassType* object, std::string& name, Args... args) {
		if (object == NULL || methods->find(name) == methods->end()) return;
		ReturnType(ClassType:: * func)(Args...) = (ReturnType(ClassType::*)(Args...))(*methods)[name].first;
		return (object->*func)(args...);
	}

	template<typename ReturnType, typename ClassType, typename arg1, typename arg2, typename arg3, typename arg4, typename arg5, typename arg6, typename arg7, typename arg8, typename arg9>
	static ReturnType ExecuteFunctionWrap(ClassType* object, std::string name, arg1 a1, arg2 a2, arg3 a3, arg4 a4, arg5 a5, arg6 a6, arg7 a7, arg8 a8, arg9 a9) {
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
		case 7:
			return ExecuteFunction<ReturnType>(object, name, a1, a2, a3, a4, a5, a6, a7);
		case 8:
			return ExecuteFunction<ReturnType>(object, name, a1, a2, a3, a4, a5, a6, a7, a8);
		case 9:
			return ExecuteFunction<ReturnType>(object, name, a1, a2, a3, a4, a5, a6, a7, a8, a9);
		default:
			break;
		}
	}

	FunctionPointerContainer() {
		methods = new std::tr1::unordered_map<std::string, std::pair<void(None::*)(), int>>();
		CvUnit::RegistReflectableFunctions();
		CvCity::RegistReflectableFunctions();
	}	
	~FunctionPointerContainer() {
		methods->clear();
		delete methods;
	}
private:
	static std::tr1::unordered_map<std::string, std::pair<void(None::*)(), int>>* methods;
};
namespace FunctionPointers {
	extern FunctionPointerContainer functionPointerUtil;
}

