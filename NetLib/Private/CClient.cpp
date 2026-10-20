#include "pch.h"
#include "CClient.h"



namespace Net
{
	CClient::CClient()
		: m_pAddress{ nullptr }, m_pSocket{ nullptr }, m_iKey{ NONE_CLIENTKEY }, m_RecvEventQueue{}
		, m_arrayServerEvents{}, m_thReceiver{}, m_pRecvQueue{ nullptr }, m_vecSendEvents{}
	{
	}
	CClient::~CClient()
	{
		Close();
	}

	HRESULT CClient::Initialize(ClientKey iClientKey,std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket, CEventQueue<Event>* pRecvQueue)
	{
		m_pAddress = std::move(pAddress);
		m_pSocket = std::move(pSocket);

		if (m_pAddress == nullptr || m_pSocket == nullptr || pRecvQueue == nullptr) return E_FAIL;

		m_iKey = iClientKey;
		m_pRecvQueue = pRecvQueue;

		/* 수신 스레드 시작 (받은 것은 CServer 큐로) */
		m_thReceiver = std::thread(&CClient::Receive_Loop, this);

		return S_OK;
	}

	HRESULT CClient::Initialize(const Init_Data& tData)
	{
		if (tData.eNetProtocalType == NET_PROTOCOL_TYPE::END ||
			tData.eProtocalType == PROTOCAL_TYPE::END ||
			tData.iEnterServerPortNum == 0 ||
			tData.szEnterServerIP == nullptr ) return E_FAIL;

		/* TCP 소켓 생성 */
		m_pSocket = Net::CSocket::Create(Net::TYPE::CLIENT, tData.eProtocalType, tData.eNetProtocalType);
		if (m_pSocket == nullptr) return E_FAIL;

		/* Connect */
		std::unique_ptr<CAddress> pServerAddress = Net::CAddress::Create(Net::TYPE::CLIENT, tData.eNetProtocalType,tData.iEnterServerPortNum, tData.szEnterServerIP);
		if (pServerAddress == nullptr) return E_FAIL;

		/* Connect 연결 요청하기 */
		if (m_pSocket->Connect(pServerAddress.get()) == false)
			return E_FAIL;

		/* 내 주소값 채워주기 */
		m_pAddress = Net::CAddress::Create_Empty();
		if (m_pSocket->Get_LocalAddress(m_pAddress.get()) == false)
			return E_FAIL;

		/* 수신 스레드 시작 (받은 것은 내 큐로) */
		m_pRecvQueue = &m_RecvEventQueue;
		m_thReceiver = std::thread(&CClient::Receive_Loop, this);

		return S_OK;
	}

	void CClient::Receive_Loop()
	{
		/* PacketHeader는 pack(1)이라 char 버퍼에 바로 받아도 정렬 문제 없음 */
		std::array<char, MAX_PACKET_TOTAL_LEN> arrBuffer{};
		PacketHeader* pHeader = reinterpret_cast<PacketHeader*>(arrBuffer.data());

		for (;;)
		{
			/* 헤더 → 길이 검사 → 본문까지 다 받을 때까지 여기서 대기 (블로킹) */
			/* OK가 아니면(정상 종료 / 에러 / 잘못된 패킷) 전부 연결 종료로 처리 */
			if (Net::Receive_Packet(m_pSocket.get(), pHeader) != RECV_RESULT::OK)
				break;

			const size_t iLen = sizeof(PacketHeader) + pHeader->usLen;

			Net::Event tEvent{};
			tEvent.eEventType = NET_EVENT_TYPE::Receive_Packet;
			tEvent.iKey = m_iKey;
			tEvent.vecPacket.assign(arrBuffer.data(), arrBuffer.data() + iLen);
			m_pRecvQueue->Push_Event(std::move(tEvent));
		}

		/* 끊김 알림 후 스레드 종료 (정리는 로직 스레드가 함) */
		Net::Event tEvent{};
		tEvent.eEventType = NET_EVENT_TYPE::Disconnect;
		tEvent.iKey = m_iKey;
		m_pRecvQueue->Push_Event(std::move(tEvent));
	}

	void CClient::Close()
	{
		/* 소켓을 끊어서 recv에서 멈춰 있는 수신 스레드를 깨움 → 스레드가 스스로 끝남 → join
		   (수신 스레드가 자기 자신을 join하지 않도록, Close는 로직 스레드에서만 호출됨) */
		if (m_pSocket)
			shutdown(m_pSocket->Get_Socket(), SD_BOTH);

		if (m_thReceiver.joinable())
			m_thReceiver.join();
	}

	void CClient::Handle_RecvEvents()
	{
		for (;;)
		{
			Event tEvent{};
			if (m_RecvEventQueue.Pop_Event(tEvent) == false) break;

			/* NetLib 내부 패킷은 여기서 처리하고 끝 (게임 콜백으로 안 넘김) */
			if (tEvent.eEventType == NET_EVENT_TYPE::Receive_Packet && tEvent.Is_NetPacket())
			{
				Handle_NetPacket(tEvent);
				continue;
			}

			/* 게임 패킷 / 끊김은 게임 쪽에 알림 */
			Call_Event(tEvent);
		}
	}

