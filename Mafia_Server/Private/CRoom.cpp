#include "pch.h"
#include "CRoom.h"
#include "CServerApp.h"
#include "CChatting.h"
#include "CMafiaLogic.h"
#include <random>
#include <algorithm>

namespace MafiaServer
{
	CRoom::CRoom()
		: m_iRoomKey{}, m_arrayPlayerClientKeys{}, m_iRoomMasterClientKey{}, m_eCurState{CRoom::STATE::WATING}
		, m_pServerApp{nullptr}, m_iCurPlayerNum{} , m_pChatting{nullptr},m_wszRoomName{}
	{

	}
	CRoom::~CRoom()
	{
		m_pServerApp = nullptr;
	}

	void CRoom::Pre_Update(ULONGLONG ullNow)
	{
		/* 대기 중이면 할 일 없음, 게임 중이면 시각을 그대로 넘김 (시간 계산은 게임 로직이) */
		if (m_pLogic == nullptr) return;

		/* 단계 시간 검사는 로직 Pre_Update에서 → 그 앞뒤로 비교해서 바뀐 프레임에만 한 번 알림 */
		const Protocol::GAME_PHASE ePrev = m_pLogic->Get_Phase();
		m_pLogic->Pre_Update(ullNow);
		if (m_pLogic->Get_Phase() != ePrev)
		{
			/* 결과가 먼저 가야 클라가 최후 변론 단계에서 지목된 사람을 앎 */
			if (ePrev == Protocol::GAME_PHASE::VOTE) Send_VoteResult();
			if (ePrev == Protocol::GAME_PHASE::LAST_VOTE) Send_LastVoteResult();	/* 처형 연출이 먼저 */
			if (ePrev == Protocol::GAME_PHASE::VOTE && m_pLogic->Is_PresidentSaved()) Send_LastVoteResult();	/* 대통령 지목 : 찬반 없이 바로 면책 알림 */

			/* 승패가 났으면 단계 알림 대신 게임 끝 (m_pLogic을 지우므로 바로 나감) */
			if (m_pLogic->Get_Phase() == Protocol::GAME_PHASE::GAME_RESULT) { End_Game(); return; }

			Send_ChangePhase(ullNow);
		}
	}

	void CRoom::Update(ULONGLONG ullNow)
	{
		/* 로딩(카운트다운)이 끝나면 게임 시작 */
		if (m_eCurState == CRoom::STATE::LOADING && ullNow - m_ullLoadingStart >= Protocol::START_COUNTDOWN_MS)
			Start_Game(ullNow);

		if (m_pLogic == nullptr) return;
		m_pLogic->Update(ullNow);
	}

	void CRoom::Start_Loading(ULONGLONG ullNow)
	{
		m_eCurState = CRoom::STATE::LOADING;		/* 이때부터 입장 막힘 (Enter가 WATING만 받음) */
		m_ullLoadingStart = ullNow;
	}

	Protocol::ENDGAME_RESULT CRoom::Judge_EndGame()
	{
		return m_pLogic ? m_pLogic->Judge_EndGame() : Protocol::ENDGAME_RESULT::NONE;
	}

