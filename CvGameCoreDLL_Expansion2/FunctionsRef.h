#pragma once

#include <unordered_map>
#include <LaterFeatures.h>
#include <ArgumentAdaptor.h>
#include "CvLuaUnit.h"

#define REGIST_INSTANCE_FUNCTION(T) InstanceFunctionReflector::RegistFunction(#T, &T);
#define REGIST_STATIC_FUNCTION(T) StaticFunctionReflector::RegistFunction(#T, &T);

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
	static void RegistFunction(std::string&& name, ReturnType(*func)(Args...)) {
		(*methods)[name] = std::make_pair((void(*)())func, sizeof...(Args));
	}
	/*
	Please use pass-by-value to pass pointers of objects instead of pass-by-reference.
	*/
	template<typename ReturnType, typename... Args>
	static ReturnType ExecuteFunction(std::string& name, Args&... args) {
		if (methods->find(name) == methods->end()) {
			throw NoSuchMethodException(name);
		}
		auto func = (ReturnType(*)(Args...))(*methods)[name].first;
		return (*func)(args...);
	}

	template<typename ReturnType,
		typename... Args, size_t... Is,
		typename later_std::enable_if<later_std::is_same<void, ReturnType>::value, int>::type = 0>
	static ReturnType Transmit(int i, index_sequence<Is...> seq, string& name, Args&... args)
	{
		int unused[] = { (i == Is ?
			CallAdapterUnAssignableStatic<Is, ReturnType, typelist<>, typelist<Args..., void>>::Transmit(name, args...)
					   : 0)... };
		(void)unused;
	}

	template<typename ReturnType,
		typename... Args, size_t... Is,
		typename later_std::enable_if<!later_std::is_same<void, ReturnType>::value, int>::type = 0>
	static ReturnType Transmit(int i, index_sequence<Is...> seq, string& name, Args&... args)
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
	static ReturnType ExecuteFunctionWraps(std::string name, Args... args) {
		if (methods->find(name) == methods->end()) {
			throw NoSuchMethodException(name);
		}
		int argNum = (*methods)[name].second;
		return Transmit<ReturnType>(argNum, make_index_sequence<sizeof...(Args) + 1>{}, name, args...);
	}

	StaticFunctionReflector() {
		methods = new std::tr1::unordered_map<std::string, std::pair<void(*)(), int>>(2048);
		CvUnit::RegistStaticFunctions();
		CvCity::RegistStaticFunctions();
		CvTeam::RegistStaticFunctions();
		CvPlot::RegistStaticFunctions();
		CvPlayer::RegistStaticFunctions();

		CvLuaUnit::RegistStaticFunctions();

	}
	~StaticFunctionReflector() {
		methods->clear();
		delete methods;
	}
private:
	static std::tr1::unordered_map<std::string, std::pair<void(*)(), int>>* methods;
};


class InstanceFunctionReflector {
public:
	template<typename ReturnType, typename ClassType, typename... Args>
	static void RegistFunction(std::string&& name, ReturnType(ClassType::* func)(Args...)) {
		(*methods)[name] = std::make_pair((void(None::*)())func, sizeof...(Args));
	}
	/*
	Please use pass-by-value to pass pointers of objects instead of pass-by-reference.
	*/
	template<typename ReturnType, typename ClassType, typename... Args>
	static ReturnType ExecuteFunction(ClassType& object, std::string& name, Args&... args) {
		if (methods->find(name) == methods->end()) {
			throw NoSuchMethodException(name);
		}
		auto func = (ReturnType(ClassType::*)(Args...))(*methods)[name].first;
		return (object.*func)(args...);
	}

	template<typename ReturnType, typename ClassType, typename... Args, size_t... Is,
		typename later_std::enable_if<later_std::is_same<void, ReturnType>::value, int>::type = 0>
	static ReturnType Transmit(int i, index_sequence<Is...> seq, ClassType& object, string& name, Args&... args)
	{
		int unused[] = { (i == Is ?
			CallAdapterUnAssignable<Is, ReturnType, ClassType, typelist<>, typelist<Args..., void>>::Transmit(object, name, args...)
					   : 0)... };
		(void)unused;
	}

	template<typename ReturnType, typename ClassType, typename... Args, size_t... Is,
		typename later_std::enable_if<!later_std::is_same<void, ReturnType>::value, int>::type = 0>
	static ReturnType Transmit(int i, index_sequence<Is...> seq, ClassType& object, string& name, Args&... args)
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
	static ReturnType ExecuteFunctionWraps(ClassType& object, std::string name, Args... args) {
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
	static ReturnType ExecuteFunctionWrapsWithIntegerArray(ClassType& object, std::string name, int* idx, index_sequence<Is...> seq) {
		return ExecuteFunctionWraps<ReturnType>(object, name, idx[Is]...);
	}

	InstanceFunctionReflector() {
		methods = new std::tr1::unordered_map<std::string, std::pair<void(None::*)(), int>>(2048);
		CvUnit::RegistInstanceFunctions();
		CvCity::RegistInstanceFunctions();
		CvPlot::RegistInstanceFunctions();
		CvTeam::RegistInstanceFunctions();
		CvPlayer::RegistInstanceFunctions();
	}	
	~InstanceFunctionReflector() {
		methods->clear();
		delete methods;
	}
private:
	static std::tr1::unordered_map<std::string, std::pair<void(None::*)(), int>>* methods;
};
namespace FunctionPointers {
	extern InstanceFunctionReflector instanceFunctions;
	extern StaticFunctionReflector staticFunctions;
}

