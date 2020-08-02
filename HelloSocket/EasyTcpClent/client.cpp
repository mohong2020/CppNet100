#define _CRT_SECURE_NO_WARNINGS
#include <thread>
#include "EasyTcpClient.hpp"

//控制台输入命令线程函数
void cmdThread(EasyTcpClient* client) {
	while (true) {
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			client->Close();
			printf("客户端exit\n");
			//closesocket(_sock);
			return;
		}
		else if (0 == strcmp(cmdBuf, "login")) {
			Login login;
			strcpy(login.userName, "lyd");
			strcpy(login.passWord, "lydpw");
			client->SendData(&login);
		}
		else if (0 == strcmp(cmdBuf, "logout")) {
			Logout logout;
			strcpy(logout.userName, "lyd");
			client->SendData(&logout);
		}
		else {
			printf("不支持该命令\n");
		}
	}
}

void test1() {
	EasyTcpClient client;
	client.InitSocket();
	/*client.Connect("127.0.0.1", 4567);*/
	client.Connect("192.168.16.134", 4567);
	std::thread t1(cmdThread, &client);		//该线程主要用来发送数据
	t1.detach();
	Login login;
	strcpy(login.userName, "lyd");
	strcpy(login.passWord, "lydmm");
	while (client.isRun()) {		// 主要用于接收消息 
		client.OnRun();
		client.SendData(&login);
	}

	client.Close();
	printf("已退出！\n");
	getchar();
}

bool g_run = true;
void cmdThread2() {
	while (true) {
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			printf("客户端exit\n");
			//closesocket(_sock);
			g_run = false;
			return;
		}
		else {
			printf("不支持该命令\n");
		}
	}
}

void test2() {
	const int cCount = 1000;
	EasyTcpClient* client[cCount];
	for (int n = 0; n < cCount; n++) {
		client[n] = new EasyTcpClient();
		//client[n]->Connect("127.0.0.1", 4567);
		client[n]->Connect("192.168.16.134", 4567);
	}

	std::thread t1(cmdThread2);
	t1.detach();

	Login login;
	strcpy(login.userName, "lyd");
	strcpy(login.passWord, "lydmm");
	while (g_run) {
		for (int n = 0; n < cCount; n++) {
			client[n]->OnRun();
			client[n]->SendData(&login);
		}
	}
	for (int n = 0; n < cCount; n++) {
		client[n]->Close();
	}

}

int main()
{
	//test1();
	test2();
	system("pause");
	return 0;
}
