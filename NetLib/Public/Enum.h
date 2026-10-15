#pragma once


namespace Net
{

	enum TYPE
	{
		SERVER,
		CLIENT,
		TYPE_END
	};

	enum PROTOCAL_TYPE
	{
		TPC = IPPROTO_TCP,
		UDP = IPPROTO_UDP,
		PROTOCAL_TYPE_END
	};

	enum NET_PROTOCOL_TYPE
	{
		IPv4,
		IPv6,
		NET_PROTOCOL_TYPE_END,
	};


	enum SERVER_EVENT_TYPE
	{
		Connect = 0,		/* Connect 요청 */
		Receive_Packet = 1, /* 패킷 받기 */
		Send_Packet = 2,	/* 패킷 보내기 */
		Disconnect = 3,		/* DisConnect 요청 */
		SERVER_EVENT_TYPE_END,
	};

}
