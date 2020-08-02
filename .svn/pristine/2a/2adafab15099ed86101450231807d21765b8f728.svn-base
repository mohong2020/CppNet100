#include <iostream>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>	//该头文件应该放在Windows.h前面，或者加一个宏定义

#pragma comment(lib,"ws2_32.lib")	//引用相关的库

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


void test1()
{
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);		//开始启动




	WSACleanup();		//进行关闭
}


int main()
{
	std::cout << "hello c++" << std::endl;
	system("pause");
	return 0;
}
