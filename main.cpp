//#include <cstdio>
//#include<memory.h>
//#include <unistd.h>
//#include <functional>
//#include <sys/socket.h>
//#include <sys/stat.h>
//#include <fcntl.h>
//
//class CFunctionBase
//{
//public:
//    virtual ~CFunctionBase() {}
//    virtual int operator()() = 0;
//};
//
//template<typename _FUNCTION_, typename... _ARGS_>
//class CFunction : public CFunctionBase
//{
//public:
//    CFunction(_FUNCTION_ func, _ARGS_... args) : m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...)
//    {
//
//    }
//    virtual ~CFunction() {}
//    virtual int operator()()
//    {
//        return m_binder();
//    }
//    //
//    typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder; 
//};
//
//class CProcess
//{
//public:
//    CProcess()
//    {
//        m_func = NULL;
//        memset(pipes, 0, sizeof(pipes));
//    }
//    ~CProcess()
//    {
//        if (m_func != NULL)
//        {
//            delete m_func;
//            m_func = NULL;
//        }
//    }
//    template<typename _FUNCTION_,typename... _ARGS_>
//    int SetEntryFunction(_FUNCTION_ func, _ARGS_... args)
//    {
//        m_func = new CFunction< _FUNCTION_, _ARGS_...>(func, args...);
//        return 0;
//    }
//
//    int CreateSubProcess()
//    {
//        if (m_func == NULL) return -1;
//        int  ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, pipes);
//        if (ret == -1) return -2;
//        pid_t pid = fork();
//        if (pid == -1) return -3;
//        if (pid == 0) //子进程
//        {
//            close(pipes[1]);//关闭掉写,子进程中只能读消息
//            pipes[1] = 0;
//            //子进程入口初始化，调用()运算符的重载
//            ret =  (*m_func)();
//            exit(0);
//        }
//        //父进程
//        close(pipes[0]);//关闭掉读端，父进程中只能写消息
//        pipes[0] = 0;
//        m_pid = pid;
//        return 0;
//    }
//
//    int SendFD(int fd) //主进程完成
//    {
//        struct msghdr msg;
//        iovec iov[2];
//        char buf[2][10] = { "edoyun","jueding"};
//        iov[0].iov_base = buf[0];
//        iov[0].iov_len = sizeof(buf[0]);
//        iov[1].iov_base = buf[1];
//        iov[1].iov_len = sizeof(buf[1]);
//        msg.msg_iov = iov;
//        msg.msg_iovlen = 2;
//
//        //下面的数据才是真正需要传递的
//        cmsghdr* cmsg = (cmsghdr*)calloc(1,CMSG_LEN(sizeof(int)));
//        if (cmsg == NULL) return -1;
//        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
//        cmsg->cmsg_level = SOL_SOCKET;
//        cmsg->cmsg_type = SCM_RIGHTS;
//        *(int*)CMSG_DATA(cmsg) = fd;
//        msg.msg_control = cmsg;
//        msg.msg_controllen = cmsg->cmsg_len;
//        //通过消息机制向子进程中传递文件描述符。
//        ssize_t ret = sendmsg(pipes[1], &msg, 0);
//        free(cmsg);
//        if (ret == -1)
//        {
//            close(pipes[1]);
//            return -2;
//        }
//        return  0;
//        close(pipes[1]);
//    }
//
//    int RecvFD(int& fd)
//    {
//        msghdr msg;
//        iovec iov[2];
//        char buf[][10] = { "","" };
//        iov[0].iov_base = buf[0];
//        iov[0].iov_len = sizeof(buf[0]);
//        iov[1].iov_base = buf[1];
//        iov[1].iov_len = sizeof(buf[1]);
//        msg.msg_iov = iov;
//        msg.msg_iovlen = 2;
//
//        cmsghdr* cmsg = (cmsghdr*)calloc(1,
//            CMSG_LEN(sizeof(int)));
//        if (cmsg == NULL)return -1;
//        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
//        cmsg->cmsg_level = SOL_SOCKET;
//        cmsg->cmsg_type = SCM_RIGHTS;
//        msg.msg_control = cmsg;
//        msg.msg_controllen =
//            CMSG_LEN(sizeof(int));
//        ssize_t ret = recvmsg(pipes[0], &msg,
//            0);
//        if (ret == -1) {
//            free(cmsg);
//            close(pipes[0]);
//            return -2;
//        }
//        fd = *(int*)CMSG_DATA(cmsg);
//        close(pipes[0]);
//        return 0;
//
//    }
//private:
//    CFunctionBase* m_func;//指针类型的类变量，不会因为CFunction中的类没有构造函数导致的空指针问题。
//    pid_t m_pid;
//    int pipes[2];
//};
//
//int CreateLogServer(CProcess* proc)
//{
//    printf("%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
//    return 0;
//}
//
//int CreateClientServer(CProcess* proc)
//{
//    printf("%s(%d):<%s> pid=%d\n", __FILE__,__LINE__, __FUNCTION__, getpid());
//    int fd = -1;
//    int ret = proc->RecvFD(fd);
//    printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
//    printf("%s(%d):<%s> fd=%d\n", __FILE__,__LINE__, __FUNCTION__, fd);
//    sleep(1);
//    char buf[10] = "";
//    lseek(fd, 0, SEEK_SET);
//    read(fd, buf, sizeof(buf));
//    printf("%s(%d):<%s> buf=%s\n", __FILE__, __LINE__, __FUNCTION__, buf);
//    close(fd);
//    return 0;
//}
//int main()
//{
//    CProcess proclog, procclients;
//    printf("%s(%d):<%s> pid=%d\n", __FILE__,__LINE__, __FUNCTION__, getpid());
//    proclog.SetEntryFunction(CreateLogServer, &proclog);
//    int ret = proclog.CreateSubProcess();
//    if (ret != 0)
//    {
//        printf("%s(%d):<%s> pid=%d\n", __FILE__,__LINE__, __FUNCTION__, getpid());
//        return -1;
//    }
//
//    printf("%s(%d):<%s> pid=%d\n", __FILE__,__LINE__, __FUNCTION__, getpid());
//    procclients.SetEntryFunction(CreateClientServer, &procclients);
//    ret = procclients.CreateSubProcess();
//    if (ret != 0)
//    {
//        printf("%s(%d):<%s> pid=%d\n", __FILE__,__LINE__, __FUNCTION__, getpid());
//        return -2;
//    }
//    printf("%s(%d):<%s> pid=%d\n", __FILE__,__LINE__, __FUNCTION__, getpid());
//    usleep(100 * 000);
//
//    int fd = open("./test.txt", O_RDWR | O_CREAT| O_APPEND);
//    printf("%s(%d):<%s> fd=%d\n", __FILE__,__LINE__, __FUNCTION__, fd);
//
//    if (fd == -1)return -3;
//    ret = procclients.SendFD(fd);
//    printf("%s(%d):<%s> ret=%d\n", __FILE__,__LINE__, __FUNCTION__, ret);
//    if (ret != 0)printf("errno:%d msg:%s\n",
//        errno, strerror(errno));
//    write(fd, "edoyun", 6);
//    close(fd);
//    return 0;
//}


