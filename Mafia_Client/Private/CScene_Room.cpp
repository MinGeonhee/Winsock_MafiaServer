#include "pch.h"
#include "CScene_Room.h"
#include "CClientApp.h"
#include "ConsoleUI.h"


namespace MafiaClient
{
	namespace
	{
		/* 서버가 보낸 enum → 화면 글자 (규칙은 없고 이름만) */
		const wchar_t* Job_Name(Protocol::JOB eJob)
		{
			switch (eJob)
			{
			case Protocol::JOB::CITIZEN:	return L"시민";
			case Protocol::JOB::MAFIA:		return L"마피아";
			default:						return L"?";
			}
		}

		const wchar_t* Phase_Name(Protocol::GAME_PHASE ePhase)
		{
			switch (ePhase)
			{
			case Protocol::GAME_PHASE::NIGHT:			return L"밤";
			case Protocol::GAME_PHASE::LOADING_DAY:		return L"아침이 밝아옵니다";
			case Protocol::GAME_PHASE::DAY:				return L"아침이 밝았습니다";
			case Protocol::GAME_PHASE::VOTE:			return L"투표";
			case Protocol::GAME_PHASE::ALLOCUTION:		return L"최후의 변론";
			case Protocol::GAME_PHASE::LAST_VOTE:		return L"찬반 투표";
			case Protocol::GAME_PHASE::LOADING_NIGHT:	return L"밤이 찾아옵니다";
			default:									return L"";
			}
		}
	}

	CScene_Room::CScene_Room()
		: CScene(), m_isDirty{ true }
	{
		m_eType = MafiaClient::SCENE::ROOM;
	}
	CScene_Room::~CScene_Room()
	{
	}

	HRESULT CScene_Room::Initialize()
	{
		if (FAILED(__super::Initialize())) return E_FAIL;

		return S_OK;
	}

	void CScene_Room::Enter()
	{
		__super::Enter();

		Apply_RoomInfo();
		m_tView.vecChats.push_back({ L"", L"방에 들어왔습니다", false, true });

		ConsoleUI::Set_CursorVisible(false);
		ConsoleUI::Clear();
		m_isDirty = true;
	}

	void CScene_Room::Exit()
	{
	}

	void CScene_Room::Pre_Update()
	{
		/* 막지 않는 입력 : 키는 프레임에 한 번만 읽어서 (Poll_Keys가 버퍼를 비움) 상태별로 넘김 */
		const std::vector<KEY_EVENT_RECORD> vecKeys = ConsoleUI::Poll_Keys(true);
		if (vecKeys.empty()) return;

		switch (m_eRoomState)
		{
		case STATE::WAITING:				Pre_Update_Waiting(vecKeys);	break;
		case STATE::STARTGAME_COUNTDOWN:	Pre_Update_Countdown(vecKeys);	break;
		case STATE::GAME:					Pre_Update_Game(vecKeys);		break;
		default: break;
		}
	}

	void CScene_Room::Pre_Update_Waiting(const std::vector<KEY_EVENT_RECORD>& vecKeys)
	{
		for (const KEY_EVENT_RECORD& tKey : vecKeys)
		{
			if (tKey.wVirtualKeyCode == VK_F5)		{ Request_StartGame(); continue; }
			if (tKey.wVirtualKeyCode == VK_ESCAPE)	{ Exit_Room(); return; }		/* Scene이 바뀌니 남은 키는 버림 */
			Edit_Chat(tKey);
		}
	}

	void CScene_Room::Pre_Update_Countdown(const std::vector<KEY_EVENT_RECORD>& vecKeys)
	{
		/* 카운트다운이 시작되면 못 나감 : 채팅만 */
		for (const KEY_EVENT_RECORD& tKey : vecKeys)
			Edit_Chat(tKey);
	}

