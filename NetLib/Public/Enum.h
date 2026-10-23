#pragma once


namespace Net
{
	using		ClientKey	= uint64_t;
	constexpr	ClientKey	NONE_CLIENTKEY = 0;
	constexpr	const char* DEFAULT_SERVER_IP = "127.0.0.1";

	enum class NET_LOG_TYPE
	{
		Connect,     
		Disconnect,		
		Reject,			// 127.0.0.1:50300  접속 거절 (요청 OFF)   ← 선택
		Custom,			// 게임이 남기는 로그 (로그인 / 로그아웃 등). 내용은 NetLog::strText
		END,
	};


	enum class TYPE
	{
		SERVER,
		CLIENT,
		END
	};

	enum class PROTOCAL_TYPE
	{
		TCP = IPPROTO_TCP,
		UDP = IPPROTO_UDP,
		END
	};

	enum class NET_PROTOCOL_TYPE
	{
		IPv4,
		IPv6,
		END,
	};


	enum class NET_EVENT_TYPE
	{
		Connect = 0,		/* Connect 요청 */
		Reject_Connect,
		Receive_Packet,		/* 패킷 받기 */
		Send_Packet,		/* 패킷 보내기 */
		Disconnect,			/* DisConnect 요청 */
		END,
	};

	/* Receive_Packet 결과 */
	enum class RECV_RESULT : uint8_t
	{
		OK = 0,				/* 패킷 하나(헤더 + 본문) 다 받음 */
		CLOSED,				/* recv == 0 : 상대가 정상 종료 */
		NET_ERROR,			/* recv < 0 또는 중간에 끊김 : 강제 종료, 네트워크 끊김 */
		INVALID_PACKET,		/* 길이가 최대값 초과 : 잘못된(조작된) 패킷 */
		END,
	};

	enum class NET_PROTOCAL_TYPE
	{
		Client_Login = 0,
		END
	};

}
