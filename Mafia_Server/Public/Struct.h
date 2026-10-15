#pragma once

namespace MafiaServer
{
	enum class Event_Type
	{
		Connect = 0,		/* Connect 요청 */
		Receive_Packet = 1, /* 패킷 받기 */
		Send_Packet = 2,	/* 패킷 보내기 */
		Disconnect = 3,		/* DisConnect 요청 */
		END,
	};


	/* Server Event 최상위 구조체 */
	struct ServerEvent
	{
		/* Event Type */
		Event_Type	eEventType{ Event_Type::END };

		/* Connect 요청 전용 */
		std::unique_ptr<Net::CSocket>   pSocket{nullptr};  
		std::unique_ptr<Net::CAddress>  pAddress{ nullptr };   

		/* Receive , Send Packet , DisConnect */
		ClientKey	iSenderKey{NONE_CLIENTKEY};
		ClientKey	iReceiverKey{NONE_CLIENTKEY};
		
		/* Receive , Send Packet Char형태의 데이터로 담아두기 */
		std::vector<char>  vecPacket{};
	};
}

