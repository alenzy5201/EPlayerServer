#pragma once
#pragma once
#include "Epoll.h"
#include "Thread.h"
#include "Function.h"
#include "Socket.h"

class CThreadPool
{
public:
	CThreadPool() 
	{
		m_server = NULL; //不在构造中初始化，是为了拿到m_server的错误情况
		timespec tp = { 0,0 };
		clock_gettime(CLOCK_REALTIME, &tp);
		char* buf = NULL;
		asprintf(&buf, "%d.%d.sock", tp.tv_sec % 100000, tp.tv_nsec % 1000000);
		if (buf != NULL) {
			m_path = buf;
			free(buf);
		}//有问题的话，在start接口里面判断m_path来解决问题。
		usleep(1);
	}
	~CThreadPool() 
	{
		Close();
	}
	CThreadPool(const CThreadPool&) = delete;
	CThreadPool& operator=(const CThreadPool&) = delete;
public:
	int Start(unsigned count) {
		int ret = 0;
		if (m_server != NULL)return -1;//已经初始化了
		if (m_path.size() == 0)return -2;//构造函数失败！！！
		m_server = new CSocket();
		if (m_server == NULL)return -3;
		ret = m_server->Init(CSockParam(m_path, SOCK_ISSERVER));
		if (ret != 0)return -4;
		ret = m_epoll.Create(count);
		if (ret != 0)return -5;
		ret = m_epoll.Add(*m_server, EpollData((void*)m_server));
		if (ret != 0)return -6;
		m_threads.resize(count);
		for (unsigned i = 0; i < count; i++) {
			m_threads[i] = new CThread(&CThreadPool::TaskDispatch, this);
			if (m_threads[i] == NULL)return -7;
			ret = m_threads[i]->Start(); //开启线程，在线程中执行TaskDispatch函数
			if (ret != 0)return -8;
		}
		return 0;
	}
	void Close() {
		m_epoll.Close();
		if (m_server) 
		{
			//delete的执行速度很慢，但是赋值的move指令很快。避免出错
			CSocketBase* p = m_server;
			m_server = NULL;
			delete p;
		}
		for (auto thread : m_threads)
		{
			if (thread)delete thread;
		}
		m_threads.clear();
		//linux下的一切皆为文件，因此使用本地套接字时使用的也是.sock的文件。
		//使用完毕后，也需要释放该文件：.sock文件
		unlink(m_path);
	}

	//每个线程拥有一个addTask，并在每一个线程中执行一次 一个TaskDispatch
	template<typename _FUNCTION_, typename... _ARGS_>
	int AddTask(_FUNCTION_ func, _ARGS_... args) 
	{
		//thread_local,一个线程拥有一个该变量。一个线程中多次调用该函数，也只会初始化一次
		static thread_local CSocket client;
		int ret = 0;
		if (client == -1) {
			ret = client.Init(CSockParam(m_path, 0));
			if (ret != 0)return -1;
			ret = client.Link();
			if (ret != 0)return -2;
		}
		CFunctionBase* base = new CFunction< _FUNCTION_, _ARGS_...>(func, args...);
		if (base == NULL)return -3;
		Buffer data(sizeof(base));
		memcpy(data, &base, sizeof(base));
		ret = client.Send(data);
		if (ret != 0) {
			delete base;
			return -4;
		}
		return 0;
	}
	size_t Size() const { return m_threads.size(); }
private:
	//在每个线程中执行taskDispath. 每个线程都会监听同一个主线程中的m_epoll. 如果主线程中向sock发送数据。空闲的线程会拿到这一个epoll事件。进行响应
	int TaskDispatch() {
		while (m_epoll != -1) {
			EPEvents events;
			int ret = 0;
			ssize_t esize = m_epoll.WaitEvents(events);
			if (esize > 0) {
				for (ssize_t i = 0; i < esize; i++) {
					if (events[i].events & EPOLLIN) {
						CSocketBase* pClient = NULL;
						//m_server状态发生了改变，有client请求连接主线程中的server
						if (events[i].data.ptr == m_server) {//客户端请求连接，把客户端放入epoll中进行监视	
							ret = m_server->Link(&pClient);
							//printf("%s(%d):<%s> isTrue=%d\n", __FILE__, __LINE__, __FUNCTION__, pClient->m_param.attr & SOCK_ISSERVER);
							if (ret != 0)continue;
							ret = m_epoll.Add(*pClient, EpollData((void*)pClient));//加入文件描述符到epoll中。监视该文件描述符pClient
							if (ret != 0) {
								delete pClient;
								continue;
							}
						}
						else {//客户端的数据来了,任务段分配任务，某个线程接受任务，并执行
							pClient = (CSocketBase*)events[i].data.ptr;
							//printf("%s(%d):<%s> isTrue=%d\n", __FILE__, __LINE__, __FUNCTION__, pClient->m_param.attr & SOCK_ISSERVER);
							if (pClient) {
								CFunctionBase* base = NULL;
								Buffer data(sizeof(base));
								//用epoll的ptr来收集epool中存放的客户端数据
								ret = pClient->Recv(data);
								if (ret <= 0) {
									m_epoll.Del(*pClient);
									delete pClient;
									continue;
								}
								memcpy(&base, (char*)data, sizeof(base));
								if (base != NULL) {
									(*base)(); //最终逻辑
									delete base;
								}
							}
						}
					}
				}
			}
		}
		return 0;
	}

private:
	CEpoll m_epoll;
	//vector中的类类型必须要可以赋值构造和默认构造，因此使用指针
	std::vector<CThread*> m_threads;
	CSocketBase* m_server;
	Buffer m_path;
};