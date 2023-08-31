#include "CServer.h"
#include "Logger.h"

int CServer::Init(CBusiness* business, const Buffer& ip, short port)
{
	int ret = 0;
	if (business == NULL) return -1;
	m_business = business;
	//m_fun:BusinessProcess(this,&process)
	ret = m_process.SetEntryFunction(&CBusiness::BusinessProcess, m_business, &m_process);
	if (ret != 0) return -2;
	ret = m_process.CreateSubProcess();
	if (ret != 0) return -3;
	ret = m_pool.Start(2);
	if (ret != 0) return -4;
	ret = m_epoll.Create(2);
	if (ret != 0) return -5;
	m_server = new CSocket();
	if (m_server != 0) return -6;
	ret = m_server->Init(CSockParam(ip, port, SOCK_ISSERVER | SOCK_ISIP));
	if (ret != 0) return -7;
	ret = m_epoll.Add(*m_server, EpollData((void*)m_server));
	if (ret != 0) return -8;
	for (size_t i = 0; i < m_pool.Size(); i++)
	{
		ret = m_pool.AddTask(&CServer::ThreadFunc, this); 
		if (ret != 0) return -9;
	}
	return 0;
}

int CServer::Run()
{
	while (m_server != NULL)
		usleep(10);
	return 0;
}

int CServer::Close()
{
	if (m_server)
	{
		CSocketBase* sock = m_server;
		m_server = NULL;
		m_epoll.Del(*sock);
		delete sock;
	}
	m_epoll.Close();
	m_process.SendFD(-1);
	m_pool.Close();
	return 0;
}

int CServer::ThreadFunc()
{
	int ret = 0;
	EPEvents events;
	while ((m_epoll != -1) && (m_server != NULL)) 
	{
		ssize_t size = m_epoll.WaitEvents(events);
		if (size < 0)break;
		if (size > 0) 
		{
			for (ssize_t i = 0; i < size; i++)
			{
				if (events[i].events & EPOLLERR) 
				{
					break;
				}
				else if (events[i].events & EPOLLIN) 
				{
					if (m_server)  //Init中添加的ip通信的服务端socket状态发生改变。获取客户端socket
					{   
						CSocketBase* pClient = NULL;
						ret = m_server->Link(&pClient);
						if (ret != 0)continue;
						//把客户端的socket文件描述符通过管道交给子进程。之后该client的数据都由子进程进行处理
						ret = m_process.SendSocket(*pClient, *pClient);
						delete pClient;
						if (ret != 0) 
						{
							TRACEE("send client %d failed!", (int)*pClient);
							continue;
						}
					}
				}
			}
		}
	}
	return 0;
}
