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
		virtual ~CServer();
	private:
		HRESULT Initialize(const CServer::Init_Data& tData);
	private:
		void	Add_Client(std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket);
	private:
		bool	Bind();
		bool	Listen();
	private:
		void	Accept_Client();
		void	Handle_ServerEventQueue();
		void	Handle_Connect(Net::Event&& tEvent);
	public:
		/* 서버로 들어오는 요청 받기 */
		void	Pre_Update();
		void	Update();
		void	Late_Update();
	public:
		void	Set_Accept(bool is);
		bool	Is_Accept() const { return m_isAllowAccept; }
	public:
		uint16_t		Get_PortNum() const;
		std::string		Get_IP() const;
	public:
		std::unique_ptr<CServer> Create(const CServer::Init_Data& tData);
	private:
		/* 우선 TCP 소켓 및 Address 관리 */
		bool							m_isOpenServer;
		std::atomic<bool>				m_isAllowAccept;

		std::thread						m_thAcceptReceiver;
		std::mutex						m_mtxAcceptReceive;
		std::unique_ptr<Net::CSocket>	m_pSocket;
		std::unique_ptr<Net::CAddress>	m_pAddress;


		/* 서버 Evnet Queue 관리 */
		Net::CEventQueue<Net::Event> m_ServerEventQueue;

		/* Client 관리 */
		ClientKey m_iClientKey{ 0 };
		std::unordered_map<ClientKey, std::unique_ptr<CClient>> m_umapClients;
	};
}