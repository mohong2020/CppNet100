#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
	#include <WinSock2.h>	//该头文件应该放在Windows.h前面，或者加一个宏定义
	#pragma comment(lib,"ws2_32.lib")	//引用相关的库
#else
	#include <unistd.h>	//uni std
	#include <arpa/inet.h>
	#include <string.h>
	#define SOCKET int
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR	       (-1)
#endif

#include <vector>
#include <iostream>
#include "MessageHeader.hpp"

#ifndef RECV_BUFF_SIZE
//缓冲区最小单元大小
#define RECV_BUFF_SIZE 10240
#endif

class ClientSocket {
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET) {
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		//消息缓冲区的数据尾部位置
		_lastPos = 0;
	}

	SOCKET sockfd() {
		return _sockfd;
	}

	char* msgBuf() {
		return _szMsgBuf;
	}

	int getLastPos() {
		return _lastPos;
	}
	
	void setLastPos(int pos) {
		_lastPos = pos;
	}
private:
	SOCKET _sockfd;		//socket fd_set 
	//第二缓冲区，消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 10];
	//消息缓冲区的数据尾部位置
	int _lastPos = 0;
};

class EasyTcpServer {

private:
	SOCKET _sock;
	std::vector<ClientSocket*> _clients;

public:
	EasyTcpServer() {
		_sock = INVALID_SOCKET;
	}

	virtual ~EasyTcpServer() {
		Close();
	}

