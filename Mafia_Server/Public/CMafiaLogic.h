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
	private:
		void					Change_Phase(ULONGLONG ullNow);	/* 다음 State로 */
		void					Start_Phase(Protocol::GAME_PHASE ePhase, ULONGLONG ullNow);	/* 이 State 시작 : 시작 시각 · 제한 시간 기록 */
	public:
		static std::unique_ptr<CMafiaLogic> Create(const CMafiaLogic::Init_Data& tData);
	private:
		/* State 시간 : 시작 시각을 저장해 두고 (지금 - 시작)으로 판단 (매 프레임 더하지 않음 → 오차 안 쌓임) */
		Protocol::GAME_PHASE	m_eCurPhase;		/* 현재 Phase */
		ULONGLONG				m_iCurStateTime;	/* 현재 Phase가 시작된 시각 (ms) */
		ULONGLONG				m_iCurStateLen;		/* 현재 Phase 제한시간 (ms) */
		ULONGLONG				m_iDay;				/* 진행 차수 */


		/* Mafia 및 Room키 관리  */
		Protocol::ROOM_KEY	m_iRoomKey;
		Net::ClientKey		m_iMafiaClientKey;
	};
}

