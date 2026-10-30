#include "pch.h"
#include "CRoomManager.h"
#include "CRoom.h"

namespace MafiaServer
{
	CRoomManager::CRoomManager()
		: m_umapRooms{}, m_iRoomKey{ 1 }	/* 0은 "방 없음"으로 비워 둠 */
	{

	}

	CRoomManager::~CRoomManager()
	{

	}

	HRESULT CRoomManager::Initialize(const CRoomManager::Init_Data& tData)
	{

		return S_OK;
	}


	/* 추후 방 생성 실패에 대한 Enum값 뱉기
	   반환 : 만든 방 키 (실패 = 0) → 클라에게 "어느 방에 들어갔는지" 보낼 때 사용
	   방 키는 증가만 하므로 겹칠 일이 없어 중복 검사는 하지 않음 */
	Protocol::ROOM_KEY CRoomManager::Create_Room(const std::wstring& wstrRoomName, Net::ClientKey iRoomMasterPlayerClientKey)
	{
		if (wstrRoomName.empty()) return 0;

		CRoom::Init_Data tData{};
		tData.iRoomKey				= m_iRoomKey;
		tData.iRoomMasterClientKey  = iRoomMasterPlayerClientKey;
		tData.wstrRoomName			= wstrRoomName;

		std::unique_ptr<CRoom> pNewRoom{ CRoom::Create(tData) };
		if (pNewRoom == nullptr) return 0;

		m_umapRooms[tData.iRoomKey] = std::move(pNewRoom);		/* 방 키로 저장 (방장 키로 저장하면 Find_Room으로 못 찾음) */
		return m_iRoomKey++;
	}

	const CRoom* CRoomManager::Find_Room(Protocol::ROOM_KEY iEnterRoomKey)
	{
		const auto iter = m_umapRooms.find(iEnterRoomKey);
		if (iter == m_umapRooms.end()) return nullptr;

		return iter->second.get();
	}

	CRoom::ENTER_RESULT CRoomManager::Enter_Room(Protocol::ROOM_KEY iEnterRoomKey, Net::ClientKey iEnterPlayerClientKey)
	{
		if (Find_Room(iEnterRoomKey) == nullptr) return CRoom::ENTER_RESULT::ID_ERROR;

		return m_umapRooms[iEnterRoomKey]->Enter(iEnterPlayerClientKey);
	}

	/* ponytail: 방을 전부 훑어서 찾음 (방 수가 적어 충분). 방이 많아지면 ClientKey → RoomKey 맵 추가 */
	Protocol::ROOM_KEY CRoomManager::Exit_Room(Net::ClientKey iExitPlayerClientKey)
	{
		for (auto iter = m_umapRooms.begin(); iter != m_umapRooms.end(); ++iter)
		{
			if (iter->second->Has_Player(iExitPlayerClientKey) == false) continue;

			const Protocol::ROOM_KEY iRoomKey = iter->first;
			iter->second->Exit(iExitPlayerClientKey);
			if (iter->second->Get_CurPlayerNum() == 0)
				m_umapRooms.erase(iter);		/* 아무도 없으면 방 삭제 */
			return iRoomKey;
		}
		return 0;
	}
	const wchar_t* CRoomManager::Chatting(Protocol::ROOM_KEY iRoomKey, const SYSTEMTIME& tTime, Net::ClientKey iChatterKey, const std::wstring& wstrText, Net::CServer* pMainServer)
	{
		if (Find_Room(iRoomKey) == nullptr) return L"없는 방";
		
		return m_umapRooms[iRoomKey]->Chatting(tTime,iChatterKey,wstrText,pMainServer);
	}

	const wchar_t* CRoomManager::Vote(Protocol::ROOM_KEY iRoomKey, Net::ClientKey iVoterKey, INT32 iSeat)
	{
		auto iter = m_umapRooms.find(iRoomKey);
		if (iter == m_umapRooms.end()) return L"없는 방";
		return iter->second->Vote(iVoterKey, iSeat);
	}

	const wchar_t* CRoomManager::LastVote(Protocol::ROOM_KEY iRoomKey, Net::ClientKey iVoterKey, bool is)
	{
		auto iter = m_umapRooms.find(iRoomKey);
		if (iter == m_umapRooms.end()) return L"없는 방";
		return iter->second->LastVote(iVoterKey,is);
	}

	const wchar_t* CRoomManager::Use_Skill(Net::ClientKey iUserKey, const Protocol::JOB_ACTION& tAction)
	{
		auto iter = m_umapRooms.find(tAction.iRoomKey);
		if (iter == m_umapRooms.end()) return L"없는 방";
		return iter->second->Use_Skill(iUserKey, tAction);
	}


	bool CRoomManager::Start_Loading(Protocol::ROOM_KEY iRoomKey, ULONGLONG ullNow)
	{
		auto iter = m_umapRooms.find(iRoomKey);
		if (iter == m_umapRooms.end()) return false;

		iter->second->Start_Loading(ullNow);
		return true;
	}

	void CRoomManager::Pre_Update(ULONGLONG ullNow)
	{
		for (auto& Pair : m_umapRooms)
			Pair.second->Pre_Update(ullNow);
	}

	void CRoomManager::Update(ULONGLONG ullNow)
	{
		for (auto& Pair : m_umapRooms)
			Pair.second->Update(ullNow);
	}
}