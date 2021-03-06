#include"stdafx.h"
#include "SocketServer.h"
#include "SocketIO.h"

#pragma comment (lib, "ws2_32.lib")//add socket moudle
struct DEALPARA
{
	SOCKET		s;		
	ReceiveCallBack	lpDealFunc;
	char		IP[32];
	int			port;
	//MAP_SOCKET *LinkSocket;
	CSocketServer* _server;
};
#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02


CRITICAL_SECTION s_CS;
static void LockClients();
static void UnLockClients();
#define DEBUG_PRINTF
//#define VIRTUAL_DEBUG//虚拟调试，指不连接硬件调试
CSocketServer::CSocketServer()
{
	m_nStartup = 0;
	m_LocalSocket = INVALID_SOCKET;
	Socket_Port=6000;
	ClientConnMode=CONN_MODE_TCP;//默认TCP连接
	ListenCount=SOMAXCONN;
	InitAllSocket();
	m_SocketClients.clear();
	InitializeCriticalSection(&s_CS);
}

CSocketServer::~CSocketServer(void)
{
	ClearResource();
}
//设定连接模式TCP/UDP
void CSocketServer::SetConnectMode(int iModeValue)
{
	if (iModeValue<CONN_MODE_TCP)
	{
		iModeValue=CONN_MODE_TCP;
	}
	if (iModeValue>CONN_MODE_UDP)
	{
		iModeValue=CONN_MODE_UDP;
	}
	ClientConnMode=iModeValue;
}
void CSocketServer::ClearResource()
{
	if (m_LocalSocket != INVALID_SOCKET) 
	{
		closesocket(m_LocalSocket);
		m_LocalSocket=INVALID_SOCKET;
	}	
	//WSACleanup();
}

//软件启动时，初始化网络连接
int CSocketServer::InitAllSocket()
{
	int err;
	if ((err = WSAStartup (0x0202, &m_WsaData)) == SOCKET_ERROR)
	{
		m_nStartup=0;
		return WSASTARTUPERROR;
	}
	else
	{
		m_nStartup=1;
	}
	return 0;
}
//客户端读取连接句柄

int CSocketServer::SetSocketPort(int pPort)
{
	if((pPort>=1024) && (pPort<=65535))
	{
		Socket_Port=pPort;
		return 0;
	}
	else
	{return -1;}
}
//设定监听个数
void CSocketServer::SetListenCount(int _paraCount)
{
	if((_paraCount>0) && (_paraCount<=SOMAXCONN))
	{ListenCount=_paraCount;}
}


