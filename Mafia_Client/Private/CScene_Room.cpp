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
			case Protocol::JOB::PRESIDENT:	return L"대통령";
			case Protocol::JOB::SNOOP:		return L"탐정";
			case Protocol::JOB::BODYGUARD:	return L"경호원";
			case Protocol::JOB::MEDIUM:		return L"영매";
			case Protocol::JOB::SOLDIER:	return L"군인";
			case Protocol::JOB::BETRAYER:	return L"배신자";
			default:						return L"?";
			}
		}

		/* 의자 위에 띄우는 직업 (아이콘 + 이름) : 아이콘은 Char_Width가 2칸으로 세는 범위(U+1F300 ~)만 */
		std::wstring Job_Role(Protocol::JOB eJob)
		{
			switch (eJob)
			{
			case Protocol::JOB::CITIZEN:	return L"👤 시민";
			case Protocol::JOB::MAFIA:		return L"🔪 마피아";
			case Protocol::JOB::PRESIDENT:	return L"👑 대통령";
			case Protocol::JOB::SNOOP:		return L"🔍 탐정";
			case Protocol::JOB::BODYGUARD:	return L"💂 경호원";
			case Protocol::JOB::MEDIUM:		return L"🔮 영매";
			case Protocol::JOB::SOLDIER:	return L"🔫 군인";
			case Protocol::JOB::BETRAYER:	return L"🎭 배신자";
			default:						return L"";
			}
		}

		/* 빨간 글씨 = 마피아 팀 */
		bool Is_RedJob(Protocol::JOB eJob) { return eJob == Protocol::JOB::MAFIA || eJob == Protocol::JOB::BETRAYER; }

		/* 직업 설명 (게임 시작 때 채팅에 한 줄) */
		const wchar_t* Job_Desc(Protocol::JOB eJob)
		{
			switch (eJob)
			{
			case Protocol::JOB::MAFIA:		return L"밤마다 한 명을 처치합니다";
			case Protocol::JOB::PRESIDENT:	return L"투표로 연행되지 않고, 밤마다 방 전원에게 공지를 남길 수 있습니다";
			case Protocol::JOB::SNOOP:		return L"둘째 밤부터 밤마다 한 명이 마피아인지 조사합니다";
			case Protocol::JOB::BODYGUARD:	return L"밤마다 한 명을 마피아로부터 지킵니다";
			case Protocol::JOB::MEDIUM:		return L"밤마다 죽은 사람 한 명의 직업을 봅니다";
			case Protocol::JOB::SOLDIER:	return L"마피아의 공격을 한 번 버팁니다";
			case Protocol::JOB::BETRAYER:	return L"한 번 직업을 확인하고, 밤마다 마피아와 접선을 시도합니다";
			default:						return L"";
			}
		}

		const wchar_t* Phase_Name(Protocol::GAME_PHASE ePhase)
		{
			switch (ePhase)
			{
			case Protocol::GAME_PHASE::NIGHT:			return L"밤";
			case Protocol::GAME_PHASE::LOADING_DAY:		return L"동틀 녘";
			case Protocol::GAME_PHASE::DAY:				return L"아침";
			case Protocol::GAME_PHASE::VOTE:			return L"투표";
			case Protocol::GAME_PHASE::ALLOCUTION:		return L"최후의 변론";
			case Protocol::GAME_PHASE::LAST_VOTE:		return L"찬반 투표";
			case Protocol::GAME_PHASE::LOADING_NIGHT:	return L"해 질 녘";
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

		/* 내 직업 값이 메모리에서 바뀌면 꺼낼 때 알림 (실제 판정은 서버가 하므로 화면 알림만) */
		Net::Protected<Protocol::JOB>::s_fnOnTampered = [this] { Show_Notice(L"메모리 변조가 감지되었습니다"); };

		Apply_RoomInfo();
		m_tView.vecChats.push_back({ L"", L"방에 들어왔습니다", false, true });

		ConsoleUI::Set_CursorVisible(false);
		ConsoleUI::Clear();
		m_isDirty = true;
	}

	void CScene_Room::Exit()
	{
		Net::Protected<Protocol::JOB>::s_fnOnTampered = nullptr;
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
		case STATE::RESULT:					Pre_Update_Result(vecKeys);		break;
		default: break;
		}
	}

	void CScene_Room::Pre_Update_Result(const std::vector<KEY_EVENT_RECORD>& vecKeys)
	{
		if (m_tView.iWinFrame < ConsoleUI::RoomScreenView::WIN_FRAMES) return;	/* 연출 중엔 키 무시 */
		for (const KEY_EVENT_RECORD& tKey : vecKeys)
		{
			if (tKey.wVirtualKeyCode != VK_RETURN) continue;
			Set_State(STATE::WAITING);			/* 결과 창 닫고 대기 화면 (채팅 · F5 · Esc 다시 가능) */
			m_tView.vecChats.push_back({ L"", L"대기실로 돌아왔습니다", false, true });
			return;
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
			case Protocol::GAME_PHASE::NIGHT:		Key_Night(tKey);	break;		/* 능력 고르기 / 공지 / 마피아 채팅 */
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
		if (!Key_Pick(tKey, Vote_Candidates())) return;

		Protocol::C_TO_S_VOTE tBody{};		/* 무효표면 NONE_SEAT 그대로 */
		tBody.iRoomKey = m_tRoomInfo.iRoomKey;
		if (m_tView.iVoteSel >= 0) tBody.iVoteSeat = m_tView.iVoteSel;
		m_pClientApp->Get_Client()->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_VOTE), tBody);
		m_tView.isVoteDone = true;
		m_tView.strHelp.clear();			/* 더는 못 바꿈 : 조작 안내 지우기 */
		m_isDirty = true;
	}

	bool CScene_Room::Key_Pick(const KEY_EVENT_RECORD& tKey, const std::vector<int>& vecCand)
	{
		/* 숫자 · ←→ = 고르기만 (바꿀 수 있음), Enter = 확정 (한 번만 : 보내는 건 부른 쪽) */
		if (m_tView.isVoteDone) return false;

		if (tKey.wVirtualKeyCode == VK_RETURN)
		{
			if (m_tView.iVoteSel == ConsoleUI::RoomScreenView::VOTE_NONE) { Show_Notice(L"먼저 번호를 고르세요"); return false; }
			return true;
		}

		/* ←→ (↑↓) : 후보 안에서 다음 / 이전 (끝에서 넘어가면 처음으로) */
		const WORD wVK = tKey.wVirtualKeyCode;
		if (wVK == VK_LEFT || wVK == VK_RIGHT || wVK == VK_UP || wVK == VK_DOWN)
		{
			const int iCount = static_cast<int>(vecCand.size());
			if (iCount == 0) return false;
			const auto iter = std::find(vecCand.begin(), vecCand.end(), m_tView.iVoteSel);
			int iIdx = (iter == vecCand.end()) ? 0 : static_cast<int>(iter - vecCand.begin());
			iIdx = (iIdx + ((wVK == VK_RIGHT || wVK == VK_DOWN) ? 1 : iCount - 1)) % iCount;
			m_tView.iVoteSel = vecCand[iIdx];
			m_isDirty = true;
			return false;
		}

		/* 숫자 : 그 자리로 바로 (0 = 무효표, 투표만) */
		const wchar_t ch = tKey.uChar.UnicodeChar;
		if (ch == L'0' && m_tView.isPickAbstain)
			m_tView.iVoteSel = ConsoleUI::RoomScreenView::VOTE_ABSTAIN;
		else if (ch >= L'1' && ch <= L'0' + Protocol::MAX_ROOM_PLAYER)
		{
			const int iSeat = ch - L'1';					/* 화면 1 ~ 7 → 자리 0 ~ 6 */
			if (std::find(vecCand.begin(), vecCand.end(), iSeat) == vecCand.end())
			{
				if (m_tView.arrSeats[iSeat].strName.empty())	Show_Notice(L"빈 자리입니다");
				else if (iSeat == m_iMySeat)					Show_Notice(L"자신은 고를 수 없습니다");
				else if (!Is_Alive(iSeat))						Show_Notice(L"죽은 사람은 고를 수 없습니다");
				else											Show_Notice(L"고를 수 없는 자리입니다");
				return false;
			}
			m_tView.iVoteSel = iSeat;
		}
		else return false;
		m_isDirty = true;
		return false;
	}

	std::vector<CScene_Room::NIGHT_MODE> CScene_Room::Night_Modes() const
	{
		std::vector<NIGHT_MODE> vecModes;
		if (!Is_Alive(m_iMySeat)) return vecModes;

		switch (m_eJob)
		{
		case Protocol::JOB::MAFIA:		if (!m_isSkillDone) vecModes.push_back(NIGHT_MODE::KILL);	vecModes.push_back(NIGHT_MODE::CHAT);	break;
		case Protocol::JOB::SNOOP:		if (!m_isSkillDone && m_tView.iDay >= 1) vecModes.push_back(NIGHT_MODE::SNOOP);	break;	/* 첫날 밤엔 조사 불가 (서버도 막음) */
		case Protocol::JOB::BODYGUARD:	if (!m_isSkillDone) vecModes.push_back(NIGHT_MODE::GUARD);	break;
		case Protocol::JOB::MEDIUM:		if (!m_isSkillDone) vecModes.push_back(NIGHT_MODE::MEDIUM);	break;
		case Protocol::JOB::PRESIDENT:	if (!m_isSkillDone) vecModes.push_back(NIGHT_MODE::NOTICE);	break;
		case Protocol::JOB::BETRAYER:	/* 직업 확인(게임 중 1회)과 접선(밤마다)은 따로 : 같은 밤에 둘 다 가능 */
			if (!m_isFindJobUsed)					vecModes.push_back(NIGHT_MODE::FIND_JOB);
			if (!m_isContacted && !m_isSkillDone)	vecModes.push_back(NIGHT_MODE::CONTACT);
			if (m_isContacted)						vecModes.push_back(NIGHT_MODE::CHAT);
			break;
		default: break;		/* 군인 · 시민 : 밤에 할 일 없음 */
		}
		return vecModes;
	}

	std::vector<int> CScene_Room::Night_Candidates() const
	{
		std::vector<int> vecCand;
		for (int i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
		{
			if (m_tView.arrSeats[i].strName.empty()) continue;
			switch (m_eNightMode)
			{
			case NIGHT_MODE::GUARD:		if (Is_Alive(i)) vecCand.push_back(i);						break;	/* 자기 자신도 지킬 수 있음 */
			case NIGHT_MODE::MEDIUM:	if (!Is_Alive(i)) vecCand.push_back(i);						break;	/* 죽은 사람만 */
			case NIGHT_MODE::KILL:
			case NIGHT_MODE::SNOOP:
			case NIGHT_MODE::FIND_JOB:
			case NIGHT_MODE::CONTACT:	if (Is_Alive(i) && i != m_iMySeat) vecCand.push_back(i);	break;
			default: break;
			}
		}
		return vecCand;
	}

	void CScene_Room::Key_Night(const KEY_EVENT_RECORD& tKey)
	{
		if (!Is_Alive(m_iMySeat)) return;

		/* Tab : 다음 모드 (지금 모드가 끝나서 목록에 없으면 첫 모드로) */
		if (tKey.wVirtualKeyCode == VK_TAB)
		{
			const std::vector<NIGHT_MODE> vecModes = Night_Modes();
			if (vecModes.empty()) return;
			const auto iter = std::find(vecModes.begin(), vecModes.end(), m_eNightMode);
			if (iter != vecModes.end() && vecModes.size() == 1) return;
			const size_t iNext = (iter == vecModes.end()) ? 0 : (static_cast<size_t>(iter - vecModes.begin()) + 1) % vecModes.size();
			Apply_NightMode(vecModes[iNext]);
			return;
		}

		switch (m_eNightMode)
		{
		case NIGHT_MODE::CHAT:
			Edit_Chat(tKey);
			break;
		case NIGHT_MODE::NOTICE:		/* 대통령 : 입력 줄에 공지를 써서 Enter */
		{
			if (m_isSkillDone) break;
			const ConsoleUI::LINE_INPUT eInput = ConsoleUI::Edit_Line(m_tView.strInput, tKey, MAX_CHAT_LEN);
			if (eInput == ConsoleUI::LINE_INPUT::CHANGED) m_isDirty = true;
			if (eInput != ConsoleUI::LINE_INPUT::SUBMIT || m_tView.strInput.empty()) break;

			Protocol::PRESIDENT_ACTION tBody{};
			tBody.iRoomKey	= m_tRoomInfo.iRoomKey;
			tBody.eJob		= m_eJob;
			wcsncpy_s(tBody.tNotice.wszText, m_tView.strInput.c_str(), _TRUNCATE);
			m_pClientApp->Get_Client()->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_JOB_ACTION), tBody);

			m_isSkillDone			= true;
			m_tView.strInput.clear();
			m_tView.strHelp.clear();
			m_tView.strInputLock	= L"공지를 보냈습니다. 아침까지 기다려 주세요";
			m_isDirty				= true;
			break;
		}
		default:						/* 고르기 막대 */
			if (Key_Pick(tKey, Night_Candidates())) Send_Skill();
			break;
		}
	}

	void CScene_Room::Apply_NightMode(NIGHT_MODE eMode)
	{
		m_eNightMode = eMode;
		const bool isPick = (eMode != NIGHT_MODE::CHAT && eMode != NIGHT_MODE::NOTICE);

		const wchar_t* szVerb = L"";
		int iColor = 0;
		switch (eMode)
		{
		case NIGHT_MODE::KILL:		szVerb = L"처치";	iColor = 0;	break;
		case NIGHT_MODE::SNOOP:		szVerb = L"조사";	iColor = 2;	break;
		case NIGHT_MODE::GUARD:		szVerb = L"경호";	iColor = 1;	break;
		case NIGHT_MODE::MEDIUM:	szVerb = L"확인";	iColor = 2;	break;
		case NIGHT_MODE::FIND_JOB:	szVerb = L"확인";	iColor = 2;	break;
		case NIGHT_MODE::CONTACT:	szVerb = L"접선";	iColor = 0;	break;
		default: break;
		}

		const std::vector<int> vecCand = isPick ? Night_Candidates() : std::vector<int>{};
		m_tView.strInput.clear();
		m_tView.isVoting		= isPick;
		m_tView.isVoteDone		= false;
		m_tView.isPickAbstain	= false;
		m_tView.strPickVerb		= szVerb;
		m_tView.iPickColor		= iColor;
		m_tView.iVoteSel		= vecCand.empty() ? ConsoleUI::RoomScreenView::VOTE_NONE : vecCand.front();
		m_tView.iPickMask		= 0;
		for (int iSeat : vecCand) m_tView.iPickMask |= static_cast<uint8_t>(1u << iSeat);	/* 목록에서 고를 수 있는 줄 */
		m_tView.strInputLabel	= (eMode == NIGHT_MODE::NOTICE) ? L"공지 ▸ " : L"▸ ";
		m_tView.strInputLock	= isPick ? L"능력을 쓰는 중입니다" : L"";		/* 막대가 대신 그려짐 */

		/* 안내 : [Tab] (모드가 둘 이상) + 모드별 조작 */
		std::wstring strHelp = (Night_Modes().size() > 1) ? L"[Tab] 모드 바꾸기   " : L"";
		switch (eMode)
		{
		case NIGHT_MODE::CHAT:		strHelp += L"밤 채팅은 마피아 편끼리만 보입니다";				break;
		case NIGHT_MODE::NOTICE:	strHelp += L"공지를 쓰고 [Enter] → 방 전원에게 보입니다";		break;
		case NIGHT_MODE::FIND_JOB:	strHelp += L"(게임 중 한 번) 직업 확인   ";						[[fallthrough]];
		default:
			strHelp += vecCand.empty() ? L"고를 수 있는 사람이 없습니다"
									   : L"[↑↓] 고르기   [1~7] 바로 고르기   [Enter] " + std::wstring(szVerb);
			break;
		}
		m_tView.strHelp = strHelp;
		m_isDirty = true;
	}

	void CScene_Room::Send_Skill()
	{
		const int iSel = m_tView.iVoteSel;
		if (iSel < 0) return;

		/* 직업별 구조체 : 머리(방 키 · 직업) + 대상. 서버가 길이 · 직업을 다시 확인 */
		auto Send = [&](auto& tBody)
		{
			tBody.iRoomKey		= m_tRoomInfo.iRoomKey;
			tBody.eJob			= m_eJob;
			tBody.tTarget.iSeat	= iSel;
			m_pClientApp->Get_Client()->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_JOB_ACTION), tBody);
		};
		switch (m_eNightMode)
		{
		case NIGHT_MODE::KILL:		{ Protocol::MAFIA_ACTION		tBody{}; Send(tBody); break; }
		case NIGHT_MODE::SNOOP:		{ Protocol::SNOOP_ACTION		tBody{}; Send(tBody); break; }
		case NIGHT_MODE::GUARD:		{ Protocol::BODYGUARD_ACTION	tBody{}; Send(tBody); break; }
		case NIGHT_MODE::MEDIUM:	{ Protocol::MEDIUM_ACTION		tBody{}; Send(tBody); break; }
		case NIGHT_MODE::FIND_JOB:
		case NIGHT_MODE::CONTACT:	{ Protocol::BETRAYER_ACTION		tBody{}; tBody.isContact = (m_eNightMode == NIGHT_MODE::CONTACT); Send(tBody); break; }
		default: return;
		}

		if (m_eNightMode == NIGHT_MODE::FIND_JOB)	m_isFindJobUsed = true;
		else										m_isSkillDone = true;
		m_tView.isVoteDone	= true;
		m_tView.strHelp		= (Night_Modes().empty()) ? L"" : L"[Tab] 모드 바꾸기";
		m_isDirty			= true;
	}

	void CScene_Room::Know_Job(int iSeat, Protocol::JOB eJob)
	{
		if (iSeat < 0 || iSeat >= Protocol::MAX_ROOM_PLAYER) return;
		m_arrKnownJob[iSeat] = eJob;
		Apply_RoomInfo();
	}

	void CScene_Room::On_JobData(const Net::Event& tEvent, Protocol::DATA_TYPE eType)
	{
		/* 자리 → "3번 민수" */
		auto Seat_Name = [&](INT32 iSeat) -> std::wstring
		{
			if (iSeat < 0 || iSeat >= Protocol::MAX_ROOM_PLAYER) return L"?";
			return m_tView.arrSeats[iSeat].strName;		/* 게임 중이라 "N번" */
		};

		switch (eType)
		{
		case Protocol::DATA_TYPE::SNOOP_RESULT:
		{
			if (tEvent.Get_BodyLen() != sizeof(Protocol::SNOOP_RESULT)) return;
			Protocol::SNOOP_RESULT tResult{}; memcpy(&tResult, tEvent.Get_Body(), sizeof(tResult));
			const INT32 iSeat = tResult.tTarget.iSeat;
			if (iSeat < 0 || iSeat >= Protocol::MAX_ROOM_PLAYER) return;

			m_arrKnownMafia[iSeat] = tResult.isMafia;
			m_arrKnownClean[iSeat] = !tResult.isMafia;
			m_tView.vecChats.push_back({ L"", L"🔍 조사 결과 : " + Seat_Name(iSeat) + (tResult.isMafia ? L" 님은 마피아입니다!" : L" 님은 마피아가 아닙니다"), false, true });
			Apply_RoomInfo();
			break;
		}
		case Protocol::DATA_TYPE::MEDIUM_RESULT:
		{
			if (tEvent.Get_BodyLen() != sizeof(Protocol::MEDIUM_RESULT)) return;
			Protocol::MEDIUM_RESULT tResult{}; memcpy(&tResult, tEvent.Get_Body(), sizeof(tResult));
			m_tView.vecChats.push_back({ L"", L"🔮 " + Seat_Name(tResult.tTarget.iSeat) + L" 님의 직업은 " + Job_Name(tResult.eJob) + L" 이었습니다", false, true });
			Know_Job(tResult.tTarget.iSeat, tResult.eJob);
			break;
		}
		case Protocol::DATA_TYPE::BETRAYER_RESULT:
		{
			if (tEvent.Get_BodyLen() != sizeof(Protocol::BETRAYER_RESULT)) return;
			Protocol::BETRAYER_RESULT tResult{}; memcpy(&tResult, tEvent.Get_Body(), sizeof(tResult));
			const INT32 iSeat = tResult.tTarget.iSeat;

			if (m_eJob == Protocol::JOB::MAFIA)		/* 마피아가 받음 = 배신자가 접선해 옴 (tTarget = 배신자 자리) */
			{
				m_isContacted = true;
				m_tView.vecChats.push_back({ L"", L"🎭 배신자 " + Seat_Name(iSeat) + L" 님이 접선했습니다. 밤에 같이 대화할 수 있습니다", false, true });
				Know_Job(iSeat, Protocol::JOB::BETRAYER);
			}
			else if (!tResult.isContact)			/* 배신자 직업 확인 */
			{
				m_tView.vecChats.push_back({ L"", L"🎭 " + Seat_Name(iSeat) + L" 님의 직업은 " + Job_Name(tResult.eJob) + L" 입니다", false, true });
				Know_Job(iSeat, tResult.eJob);
			}
			else if (tResult.isSucceeded)			/* 접선 성공 : 바로 마피아와 채팅 */
			{
				m_isContacted = true;
				m_tView.vecChats.push_back({ L"", L"🎭 접선 성공! " + Seat_Name(iSeat) + L" 님이 마피아입니다. 이제 밤에 같이 대화할 수 있습니다", false, true });
				Know_Job(iSeat, Protocol::JOB::MAFIA);
				if (m_ePhase == Protocol::GAME_PHASE::NIGHT) Apply_NightMode(NIGHT_MODE::CHAT);
			}
			else
				m_tView.vecChats.push_back({ L"", L"🎭 접선 실패 : " + Seat_Name(iSeat) + L" 님은 마피아가 아닙니다", false, true });
			break;
		}
		case Protocol::DATA_TYPE::PRESIDENT_NOTICE:
		{
			if (tEvent.Get_BodyLen() != sizeof(Protocol::PRESIDENT_NOTICE)) return;
			Protocol::PRESIDENT_NOTICE tNotice{}; memcpy(&tNotice, tEvent.Get_Body(), sizeof(tNotice));
			tNotice.tNotice.wszText[Protocol::MAX_CHAT_BUF - 1] = L'\0';
			m_tView.strPresident = tNotice.tNotice.wszText;		/* 왼쪽 위 띠 (채팅엔 안 올림, 새 공지가 오면 바뀜) */
			Show_Notice(L"대통령 공지가 도착했습니다");
			break;
		}
		default: return;
		}
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
		if (m_isLastVoteDone) return;		/* 한 번만 */
		if (m_iMySeat == m_iAccusedSeat || !Is_Alive(m_iMySeat)) { Show_Notice(L"찬반 투표에 참여할 수 없습니다"); return; }

		Protocol::C_TO_S_LAST_VOTE tBody{};
		tBody.iRoomKey = m_tRoomInfo.iRoomKey;
		tBody.isAgree = isYes;
		m_pClientApp->Get_Client()->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_LAST_VOTE), tBody);
		Show_Notice(isYes ? L"찬성했습니다" : L"반대했습니다");
		m_isLastVoteDone	= true;
		m_tView.strHelp		= L"다음 단계까지 기다려 주세요";
		m_isDirty			= true;
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
			m_tView.strTally.clear();
		}
		if (eState == STATE::RESULT)
			m_tView.strInputLock = L"결과 창에서 [Enter] 를 누르면 대기실로 돌아갑니다";
		else
		{
			m_tView.iWinFrame = -1;				/* 결과 창 닫기 (Enter, 또는 그 사이 방장이 새 게임을 시작함) */
			m_tView.vecResult.clear();
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
		m_tView.strTally.clear();
		m_tView.strPickVerb		= L"투표";		/* 막대 기본 = 투표 (밤이면 Apply_NightMode가 바꿈) */
		m_tView.iPickColor		= 0;
		m_tView.isPickAbstain	= true;
		m_tView.strInputLabel	= L"▸ ";
		m_tView.isVoteTable		= (ePhase == Protocol::GAME_PHASE::VOTE || ePhase == Protocol::GAME_PHASE::LAST_VOTE);
		m_tView.isSeatList		= (ePhase == Protocol::GAME_PHASE::NIGHT || ePhase == Protocol::GAME_PHASE::VOTE);	/* 밤 · 투표 : 원탁 대신 자리 목록 */
		m_tView.iPickMask		= 0;

		switch (ePhase)
		{
		case Protocol::GAME_PHASE::VOTE:
			if (m_tView.isVoting) m_tView.strHelp = L"[↑↓] 고르기   [1~7] 바로 고르기   [0] 무효표   [Enter] 투표";
			for (int iSeat : Vote_Candidates())		/* 목록에서 고를 수 있는 줄 (무효표 줄은 따로) */
				if (iSeat >= 0) m_tView.iPickMask |= static_cast<uint8_t>(1u << iSeat);
			m_tView.strInputLock	= L"투표 중에는 채팅을 칠 수 없습니다";		/* 막대가 대신 그려짐 (채팅 잠금 표시용) */
			for (int i = 0; i < static_cast<int>(m_tView.arrSeats.size()); ++i)	/* 살아 있는 의자 아래 "투표 : 0 표" (단계 바뀌면 Apply_RoomInfo가 지움) */
				if (!m_tView.arrSeats[i].strName.empty() && Is_Alive(i)) m_tView.arrSeats[i].iVotes = 0;
			break;
		case Protocol::GAME_PHASE::ALLOCUTION:
			if (m_iAccusedSeat < 0 || m_iAccusedSeat != m_iMySeat)
				m_tView.strInputLock = L"최후 변론자 외에는 채팅을 칠 수 없습니다";
			break;
		case Protocol::GAME_PHASE::LAST_VOTE:
			m_iAgree = m_iDisagree	= 0;
			m_isLastVoteDone		= (m_iMySeat == m_iAccusedSeat) || !Is_Alive(m_iMySeat);	/* 지목된 사람 · 죽은 사람은 못 함 */
			m_tView.strTally		= L"찬성 0   반대 0";
			m_tView.strHelp			= m_isLastVoteDone ? L"찬반 투표가 진행 중입니다" : L"[Y] 찬성  [N] 반대";
			m_tView.strInputLock	= L"찬반 투표 중에는 채팅을 칠 수 없습니다";
			break;
		case Protocol::GAME_PHASE::NIGHT:		/* 직업별 능력 막대 / 공지 / 마피아 채팅 (첫 모드부터) */
		{
			m_isSkillDone = false;
			const std::vector<NIGHT_MODE> vecModes = Night_Modes();
			if (vecModes.empty())
				m_tView.strInputLock	= (m_eJob == Protocol::JOB::SNOOP && m_tView.iDay < 1 && Is_Alive(m_iMySeat))
										? L"탐정은 둘째 밤부터 조사할 수 있습니다" : L"밤에는 채팅을 칠 수 없습니다";
			else
				Apply_NightMode(vecModes.front());
			break;
		}
		case Protocol::GAME_PHASE::LOADING_DAY:
		case Protocol::GAME_PHASE::LOADING_NIGHT:
			m_tView.strInputLock	= L"지금은 채팅을 칠 수 없습니다";
			break;
		default: break;		/* DAY (토론) : 채팅 가능 */
		}

		if (!Is_Alive(m_iMySeat))		/* 죽으면 단계와 상관없이 채팅 X (서버도 막음) */
		{
			m_tView.isVoting = false;
			m_tView.strHelp.clear();
			m_tView.strInputLock	= L"죽은 사람은 채팅을 칠 수 없습니다";
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
			tView.strName	= m_tView.isPlaying ? std::to_wstring(i + 1) + L"번" : std::wstring(wszNick);	/* 게임 중엔 누군지 모르게 자리 번호만 */
			tView.isMe		= (tSeat.iKey == iMyKey);
			if (tView.isMe) m_iMySeat = static_cast<int>(i);
			tView.isMaster	= !m_tView.isPlaying && (tSeat.iKey == tInfo.iMasterKey);	/* 방장 ★은 대기실에서만 */
			if (tView.isMe)
			{
				tView.strRole	= Job_Role(m_eJob);	/* 내 직업 : 의자 위 (자리가 갱신돼도 유지) */
				tView.isRoleRed	= Is_RedJob(m_eJob);
			}
			else if (m_arrKnownJob[i] != Protocol::JOB::UNKNOWN)	/* 능력으로 알아낸 직업 (영매 · 배신자 · 접선 · 군인 공개) */
			{
				tView.strRole	= Job_Role(m_arrKnownJob[i]);
				tView.isRoleRed	= Is_RedJob(m_arrKnownJob[i]);
			}
			else if (m_arrKnownMafia[i] || m_arrKnownClean[i])		/* 탐정 결과 : 직업은 모름 */
			{
				tView.strRole	= m_arrKnownMafia[i] ? L"🔍 마피아" : L"🔍 무고";
				tView.isRoleRed	= m_arrKnownMafia[i];
			}
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
		case STATE::RESULT:					Update_Result();	break;
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
		Update_Death();
	}

	void CScene_Room::Update_Result()
	{
		/* 마지막 연행 연출이 있으면 그게 먼저 (연행 → 신문 1면 / 마피아 초상화 순서) */
		Update_Death();
		if (m_isFxPlaying) return;

		const ULONGLONG ullNow = GetTickCount64();
		if (m_ullWinStart == 0) m_ullWinStart = ullNow;
		const int iFrame = (std::min)(static_cast<int>((ullNow - m_ullWinStart) / DEATH_FRAME_MS), ConsoleUI::RoomScreenView::WIN_FRAMES);	/* 끝 프레임 = 결과 창 */
		if (iFrame != m_tView.iWinFrame) { m_tView.iWinFrame = iFrame; m_isDirty = true; }
	}

	void CScene_Room::Update_Death()
	{
		if (!m_isFxPlaying) return;

		const int iFrame = static_cast<int>((GetTickCount64() - m_ullDeathStart) / DEATH_FRAME_MS);
		if (iFrame >= (m_tView.eNightFx == ConsoleUI::RoomScreenView::NIGHT_FX::NONE ? ConsoleUI::RoomScreenView::EXEC_FRAMES : ConsoleUI::RoomScreenView::NIGHT_FRAMES))	/* 끝 : 원탁이 다시 보이고 그 의자는 생존 비트대로 회색 + ☠ */
		{
			m_tView.iExecFrame	= -1;
			m_isFxPlaying		= false;
			m_isDirty			= true;
			return;
		}
		if (iFrame != m_tView.iExecFrame) { m_tView.iExecFrame = iFrame; m_isDirty = true; }
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

			/* 서버가 섞은 자리 순서대로 다시 앉힘 (키로 찾아 옮김) */
			{
				Protocol::ROOM_SEAT arrOld[Protocol::MAX_ROOM_PLAYER]{};
				memcpy(arrOld, m_tRoomInfo.arrSeats, sizeof(arrOld));
				for (int i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
				{
					m_tRoomInfo.arrSeats[i] = {};
					for (const Protocol::ROOM_SEAT& tOld : arrOld)
						if (tStart.arrSeatKeys[i] != 0 && tOld.iKey == tStart.arrSeatKeys[i]) m_tRoomInfo.arrSeats[i] = tOld;
				}
			}

			/* 게임 시작 : 내 직업 + 첫 단계 */
			m_eJob = tStart.eJob;
			m_iAccusedSeat = -1;
			m_iAliveMask = 0x7F;		/* 시작은 전원 생존 (다음 CHANGEPHASE부터 서버 값) */
			m_isFindJobUsed = m_isContacted = false;
			m_tView.strPresident.clear();
			m_arrKnownJob.fill(Protocol::JOB::UNKNOWN);
			m_arrKnownMafia.fill(false);
			m_arrKnownClean.fill(false);
			Set_State(STATE::GAME);
			m_tView.vecChats.push_back({ L"", L"게임이 시작되었습니다. 자리가 섞였고 이름은 가려집니다", false, true });
			m_tView.vecChats.push_back({ L"", std::wstring(L"당신의 직업은 ") + Job_Name(m_eJob) + L" 입니다", false, true });
			if (*Job_Desc(m_eJob)) m_tView.vecChats.push_back({ L"", Job_Desc(m_eJob), false, true });
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

			if (tResult.iSeat < 0 || tResult.iSeat >= Protocol::MAX_ROOM_PLAYER || m_tView.arrSeats[tResult.iSeat].strName.empty())
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
		case Protocol::PACKET_TYPE::S_TO_C_LASTVOTE_RESULT:
		{
			if (tEvent.Get_BodyLen() != sizeof(Protocol::S_TO_C_LASTVOTE_RESULT)) break;
			Protocol::S_TO_C_LASTVOTE_RESULT tResult{}; memcpy(&tResult, tEvent.Get_Body(), sizeof(tResult));

			const std::wstring strCount = L"찬성 " + std::to_wstring(m_iAgree) + L" · 반대 " + std::to_wstring(m_iDisagree);	/* 개수는 중계(GAME_LASTVOTE)로 센 값 */
			if (tResult.isPresident && tResult.iSeat == m_iAccusedSeat && tResult.iSeat >= 0 && tResult.iSeat < Protocol::MAX_ROOM_PLAYER)	/* 대통령 : 연행 안 됨 → 확정 */
			{
				const std::wstring& strName = m_tView.arrSeats[tResult.iSeat].strName;
				const std::wstring strHead = (m_ePhase == Protocol::GAME_PHASE::LAST_VOTE) ? strCount + L"  →  " : L"";	/* 투표에서 바로 왔으면 찬반 없음 */
				m_tView.vecChats.push_back({ L"", strHead + strName + L" 님은 대통령입니다! 투표로 연행할 수 없습니다", false, true });
				Know_Job(tResult.iSeat, Protocol::JOB::PRESIDENT);		/* 방 전원 공개 */
				m_isFxPlaying		= true;
				m_ullDeathStart		= GetTickCount64();
				m_tView.strExecName	= strName;
				m_tView.iExecSeat	= tResult.iSeat;
				m_tView.eNightFx	= ConsoleUI::RoomScreenView::NIGHT_FX::NONE;
				m_tView.isPardonFx	= true;
			}
			else if (tResult.iSeat == Protocol::NONE_SEAT || tResult.iSeat != m_iAccusedSeat)	/* 살아남음 */
			{
				m_tView.vecChats.push_back({ L"", strCount + L"  →  연행되지 않았습니다", false, true });
			}
			else																		/* 연행 : 머그샷 연출 시작 (실제로 빠지는 건 CHANGEPHASE 생존 비트로) */
			{
				m_tView.vecChats.push_back({ L"", strCount + L"  →  " + m_tView.arrSeats[tResult.iSeat].strName + L" 님이 연행되었습니다", false, true });
				m_isFxPlaying		= true;
				m_ullDeathStart		= GetTickCount64();
				m_tView.strExecName	= m_tView.arrSeats[tResult.iSeat].strName;
				m_tView.iExecSeat	= tResult.iSeat;
				m_tView.eNightFx	= ConsoleUI::RoomScreenView::NIGHT_FX::NONE;
				m_tView.isPardonFx	= false;
			}
			m_isDirty = true;
			break;
		}
		/* 밤 결과 (밤이 끝날 때, 단계 알림보다 먼저) : 총 → 총알 → 살해 / 경호 막기 / 군인 버팀 연출, 아무도 안 노렸으면 조용한 밤 */
		case Protocol::PACKET_TYPE::S_TO_C_NIGHT_RESULT:
		{
			if (tEvent.Get_BodyLen() != sizeof(Protocol::S_TO_C_NIGHT_RESULT)) break;
			Protocol::S_TO_C_NIGHT_RESULT tResult{}; memcpy(&tResult, tEvent.Get_Body(), sizeof(tResult));
			auto Is_Seat = [&](INT32 iSeat) { return iSeat >= 0 && iSeat < Protocol::MAX_ROOM_PLAYER && !m_tView.arrSeats[iSeat].strName.empty(); };
			using FX = ConsoleUI::RoomScreenView::NIGHT_FX;

			FX eFx = FX::NONE;
			std::wstring strName;
			int iTarget = 0;		/* 노린 자리 (연출 속 머리 · 넥타이 색, 막았으면 모름) */
			if (Is_Seat(tResult.iKilledSeat))
			{
				eFx		= FX::KILL;
				iTarget	= tResult.iKilledSeat;
				strName	= m_tView.arrSeats[tResult.iKilledSeat].strName;
				m_tView.vecChats.push_back({ L"", L"간밤에 " + strName + L" 님이 마피아에게 살해당했습니다", false, true });
			}
			else if (tResult.isBlocked)
			{
				eFx = FX::BLOCK;
				m_tView.vecChats.push_back({ L"", L"💂 경호원이 마피아의 공격을 막아냈습니다", false, true });
			}
			else if (tResult.isSoldierSurvived && Is_Seat(tResult.iSoldierSeat))
			{
				eFx		= FX::SOLDIER;
				iTarget	= tResult.iSoldierSeat;
				strName	= m_tView.arrSeats[tResult.iSoldierSeat].strName;
				m_tView.vecChats.push_back({ L"", L"🔫 " + strName + L" 님은 군인이었습니다! 마피아의 공격을 버텨냈습니다", false, true });
				Know_Job(tResult.iSoldierSeat, Protocol::JOB::SOLDIER);		/* 방 전원 공개 */
			}
			else
				m_tView.vecChats.push_back({ L"", L"조용한 밤이었습니다", false, true });

			if (eFx != FX::NONE)		/* 연출 시작 (실제로 빠지는 건 CHANGEPHASE 생존 비트로) */
			{
				m_isFxPlaying		= true;
				m_ullDeathStart		= GetTickCount64();
				m_tView.eNightFx	= eFx;
				m_tView.isPardonFx	= false;
				m_tView.strExecName	= strName;
				m_tView.iExecSeat	= iTarget;
			}
			m_isDirty = true;
			break;
		}
		/* 게임 끝 : 이긴 팀 + 전원 직업 공개 → 결과 상태 (서버 방은 이미 대기로 돌아감) */
		case Protocol::PACKET_TYPE::S_TO_C_GAME_END:
		{
			if (tEvent.Get_BodyLen() != sizeof(Protocol::S_TO_C_GAME_END)) break;
			Protocol::S_TO_C_GAME_END tEnd{}; memcpy(&tEnd, tEvent.Get_Body(), sizeof(tEnd));

			m_tView.isMafiaWin = (tEnd.eWinner == Protocol::ENDGAME_RESULT::MAFIA);
			m_tView.vecResult.clear();
			for (int i = 0; i < static_cast<int>(m_tView.arrSeats.size()); ++i)
			{
				if (m_tView.arrSeats[i].strName.empty()) continue;
				wchar_t wszNick[Protocol::MAX_NICNAME_BUF]{};
				wcsncpy_s(wszNick, m_tRoomInfo.arrSeats[i].wszNickname, _TRUNCATE);
				m_tView.vecResult.push_back({ std::to_wstring(i + 1) + L"번 " + wszNick, Job_Role(tEnd.arrJobs[i]), Is_RedJob(tEnd.arrJobs[i]), ((tEnd.iAliveMask >> i) & 1) == 0, i == m_iMySeat });
			}
			m_tView.vecChats.push_back({ L"", m_tView.isMafiaWin ? L"마피아가 승리했습니다" : L"시민이 승리했습니다", false, true });

			/* 게임 값 정리 (연출이 원탁을 덮고 있어서 바로 지워도 안 보임) */
			m_eJob			= Protocol::JOB::UNKNOWN;
			m_tView.strPresident.clear();
			m_arrKnownJob.fill(Protocol::JOB::UNKNOWN);
			m_arrKnownMafia.fill(false);
			m_arrKnownClean.fill(false);
			m_iAliveMask	= 0x7F;
			m_iAccusedSeat	= -1;
			m_ullWinStart	= 0;				/* 연행 연출이 끝난 뒤 Update_Result가 시작 */
			Set_State(STATE::RESULT);
			Apply_RoomInfo();
			break;
		}
		case Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA:
		{
			const Protocol::PROVIDE_DATA* pData = reinterpret_cast<const Protocol::PROVIDE_DATA*>(tEvent.Get_Body());
			if (pData == nullptr || tEvent.Get_BodyLen() < sizeof(Protocol::PROVIDE_DATA)) break;	/* 종류(eDataType)를 읽기 전에 길이부터 */

			switch (pData->eDataType)
			{
			case Protocol::DATA_TYPE::SNOOP_RESULT:
			case Protocol::DATA_TYPE::MEDIUM_RESULT:
			case Protocol::DATA_TYPE::BETRAYER_RESULT:
			case Protocol::DATA_TYPE::PRESIDENT_NOTICE:
				On_JobData(tEvent, pData->eDataType);
				break;
			default: break;
			}

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
			/* 누가 한 표 넣을 때마다 : 로그 + 그 의자 아래에 표 수 (단계 바뀌면 Apply_RoomInfo가 지움) */
			else if (pData->eDataType == Protocol::DATA_TYPE::GAME_VOTE)
			{
				if (tEvent.Get_BodyLen() != sizeof(Protocol::GAME_VOTE)) break;
				Protocol::GAME_VOTE tVote{}; memcpy(&tVote, tEvent.Get_Body(), sizeof(tVote));

				if (tVote.iVoteSeat >= 0 && tVote.iVoteSeat < static_cast<INT32>(m_tView.arrSeats.size()))
				{
					m_tView.arrSeats[tVote.iVoteSeat].iVotes = tVote.iCount;
					m_tView.vecChats.push_back({ L"", std::to_wstring(tVote.iVoteSeat + 1) + L"번이 투표되었습니다", false, true });
				}
				else
					m_tView.vecChats.push_back({ L"", L"무효표가 나왔습니다", false, true });
				m_isDirty = true;
			}
			else if (pData->eDataType == Protocol::DATA_TYPE::GAME_LASTVOTE)
			{
				if (tEvent.Get_BodyLen() != sizeof(Protocol::GAME_LASTVOTE)) break;
				Protocol::GAME_LASTVOTE tLast{}; memcpy(&tLast, tEvent.Get_Body(), sizeof(tLast));
				m_tView.vecChats.push_back({ L"", tLast.is ? L"찬성 한 표가 나왔습니다" : L"반대 한 표가 나왔습니다", false, true });
				++(tLast.is ? m_iAgree : m_iDisagree);
				m_tView.strTally = L"찬성 " + std::to_wstring(m_iAgree) + L"   반대 " + std::to_wstring(m_iDisagree);	/* 탁자 가운데 */
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