	void CScene_Room::Pre_Update_Game(const std::vector<KEY_EVENT_RECORD>& vecKeys)
	{
		/* 게임 중엔 못 나감. 단계별로 받는 키가 다름 */
		for (const KEY_EVENT_RECORD& tKey : vecKeys)
		{
			switch (m_ePhase)
			{
			case Protocol::GAME_PHASE::VOTE:		Key_Vote(tKey);		break;		/* 1 ~ 7만 (채팅 X) */
			case Protocol::GAME_PHASE::LAST_VOTE:	Key_LastVote(tKey);	break;		/* Y / N만 (채팅 X) */
			default:								Edit_Chat(tKey);	break;		/* 잠겨 있으면 Edit_Chat이 무시 */
			}
		}
	}

	void CScene_Room::Edit_Chat(const KEY_EVENT_RECORD& tKey)
	{
		if (!m_tView.strInputLock.empty()) return;		/* 지금 단계는 채팅 불가 */

		switch (ConsoleUI::Edit_Line(m_tView.strInput, tKey, MAX_CHAT_LEN))
		{
		case ConsoleUI::LINE_INPUT::SUBMIT:
		{
			/* 빈 Text가 가지 못하도록 체크 */
			if (m_tView.strInput.empty()) break;

			Protocol::C_TO_S_CHAT tBody{};
			tBody.iRoomKey = m_tRoomInfo.iRoomKey;
			wcsncpy_s(tBody.wszText, m_tView.strInput.c_str(), _TRUNCATE);
			m_pClientApp->Get_Client()->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_CHAT), tBody);
			m_tView.strInput.clear();
			m_isDirty = true;
			break;
		}
		case ConsoleUI::LINE_INPUT::CHANGED:
			m_isDirty = true;
			break;
		default: break;
		}
	}

	void CScene_Room::Request_StartGame()
	{
		/* 방장만 요청 (서버도 다시 확인) */
		if (m_tRoomInfo.iMasterKey != m_pClientApp->Get_Client()->Get_ClientKey()) return;

		Protocol::C_TO_S_STARTGAME tBody{};
		tBody.iRoomKey = m_tRoomInfo.iRoomKey;
		m_pClientApp->Get_Client()->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_STARTGAME), tBody);
	}

	void CScene_Room::Exit_Room()
	{
		/* 서버에 나간다고 알리고 로비로 (응답 기다리지 않음) */
		if (Net::CClient* pClient = m_pClientApp->Get_Client())
			pClient->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_EXIT_ROOM), nullptr, 0);
		m_pClientApp->Change_Scene(MafiaClient::SCENE::LOBBY);
	}

	void CScene_Room::Key_Vote(const KEY_EVENT_RECORD& tKey)
	{
		/* 숫자 = 고르기만 (바꿀 수 있음), Enter = 확정해서 보냄 (한 번만) */
		if (m_tView.isVoteDone) return;

		if (tKey.wVirtualKeyCode == VK_RETURN)
		{
			if (m_tView.iVoteSel == ConsoleUI::RoomScreenView::VOTE_NONE) { Show_Notice(L"먼저 번호를 고르세요"); return; }

			Protocol::C_TO_S_VOTE tBody{};		/* 무효표면 NONE_SEAT 그대로 */
			tBody.iRoomKey = m_tRoomInfo.iRoomKey;
			if (m_tView.iVoteSel >= 0) tBody.iVoteSeat = static_cast<uint8_t>(m_tView.iVoteSel);
			m_pClientApp->Get_Client()->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_VOTE), tBody);
			m_tView.isVoteDone = true;
			m_isDirty = true;
			return;
		}

		/* ←→ : 후보 안에서 시계 방향으로 돌기 (끝에서 넘어가면 처음으로) */
		if (tKey.wVirtualKeyCode == VK_LEFT || tKey.wVirtualKeyCode == VK_RIGHT)
		{
			const std::vector<int> vecCand = Vote_Candidates();
			const int iCount = static_cast<int>(vecCand.size());
			const auto iter = std::find(vecCand.begin(), vecCand.end(), m_tView.iVoteSel);
			int iIdx = (iter == vecCand.end()) ? 0 : static_cast<int>(iter - vecCand.begin());
			iIdx = (iIdx + (tKey.wVirtualKeyCode == VK_RIGHT ? 1 : iCount - 1)) % iCount;
			m_tView.iVoteSel = vecCand[iIdx];
			m_isDirty = true;
			return;
		}

		/* 숫자 : 그 자리로 바로 (0 = 무효표) */
		const wchar_t ch = tKey.uChar.UnicodeChar;
		if (ch == L'0')
			m_tView.iVoteSel = ConsoleUI::RoomScreenView::VOTE_ABSTAIN;
		else if (ch >= L'1' && ch <= L'0' + Protocol::MAX_ROOM_PLAYER)
		{
			const int iSeat = ch - L'1';					/* 화면 1 ~ 7 → 자리 0 ~ 6 */
			if (m_tView.arrSeats[iSeat].strName.empty())	{ Show_Notice(L"빈 자리입니다"); return; }
			if (iSeat == m_iMySeat)							{ Show_Notice(L"자신에게는 투표할 수 없습니다"); return; }
			if (!Is_Alive(iSeat))							{ Show_Notice(L"죽은 사람에게는 투표할 수 없습니다"); return; }
			m_tView.iVoteSel = iSeat;
		}
		else return;
		m_isDirty = true;
	}

	std::vector<int> CScene_Room::Vote_Candidates() const
	{
		std::vector<int> vecCand;
		for (int i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
			if (!m_tView.arrSeats[i].strName.empty() && i != m_iMySeat && Is_Alive(i))
				vecCand.push_back(i);
		vecCand.push_back(ConsoleUI::RoomScreenView::VOTE_ABSTAIN);
		return vecCand;
	}

	void CScene_Room::Key_LastVote(const KEY_EVENT_RECORD& tKey)
	{
		const wchar_t ch = tKey.uChar.UnicodeChar;
		const bool isYes = (ch == L'y' || ch == L'Y' || tKey.wVirtualKeyCode == 'Y');	/* 한글 상태(ㅛ)여도 Y 키 */
		const bool isNo  = (ch == L'n' || ch == L'N' || tKey.wVirtualKeyCode == 'N');
		if (!isYes && !isNo) return;

		Protocol::C_TO_S_LAST_VOTE tBody{};
		tBody.isAgree = isYes;
		m_pClientApp->Get_Client()->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_LAST_VOTE), tBody);
		Show_Notice(isYes ? L"찬성했습니다" : L"반대했습니다");
	}

	void CScene_Room::Set_State(STATE eState)
	{
		m_eRoomState		= eState;
		m_tView.isWaiting	= (eState == STATE::WAITING);		/* [F5] · [Esc] 안내는 대기 중만 */
		m_tView.isPlaying	= (eState == STATE::GAME);
		if (eState != STATE::GAME)
		{
			m_tView.strHelp.clear();
			m_tView.strInputLock.clear();
			m_tView.isVoting = false;
		}
		m_isDirty = true;
	}

	void CScene_Room::Apply_PhaseRule(Protocol::GAME_PHASE ePhase)
	{
		/* 화면만 : 채팅 잠금 문구 · 조작 안내 (서버도 같은 규칙으로 채팅을 막음) */
		m_tView.strHelp.clear();
		m_tView.strInputLock.clear();
		m_tView.isVoting	= (ePhase == Protocol::GAME_PHASE::VOTE) && Is_Alive(m_iMySeat);	/* 입력 줄 자리에 투표 막대 (죽었으면 X) */
		m_tView.iVoteSel	= m_tView.isVoting ? Vote_Candidates().front() : ConsoleUI::RoomScreenView::VOTE_NONE;	/* 첫 후보부터 */
		m_tView.isVoteDone	= false;

		switch (ePhase)
		{
		case Protocol::GAME_PHASE::VOTE:
			if (m_tView.isVoting) m_tView.strHelp = L"[←→] 고르기   [1~7] 바로 고르기   [0] 무효표   [Enter] 투표";
			m_tView.strInputLock	= L"투표 중에는 채팅을 칠 수 없습니다";		/* 막대가 대신 그려짐 (채팅 잠금 표시용) */
			break;
		case Protocol::GAME_PHASE::ALLOCUTION:
			if (m_iAccusedSeat < 0 || m_iAccusedSeat != m_iMySeat)
				m_tView.strInputLock = L"최후 변론자 외에는 채팅을 칠 수 없습니다";
			break;
		case Protocol::GAME_PHASE::LAST_VOTE:
			m_tView.strHelp			= L"[Y] 찬성  [N] 반대";
			m_tView.strInputLock	= L"찬반 투표 중에는 채팅을 칠 수 없습니다";
			break;
		case Protocol::GAME_PHASE::NIGHT:
			if (m_eJob == Protocol::JOB::MAFIA)
				m_tView.strHelp		= L"밤 채팅은 마피아끼리만 보입니다";
			else
				m_tView.strInputLock	= L"밤에는 채팅을 칠 수 없습니다";
			break;
		case Protocol::GAME_PHASE::LOADING_DAY:
		case Protocol::GAME_PHASE::LOADING_NIGHT:
			m_tView.strInputLock	= L"지금은 채팅을 칠 수 없습니다";
			break;
		default: break;		/* DAY (토론) : 채팅 가능 */
		}

		if (!m_tView.strInputLock.empty()) m_tView.strInput.clear();	/* 치던 글자는 버림 */
		m_isDirty = true;
	}

	void CScene_Room::Apply_RoomInfo()
	{
		const Protocol::S_TO_C_ENTER_ROOM& tInfo = m_tRoomInfo;
		const Net::CClient* pClient = m_pClientApp->Get_Client();
		const uint64_t iMyKey = pClient ? pClient->Get_ClientKey() : 0;

		/* 받은 값은 끝을 막아 둔 복사본으로 */
		wchar_t wszTitle[Protocol::MAX_ROOMNAME_BUF]{};
		wcsncpy_s(wszTitle, tInfo.wszRoomName, _TRUNCATE);

		m_tView.iRoomNo		= static_cast<int>(tInfo.iRoomKey);
		m_tView.strTitle	= wszTitle;
		m_tView.isMaster	= (tInfo.iMasterKey == iMyKey);

		for (size_t i = 0; i < m_tView.arrSeats.size(); ++i)
		{
			const Protocol::ROOM_SEAT& tSeat = tInfo.arrSeats[i];
			ConsoleUI::SeatView& tView = m_tView.arrSeats[i];
			tView = {};
			if (tSeat.iKey == 0) continue;		/* 빈자리 */

			wchar_t wszNick[Protocol::MAX_NICNAME_BUF]{};
			wcsncpy_s(wszNick, tSeat.wszNickname, _TRUNCATE);
			tView.strName	= wszNick;
			tView.isMe		= (tSeat.iKey == iMyKey);
			if (tView.isMe) m_iMySeat = static_cast<int>(i);
			tView.isMaster	= (tSeat.iKey == tInfo.iMasterKey);
			if (tView.isMe && m_eJob != Protocol::JOB::UNKNOWN) tView.strTag = Job_Name(m_eJob);	/* 내 직업 (자리가 갱신돼도 유지) */
			tView.isDead	= m_tView.isPlaying && !Is_Alive(static_cast<int>(i));
		}
		m_isDirty = true;
	}

	void CScene_Room::Update()
	{
		/* 알림 시간이 지나면 지움 */
		if (!m_tView.strNotice.empty() && GetTickCount64() >= m_ullNoticeEnd)
		{
			m_tView.strNotice.clear();
			m_isDirty = true;
		}

		switch (m_eRoomState)
		{
		case STATE::STARTGAME_COUNTDOWN:	Update_Countdown();	break;
		case STATE::GAME:					Update_Game();		break;
		default: break;		/* 대기 : 할 일 없음 */
		}
	}

	void CScene_Room::Update_Countdown()
	{
		/* "5초 후 게임이 시작됩니다" → 4 → … (0초가 돼도 스스로 넘어가지 않음 : 서버의 S_TO_C_STARTGAME을 기다림) */
		const ULONGLONG ullNow = GetTickCount64();
		if (ullNow >= m_ullStartAt) return;

		const int iCount = static_cast<int>((m_ullStartAt - ullNow + 999) / 1000);	/* 남은 초 (올림) */
		if (iCount == m_iLastCount) return;

		m_iLastCount = iCount;
		m_tView.vecChats.push_back({ L"", std::to_wstring(iCount) + L"초 후 게임이 시작됩니다", false, true });
		m_isDirty = true;
	}

	void CScene_Room::Update_Game()
	{
		/* 이번 단계 남은 시간 표시 */
		Update_PhaseTime();
	}

	void CScene_Room::Apply_Phase(Protocol::GAME_PHASE ePhase, uint16_t iDay, uint32_t iRemainMs)
	{
		m_ePhase				= ePhase;
		m_ullPhaseEnd			= GetTickCount64() + iRemainMs;		/* 받은 순간 기준으로 내 시계로 셈 */
		m_iLastRemainSec		= -1;
		m_tView.iDay			= iDay;
		m_tView.isNight			= (ePhase == Protocol::GAME_PHASE::NIGHT || ePhase == Protocol::GAME_PHASE::LOADING_NIGHT);
		Update_PhaseTime();
	}

	void CScene_Room::Update_PhaseTime()
	{
		const ULONGLONG ullNow = GetTickCount64();
		const int iRemainSec = (ullNow >= m_ullPhaseEnd) ? 0 : static_cast<int>((m_ullPhaseEnd - ullNow + 999) / 1000);
		if (iRemainSec == m_iLastRemainSec) return;

		m_iLastRemainSec = iRemainSec;
		wchar_t wszTime[16]{};
		swprintf_s(wszTime, L"%d : %02d", iRemainSec / 60, iRemainSec % 60);
		m_tView.strSub = std::wstring(Phase_Name(m_ePhase)) + L"   " + wszTime;		/* 탁자 가운데 둘째 줄 : "투표   0 : 15" */
		m_isDirty = true;
	}

	void CScene_Room::Show_Notice(const std::wstring& strText)
	{
		m_tView.strNotice = strText;
		m_ullNoticeEnd = GetTickCount64() + NOTICE_MS;
		m_isDirty = true;
	}

	void CScene_Room::Late_Update()
	{
	}

	void CScene_Room::Render()
	{
		if (m_isDirty == false && ConsoleUI::Need_Redraw() == false) return;
		m_isDirty = false;

		ConsoleUI::Render_Room(m_tView);
	}

	std::unique_ptr<CScene_Room> CScene_Room::Create(const Protocol::S_TO_C_ENTER_ROOM& tRoomInfo)
	{
		std::unique_ptr<CScene_Room> pScene{ new CScene_Room() };
		pScene->m_tRoomInfo = tRoomInfo;
		if (FAILED(pScene->Initialize()))
			return nullptr;
		return pScene;
	}

	void CScene_Room::Register_NetEventCallBack()
	{
		/* 기존 Default CallBack함수등록 */
		m_pClientApp->Register_DefaultNetEvents();
		m_pClientApp->Register_NetEvents(Net::NET_EVENT_TYPE::Receive_Packet, [this](const Net::Event& tEvent) { On_Packet(tEvent); });
	}

	void CScene_Room::On_Packet(const Net::Event& tEvent)
	{
		switch (static_cast<Protocol::PACKET_TYPE>(tEvent.Get_PacketType()))
		{
		/* 자리 바뀜 : 예전 자리와 비교해서 들어온 / 나간 사람 알림 → 내 방 정보(m_tRoomInfo) 갱신 → 다시 그림 */
		case Protocol::PACKET_TYPE::S_TO_C_ROOM_MEMBERS:
		{
			if (tEvent.Get_BodyLen() != sizeof(Protocol::S_TO_C_ROOM_MEMBERS)) break;
			Protocol::S_TO_C_ROOM_MEMBERS tMembers{};
			memcpy(&tMembers, tEvent.Get_Body(), sizeof(tMembers));

			Protocol::S_TO_C_ENTER_ROOM& tInfo = m_tRoomInfo;
			for (size_t i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
			{
				Protocol::ROOM_SEAT& tOld = tInfo.arrSeats[i];
				Protocol::ROOM_SEAT& tNew = tMembers.arrSeats[i];
				tOld.wszNickname[Protocol::MAX_NICNAME_BUF - 1] = L'\0';
				tNew.wszNickname[Protocol::MAX_NICNAME_BUF - 1] = L'\0';

				if (tOld.iKey == tNew.iKey) continue;
				if (tOld.iKey != 0) m_tView.vecChats.push_back({ L"", std::wstring(tOld.wszNickname) + L" 님이 나갔습니다", false, true });
				if (tNew.iKey != 0) m_tView.vecChats.push_back({ L"", std::wstring(tNew.wszNickname) + L" 님이 들어왔습니다", false, true });
			}
			if (tInfo.iMasterKey != tMembers.iMasterKey)
				m_tView.vecChats.push_back({ L"", L"방장이 바뀌었습니다", false, true });

			tInfo.iMasterKey = tMembers.iMasterKey;
			memcpy(tInfo.arrSeats, tMembers.arrSeats, sizeof(tInfo.arrSeats));
			Apply_RoomInfo();
			break;
		}
		/* 게임 시작 가능 여부 패킷(방장만 체크) */
		case Protocol::PACKET_TYPE::S_TO_C_STARTGAME_RESULT:
		{
			const Protocol::S_TO_C_STARTGAME_RESULT* pData = reinterpret_cast<const Protocol::S_TO_C_STARTGAME_RESULT*>(tEvent.Get_Body());
			if (pData == nullptr || tEvent.Get_BodyLen() != sizeof(Protocol::S_TO_C_STARTGAME_RESULT)) break;

			switch (pData->eResult)
			{
			case Protocol::STARTGAME_RESULT::OK: /* 게임 시작 가능 : 방 전원 카운트다운 (이때부터 못 나감) */
			{
				m_ullStartAt = GetTickCount64() + Protocol::START_COUNTDOWN_MS;
				m_iLastCount = 0;
				Set_State(STATE::STARTGAME_COUNTDOWN);
				break;
			}
			case Protocol::STARTGAME_RESULT::NOT_FULL:		
			{
				int iSeated{ 0 };
				for (const ConsoleUI::SeatView& tSeat : m_tView.arrSeats) if (!tSeat.strName.empty()) ++iSeated;
				Show_Notice(L"인원이 부족합니다 (" + std::to_wstring(iSeated) + L"/" + std::to_wstring(Protocol::MAX_ROOM_PLAYER) + L")");

				break;
			}
			case Protocol::STARTGAME_RESULT::SERVER_ERROR:			
			{
				Show_Notice(L"게임을 시작할 수 없습니다");
				break;
			}
			default: break;
			}



			break;
		}
		/* 5초가 지난 뒤 서버에서 판단해서 줌 */
		case Protocol::PACKET_TYPE::S_TO_C_STARTGAME:
		{
			if (tEvent.Get_BodyLen() != sizeof(Protocol::S_TO_C_STARTGAME)) break;
			Protocol::S_TO_C_STARTGAME tStart{};
			memcpy(&tStart, tEvent.Get_Body(), sizeof(tStart));

			if (tStart.eResult != Protocol::STARTGAME_RESULT::OK)
			{
				/* 카운트다운 중 인원이 빠져 취소 → 다시 대기 */
				Set_State(STATE::WAITING);
				m_tView.vecChats.push_back({ L"", L"인원이 부족해 시작이 취소되었습니다", false, true });
				Show_Notice(L"게임 시작이 취소되었습니다");
				break;
			}

			/* 게임 시작 : 내 직업 + 첫 단계 */
			m_eJob = tStart.eJob;
			m_iAccusedSeat = -1;
			m_iAliveMask = 0x7F;		/* 시작은 전원 생존 (다음 CHANGEPHASE부터 서버 값) */
			Set_State(STATE::GAME);
			m_tView.vecChats.push_back({ L"", L"게임이 시작되었습니다", false, true });
			m_tView.vecChats.push_back({ L"", std::wstring(L"당신의 직업은 ") + Job_Name(m_eJob) + L" 입니다", false, true });
			Apply_RoomInfo();		/* 내 의자 아래에 직업 표시 */
			Apply_Phase(tStart.ePhase, tStart.iDay, tStart.iRemainMs);
			Apply_PhaseRule(tStart.ePhase);
			break;
		}
		/* 투표 결과 : 지목된 자리 (없으면 바로 밤으로) */
		case Protocol::PACKET_TYPE::S_TO_C_VOTE_RESULT:
		{
			if (tEvent.Get_BodyLen() != sizeof(Protocol::S_TO_C_VOTE_RESULT)) break;
			Protocol::S_TO_C_VOTE_RESULT tResult{};
			memcpy(&tResult, tEvent.Get_Body(), sizeof(tResult));

			if (tResult.iSeat >= Protocol::MAX_ROOM_PLAYER || m_tView.arrSeats[tResult.iSeat].strName.empty())
			{
				m_iAccusedSeat = -1;
				m_tView.vecChats.push_back({ L"", L"투표 결과 지목된 사람이 없습니다", false, true });
			}
			else
			{
				m_iAccusedSeat = tResult.iSeat;
				m_tView.vecChats.push_back({ L"", m_tView.arrSeats[tResult.iSeat].strName + L" 님이 지목되었습니다", false, true });
			}
			m_isDirty = true;
			break;
		}
		case Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA:
		{
			const Protocol::PROVIDE_DATA* pData = reinterpret_cast<const Protocol::PROVIDE_DATA*>(tEvent.Get_Body());
			if (pData == nullptr) break;
			
			if (pData->eDataType == Protocol::DATA_TYPE::CHAT_LOG)
			{
				if (tEvent.Get_BodyLen() != sizeof(Protocol::CHAT_LOG)) break;		/* 크기 안 맞으면 버림 (짧은 패킷이면 memcpy가 범위 밖을 읽음) */
				Protocol::CHAT_LOG tChat{};
				memcpy(&tChat, tEvent.Get_Body() , sizeof(tChat));                   // 복사본에서
				tChat.wszName[Protocol::MAX_NICNAME_BUF - 1] = L'\0';                // 끝 막기
				tChat.wszText[Protocol::MAX_CHAT_BUF - 1] = L'\0';

				m_tView.vecChats.push_back({ tChat.wszName, tChat.wszText, false , false });
				m_isDirty = true;
			}

			break;
		}
		case Protocol::PACKET_TYPE::S_TO_C_CHANGEPHASE:
		{
			if (tEvent.Get_BodyLen() != sizeof(Protocol::S_TO_C_CHANGEPHASE)) break;
			Protocol::S_TO_C_CHANGEPHASE tBody{}; memcpy(&tBody, tEvent.Get_Body(), sizeof(tBody));
			
			m_iAliveMask = tBody.iAliveMask;
			Apply_RoomInfo();		/* 죽은 의자 회색 */
			Apply_Phase(tBody.eCurPhase, tBody.iDay, tBody.iRemainMs);
			Apply_PhaseRule(tBody.eCurPhase);
			m_tView.vecChats.push_back({ L"", Phase_Message(tBody.eCurPhase), false, true });

			break;
		}


		default: break;
		}
	}
}
