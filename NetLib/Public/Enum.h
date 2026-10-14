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

}
