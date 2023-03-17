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
		m_server = NULL; //���ڹ����г�ʼ������Ϊ���õ�m_server�Ĵ������
		timespec tp = { 0,0 };
		clock_gettime(CLOCK_REALTIME, &tp);
		char* buf = NULL;
		asprintf(&buf, "%d.%d.sock", tp.tv_sec % 100000, tp.tv_nsec % 1000000);
		if (buf != NULL) {
			m_path = buf;
			free(buf);
		}//������Ļ�����start�ӿ������ж�m_path��������⡣
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
		if (m_server != NULL)return -1;//�Ѿ���ʼ����
		if (m_path.size() == 0)return -2;//���캯��ʧ�ܣ�����
		m_server = new CLocalSocket();
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
			ret = m_threads[i]->Start();
			if (ret != 0)return -8;
		}
		return 0;
	}
	void Close() {
		m_epoll.Close();
		if (m_server) 
		{
			//delete��ִ���ٶȺ��������Ǹ�ֵ��moveָ��ܿ졣�������
			CSocketBase* p = m_server;
			m_server = NULL;
			delete p;
		}
		for (auto thread : m_threads)
		{
			if (thread)delete thread;
		}
		m_threads.clear();
		//linux�µ�һ�н�Ϊ�ļ������ʹ�ñ����׽���ʱʹ�õ�Ҳ��.sock���ļ���
		//ʹ����Ϻ�Ҳ��Ҫ�ͷŸ��ļ���.sock�ļ�
		unlink(m_path);
	}
	template<typename _FUNCTION_, typename... _ARGS_>
	int AddTask(_FUNCTION_ func, _ARGS_... args) 
	{
		//thread_local,һ���߳�ӵ��һ���ñ�����һ���߳��ж�ε��øú�����Ҳֻ���ʼ��һ��
		static thread_local CLocalSocket client;
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
private:
	int TaskDispatch() {
		while (m_epoll != -1) {
			EPEvents events;
			int ret = 0;
			ssize_t esize = m_epoll.WaitEvents(events);
			if (esize > 0) {
				for (ssize_t i = 0; i < esize; i++) {
					if (events[i].events & EPOLLIN) {
						CSocketBase* pClient = NULL;
						//m_server״̬�����˸ı䣬��client�����������߳��е�server
						if (events[i].data.ptr == m_server) {//�ͻ����������ӣ��ѿͻ��˷���epoll�н��м���	
							ret = m_server->Link(&pClient);
							//printf("%s(%d):<%s> isTrue=%d\n", __FILE__, __LINE__, __FUNCTION__, pClient->m_param.attr & SOCK_ISSERVER);
							if (ret != 0)continue;
							ret = m_epoll.Add(*pClient, EpollData((void*)pClient));//�����ļ���������epoll�С����Ӹ��ļ�������pClient
							if (ret != 0) {
								delete pClient;
								continue;
							}
						}
						else {//�ͻ��˵���������,����η�������ĳ���߳̽������񣬲�ִ��
							pClient = (CSocketBase*)events[i].data.ptr;
							//printf("%s(%d):<%s> isTrue=%d\n", __FILE__, __LINE__, __FUNCTION__, pClient->m_param.attr & SOCK_ISSERVER);
							if (pClient) {
								CFunctionBase* base = NULL;
								Buffer data(sizeof(base));
								//��epoll��ptr���ռ�epool�д�ŵĿͻ�������
								ret = pClient->Recv(data);
								if (ret <= 0) {
									m_epoll.Del(*pClient);
									delete pClient;
									continue;
								}
								memcpy(&base, (char*)data, sizeof(base));
								if (base != NULL) {
									(*base)();
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
	//vector�е������ͱ���Ҫ���Ը�ֵ�����Ĭ�Ϲ��죬���ʹ��ָ��
	std::vector<CThread*> m_threads;
	CSocketBase* m_server;
	Buffer m_path;
};