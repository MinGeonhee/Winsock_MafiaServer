#pragma once

namespace Net
{
	class CSocket;
	class CAddress;


	/* Server Event 최상위 구조체 */
	struct Event
	{
		friend class CServer;   // (Net Lib 자체적으로 사용할 경우에는 CSocket, CAddress를 채울 수 있게 하기 위함)
		friend class CClient;   // 
	public:
		/* Event Type */
		NET_EVENT_TYPE eEventType{ NET_EVENT_TYPE::END };
	private:
		/* Connect 요청 전용 */
		std::unique_ptr <Net::CSocket>  pSocket{ nullptr };
		std::unique_ptr<Net::CAddress>  pAddress{ nullptr };
	public:
		/* Receive , Send Packet , DisConnect */
		ClientKey	iKey{ NONE_CLIENTKEY };

		/* Receive , Send Packet Char형태의 데이터로 담아두기 (헤더 + 본문 그대로) */
		std::vector<char>  vecPacket{};
	public:
		/* 게임 쪽에서 패킷 읽을 때 사용 (vecPacket이 비었거나 짧으면 0 / nullptr) */
		uint16_t	Get_PacketType() const;
		uint16_t	Get_BodyLen() const;
		const char* Get_Body() const;
		bool		Is_NetPacket() const;	/* NetLib 내부 패킷인지 (키 할당 등) */
	};

	/* 앞쪽 4바이트 읽는 용도 */
#pragma pack(push, 1)
	struct PacketHeader
	{
		uint16_t usType{ 0 }; //어떤 타입의 데이터를 보낼건지 서버&클라이언트 끼리 합의한 타입으로 보내기
		uint16_t usLen{ 0 }; //헤더를 제외한 실제로 보내질 데이터의 크기
		uint8_t	 isNet{ 0 };	//1 = NetLib 내부 패킷 (게임 콜백으로 안 넘김), 0 = 게임 패킷. 네트워크로 나가므로 bool 대신 크기 고정 타입

	public:
		bool Is_NetPacket() const { return isNet != 0; }
		PacketHeader() = default;
		PacketHeader(uint16_t _usType, uint16_t _usLen, uint8_t _isNet = 0)
			: usType(_usType), usLen(_usLen), isNet(_isNet) {
		}
	};
#pragma pack(pop)

	constexpr uint16_t MAX_PACKET_TOTAL_LEN = 1024;                                  // 전체
	constexpr uint16_t MAX_PACKET_DATA_LEN{ MAX_PACKET_TOTAL_LEN - static_cast<uint16_t>(sizeof(PacketHeader)) };  //

	inline uint16_t Event::Get_PacketType() const
	{
		if (vecPacket.size() < sizeof(PacketHeader)) return 0;
		PacketHeader tHeader{};
		memcpy(&tHeader, vecPacket.data(), sizeof(PacketHeader));
		return tHeader.usType;
	}
	inline uint16_t Event::Get_BodyLen() const
	{
		return vecPacket.size() < sizeof(PacketHeader) ? 0 : static_cast<uint16_t>(vecPacket.size() - sizeof(PacketHeader));
	}
	inline const char* Event::Get_Body() const
	{
		return vecPacket.size() < sizeof(PacketHeader) ? nullptr : vecPacket.data() + sizeof(PacketHeader);
	}
	inline bool Event::Is_NetPacket() const
	{
		if (vecPacket.size() < sizeof(PacketHeader)) return false;
		PacketHeader tHeader{};
		memcpy(&tHeader, vecPacket.data(), sizeof(PacketHeader));
		return tHeader.Is_NetPacket();
	}

	/* 헤더 + 본문을 이어 붙인 보낼 바이트 만들기 (본문이 너무 크면 빈 vector) */
	inline std::vector<char> Make_PacketBytes(uint8_t isNet,uint16_t usType, const void* pBody, uint16_t usLen)
	{
		if (usLen > MAX_PACKET_DATA_LEN || (usLen > 0 && pBody == nullptr)) return {};

		std::vector<char> vecBytes(sizeof(PacketHeader) + usLen);
		const PacketHeader tHeader{ usType, usLen, isNet };
		memcpy(vecBytes.data(), &tHeader, sizeof(PacketHeader));
		if (usLen > 0) memcpy(vecBytes.data() + sizeof(PacketHeader), pBody, usLen);
		return vecBytes;
	}

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


	/* NetLib 내부 패킷 (isNet = 1) */
#pragma pack(push, 1)
	struct Client_Login		/* 서버 NetLib → 클라 NetLib : 접속 직후 "너의 키는 #n" */
	{
		ClientKey iClientKey{ NONE_CLIENTKEY };
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

		std::wstring	  strText{};	/* Custom 로그일 때만 사용 */
	};

	using		NetEventFuntion = std::function<void(const Net::Event& tEvent)>;
}
