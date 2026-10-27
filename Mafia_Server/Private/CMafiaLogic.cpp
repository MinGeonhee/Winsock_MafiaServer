#include "pch.h"
#include "CMafiaLogic.h"


namespace MafiaServer
{
	CMafiaLogic::CMafiaLogic()
		: m_iMafiaClientKey{Net::NONE_CLIENTKEY}, m_iRoomKey{Protocol::NONE_ROOMKEY}
		, m_eCurState{CMafiaLogic::STATE::NIGHT}
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
		Start_State(CMafiaLogic::STATE::NIGHT, tData.ullNow);
		return S_OK;
	}

	void CMafiaLogic::Pre_Update(ULONGLONG ullNow)
	{
		/*  */

	}

	void CMafiaLogic::Update(ULONGLONG ullNow)
	{
		/* 이번 State 제한 시간이 지났으면 다음 State로 */
		if (ullNow - m_iCurStateTime >= m_iCurStateLen)
			Change_State(ullNow);
	}

	void CMafiaLogic::Late_Update(ULONGLONG ullNow)
	{

	}

	ULONGLONG CMafiaLogic::Get_Remain(ULONGLONG ullNow) const
	{
		const ULONGLONG ullElapsed = ullNow - m_iCurStateTime;
		return ullElapsed >= m_iCurStateLen ? 0 : m_iCurStateLen - ullElapsed;
	}

	void CMafiaLogic::Change_State(ULONGLONG ullNow)
	{
		/* 현재 State 증가 */
		uint16_t iState = static_cast<uint16_t>(m_eCurState);
		++iState;
		if (iState >= static_cast<uint16_t>(CMafiaLogic::STATE::END)) iState = 0;

		Start_State(static_cast<CMafiaLogic::STATE>(iState), ullNow);
	}

	void CMafiaLogic::Start_State(CMafiaLogic::STATE eState, ULONGLONG ullNow)
	{
		m_eCurState = eState;
		m_iCurStateTime = ullNow;			/* 이 State가 시작된 시각 */
		if (m_eCurState == CMafiaLogic::STATE::DAY) ++m_iDay;	/* 낮이 될 때마다 하루 */

		/* 추후 DB연결해서 사용하고싶긴해 */
		switch (m_eCurState)
		{
		case CMafiaLogic::STATE::NIGHT:
			m_iCurStateLen = 30'000;  /* 30초 */
			break;
		case CMafiaLogic::STATE::LOADING_DAY:
			m_iCurStateLen = 5'000;  /* 5초 */
			break;
		case CMafiaLogic::STATE::DAY:
			m_iCurStateLen = 30'000; /* 30초 */
			break;
		case CMafiaLogic::STATE::VOTE:
			m_iCurStateLen = 15'000; /* 15초 */ 
			break;
		case CMafiaLogic::STATE::ALLOCUTION:
			m_iCurStateLen = 15'000; /* 15초 */
			break;
		case CMafiaLogic::STATE::LAST_VOTE:
			m_iCurStateLen = 10'000; /* 10초 */
			break;
		case CMafiaLogic::STATE::LOADING_NIGHT:
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

