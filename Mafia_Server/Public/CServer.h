#pragma once

#include "ConsoleUI.h"

namespace Net
{
	class CSocket;
}

namespace MafiaServer
{
	class CClient;
	class CServerEventQueue;

	class CServer
	{
	private:
		explicit CServer();
		virtual ~CServer();
		CServer(const CServer& cCopy) = delete;
		CServer(CServer&& cMove) = delete;
		const CServer& operator=(const CServer& cCopy) = delete;
		const CServer& operator&&(const CServer& cMove) = delete;
	public:
		static CServer* Get_Instance()
		{
			if (m_pInstance == nullptr)
			{
				m_pInstance = new CServer();
			}
			return m_pInstance;
		}
		void	Destory()
		{
			if (m_pInstance) delete m_pInstance;
			m_pInstance = nullptr;
		}
		/* 기본으로 TCP 소켓 생성 */
		HRESULT Initialize(int iPortNumber);
	private:
		void	Add_Client(std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket);
	private:
		bool	Bind();
		bool	Listen();
	private:
		void	Accept_Client();
		void	Handle_ServerEventQueue();
		void	Handle_Connect(MafiaServer::ServerEvent&& tEvent);
	public:
		/* 서버로 들어오는 요청 받기 */
		void	Pre_Update();
		void	Update();
		void	Late_Update();
	public:
		void	Set_Accept(bool is);
		bool	Is_Accept() const { return m_isAllowAccept; }
	public:
		std::vector<ConsoleUI::ClientView> Get_ClientView();
		uint16_t		Get_PortNum() const;
		std::string		Get_IP() const;
	private:
		inline static CServer* m_pInstance{ nullptr };
	private:
		/* 우선 TCP 소켓 및 Address 관리 */
		bool							m_isOpenServer;
		std::atomic<bool>				m_isAllowAccept;

		std::thread						m_thAcceptReceiver;
		std::mutex						m_mtxAcceptReceive;
		std::unique_ptr<Net::CSocket>	m_pSocket_TCPServer;
		std::unique_ptr<Net::CAddress>	m_pAdrress_Server;


		/* 서버 Evnet Queue 관리 */
		Net::CEventQueue<MafiaServer::ServerEvent> m_ServerEventQueue;

		/* Client 관리 */
		ClientKey m_iClientKey{ 0 };
		std::unordered_map<ClientKey, std::unique_ptr<CClient>> m_umapClients;
	};
}