//开始初始化端口并绑定监听端口
int CSocketServer::CreateServer()
{
	if (m_LocalSocket != INVALID_SOCKET) 
	{
		ClearResource();	
	}    
	
	if ((m_LocalSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) 
	{ 
		int err = WSAGetLastError();
		return CREATESOCKETFAIL; 
	}

	int nRet(0);
	sockaddr_in SockAddress;
	SockAddress.sin_family=AF_INET;//表示在INT上通信
	SockAddress.sin_addr.S_un.S_addr=INADDR_ANY;
	SockAddress.sin_port=htons(Socket_Port); 
	nRet=bind(m_LocalSocket,(LPSOCKADDR)&SockAddress,sizeof(SockAddress));//绑定
	if(nRet==SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		closesocket(m_LocalSocket);
		return BIND_FAIL;
	}

	nRet=listen(m_LocalSocket,ListenCount);//监听
	if(nRet==SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		closesocket(m_LocalSocket);
		return LISTEN_FAIL;
	}
	return 1;
}
//接收客户端请求
int CSocketServer::StartServer(ReceiveCallBack receive)
{
	if(m_LocalSocket == INVALID_SOCKET) return -1;

	DWORD dwThreadId,dwThreadId1;
	
	SERVERPARA *para;
	para = new SERVERPARA;
	para->s = m_LocalSocket;
	para->port = Socket_Port;
	para->receive = receive;
	//para->SocketClients = &m_SocketClients;//zhanghongwei  2013.2.6
	para->_server = this;
	HANDLE hServerThread = CreateThread(NULL, 0, ServerListen, (LPVOID)(para), 0, &dwThreadId);
	if(hServerThread == NULL)
	{
		delete para;
		int err = WSAGetLastError();
		return -1;
	}
	CloseHandle(hServerThread);
	return 1;
}

DWORD CALLBACK CSocketServer::ServerListen(LPVOID lpParm)
{
	SERVERPARA *para = (SERVERPARA*)lpParm;

	if(para == NULL) return -1;

	SOCKET		s = para->s;
	int			port = para->port;
	ReceiveCallBack	lpDealFunc = para->receive;
	//MAP_SOCKET* SocketClients = para->SocketClients; //zhanghongwei  2013.2.6
	
	SOCKET	sClient;	
	int		iAddrSize;
	struct	sockaddr_in addr;
	char	IP[32];
	HANDLE	hThread;
	DWORD	dwThreadId;
	DEALPARA *parac;

	DEALPARA *paracSend;//zhanghongwei2013.3.20
	HANDLE	hThreadSend;
	DWORD	dwThreadIdSend;

	iAddrSize = sizeof(addr);

	while(1)
	{
		sClient = accept(s, (struct sockaddr *)&addr, &iAddrSize);

		if(sClient == SOCKET_ERROR)
			break;

		sprintf_s(IP, "%d.%d.%d.%d", 
			addr.sin_addr.S_un.S_un_b.s_b1, 
			addr.sin_addr.S_un.S_un_b.s_b2,
			addr.sin_addr.S_un.S_un_b.s_b3,
			addr.sin_addr.S_un.S_un_b.s_b4);

		parac = new DEALPARA;
		memset(parac->IP, 0, sizeof(parac->IP));
		parac->s = sClient;
		parac->port = port;
		parac->lpDealFunc = lpDealFunc;
		memcpy(parac->IP, IP, strlen(IP));
		//parac->LinkSocket = SocketClients;//zhanghongwei  2013.2.6
		parac->_server = para->_server;

		//侦听到连接，开一个线程		
		hThread = CreateThread(NULL, 0, DealClientProc, (LPVOID)(parac), 0, &dwThreadId);
		if(hThread == NULL) 
			delete parac;
		else
			CloseHandle(hThread);
	}

	delete para;
	return 0;
}


//处理客户消息线程
DWORD CALLBACK CSocketServer::DealClientProc(LPVOID lpParm)
{
	DEALPARA *para = (DEALPARA*)lpParm;

	if(para == NULL) return -1;

	SOCKET		s = para->s;
	int			port = para->port;
	ReceiveCallBack	lpDealFunc = para->lpDealFunc;
	char		IP[32];
	memcpy(IP, para->IP, sizeof(IP));
	//MAP_SOCKET* linkSocket = para->LinkSocket; //zhanghongwei  2013.2.6
	
	LockClients();
	//linkSocket->insert(make_pair(GetTickCount(),s));
	CSocketServer* pServer = para->_server;
	SocketIOPtr socketIO = SocketIOPtr( new SocketIO( s ) );
	pServer->m_SocketClients.insert( make_pair( GetTickCount(), socketIO ) );
	UnLockClients();

	lpDealFunc(s, port, IP, para->_server );
	

	LockClients();
	MAP_SOCKET::iterator iter = pServer->m_SocketClients.begin();

	for ( iter = pServer->m_SocketClients.begin(); iter != pServer->m_SocketClients.end(); iter++ )
	{
		if ( iter->second->_socket == s )
		{
			shutdown( s, SD_BOTH );
			closesocket( s );
			pServer->m_SocketClients.erase( iter );
			break;
		}
	}
	UnLockClients();

	delete para;
	return 0;
}

void LockClients()
{
	EnterCriticalSection(&s_CS);
}
void UnLockClients()
{
	LeaveCriticalSection(&s_CS);
}
int CSocketServer::SendToSocket(SOCKET socket,const char* buf, int len)
{
	//len++;

	if (socket == INVALID_SOCKET)
	{
		return -2;
	}
	int nRet;
	unsigned long len_n=htonl(len);
	//nRet= send(socket, (char*)&len_n, 4, 0);
	nRet= send(socket, buf, len, 0);
	if(nRet == SOCKET_ERROR) 
		int err = WSAGetLastError();

	return nRet;
}

int CSocketServer::BroadCastMsg( const char* buf, int len )
{
	LockClients();
	for ( MAP_SOCKET::iterator iter = m_SocketClients.begin(); iter != m_SocketClients.end(); iter++ )
	{
		if (iter->second->_initFlag)	//	判读是否已经初始化
		{
			SendToSocket( iter->second->_socket, buf, len );
		}
	}
	UnLockClients();
	return 0;
}

void CSocketServer::SetClientFlag(SOCKET clientSock)
{
	LockClients();
	for ( MAP_SOCKET::iterator iter = m_SocketClients.begin(); iter != m_SocketClients.end(); iter++ )
	{
		if (iter->second->_socket == clientSock)	//	判读是否已经初始化
		{
			iter->second->_initFlag = true;
			break;
		}
	}
	UnLockClients();
}
// 服务端接收数据
int CSocketServer::RecvFromSocket(SOCKET m_Socket, char* buf, int len,int timeout)	
{
	if (m_Socket == INVALID_SOCKET)
	{
		return -2;
	}

	//HANDLE hThread;
	//DWORD dwThreadId;
	//TPARA para;

	//para.OutTime = timeout;
	//para.s = m_Socket;
	//para.bExit = FALSE;
	//para.IsExit = FALSE;
	//hThread = CreateThread(NULL, NULL, TimeoutControl, (LPVOID)(&para), 0, &dwThreadId);
	//if (hThread == NULL) return -1;

	int nRet = recv(m_Socket, buf, len, 0);
	if(nRet == SOCKET_ERROR) 
	{
		int err = WSAGetLastError();
		int i = 0;
	}

	//para.bExit = TRUE;
	//while(!para.IsExit) Sleep(1);
	return nRet;
	
}
void CSocketServer::Close(SOCKET m_sSocket)
{
	shutdown(m_sSocket, SD_RECEIVE);
	Sleep(50);
	closesocket(m_sSocket);
	m_sSocket = NULL;	
}


                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                

// 删除多余socket
int CSocketServer::DeleteSocket(SOCKET s)
{
	LockClients();
	MAP_SOCKET::iterator iter = m_SocketClients.begin();

	for (iter = m_SocketClients.begin(); iter != m_SocketClients.end(); iter++)
	{
		if (iter->second->_socket == s)
		{
			shutdown(s, SD_BOTH);
			closesocket(s);
			m_SocketClients.erase(iter);
			break;
		}
	}
	UnLockClients();
	return 0;
}
