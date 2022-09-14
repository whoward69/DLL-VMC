#pragma once

#include <unordered_map>
#include <LaterFeatures.h>
#include <ArgumentAdaptor.h>

#define REGIST_INSTANCE_FUNCTION(T) InstanceFunctionReflector::RegistFunction(#T, &T);
#define REGIST_STATIC_FUNCTION(T) StaticFunctionReflector::RegistFunction(#T, &T);

#define EXECUTE_INSTANCE_FUNC_WITH_ARGS(REF, ARGS) InstanceFunctionReflector::ExecuteFunctionWraps<void>(REF, ARGS.functiontocall(),\
ARGS.args1(),\
ARGS.args2(),\
ARGS.args3(),\
ARGS.args4(),\
ARGS.args5(),\
ARGS.args6(),\
ARGS.args7(),\
ARGS.args8(),\
ARGS.args9()\
);\

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
			CallAdapterUnAssignableStatic<Is, ReturnType, typelist<>, typelist<Args..., void>>{}.operator()(name, args...)
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
			CallAdapterStatic<Is, ReturnType, typelist<>, typelist<Args..., void>>{}.operator()(ret, name, args...)
					   : 0)... };
		(void)unused;
		return ret;
	}

	template<typename ReturnType, typename... Args>
	static ReturnType ExecuteFunctionWraps(std::string name, Args... args) {
		if (methods->find(name) == methods->end()) {
			throw NoSuchMethodException(name);
		}
		int argNum = (*methods)[name].second;
		if (argNum > sizeof...(Args)) {
			throw "Arguments miss match";
		}
		return Transmit<ReturnType>(argNum, make_index_sequence<sizeof...(Args) + 1>{}, name, args...);
	}

	StaticFunctionReflector() {
		methods = new std::tr1::unordered_map<std::string, std::pair<void(*)(), int>>();
		CvUnit::RegistStaticFunctions();
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

	template<typename ReturnType, typename ClassType, typename... Args>
	static ReturnType ExecuteFunction(ClassType& object, std::string& name, Args&... args) {
		if (methods->find(name) == methods->end()) {
			throw "No such method";
		}
		auto func = (ReturnType(ClassType::*)(Args...))(*methods)[name].first;
		return (object.*func)(args...);
	}

	template<typename ReturnType,
		typename ClassType, typename... Args, size_t... Is,
		typename later_std::enable_if<later_std::is_same<void, ReturnType>::value, int>::type = 0>
	static ReturnType Transmit(int i, index_sequence<Is...> seq, ClassType& object, string& name, Args&... args)
	{
		int unused[] = { (i == Is ?
			CallAdapterUnAssignable<Is, ReturnType, ClassType, typelist<>, typelist<Args..., void>>{}.operator()(object, name, args...)
					   : 0)... };

		(void)unused;
	}

	template<typename ReturnType, 
		typename ClassType, typename... Args, size_t... Is,
		typename later_std::enable_if<!later_std::is_same<void, ReturnType>::value, int>::type = 0>
	static ReturnType Transmit(int i, index_sequence<Is...> seq, ClassType& object, string& name, Args&... args)
	{
		ReturnType ret{};
		int unused[] = { (i == Is ?
			CallAdapter<Is, ReturnType, ClassType, typelist<>, typelist<Args..., void>>{}.operator()(ret, object, name, args...)
					   : 0)... };
		(void)unused;
		return ret;
	}

	template<typename ReturnType, 
		typename ClassType, typename... Args>
	static ReturnType ExecuteFunctionWraps(ClassType& object, std::string name, Args... args) {
		if (methods->find(name) == methods->end()) {
			throw "No such method";
		}
		int argNum = (*methods)[name].second;
		if (argNum > sizeof...(Args)) {
			throw "Arguments miss match";
		}
		return Transmit<ReturnType>(argNum, make_index_sequence<sizeof...(Args) + 1>{}, object, name, args...);
	}

	InstanceFunctionReflector() {
		methods = new std::tr1::unordered_map<std::string, std::pair<void(None::*)(), int>>();
		CvUnit::RegistInstanceFunctions();
		CvCity::RegistInstanceFunctions();
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