#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <memory.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "Function.h"



class CProcess
{
public:
	CProcess() {
		m_func = NULL;
		memset(pipes, 0, sizeof(pipes));
	}
	~CProcess() {
		if (m_func != NULL) {
			delete m_func;
			m_func = NULL;
		}
	}

	template<typename _FUNCTION_, typename... _ARGS_>
	int SetEntryFunction(_FUNCTION_ func, _ARGS_... args)
	{
		m_func = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
		return 0;
	}

	int CreateSubProcess() {
		if (m_func == NULL)return -1;
		int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, pipes);
		if (ret == -1)return -2;
		pid_t pid = fork();
		if (pid == -1)return -3;
		if (pid == 0) {
			//子进程
			close(pipes[1]);//关闭掉写
			pipes[1] = 0;
			ret = (*m_func)();
			exit(0);
		}
		//主进程
		close(pipes[0]);
		pipes[0] = 0;
		m_pid = pid;
		return 0;
	}

	int SendFD(int fd) {//主进程完成
		//struct msghdr msg;
		//iovec iov[2];
		//char buf[2][10] = { "edoyun","jueding" };
		//iov[0].iov_base = buf[0];
		//iov[0].iov_len = sizeof(buf[0]);
		//iov[1].iov_base = buf[1];
		//iov[1].iov_len = sizeof(buf[1]);
		//msg.msg_iov = iov;
		//msg.msg_iovlen = 2;

		//// 下面的数据，才是我们需要传递的。
		//cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));
		//if (cmsg == NULL)return -1;
		//cmsg->cmsg_len = CMSG_LEN(sizeof(int));
		//cmsg->cmsg_level = SOL_SOCKET;
		//cmsg->cmsg_type = SCM_RIGHTS;
		//*(int*)CMSG_DATA(cmsg) = fd;
		//msg.msg_control = cmsg;
		//msg.msg_controllen = cmsg->cmsg_len;
		struct iovec iov[1];
		iov[0].iov_base = &fd;
		iov[0].iov_len = sizeof(int);

		// Set up the msghdr structure.
		struct msghdr message_header {};
		message_header.msg_iov = iov;
		message_header.msg_iovlen = 1;

		// Send the message using sendmsg.
		ssize_t ret = sendmsg(pipes[1], &message_header, 0);
		close(pipes[1]);
		if (ret == -1) {
			return -2;
		}
		return 0;
	}

	int RecvFD(int& fd)
	{

		/*
		void *msg_name;		 Address to send to/receive from.  
		socklen_t msg_namelen;	 Length of address data.  

		struct iovec* msg_iov;	 Vector of data to send/receive into.  
		size_t msg_iovlen;		 Number of elements in the vector.  

		void* msg_control;		 Ancillary data (eg BSD filedesc passing). 
		size_t msg_controllen;	Ancillary data buffer length.
					   !! The type should be socklen_t but the
					   definition of the kernel is incompatible
					   with this.  

		int msg_flags;		Flags on received message.  */

		char buffer[256];
		struct iovec iov[1];
		iov[0].iov_base = buffer;
		iov[0].iov_len = sizeof(buffer);

		// Set up the msghdr structure.
		struct msghdr message_header {};
		message_header.msg_iov = iov;
		message_header.msg_iovlen = 1;

		//cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));
		//if (cmsg == NULL)return -1;
		//cmsg->cmsg_len = CMSG_LEN(sizeof(int));
		//cmsg->cmsg_level = SOL_SOCKET;
		//cmsg->cmsg_type = SCM_RIGHTS;
		//msg.msg_control = cmsg;
		//msg.msg_controllen = CMSG_LEN(sizeof(int));

		ssize_t ret = recvmsg(pipes[0], &message_header, 0);
		if (ret == -1) {
			return -2;
		}
		fd = *((int*)message_header.msg_iov[0].iov_base);
		printf("%s(%d):<%s> fd=%d\n", __FILE__, __LINE__, __FUNCTION__, fd);
		close(pipes[0]);
		return 0;
	}


