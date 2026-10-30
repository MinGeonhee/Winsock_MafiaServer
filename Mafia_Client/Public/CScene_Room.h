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
			RESULT,					/* 게임 끝 : 승리 연출 → 결과 창 (Enter → 대기) */
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
		void	Pre_Update_Result(const std::vector<KEY_EVENT_RECORD>& vecKeys);	/* 결과 창이 뜬 뒤 Enter → 대기 */
		void	Edit_Chat(const KEY_EVENT_RECORD& tKey);		/* 채팅 입력 · 보내기 (막혀 있으면 무시) */
		void	Request_StartGame();							/* 방장만 */
		void	Exit_Room();
		void	Key_Vote(const KEY_EVENT_RECORD& tKey);			/* VOTE : 1 ~ 7 */
		void	Key_LastVote(const KEY_EVENT_RECORD& tKey);		/* LAST_VOTE : Y / N */
		std::vector<int> Vote_Candidates() const;				/* 투표 후보 : 살아 있는 사람 중 나 빼고 (시계 방향) + 마지막에 무효표 */
		bool	Key_Pick(const KEY_EVENT_RECORD& tKey, const std::vector<int>& vecCand);	/* 막대 고르기 공용 (←→ · 숫자) : Enter로 확정하면 true */

		/* 밤 능력 : 고르기 · 보내기만 (판정은 서버). [Tab] 으로 모드 바꿈 (마피아 : 처치 ↔ 채팅) */
		enum class NIGHT_MODE { KILL, SNOOP, GUARD, MEDIUM, FIND_JOB, CONTACT, NOTICE, CHAT };
		std::vector<NIGHT_MODE> Night_Modes() const;			/* 지금 쓸 수 있는 모드 (직업 · 이번 밤 사용 여부) */
		std::vector<int>		Night_Candidates() const;		/* 지금 모드로 고를 수 있는 자리 */
		void	Key_Night(const KEY_EVENT_RECORD& tKey);
		void	Apply_NightMode(NIGHT_MODE eMode);				/* 막대 / 입력 줄 / 안내 바꾸기 */
		void	Send_Skill();									/* 고른 자리로 C_TO_S_JOB_ACTION */
		void	On_JobData(const Net::Event& tEvent, Protocol::DATA_TYPE eType);	/* 능력 결과 · 대통령 공지 */
		void	Know_Job(int iSeat, Protocol::JOB eJob);		/* 알아낸 직업 → 의자 위 */
		bool	Is_Alive(int iSeat) const { return iSeat >= 0 && (m_iAliveMask >> iSeat) & 1; }

		/* 상태별 업데이트 */
		void	Update_Countdown();								/* 5, 4, 3 … 알림 */
		void	Update_Game();									/* 단계 남은 시간 · 처형 연출 */
		void	Update_Death();									/* 연행 연출 프레임 (DEATH_FRAME_MS마다 다시 그림) */
		void	Update_Result();								/* 연행 연출이 끝난 뒤 승리 연출 → 결과 창 */

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
		int							m_iAgree{ 0 };			/* 찬반 중계로 센 찬성 (탁자 표시용) */
		int							m_iDisagree{ 0 };
		bool						m_isLastVoteDone{ false };	/* 찬반은 한 번만 */

		/* 밤 능력 (서버도 같은 규칙으로 막음 : 여기선 화면 · 입력만) */
		NIGHT_MODE					m_eNightMode{ NIGHT_MODE::CHAT };
		bool						m_isSkillDone{ false };		/* 이번 밤 능력 씀 (밤마다 초기화) */
		bool						m_isFindJobUsed{ false };	/* 배신자 직업 확인 (게임 중 1회) */
		bool						m_isContacted{ false };		/* 배신자 ↔ 마피아 접선 (게임 내내) */
		std::array<Protocol::JOB, Protocol::MAX_ROOM_PLAYER>	m_arrKnownJob{};	/* 능력으로 알아낸 남의 직업 (UNKNOWN = 모름) */
		std::array<bool, Protocol::MAX_ROOM_PLAYER>			m_arrKnownMafia{};	/* 탐정 결과 "마피아" (직업은 모름) */
		std::array<bool, Protocol::MAX_ROOM_PLAYER>			m_arrKnownClean{};	/* 탐정 결과 "마피아 아님" */
		std::array<Protocol::JOB, Protocol::MAX_ROOM_PLAYER>	m_arrAllJobs{};		/* [취약 예시] 결과 창에 쓰려고 들고 있는 전원 직업 (화면엔 안 그림) */

		/* 처형 연출 : 결과를 받은 순간부터 원탁을 덮는 해골 상자 (프레임은 m_tView.iExecFrame) */
		static constexpr ULONGLONG	DEATH_FRAME_MS{ 150 };
		bool						m_isFxPlaying{ false };	/* 연행 · 밤 습격 연출 중 */
		ULONGLONG					m_ullDeathStart{ 0 };
		ULONGLONG					m_ullWinStart{ 0 };		/* 승리 연출 시작 시각 (0 = 아직 : 연행 연출이 먼저 끝나야 함) */
	};
}
