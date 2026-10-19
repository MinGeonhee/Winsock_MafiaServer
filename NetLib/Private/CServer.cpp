#include "pch.h"
#include "CServer.h"


namespace Net
{
	CServer::CServer()
		: m_pSocket{ nullptr }, m_pAddress{ nullptr }, m_isOpenServer{ false }
		, m_thAcceptReceiver{}, m_isAllowAccept{ true }, m_iClientKey{ 1 }
		, m_RecvEventQueue{}, m_vecSendEvents{}, m_dequeLogs{}, m_arrayServerEvents{}
	{

	}

	CServer::~CServer()
	{
		m_isAllowAccept = false;
		m_pSocket.reset();
		m_pAddress.reset();

		for (auto& Key : m_vecClientOrder)
			m_umapClients[Key].reset();

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

	ClientKey CServer::Add_Client(std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket)
	{
		if (pAddress == nullptr || pSocket == nullptr) return NONE_CLIENTKEY;

		/* Client 생성 요청 (수신 스레드가 받은 것은 서버 받기 큐로) */
		std::unique_ptr<Net::CClient> pClient = Net::CClient::Create(m_iClientKey++,std::move(pAddress), std::move(pSocket), &m_RecvEventQueue);
		if (pClient == nullptr) return NONE_CLIENTKEY;

		const ClientKey iKey = pClient->Get_ClientKey();	/* move 하기 전에 꺼내 두기 */
		Add_Logs(NET_LOG_TYPE::Connect, iKey, pClient->Get_IP(), pClient->Get_PortNum());

		m_vecClientOrder.push_back(iKey);
		m_umapClients[iKey] = std::move(pClient);


		/* 연결 된 이후 클라이언트에 키 알려주기 (NetLib 내부 패킷 → 클라 NetLib이 받아서 자기 키로 저장) */
		Net::Client_Login tBody{};
		tBody.iClientKey = iKey;
		Send_NetPacket(iKey, static_cast<uint16_t>(Net::NET_PROTOCAL_TYPE::Client_Login), &tBody, static_cast<uint16_t>(sizeof(tBody)));

		return iKey;
	}

	void CServer::Unconnect_Client(ClientKey iKey)
	{
		if (m_umapClients.find(iKey) == m_umapClients.end()) return;

		m_umapClients.erase(iKey);

		auto iter = std::find(m_vecClientOrder.begin(), m_vecClientOrder.end(), iKey);
		if (iter != m_vecClientOrder.end())
			m_vecClientOrder.erase(iter);
	}

	bool CServer::Bind()
	{
		if (m_pSocket == nullptr || m_pAddress == nullptr) return false;

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
		while (true)
		{
			std::unique_ptr<Net::CSocket> pClientSocket{ Net::CSocket::Create_Empty() };
			if (pClientSocket == nullptr) break;

			std::unique_ptr<Net::CAddress> pClientAddress{ Net::CAddress::Create_Empty() };
			if (pClientAddress == nullptr) break;

			/* Accepth 받기 */
			if (m_pSocket->Accept(pClientAddress.get(), pClientSocket.get()) == false) break;

			Net::Event tEvent{};
			tEvent.eEventType = m_isAllowAccept == true ? NET_EVENT_TYPE::Connect : NET_EVENT_TYPE::Reject_Connect;
			tEvent.pAddress = std::move(pClientAddress);
			tEvent.pSocket = std::move(pClientSocket);
			m_RecvEventQueue.Push_Event(std::move(tEvent));
		}
	}

	void CServer::Add_Logs(NET_LOG_TYPE eType, ClientKey iKey, std::string strIP, uint16_t iPortNum)
	{
		NetLog tLog{};
		tLog.eType = eType;
		tLog.iKey = iKey;
		tLog.strIP = strIP;
		tLog.iPortNum = iPortNum;
		GetLocalTime(&tLog.tTime);

		m_dequeLogs.push_back(std::move(tLog));
		if (m_dequeLogs.size() > 100)
			m_dequeLogs.pop_front();
	}

	bool CServer::Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE eType, NetEventFuntion callback)
	{
		int iIndex = static_cast<int>(eType);
		if (iIndex < 0 || iIndex >= static_cast<int>(NET_EVENT_TYPE::END)) return false;
		if (m_arrayServerEvents[iIndex]) return false;

		m_arrayServerEvents[iIndex] = std::move(callback);

		return true;
	}

	void CServer::Pre_Update()
	{
		/* Event Queue 처리하기 */
		Handle_RecvEvents();
	}

	void CServer::Update()
	{
	}

	void CServer::Late_Update()
	{
		/* 이번 프레임에 쌓인 보내기 요청 전송 */
		Handle_SendEvents();
	}

	bool CServer::Send_Packet(ClientKey iKey, uint16_t usType, const void* pBody, uint16_t usLen)
	{
		/* 게임 패킷 : isNet = 0 */
		return Push_SendEvent(iKey, Make_PacketBytes(0, usType, pBody, usLen));
	}

	bool CServer::Send_NetPacket(ClientKey iKey, uint16_t usType, const void* pBody, uint16_t usLen)
	{
		/* NetLib 내부 패킷 : isNet = 1 */
		return Push_SendEvent(iKey, Make_PacketBytes(1, usType, pBody, usLen));
	}

