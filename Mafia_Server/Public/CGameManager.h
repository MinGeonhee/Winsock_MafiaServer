#pragma once


namespace MafiaServer
{
	class CServer;

	class CGameManager
	{
	public:
		struct Init_Data
		{
			int iUsingPortNumber{ 0 }; // 서버를 여는 사람이 사용할 포트번호
		};
	private:
		explicit CGameManager();
		virtual ~CGameManager();
		CGameManager(const CGameManager& cCopy) = delete;
		CGameManager(CGameManager&& cMove) = delete;
	public:
		static CGameManager* Get_Instance()
		{
			if (m_pInstance == nullptr)
			{
				m_pInstance = new CGameManager();
			}
			return m_pInstance;
		}
		void	Destory();
		/* 기본으로 TCP 소켓 생성 */
		HRESULT Initialize(const Init_Data& tData);
	private:
		/* Accept 요청 받기 */
		void	Start_Accept();
	public:
		/* 서버로 들어오는 요청 받기 */
		void	Pre_Update();
		void	Update();
		void	Late_Update();
		void	Render();
	private:
		inline static CGameManager* m_pInstance{ nullptr };
	private:
		/* 서버 관리 */
		CServer* m_pMainServer;
	};
}

