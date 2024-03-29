#include "NetSocket.h"
#include "NetServer.h"

/*
   默认SocketDefine

*/
SocketDefine SocketDefine::g_sDefine;

/*-------------------------------------------------------------------
 * @Brief : Scoket数据处理类
 * 
 * @Author:hzd 2012/04/03
 *------------------------------------------------------------------*/
NetSocket::NetSocket(io_service& rIo_service,int32 nMaxRecivedSize,int32 nMaxSendoutSize,int32 nMaxRecivedCacheSize,int32 nMaxSendoutCacheSize):tcp::socket(rIo_service)
	, m_cHeadBuffer(buffer(m_arrRecvBuffer, PACKAGE_HEADER_SIZE))
	, m_cBodyBuffer(buffer(m_arrRecvBuffer + PACKAGE_HEADER_SIZE, nMaxRecivedSize - PACKAGE_HEADER_SIZE))
	, m_cSendBuffer(buffer(m_arrSendBuffer, nMaxSendoutSize))
	, m_nBodyLen(0)
	, m_eRecvStage(REVC_FSRS_NULL)
	, m_bSending(false)
	, m_bExit(false)
	, m_cTimer(rIo_service)
	, m_cCloseTimer(rIo_service)
	, m_nMaxSendoutSize(nMaxSendoutSize)
	, m_nMaxRecivedSize(nMaxRecivedSize)
	, m_nTimeout(0)
	, m_errorCode(-1)
{
	static int32 nSocketIncreseID = 1;
	m_nID = nSocketIncreseID++;
	m_cIBuffer.InitBuffer(nMaxRecivedCacheSize);
	m_cOBuffer.InitBuffer(nMaxSendoutCacheSize);
}

NetSocket::~NetSocket()
{

}