private:
	CFunctionBase* m_func;
	pid_t m_pid;
	int pipes[2];
};


int CreateLogServer(CProcess* proc)
{
	printf("%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
	return 0;
}

int CreateClientServer(CProcess* proc)
{
	printf("%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
	int fd = -1;
	int ret = proc->RecvFD(fd);
	printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
	printf("%s(%d):<%s> fd=%d\n", __FILE__, __LINE__, __FUNCTION__, fd);
	sleep(1);
	char buf[10] = {};
	lseek(fd, 0, SEEK_SET);
	printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
	ret = read(fd, buf, sizeof(buf));
	printf("errno:%d read:%s\n", errno, strerror(errno));
	printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
	printf("%s(%d):<%s> buf=%s\n", __FILE__, __LINE__, __FUNCTION__, buf);
	close(fd);
	return 0;
}

int main()
{
	CProcess proclog, procclients;
	printf("%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
	proclog.SetEntryFunction(CreateLogServer, &proclog);
	int ret = proclog.CreateSubProcess();
	if (ret != 0) {
		printf("%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
		return -1;
	}
	printf("%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
	procclients.SetEntryFunction(CreateClientServer, &procclients);
	ret = procclients.CreateSubProcess();
	if (ret != 0) {
		printf("%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
		return -2;
	}
	printf("%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
	usleep(100 * 000);
	int fd = open("./test.txt", O_RDWR | O_CREAT | O_APPEND);
	printf("%s(%d):<%s> fd=%d\n", __FILE__, __LINE__, __FUNCTION__, fd);
	if (fd == -1)return -3;
	ret = procclients.SendFD(fd);
	printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
	if (ret != 0)printf("errno:%d msg:%s\n", errno, strerror(errno));
	write(fd, "edoyun", 6);
	close(fd);
	return 0;
}