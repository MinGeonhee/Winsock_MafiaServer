#pragma once


namespace Net
{
	class CClient;
}


namespace MafiaClient
{
	class CScene;

	class CClientApp
	{
	public:
		struct Init_Data
		{
			Net::CClient::Init_Data tClientData{};		/* 로그인 화면의 기본값으로 사용 (IP, 포트) */
		};
	private:
		explicit CClientApp();
		virtual ~CClientApp();
		CClientApp(const CClientApp& cCopy) = delete;
		CClientApp(CClientApp&& cMove) = delete;
	public:
		static CClientApp* Get_Instance()
		{
			if (m_pInstance == nullptr)
			{
				m_pInstance = new CClientApp();
			}
			return m_pInstance;
		}
		void	Destory();
		HRESULT Initialize(const Init_Data& tData);
	public:
		void	Pre_Update();
		void	Update();
		void	Late_Update();
		void	Render();
	public:
		/* ---- Scene에서 사용 ---- */
		void	Change_Scene(SCENE eScene) { m_eNextScene = eScene; }	/* 바로 바꾸지 않고 요청만 저장 → Late_Update에서 교체 */
		void	Quit() { m_isRunning = false; }
		bool	Is_Running() const { return m_isRunning; }

		/* 로그인 성공: 연결과 내 정보 보관 */
		void	Set_Connection(std::unique_ptr<Net::CClient> pClient, const std::wstring& strNickname, const std::wstring& strServer);
		void	Disconnect();

		Net::CClient*			Get_Client() const { return m_pClient.get(); }
		const std::wstring&		Get_Nickname() const { return m_strNickname; }
		const std::wstring&		Get_ServerText() const { return m_strServer; }
		const Init_Data&		Get_InitData() const { return m_tInitData; }

	private:
		/* ---- NetLib 콜백 (m_pClient->Pre_Update 안에서 불림 = 로직 스레드) ---- */
		void	Register_NetEvents();
		void	On_ReceivePacket(const Net::Event& tEvent);
		void	On_SendPacket(const Net::Event& tEvent);
		void	On_Disconnect(const Net::Event& tEvent);
	private:
		std::unique_ptr<CScene>	Create_Scene(SCENE eScene);
		void					Apply_SceneChange();
	private:
		inline static CClientApp* m_pInstance{ nullptr };
	private:
		Init_Data					m_tInitData;
		bool						m_isRunning;

		/* Scene */
		std::unique_ptr<CScene>		m_pScene;
		MafiaClient::SCENE			m_eCurScene;
		MafiaClient::SCENE			m_eNextScene;

		/* 서버 연결 (나) */
		std::unique_ptr<Net::CClient>	m_pClient;
		bool							m_isLostConnection{ false };	/* 끊김 표시 → Pre_Update 끝에서 정리 */
		std::wstring					m_strNickname;
		std::wstring					m_strServer;		/* "127.0.0.1:1234" 화면 표시용 */
	};
}
