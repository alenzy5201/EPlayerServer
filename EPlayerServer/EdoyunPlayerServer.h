#pragma once
#include "Logger.h"
#include "CServer.h"
#include <HttpParser.h>
#include <map>
#include "Crypto.h"
/*
* 1. �ͻ��˵ĵ�ַ����
* 2. ���ӻص��Ĳ�������
* 3. ���ջص��Ĳ�������
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
	//�Ը���Ĵ��麯������ʵ��
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
		//�����̳߳�
		for (unsigned i = 0; i < m_count; i++) 
		{
			ret = m_pool.AddTask(&CEdoyunPlayerServer::ThreadFunc, this);
			ERR_RETURN(ret, -3);
		}
		int sock = 0;
		sockaddr_in addrin;
		while (m_epoll != -1) 
		{
			//���ܸ����̷��������ļ�������sock
			ret = proc->RecvSocket(sock, &addrin);
			if (ret < 0 || (sock == 0))break;
			CSocketBase* pClient = new CSocket(sock);
			if (pClient == NULL)continue;
			//���׽��ְ���Ϣ
			ret = pClient->Init(CSockParam(&addrin, SOCK_ISIP));
			WARN_CONTINUE(ret);
			//��Epoll��ע���sock�ļ����������������״̬�ı䣬�����epoll�У������߳�ȥ��ע
			ret = m_epoll.Add(sock, EpollData((void*)pClient));
			if (m_connectedcallback) 
			{
				//����������¼������˷���֮�⣬����Ҫִ�ж�Ӧ����Ӧ������
				(*m_connectedcallback)(pClient);
			}
			WARN_CONTINUE(ret);
		}
		return 0;
	}
private:
	int Connected(CSocketBase* pClient) {
		//�򵥴�ӡ�ͻ�����Ϣ
		sockaddr_in* paddr = *pClient;
		TRACEI("client connected arrd %s port:%d", inet_ntoa(paddr->sin_addr),paddr->sin_port);
		return 0;
	}
	int Received(CSocketBase* pClient, const Buffer& data) {
		//��Ҫҵ���ڴ˴���
		//HTTP����
		int ret = 0;
		Buffer response = "";
		ret = HttpParser(data);
		if (ret != 0)
		{
			TRACEE("http parser failed!%d", ret);
			//ʧ�ܵ�Ӧ��
			return 0;
		}
		else
		{

		}
		//���ݿ�Ĳ�ѯ
		//��¼�������֤
		//��֤����ķ���
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
				//�����¼
				Buffer time = url["time"];
				Buffer salt = url["salt"];
				Buffer user = url["user"];
				Buffer sign = url["sign"];
				TRACEI("time %s salt %s user %s");
				//���ݿ�ĵ�¼ ������֤
			}

			//get
		}
		else if (parser.Method() == HTTP_POST)
		{
			//post ����
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
						//�յ�������������ͨ���ܵ��������Ŀͻ��˵�sock�׽���
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
