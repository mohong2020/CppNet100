#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#ifdef _WIN32

	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS 
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

#include <iostream>

#include "MessageHeader.hpp"

//建立TCP服务端
//1 建立一个socket
//2 绑定接受客户端连接的端口 bind
//3 监听网络端口 listen
//4 等待接收客户端连接accept
//5 向客户端发送一条数据 send
//6 关闭socket closesocket
//建立TCP客户端
//1 建立一个 socket
//2 连接服务器 connect
//3 接受服务器信息 recv
//4 关闭socket closesocket

class EasyTcpClient
{
private:
	SOCKET _sock;

public:
	EasyTcpClient(){
		_sock = INVALID_SOCKET;		//开始时初始化为无效的sock
		InitOthers();
	}

	virtual ~EasyTcpClient() {
		Close();
	}

	//初始化socket
	void InitSocket() {
		//启动Win Sock 2.x环境
		if (_sock != INVALID_SOCKET) {
			printf("<socket=%d> 关闭之前的连接!\n",_sock);
			Close();
		}
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (INVALID_SOCKET == _sock) {
			printf("错误，INVALID_SOCKET == _sock！\n");
		}
		else {
			printf("建立_sock成功！\n");
		}
	}

	//连接服务器
	int Connect(const char* ip,unsigned short port) {
		if (INVALID_SOCKET == _sock) {
			InitSocket();
		}
		sockaddr_in _sin = {};	//空的结构体
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret) {
			printf("错误，连接服务器失败！\n");
		}
		else {
			printf("连接服务器成功<socket=%d>！\n",_sock);
		}
		return ret;
	}

	//关闭socket
	void Close() {
		if (_sock != INVALID_SOCKET) {
#ifdef _WIN32
			closesocket(_sock);
			WSACleanup();
#else
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
		}
	}

	//查询网络消息
	bool OnRun() {
		if (isRun()) {
			fd_set fdReads;
			FD_ZERO(&fdReads);
			FD_SET(_sock, &fdReads);
			timeval t = { 1,0 };
			int ret = select(_sock + 1, &fdReads, 0, 0, &t);
			if (ret < 0) {
				Close();
				printf("<socket=%d> select任务结束，ret < 0!\n", _sock);
				return false;
			}
			if (FD_ISSET(_sock, &fdReads)) {
				FD_CLR(_sock, &fdReads);
				if (-1 == RecvData(_sock)) {
					Close();		//自己添加的
					printf("<socket=%d> select任务结束，-1 == RecvData(_sock)!\n", _sock);
					return false;
				}
			}
			return true;
		}
		return false;
	}
	
	//是否在工作中
	bool isRun() {
		return _sock != INVALID_SOCKET;
	}
#ifndef RECV_BUFF_SIZE
	//缓冲区最小单元大小
#define RECV_BUFF_SIZE 10240
#endif
	//接收缓冲区
	char _szRecv[RECV_BUFF_SIZE] = {};
	//第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 10] = {};
	//消息缓冲区的尾部位置
	int _lastPos = 0;

	void InitOthers() {
		//接收缓冲区
		_szRecv[RECV_BUFF_SIZE] = {};
		//第二缓冲区 消息缓冲区
		_szMsgBuf[RECV_BUFF_SIZE * 10] = {};
		_lastPos = 0;
	}

	//接收数据 处理粘包 拆分包
	int RecvData(SOCKET cSock) {
		//接收到客户端发来的数据
		int nLen = (int)recv(cSock, _szRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0) {
			printf("<socket=%d>与服务器断开连接，任务结束。\n", cSock);
			return -1;
		}
		//将收取到的数据拷贝到消息缓冲区
		memcpy(_szMsgBuf + _lastPos, _szRecv, nLen);
		//消息缓冲区的数据尾部位置后移
		_lastPos += nLen;
		//判断消息缓冲区的数据长度大于消息头
		while (_lastPos >= sizeof(DataHeader)) {
			//这时就可以知道当前消息的长度
			DataHeader* header = (DataHeader*)_szMsgBuf;
			//判断消息缓冲区的长度大于消息的长度
			if (_lastPos > header->dataLength) {
				//消息缓冲区剩余未处理的长度
				int nSize = _lastPos - header->dataLength;
				//处理网络消息
				OnNetMsg(header);
				//将消息缓冲区剩余未处理数据前移
				memcpy(_szMsgBuf, _szMsgBuf + header->dataLength, nSize);
				//消息缓冲区的数据尾部前移
				_lastPos = nSize;
			}
			else {
				//消息缓冲区的剩余消息不足，不够一个完整的消息
				break;
			}
		}

		return 0;
	}

	////接收数据 要处理粘包，
	//int RecvData(SOCKET _cSock) {
	//	//使用一个缓冲来接受数据
	//	char szRecv[4096] = {};

	//	//#5 接收客户端数据
	//	int nlen = recv(_cSock, szRecv, sizeof(DataHeader), 0);	//必须是连入客户端的socket
	//	DataHeader* header = (DataHeader*)szRecv;
	//	if (nlen <= 0) {
	//		printf("与服务器断开连接<Socket=%d>，任务结束。\n", _cSock);
	//		return -1;
	//	}
	//	recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
	//	OnNetMsg(header);
	//	return 0;
	//}

	//相应网络消息
	virtual void OnNetMsg(DataHeader* header) {
		//#6 处理请求
		switch (header->cmd) {
			case CMD_LOGIN_RESULT:
			{
				LoginResult* loginr = (LoginResult*)header;
				//printf("收到服务器数据：CMD_LOGIN_RESULT，数据长度 = %d\n", loginr->dataLength);
			}
			break;
			case CMD_LOGOUT_RESULT:
			{
				LogoutResult* logoutr = (LogoutResult*)header;
				//printf("收到服务器数据：CMD_LOGOUT_RESULT，数据长度 = %d\n", logoutr->dataLength);
			}
			break;
			case CMD_NEW_USER_JOIN:
			{
				NewUserJoin* userjoin = (NewUserJoin*)header;
				//printf("收到服务器数据：CMD_NEW_USER_JOIN，新加入客户端<%d>，数据长度 = %d\n", userjoin->sock, userjoin->dataLength);
			}
			break;
			case CMD_ERROR:
			{
				printf("<socket=%d>收到服务器数据：CMD_ERROR，数据长度 = %d\n", _sock, header->dataLength);
			}
			break;
			default: 
			{
				printf("<socket=%d>收到服务器数据：未定义消息，数据长度 = %d\n", _sock, header->dataLength);
			}
			break;
		}
	}


	//发送数据
	int SendData(DataHeader* header) {
		if (isRun() && header) {
			return send(_sock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

private:


};


#endif