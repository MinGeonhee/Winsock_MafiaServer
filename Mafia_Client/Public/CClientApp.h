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
		/* 바로 바꾸지 않고 요청 큐에 쌓기만 함 → Render까지 끝난 프레임 맨 끝에서 교체 */
		/* anyArg : 그 Scene을 만들 때 넘길 데이터 (예: ROOM ← S_TO_C_ENTER_ROOM). 없으면 비워 둠 */
		void	Change_Scene(SCENE eScene, std::any anyArg = {}) { m_queueSceneRequests.push({ eScene, std::move(anyArg) }); }
		/* 콜백 안에서 연결을 끊고 싶을 때 : 표시만 하고, m_pClient->Pre_Update()가 끝난 뒤 App이 정리 */
		void	Request_Disconnect() { m_isLostConnection = true; }
		void	Quit() { m_isRunning = false; }
		bool	Is_Running() const { return m_isRunning; }

		/* 접속 성공 : 연결 보관 (CScene_Connect) / 로그인 성공 : 닉네임 보관 (CScene_Login) */
		void	Set_Connection(std::unique_ptr<Net::CClient> pClient, const std::wstring& strServer);
		void	Set_Nickname(const std::wstring& strNickname) { m_strNickname = strNickname; }
		void	Disconnect();

		Net::CClient*			Get_Client() const { return m_pClient.get(); }
		const std::wstring&		Get_Nickname() const { return m_strNickname; }
		const std::wstring&		Get_ServerText() const { return m_strServer; }
		const Init_Data&		Get_InitData() const { return m_tInitData; }

		/* ---- NetLib 콜백 (m_pClient->Pre_Update 안에서 불림 = 로직 스레드) ---- */
		void	Clear_NetEvents();
		void	On_ReceivePacket(const Net::Event& tEvent);
		void	On_SendPacket(const Net::Event& tEvent);
		void	On_Disconnect(const Net::Event& tEvent);
		void	Register_DefaultNetEvents();
		void	Register_NetEvents(Net::NET_EVENT_TYPE eType, Net::NetEventFuntion func);
	private:
		std::unique_ptr<CScene>	Create_Scene(SCENE eScene, const std::any& anyArg);
		void					Apply_SceneChange();
	private:
		inline static CClientApp* m_pInstance{ nullptr };
	private:
		Init_Data					m_tInitData;
		bool						m_isRunning;

		/* Scene */
		std::unique_ptr<CScene>		m_pScene;
		MafiaClient::SCENE			m_eCurScene;
		struct SceneRequest
		{
			MafiaClient::SCENE	eScene;
			std::any			anyArg;		/* Scene마다 필요한 데이터 (없으면 비어 있음) */
		};
		std::queue<SceneRequest>		m_queueSceneRequests;	/* 이번 프레임에 들어온 Scene 변경 요청 */

		/* 서버 연결 (나) */
		std::unique_ptr<Net::CClient>	m_pClient;
		bool							m_isLostConnection{ false };	/* 끊김 표시 → Pre_Update 끝에서 정리 */
		std::wstring					m_strNickname;
		std::wstring					m_strServer;		/* "127.0.0.1:1234" 화면 표시용 */	};
}
