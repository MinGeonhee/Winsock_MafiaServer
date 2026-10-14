#pragma once

namespace Net
{
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

	/* 임시용 */
	struct Chat
	{
		char szMsg[256];
	};

#pragma pack(pop)

}
