#include <iostream>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>	//��ͷ�ļ�Ӧ�÷���Windows.hǰ�棬���߼�һ���궨��

#pragma comment(lib,"ws2_32.lib")	//������صĿ�

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


void test1()
{
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);		//��ʼ����




	WSACleanup();		//���йر�
}


int main()
{
	std::cout << "hello c++" << std::endl;
	system("pause");
	return 0;
}
