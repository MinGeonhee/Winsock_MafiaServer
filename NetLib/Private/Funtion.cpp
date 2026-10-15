#include "pch.h"

namespace Net
{
	bool Net_Init()
	{
		WSADATA tData{};
		return WSAStartup(MAKEWORD(2, 2), &tData) == 0;
	}

	bool Send_Packet(CSocket* pReceiveSocket, const PacketHeader* pPacket)
	{
		if (pReceiveSocket == nullptr) return false;
		if (pPacket == nullptr) return false;
		if (pPacket->usLen > MAX_PACKET_DATA_LEN) return false;


		if (send(pReceiveSocket->Get_Socket(), reinterpret_cast<const char*>(pPacket), pPacket->usLen + sizeof(PacketHeader), 0) == SOCKET_ERROR)
		{
			NET_LOG("Send 실패 : %d", WSAGetLastError());
			return false;
		}

		return true;
	}
	int Receive_Packet(CSocket* pReceiveSocket, PacketHeader* pOutPacket)
	{
		if (pReceiveSocket == nullptr)		return -1;
		if (pOutPacket == nullptr)			return -1;

		/* 처음 패킷 헤더만큼만 읽는다 */
		int iLen{ recv(pReceiveSocket->Get_Socket(), reinterpret_cast<char*>(pOutPacket) , sizeof(PacketHeader), MSG_WAITALL) };
		if (iLen != sizeof(PacketHeader)) return iLen;

		if (pOutPacket->usLen > 0) // 실제 데이터 량이 0 보다 크다면 받기
		{
			if (pOutPacket->usLen > MAX_PACKET_DATA_LEN) return -1;

			iLen = recv(pReceiveSocket->Get_Socket(), reinterpret_cast<char*>(pOutPacket) + sizeof(PacketHeader),
				pOutPacket->usLen, MSG_WAITALL);

			if (iLen != pOutPacket->usLen ) return iLen;
		}

		return pOutPacket->usLen + sizeof(PacketHeader);
	}
}
