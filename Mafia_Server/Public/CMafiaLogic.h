#pragma once


namespace MafiaServer
{
	class CMafiaLogic
	{
	public:
		struct Init_Data
		{
			Protocol::ROOM_KEY	iRoomKey{Protocol::NONE_ROOMKEY};
			Net::ClientKey		iMafiaClientKey{Net::NONE_CLIENTKEY};
			ULONGLONG			ullNow{ 0 };		/* 게임 시작 시각 (CServerApp의 이번 프레임 시각) */
			int					iMafiaSeat{ -1 };	/* 마피아 자리 (0 ~ 6) */
			std::array<bool, Protocol::MAX_ROOM_PLAYER> arrSeated{};	/* 시작할 때 앉아 있던 자리 = 생존 */
		};
	private:
		explicit CMafiaLogic();
		CMafiaLogic(const CMafiaLogic& cCopy) = delete;
		CMafiaLogic(CMafiaLogic&& cMove) = delete;
	public:
		virtual ~CMafiaLogic();
	private:
		HRESULT	Initialize(const CMafiaLogic::Init_Data& tData);
	public:
		/* 시각은 저장하지 않고 매 함수가 같은 값(CServerApp의 이번 프레임 시각)을 인자로 받음 */
		void	Pre_Update(ULONGLONG ullNow);
		void	Update(ULONGLONG ullNow);
		void	Late_Update(ULONGLONG ullNow);

		Protocol::GAME_PHASE	Get_Phase()		const { return m_eCurPhase; }
		ULONGLONG				Get_Day()		const { return m_iDay; }
		/* 이번 State 남은 시간 (ms, 지났으면 0) : 클라에 알릴 때 */
		ULONGLONG				Get_Remain(ULONGLONG ullNow) const;
		/* 지금 단계에 이 자리가 채팅할 수 있나 (토론 : 전원 / 최후 변론 : 지목된 사람만 / 나머지 : 불가) */
		bool					Can_Chat(int iSeat) const;
		bool					Is_Mafia(int iSeat) const { return iSeat >= 0 && iSeat == m_iMafiaSeat; }
		int						Get_AccusedSeat() const { return m_iAccusedSeat; }	/* VOTE가 끝난 뒤 지목된 자리 (-1 = 없음) */
		uint8_t					Get_AliveMask() const									/* 비트 i = i번 자리 생존 */
		{
			uint8_t iMask = 0;
			for (int i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
				if (m_arrAlive[i]) iMask |= static_cast<uint8_t>(1u << i);
			return iMask;
		}

		/* VOTE 단계에 iVoter 자리가 iTarget 자리에 투표 (-1 = 무효표). 다시 오면 덮어씀. 자기 자신 · 죽은 사람 · 빈자리는 거부 */
		bool					Vote(int iVoter, int iTarget);
	private:
		void					Change_Phase(ULONGLONG ullNow);	/* 다음 State로 */
		void					Start_Phase(Protocol::GAME_PHASE ePhase, ULONGLONG ullNow);	/* 이 State 시작 : 시작 시각 · 제한 시간 기록 */
		int						Count_Vote() const;			/* 최다 득표 자리 (동점 / 0표 = -1) */
	public:
		static std::unique_ptr<CMafiaLogic> Create(const CMafiaLogic::Init_Data& tData);
	private:
		/* State 시간 : 시작 시각을 저장해 두고 (지금 - 시작)으로 판단 (매 프레임 더하지 않음 → 오차 안 쌓임) */
		Protocol::GAME_PHASE	m_eCurPhase;		/* 현재 Phase */
		ULONGLONG				m_iCurStateTime;	/* 현재 Phase가 시작된 시각 (ms) */
		ULONGLONG				m_iCurStateLen;		/* 현재 Phase 제한시간 (ms) */
		ULONGLONG				m_iDay;				/* 진행 차수 */

		/* 투표 : 인덱스 = 자리 번호(0 ~ 6, 방의 m_arrayPlayerClientKeys와 같은 순서) */
		std::array<int, Protocol::MAX_ROOM_PLAYER>	m_arrVote;		/* 자리별로 "누구를 골랐나" (-1 = 안 고름) */
		std::array<bool, Protocol::MAX_ROOM_PLAYER>	m_arrAlive;		/* 자리별 생존 (빈자리 = false) */
		int											m_iAccusedSeat{ -1 };	/* 투표로 지목된 자리 (최후 변론자, -1 = 없음) */
		int											m_iMafiaSeat{ -1 };


		/* Mafia 및 Room키 관리  */
		Protocol::ROOM_KEY	m_iRoomKey;
		Net::ClientKey		m_iMafiaClientKey;
	};
}

