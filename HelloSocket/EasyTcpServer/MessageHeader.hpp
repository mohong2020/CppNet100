#ifndef _MessageHeader_hpp_
#define _MessageHeader_hpp_

#define _CRT_SECURE_NO_WARNINGS

enum CMD 
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR,
};

/*

严重性	代码	说明	项目	文件	行	禁止显示状态
错误	C2447	“{”: 缺少函数标题(是否是老式的形式表?)	EasyTcpServer	d:\mystudy\cppmillionnetwork\v1\cppnet100\hellosocket\easytcpserver\messageheader.hpp	17

*/
//消息头
struct DataHeader 
{
	DataHeader() {
		dataLength = sizeof(DataHeader);
		cmd = CMD_ERROR;
	}
	short dataLength;	//数据长度
	CMD cmd;
};

//消息体
//登录
//DataPackage
struct Login : public DataHeader {
	Login() {
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char passWord[32];
	char data[1000 - 68];
};

struct LoginResult : public DataHeader {
	LoginResult() {
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
	char data[1000 - 8];
};

//登出
struct Logout : public DataHeader {
	char userName[32];
	Logout() {
		dataLength = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}
};

struct LogoutResult : public DataHeader {
	LogoutResult() {
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 0;
	}
	int result;
};

struct NewUserJoin : public DataHeader {
	NewUserJoin() {
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	int sock;
};

#endif