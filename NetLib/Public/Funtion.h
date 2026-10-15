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

	int		Receive_Packet(class CSocket* pReceiveSocket,  struct PacketHeader* pOutPacket);
}

