#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#ifdef _WIN32

	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS 
	#include <Windows.h>
	#include <WinSock2.h>	//��ͷ�ļ�Ӧ�÷���Windows.hǰ�棬���߼�һ���궨��
	#pragma comment(lib,"ws2_32.lib")	//������صĿ�

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

//����TCP�����
//1 ����һ��socket
//2 �󶨽��ܿͻ������ӵĶ˿� bind
//3 ��������˿� listen
//4 �ȴ����տͻ�������accept
//5 ��ͻ��˷���һ������ send
//6 �ر�socket closesocket
//����TCP�ͻ���
//1 ����һ�� socket
//2 ���ӷ����� connect
//3 ���ܷ�������Ϣ recv
//4 �ر�socket closesocket

class EasyTcpClient
{
private:
	SOCKET _sock;

public:
	EasyTcpClient(){
		_sock = INVALID_SOCKET;		//��ʼʱ��ʼ��Ϊ��Ч��sock
		InitOthers();
	}

	virtual ~EasyTcpClient() {
		Close();
	}

	//��ʼ��socket
	void InitSocket() {
		//����Win Sock 2.x����
		if (_sock != INVALID_SOCKET) {
			printf("<socket=%d> �ر�֮ǰ������!\n",_sock);
			Close();
		}
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (INVALID_SOCKET == _sock) {
			printf("����INVALID_SOCKET == _sock��\n");
		}
		else {
			printf("����_sock�ɹ���\n");
		}
	}

	//���ӷ�����
	int Connect(const char* ip,unsigned short port) {
		if (INVALID_SOCKET == _sock) {
			InitSocket();
		}
		sockaddr_in _sin = {};	//�յĽṹ��
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret) {
			printf("�������ӷ�����ʧ�ܣ�\n");
		}
		else {
			printf("���ӷ������ɹ�<socket=%d>��\n",_sock);
		}
		return ret;
	}

	//�ر�socket
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

	//��ѯ������Ϣ
	bool OnRun() {
		if (isRun()) {
			fd_set fdReads;
			FD_ZERO(&fdReads);
			FD_SET(_sock, &fdReads);
			timeval t = { 1,0 };
			int ret = select(_sock + 1, &fdReads, 0, 0, &t);
			if (ret < 0) {
				Close();
				printf("<socket=%d> select���������ret < 0!\n", _sock);
				return false;
			}
			if (FD_ISSET(_sock, &fdReads)) {
				FD_CLR(_sock, &fdReads);
				if (-1 == RecvData(_sock)) {
					Close();		//�Լ���ӵ�
					printf("<socket=%d> select���������-1 == RecvData(_sock)!\n", _sock);
					return false;
				}
			}
			return true;
		}
		return false;
	}
	
	//�Ƿ��ڹ�����
	bool isRun() {
		return _sock != INVALID_SOCKET;
	}
#ifndef RECV_BUFF_SIZE
	//��������С��Ԫ��С
#define RECV_BUFF_SIZE 10240
#endif
	//���ջ�����
	char _szRecv[RECV_BUFF_SIZE] = {};
	//�ڶ������� ��Ϣ������
	char _szMsgBuf[RECV_BUFF_SIZE * 10] = {};
	//��Ϣ��������β��λ��
	int _lastPos = 0;

	void InitOthers() {
		//���ջ�����
		_szRecv[RECV_BUFF_SIZE] = {};
		//�ڶ������� ��Ϣ������
		_szMsgBuf[RECV_BUFF_SIZE * 10] = {};
		_lastPos = 0;
	}

	//�������� ����ճ�� ��ְ�
	int RecvData(SOCKET cSock) {
		//���յ��ͻ��˷���������
		int nLen = (int)recv(cSock, _szRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0) {
			printf("<socket=%d>��������Ͽ����ӣ����������\n", cSock);
			return -1;
		}
		//����ȡ�������ݿ�������Ϣ������
		memcpy(_szMsgBuf + _lastPos, _szRecv, nLen);
		//��Ϣ������������β��λ�ú���
		_lastPos += nLen;
		//�ж���Ϣ�����������ݳ��ȴ�����Ϣͷ
		while (_lastPos >= sizeof(DataHeader)) {
			//��ʱ�Ϳ���֪����ǰ��Ϣ�ĳ���
			DataHeader* header = (DataHeader*)_szMsgBuf;
			//�ж���Ϣ�������ĳ��ȴ�����Ϣ�ĳ���
			if (_lastPos > header->dataLength) {
				//��Ϣ������ʣ��δ����ĳ���
				int nSize = _lastPos - header->dataLength;
				//����������Ϣ
				OnNetMsg(header);
				//����Ϣ������ʣ��δ��������ǰ��
				memcpy(_szMsgBuf, _szMsgBuf + header->dataLength, nSize);
				//��Ϣ������������β��ǰ��
				_lastPos = nSize;
			}
			else {
				//��Ϣ��������ʣ����Ϣ���㣬����һ����������Ϣ
				break;
			}
		}

		return 0;
	}

	////�������� Ҫ����ճ����
	//int RecvData(SOCKET _cSock) {
	//	//ʹ��һ����������������
	//	char szRecv[4096] = {};

	//	//#5 ���տͻ�������
	//	int nlen = recv(_cSock, szRecv, sizeof(DataHeader), 0);	//����������ͻ��˵�socket
	//	DataHeader* header = (DataHeader*)szRecv;
	//	if (nlen <= 0) {
	//		printf("��������Ͽ�����<Socket=%d>�����������\n", _cSock);
	//		return -1;
	//	}
	//	recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
	//	OnNetMsg(header);
	//	return 0;
	//}

	//��Ӧ������Ϣ
	virtual void OnNetMsg(DataHeader* header) {
		//#6 ��������
		switch (header->cmd) {
			case CMD_LOGIN_RESULT:
			{
				LoginResult* loginr = (LoginResult*)header;
				//printf("�յ����������ݣ�CMD_LOGIN_RESULT�����ݳ��� = %d\n", loginr->dataLength);
			}
			break;
			case CMD_LOGOUT_RESULT:
			{
				LogoutResult* logoutr = (LogoutResult*)header;
				//printf("�յ����������ݣ�CMD_LOGOUT_RESULT�����ݳ��� = %d\n", logoutr->dataLength);
			}
			break;
			case CMD_NEW_USER_JOIN:
			{
				NewUserJoin* userjoin = (NewUserJoin*)header;
				//printf("�յ����������ݣ�CMD_NEW_USER_JOIN���¼���ͻ���<%d>�����ݳ��� = %d\n", userjoin->sock, userjoin->dataLength);
			}
			break;
			case CMD_ERROR:
			{
				printf("<socket=%d>�յ����������ݣ�CMD_ERROR�����ݳ��� = %d\n", _sock, header->dataLength);
			}
			break;
			default: 
			{
				printf("<socket=%d>�յ����������ݣ�δ������Ϣ�����ݳ��� = %d\n", _sock, header->dataLength);
			}
			break;
		}
	}


	//��������
	int SendData(DataHeader* header) {
		if (isRun() && header) {
			return send(_sock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

private:


};


#endif