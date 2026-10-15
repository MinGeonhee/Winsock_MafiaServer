#pragma once

namespace Net
{
	class CSocket;
	class CAddress;


	/* Server Event 최상위 구조체 */
	struct Event
	{
		/* Event Type */
		SERVER_EVENT_TYPE eEventType{ SERVER_EVENT_TYPE::END };

		/* Connect 요청 전용 */
		std::unique_ptr <Net::CSocket> pSocket{ nullptr };
		std::unique_ptr<Net::CAddress>  pAddress{ nullptr };

		/* Receive , Send Packet , DisConnect */
		ClientKey	iSenderKey{ NONE_CLIENTKEY };
		ClientKey	iReceiverKey{ NONE_CLIENTKEY };

		/* Receive , Send Packet Char형태의 데이터로 담아두기 */
		std::vector<char>  vecPacket{};
	};

	/* 앞쪽 4바이트 읽는 용도 */
#pragma pack(push, 1)
	struct PacketHeader
	{
		uint16_t usType{ 0 }; //어떤 타입의 데이터를 보낼건지 서버&클라이언트 끼리 합의한 타입으로 보내기
		uint16_t usLen{ 0 }; //헤더를 제외한 실제로 보내질 데이터의 크기

	public:
		PacketHeader() = default;
		PacketHeader(uint16_t _usType, uint16_t _usLen)
			: usType(_usType), usLen(_usLen) {
		}
	};
#pragma pack(pop)

	constexpr uint16_t MAX_PACKET_TOTAL_LEN = 1024;                                  // 전체
	constexpr uint16_t MAX_PACKET_DATA_LEN{ MAX_PACKET_TOTAL_LEN - static_cast<uint16_t>(sizeof(PacketHeader)) };  //

#pragma pack(push, 1)
	template<class T>
	struct  Packet : public PacketHeader
	{
		T				tData{};
	public:
		Packet() = default;
		Packet(uint16_t _iPacketType, const T& _tData)
			: PacketHeader{ _iPacketType,sizeof(T),} ,tData{ _tData }
		{
		}
	};
#pragma pack(pop)


	/* 기록 */
	struct NetLog
	{
		/* 어떤 타입의 로그인지 */
		Net::NET_LOG_TYPE eType{ Net::NET_LOG_TYPE::END };
		
		ClientKey		  iKey{};
		std::string		  strIP{};
		uint16_t		  iPortNum{};
		SYSTEMTIME		  tTime{};
	};
}
