#pragma once


namespace MafiaServer
{
	class CRoom;

	class CMafiaLogic
	{
	public:
		/* 자리에 따른 Info */
		struct Seat_Info
		{
			bool		    isAlive{ true };						/* 현재 살았는지 */
			bool		    isUsePhaseAbility{ true };				/* 능력을 사용했는지 안했는지 */
			INT32		    iVoteSeat{Protocol::NONE_VOTE};			/* 이자리가 투표한 자리 -1일시 아직 투표를 진행하지 않음*/
			Protocol::JOB   eJob{ Protocol::JOB::CITIZEN};			/* 직업 */
		public:
			/* 투표 결과 리셋 투표 페이지 넘어갈때 호출 */
			void  Reset_Vote()
			{
				iVoteSeat = -1;
			}
		};

		struct UseSkillData
		{
			INT32 iMafiaKillTarget		{ Protocol::NONE_SEAT };
			INT32 iBodyguardTarget		{ Protocol::NONE_SEAT };
			bool  isBetRayerFindJob		{ false };
			bool  isBetRayerFindMafia	{ false };
			bool  isSoldierUsed			{ false };	/* 군인 1회 버팀 사용 (게임 내내 유지) */
		public:
			void   Reset()
			{
				iMafiaKillTarget = iBodyguardTarget = Protocol::NONE_SEAT;
				/* 배신자 능력은 단 한번만 사용가능 */
			}
		};
		
		struct Init_Data
		{
			ULONGLONG ullNow{ 0 };													 /* 게임 시작 시각 (CServerApp의 이번 프레임 시각) */
			std::array<bool, Protocol::MAX_ROOM_PLAYER> arrayParticipant;			 /* 참가자 array 배열 있으면 true 없으면 false */
			const CRoom* pRoom{ nullptr };												 /* 패킷 배달 (자리 → 키는 Room이 바꿈) */
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
		uint16_t				Get_Day()		const { return m_iDay; }
		/* 이번 State 남은 시간 (ms, 지났으면 0) : 클라에 알릴 때 */
		ULONGLONG				Get_Remain(ULONGLONG ullNow) const;
		/* 지금 단계에 이 자리가 채팅할 수 있나 (토론 : 전원 / 최후 변론 : 지목된 사람만 / 나머지 : 불가) */
		bool					Can_Chat(int iSeat) const;
		Protocol::JOB			Get_Job(INT32 iSeat)const;
		int						Get_AccusedSeat() const { return m_iAccusedSeat; }	/* VOTE가 끝난 뒤 지목된 자리 (-1 = 없음) */
		uint8_t					Get_AliveMask() const								/* 비트 i = i번 자리 생존 */
		{
			uint8_t iMask = 0;
			for (int i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
				if (m_arraySeatInfo[i] && m_arraySeatInfo[i]->isAlive) iMask |= static_cast<uint8_t>(1u << i);
			return iMask;
		}
		bool					Is_Mafia(INT32 iSeat) const { return Get_Job(iSeat) == Protocol::JOB::MAFIA; }	/* 밤 채팅 · 처치 */
		bool					Is_MafiaTeam(INT32 iSeat) const													/* 승패 판정 (배신자 포함) */
		{
			const Protocol::JOB eJob = Get_Job(iSeat);
			return eJob == Protocol::JOB::MAFIA || eJob == Protocol::JOB::BETRAYER;
		}
		bool					Is_NightChatMember(INT32 iSeat) const											/* 밤 채팅 : 마피아 + 접선한 배신자 */
		{
			return Is_Mafia(iSeat) || (m_tUseSkillData.isBetRayerFindMafia && Get_Job(iSeat) == Protocol::JOB::BETRAYER);
		}
		bool							Check_SeatRange(INT32 iSeat) const;

		void							Kill(INT32 iSeat, INT32 iKillerSeat);
		void							Arrest(INT32 iSeat);
		Protocol::ENDGAME_RESULT		Judge_EndGame();
		INT32							Get_SeatVoteCount(INT32 iSeat) const;			/* iSeat자리가 투표받은 숫자 */
		INT32							Get_ExecutedSeat() const { return m_iExecutedSeat; }
		bool							Is_PresidentSaved() const { return m_isPresidentSaved; }	/* 찬반에서 대통령이라 연행 안 됨 */
		Protocol::ENDGAME_RESULT		Get_Winner() const { return m_eEndGameResult; }	/* GAME_RESULT로 넘어갈 때 정해진 이긴 팀 */
		INT32							Get_LastVote(INT32 iSeat);
		INT32							Get_LastVoteResult();
		/* VOTE 단계에 iVoter 자리가 iTarget 자리에 투표 (-1 = 무효표). 다시 오면 덮어씀. 자기 자신 · 죽은 사람 · 빈자리는 거부 */
		bool							Vote(INT32 iVoter, INT32 iTarget);
		bool							LastVote(INT32 iVoter, bool isAgree);

		/* 각 직업별로 능력사용처리 : tAction은 받은 버퍼라 값만 꺼내 저장 (크기 검사는 CServerApp에서 끝난 상태) */
		void							Use_Skill(INT32 iUseSeat, const Protocol::JOB_ACTION& tAction);
	private:
		void							Change_Phase(ULONGLONG ullNow);	/* 다음 State로 */
		void							Start_Phase(Protocol::GAME_PHASE ePhase, ULONGLONG ullNow);	/* 이 State 시작 : 시작 시각 · 제한 시간 기록 */
		INT32							Get_MostVoteSeat() const;			/* 최다 득표 자리 (동점 / 0표 = -1) */
	public:
		static std::unique_ptr<CMafiaLogic> Create(const CMafiaLogic::Init_Data& tData);
	private:
		/* State 시간 : 시작 시각을 저장해 두고 (지금 - 시작)으로 판단 (매 프레임 더하지 않음 → 오차 안 쌓임) */
		Protocol::GAME_PHASE	m_eCurPhase;		/* 현재 Phase */
		ULONGLONG				m_iCurStateTime;	/* 현재 Phase가 시작된 시각 (ms) */
		ULONGLONG				m_iCurStateLen;		/* 현재 Phase 제한시간 (ms) */
		uint16_t				m_iDay;				/* 진행 차수 */

		/* 투표 : 인덱스 = 자리 번호(0 ~ 6, 방의 m_arrayPlayerClientKeys와 같은 순서) */
		std::array<Seat_Info*, Protocol::MAX_ROOM_PLAYER>		m_arraySeatInfo;

		/* 각 능력자들의 능력지정대상 */
		/* 각 능력자들의 능력지정대상 (NONE_SEAT = 아직 안 씀, 밤 시작마다 초기화) */
		UseSkillData					m_tUseSkillData;
		INT32							m_iMafiaSeat;

		INT32							m_iAccusedSeat;	/* 투표로 지목된 자리 (최후 변론자, -1 = 없음) */
		INT32							m_iExecutedSeat;
		bool							m_isPresidentSaved{ false };
		bool							m_isFxNext{ false };		/* 다음 로딩 단계에 클라 연출이 있음 (밤 습격 · 연행 · 면책) → 로딩을 길게 */
		Protocol::ENDGAME_RESULT		m_eEndGameResult;

		/* Room키 관리  */
		Protocol::ROOM_KEY				m_iRoomKey;

		/* 이 로직을 가진 방 : Room이 unique_ptr로 로직을 소유 → 방이 로직보다 먼저 사라지지 않음 */
		const CRoom*		m_pRoom{ nullptr };
	};
}

