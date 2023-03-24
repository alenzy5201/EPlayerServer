#pragma once
#include "Logger.h"
#include "CServer.h"
#include <HttpParser.h>
#include <map>
#include "Crypto.h"
/*
* 1. 客户端的地址问题
* 2. 连接回调的参数问题
* 3. 接收回调的参数问题
*/
#define ERR_RETURN(ret, err) if(ret!=0){TRACEE("ret= %d errno = %d msg = [%s]", ret, errno, strerror(errno));return err;}

#define WARN_CONTINUE(ret) if(ret!=0){TRACEW("ret= %d errno = %d msg = [%s]", ret, errno, strerror(errno));continue;}


class CEdoyunPlayerServer :
	public CBusiness
{
public:
	CEdoyunPlayerServer(unsigned count) :CBusiness() 
	{
		m_count = count;
	}
	~CEdoyunPlayerServer()
	{

	}
	//对父类的纯虚函数进行实现
	virtual int BusinessProcess(CProcess* proc) {
		using namespace std::placeholders;
		int ret = 0;
		ret = setConnectedCallback(&CEdoyunPlayerServer::Connected, this, _1);
		ERR_RETURN(ret, -1);
		ret = setRecvCallback(&CEdoyunPlayerServer::Received, this, _1, _2);
		ERR_RETURN(ret, -2);
		ret = m_epoll.Create(m_count);
		ERR_RETURN(ret, -1);
		ret = m_pool.Start(m_count);
		ERR_RETURN(ret, -2);
		//开启线程池
		for (unsigned i = 0; i < m_count; i++) 
		{
			ret = m_pool.AddTask(&CEdoyunPlayerServer::ThreadFunc, this);
			ERR_RETURN(ret, -3);
		}
		int sock = 0;
		sockaddr_in addrin;
		while (m_epoll != -1) 
		{
			//接受父进程发送来的文件描述符sock
			ret = proc->RecvSocket(sock, &addrin);
			if (ret < 0 || (sock == 0))break;
			CSocketBase* pClient = new CSocket(sock);
			if (pClient == NULL)continue;
			//给套接字绑定信息
			ret = pClient->Init(CSockParam(&addrin, SOCK_ISIP));
			WARN_CONTINUE(ret);
			//让Epoll关注这个sock文件描述符，如果发生状态改变，则放入epoll中，让子线程去关注
			ret = m_epoll.Add(sock, EpollData((void*)pClient));
			if (m_connectedcallback) 
			{
				//如果是连接事件，除了放入之外，还需要执行对应的相应函数。
				(*m_connectedcallback)(pClient);
			}
			WARN_CONTINUE(ret);
		}
		return 0;
	}
private:
	int Connected(CSocketBase* pClient) {
		//简单打印客户端信息
		sockaddr_in* paddr = *pClient;
		TRACEI("client connected arrd %s port:%d", inet_ntoa(paddr->sin_addr),paddr->sin_port);
		return 0;
	}
	int Received(CSocketBase* pClient, const Buffer& data) {
		//主要业务，在此处理
		//HTTP解析
		int ret = 0;
		Buffer response = "";
		ret = HttpParser(data);
		if (ret != 0)
		{
			TRACEE("http parser failed!%d", ret);
			//失败的应答
			return 0;
		}
		else
		{

		}
		//数据库的查询
		//登录请求的验证
		//验证结果的反馈
		ret = pClient->Send(response);
		if (ret != 0)
		{
			TRACEE("http response failed!%d", ret);
		}
		else
		{
			TRACEI("http response success!%d", ret);
		}
		return 0;
	}
	int HttpParser(const Buffer& data)
	{
		CHttpParser parser;
		size_t size = parser.Parser(data);
		if (size == 0 || parser.Errno() != 0)
		{
			TRACEE("size:%llu error:%u", size, parser.Errno());
			return -1;
		}
		if (parser.Method() == HTTP_GET)
		{
			UrlParser url("https://192.168.1.200" + parser.Url());
			int ret = url.Parser();
			if (ret != 0)
			{
				TRACEE("ret = %d url[%s]", ret,"https://192.168.1.200"+parser.Url());
				return -2;
			}
			Buffer uri = url.Uri();
			if (uri == "login")
			{
				//处理登录
				Buffer time = url["time"];
				Buffer salt = url["salt"];
				Buffer user = url["user"];
				Buffer sign = url["sign"];
				TRACEI("time %s salt %s user %s");
				//数据库的登录 请求验证
			}

			//get
		}
		else if (parser.Method() == HTTP_POST)
		{
			//post 处理
		}
	}
	int ThreadFunc()
	{
		int ret = 0;
		EPEvents events;
		while (m_epoll != -1)
		{
			ssize_t size = m_epoll.WaitEvents(events);
			if (size < 0)
			{
				break;
			}
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
						//收到来自主进程中通过管道发送来的客户端的sock套接字
						CSocketBase* pClient = (CSocketBase*)events[i].data.ptr;
						if (pClient)
						{
							Buffer data;
							ret = pClient->Recv(data);
							WARN_CONTINUE(ret);
							if (m_recvcallback)
							{
								(*m_recvcallback)(pClient, data);
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
	std::map<int, CSocketBase*> m_mapClients;
	CThreadPool m_pool;
	unsigned m_count;
};
