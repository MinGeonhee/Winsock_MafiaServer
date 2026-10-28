#include "pch.h"
#include "CMafiaLogic.h"
#include "CServerApp.h"

namespace MafiaServer
{
	CMafiaLogic::CMafiaLogic()
		: m_iMafiaClientKey{Net::NONE_CLIENTKEY}, m_iRoomKey{Protocol::NONE_ROOMKEY}
		, m_eCurPhase{Protocol::GAME_PHASE::NIGHT}
		, m_iCurStateTime{}
		, m_iCurStateLen{}
		, m_iDay{}
		, m_iAccusedSeat{-1}
		, m_iMafiaSeat{-1}
		, m_iExecutedSeat{-1}
		, m_eEndGameResult{Protocol::ENDGAME_RESULT::NONE}
	{
		m_arrVote.fill(-1);			/* 아무도 안 고름 */
		m_arrAlive.fill(false);		/* 게임 시작할 때 앉은 자리만 true로 */
	}	  


	CMafiaLogic::~CMafiaLogic()
	{

	}

	HRESULT CMafiaLogic::Initialize(const CMafiaLogic::Init_Data& tData)
	{
		if (tData.iMafiaClientKey == Net::NONE_CLIENTKEY)	return E_FAIL;
		if (tData.iRoomKey == Protocol::NONE_ROOMKEY)		return E_FAIL;

		/* Room에서 정해진 역할 부여 */
		m_iMafiaClientKey = tData.iMafiaClientKey;
		m_iRoomKey = tData.iRoomKey;
		m_iMafiaSeat = tData.iMafiaSeat;
		m_arrAlive = tData.arrSeated;

		/* 첫 State(밤)부터 시작 */
		m_iDay = 0;
		Start_Phase(Protocol::GAME_PHASE::NIGHT, tData.ullNow);
		return S_OK;
	}

	void CMafiaLogic::Pre_Update(ULONGLONG ullNow)
	{
		/* 이번 State 제한 시간이 지났으면 다음 State로 */
		if (ullNow - m_iCurStateTime >= m_iCurStateLen)
			Change_Phase(ullNow);
	}

	void CMafiaLogic::Update(ULONGLONG ullNow)
	{
		/*  */

	}

	void CMafiaLogic::Late_Update(ULONGLONG ullNow)
	{

	}

	ULONGLONG CMafiaLogic::Get_Remain(ULONGLONG ullNow) const
	{
		const ULONGLONG ullElapsed = ullNow - m_iCurStateTime;
		return ullElapsed >= m_iCurStateLen ? 0 : m_iCurStateLen - ullElapsed;
	}

	bool CMafiaLogic::Can_Chat(int iSeat) const
	{
		if (iSeat < 0 || iSeat >= Protocol::MAX_ROOM_PLAYER || !m_arrAlive[iSeat]) return false;	/* 죽은 사람은 못 침 */

		switch (m_eCurPhase)
		{
		case Protocol::GAME_PHASE::NIGHT:		return Is_Mafia(iSeat);		/* 밤 : 마피아끼리 (받는 사람도 마피아만, CRoom::Chatting) */
		case Protocol::GAME_PHASE::DAY:			return true;
		case Protocol::GAME_PHASE::ALLOCUTION:	return iSeat >= 0 && iSeat == m_iAccusedSeat;
		default:								return false;
		}
	}

	void CMafiaLogic::Kill(INT32 iSeat, INT32 iKillerSeat)
	{
		if (m_arrAlive[iSeat] == false || iKillerSeat == iSeat) return;

		/* 처형이아닌 킬러에 의해서 죽은경우 */
		if (iKillerSeat != Protocol::NONE_SEAT)
			if (iKillerSeat != m_iMafiaSeat) return;

		m_arrAlive[iSeat] = false;
	}