	void CRoom::Start_Game(ULONGLONG ullNow)
	{
		/* 우선 게임이 진행 가능한지 다시 체크 (카운트다운 동안 나간 사람) */
		Protocol::S_TO_C_STARTGAME tBody{};
		if (m_iCurPlayerNum < Protocol::MIN_START_PLAYER)
		{
			/* 게임 시작을 할 수 없음 : 대기로 되돌리고 (안 돌리면 매 프레임 여기로 옴) 방 전원에게 취소 */
			m_eCurState = CRoom::STATE::WATING;
			tBody.eResult = Protocol::STARTGAME_RESULT::CANCELED;

			for (auto Key : m_arrayPlayerClientKeys)
			{
				if (Key == Net::NONE_CLIENTKEY) continue;
				m_pServerApp->Send_Packet(Key, Protocol::PACKET_TYPE::S_TO_C_STARTGAME, tBody);
			}
			return;
		}

		/* 자리 섞기 : 대기실 자리로 누군지 알 수 없게 (게임 중엔 이름 대신 "N번"만 보임) */
		static std::mt19937 s_Random{ std::random_device{}() };
		std::shuffle(m_arrayPlayerClientKeys.begin(), m_arrayPlayerClientKeys.end(), s_Random);

		/* 앉은 자리만 넘김 : 직업 배정은 게임 로직(CMafiaLogic::Initialize)이 덱으로 */
		CMafiaLogic::Init_Data tData{};
		tData.ullNow = ullNow;
		tData.pRoom = this;
		for (int i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
			tData.arrayParticipant[i] = (m_arrayPlayerClientKeys[i] != Net::NONE_CLIENTKEY);

		m_pLogic = CMafiaLogic::Create(tData);
		if (m_pLogic == nullptr) { m_eCurState = CRoom::STATE::WATING; return; }
		m_eCurState = CRoom::STATE::GAME;

		/* 사람마다 : 첫 단계는 같고 직업만 다름 */
		tBody.eResult	= Protocol::STARTGAME_RESULT::OK;
		tBody.ePhase	= m_pLogic->Get_Phase();
		tBody.iDay		= static_cast<uint16_t>(m_pLogic->Get_Day());
		tBody.iRemainMs = static_cast<uint32_t>(m_pLogic->Get_Remain(ullNow));
		for (int i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i) tBody.arrSeatKeys[i] = m_arrayPlayerClientKeys[i];	/* 섞인 자리 */

		for (int i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
		{
			if (m_arrayPlayerClientKeys[i] == Net::NONE_CLIENTKEY) continue;
			tBody.eJob = m_pLogic->Get_Job(i);
			m_pServerApp->Send_Packet(m_arrayPlayerClientKeys[i], Protocol::PACKET_TYPE::S_TO_C_STARTGAME, tBody);
		}
	}

	void CRoom::LateUpdate(ULONGLONG ullNow)
	{

	}

	bool CRoom::Get_IsPlaying() const
	{
		if (m_eCurState == CRoom::STATE::GAME || m_eCurState == CRoom::STATE::LOADING)
			return true;
		return false;
	}

	bool CRoom::Can_Start() const
	{
		/* 대기 중이고, 최소 인원 이상 앉아 있어야 시작 (테스트용 최소 인원은 Protocol::MIN_START_PLAYER) */
		if (m_eCurState != CRoom::STATE::WATING) return false;
		return m_iCurPlayerNum >= Protocol::MIN_START_PLAYER;
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

	void CRoom::Chatting(const SYSTEMTIME& tTime, Net::ClientKey iChatterKey, const std::wstring& wstrText,Net::CServer* pMainServer)
	{
		/* 현재 방안에 해당 Key의 인원이 있는지 확인 */
		if (pMainServer == nullptr) return;
		if (Has_Player(iChatterKey) == false) return;
		const CPlayer* pPlayer = m_pServerApp->Get_PlayerManager()->Find_Player(iChatterKey);
		if (pPlayer == nullptr) return;
		if (m_pChatting == nullptr) return;

		/* 게임 중 : 단계 규칙상 못 치는 사람의 채팅은 버림 (클라도 막지만 서버가 최종 판단) */
		const bool isGame = (m_eCurState == CRoom::STATE::GAME && m_pLogic);
		if (isGame && !m_pLogic->Can_Chat(Find_Seat(iChatterKey))) return;
		const bool isMafiaOnly = isGame && m_pLogic->Get_Phase() == Protocol::GAME_PHASE::NIGHT;	/* 밤 채팅은 마피아만 받음 */

		m_pChatting->Chatting(tTime,iChatterKey,wstrText);
		/* 추후 채팅을 실패했을 경우에 실패 메세지 따로 첨부 */


		/* 모든 인원한테 채팅 발송하기 */
		Protocol::CHAT_LOG tChat{};
		tChat.eDataType = Protocol::DATA_TYPE::CHAT_LOG;
		tChat.tTime = tTime;
		const std::wstring strName = isGame ? std::to_wstring(Find_Seat(iChatterKey) + 1) + L"번" : pPlayer->Get_Nickname();	/* 게임 중엔 이름 대신 자리 번호 */
		wcsncpy_s(tChat.wszName, strName.c_str(), _TRUNCATE);
		wcsncpy_s(tChat.wszText, wstrText.c_str(), _TRUNCATE);

		/* 발송 */
		for (int i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
		{
			const Net::ClientKey Key = m_arrayPlayerClientKeys[i];
			if (Key == Net::NONE_CLIENTKEY) continue;
			if (isMafiaOnly && !m_pLogic->Is_NightChatMember(i)) continue;
			pMainServer->Send_Packet(Key, static_cast<uint16_t>(Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA),tChat);
		}
	}

	void CRoom::Vote(Net::ClientKey iVoterKey, INT32 iSeat)
	{
		if (m_eCurState != CRoom::STATE::GAME || m_pLogic == nullptr) return;
		if (m_pLogic->Vote(Find_Seat(iVoterKey), iSeat) == false) return;	/* NONE_SEAT = 무효표 그대로 */

		Protocol::GAME_VOTE tData{};
		tData.eDataType = Protocol::DATA_TYPE::GAME_VOTE;
		tData.iCount = m_pLogic->Get_SeatVoteCount(iSeat);
		tData.iVoteSeat = iSeat;

		/* 찬반 투표가 진행될 때마다 발송 */
		for (auto Key : m_arrayPlayerClientKeys)
		{
			if (Key == Net::NONE_CLIENTKEY) continue;
			m_pServerApp->Send_Packet(Key, Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA, &tData, sizeof(tData));
		}
	}

	void CRoom::LastVote(Net::ClientKey iVoterKey, bool is)
	{
		if (m_eCurState != CRoom::STATE::GAME || m_pLogic == nullptr) return;
		const int iSeat = Find_Seat(iVoterKey);
		if (m_pLogic->LastVote(iSeat, is) == false) return;


		Protocol::GAME_LASTVOTE tData{};
		tData.eDataType = Protocol::DATA_TYPE::GAME_LASTVOTE;
		tData.is = is;

		/* 찬반 투표가 진행될 때마다 발송 */
		for (auto Key : m_arrayPlayerClientKeys)
		{
			if (Key == Net::NONE_CLIENTKEY) continue;
			m_pServerApp->Send_Packet( Key, Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA, &tData, sizeof(tData));
		}
	}

	void CRoom::Send_ToSeat(INT32 iSeat, Protocol::PACKET_TYPE eType, const void* pBody, uint16_t usLen) const
	{
		if (iSeat < 0 || iSeat >= Protocol::MAX_ROOM_PLAYER) return;
		const Net::ClientKey iKey = m_arrayPlayerClientKeys[iSeat];
		if (iKey == Net::NONE_CLIENTKEY) return;
		m_pServerApp->Send_Packet(iKey, eType, pBody, usLen);
	}

	void CRoom::Send_ToAll(Protocol::PACKET_TYPE eType, const void* pBody, uint16_t usLen) const
	{
		for (INT32 i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
			Send_ToSeat(i, eType, pBody, usLen);
	}

	void CRoom::Use_Skill(Net::ClientKey iUserKey, const Protocol::JOB_ACTION& tAction)
	{
		if (m_eCurState != CRoom::STATE::GAME || m_pLogic == nullptr) return;
		m_pLogic->Use_Skill(Find_Seat(iUserKey), tAction);
	}

	int CRoom::Find_Seat(Net::ClientKey iKey) const
	{
		if (iKey == Net::NONE_CLIENTKEY) return -1;
		for (int i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
			if (m_arrayPlayerClientKeys[i] == iKey) return i;
		return -1;
	}

	void CRoom::Send_VoteResult()
	{
		Protocol::S_TO_C_VOTE_RESULT tBody{};
		const int iSeat = m_pLogic->Get_AccusedSeat();
		if (iSeat >= 0) tBody.iSeat = iSeat;

		for (auto Key : m_arrayPlayerClientKeys)
		{
			if (Key == Net::NONE_CLIENTKEY) continue;
			m_pServerApp->Send_Packet(Key, Protocol::PACKET_TYPE::S_TO_C_VOTE_RESULT, tBody);
		}
	}

	void CRoom::Send_LastVoteResult()
	{
		Protocol::S_TO_C_LASTVOTE_RESULT tBody{};
		const int iSeat = m_pLogic->Get_ExecutedSeat();
		if (iSeat >= 0) tBody.iSeat = iSeat;
		tBody.isPresident = m_pLogic->Is_PresidentSaved();		/* 대통령이라 살아남음 */

		for (auto Key : m_arrayPlayerClientKeys)
		{
			if (Key == Net::NONE_CLIENTKEY) continue;
			m_pServerApp->Send_Packet(Key, Protocol::PACKET_TYPE::S_TO_C_LASTVOTE_RESULT, tBody);
		}
	}

	void CRoom::End_Game()
	{
		Protocol::S_TO_C_GAME_END tBody{};
		tBody.eWinner		= m_pLogic->Get_Winner();
		tBody.iAliveMask	= m_pLogic->Get_AliveMask();
		for (int i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
		{
			if (m_arrayPlayerClientKeys[i] == Net::NONE_CLIENTKEY) continue;
			tBody.arrJobs[i]		= m_pLogic->Get_Job(i);
			m_arrViewingResult[i]	= true;		/* 결과 창 확인 전엔 F5 막기 */
		}
		for (auto Key : m_arrayPlayerClientKeys)
		{
			if (Key == Net::NONE_CLIENTKEY) continue;
			m_pServerApp->Send_Packet(Key, Protocol::PACKET_TYPE::S_TO_C_GAME_END, tBody);
		}

		m_pLogic.reset();							/* 게임 정리 */
		m_eCurState = CRoom::STATE::WATING;			/* 다시 입장 · F5 가능 */
	}

	void CRoom::SendPacket_All(Protocol::PACKET_TYPE tType, const void* pBody, uint16_t tBodyLen  ,Net::CServer* pMainServer) const
	{
		if (pBody == nullptr) return;

		for (auto Key : m_arrayPlayerClientKeys)
		{
			if (Key == Net::NONE_CLIENTKEY)continue;

			pMainServer->Send_Packet(Key, static_cast<uint16_t>(tType), pBody , tBodyLen );
		}
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

	void CRoom::Send_ChangePhase(ULONGLONG uillNow)
	{
		if (m_pLogic == nullptr) return;

		Protocol::S_TO_C_CHANGEPHASE tBody{};
		tBody.eCurPhase = m_pLogic->Get_Phase();
		tBody.iDay		= m_pLogic->Get_Day();
		tBody.iRemainMs	= m_pLogic->Get_Remain(uillNow);
		tBody.iAliveMask = m_pLogic->Get_AliveMask();

		for (auto Key : m_arrayPlayerClientKeys)
		{
			if (Key == Net::NONE_CLIENTKEY) continue;
				m_pServerApp->Send_Packet( Key , Protocol::PACKET_TYPE::S_TO_C_CHANGEPHASE, &tBody, sizeof(tBody) );
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


		CChatting::Init_Data tChatting_InitData{};
		tChatting_InitData.iMaxChatLen		 = 100;
		tChatting_InitData.iSaveChattingSize = 100;
		m_pChatting = std::move(CChatting::Create(tChatting_InitData));

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
