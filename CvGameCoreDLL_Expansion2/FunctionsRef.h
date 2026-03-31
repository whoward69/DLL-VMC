#pragma once

#include <unordered_map>
#include <LaterFeatures.h>
#include <ArgumentAdaptor.h>

#define REGIST_INSTANCE_FUNCTION(T) FunctionPointers::instanceFunctions.RegistFunction(#T, &T);
#define REGIST_STATIC_FUNCTION(T) FunctionPointers::staticFunctions.RegistFunction(#T, &T);

struct NoSuchMethodException :public std::exception
{
public:
	explicit NoSuchMethodException(const std::string& method_name);
	virtual ~NoSuchMethodException()throw();
	virtual const char* what()const throw();
protected:
	std::string message;
};

class None {};

class StaticFunctionReflector {
public:
	template<typename ReturnType, typename... Args>
	void RegistFunction(std::string&& name, ReturnType(*func)(Args...)) {
		(*methods)[name] = std::make_pair((void(*)())func, sizeof...(Args));
		(*names)[(void(*)())func] = name;
	}

	template<typename ReturnType, typename... Args>
	const std::string& GetFunctionPointerName(ReturnType(*func)(Args...)) {
		const auto& name = names->find((void(*)())func);
		if (name == names->end()) throw NoSuchMethodException("Unknown function in arguments");
		return name->second;
	}
	/*
	Please use pass-by-value to pass pointers of objects instead of pass-by-reference.
	*/
	template<typename ReturnType, typename... Args>
	ReturnType ExecuteFunction(std::string& name, Args&... args) {
		if (methods->find(name) == methods->end()) {
			throw NoSuchMethodException(name);
		}
		auto func = (ReturnType(*)(Args...))(*methods)[name].first;
		return (*func)(args...);
	}

	template<typename ReturnType, typename... Args>
	ReturnType ExecuteFunction(const std::string& name, Args&... args) {
		if (methods->find(name) == methods->end()) {
			throw NoSuchMethodException(name);
		}
		auto func = (ReturnType(*)(Args...))(*methods)[name].first;
		return (*func)(args...);
	}

	template<typename ReturnType,
		typename... Args, size_t... Is,
		typename later_std::enable_if<later_std::is_same<void, ReturnType>::value, int>::type = 0>
	ReturnType Transmit(int i, index_sequence<Is...> seq, string& name, Args&... args)
	{
		int unused[] = { (i == Is ?
			CallAdapterUnAssignableStatic<Is, ReturnType, typelist<>, typelist<Args..., void>>::Transmit(name, args...)
					   : 0)... };
		(void)unused;
	}

	template<typename ReturnType,
		typename... Args, size_t... Is,
		typename later_std::enable_if<!later_std::is_same<void, ReturnType>::value, int>::type = 0>
	ReturnType Transmit(int i, index_sequence<Is...> seq, string& name, Args&... args)
	{
		ReturnType ret{};
		int unused[] = { (i == Is ?
			CallAdapterStatic<Is, ReturnType, typelist<>, typelist<Args..., void>>::Transmit(ret, name, args...)
					   : 0)... };
		(void)unused;
		return ret;
	}

	/***
	Please use pass-by-value to pass pointers of objects if the function you want to call accepts object's 
	reference or their pointers.
	If you call a function with neither too much nor too less arguments, call ExecuteFunction() instead.
	If the return type of your function is not a simple value type (integers, pointers, bools, etc), please
	instantiate the function with ReturnType set to "void".
	***/
	template<typename ReturnType, typename... Args>
	ReturnType ExecuteFunctionWraps(std::string name, Args... args) {
		if (methods->find(name) == methods->end()) {
			throw NoSuchMethodException(name);
		}
		int argNum = (*methods)[name].second;
		return Transmit<ReturnType>(argNum, make_index_sequence<sizeof...(Args) + 1>{}, name, args...);
	}

	StaticFunctionReflector();
	~StaticFunctionReflector() {
		methods->clear();
		delete methods;
		if (names) {
			names->clear();
			delete names;
		}
	}
private:
	std::tr1::unordered_map<std::string, std::pair<void(*)(), int>>* methods;
	std::tr1::unordered_map<void(*)(), std::string>* names;
};