	/* Game End 조건 */
	Protocol::ENDGAME_RESULT CMafiaLogic::Judge_EndGame()
	{
		/* 살아 있는 사람만 팀별로 셈 (죽은 사람 · 빈자리는 m_arrAlive = false) */
		INT32 iMafia{ 0 }, iCitizen{ 0 };
		for (INT32 i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
		{
			if (!m_arrAlive[i]) continue;
			Is_Mafia(i) ? ++iMafia : ++iCitizen;
		}

		if (iMafia == 0)		return Protocol::ENDGAME_RESULT::CITIZEN;	/* 마피아가 다 잡힘 */
		if (iMafia >= iCitizen)	return Protocol::ENDGAME_RESULT::MAFIA;		/* 투표로 절대 못 이기는 수 (동점이면 아무도 연행 안 됨) */
		return Protocol::ENDGAME_RESULT::NONE;
	}

	INT32 CMafiaLogic::Get_SeatVoteCount(INT32 iSeat)
	{
		if (Protocol::NONE_SEAT == iSeat || iSeat < 0) return -1;
		if (m_arrAlive[iSeat] == false)				   return - 1;
		
		INT32 iCount{0};

		for (INT32 i = 0; i < Protocol::MAX_ROOM_PLAYER ; ++i)
		{
			if (m_arrAlive[i] == false) continue;
			if (m_arrVote[i] == iSeat)	++iCount;
		}

		return iCount;
	}

	INT32 CMafiaLogic::Get_LastVote(INT32 iSeat)
	{
		if (Protocol::NONE_SEAT == iSeat || iSeat < 0)				return -1;
		if (m_arrAlive[iSeat] == false || m_arrVote[iSeat] == -1)	return -1;
		return static_cast<bool>(m_arrVote[iSeat]);
	}

	INT32 CMafiaLogic::Get_LastVoteResult()
	{
		uint8_t iYes{};
		uint8_t iNo{};

		for (uint8_t i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
		{
			if (m_arrAlive[i] == false || m_arrVote[i] == -1) continue;
			m_arrVote[i] == true ? iYes++ : iNo++;
		}


		INT32 iReturn{};
		if (iYes > iNo)
		{
			iReturn = m_iAccusedSeat;
		}
		else iReturn = -1;

		return iReturn;
	}


	bool CMafiaLogic::Vote(int iVoter, int iTarget)
	{
		if (m_eCurPhase != Protocol::GAME_PHASE::VOTE) return false;
		if (iVoter < 0 || iVoter >= Protocol::MAX_ROOM_PLAYER || !m_arrAlive[iVoter]) return false;
		if (iTarget != -1)
		{
			if (iTarget < 0 || iTarget >= Protocol::MAX_ROOM_PLAYER || !m_arrAlive[iTarget]) return false;
			if (iTarget == iVoter) return false;		/* 자기 자신은 안 됨 */
		}
		m_arrVote[iVoter] = iTarget;
		return true;
	}

	bool CMafiaLogic::LastVote(int iVoter, bool is)
	{
		if (m_eCurPhase != Protocol::GAME_PHASE::LAST_VOTE) return false;
		if (iVoter < 0 || iVoter >= Protocol::MAX_ROOM_PLAYER || !m_arrAlive[iVoter]) return false;
		if (m_iAccusedSeat == iVoter) return false;		/* 자기 자신은 안 됨 */
		if (m_arrVote[iVoter] != -1) return false;		/* 이미 찬반함 : 한 번만 (다시 받으면 중계가 두 번 나가 집계가 틀어짐) */


		/* 서버에 저장되어 있는 최후의 변론인한테 찬반투표 */
		m_arrVote[iVoter] = is;

		return true;
	}

	int CMafiaLogic::Count_Vote() const
	{
		std::array<int, Protocol::MAX_ROOM_PLAYER> arrCount{};
		for (int iTarget : m_arrVote)
			if (iTarget >= 0) ++arrCount[iTarget];

		int iBest = -1, iBestCount = 0;
		bool isTie = false;
		for (int i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
		{
			if (arrCount[i] > iBestCount)							{ iBest = i; iBestCount = arrCount[i]; isTie = false; }
			else if (arrCount[i] == iBestCount && iBestCount > 0)	isTie = true;
		}
		return isTie ? -1 : iBest;
	}

	void CMafiaLogic::Change_Phase(ULONGLONG ullNow)
	{
		/* 현재 State 증가 */
		uint16_t iState = static_cast<uint16_t>(m_eCurPhase);
		++iState;
		if (iState >= static_cast<uint16_t>(Protocol::GAME_PHASE::GAME_RESULT)) iState = 0;

		/* 투표 끝 : 집계. 지목된 사람이 없으면 최후 변론 · 찬반 건너뛰고 바로 밤으로 */
		if (m_eCurPhase == Protocol::GAME_PHASE::VOTE)
		{
			m_iAccusedSeat = Count_Vote();
			if (m_iAccusedSeat < 0) iState = static_cast<uint16_t>(Protocol::GAME_PHASE::LOADING_NIGHT);
		}

		/* 투표 끝 : 집계. 지목된 사람이 없으면 최후 변론 · 찬반 건너뛰고 바로 밤으로 */
		if (m_eCurPhase == Protocol::GAME_PHASE::LAST_VOTE)
		{
			m_iExecutedSeat = Get_LastVoteResult();

			/* -1이 아닌 경우 처형 */
			if (m_iExecutedSeat != -1)
			{
				/* 죽인 뒤 해당 */
				Kill(m_iExecutedSeat);
				m_eEndGameResult = Judge_EndGame();		/* Room이 Get_Winner로 읽어 감 */
				const Protocol::ENDGAME_RESULT eResult = m_eEndGameResult;

				/* 게임이 끝났다면 결과 창으로 이동*/
				if (eResult != Protocol::ENDGAME_RESULT::NONE && eResult != Protocol::ENDGAME_RESULT::END)
				{
					iState = static_cast<uint16_t>(Protocol::GAME_PHASE::GAME_RESULT);
				}
			}
		}

		Start_Phase(static_cast<Protocol::GAME_PHASE>(iState), ullNow);
	}

	void CMafiaLogic::Start_Phase(Protocol::GAME_PHASE ePhase, ULONGLONG ullNow)
	{
		m_eCurPhase = ePhase;
		m_iCurStateTime = ullNow;								/* 이 State가 시작된 시각 */
		if (m_eCurPhase == Protocol::GAME_PHASE::DAY) ++m_iDay;	/* 낮이 될 때마다 하루 */
		if (m_eCurPhase == Protocol::GAME_PHASE::VOTE) { m_arrVote.fill(-1); m_iAccusedSeat = -1; }	/* 새 투표 */

		/* 추후 DB연결해서 사용하고싶긴해 */
		switch (m_eCurPhase)
		{
		case Protocol::GAME_PHASE::NIGHT:
			m_iCurStateLen = 30'000;  /* 30초 */
			break;
		case Protocol::GAME_PHASE::LOADING_DAY:
			m_iCurStateLen = 5'000;  /* 5초 */
			break;
		case Protocol::GAME_PHASE::DAY:
			m_iCurStateLen = 30'000; /* 30초 */
			break;
		case Protocol::GAME_PHASE::VOTE:
			m_arrVote.fill(-1);
			m_iAccusedSeat = 0;
			m_iExecutedSeat = 0;
			m_iCurStateLen = 15'000; /* 15초 */ 
			break;
		case Protocol::GAME_PHASE::ALLOCUTION:
			m_iCurStateLen = 15'000; /* 15초 */
			break;
		case Protocol::GAME_PHASE::LAST_VOTE:
			m_iCurStateLen = 10'000; /* 10초 */
			m_arrVote.fill(-1);
			break;
		case Protocol::GAME_PHASE::LOADING_NIGHT:
			m_iCurStateLen = 5'000; /* 5초 */
			break;
		default:
			break;
		}

	}


	std::unique_ptr<CMafiaLogic> CMafiaLogic::Create(const CMafiaLogic::Init_Data& tData)
	{
		std::unique_ptr<CMafiaLogic> pMafiaLogic{new CMafiaLogic()};

		if (FAILED(pMafiaLogic->Initialize(tData)))
			return nullptr;

		return pMafiaLogic;
	}

}

