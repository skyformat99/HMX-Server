#ifndef _GAMEPLAYER_H_
#define _GAMEPLAYER_H_

#include "BaseSingle.h"
#include "CommonDefine.h"
#include "BaseSession.h"
#include "ObjPool.h"
#include "ServerSession.h"

class NetSocket;
struct NetMsgHead;
class ServerSession;

/*------------------------------------------------------------------
 *
 * @Brief : 通用所有Server的客户端Session，只能用于记录通用的信息
 *          本类不能修改用于游戏功能，只限于游戏框架修改
 * @Author: hzd 
 * @File  : ClientSession.h
 * @Date  : 2015/10/18 22:33
 * @Copyright (c) 2015,hzd, All rights reserved.
 *-----------------------------------------------------------------*/
// 玩家状态 
enum EClientStatus
{
	/*-------------------------------------------------------------------
	 * @Brief : 初始化时，当Client连接成功时，初始化该Session时
	 *------------------------------------------------------------------*/
	E_CLIENT_STATUS_INITED		=0 ,	// 

	/*-------------------------------------------------------------------
	 * @Brief : 初始化完成后，设置为已经连接成功状态，
	 *          这时仅能请求密钥接口
	 *------------------------------------------------------------------*/
	E_CLIENT_STATUS_CONNECTED	 , 

	/*-------------------------------------------------------------------
	 * @Brief : 已经请求获得密钥成功，此时，Client是不能请求任何接口的，
	 *          这时服务器正给它分配负载最低的dp,ls服务器ID
	 *------------------------------------------------------------------*/
	E_CLIENT_STATUS_ENCRYPTED	 ,	// 获得密钥成功完成 

	/*-------------------------------------------------------------------
	 * @Brief : 分配ls,dp服务器id成功，此时client仅可以向ls发送帐号登录
	 * 
	 *------------------------------------------------------------------*/
	E_CLIENT_STATUS_NOTIFY_INITED ,  

	/*-------------------------------------------------------------------
	 * @Brief : 
	 * 
	 *------------------------------------------------------------------*/
	E_CLIENT_STATUS_LOGINED	,	// 登录完成完成 
	E_CLIENT_STATUS_SELECTED,	// 选择角色完成 
	E_CLIENT_STATUS_IN_WORLD,	// 可以进行数据交互完成  
	E_CLIENT_STATUS_IN_SCENE,	// 在游戏场景中 
};


/*
 *
 *	Detail: 玩家类
 *   
 *  Created by hzd 2015/01/26  
 *
 */
class ClientSession : public BaseSession
{

public:

	ClientSession(int32 nClientSessionID);

	virtual ~ClientSession();

	// fep中forclient使用 
	void SetForRemoteClient(NetSocket& rSocket);

	// all s 中for ws 
	void SetForRemoteWs(ServerSession* pWsSession);

	// fep,dp中使用 
	void SetForLocalFepDp(ServerSession* pLsSession,ServerSession* pSsSession); // 同步设置 

	// ls,ss中使用 
	void SetForLocalLsSs(ServerSession* pWsSession,ServerSession* pFepSession,ServerSession* pDpSession); // 同步设置 

	// ws中来说
	void SetForLocalWs1(ServerSession* pFepSession,ServerSession* pLsSession,ServerSession* pDpSession); // 同步设置 
	void SetForLocalWs2(ServerSession* pSsSession,int32 nSceneID); // 进入地图设置 

	/*
	 *
	 *	Detail: 获得ID
	 *   
	 *  Created by hzd 2015/01/21  
	 *
	 */
	int32 SessionID();

	int32 GetLsServerID()
	{
		if (m_pLs)
			return m_pLs->ServerID();
		return 0;
	}

	int32 GetSsServerID()
	{
		if (m_pSs)
			return m_pSs->ServerID();
		return 0;
	}

	int32 GetFepServerID()
	{
		if (m_pFep)
			return m_pFep->ServerID();
		return 0;
	}

	int32 GetDpServerID()
	{
		if (m_pDp)
			return m_pDp->ServerID();
		return 0;
	}

	/*
	 *
	 *	Detail: 发送消息到client 
	 *   
	 *  Created by hzd 2015/01/21  
	 *
	 */
	virtual void SendMsg( NetMsgHead* pMsg,int32 nSize);

	void SendMsgToWs( NetMsgHead* pMsg,int32 nSize);

	void SendMsgToSs( NetMsgHead* pMsg,int32 nSize);

	void SendMsgToLs( NetMsgHead* pMsg,int32 nSize);

	void SendMsgToDp( NetMsgHead* pMsg,int32 nSize);

	void SendMsgToFep( NetMsgHead* pMsg,int32 nSize);

	/*
	 *	惹要断开该连接，则调用该接口 
	 *	Detail: 玩家主动/服务器调用此退出
	 *   
	 *  Created by hzd 2015-1-21
	 *
	 */
	virtual void Exist();

	/*
	 *
	 *	Detail: 设置状态
	 *   
	 *  Created by hzd 2015-5-27
	 *
	 */
	virtual void SetStatus(int32 nStatus);

