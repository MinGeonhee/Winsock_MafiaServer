#pragma once


namespace Net
{
	/* 스마트 포인터 사용시  */
	struct  FreeDeleter
	{
		template<class T>
		void operator()(T* p) const { if (p) p->Free(); }
	};

	bool Net_Init();
	inline void Net_Release()
	{
		WSACleanup();
	}

	/* Send Packet */
	bool	Send_Packet(class CSocket* pReceiveSocket, const struct PacketHeader* pPacket);

	/* Receive Packet : pOutPacket 버퍼는 MAX_PACKET_TOTAL_LEN 이상이어야 함
	   (Enum.h보다 먼저 include되므로 여기서는 선언만. 정의는 Enum.h) */
	enum class RECV_RESULT : uint8_t;
	RECV_RESULT	Receive_Packet(class CSocket* pReceiveSocket,  struct PacketHeader* pOutPacket);
}

