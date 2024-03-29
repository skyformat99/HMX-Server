#include "WorldServer_PCH.h"
#include "Statistic.h"

#include "SharedIncludes.h"
#include "ServerInfoMgr.h"
#include "SceneInfoManger.h"


Statistic::Statistic(void)
{
}


Statistic::~Statistic(void)
{
}

void Statistic::Update(int32 nSrvTime)
{
	if(nSrvTime % 25 == 0)
	{

		ServerSession* pDpSession =	ServerSessionMgr::Instance()->GetMainDbSession();
		if(pDpSession == NULL)
		{
			
			return;
		}

		// 收集server_info信息 
		
		ServerInfoVecType o_vecServerInfo;
		ServerInfoMgr::Instance()->GetServerInfoList(0,o_vecServerInfo);

		W2DNotifyServerInfo sServerInfo;
		sServerInfo.nServerNum = 0;
		for (ServerInfoVecType::iterator it = o_vecServerInfo.begin();it != o_vecServerInfo.end() && sServerInfo.nServerNum < MAX_SERVER_NUM;it++)
		{
			ServerInfo& rInfo = *it;
			memcpy(sServerInfo.arrServerInfo[sServerInfo.nServerNum].arrHost,rInfo.arrHost,32);
			sServerInfo.arrServerInfo[sServerInfo.nServerNum].nPort = rInfo.nPort;
			sServerInfo.arrServerInfo[sServerInfo.nServerNum].nServerID = rInfo.nServerID;
			sServerInfo.arrServerInfo[sServerInfo.nServerNum].nServerType = rInfo.nServerType;
			sServerInfo.arrServerInfo[sServerInfo.nServerNum].nServerLoad = rInfo.nServerLoad;
			sServerInfo.arrServerInfo[sServerInfo.nServerNum].nClientLoad = rInfo.nClientLoad;
			sServerInfo.nServerNum++;
		}

		ServerInfoOpt& rLocalInfo = NetServerOpt::Instance()->GetLocalServerInfo();
		strcpy(sServerInfo.arrServerInfo[sServerInfo.nServerNum].arrHost,rLocalInfo.strHost.c_str());
		sServerInfo.arrServerInfo[sServerInfo.nServerNum].nPort = rLocalInfo.nPort;
		sServerInfo.arrServerInfo[sServerInfo.nServerNum].nServerType = ESERVER_TYPE_WS;
		sServerInfo.arrServerInfo[sServerInfo.nServerNum].nServerID = rLocalInfo.nID;

		int32 nServerNum = ServerSessionMgr::Instance()->CountConnected();
		int32 nClientNum = ClientSessionMgr::Instance()->ConnectedSessions();
		sServerInfo.arrServerInfo[sServerInfo.nServerNum].nServerLoad = nServerNum;
		sServerInfo.arrServerInfo[sServerInfo.nServerNum].nClientLoad = nClientNum;
		sServerInfo.nServerNum++;

		// 发送到dp 
		pDpSession->SendMsg(&sServerInfo,sServerInfo.GetPackLength());

		// 收集scene_info信息 
		SceneInfoVectorType o_vecSceneInfo;
		SceneInfoManager::Instance()->GetSceneInfo(o_vecSceneInfo);

		W2DNotifySceneInfo sSceneInfo;
		sSceneInfo.nSceneNum = 0;
		for (SceneInfoVectorType::iterator it = o_vecSceneInfo.begin(); it != o_vecSceneInfo.end();++it)
		{
			SceneInfo& rSceneInfo = *it;
			sSceneInfo.arrSceneInfo[sSceneInfo.nSceneNum].nServerID = rSceneInfo.nServerID;
			sSceneInfo.arrSceneInfo[sSceneInfo.nSceneNum].nSceneID = rSceneInfo.nSceneID;
			sSceneInfo.arrSceneInfo[sSceneInfo.nSceneNum].nLoadNum = rSceneInfo.nLoadNum;
			sSceneInfo.nSceneNum++;
		}
		pDpSession->SendMsg(&sSceneInfo,sSceneInfo.GetPackLength());
	}

	if(nSrvTime % 300 == 290)
	{
		// 发送到dp 
		ServerSession* pDpSession =	ServerSessionMgr::Instance()->GetMainDbSession();
		if(pDpSession == NULL)
		{
			//ASSERT(0);
			return;
		}

		// 执行命令，发送指令给dp，dp查出数据返回给ws，ws处理再广播到ss等 
		W2DReqEventInfo sMsg;
		sMsg.nTimestamp = Utility::NowTime(); // todo 
		pDpSession->SendMsg(&sMsg,sMsg.GetPackLength());
	}
}


