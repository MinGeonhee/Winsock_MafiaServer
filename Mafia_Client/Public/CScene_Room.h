#pragma once
#include "CScene.h"
#include "ConsoleUI.h"


namespace MafiaClient
{
	class CScene_Room : public CScene
	{
		enum class STATE
		{
			WAITING,				/* 게임 시작 대기 */
			STARTGAME_COUNTDOWN,	/* 승인 완료 후 카운트 다운 */
			GAME,					/* 게임 중 */
			END,
		};
	protected:
		CScene_Room();
	public:
		virtual ~CScene_Room();
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
		/* 상태 바꾸기 : 패킷 받는 곳(On_Packet)에서만 부름. 화면 값(isWaiting / isPlaying)도 여기서 같이 */
		void	Set_State(STATE eState);

		/* 상태별 입력 (키는 Pre_Update에서 한 번만 읽어서 넘김) */
		void	Pre_Update_Waiting(const std::vector<KEY_EVENT_RECORD>& vecKeys);	/* F5(방장) · Esc · 채팅 */
		void	Pre_Update_Countdown(const std::vector<KEY_EVENT_RECORD>& vecKeys);	/* 채팅만 (못 나감) */
		void	Pre_Update_Game(const std::vector<KEY_EVENT_RECORD>& vecKeys);		/* 단계별 : 투표 / 찬반 / 채팅 (못 나감) */
		void	Edit_Chat(const KEY_EVENT_RECORD& tKey);		/* 채팅 입력 · 보내기 (막혀 있으면 무시) */
		void	Request_StartGame();							/* 방장만 */
		void	Exit_Room();
		void	Key_Vote(const KEY_EVENT_RECORD& tKey);			/* VOTE : 1 ~ 7 */
		void	Key_LastVote(const KEY_EVENT_RECORD& tKey);		/* LAST_VOTE : Y / N */
		std::vector<int> Vote_Candidates() const;				/* 투표 후보 : 살아 있는 사람 중 나 빼고 (시계 방향) + 마지막에 무효표 */
		bool	Is_Alive(int iSeat) const { return iSeat >= 0 && (m_iAliveMask >> iSeat) & 1; }

		/* 상태별 업데이트 */
		void	Update_Countdown();								/* 5, 4, 3 … 알림 */
		void	Update_Game();									/* 단계 남은 시간 */

		/* 단계별 화면 규칙 : 채팅 잠금 문구 · 조작 안내 (실제 차단은 서버도 함) */
		void	Apply_PhaseRule(Protocol::GAME_PHASE ePhase);
	private:
		void	Apply_RoomInfo();		/* m_tRoomInfo → 화면용 m_tView */
		void	On_Packet(const Net::Event& tEvent);
		void	Show_Notice(const std::wstring& strText);	/* 안내 줄에 빨간 알림 (NOTICE_MS 뒤 자동으로 사라짐) */
		void	Apply_Phase(Protocol::GAME_PHASE ePhase, uint16_t iDay, uint32_t iRemainMs);	/* 서버가 알려준 게임 단계 → 원탁 가운데 표시 */
		void	Update_PhaseTime();							/* 남은 시간 표시 (초가 바뀔 때만 다시 그림) */
	public:
		/* 입장 응답(S_TO_C_ENTER_ROOM)을 받아서 만듦 : 방 정보는 이 Scene이 들고 갱신함 */
		static std::unique_ptr<CScene_Room> Create(const Protocol::S_TO_C_ENTER_ROOM& tRoomInfo);
	private:
		STATE						m_eRoomState{STATE::WAITING};
		Protocol::S_TO_C_ENTER_ROOM	m_tRoomInfo{};
		static constexpr size_t		MAX_CHAT_LEN{ 60 };
		ConsoleUI::RoomScreenView	m_tView;
		bool						m_isDirty;

		static constexpr ULONGLONG	NOTICE_MS{ 3000 };
		ULONGLONG					m_ullNoticeEnd{ 0 };	/* 알림이 사라질 시각 */

		/* 게임 시작 카운트다운 (표시용 : 실제 시작 판정은 서버) */
		ULONGLONG					m_ullStartAt{ 0 };		/* 카운트다운이 끝나는 시각 (표시용) */
		int							m_iLastCount{ 0 };		/* 마지막으로 띄운 초 (같은 초를 두 번 띄우지 않게) */

		/* 게임 중 (서버가 알려준 값 그대로 : 클라는 판정하지 않음) */
		Protocol::JOB				m_eJob{ Protocol::JOB::UNKNOWN };		/* 내 직업 */
		Protocol::GAME_PHASE		m_ePhase{ Protocol::GAME_PHASE::END };	/* 지금 단계 */
		ULONGLONG					m_ullPhaseEnd{ 0 };		/* 이번 단계가 끝나는 시각 (내 시계 기준, 표시용) */
		int							m_iLastRemainSec{ -1 };	/* 마지막으로 그린 남은 초 */
		int							m_iMySeat{ -1 };		/* 내 자리 (0 ~ 6, Apply_RoomInfo에서) */
		int							m_iAccusedSeat{ -1 };	/* 투표로 지목된 자리 (최후 변론자, -1 = 없음) */
		uint8_t						m_iAliveMask{ 0x7F };	/* 비트 i = i번 자리 생존 (CHANGEPHASE로 갱신, 시작은 전원 생존) */
	};
}
