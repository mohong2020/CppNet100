#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#ifdef _WIN32
	//#define FD_SETSIZE 1024
	#define _WINSOCK_DEPRECATED_NO_WARNINGS 
	#define WIN32_LEAN_AND_MEAN
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

#include <vector>
#include <iostream>
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"

#ifndef RECV_BUFF_SIZE
//��������С��Ԫ��С
#define RECV_BUFF_SIZE 10240
#endif

//�ͻ���
class ClientSocket {
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET) {
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		//��Ϣ������������β��λ��
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
	//�ڶ�����������Ϣ������
	char _szMsgBuf[RECV_BUFF_SIZE * 10];
	//��Ϣ������������β��λ��
	int _lastPos = 0;
};

class EasyTcpServer {

private:
	SOCKET _sock;
	std::vector<ClientSocket*> _clients;
	CELLTimestamp _tTime;	//��ʱ
	int _recvCount;			//����
public:
	EasyTcpServer() {
		_sock = INVALID_SOCKET;
		_recvCount = 0;
	}

	virtual ~EasyTcpServer() {
		Close();
	}

	//��ʼ��socket
	SOCKET InitSocket() {
#ifdef _WIN32
		//����Windows socket 2.x����
		WORD ver MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif 
		if (INVALID_SOCKET != _sock) {
			printf("<socket=%d>�رվ����� ...\n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);		//����һ���׽���
		if (INVALID_SOCKET == _sock) {
			printf("���󣬴����׽���ʧ��  ...\n");
		}
		else {
			printf("����<socket=%d>�ɹ�  ...\n", _sock);
		}
		return _sock;
	}
	                          
	//��IP�Ͷ˿ں�
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
			printf("����<socket=%d>������˿ں�ʧ�� ...\n",_sock);
		}
		else {
			printf("<socket=%d>������˿ںųɹ� ...\n",_sock);
		}
		return ret;
	}

