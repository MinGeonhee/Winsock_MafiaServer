#pragma once

namespace Net
{
	/* 연결 1개 (서버·클라 공용)
	   - 서버 쪽 : 접속한 클라 1명. 수신 스레드가 받은 것을 CServer의 받기 큐에 넣음 (처리는 CServer가)
	   - 클라 쪽 : 서버와 연결된 나. 자기 받기 큐 / 보내기 목록 / 콜백을 직접 처리 */
	class CClient
	{
	public:
		struct Init_Data
		{
			const char*			szEnterServerIP{ Net::DEFAULT_SERVER_IP };
			uint16_t			iEnterServerPortNum{ 0 };
			PROTOCAL_TYPE		eProtocalType{ Net::PROTOCAL_TYPE::TCP };
			NET_PROTOCOL_TYPE	eNetProtocalType{ Net::NET_PROTOCOL_TYPE::IPv4 };
		};
	private:
		explicit CClient();
		CClient(const CClient& cCopy) = delete;
		CClient(CClient&& cMove) = delete;
	public:
		virtual ~CClient();
	private:
		HRESULT	Initialize(ClientKey iClientKey, std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket, CEventQueue<Event>* pRecvQueue);
		HRESULT	Initialize(const Init_Data& tData);

		/* 수신 스레드 : 헤더 → 본문 받아서 받기 큐에 Push. recv 실패(끊김)면 Disconnect Push 후 종료 */
		void	Receive_Loop();
		void	Close();

		/* 클라 쪽 전용 */
		void	Handle_RecvEvents();
		void	Handle_NetPacket(const Net::Event& tEvent);	/* NetLib 내부 패킷 처리 (키 할당 등) → 게임 쪽엔 안 넘김 */
		void	Handle_SendEvents();
		void	Call_Event(const Net::Event& tEvent);
	public:
		ClientKey	Get_ClientKey() const { return m_iKey; }
		uint16_t	Get_PortNum()const;
		std::string Get_IP() const;
	public:
		/* 클라 쪽 : Pre_Update = 받은 것 처리, Late_Update = 모아둔 Send 전송 */
		void		Pre_Update();
		void		Update();
		void		Late_Update();
		void		Render();
	public:
		void		Clear_CallBackNetEvent();
		void		Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE eType, NetEventFuntion callback);
		void		Resigster_CallBackNetEvent(NetEventFuntion ConnectCallBack, NetEventFuntion RejectConnectCallBack,
											   NetEventFuntion ReceivePacketCallBack, NetEventFuntion SendPacketCallBack, NetEventFuntion DisconnectCallBack);

		/* 클라 쪽 : 서버에게 보내기 요청 (Late_Update에서 실제 전송) */
		bool		Send_Packet(uint16_t usType, const void* pBody, uint16_t usLen);
		template<class T>
		bool		Send_Packet(uint16_t usType, const T& tBody) { return Send_Packet(usType, &tBody, static_cast<uint16_t>(sizeof(T))); }

		/* NetLib 내부용 : 실제 send (CServer::Late_Update / Handle_SendEvents에서 호출) */
		bool		Send(const std::vector<char>& vecPacket);
	public:
		/* 서버 쪽 : pRecvQueue = CServer의 받기 큐 */
		static	std::unique_ptr<CClient> Create(ClientKey iClientKey, std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket, CEventQueue<Event>* pRecvQueue);

		/* 실제 클라이언트에서 사용하는 클래스로서의 역할 (실패 = 접속 실패 → nullptr) */
		static	std::unique_ptr<CClient> Create(const Init_Data& tData);
	private:
		std::array < Net::NetEventFuntion, static_cast<int>(Net::NET_EVENT_TYPE::END) > m_arrayServerEvents;

		std::atomic<ClientKey>			m_iKey;
		std::unique_ptr<Net::CAddress>  m_pAddress; /* Client Address */
		std::unique_ptr<Net::CSocket>   m_pSocket;  /* Client Socket  */

		/* 수신 */
		std::thread						m_thReceiver;
		CEventQueue<Event>*				m_pRecvQueue;		/* 수신 스레드가 넣을 곳 (서버 쪽 = CServer 큐, 클라 쪽 = m_RecvEventQueue) */
		Net::CEventQueue<Net::Event>	m_RecvEventQueue;	/* 클라 쪽 받기 큐 (수신 스레드 → 로직 스레드, mutex) */

		/* 클라 쪽 보내기 목록 (로직 스레드만 사용 → 락 없음) */
		std::vector<Net::Event>			m_vecSendEvents;
	};
}
