#include "pch.h"
#include "CScene_Lobby.h"
#include "CClientApp.h"
#include "ConsoleUI.h"

namespace MafiaClient
{
	CScene_Lobby::CScene_Lobby()
		: CScene(), m_tView{}, m_isDirty{ true } , m_vecPlayerInfos{}, m_vecRoomInfos{}
	{
		m_eType = MafiaClient::SCENE::LOBBY;
	}
	CScene_Lobby::~CScene_Lobby()
	{
	}

	HRESULT CScene_Lobby::Initialize()
	{
		if (FAILED(__super::Initialize())) return E_FAIL;
		m_vecRoomInfos.clear();
		m_vecPlayerInfos.clear();

		m_tView.strNickname = m_pClientApp->Get_Nickname();
		m_tView.strServer	= m_pClientApp->Get_ServerText();
		m_tView.vecMembers	= { m_pClientApp->Get_Nickname() };		/* 서버에서 목록 받기 전까지는 나만 */

		return S_OK;
	}

	void CScene_Lobby::Enter()
	{
		__super::Enter();
		Request_Infos();

		ConsoleUI::Set_CursorVisible(false);
		ConsoleUI::Clear();
		m_isDirty = true;
	}

	void CScene_Lobby::Exit()
	{
		m_vecPlayerInfos.clear();
	}

	void CScene_Lobby::Pre_Update()
	{
		/* 방 만드는 중에는 키를 버림 (응답 기다리는 동안 로비 조작 막기) */
		if (m_isWaitingRoom) { ConsoleUI::Poll_Keys(); return; }

		const int iRoomCount = static_cast<int>(m_tView.vecRooms.size());

		for (const KEY_EVENT_RECORD& tKey : ConsoleUI::Poll_Keys())
		{
			switch (tKey.wVirtualKeyCode)
			{
			case VK_UP:
				if (iRoomCount > 0) m_tView.iSelected = (m_tView.iSelected + iRoomCount - 1) % iRoomCount;
				m_isDirty = true;
				break;
			case VK_DOWN:
				if (iRoomCount > 0) m_tView.iSelected = (m_tView.iSelected + 1) % iRoomCount;
				m_isDirty = true;
				break;
			case VK_RETURN:
				/* 서버에 입장 요청 → S_TO_C_ENTER_ROOM 오면 ROOM (바로 넘어가지 않음) */
				if (iRoomCount > 0) Enter_Room();
				break;
			case 'C':
				/* TODO : 방만들기 */
				Create_Room();
				break;
			case 'R':
				Request_Infos();		/* 새로고침 */
				break;
			case VK_ESCAPE:
				m_pClientApp->Disconnect();
				m_pClientApp->Change_Scene(MafiaClient::SCENE::CONNECT);
				break;
			default: break;
			}
		}
	}

