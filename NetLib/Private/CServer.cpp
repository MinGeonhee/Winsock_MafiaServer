#include "pch.h"
#include "CServer.h"


namespace Net
{
	CServer::CServer()
		: m_pSocket{ nullptr }, m_pAddress{ nullptr }, m_isOpenServer{ false }
		, m_thAcceptReceiver{}, m_mtxAcceptReceive{}, m_isAllowAccept{ false }, m_iClientKey{ 1 }
		, m_ServerEventQueue{}
	{
	}
	CServer::~CServer()
	{
		Net::Net_Release();
		m_isAllowAccept = false;
		m_pSocket.reset();
		m_pAddress.reset();

		if (m_thAcceptReceiver.joinable())
			m_thAcceptReceiver.join();
	}

	HRESULT CServer::Initialize(const CServer::Init_Data& tData)
	{
		if (tData.iPortNum == 0 || 
			tData.eProtocalType		== Net::PROTOCAL_TYPE::END ||
			tData.eNetProtocalType	== Net::NET_PROTOCOL_TYPE::END) return E_FAIL;


		/* TCP 소켓 생성 후 Bind 및 Listen */
		m_pSocket = Net::CSocket::Create(Net::TYPE::SERVER,tData.eProtocalType,tData.eNetProtocalType);
		if (m_pSocket == nullptr) return E_FAIL;


		m_pAddress = Net::CAddress::Create(Net::TYPE::SERVER,tData.eNetProtocalType,tData.iPortNum);
		if (m_pAddress == nullptr) return E_FAIL;


		/* Bind Listen 까지 진행 */
		if (Bind() == false) return E_FAIL;
		if (Listen() == false) return E_FAIL;


		m_thAcceptReceiver = std::thread(&CServer::Accept_Client, this);


		return S_OK;
	}

	void CServer::Add_Client(std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket)
	{
		if (pAddress == nullptr || pSocket == nullptr) return;

		/* Client 생성 요청 */
		std::unique_ptr<Net::CClient> pClient = Net::CClient::Create(std::move(pAddress), std::move(pSocket));
		if (pClient == nullptr)
		{
			/* 서버 Log에 기록하기*/

			// 연동 실패 몇번 뭐뭐 이렇게
			return;
		}


		/* Log 추가하기 *//*
		ConsoleUI::Add_Log(L"#" + std::to_wstring(pClient->Get_ClientKey()) + L"  IP [ " +
			ConsoleUI::Widen(pClient->Get_IP()) + L" ] Port [ " + std::to_wstring(pClient->Get_PortNum()) + L" ]  Client 접속");*/

		m_umapClients[pClient->Get_ClientKey()] = std::move(pClient);

		return;
	}

	bool CServer::Bind()
	{
		if (m_pSocket == nullptr || m_pSocket == nullptr) return false;

		return m_pSocket->Bind(m_pAddress.get());
	}
	bool CServer::Listen()
	{
		if (m_pSocket == nullptr || m_pAddress == nullptr) return false;

		return m_pSocket->Listen();
	}

	void CServer::Accept_Client()
	{
		if (m_pSocket == nullptr || m_pAddress == nullptr) return;

		/* 서버가 다운되기 전 까지 계속 돈다 */
		std::lock_guard<std::mutex> lock(m_mtxAcceptReceive);
		while (true)
		{
			std::unique_ptr<Net::CSocket> pClientSocket{ Net::CSocket::Create_Empty() };
			if (pClientSocket == nullptr) break;

			std::unique_ptr<Net::CAddress> pClientAddress{ Net::CAddress::Create_Empty() };
			if (pClientAddress == nullptr) break;

			/* Accepth 받기 */
			if (m_pSocket->Accept(pClientAddress.get(), pClientSocket.get()) == false) break;
			if (m_isAllowAccept == false) continue;

			Net::Event tEvent{};
			tEvent.eEventType = SERVER_EVENT_TYPE::Connect;
			tEvent.pAddress = std::move(pClientAddress);
			tEvent.pSocket = std::move(pClientSocket);
			m_ServerEventQueue.Push_Event(std::move(tEvent));
		}
	}

	void CServer::Pre_Update()
	{
		/* Event Queue 처리하기 */
		Handle_ServerEventQueue();

	}
	void CServer::Update()
	{
	}
	void CServer::Late_Update()
	{
	}

	void CServer::Set_Accept(bool is)
	{
		if (m_isAllowAccept == is) return;

		m_isAllowAccept = is;
	}

	uint16_t CServer::Get_PortNum() const
	{
		return m_pSocket ? m_pAddress->Get_PortNum() : 0;
	}

	std::string CServer::Get_IP() const
	{
		return m_pSocket ? m_pAddress->Get_IP() : std::string();
	}

	void CServer::Handle_ServerEventQueue()
	{
		for (;;)
		{
			Event tEvent{};
			if (m_ServerEventQueue.Pop_Event(tEvent) == false) break;
			switch (tEvent.eEventType)
			{
			case Net::SERVER_EVENT_TYPE::Connect:			Handle_Connect(std::move(tEvent)); break;
			case Net::SERVER_EVENT_TYPE::Receive_Packet:	break;
			case Net::SERVER_EVENT_TYPE::Send_Packet:		break;
			case Net::SERVER_EVENT_TYPE::Disconnect:		break;
			default: break;
			}
		}
	}

	void CServer::Handle_Connect(Net::Event&& tEvent)
	{
		/* 현재 Off인제 On인지 판단 */
		if (m_isAllowAccept == false) return;

		Add_Client(std::move(tEvent.pAddress), std::move(tEvent.pSocket));
	}

	std::unique_ptr<CServer> CServer::Create(const CServer::Init_Data& tData)
	{
		std::unique_ptr<CServer> pServer{ new CServer() };

		/* 지역변수 처럼 써도되네 */
		if (FAILED(pServer->Initialize(tData)))
			return nullptr;

		return pServer;
	}
}