#pragma once

#include <unordered_map>
#include <LaterFeatures.h>
#define REGIST_INSTANCE_FUNCTION(T) InstanceFunctionReflector::RegistFunction(#T, &T);

#define REGIST_INSTANCE_EXECUTER_FUNCTION(TYPE, EXECUTER) InstanceFunctionReflector::RegistFunction(#TYPE, &EXECUTER);

#define EXECUTE_FUNC_WITH_ARGS(REF, ARGS) FunctionPointers::functionPointerUtil.ExecuteFunctionWrap<void>(REF, ARGS->functiontocall(),\
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

struct NoSuchClassException :public std::exception
{
public:
	explicit NoSuchClassException(const std::string& type_name);
	virtual ~NoSuchClassException()throw();
	virtual const char* what()const throw();
protected:
	std::string message;
};

template<typename...>
class typelist {};

template<size_t, typename, typename, typename, typename>
class CallAdapter
{
public:
	template<typename... Ts>
	int operator()(Ts&&...){return 0;}
};

class None {};
template<size_t N, typename ReturnType, typename ClassType, typename... Accum, typename Head, typename... Tail>
class CallAdapter<N, ReturnType, ClassType, typelist<Accum...>, typelist<Head, Tail...>>
	: CallAdapter<N, ReturnType, ClassType, typelist<Accum..., Head>, typelist<Tail...>>
{
public:
	using CallAdapter<N, ReturnType, ClassType, typelist<Accum..., Head>, typelist<Tail...>>::operator();
	template<size_t D = 0, typename... Ts>
	auto operator()(ReturnType& ret, ClassType& object, string& name, Accum&... args, Ts&&...)
		-> typename later_std::enable_if<N + D == sizeof...(Accum), int>::type
	{
		ret = InstanceFunctionReflector::ExecuteFunction<ReturnType>(object, name, args...);
		return 1;
	}
};

template<size_t, typename, typename, typename, typename>
class CallAdapterUnAssignable
{};

template<size_t N, typename ReturnType, typename ClassType, typename... Accum>
class CallAdapterUnAssignable<N, ReturnType, ClassType, typelist<Accum...>, typelist<>> 
	: CallAdapter<N, ReturnType, ClassType, typelist<Accum...>, typelist<>> {};

template<size_t N, typename ReturnType, typename ClassType, typename... Accum, typename Head, typename... Tail>
class CallAdapterUnAssignable<N, ReturnType, ClassType, typelist<Accum...>, typelist<Head, Tail...>>
	: CallAdapterUnAssignable<N, ReturnType, ClassType, typelist<Accum..., Head>, typelist<Tail...>>
{
public:
	using CallAdapterUnAssignable<N, ReturnType, ClassType, typelist<Accum..., Head>, typelist<Tail...>>::operator();
	template<size_t D = 0, typename... Ts>
	auto operator()(ClassType& object, string& name, Accum&... args, Ts&&...)
		-> typename later_std::enable_if<N + D == sizeof...(Accum), int>::type
	{
		InstanceFunctionReflector::ExecuteFunction<ReturnType>(object, name, args...);
		return 1;
	}
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
		CvUnit::RegistReflectableFunctions();
		CvCity::RegistReflectableFunctions();
		CvPlayer::RegistReflectableFunctions();
	}	
	~InstanceFunctionReflector() {
		methods->clear();
		delete methods;
	}
private:
	static std::tr1::unordered_map<std::string, std::pair<void(None::*)(), int>>* methods;
};
namespace FunctionPointers {
	extern InstanceFunctionReflector functionPointerUtil;
}

