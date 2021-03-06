#pragma once
#include "SocketDefine.h"
#include "WinSock2.h"
#include <map>
#include <vector>
#include "SocketIO.h"
using namespace std;
typedef map<int, SocketIOPtr > MAP_SOCKET;

typedef void (CALLBACK* ReceiveCallBack)(SOCKET s, int ServerPort, const char *ClientIP, void* pServer );
class  CSocketServer;
struct SERVERPARA
{
	SOCKET		s;
	int			port;
	ReceiveCallBack	receive;
	//MAP_SOCKET *SocketClients;
	CSocketServer* _server;
};

class  CSocketServer {
public:
	CSocketServer(void);
	// TODO: 在此添加您的方法。
	~CSocketServer();          
	// 启动服务器的接口
	int CreateServer();
	// 开始服务Accept监视线程
	int StartServer(ReceiveCallBack lpDealFunc);	
	// 服务端->客户端发送数据
	int SendToSocket(SOCKET SocketClient,const char* buf, int len);
	// 服务端接收客户端数据
	int BroadCastMsg(const char* buf, int len);
	void SetClientFlag(SOCKET clientSock);
	int RecvFromSocket(SOCKET SocketClient, char* buf, int len,int timeout);
	//处理客户到达线程
	static DWORD CALLBACK DealClientProc(LPVOID lpParm);
	static DWORD CALLBACK ServerListen(LPVOID lpParm);
	//设置Socket连接端口号
	int SetSocketPort(int pPort);
	//释放资源
	void ClearResource();
	//软件启动时，初始化网络连接
	int InitAllSocket();
	void Close(SOCKET m_sSocket);
	//设定监听个数
	void SetListenCount(int _paraCount);	
	//设定连接模式TCP/UDP
	void SetConnectMode(int iModeValue);
protected:
	int		m_nStartup;		// 启动服务是否成功
	SOCKET	m_LocalSocket;
	//the server's WsaData
	WSADATA m_WsaData;	     
private:
	int Socket_Port;//端口号
	char        InBuffer[DATA_BUFSIZE];   // 输入
	char        OutBuffer[DATA_BUFSIZE];  // 输出

	
	//服务端监听个数
	int ListenCount;
	//连接模式TCP/UDP
	int ClientConnMode;


public:
	MAP_SOCKET m_SocketClients;//存放用户的ID，以及对应的socket
	SOCKET UDPSERVER;
	// 删除多余socket
	int DeleteSocket(SOCKET s);
};

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                