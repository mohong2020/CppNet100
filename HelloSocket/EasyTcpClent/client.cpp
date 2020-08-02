#define _CRT_SECURE_NO_WARNINGS
#include <thread>
#include "EasyTcpClient.hpp"

//����̨���������̺߳���
void cmdThread(EasyTcpClient* client) {
	while (true) {
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			client->Close();
			printf("�ͻ���exit\n");
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
			printf("��֧�ָ�����\n");
		}
	}
}

void test1() {
	EasyTcpClient client;
	client.InitSocket();
	/*client.Connect("127.0.0.1", 4567);*/
	client.Connect("192.168.16.134", 4567);
	std::thread t1(cmdThread, &client);		//���߳���Ҫ������������
	t1.detach();
	Login login;
	strcpy(login.userName, "lyd");
	strcpy(login.passWord, "lydmm");
	while (client.isRun()) {		// ��Ҫ���ڽ�����Ϣ 
		client.OnRun();
		client.SendData(&login);
	}

	client.Close();
	printf("���˳���\n");
	getchar();
}

bool g_run = true;
void cmdThread2() {
	while (true) {
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			printf("�ͻ���exit\n");
			//closesocket(_sock);
			g_run = false;
			return;
		}
		else {
			printf("��֧�ָ�����\n");
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
