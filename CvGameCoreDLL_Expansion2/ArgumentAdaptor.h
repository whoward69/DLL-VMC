#pragma once
//Thanks stack-overflow user Passer By. Please refer to https://stackoverflow.com/questions/73685911/

template<typename...>
class typelist {};

template<size_t, typename, typename, typename, typename>
class CallAdapter
{
public:
	template<typename... Ts>
	static int Transmit(Ts&&...) { return 0; }
};

template<size_t N, typename ReturnType, typename ClassType, typename... Accum, typename Head, typename... Tail>
class CallAdapter<N, ReturnType, ClassType, typelist<Accum...>, typelist<Head, Tail...>>
	: CallAdapter<N, ReturnType, ClassType, typelist<Accum..., Head>, typelist<Tail...>>
{
public:
	using CallAdapter<N, ReturnType, ClassType, typelist<Accum..., Head>, typelist<Tail...>>::Transmit;
	template<size_t D = 0, typename... Ts>
	static auto Transmit(ReturnType& ret, ClassType& object, string& name, Accum&... args, Ts&&...)
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
	: public CallAdapter<N, ReturnType, ClassType, typelist<Accum...>, typelist<>> {};

template<size_t N, typename ReturnType, typename ClassType, typename... Accum, typename Head, typename... Tail>
class CallAdapterUnAssignable<N, ReturnType, ClassType, typelist<Accum...>, typelist<Head, Tail...>>
	: CallAdapterUnAssignable<N, ReturnType, ClassType, typelist<Accum..., Head>, typelist<Tail...>>
{
public:
	using CallAdapterUnAssignable<N, ReturnType, ClassType, typelist<Accum..., Head>, typelist<Tail...>>::Transmit;
	template<size_t D = 0, typename... Ts>
	static auto Transmit(ClassType& object, string& name, Accum&... args, Ts&&...)
		-> typename later_std::enable_if<N + D == sizeof...(Accum), int>::type
	{
		InstanceFunctionReflector::ExecuteFunction<ReturnType>(object, name, args...);
		return 1;
	}
};


template<size_t, typename, typename, typename>
class CallAdapterStatic
{
public:
	template<typename... Ts>
	static int Transmit(Ts&&...) { return 0; }
};

template<size_t N, typename ReturnType, typename... Accum, typename Head, typename... Tail>
class CallAdapterStatic<N, ReturnType, typelist<Accum...>, typelist<Head, Tail...>>
	: CallAdapterStatic<N, ReturnType, typelist<Accum..., Head>, typelist<Tail...>>
{
public:
	using CallAdapterStatic<N, ReturnType, typelist<Accum..., Head>, typelist<Tail...>>::Transmit;
	template<size_t D = 0, typename... Ts>
	static auto Transmit(ReturnType& ret, string& name, Accum&... args, Ts&&...)
		-> typename later_std::enable_if<N + D == sizeof...(Accum), int>::type
	{
		ret = StaticFunctionReflector::ExecuteFunction<ReturnType>(name, args...);
		return 1;
	}
};

template<size_t, typename, typename, typename>
class CallAdapterUnAssignableStatic
{};

template<size_t N, typename ReturnType, typename... Accum>
class CallAdapterUnAssignableStatic<N, ReturnType, typelist<Accum...>, typelist<>>
	: public CallAdapterStatic<N, ReturnType, typelist<Accum...>, typelist<>> {};

template<size_t N, typename ReturnType, typename... Accum, typename Head, typename... Tail>
class CallAdapterUnAssignableStatic<N, ReturnType, typelist<Accum...>, typelist<Head, Tail...>>
	: CallAdapterUnAssignableStatic<N, ReturnType, typelist<Accum..., Head>, typelist<Tail...>>
{
public:
	using CallAdapterUnAssignableStatic<N, ReturnType, typelist<Accum..., Head>, typelist<Tail...>>::Transmit;
	template<size_t D = 0, typename... Ts>
	static auto Transmit(string& name, Accum&... args, Ts&&...)
		-> typename later_std::enable_if<N + D == sizeof...(Accum), int>::type
	{
		StaticFunctionReflector::ExecuteFunction<ReturnType>(name, args...);
		return 1;
	}
};