	//初始化socket
	SOCKET InitSocket() {
#ifdef _WIN32
		//启动Windows socket 2.x环境
		WORD ver MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif 
		if (INVALID_SOCKET != _sock) {
			printf("<socket=%d>关闭旧连接 ...\n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);		//创建一个套接字
		if (INVALID_SOCKET == _sock) {
			printf("错误，创建套接字失败  ...\n");
		}
		else {
			printf("建立<socket=%d>成功  ...\n", _sock);
		}
		return _sock;
	}
	                          
	//绑定IP和端口号
	int Bind(const char* ip,unsigned short port) {
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(4567);

#ifdef _WIN32
		if (ip) {
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else
		if (ip) {
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		if (SOCKET_ERROR == ret) {
			printf("错误，<socket=%d>绑定网络端口号失败 ...\n",_sock);
		}
		else {
			printf("<socket=%d>绑定网络端口号成功 ...\n",_sock);
		}
		return ret;
	}

	//监听端口号，参数等待连接数
	int Listen(int n) {
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret) {
			printf("错误，服务器<socket=%d>，监听网络端口失败 ...\n",_sock);
		}
		else {
			printf("监听网络端口号成功,服务器<socket=%d> ...\n",_sock);
		}
		return ret;
	}

	//接收客户端连接
	SOCKET Accept() {
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
		cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
		if (INVALID_SOCKET == cSock) {
			printf("错误，接收到无效客户端INVALID_SOCKET == cSock ...\n");
		}
		else {
			//新登录的客户端消息发送给其他客户端
			NewUserJoin userJoin;
			SendDataToAll(&userJoin);
			_clients.push_back(new ClientSocket(cSock));
			printf("有新的客户端加入: socket = %d, IP = %s \n", (int)cSock, inet_ntoa(clientAddr.sin_addr));
		}
		return cSock;
	}

	//关闭socket
	void Close() {
		if (_sock != INVALID_SOCKET) {
#ifdef _WIN32
			for (int n = (int)_clients.size() - 1; n >= 0; n--) {
				closesocket(_clients[n]->sockfd());
				delete _clients[n];
			}
			//关闭套接字
			closesocket(_sock);
			WSACleanup();
#else
			for (int n = (int)_clients.size() - 1; n >= 0; n--) {
				close(_clients[n]->sockfd());
				delete _clients[n];
			}
			close(_sock);
#endif
			_clients.clear();
		}
	}

	//处理网络消息
	bool OnRun() {
		if (isRun()) {
			//伯克利套接字 BSD socket
			//各种描述符的计合
			fd_set fdRead;			//可读
			fd_set fdWrite;			//可写
			fd_set fdExp;			//异常
			//清理集合，置零操作
			FD_ZERO(&fdRead);
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExp);
			//将描述符(socket)加入集合，（此时加入的是服务器的套接字）
			FD_SET(_sock, &fdRead);
			FD_SET(_sock, &fdWrite);
			FD_SET(_sock, &fdExp);
			SOCKET maxSock = _sock;
			//将所有与客户端连接的套接字加入集合
			for (int n = (int)_clients.size() - 1; n >= 0; n--) {
				FD_SET(_clients[n]->sockfd(), &fdRead);
				if (maxSock < _clients[n]->sockfd()) {
					maxSock = _clients[n]->sockfd();
				}
			}
			timeval t = { 1,0 };	//等待超时时间，隔这么多时间就返回一次
			int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
			//当select有事件返回时，如果待检测的fd上真的有事件发生，则系统会保留arr[fd]=1的值，
			//若没有事件发生，则arr[fd]会被置为0
			if (ret < 0) {	//发生了错误
				printf("ret < 0 select任务结束...\n");
				Close();
				return false;
			}
			//判断描述符(socket)是否在集合中，然后要对_sock进行操作（
			if (FD_ISSET(_sock,&fdRead)) {
				FD_CLR(_sock, &fdRead);
				Accept();
			}
			//查看与客户端连接的哪个套接字上有可读事件
			for (int n = _clients.size() - 1; n >= 0; n--) {
				if (FD_ISSET(_clients[n]->sockfd(), &fdRead)) {
					int rRecvData = RecvData(_clients[n]);		//进行处理
					if (-1 == rRecvData) {
						//进行事件处理时发生了错误
						auto iter = _clients.begin() + n;		//获得当前套接字所对应的迭代器
						if (iter != _clients.end()) {
							delete _clients[n];
							_clients.erase(iter);
						}
					}
				}
			}
			return true;
		}
		return false;
	}

	//是否工作中
	bool isRun() {
		return _sock != INVALID_SOCKET;
	}

	//缓冲区
	char _szRecv[RECV_BUFF_SIZE] = {};
	//接收数据 处理粘包 拆分包
	int RecvData(ClientSocket* pClient) {
		//接收数据的缓冲区
		//接收到客户端发来的数据
		int nLen = (int)recv(pClient->sockfd(), _szRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0) {
			printf("从与客户端关联的<_cSock=%d>接收数据nLen <= 0，任务结束 ...\n", pClient->sockfd());
			return -1;
		}
		//将收到的数据拷贝到消息缓冲区
		memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		pClient->setLastPos(pClient->getLastPos() + nLen);
		
		//判断消息缓冲区的数据长度是否大于消息头DataHeader长度
		while (pClient->getLastPos() >= sizeof(DataHeader)) {
			DataHeader* header = (DataHeader*)pClient->msgBuf();
			//判断消息缓冲区的数据长度大于消息体的长度
			if (pClient->getLastPos() >= header->dataLength) {
				//消息缓冲区剩余未处理的消息长度
				int nSize = pClient->getLastPos() - header->dataLength;
				//处理网络消息
				OnNetMsg(pClient->sockfd(),header);
				//将消息缓冲区中未处理的数据前移
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				//消息缓冲区的数据尾部位置前移
				pClient->setLastPos(nSize);
			}
			else {
				//消息缓冲区剩余的数据不够一条完整消息
				break;
			}
		}

		return 0;
	}

	//响应网络消息
	virtual void OnNetMsg(SOCKET cSock,DataHeader* header) {
		//处理请求，根据命令，继续接收没有接收完的部分
		switch (header->cmd) {
			case CMD_LOGIN:
			{
			
				Login* login = (Login*)header;
				//printf("<_cSock=%d> 收到命令 = CMD_LOGIN，数据长度 = %d，uesrName = %s，用户密码 = %s\n",
				//	cSock, login->dataLength, login->userName, login->passWord);
				//判断账号密码是否正确
				//...
				//若登录成功
				LoginResult ret = {};
				SendData(cSock, &ret);
				//send(_cSock, (char*)&logret, sizeof(LoginResult), 0);
			}
			break;
			case CMD_LOGOUT:
			{
				Logout* logout = (Logout*)header;
	/*			printf("<_cSock=%d> 收到命令 = CMD_LOGOUT，数据长度 = %d，userName = %s\n",
					cSock, logout->dataLength, logout->userName);*/
				//客户端的登出操作
				//...
				LogoutResult ret = {};
				SendData(cSock, &ret);
				//send(_cSock, (char*)&ret, sizeof(LogoutResult), 0);
			}
			break;
			default:
			{
				printf("<cSock=%d>收到未定义消息，数据长度:%d\n", cSock, header->dataLength);
				//发生了错误
				//DataHeader header = {};
				//header.dataLength = 0;
				//header.cmd = CMD_ERROR;
				//SendData(cSock, &header);
			}
			break;

		}
	}

	//发送指定_cSock数据
	int SendData(SOCKET cSock, DataHeader* header) {
		if (isRun() && header) {
			return send(cSock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

	//群发消息，发送给所有客户端消息
	void SendDataToAll(DataHeader* header) {
		if (isRun() && header) {
			for (int n = (int)_clients.size() - 1; n >= 0; n--) {
				SendData(_clients[n]->sockfd(), header);
			}
		}
	}


};

#endif