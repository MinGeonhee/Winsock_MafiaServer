#pragma once
#include "CScene.h"
#include "ConsoleUI.h"


namespace MafiaClient
{
	class CScene_Lobby : public CScene
	{
	protected:
		CScene_Lobby();
	public:
		virtual ~CScene_Lobby();
	protected:
		HRESULT	Initialize() override;
		void	Register_NetEventCallBack() override;
	public:
		void	Enter() override;
		void	Exit() override;
		void	Pre_Update() override;
		void	Update() override;
		void	Late_Update() override;
		void	Render() override;
	private:
		void	Create_Room();
		void	Enter_Room();			/* 선택한 방에 입장 요청 → 응답 기다리기 */
		void	Start_WaitRoom(const std::wstring& strText);	/* "방 만드는 중" / "입장하는 중" 기다리기 시작 */
	private:
		void	On_Packet(const Net::Event& tEvent);
		void	Request_Infos();		/* 서버에 접속 중인 플레이어 목록 요청 (받은 목록은 비우고 새로 받음) */
		void	Apply_Infos();		/* 받은 LOBBY_PLAYER → 화면용 MemberView로 바꾸고 다시 그리기 */
	public:
		static std::unique_ptr<CScene_Lobby> Create();
	private:
		std::vector<Protocol::LOBBY_PLAYER>	m_vecPlayerInfos;	/* 서버에서 받은 목록 복사본 (요청할 때마다 비우고 새로 받음) */
		std::vector<Protocol::LOBBY_ROOM>	m_vecRoomInfos;	/* 서버에서 받은 목록 복사본 (요청할 때마다 비우고 새로 받음) */
		ConsoleUI::LobbyView				m_tView;			/* 화면에 그릴 로비 정보 (서버 데이터 오면 여기 채움) */
		bool								m_isDirty;
		ULONGLONG							m_ullLastRequest{ 0 };	/* 마지막으로 목록을 요청한 시각 (iLobbyListUpdateSecond마다 다시 요청) */

		/* 방 만들기 / 입장 응답 기다리는 중 : "방 만드는 중" / "입장하는 중" (로그인과 같은 방식) */
		std::wstring						m_strWaitText;
		static constexpr ULONGLONG			ROOM_WAIT_MIN_MS{ 1000 };		/* 응답이 빨라도 최소 이만큼은 보여줌 */
		static constexpr ULONGLONG			ROOM_WAIT_TIMEOUT_MS{ 5000 };	/* 응답이 없으면 로비로 */
		bool								m_isWaitingRoom{ false };
		bool								m_isEntered{ false };		/* S_TO_C_ENTER_ROOM 받음 */
		Protocol::S_TO_C_ENTER_ROOM			m_tEnterInfo{};				/* 받은 방 정보 → 방 Scene 만들 때 넘김 */
		ULONGLONG							m_ullWaitStart{ 0 };
	};
}
