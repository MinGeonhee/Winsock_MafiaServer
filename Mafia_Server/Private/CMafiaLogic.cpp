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
	{ 

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

	void CMafiaLogic::Change_Phase(ULONGLONG ullNow)
	{
		/* 현재 State 증가 */
		uint16_t iState = static_cast<uint16_t>(m_eCurPhase);
		++iState;
		if (iState >= static_cast<uint16_t>(Protocol::GAME_PHASE::END)) iState = 0;

		Start_Phase(static_cast<Protocol::GAME_PHASE>(iState), ullNow);
	}

	void CMafiaLogic::Start_Phase(Protocol::GAME_PHASE ePhase, ULONGLONG ullNow)
	{
		m_eCurPhase = ePhase;
		m_iCurStateTime = ullNow;								/* 이 State가 시작된 시각 */
		if (m_eCurPhase == Protocol::GAME_PHASE::DAY) ++m_iDay;	/* 낮이 될 때마다 하루 */

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
			m_iCurStateLen = 15'000; /* 15초 */ 
			break;
		case Protocol::GAME_PHASE::ALLOCUTION:
			m_iCurStateLen = 15'000; /* 15초 */
			break;
		case Protocol::GAME_PHASE::LAST_VOTE:
			m_iCurStateLen = 10'000; /* 10초 */
			break;
		case Protocol::GAME_PHASE::LOADING_NIGHT:
			m_iCurStateLen = 5'000; /* 5초 */
			break;
		default:
			break;
		}

		/* 턴이 바꼈다면 */

	}


	std::unique_ptr<CMafiaLogic> CMafiaLogic::Create(const CMafiaLogic::Init_Data& tData)
	{
		std::unique_ptr<CMafiaLogic> pMafiaLogic{new CMafiaLogic()};

		if (FAILED(pMafiaLogic->Initialize(tData)))
			return nullptr;

		return pMafiaLogic;
	}

}