	/*
	 *
	 *	Detail: 获得状态
	 *   
	 *  Created by hzd 2015-5-27
	 *
	 */
	virtual int32 Status();

	/*
	 *
	 *	Detail: 定时循环，参数为延时毫秒
	 *   
	 *  Created by hzd 2015/01/21  
	 *
	 */
	void Update(int32 nSrvTime);

	/*
	 *
	 *	Detail: 设置通信密钥
	 *   
	 *  Created by hzd 2015/01/21  
	 *
	 */
	void SetEncryptKey(char arrNewKey[MAX_ENCRYPT_LENTH]);

	/*
	 *
	 *	Detail: 获得通信密钥
	 *   
	 *  Created by hzd 2015/01/21  
	 *
	 */
	void GetEncryptKey(char o_arrEncryptKey[MAX_ACCOUNT_LENG]);

	/*
	 *
	 *	Detail: 设置服务器类型
	 *   
	 *  Created by hzd 2015-5-28
	 *
	 */
	void SetOnServerType(int32 nServerType);

	/*
	 *
	 *	Detail: 获得服务器类型
	 *   
	 *  Created by hzd 2015-5-28
	 *
	 */
	int32 OnServerType();

	ServerSession* GetWsSession();

	ServerSession* GetSsSession();

	ServerSession* GetFepSession();

	ServerSession* GetLsSession();

	ServerSession* GetDpSession();

	void SetSceneID(int32 nSceneID);

	int32 ScenenID();


protected:

	void SendMsg(NetSocket* pSocket,NetMsgHead* pMsg,int32 nSize);

private:

private:

	NetSocket*			m_pSocket;							// Fep中给client专用 

	int32				m_nSessionID;						// 即在fep中client过来的SocketID 
	int32				m_SendPackCount;					// 发包数量（校验包顺序、丢包情况）
	char				m_arrEncryptKey[MAX_ENCRYPT_LENTH];	// 通信密钥 
	int32				m_nSceneID;							// 所在的场景ID 
	
	ServerSession*		m_pWs;								// 所有的ws 
	ServerSession*      m_pFep;								// 所在的fep，连接后可获得 
	ServerSession*		m_pLs;								// 所在的ls,预备后获得	 
	ServerSession*		m_pDp;								// 所在的dp,预备后获得 
	ServerSession*		m_pSs;								// 所在的ss,登录后获得 
	
	int32				m_nOnServerType;					// 所属服务器类型上 

};


/*
 *
 *	Detail: 玩家管理器
 *   
 *  Created by hzd 2015/01/26  
 *
 */
class ClientSessionMgr : public BaseSingle<ClientSessionMgr>
{
	typedef map< int32 , ClientSession* >	ClientSessionMapType; // key为ssessionID(socketid不唯一，多个session可以对应一个socket) 
public:

	ClientSessionMgr();
	~ClientSessionMgr();

	/*
	 *
	 *	Detail: 创建一个ClientSession
	 *   
	 *  Created by hzd 2015-5-27
	 *
	 */
	ClientSession* AddSession(int32 nClientSessionID);

	/*
	 *
	 *	Detail: 释放Player
	 *   
	 *  Created by hzd 2015/01/26  
	 *
	 */
	void RemoveSession(int32 nFepSessionID);

	/*
	 *
	 *	Detail: 获得Player
	 *   
	 *  Created by hzd 2015/01/26  
	 *
	 */
	ClientSession*	GetSession(int32 nFepSessionID);
		  
	/*
	 *
	 *	Detail: 广播消息
	 *   
	 *  Created by hzd 2015/01/26  
	 *
	 */
	void SendToAll( NetMsgHead* pMsg,int32 nSize,EServerType eToServerType);

	/*
	 *
	 *	Detail: 更新
	 *   
	 *  Created by hzd 2015/01/26  
	 *
	 */
	void Update(uint32 nSrvTime);

	/*
	 *
	 *	Detail: no
	 *   
	 * Copyright (c) Created by hzd 2015-4-28.All rights reserved
	 *
	 */
	void SendMsgToLs(NetMsgHead* pMsg,int32 nSize);

	/*
	 *
	 *	Detail: no
	 *   
	 * Copyright (c) Created by hzd 2015-4-28.All rights reserved
	 *
	 */
	void SendMsgToWs(NetMsgHead* pMsg,int32 nSize);

	/*
	 *
	 *	Detail: no
	 *   
	 * Copyright (c) Created by hzd 2015-4-28.All rights reserved
	 *
	 */
	void SendMsgToSs(NetMsgHead* pMsg,int32 nSize);

	/*
	 *
	 *	Detail: 统计连接上ws的玩家数
	 *   
	 *  Created by hzd 2015-5-29
	 *
	 */
	int32 ConnectedSessions();

private:

	ClientSessionMapType			m_mapClientSession;			// 所有玩家缓存(连接与未连接)					
	static ObjPool<ClientSession>	s_cClientSessionFactory;	// ClientSession工厂 

};

#endif

