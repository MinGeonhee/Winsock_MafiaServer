#pragma once


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
			Net::CServer::Init_Data tServerData{};
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
		inline static CServerApp* m_pInstance{ nullptr };
	private:
		/* 서버 관리 */
		std::unique_ptr<Net::CServer> m_pMainServer;
		bool	 m_isDirty;	// 바뀐 게 있을 때만 다시 그림
	};
}