	void CScene_Lobby::Update()
	{
		/* 내 번호는 접속 직후 서버 NetLib이 보내줌 → 로비에 들어온 뒤에 도착할 수 있으니 바뀌면 다시 그림 */
		const Net::CClient* pClient = m_pClientApp->Get_Client();
		const uint64_t iKey = pClient ? pClient->Get_ClientKey() : 0;
		if (iKey != m_tView.iMyKey)
		{
			m_tView.iMyKey = iKey;
			m_isDirty = true;
		}

		/* 접속자 목록 주기적 요청 (폴링) : 마지막 요청(Enter / R / 자동) 후 일정 시간이 지나면 다시 요청 */
		if (GetTickCount64() - m_ullLastRequest >= iLobbyListUpdateSecond * 1000)
			Request_Infos();

		/* 방 만들기 응답 기다리는 중 : 응답(On_Packet) + 최소 연출 시간이 지나면 방으로, 너무 오래 걸리면 로비로 */
		if (m_isWaitingRoom)
		{
			const ULONGLONG ullElapsed = GetTickCount64() - m_ullWaitStart;
			if (m_isEntered && ullElapsed >= ROOM_WAIT_MIN_MS)
			{
				FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));	/* 기다리는 동안 누른 키가 방으로 새지 않게 */
				m_pClientApp->Change_Scene(MafiaClient::SCENE::ROOM, m_tEnterInfo);	/* 받은 방 정보를 방 Scene에 넘김 */
			}
			else if (!m_isEntered && ullElapsed >= ROOM_WAIT_TIMEOUT_MS)
			{
				m_isWaitingRoom = false;
				ConsoleUI::Clear();
				m_isDirty = true;
			}
		}
	}

	void CScene_Lobby::Late_Update()
	{
	}

	void CScene_Lobby::Render()
	{
		/* 기다리는 중 : 화면(카드 / 로비)은 그대로 두고 그 아래에 진행 표시만 매 프레임 */
		if (m_isWaitingRoom)
		{
			const ULONGLONG ullElapsed = GetTickCount64() - m_ullWaitStart;
			const float fProgress = (std::min)(1.f, static_cast<float>(ullElapsed) / ROOM_WAIT_MIN_MS);
			ConsoleUI::Draw_Progress(m_strWaitText, static_cast<int>(ullElapsed / 80), fProgress);
			return;
		}

		if (m_isDirty == false && ConsoleUI::Need_Redraw() == false) return;
		m_isDirty = false;

		ConsoleUI::Render_Lobby(m_tView);
	}

	void CScene_Lobby::Create_Room()
	{
		ConsoleUI::CreateRoomResult  tResult = ConsoleUI::Run_CreateRoom(L"서버 " + m_pClientApp->Get_ServerText(), L"", L"", Protocol::MAX_ROOMNAME_LEN);

		/* Esc : 카드가 화면을 지웠으니 로비를 다시 그림 */
		if (tResult.isOk == false)
		{
			ConsoleUI::Clear();
			m_isDirty = true;
			return;
		}

		/* Create Room 패킷 보내기 */
		Protocol::C_TO_S_CREATE_ROOM tBody{};
		wcsncpy_s(tBody.wszRoomName, tResult.strTitle.c_str(), _TRUNCATE);

		m_pClientApp->Get_Client()->Send_Packet(
			static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_CREATE_ROOM), &tBody,sizeof(Protocol::C_TO_S_CREATE_ROOM));

		/* 카드는 그대로 두고 응답 기다리기 */
		Start_WaitRoom(L"방 만드는 중");
	}

	void CScene_Lobby::Enter_Room()
	{
		const int iIndex = (std::min)((std::max)(m_tView.iSelected, 0), static_cast<int>(m_tView.vecRooms.size()) - 1);

		Protocol::C_TO_S_ENTER_ROOM tBody{};
		tBody.iRoomKey = static_cast<Protocol::ROOM_KEY>(m_tView.vecRooms[iIndex].iNo);
		m_pClientApp->Get_Client()->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_ENTER_ROOM), tBody);

		/* 로비는 그대로 두고 아래 안내 줄 밑에 "입장하는 중" */
		Start_WaitRoom(L"입장하는 중");
	}

	void CScene_Lobby::Start_WaitRoom(const std::wstring& strText)
	{
		ConsoleUI::Set_CursorVisible(false);		/* 카드가 끝나면서 켠 커서가 진행 바 끝에서 깜빡이지 않게 */
		m_strWaitText	= strText;
		m_isWaitingRoom	= true;
		m_isEntered		= false;
		m_ullWaitStart	= GetTickCount64();
	}

	void CScene_Lobby::On_Packet(const Net::Event& tEvent)
	{
		Protocol::PACKET_TYPE ePacketType = static_cast<Protocol::PACKET_TYPE>(tEvent.Get_PacketType());
		
		switch (ePacketType)
		{
		/* 방 입장 성공 (내가 만든 방 포함) : 받아 두고 넘어가는 건 Update에서 (최소 연출 시간) */
		case Protocol::PACKET_TYPE::S_TO_C_ENTER_ROOM:
		{
			if (tEvent.Get_BodyLen() != sizeof(Protocol::S_TO_C_ENTER_ROOM)) break;
			memcpy(&m_tEnterInfo, tEvent.Get_Body(), sizeof(m_tEnterInfo));
			m_isEntered = true;
			break;
		}
		case Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA:
		{
			const Protocol::PROVIDE_DATA* pData = reinterpret_cast<const Protocol::PROVIDE_DATA*>(tEvent.Get_Body());
			if (pData == nullptr) break;

			/* 로비 접속자 목록이라면 */
			if (pData->eDataType == Protocol::DATA_TYPE::LOBBY_PLAYERS)
			{
				if (tEvent.Get_BodyLen() != sizeof(Protocol::LOBBY_PLAYERS)) break;		/* 크기 안 맞으면 버림 */
				const Protocol::LOBBY_PLAYERS* pInfo = reinterpret_cast<const Protocol::LOBBY_PLAYERS*>(tEvent.Get_Body());
				if (pInfo == nullptr) break;

				const uint16_t iCount = (std::min)(pInfo->iCount, static_cast<uint16_t>(Protocol::MAX_ONLINE_PLAYER));	/* 서버 값도 범위 검사 */
				size_t iCurSize = m_vecPlayerInfos.size();
				m_vecPlayerInfos.resize(iCurSize + iCount);
				memcpy(m_vecPlayerInfos.data() + iCurSize, pInfo->arr, iCount * sizeof(Protocol::LOBBY_PLAYER));

				Apply_Infos();
			}

			/* 로비 접속자 목록이라면 */
			if (pData->eDataType == Protocol::DATA_TYPE::LOBBY_ROOMS)
			{
				if (tEvent.Get_BodyLen() != sizeof(Protocol::LOBBY_ROOMS)) break;		/* 크기 안 맞으면 버림 */
				const Protocol::LOBBY_ROOMS* pInfo = reinterpret_cast<const Protocol::LOBBY_ROOMS*>(tEvent.Get_Body());
				if (pInfo == nullptr) break;

				const uint16_t iCount = (std::min)(pInfo->iCount, static_cast<uint16_t>(Protocol::MAX_ROOM_PACKET_COUNT));	/* 서버 값도 범위 검사 */
				size_t iCurSize = m_vecRoomInfos.size();
				m_vecRoomInfos.resize(iCurSize + iCount);
				memcpy(m_vecRoomInfos.data() + iCurSize, pInfo->arr , iCount * sizeof(Protocol::LOBBY_ROOM));

				Apply_Infos();
			}

			break;
		}
		default:	break;
		}
	}

	void CScene_Lobby::Request_Infos()
	{
		if (m_pClientApp->Get_Client() == nullptr) return;

		m_ullLastRequest = GetTickCount64();	/* 여기서 기록 → 자동 · 수동(R) 어느 쪽이든 그 시점부터 다시 셈 */
		m_vecPlayerInfos.clear();				/* 새 목록을 받을 거라 비움 (안 비우면 새로고침마다 같은 사람이 쌓임) */
		m_vecRoomInfos.clear();

		Protocol::REQ_DATA tPacket{};
		tPacket.eDataType = Protocol::DATA_TYPE::LOBBY_PLAYERS;
		m_pClientApp->Get_Client()->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_REQ_DATA), tPacket);
		tPacket.eDataType = Protocol::DATA_TYPE::LOBBY_ROOMS;
		m_pClientApp->Get_Client()->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_REQ_DATA), tPacket);
	}

	void CScene_Lobby::Apply_Infos()
	{
		static const wchar_t* szState[] = { L"로비", L"방", L"게임" };

		m_tView.vecPlayers.clear();
		m_tView.vecRooms.clear();
		for (Protocol::LOBBY_PLAYER& tInfo : m_vecPlayerInfos)
		{
			tInfo.wszNickname[Protocol::MAX_NICNAME_BUF - 1] = L'\0';	/* 서버 값도 끝은 막아 둠 */
			const std::wstring strState = (tInfo.eState < 3) ? szState[tInfo.eState] : L"?";
			m_tView.vecPlayers.push_back({ tInfo.iKey, tInfo.wszNickname, strState });
		}
		for (Protocol::LOBBY_ROOM& tRoomInfo : m_vecRoomInfos)
		{
			tRoomInfo.wszRoomName[Protocol::MAX_ROOMNAME_BUF - 1] = L'\0';
			ConsoleUI::RoomView tView{};
			tView.iCur		= tRoomInfo.iPlayerNumber;
			tView.iMax		= Protocol::MAX_ROOM_PLAYER;
			tView.iNo		= static_cast<int>(tRoomInfo.iRoomKey);
			tView.strName	= tRoomInfo.wszRoomName;
			tView.isPlaying = tRoomInfo.isPlaying;

			m_tView.vecRooms.push_back(std::move(tView));
		}
		m_isDirty = true;
	}

	std::unique_ptr<CScene_Lobby> CScene_Lobby::Create()
	{
		std::unique_ptr<CScene_Lobby> pScene{ new CScene_Lobby() };
		if (FAILED(pScene->Initialize()))
			return nullptr;
		return pScene;
	}

	void CScene_Lobby::Register_NetEventCallBack()
	{
		m_pClientApp->Register_DefaultNetEvents();
		m_pClientApp->Register_NetEvents(Net::NET_EVENT_TYPE::Receive_Packet, [this](const Net::Event& tEvent) {On_Packet(tEvent); });
	}
}