	//�����˿ںţ������ȴ�������
	int Listen(int n) {
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret) {
			printf("���󣬷�����<socket=%d>����������˿�ʧ�� ...\n",_sock);
		}
		else {
			printf("��������˿ںųɹ�,������<socket=%d> ...\n",_sock);
		}
		return ret;
	}

	//���տͻ�������
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
			printf("���󣬽��յ���Ч�ͻ���INVALID_SOCKET == cSock ...\n");
		}
		else {
			//�µ�¼�Ŀͻ�����Ϣ���͸������ͻ���
			NewUserJoin userJoin;
			SendDataToAll(&userJoin);
			_clients.push_back(new ClientSocket(cSock));
			printf("���µĿͻ��˼���: socket = %d, IP = %s \n", (int)cSock, inet_ntoa(clientAddr.sin_addr));
		}
		return cSock;
	}

	//�ر�socket
	void Close() {
		if (_sock != INVALID_SOCKET) {
#ifdef _WIN32
			for (int n = (int)_clients.size() - 1; n >= 0; n--) {
				closesocket(_clients[n]->sockfd());
				delete _clients[n];
			}
			//�ر��׽���
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

	//����������Ϣ
	bool OnRun() {
		if (isRun()) {
			//�������׽��� BSD socket
			//�����������ļƺ�
			fd_set fdRead;			//�ɶ�
			fd_set fdWrite;			//��д
			fd_set fdExp;			//�쳣
			//�����ϣ��������
			FD_ZERO(&fdRead);
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExp);
			//��������(socket)���뼯�ϣ�����ʱ������Ƿ��������׽��֣�
			FD_SET(_sock, &fdRead);
			FD_SET(_sock, &fdWrite);
			FD_SET(_sock, &fdExp);
			SOCKET maxSock = _sock;
			//��������ͻ������ӵ��׽��ּ��뼯��
			for (int n = (int)_clients.size() - 1; n >= 0; n--) {
				FD_SET(_clients[n]->sockfd(), &fdRead);
				if (maxSock < _clients[n]->sockfd()) {
					maxSock = _clients[n]->sockfd();
				}
			}
			timeval t = { 1,0 };	//�ȴ���ʱʱ�䣬����ô��ʱ��ͷ���һ��
			int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
			//��select���¼�����ʱ�����������fd��������¼���������ϵͳ�ᱣ��arr[fd]=1��ֵ��
			//��û���¼���������arr[fd]�ᱻ��Ϊ0
			if (ret < 0) {	//�����˴���
				printf("ret < 0 select�������...\n");
				Close();
				return false;
			}
			//�ж�������(socket)�Ƿ��ڼ����У�Ȼ��Ҫ��_sock���в�����
			if (FD_ISSET(_sock,&fdRead)) {
				FD_CLR(_sock, &fdRead);
				Accept();
			}
			//�鿴��ͻ������ӵ��ĸ��׽������пɶ��¼�
			for (int n = _clients.size() - 1; n >= 0; n--) {
				if (FD_ISSET(_clients[n]->sockfd(), &fdRead)) {
					int rRecvData = RecvData(_clients[n]);		//���д���
					if (-1 == rRecvData) {
						//�����¼�����ʱ�����˴���
						auto iter = _clients.begin() + n;		//��õ�ǰ�׽�������Ӧ�ĵ�����
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

	//�Ƿ�����
	bool isRun() {
		return _sock != INVALID_SOCKET;
	}

	//������
	char _szRecv[RECV_BUFF_SIZE] = {};
	//�������� ����ճ�� ��ְ�
	int RecvData(ClientSocket* pClient) {
		//�������ݵĻ�����
		//���յ��ͻ��˷���������
		int nLen = (int)recv(pClient->sockfd(), _szRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0) {
			printf("����ͻ��˹�����<_cSock=%d>��������nLen <= 0��������� ...\n", pClient->sockfd());
			return -1;
		}
		//���յ������ݿ�������Ϣ������
		memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		pClient->setLastPos(pClient->getLastPos() + nLen);
		
		//�ж���Ϣ�����������ݳ����Ƿ������ϢͷDataHeader����
		while (pClient->getLastPos() >= sizeof(DataHeader)) {
			DataHeader* header = (DataHeader*)pClient->msgBuf();
			//�ж���Ϣ�����������ݳ��ȴ�����Ϣ��ĳ���
			if (pClient->getLastPos() >= header->dataLength) {
				//��Ϣ������ʣ��δ�������Ϣ����
				int nSize = pClient->getLastPos() - header->dataLength;
				//����������Ϣ
				OnNetMsg(pClient->sockfd(),header);
				//����Ϣ��������δ���������ǰ��
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				//��Ϣ������������β��λ��ǰ��
				pClient->setLastPos(nSize);
			}
			else {
				//��Ϣ������ʣ������ݲ���һ��������Ϣ
				break;
			}
		}

		return 0;
	}

	//��Ӧ������Ϣ
	virtual void OnNetMsg(SOCKET cSock,DataHeader* header) {
		//�������󣬸��������������û�н�����Ĳ���
		_recvCount++;
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0) {
			printf("time<%lf>,socket<%d>,clients<%d>,recvCount<%d>\n", t1, _sock, _clients.size(),_recvCount);
			_recvCount = 0;
			_tTime.update();
		}
		switch (header->cmd) {
			case CMD_LOGIN:
			{
			
				Login* login = (Login*)header;
				//printf("<_cSock=%d> �յ����� = CMD_LOGIN�����ݳ��� = %d��uesrName = %s���û����� = %s\n",
				//	cSock, login->dataLength, login->userName, login->passWord);
				//�ж��˺������Ƿ���ȷ
				//...
				//����¼�ɹ�
				LoginResult ret = {};
				SendData(cSock, &ret);
				//send(_cSock, (char*)&logret, sizeof(LoginResult), 0);
			}
			break;
			case CMD_LOGOUT:
			{
				Logout* logout = (Logout*)header;
	/*			printf("<_cSock=%d> �յ����� = CMD_LOGOUT�����ݳ��� = %d��userName = %s\n",
					cSock, logout->dataLength, logout->userName);*/
				//�ͻ��˵ĵǳ�����
				//...
				LogoutResult ret = {};
				SendData(cSock, &ret);
				//send(_cSock, (char*)&ret, sizeof(LogoutResult), 0);
			}
			break;
			default:
			{
				printf("<cSock=%d>�յ�δ������Ϣ�����ݳ���:%d\n", cSock, header->dataLength);
				//�����˴���
				//DataHeader header = {};
				//header.dataLength = 0;
				//header.cmd = CMD_ERROR;
				//SendData(cSock, &header);
			}
			break;

		}
	}

	//����ָ��_cSock����
	int SendData(SOCKET cSock, DataHeader* header) {
		if (isRun() && header) {
			return send(cSock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

	//Ⱥ����Ϣ�����͸����пͻ�����Ϣ
	void SendDataToAll(DataHeader* header) {
		if (isRun() && header) {
			for (int n = (int)_clients.size() - 1; n >= 0; n--) {
				SendData(_clients[n]->sockfd(), header);
			}
		}
	}


};

#endif