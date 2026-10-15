#pragma once


namespace Net
{
	using		ClientKey	= uint64_t;
	constexpr	ClientKey	NONE_CLIENTKEY = 0;
	constexpr	const char* DEFAULT_SERVER_IP = "127.0.0.1";

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


	enum class SERVER_EVENT_TYPE
	{
		Connect = 0,		/* Connect 요청 */
		Receive_Packet = 1, /* 패킷 받기 */
		Send_Packet = 2,	/* 패킷 보내기 */
		Disconnect = 3,		/* DisConnect 요청 */
		END,
	};
}
