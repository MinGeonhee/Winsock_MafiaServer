#include "pch.h"
#include "CRoom.h"
#include "CServerApp.h"
#include "CChatting.h"
#include "CMafiaLogic.h"
#include <random>

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
			Send_ChangePhase(ullNow);
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

		/* 앉은 사람 중 한 명을 마피아로 (마피아 1명 고정, 직업 배정은 게임 로직으로 옮길 예정) */
		std::vector<Net::ClientKey> vecSeated;
		for (Net::ClientKey Key : m_arrayPlayerClientKeys)
			if (Key != Net::NONE_CLIENTKEY) vecSeated.push_back(Key);

		static std::mt19937 s_Random{ std::random_device{}() };
		std::uniform_int_distribution<size_t> tPick(0, vecSeated.size() - 1);

		CMafiaLogic::Init_Data tData{};
		tData.iRoomKey = m_iRoomKey;
		tData.iMafiaClientKey = vecSeated[tPick(s_Random)];
		tData.ullNow = ullNow;

		m_pLogic = CMafiaLogic::Create(tData);
		if (m_pLogic == nullptr) { m_eCurState = CRoom::STATE::WATING; return; }
		m_eCurState = CRoom::STATE::GAME;

		/* 사람마다 : 첫 단계는 같고 직업만 다름 */
		tBody.eResult	= Protocol::STARTGAME_RESULT::OK;
		tBody.ePhase	= m_pLogic->Get_Phase();
		tBody.iDay		= static_cast<uint16_t>(m_pLogic->Get_Day());
		tBody.iRemainMs = static_cast<uint32_t>(m_pLogic->Get_Remain(ullNow));

		for (auto Key : m_arrayPlayerClientKeys)
		{
			if (Key == Net::NONE_CLIENTKEY) continue;
			tBody.eJob = (Key == tData.iMafiaClientKey) ? Protocol::JOB::MAFIA : Protocol::JOB::CITIZEN;
			m_pServerApp->Send_Packet(Key, Protocol::PACKET_TYPE::S_TO_C_STARTGAME, tBody);
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

		m_pChatting->Chatting(tTime,iChatterKey,wstrText);
		/* 추후 채팅을 실패했을 경우에 실패 메세지 따로 첨부 */


		/* 모든 인원한테 채팅 발송하기 */
		Protocol::CHAT_LOG tChat{};
		tChat.eDataType = Protocol::DATA_TYPE::CHAT_LOG;
		tChat.tTime = tTime;
		wcsncpy_s(tChat.wszName, pPlayer->Get_Nickname().c_str(), _TRUNCATE);
		wcsncpy_s(tChat.wszText, wstrText.c_str(), _TRUNCATE);

		/* 발송 */
		for (auto Key : m_arrayPlayerClientKeys)
			pMainServer->Send_Packet(Key, static_cast<uint16_t>(Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA),tChat);
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
