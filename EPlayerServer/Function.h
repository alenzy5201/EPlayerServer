#pragma once
#include <unistd.h>
#include <sys/types.h>
#include <functional>

class CFunctionBase
{
public:
	virtual ~CFunctionBase() {}
	virtual int operator()() = 0;
};

template<typename _FUNCTION_, typename... _ARGS_>
class CFunction :public CFunctionBase
{
public:
	CFunction(_FUNCTION_ func, _ARGS_... args)
		:m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...)
		//先进行转发，再执行拆包,因为m_binder需要接受一个可变参数列表
	{}
	virtual ~CFunction() {}
	virtual int operator()() {
		return m_binder();
	}
	//该模板会计算出std::function<int(func,arg...)的函数模板类型
	typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder;
};