class InstanceFunctionReflector {
public:
	template<typename ReturnType, typename ClassType, typename... Args>
	void RegistFunction(std::string&& name, ReturnType(ClassType::* func)(Args...)) {
		(*methods)[name] = std::make_pair((void(None::*)())func, sizeof...(Args));
	}
	/*
	Please use pass-by-value to pass pointers of objects instead of pass-by-reference.
	*/
	template<typename ReturnType, typename ClassType, typename... Args>
	ReturnType ExecuteFunction(ClassType& object, std::string& name, Args&... args) {
		if (methods->find(name) == methods->end()) {
			throw NoSuchMethodException(name);
		}
		auto func = (ReturnType(ClassType::*)(Args...))(*methods)[name].first;
		return (object.*func)(args...);
	}

	template<typename ReturnType, typename ClassType, typename... Args>
	ReturnType ExecuteFunction(ClassType& object, const std::string& name, Args&... args) {
		if (methods->find(name) == methods->end()) {
			throw NoSuchMethodException(name);
		}
		auto func = (ReturnType(ClassType::*)(Args...))(*methods)[name].first;
		return (object.*func)(args...);
	}

	template<typename ReturnType, typename ClassType, typename... Args, size_t... Is,
		typename later_std::enable_if<later_std::is_same<void, ReturnType>::value, int>::type = 0>
	ReturnType Transmit(int i, index_sequence<Is...> seq, ClassType& object, string& name, Args&... args)
	{
		int unused[] = { (i == Is ?
			CallAdapterUnAssignable<Is, ReturnType, ClassType, typelist<>, typelist<Args..., void>>::Transmit(object, name, args...)
					   : 0)... };
		(void)unused;
	}

	template<typename ReturnType, typename ClassType, typename... Args, size_t... Is,
		typename later_std::enable_if<!later_std::is_same<void, ReturnType>::value, int>::type = 0>
	ReturnType Transmit(int i, index_sequence<Is...> seq, ClassType& object, string& name, Args&... args)
	{
		ReturnType ret{};
		int unused[] = { (i == Is ?
			CallAdapter<Is, ReturnType, ClassType, typelist<>, typelist<Args..., void>>::Transmit(ret, object, name, args...)
					   : 0)... };
		(void)unused;
		return ret;
	}

	/***
	Please use pass-by-value to pass pointers of objects if the function you want to call accepts object's
	reference or their pointers.
	If you call a function with neither too much nor too less arguments, call ExecuteFunction() instead.
	If the return type of your function is not a simple value type (integers, pointers, bools, etc), please
	instantiate the function with ReturnType set to "void".
	***/
	template<typename ReturnType, typename ClassType, typename... Args>
	ReturnType ExecuteFunctionWraps(ClassType& object, std::string name, Args... args) {
		if (methods->find(name) == methods->end()) {
			throw NoSuchMethodException(name);
		}
		int argNum = (*methods)[name].second;
		if (argNum > sizeof...(Args)) {
			throw "Arguments miss match";
		}
		return Transmit<ReturnType>(argNum, make_index_sequence<sizeof...(Args) + 1>{}, object, name, args...);
	}
	/***
	Make sure the argument list of the function you want to call is compatible with 32-bit integer,
	e.g. int, bool, float, object pointers, object references (if you pass its pointer instead).
	***/
	template<typename ReturnType, typename ClassType, size_t... Is>
	ReturnType ExecuteFunctionWrapsWithIntegerArray(ClassType& object, std::string name, int* idx, index_sequence<Is...> seq) {
		return ExecuteFunctionWraps<ReturnType>(object, name, idx[Is]...);
	}

	InstanceFunctionReflector();
	~InstanceFunctionReflector() {
		methods->clear();
		delete methods;
	}
private:
	std::tr1::unordered_map<std::string, std::pair<void(None::*)(), int>>* methods;
};
namespace FunctionPointers {
	extern InstanceFunctionReflector instanceFunctions;
	extern StaticFunctionReflector staticFunctions;
}

