#pragma once
namespace later_std {
	template<bool _Test, class _Ty = void>
	struct enable_if
	{	// type is undefined for assumed !_Test
	};

	template<class _Ty>
	struct enable_if<true, _Ty>
	{	// type is _Ty for _Test
		typedef _Ty type;
	};
	template<class _Ty, _Ty _Val>
	struct integral_constant
	{	// convenient template for integral constant types
		static const _Ty value = _Val;

		typedef _Ty value_type;
		typedef integral_constant<_Ty, _Val> type;

		operator value_type() const
		{	// return stored value
			return (value);
		}
	};


	typedef integral_constant<bool, true> true_type;
	typedef integral_constant<bool, false> false_type;

	template<class _Ty1, class _Ty2>
	struct is_same
		: false_type
	{	// determine whether _Ty1 and _Ty2 are the same type
	};

	template<class _Ty1>
	struct is_same<_Ty1, _Ty1>
		: true_type
	{	// determine whether _Ty1 and _Ty2 are the same type
	};
}

//Thanks stack-overflow user Passer By. Please refer to https://stackoverflow.com/questions/73685911/
namespace cpp11_polyfill
{
	template<class T> using Invoke = typename T::type;
	template<size_t...> struct seq { using type = seq; };
	template<class S1, class S2> struct concat;
	template<size_t... I1, size_t... I2>
	struct concat<seq<I1...>, seq<I2...>>
		: seq<I1..., (sizeof...(I1) + I2)...> {};
	template<class S1, class S2>
	using Concat = Invoke<concat<S1, S2>>;
	template<size_t N> struct gen_seq;
	template<size_t N> using GenSeq = Invoke<gen_seq<N>>;
	template<size_t N>
	struct gen_seq : Concat<GenSeq<N / 2>, GenSeq<N - N / 2>> {};
	template<> struct gen_seq<0> : seq<> {};
	template<> struct gen_seq<1> : seq<0> {};
}
template<size_t... Is>
using index_sequence = cpp11_polyfill::seq<Is...>;

template<size_t N>
using make_index_sequence = typename cpp11_polyfill::gen_seq<N>::type;