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
	RECV_RESULT Receive_Packet(CSocket* pReceiveSocket, PacketHeader* pOutPacket)
	{
		if (pReceiveSocket == nullptr)		return RECV_RESULT::NET_ERROR;
		if (pOutPacket == nullptr)			return RECV_RESULT::NET_ERROR;

		/* 처음 패킷 헤더만큼만 읽는다 */
		int iLen{ recv(pReceiveSocket->Get_Socket(), reinterpret_cast<char*>(pOutPacket) , sizeof(PacketHeader), MSG_WAITALL) };
		if (iLen == 0)						return RECV_RESULT::CLOSED;		// 상대가 정상 종료
		if (iLen != sizeof(PacketHeader))	return RECV_RESULT::NET_ERROR;	// 에러 / 헤더 받다가 끊김

		/* 길이 검사 : 버퍼보다 크면 받지 않고 버림 */
		if (pOutPacket->usLen > MAX_PACKET_DATA_LEN) return RECV_RESULT::INVALID_PACKET;

		if (pOutPacket->usLen > 0) // 실제 데이터 량이 0 보다 크다면 받기
		{
			iLen = recv(pReceiveSocket->Get_Socket(), reinterpret_cast<char*>(pOutPacket) + sizeof(PacketHeader),
				pOutPacket->usLen, MSG_WAITALL);

			if (iLen == 0)					return RECV_RESULT::CLOSED;
			if (iLen != pOutPacket->usLen)	return RECV_RESULT::NET_ERROR;	// 에러 / 본문 받다가 끊김
		}

		return RECV_RESULT::OK;
	}
}
