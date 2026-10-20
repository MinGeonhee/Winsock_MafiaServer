#pragma once


namespace Net
{
	class CClient;
	class CServerEventQueue;

	class CServer
	{
	public:
		struct  Init_Data
		{
			uint16_t			iPortNum{0};									/* 필수 기입 */
			PROTOCAL_TYPE		eProtocalType{Net::PROTOCAL_TYPE::TCP};
			NET_PROTOCOL_TYPE	eNetProtocalType{Net::NET_PROTOCOL_TYPE::IPv4};
		};
	private:
		explicit CServer();
	public:
		virtual ~CServer();
	private:
		HRESULT Initialize(const CServer::Init_Data& tData);
	private:
		ClientKey	Add_Client(std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket);	/* 실패 = NONE_CLIENTKEY */
		void		Unconnect_Client(ClientKey iKey);
	private:
		bool	Bind();
		bool	Listen();
	private:
		void	Accept_Client();
		void	Handle_RecvEvents();
		void	Handle_Connect(Net::Event&& tEvent);
		void	Handle_RejectConnect(Net::Event&& tEvent);
		void	Handle_Receive_Packet(Net::Event&& tEvent);
		void	Handle_Disconnect(Net::Event&& tEvent);
		void	Handle_SendEvents();
		void	Call_Event(const Net::Event& tEvent);
		void	Add_Logs(NET_LOG_TYPE eType, ClientKey iKey, std::string strIP, uint16_t iPortNum);

		/* NetLib 내부 패킷 보내기 (isNet = 1). private이라 게임 쪽에서는 못 부름 */
		bool	Send_NetPacket(ClientKey iKey, uint16_t usType, const void* pBody, uint16_t usLen);
		bool	Push_SendEvent(ClientKey iKey, std::vector<char>&& vecPacket);

	public:
		void	Clear_CallBackNetEvent();
		void	Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE eType, NetEventFuntion callback);
		void	Resigster_CallBackNetEvent(NetEventFuntion ConnectCallBack, NetEventFuntion RejectConnectCallBack,
										   NetEventFuntion ReceivePacketCallBack,NetEventFuntion SendPacketCallBack, NetEventFuntion DisconnectCallBack);

		/* 게임 쪽 : iKey 클라에게 보내기 요청 (Late_Update에서 실제 전송) */
		bool	Send_Packet(ClientKey iKey, uint16_t usType, const void* pBody, uint16_t usLen);
		template<class T>
		bool	Send_Packet(ClientKey iKey, uint16_t usType, const T& tBody) { return Send_Packet(iKey, usType, &tBody, static_cast<uint16_t>(sizeof(T))); }

	public:
		/* Pre_Update = 받기 큐 처리 (접속 / 거절 / 수신 / 끊김), Late_Update = 모아둔 Send 전송 */
		void	Pre_Update();
		void	Update();
		void	Late_Update();
	public:
		void	Set_Accept(bool is);
		bool	Is_Accept() const { return m_isAllowAccept; }
	public:
		const std::deque<Net::NetLog>&		Get_Logs() const { return m_dequeLogs; }
		const CClient*						Get_Client(ClientKey iKey) const;
		const std::vector<ClientKey>&		Get_ClientKeys() const { return m_vecClientOrder; }
		uint16_t							Get_PortNum() const;
		std::string							Get_IP() const;
	public:
		static std::unique_ptr<CServer> Create(const CServer::Init_Data& tData);
	private:
		/* 우선 TCP 소켓 및 Address 관리 */
		bool							m_isOpenServer;
		std::atomic<bool>				m_isAllowAccept;

		std::thread						m_thAcceptReceiver;
		std::unique_ptr<Net::CSocket>	m_pSocket;
		std::unique_ptr<Net::CAddress>	m_pAddress;


		/* 서버 Evnet Queue 관리 */
		Net::CEventQueue<Net::Event>	m_RecvEventQueue;		/* 받기 큐 : Accept·수신 스레드 → 로직 스레드 (mutex) */
		std::vector<Net::Event>			m_vecSendEvents;		/* 보내기 목록 : 로직 스레드만 사용 (락 없음) */
		std::array < Net::NetEventFuntion, static_cast<int>(Net::NET_EVENT_TYPE::END)> m_arrayServerEvents;


		/* Client 관리 */
		ClientKey m_iClientKey;
		std::unordered_map<ClientKey, std::unique_ptr<CClient>> m_umapClients;
		std::vector<ClientKey>									m_vecClientOrder;

		/* Log관리용 */
		std::deque<Net::NetLog>		  m_dequeLogs;
	};
}