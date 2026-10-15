#include "pch.h"
#include "CServer.h"
#include "CClient.h"


namespace MafiaServer
{
	CServer::CServer()
		: m_pSocket_TCPServer{ nullptr }, m_pAdrress_Server{ nullptr } , m_isOpenServer{false}
		, m_thAcceptReceiver{} , m_mtxAcceptReceive{} , m_isAllowAccept{false}, m_iClientKey{1}
		, m_ServerEventQueue{}
	{
	}
	CServer::~CServer()
	{
		Net::Net_Release();
		m_isAllowAccept = false;
		m_pSocket_TCPServer.reset();
		m_pAdrress_Server.reset();

		if (m_thAcceptReceiver.joinable())
			m_thAcceptReceiver.join();
	}

	HRESULT CServer::Initialize(int iPortNumber)
	{
		/* Net 초기화 해주기 */
		if (Net::Net_Init() == false)
			return E_FAIL;

		/* TCP 소켓 생성 후 Bind 및 Listen */
		m_pSocket_TCPServer = Net::CSocket::Create(Net::TYPE::SERVER, Net::PROTOCAL_TYPE::TPC, Net::NET_PROTOCOL_TYPE::IPv4);
		if (m_pSocket_TCPServer == nullptr) return E_FAIL;


		m_pAdrress_Server = Net::CAddress::Create(Net::TYPE::SERVER, Net::NET_PROTOCOL_TYPE::IPv4, iPortNumber);
		if (m_pAdrress_Server == nullptr) return E_FAIL;
		

		/* Bind Listen 까지 진행 */
		if (Bind()==false) return E_FAIL;
		if (Listen()==false) return E_FAIL;

		
		m_thAcceptReceiver = std::thread(&CServer::Accept_Client,this);

		return S_OK;
	}

	void CServer::Add_Client(std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket)
	{
		if (pAddress == nullptr || pSocket == nullptr) return;

		/* Client 생성 요청 */
		std::unique_ptr<CClient> pClient = CClient::Create(m_iClientKey, std::move(pAddress), std::move(pSocket));
		if (pClient == nullptr)
		{
			/* 서버 Log에 기록하기*/

			// 연동 실패 몇번 뭐뭐 이렇게
			return;
		}


		/* Log 추가하기 */
		ConsoleUI::Add_Log(L"#" + std::to_wstring(pClient->Get_ClientKey()) + L"  IP [ " +
			ConsoleUI::Widen(pClient->Get_IP()) + L" ] Port [ " + std::to_wstring(pClient->Get_PortNum()) + L" ]  Client 접속");

		m_umapClients[m_iClientKey++] = std::move(pClient);


		return;
	}

	bool CServer::Bind()
	{
		if (m_pSocket_TCPServer == nullptr || m_pAdrress_Server == nullptr) return false;

		return m_pSocket_TCPServer->Bind(m_pAdrress_Server.get());
	}
	bool CServer::Listen()
	{
		if (m_pSocket_TCPServer == nullptr || m_pAdrress_Server == nullptr) return false;

		return m_pSocket_TCPServer->Listen();
	}

	void CServer::Accept_Client()
	{
		if (m_pAdrress_Server == nullptr || m_pSocket_TCPServer == nullptr) return;

		/* 서버가 다운되기 전 까지 계속 돈다 */
		std::lock_guard<std::mutex> lock(m_mtxAcceptReceive);
		while (true)
		{
			std::unique_ptr<Net::CSocket> pClientSocket{ Net::CSocket::Create_Empty() };
			if (pClientSocket == nullptr) break;

			std::unique_ptr<Net::CAddress> pClientAddress{ Net::CAddress::Create_Empty() };
			if (pClientAddress == nullptr) break;

			/* Accepth 받기 */
			if (m_pSocket_TCPServer->Accept(pClientAddress.get(), pClientSocket.get()) == false) break;
			if (m_isAllowAccept == false) continue;

			MafiaServer::ServerEvent tEvent{};
			tEvent.eEventType = Event_Type::Connect;
			tEvent.pAddress = std::move(pClientAddress);
			tEvent.pSocket  = std::move(pClientSocket);
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

	std::vector<ConsoleUI::ClientView> CServer::Get_ClientView()
	{
		std::vector<ConsoleUI::ClientView> vec{};
		vec.reserve(m_umapClients.size());

		ConsoleUI::ClientView tView{};
		for (auto& Pair : m_umapClients)
		{
			tView.iKey = Pair.second->Get_ClientKey();
			tView.iPort = Pair.second->Get_PortNum();
			tView.strIP = Pair.second->Get_IP();
			vec.push_back(tView);
		}

		/* 들어온 Key 순서대로 */
		std::sort(vec.begin(), vec.end(), [](const ConsoleUI::ClientView& A, const ConsoleUI::ClientView& B){
			return A.iKey < B.iKey;
		});

		return vec;
	}

	uint16_t CServer::Get_PortNum() const
	{
		return m_pAdrress_Server ? m_pAdrress_Server->Get_PortNum() : 0;
	}

	std::string MafiaServer::CServer::Get_IP() const
	{
		return m_pAdrress_Server ? m_pAdrress_Server->Get_IP() : std::string() ;
	}


	void CServer::Handle_ServerEventQueue()
	{
		for (;;)
		{
			ServerEvent tEvent{};
			if (m_ServerEventQueue.Pop_Event(tEvent) == false) break;
			switch (tEvent.eEventType)
			{
			case MafiaServer::Event_Type::Connect:			Handle_Connect(std::move(tEvent)); break;
			case MafiaServer::Event_Type::Receive_Packet:	break;
			case MafiaServer::Event_Type::Send_Packet:		break;
			case MafiaServer::Event_Type::Disconnect:		break;
			default: break;
			}
		}
	}

	void CServer::Handle_Connect(MafiaServer::ServerEvent&& tEvent)
	{
		/* 현재 Off인제 On인지 판단 */
		if (m_isAllowAccept == false) return;

		Add_Client(std::move(tEvent.pAddress), std::move(tEvent.pSocket));
	}
}