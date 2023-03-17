#pragma once
#include <unistd.h>
#include <sys/types.h>
#include <functional>
//一个函数：虚函数特性和模板函数特性，不能同时存在
//一个模板类可以有虚函数

class CSocketBase;
class Buffer;

class CFunctionBase
{
public:
	virtual ~CFunctionBase() {}
	//如果都是纯虚函数，不符合业务逻辑。因为以下几个函数只要其中一种形式，不需要全部实现。
	virtual int operator()() { return -1; }
	virtual int operator()(CSocketBase*) { return -1; }
	virtual int operator()(CSocketBase*, const Buffer&) { return -1; }
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