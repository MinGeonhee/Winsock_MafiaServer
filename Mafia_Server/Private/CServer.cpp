#include "pch.h"
#include "CServer.h"
#include "CClient.h"

namespace MafiaServer
{
	CServer::CServer()
		: m_pSocket_TCPServer{ nullptr }, m_pAdrress_Server{ nullptr } , m_isOpenServer{false}
		, m_thAcceptReceiver{} , m_mtxAcceptReceive{} , m_isAllowAccept{false}
	{
	}
	CServer::~CServer()
	{
		Net::Net_Release();
		m_isAllowAccept = false;
		m_pSocket_TCPServer.reset();
		m_pAdrress_Server.reset();
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
		while (m_isAllowAccept)
		{
			std::unique_ptr<Net::CSocket> pClientSocket{ Net::CSocket::Create_Empty() };
			if (pClientSocket == nullptr) break;

			std::unique_ptr<Net::CAddress> pClientAddress{ Net::CAddress::Create_Empty() };
			if (pClientAddress == nullptr) break;

			/* Accepth 받기 */
			if (m_pSocket_TCPServer->Accept(pClientAddress.get(), pClientSocket.get()) == false) break;
			
			/* Client 생성 요청 */
			Add_Client(std::move(pClientAddress),std::move(pClientSocket));
		}
	}

	void CServer::Pre_Update()
	{
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


		if (m_isAllowAccept == true)
			m_thAcceptReceiver = std::thread(&CServer::Accept_Client,this);
	}
}