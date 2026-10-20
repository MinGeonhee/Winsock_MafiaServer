#pragma once
#include "CPlayerManager.h"

namespace Net
{
	class CServer;
}

namespace MafiaServer
{
	class CServerApp
	{
	public:
		struct Init_Data
		{
			Net::CServer::Init_Data		tServer_InitData{};
			CPlayerManager::Init_Data	tPlayerManager_InitData{};
		};
	private:
		explicit CServerApp();
		virtual ~CServerApp();
		CServerApp(const CServerApp& cCopy) = delete;
		CServerApp(CServerApp&& cMove) = delete;
	public:
		static CServerApp* Get_Instance()
		{
			if (m_pInstance == nullptr)
			{
				m_pInstance = new CServerApp();
			}
			return m_pInstance;
		}
		void	Destory();
		/* 기본으로 TCP 소켓 생성 */
		HRESULT Initialize(const Init_Data& tData);
	public:
		/* 서버로 들어오는 요청 받기 */
		void	Pre_Update();
		void	Update();
		void	Late_Update();
		void	Render();
	private:
		void	Key_Input();
		void	Allow_Accept();
	private:
		/* ---- NetLib 콜백 (m_pMainServer->Pre_Update 안에서 불림 = 로직 스레드) ---- */
		void	Register_NetEvents();
		void	On_Connect(const Net::Event& tEvent);
		void	On_RejectConnect(const Net::Event& tEvent);
		void	On_ReceivePacket(const Net::Event& tEvent);
		void	On_SendPacket(const Net::Event& tEvent);
		void	On_Disconnect(const Net::Event& tEvent);
	private:
		void	Handle_RequestData(const Net::Event& tEvent);
	private:
		inline static CServerApp* m_pInstance{ nullptr };
	private:
		/* Player Manager */
		CPlayerManager*				 m_pPlayerManager;

		/* 서버 관리 */
		std::unique_ptr<Net::CServer> m_pMainServer;
		bool						  m_isDirty;	// 바뀐 게 있을 때만 다시 그림
	};
}