void NetSocket::ReadHead()
{
	async_read(*this, m_cHeadBuffer,
		transfer_exactly(PACKAGE_HEADER_SIZE), 
		boost::bind(&NetSocket::RecvMsg, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	TimeoutStart();
}


void NetSocket::ReadBody()
{
	async_read(*this, m_cBodyBuffer,
		transfer_exactly(m_nBodyLen),
		boost::bind(&NetSocket::RecvMsg, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	TimeoutStart();
}

void NetSocket::Disconnect()
{
#ifdef WIN32
	boost::system::error_code ec;
	HandleClose(ec);
#else
	try
	{
		boost::system::error_code ec;
		shutdown(socket_base::shutdown_both, ec);
	}
	catch (...)
	{
	}

	m_cCloseTimer.cancel();
	m_cCloseTimer.expires_from_now(boost::posix_time::seconds(1));
	m_cCloseTimer.async_wait(boost::bind(&NetSocket::HandleClose, this, boost::asio::placeholders::error));

#endif // WIN32

}

void NetSocket::HandleClose(const boost::system::error_code& error)
{
	try
	{
		boost::asio::socket_base::linger option(true, 0);
		boost::system::error_code ec1;
		set_option(option, ec1);
		boost::system::error_code ec2;
		tcp::socket::close(ec2); // 这个不会再收到消息回调 end 
	} catch(...)
	{
	}
}

EMsgRead NetSocket::ReadMsg(NetMsgHead** pMsg,int32& nSize)
{
	if(m_bExit)
	{
		printf("[INFO]:m_bExit == true\n");
		return MSG_READ_INVALID;
	}	

	uint32 nLen = m_cIBuffer.GetLen();
	if(!nLen)
	{	
		return MSG_READ_WAITTING;
	}

	void* cBuff = m_cIBuffer.GetStart();
	memcpy(&nSize,cBuff, PACKAGE_HEADER_SIZE );
	if(nLen < nSize + PACKAGE_HEADER_SIZE)
	{
		return MSG_READ_REVCING;
	}
	*pMsg = (NetMsgHead*)((char*)cBuff + PACKAGE_HEADER_SIZE);
	return MSG_READ_OK;
	
}

void NetSocket::RemoveMsg(uint32 nLen)
{
	m_cIBuffer.RemoveBuffer(nLen);
}

void NetSocket::RecvMsg(const boost::system::error_code& ec, size_t bytes_transferred)
{
	TimeoutCancel();
	if(ec)
	{
		printf("[ERROR]:Recv error msg,%s\n",ec.message().c_str());
		m_bExit = true;
		m_errorCode = ESOCKET_EXIST_REMOTE;
		return;
	}

	switch(m_eRecvStage)
	{
		case REVC_FSRS_HEAD:
		{
			ASSERT(bytes_transferred == PACKAGE_HEADER_SIZE);
			memcpy(&m_nBodyLen,m_arrRecvBuffer,PACKAGE_HEADER_SIZE);
			if(m_nBodyLen < sizeof(NetMsgHead) || m_nBodyLen > m_nMaxRecivedSize)
			{
			   printf("[ERROR]:Recv data length,bodylen:%d, maxLimitLength:%d\n",m_nBodyLen,m_nMaxRecivedSize);
			   m_bExit = true;
			   m_errorCode = ESOCKET_EXIST_PACK_LENGTH_ERROR;
			   m_nBodyLen = 0;
			   return;
			}
			bool bResult = m_cIBuffer.Write(m_arrRecvBuffer, PACKAGE_HEADER_SIZE);
			if(!bResult)
			{
				printf("[ERROR]:Write too much data to buffer\n");
				m_bExit = true;
				m_errorCode = ESOCKET_EXIST_WRITE_TOO_DATA;
				return;
			}
			m_eRecvStage = REVC_FSRS_BODY;
			ReadBody();
		}
			break;
		case REVC_FSRS_BODY:
		{
			bool bResult = m_cIBuffer.Write(m_arrRecvBuffer + PACKAGE_HEADER_SIZE, m_nBodyLen);
			if(!bResult)
			{
				printf("[ERROR]:Write too much data to buffer\n");
				m_bExit = true;
				m_errorCode = ESOCKET_EXIST_WRITE_TOO_DATA;
				return;
			}
			m_eRecvStage = REVC_FSRS_HEAD;
			ReadHead();
		}
			break;
		default:
		{
			ASSERT(0);
		}
			break;
	}
}

void NetSocket::Clear()
{
	m_bSending = false;
	m_nBodyLen = 0;
	m_bExit = false;
	m_eRecvStage = REVC_FSRS_NULL;
	m_cIBuffer.ClearBuffer();
	m_cOBuffer.ClearBuffer();
}

void NetSocket::ParkMsg(NetMsgHead* pMsg,int32 nLength)
{
	ASSERT(nLength < 65336);
	char arrLen[4];
	memcpy(arrLen,&nLength,4);;
	m_cOBuffer.Write(arrLen,4);
	m_cOBuffer.Write((char*)pMsg, nLength);
}

void NetSocket::SendMsg()
{
	if(m_bSending)
		return;

	if(int nLen = m_cOBuffer.ReadRemove(&m_arrSendBuffer,m_nMaxSendoutSize))
	{
		m_bSending = true;
		async_write(*this, m_cSendBuffer, transfer_exactly(nLen), boost::bind(&NetSocket::SendMsg, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}
}

void NetSocket::SendMsg(const boost::system::error_code& ec, size_t bytes_transferred)
{
	if(ec)
	{
		printf("[ERROR]:Send msg date error\n");
		m_bExit = true;
		m_errorCode = ESCOKET_EXIST_SEND_CONNECT_INVAILD;
		return;
	}

	if(int nLen = m_cOBuffer.ReadRemove(&m_arrSendBuffer,m_nMaxSendoutSize))
	{
		async_write(*this, m_cSendBuffer, transfer_exactly(nLen), boost::bind(&NetSocket::SendMsg, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	} else
	{
		m_bSending = false;
	}
}

void NetSocket::HandleWait(const boost::system::error_code& error)
{
	if(error)
	{
		return;
	}
	printf("[INFO]:That is timeout!\n");
	m_bExit = true;
	m_errorCode = ESOCKET_EXIST_TIMEOUT;
}

void NetSocket::Run()
{
	m_eRecvStage = REVC_FSRS_HEAD;
	ReadHead();
}

const string NetSocket::GetIp()
{
	return remote_endpoint().address().to_string();
}

void NetSocket::SetWillColse()
{
	printf("[WARRING]:SetWillColse\n");
	m_bExit = true;
	m_errorCode = ESOCKET_EXIST_LOCAL;
}

void NetSocket::SetTimeout(int32 nTimeout)
{
	ASSERT(nTimeout > -1);
	m_nTimeout = nTimeout;
}

void NetSocket::TimeoutStart()
{
	if(m_nTimeout)
	{
		m_cTimer.cancel();
		m_cTimer.expires_from_now(boost::posix_time::seconds(m_nTimeout));
		m_cTimer.async_wait(boost::bind(&NetSocket::HandleWait, this, boost::asio::placeholders::error));
	}
}

void NetSocket::TimeoutCancel()
{
	if(m_nTimeout)
	{
		m_cTimer.cancel();
	}

}

int32 NetSocket::ErrorCode(std::string& error)
{
	switch (m_errorCode)
	{
	case ESOCKET_EXIST_NULL:
		error = "Not thing";
		break;
	case ESOCKET_EXIST_LOCAL:
		error = "Local active close";
		break;
	case ESOCKET_EXIST_REMOTE:
		error = "Remote active close";
		break;
	case ESOCKET_EXIST_TIMEOUT:
		error = "Timeout";
		break;
	case ESOCKET_EXIST_PACK_LENGTH_ERROR:
		error = "Data lenth error";
		break;
	case ESOCKET_EXIST_WRITE_TOO_DATA:
		error = "Write too data";
		break;
	case ESCOKET_EXIST_SEND_CONNECT_INVAILD:
		error = "Send connected invaild";
		break;
	default:
		error = "Unkown";
		break;
	}
	return m_errorCode;
}