	void CClient::Handle_NetPacket(const Net::Event& tEvent)
	{
		switch (static_cast<NET_PROTOCAL_TYPE>(tEvent.Get_PacketType()))
		{
		case NET_PROTOCAL_TYPE::Client_Login:
		{
			if (tEvent.Get_BodyLen() != sizeof(Client_Login)) break;	/* 크기 안 맞으면 버림 */

			Client_Login tBody{};
			memcpy(&tBody, tEvent.Get_Body(), sizeof(tBody));

			/* 서버가 정해준 키로 갱신 (수신 스레드도 m_iKey를 읽으므로 atomic) */
			m_iKey = tBody.iClientKey;

			/* 게임 쪽에는 "연결 준비 완료 (내 키 확정)"으로 알림 */
			Net::Event tConnect{};
			tConnect.eEventType = NET_EVENT_TYPE::Connect;
			tConnect.iKey = m_iKey;
			Call_Event(tConnect);
			break;
		}
		default: break;
		}
	}

	void CClient::Handle_SendEvents()
	{
		/* 콜백 안에서 또 Send_Packet을 부를 수 있으므로 지역으로 옮긴 뒤 처리 (도는 중에 vector가 바뀌지 않게) */
		std::vector<Net::Event> vecRequests;
		vecRequests.swap(m_vecSendEvents);

		for (const Net::Event& tEvent : vecRequests)
		{
			/* 실패하면 연결이 끊긴 것 → 수신 스레드가 Disconnect를 올려줌 */
			if (Send(tEvent.vecPacket))
				Call_Event(tEvent);
		}
	}

	void CClient::Call_Event(const Net::Event& tEvent)
	{
		const size_t iIndex = static_cast<size_t>(tEvent.eEventType);
		if (iIndex >= m_arrayServerEvents.size()) return;

		if (m_arrayServerEvents[iIndex])
			m_arrayServerEvents[iIndex](tEvent);
	}

	bool CClient::Send_Packet(uint16_t usType, const void* pBody, uint16_t usLen)
	{
		Net::Event tEvent{};
		tEvent.eEventType = NET_EVENT_TYPE::Send_Packet;
		tEvent.iKey = m_iKey;
		tEvent.vecPacket = Make_PacketBytes(0, usType, pBody, usLen);	/* 게임 패킷 : isNet = 0 */
		if (tEvent.vecPacket.empty()) return false;

		m_vecSendEvents.push_back(std::move(tEvent));
		return true;
	}

	bool CClient::Send(const std::vector<char>& vecPacket)
	{
		if (m_pSocket == nullptr || vecPacket.size() < sizeof(PacketHeader)) return false;

		return Net::Send_Packet(m_pSocket.get(), reinterpret_cast<const PacketHeader*>(vecPacket.data()));
	}


	std::unique_ptr<CClient> CClient::Create(ClientKey iClientKey,std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket, CEventQueue<Event>* pRecvQueue)
	{
		std::unique_ptr<CClient> pClient{ new CClient };

		if (FAILED(pClient->Initialize(iClientKey,std::move(pAddress), std::move(pSocket), pRecvQueue)))
			return nullptr;

		return pClient;
	}

	std::unique_ptr<CClient> CClient::Create(const Init_Data& tData)
	{
		std::unique_ptr<CClient> pClient{ new CClient };

		if (FAILED(pClient->Initialize(tData)))
			return nullptr;

		return pClient;
	}

	void CClient::Clear_CallBackNetEvent()
	{
		for (auto& Func : m_arrayServerEvents)	Func = nullptr;
	}

	void CClient::Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE eType, NetEventFuntion callback)
	{
		int iIndex = static_cast<int>(eType);
		if (iIndex < 0 || iIndex >= static_cast<int>(NET_EVENT_TYPE::END)) return;
		m_arrayServerEvents[iIndex] = nullptr;
		m_arrayServerEvents[iIndex] = std::move(callback);
	}

	void CClient::Resigster_CallBackNetEvent(NetEventFuntion ConnectCallBack, NetEventFuntion RejectConnectCallBack, NetEventFuntion ReceivePacketCallBack,
		NetEventFuntion SendPacketCallBack, NetEventFuntion DisconnectCallBack)
	{
		if (ConnectCallBack) m_arrayServerEvents[static_cast<int>(Net::NET_EVENT_TYPE::Connect)] = ConnectCallBack;
		if (RejectConnectCallBack) m_arrayServerEvents[static_cast<int>(Net::NET_EVENT_TYPE::Reject_Connect)] = RejectConnectCallBack;
		if (ReceivePacketCallBack) m_arrayServerEvents[static_cast<int>(Net::NET_EVENT_TYPE::Receive_Packet)] = ReceivePacketCallBack;
		if (SendPacketCallBack) m_arrayServerEvents[static_cast<int>(Net::NET_EVENT_TYPE::Send_Packet)] = SendPacketCallBack;
		if (DisconnectCallBack) m_arrayServerEvents[static_cast<int>(Net::NET_EVENT_TYPE::Disconnect)] = DisconnectCallBack;

		return;
	}


	uint16_t CClient::Get_PortNum() const
	{
		return m_pAddress ? m_pAddress->Get_PortNum() : 0;
	}


	std::string CClient::Get_IP() const
	{
		return m_pAddress ? m_pAddress->Get_IP() : std::string();
	}


	void CClient::Pre_Update()
	{
		Handle_RecvEvents();
	}

	void CClient::Update()
	{
	}

	void CClient::Late_Update()
	{
		Handle_SendEvents();
	}

	void CClient::Render()
	{

	}

}