	bool CServer::Push_SendEvent(ClientKey iKey, std::vector<char>&& vecPacket)
	{
		if (vecPacket.empty()) return false;	/* 본문이 너무 큼 */

		Net::Event tEvent{};
		tEvent.eEventType = NET_EVENT_TYPE::Send_Packet;
		tEvent.iKey = iKey;
		tEvent.vecPacket = std::move(vecPacket);

		m_vecSendEvents.push_back(std::move(tEvent));
		return true;
	}

	void CServer::Handle_SendEvents()
	{
		std::vector<Net::Event> vecRequests;
		vecRequests.swap(m_vecSendEvents);

		for (const Net::Event& tEvent : vecRequests)
		{
			auto iter = m_umapClients.find(tEvent.iKey);
			if (iter == m_umapClients.end()) continue;		/* 그 사이 나간 클라 */

			/* 실패하면 연결이 끊긴 것 → 그 클라의 수신 스레드가 Disconnect를 올려줌 */
			if (iter->second->Send(tEvent.vecPacket))
				Call_Event(tEvent);
		}
	}

	void CServer::Call_Event(const Net::Event& tEvent)
	{
		const size_t iIndex = static_cast<size_t>(tEvent.eEventType);
		if (iIndex >= m_arrayServerEvents.size()) return;

		if (m_arrayServerEvents[iIndex])
			m_arrayServerEvents[iIndex](tEvent);
	}

	void CServer::Set_Accept(bool is)
	{
		if (m_isAllowAccept == is) return;

		m_isAllowAccept = is;
	}

	const CClient* CServer::Get_Client(ClientKey iKey) const
	{
		const auto& iter = m_umapClients.find(iKey);
		if (iter == m_umapClients.end()) return nullptr;
		return iter->second.get();
	}

	uint16_t CServer::Get_PortNum() const
	{
		return m_pSocket ? m_pAddress->Get_PortNum() : 0;
	}

	std::string CServer::Get_IP() const
	{
		return m_pSocket ? m_pAddress->Get_IP() : std::string();
	}

	void CServer::Handle_RecvEvents()
	{
		for (;;)
		{
			Event tEvent{};
			if (m_RecvEventQueue.Pop_Event(tEvent) == false) break;
			switch (tEvent.eEventType)
			{
			case Net::NET_EVENT_TYPE::Connect:			Handle_Connect(std::move(tEvent));			break;
			case Net::NET_EVENT_TYPE::Reject_Connect:	Handle_RejectConnect(std::move(tEvent));	break;
			case Net::NET_EVENT_TYPE::Receive_Packet:	Handle_Receive_Packet(std::move(tEvent));	break;
			case Net::NET_EVENT_TYPE::Disconnect:		Handle_Disconnect(std::move(tEvent));		break;
			default: break;	/* Send_Packet은 이 큐에 들어오지 않음 (보내기 목록 → Late_Update) */
			}
		}
	}

	void CServer::Handle_Connect(Net::Event&& tEvent)
	{
		if (m_isAllowAccept == false) return;

		/* NetLib 일 먼저 (소켓 소유권은 여기서 CClient로 넘어감 → 콜백에는 키만 남음) */
		tEvent.iKey = Add_Client(std::move(tEvent.pAddress), std::move(tEvent.pSocket));
		if (tEvent.iKey == NONE_CLIENTKEY) return;

		Call_Event(tEvent);
	}

	void CServer::Handle_RejectConnect(Net::Event&& tEvent)
	{
		if (tEvent.pAddress)
			Add_Logs(Net::NET_LOG_TYPE::Reject, NONE_CLIENTKEY, tEvent.pAddress->Get_IP(), tEvent.pAddress->Get_PortNum());

		/* 소켓은 Event가 사라질 때 닫힘 */
		tEvent.pSocket.reset();
		tEvent.pAddress.reset();
		Call_Event(tEvent);
	}

	void CServer::Handle_Receive_Packet(Net::Event&& tEvent)
	{
		/* 이미 정리된 클라의 늦게 도착한 패킷은 버림 */
		if (m_umapClients.find(tEvent.iKey) == m_umapClients.end()) return;

		/* 클라 → 서버 NetLib 내부 패킷은 아직 없음 → 클라가 isNet을 켜서 보낸 건 버림 (게임 쪽에 안 넘김) */
		if (tEvent.Is_NetPacket()) return;

		Call_Event(tEvent);
	}

	void CServer::Handle_Disconnect(Net::Event&& tEvent)
	{
		auto iter = m_umapClients.find(tEvent.iKey);
		if (iter == m_umapClients.end()) return;

		Add_Logs(Net::NET_LOG_TYPE::Disconnect, tEvent.iKey, iter->second->Get_IP(), iter->second->Get_PortNum());

		/* CClient 소멸자에서 수신 스레드 join (스레드는 이미 끝나는 중이라 바로 돌아옴) */
		Unconnect_Client(tEvent.iKey);

		Call_Event(tEvent);
	}

	std::unique_ptr<CServer> CServer::Create(const CServer::Init_Data& tData)
	{
		std::unique_ptr<CServer> pServer{ new CServer() };

		if (FAILED(pServer->Initialize(tData)))
			return nullptr;

		return pServer;
	}
}