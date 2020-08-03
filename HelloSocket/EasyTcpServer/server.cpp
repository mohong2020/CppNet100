
#ifdef _WIN32
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

#include <string>
#include <vector>
#include <iostream>

#include "EasyTcpServer.hpp"

std::vector<SOCKET> g_clients;

void test1()
{
	EasyTcpServer server;
	server.InitSocket();
	server.Bind(nullptr, 4567);
	server.Listen(5);
	
	while (server.isRun()) {
		server.OnRun();
	}
	server.Close();

}	


int main()
{
	test1();
	system("pause");
	return 0;
}
