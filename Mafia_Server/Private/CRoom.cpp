#include "pch.h"
#include "CRoom.h"
#include "CServerApp.h"

namespace MafiaServer
{
	CRoom::CRoom()
		: m_iRoomKey{}, m_arrayPlayerClientKeys{}, m_iRoomMasterClientKey{}, m_eCurState{CRoom::STATE::WATING}
		, m_pServerApp{nullptr}, m_iCurPlayerNum{}
	{

	}
	CRoom::~CRoom()
	{
		m_pServerApp = nullptr;
	}

	bool CRoom::Get_IsPlaying() const
	{
		if (m_eCurState == CRoom::STATE::GAME || m_eCurState == CRoom::STATE::LOADING)
			return true;
		return false;
	}

	CRoom::ENTER_RESULT CRoom::Enter(Net::ClientKey iEnterPlayerClientKey)
	{
		/* 이미 게임이 시작 중인 경우 */
		if (m_eCurState != CRoom::STATE::WATING) return CRoom::ENTER_RESULT::START_GAME;

		/* Find로 해당 플레이어를 찾아서 검색해보기 - 한번더 체크 검사용 */
		const CPlayer* pPlayer =  m_pServerApp->Get_PlayerManager()->Find_Player(iEnterPlayerClientKey);
		if (pPlayer == nullptr) return CRoom::ENTER_RESULT::ID_ERROR;

		/* 남는 공간 찾기 : 첫 빈자리 하나만 차지 (break 없으면 남은 빈칸을 한 사람이 다 채움) */
		bool isFull{true};
		for (auto& Key : m_arrayPlayerClientKeys)
		{
			if (Key != Net::NONE_CLIENTKEY) continue;
			Key = iEnterPlayerClientKey;
			isFull = false;
			break;
		}

		if (isFull == true)
			return CRoom::ENTER_RESULT::FULL_ROOM;

		m_iCurPlayerNum++;

		return CRoom::ENTER_RESULT::OK;
	}

	void CRoom::Exit(Net::ClientKey iExitPlayerClientKey)
	{
		if (iExitPlayerClientKey == Net::NONE_CLIENTKEY) return;

		/* 방에서 제외 (방에 없는 키면 아무것도 안 함) */
		bool isFound{ false };
		for (auto& Key : m_arrayPlayerClientKeys)
		{
			if (Key != iExitPlayerClientKey) continue;
			Key = Net::NONE_CLIENTKEY;
			isFound = true;
			break;
		}
		if (isFound == false) return;

		--m_iCurPlayerNum;

		if (iExitPlayerClientKey == m_iRoomMasterClientKey)
			Change_Master();
	}

	void CRoom::Change_Master()
	{
		/* 그냥 앞에서부터 : 남아 있는 첫 사람 (아무도 없으면 NONE) */
		m_iRoomMasterClientKey = Net::NONE_CLIENTKEY;
		for (const auto& Key : m_arrayPlayerClientKeys)
		{
			if (Key == Net::NONE_CLIENTKEY) continue;
			m_iRoomMasterClientKey = Key;
			break;
		}
	}

	HRESULT CRoom::Initialize(const Init_Data& tData)
	{
		m_pServerApp = CServerApp::Get_Instance();
		if (m_pServerApp == nullptr) return E_FAIL;

		m_iRoomKey = tData.iRoomKey;
		m_iRoomMasterClientKey = tData.iRoomMasterClientKey;
		m_eCurState = CRoom::STATE::WATING;


		m_arrayPlayerClientKeys.fill(Net::NONE_CLIENTKEY);
		m_arrayPlayerClientKeys[0] = m_iRoomMasterClientKey;
		wcsncpy_s(m_wszRoomName, tData.wstrRoomName.c_str(), _TRUNCATE);	/* 길면 잘라냄 (개수를 넘기면 길 때 프로그램이 죽음) */
	
		m_iCurPlayerNum = 1;

		return S_OK;
	}

	std::unique_ptr<CRoom> CRoom::Create(const Init_Data& tData)
	{
		std::unique_ptr<CRoom> pRoom{ new CRoom() };
		
		if(FAILED(pRoom->Initialize(tData)))
			return nullptr;

		return pRoom;
	}
}
