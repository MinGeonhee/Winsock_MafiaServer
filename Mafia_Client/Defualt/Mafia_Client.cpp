// Mafia_Client.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include "pch.h"

using namespace std;

int main()
{
	Net::Net_Init();

	std::unique_ptr<Net::CSocket> pSocket{ Net::CSocket::Create(Net::TYPE::CLIENT, Net::PROTOCAL_TYPE::TPC, Net::NET_PROTOCOL_TYPE::IPv4) };
	std::unique_ptr<Net::CAddress> pAddress = Net::CAddress::Create(Net::TYPE::CLIENT, Net::NET_PROTOCOL_TYPE::IPv4 , 1234 , "127.0.0.1");

	/* 메세지 받기 */
	pSocket->Connect(pAddress.get());

	char szBuf[Net::MAX_PACKET_TOTAL_LEN]{};
	Net::Receive_Packet(pSocket.get(), reinterpret_cast<Net::PacketHeader*>(szBuf));
	


	if (reinterpret_cast<Net::PacketHeader*>(szBuf)->usType == 0)
	{
		//채팅
		Net::Packet<Net::Chat>* pChat = reinterpret_cast<Net::Packet<Net::Chat>*>(szBuf);

		std::cout << pChat->tData.szMsg << std::endl;
	}


	Net::Net_Release();
}
