#include "pch.h"
#include "CServerApp.h"

#include "CServer.h"
#include "ConsoleUI.h"
#include "ConsoleUI_Server.h"
#include "CPlayerManager.h"
#include "CMafiaLogic.h"

namespace MafiaServer
{
	/* C_TO_S_JOB_ACTION : 직업별 구조체 크기 (모르는 직업 = 0) */
	static size_t JobAction_Size(Protocol::JOB eJob)
	{
		switch (eJob)
		{
		case Protocol::JOB::MAFIA:		return sizeof(Protocol::MAFIA_ACTION);
		case Protocol::JOB::SNOOP:		return sizeof(Protocol::SNOOP_ACTION);
		case Protocol::JOB::BODYGUARD:	return sizeof(Protocol::BODYGUARD_ACTION);
		case Protocol::JOB::MEDIUM:		return sizeof(Protocol::MEDIUM_ACTION);
		case Protocol::JOB::BETRAYER:	return sizeof(Protocol::BETRAYER_ACTION);
		case Protocol::JOB::PRESIDENT:	return sizeof(Protocol::PRESIDENT_ACTION);
		default:						return 0;
		}
	}

	CServerApp::CServerApp()
		: m_pMainServer{nullptr} , m_isDirty{false} , m_pPlayerManager{ nullptr }, m_pRoomManager{ nullptr }
	{
	}
	CServerApp::~CServerApp()
	{
	}

	CPlayerManager::CPlayerManager()
	{
	}

	CPlayerManager::~CPlayerManager()
	{
	}

	void CServerApp::Destory()
	{
		m_pMainServer.reset();

		Net::Net_Release();
	}

	HRESULT CServerApp::Initialize(const Init_Data& tData)
	{
		Net::Net_Init();

		/* Server 만들고 초기화 */
		m_pMainServer = Net::CServer::Create(tData.tServer_InitData);
		if (m_pMainServer == nullptr) return E_FAIL;
		Register_NetEvents();


		/* Player Manager 만들기 */
		m_pPlayerManager = CPlayerManager::Get_Instance();
		if (FAILED(m_pPlayerManager->Initialize(tData.tPlayerManager_InitData))) return E_FAIL;


		/* Room Manager 만들기 */
		m_pRoomManager = CRoomManager::Get_Instance();
		if (FAILED(m_pRoomManager->Initialize(tData.tRoomManager_InitData))) return E_FAIL;

		return S_OK;
	}


	void CServerApp::Pre_Update()
	{
		/* 현재 시각 먼저 : 아래 패킷 처리(콜백)도 이 시각으로 판단 */
		m_ullNow = GetTickCount64();
		m_pRoomManager->Pre_Update(m_ullNow);		/* 방 → (게임 중이면) 게임 로직에 시각 전달 */

		m_pMainServer->Pre_Update();
	}

	void CServerApp::Update()
	{
		Key_Input();

		/* 방 → (게임 중이면) 게임 로직 : 단계 시간 검사 */
		m_pRoomManager->Update(m_ullNow);

		m_pMainServer->Update();
	}

	void CServerApp::Late_Update()
	{
		m_pMainServer->Late_Update();
	}

	void CServerApp::Render()
	{
		if (m_isDirty == false && ConsoleUI::Need_Redraw(*m_pMainServer) == false) return;

		ConsoleUI::Render_ServerMain(*m_pMainServer);

		m_isDirty = false;
	}


	void CServerApp::Key_Input()
	{
		/* 받아온 Key Input 처내기 */
		for (const KEY_EVENT_RECORD& tKey : ConsoleUI::Poll_Keys())
		{
			/* 클라이언트 접속 받기 On Off */
			if (tKey.wVirtualKeyCode == VK_F5) Allow_Accept();
		}
	}

	void CServerApp::Allow_Accept()
	{
		m_pMainServer->Set_Accept(!m_pMainServer->Is_Accept());
		m_isDirty = true;
	}

	void CServerApp::Register_NetEvents()
	{
		/* std::function은 멤버 함수를 바로 못 담음 (어느 객체의 함수인지 모름) → this를 잡은 람다로 연결 */
		m_pMainServer->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Connect,			[this](const Net::Event& tEvent) { On_Connect(tEvent); });
		m_pMainServer->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Reject_Connect,	[this](const Net::Event& tEvent) { On_RejectConnect(tEvent); });
		m_pMainServer->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Receive_Packet,	[this](const Net::Event& tEvent) { On_ReceivePacket(tEvent); });
		m_pMainServer->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Send_Packet,		[this](const Net::Event& tEvent) { On_SendPacket(tEvent); });
		m_pMainServer->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Disconnect,		[this](const Net::Event& tEvent) { On_Disconnect(tEvent); });
	}

	void CServerApp::On_Connect(const Net::Event& tEvent)
	{
		/* tEvent.iKey : 새로 들어온 클라이언트 (CPlayer는 로그인 패킷을 받은 뒤에 생성) */
		m_isDirty = true;
	}

	void CServerApp::On_RejectConnect(const Net::Event& tEvent)
	{
		m_isDirty = true;
	}

	void CServerApp::On_ReceivePacket(const Net::Event& tEvent)
	{
		/* TODO: 헤더 꺼내서 Protocol::PACKET_TYPE 별로 switch (C_TO_S_LOGIN 등) */
		Protocol::PACKET_TYPE tType = static_cast<Protocol::PACKET_TYPE>(tEvent.Get_PacketType());

		switch (tType)
		{
		case Protocol::PACKET_TYPE::C_TO_S_LOGIN:
#pragma region C_TO_S_LOGIN
		/* Client -> Server에게 본인 아이디를 줌 */
		{
			if (tEvent.Is_NetPacket()) return;

			uint16_t iLen = tEvent.Get_BodyLen();
			if (iLen != sizeof(Protocol::C_TO_S_LOGIN)) break;
			Net::Packet<Protocol::C_TO_S_LOGIN> tPacket{};
			memcpy(&tPacket.tData, tEvent.Get_Body(),iLen);

			/* 닉네임 체크하기 */
			Protocol::CHECK_NAME eCheck =  Protocol::Is_VailedNickName(tPacket.tData.wszNicName);

			switch (eCheck)
			{
			case Protocol::CHECK_NAME::OK:
			{
				/* OK 라면 Client 번호 연결 시켜주기 */
				Protocol::S_TO_C_LOGIN tLoginBody{};
				tLoginBody.iClientKey = tEvent.iKey;
				m_pMainServer->Send_Packet(tEvent.iKey, static_cast<uint16_t>(Protocol::PACKET_TYPE::S_TO_C_LOGIN), tLoginBody);

				if (m_pPlayerManager->LogIn_Player(tEvent.iKey, tPacket.tData.wszNicName) == false) break;

				/* 로그인 했을시 클라이언트 로그인 목록 업데이트 */


				m_pMainServer->Add_CustomLog(tEvent.iKey, std::wstring(L"로그인 (") + tPacket.tData.wszNicName + L")");
				m_isDirty = true;

				break;
			}
			case Protocol::CHECK_NAME::EMPTY:
			case Protocol::CHECK_NAME::OVER:
			case Protocol::CHECK_NAME::INVALID_CHARACTER:
			{
				/* 로그인 실패 이유 보내주기 */
				Protocol::S_TO_C_LOGIN_FAILED tLoginFailedBody{};
				tLoginFailedBody.eFailedType = eCheck;
				m_pMainServer->Send_Packet(tEvent.iKey, static_cast<uint16_t>(Protocol::PACKET_TYPE::S_TO_C_LOGIN_FAILED), tLoginFailedBody);
				break;
			}
			default:break;
			}
			break; 
#pragma endregion
		}
		case Protocol::PACKET_TYPE::C_TO_S_CHAT:
#pragma region C_TO_S_CHAT
		{
			if (tEvent.Is_NetPacket()) return;
			uint16_t iLen = tEvent.Get_BodyLen();
			if (iLen != sizeof(Protocol::C_TO_S_CHAT)) break;
			Net::Packet<Protocol::C_TO_S_CHAT> tPacket{};
			memcpy(&tPacket.tData, tEvent.Get_Body(), iLen);

			SYSTEMTIME tTime{};
			GetLocalTime(&tTime);
			m_pRoomManager->Chatting(tPacket.tData.iRoomKey, tTime, tEvent.iKey, tPacket.tData.wszText, m_pMainServer.get());
			break; 
#pragma endregion
		}
		case Protocol::PACKET_TYPE::C_TO_S_REQ_DATA:
		{
			Handle_RequestData(tEvent);
			break;
		}
		/* 방 생성 처리 */
		case Protocol::PACKET_TYPE::C_TO_S_CREATE_ROOM:
		{
			Handle_CreateRoom(tEvent);
			break;
		}
		/* 방 들어가기 */
		case Protocol::PACKET_TYPE::C_TO_S_ENTER_ROOM:
		{
			Handle_EnterRoom(tEvent);
			break;
		}
		/* 방 나가기 (본문 없음) */
		case Protocol::PACKET_TYPE::C_TO_S_EXIT_ROOM:
		{
#pragma region C_TO_S_EXIT_ROOM

			/* 카운트다운 · 게임 중엔 못 나감 (끊김은 Leave_Room을 바로 부르니 여기 안 걸림) */
			if (const CPlayer* pPlayer = m_pPlayerManager->Find_Player(tEvent.iKey))
				if (const CRoom* pRoom = m_pRoomManager->Find_Room(pPlayer->Get_RoomKey()); pRoom && pRoom->Get_IsPlaying())
					break;
			Leave_Room(tEvent.iKey);
			break;
#pragma endregion
		}
		case Protocol::PACKET_TYPE::C_TO_S_STARTGAME:
		{
#pragma region C_TO_S_STARTGAME
			uint16_t iLen = tEvent.Get_BodyLen();
			const Protocol::C_TO_S_STARTGAME* pBody = reinterpret_cast<const Protocol::C_TO_S_STARTGAME*>(tEvent.Get_Body());

			/* 확인 ① 결과 (실제 시작 S_TO_C_STARTGAME은 5초 뒤 방 Update에서 따로) */
			Protocol::S_TO_C_STARTGAME_RESULT tGameStartResult{};
			if (iLen != sizeof(Protocol::C_TO_S_STARTGAME) || pBody == nullptr)
			{
				tGameStartResult.eResult = Protocol::STARTGAME_RESULT::SERVER_ERROR;
			}
			else
			{
				/* 게임 시작할 수 있는지 체크 */
				Net::ClientKey iRoomMasterKey = tEvent.iKey;
				Protocol::ROOM_KEY iRoomKey = pBody->iRoomKey;

				/* 유효성 체크 */
				const CRoom* pRoom = m_pRoomManager->Find_Room(iRoomKey);
				if (pRoom == nullptr) tGameStartResult.eResult = Protocol::STARTGAME_RESULT::SERVER_ERROR;
				else if (pRoom->Get_MasterKey() != iRoomMasterKey) tGameStartResult.eResult = Protocol::STARTGAME_RESULT::SERVER_ERROR;
				else 	pRoom->Can_Start() ? tGameStartResult.eResult = Protocol::STARTGAME_RESULT::OK : tGameStartResult.eResult = Protocol::STARTGAME_RESULT::NOT_FULL;
			}

			/* 승인 : 방을 로딩(카운트다운) 상태로 → START_COUNTDOWN_MS 뒤 방 Update에서 게임 시작 */
			if (tGameStartResult.eResult == Protocol::STARTGAME_RESULT::OK)
				m_pRoomManager->Start_Loading(pBody->iRoomKey, m_ullNow);

			if (tGameStartResult.eResult == Protocol::STARTGAME_RESULT::OK)
				m_pRoomManager->Find_Room(pBody->iRoomKey)->SendPacket_All( Protocol::PACKET_TYPE::S_TO_C_STARTGAME_RESULT ,&tGameStartResult,sizeof( Protocol::S_TO_C_STARTGAME_RESULT ) , m_pMainServer.get() );
			else m_pMainServer->Send_Packet(tEvent.iKey,static_cast<uint16_t>(Protocol::PACKET_TYPE::S_TO_C_STARTGAME_RESULT), tGameStartResult);
	
			break;
		}
#pragma endregion
		case Protocol::PACKET_TYPE::C_TO_S_VOTE:
#pragma region 투표 처리
		{
			if (tEvent.Get_BodyLen() != sizeof(Protocol::C_TO_S_VOTE)) break;
			const Protocol::C_TO_S_VOTE* pBody = reinterpret_cast<const Protocol::C_TO_S_VOTE*>(tEvent.Get_Body());
			if ( pBody  == nullptr) break;

			/* 투표 결과전달하기 */
			m_pRoomManager->Get_Instance()->Vote(pBody->iRoomKey,tEvent.iKey,pBody->iVoteSeat);
			break;
		}
#pragma endregion
		case Protocol::PACKET_TYPE::C_TO_S_LAST_VOTE:
		{
#pragma region 최후의 투표 처리
			if (tEvent.Get_BodyLen() != sizeof(Protocol::C_TO_S_LAST_VOTE)) break;
			const Protocol::C_TO_S_LAST_VOTE* pBody = reinterpret_cast<const Protocol::C_TO_S_LAST_VOTE*>(tEvent.Get_Body());
			if (pBody == nullptr) break;
			
			m_pRoomManager->LastVote(pBody->iRoomKey, tEvent.iKey, pBody->isAgree);
			break;
#pragma endregion
		}
		case Protocol::PACKET_TYPE::C_TO_S_JOB_ACTION:
		{
			/* 머리부터 : 직업별 구조체 크기는 eJob을 보고 한 번 더 확인 */
			if (tEvent.Get_BodyLen() < sizeof(Protocol::JOB_ACTION)) break;
			const Protocol::JOB_ACTION* pBody{ reinterpret_cast<const Protocol::JOB_ACTION*>(tEvent.Get_Body())};
			if (pBody == nullptr) break;

			if (tEvent.Get_BodyLen() != JobAction_Size(pBody->eJob)) break;	/* 모르는 직업이면 0 → 버림 */

			m_pRoomManager->Use_Skill(tEvent.iKey, *pBody);
			break;
		}
		case Protocol::PACKET_TYPE::END:			break;
		default:									break;
		}
	}

	void CServerApp::On_SendPacket(const Net::Event& tEvent)
	{
	}

	void CServerApp::On_Disconnect(const Net::Event& tEvent)
	{
		/* 로그인했던 사람만 로그아웃 (지우기 전에 닉네임 꺼내 두기) */
		const CPlayer* pPlayer = m_pPlayerManager->Find_Player(tEvent.iKey);
		if (pPlayer == nullptr) return;
		const std::wstring strNickname = pPlayer->Get_Nickname();

		Leave_Room(tEvent.iKey);		/* 방에 있었으면 먼저 빼기 (안 빼면 빈 의자가 남음) */
		if (m_pPlayerManager->LogOut_Player(tEvent.iKey) == false) return;

		m_pMainServer->Add_CustomLog(tEvent.iKey, L"로그아웃 (" + strNickname + L")");
		m_isDirty = true;
	}

	void CServerApp::Handle_CreateRoom(const Net::Event& tEvent)
	{
		if (tEvent.Get_BodyLen() != sizeof(Protocol::C_TO_S_CREATE_ROOM)) return;
		Protocol::C_TO_S_CREATE_ROOM tBody{};
		memcpy(&tBody, tEvent.Get_Body(), sizeof(tBody));
		tBody.wszRoomName[Protocol::MAX_ROOMNAME_BUF - 1] = L'\0';		/* 받은 값은 끝을 막아 둠 */

		/* 로그인했고 로비에 있는 사람만 (이미 방에 있는 사람이 또 만들기 막기) */
		const CPlayer* pPlayer = m_pPlayerManager->Find_Player(tEvent.iKey);
		if (pPlayer == nullptr || pPlayer->Get_State() != Protocol::PLAYER_STATE::LOBBY) return;

		/* TODO: 실패 이유 패킷 (지금은 실패하면 응답 없음 → 클라는 시간 초과로 로비 복귀) */
		const Protocol::ROOM_KEY iRoomKey = m_pRoomManager->Create_Room(tBody.wszRoomName, tEvent.iKey);
		if (iRoomKey == 0) return;

		m_pPlayerManager->Set_PlayerRoom(tEvent.iKey, iRoomKey);
		Send_EnterRoom(tEvent.iKey, iRoomKey);
		m_pMainServer->Add_CustomLog(tEvent.iKey, L"방 생성 (#" + std::to_wstring(iRoomKey) + L" " + tBody.wszRoomName + L")");
		m_isDirty = true;
	}

	void CServerApp::Handle_EnterRoom(const Net::Event& tEvent)
	{
		if (tEvent.Get_BodyLen() != sizeof(Protocol::C_TO_S_ENTER_ROOM)) return;
		Protocol::C_TO_S_ENTER_ROOM tBody{};
		memcpy(&tBody, tEvent.Get_Body(), sizeof(tBody));

		/* 로그인했고 로비에 있는 사람만 */
		const CPlayer* pPlayer = m_pPlayerManager->Find_Player(tEvent.iKey);
		if (pPlayer == nullptr || pPlayer->Get_State() != Protocol::PLAYER_STATE::LOBBY) return;

		/* TODO: 실패 이유 패킷 (꽉 참 / 게임 중 / 없는 방). 지금은 응답 없음 → 클라는 시간 초과로 로비 */
		if (m_pRoomManager->Enter_Room(tBody.iRoomKey, tEvent.iKey) != CRoom::ENTER_RESULT::OK) return;

		m_pPlayerManager->Set_PlayerRoom(tEvent.iKey, tBody.iRoomKey);
		Send_EnterRoom(tEvent.iKey, tBody.iRoomKey);					/* 들어온 사람 : 방 전체 정보 */
		Broadcast_RoomMembers(tBody.iRoomKey, tEvent.iKey);			/* 원래 있던 사람들 : 바뀐 자리 */
		m_pMainServer->Add_CustomLog(tEvent.iKey, L"방 입장 (#" + std::to_wstring(tBody.iRoomKey) + L")");
		m_isDirty = true;
	}

	void CServerApp::Fill_Seats(const CRoom* pRoom, Protocol::ROOM_SEAT (&arrSeats)[Protocol::MAX_ROOM_PLAYER])
	{
		/* 자리 순서 그대로 (빈자리는 iKey 0) */
		const auto& arrKeys = pRoom->Get_PlayerKeys();
		for (size_t i = 0; i < arrKeys.size(); ++i)
		{
			arrSeats[i] = {};
			const CPlayer* pSeat = m_pPlayerManager->Find_Player(arrKeys[i]);
			if (pSeat == nullptr) continue;
			arrSeats[i].iKey = arrKeys[i];
			wcsncpy_s(arrSeats[i].wszNickname, pSeat->Get_Nickname().c_str(), _TRUNCATE);
		}
	}

	void CServerApp::Send_EnterRoom(Net::ClientKey iClientKey, Protocol::ROOM_KEY iRoomKey)
	{
		const CRoom* pRoom = m_pRoomManager->Find_Room(iRoomKey);
		if (pRoom == nullptr) return;

		Protocol::S_TO_C_ENTER_ROOM tBody{};
		tBody.iRoomKey	 = iRoomKey;
		tBody.iMasterKey = pRoom->Get_MasterKey();
		wcsncpy_s(tBody.wszRoomName, pRoom->Get_Name(), _TRUNCATE);
		Fill_Seats(pRoom, tBody.arrSeats);
		m_pMainServer->Send_Packet(iClientKey, static_cast<uint16_t>(Protocol::PACKET_TYPE::S_TO_C_ENTER_ROOM), tBody);
	}

	void CServerApp::Broadcast_RoomMembers(Protocol::ROOM_KEY iRoomKey, Net::ClientKey iExceptKey)
	{
		const CRoom* pRoom = m_pRoomManager->Find_Room(iRoomKey);
		if (pRoom == nullptr) return;		/* 마지막 사람이 나가 방이 지워졌으면 알릴 사람도 없음 */

		Protocol::S_TO_C_ROOM_MEMBERS tBody{};
		tBody.iMasterKey = pRoom->Get_MasterKey();
		Fill_Seats(pRoom, tBody.arrSeats);

		for (Net::ClientKey iKey : pRoom->Get_PlayerKeys())
		{
			if (iKey == Net::NONE_CLIENTKEY || iKey == iExceptKey) continue;
			m_pMainServer->Send_Packet(iKey, static_cast<uint16_t>(Protocol::PACKET_TYPE::S_TO_C_ROOM_MEMBERS), tBody);
		}
	}

	void CServerApp::Leave_Room(Net::ClientKey iClientKey)
	{
		const Protocol::ROOM_KEY iRoomKey = m_pRoomManager->Exit_Room(iClientKey);
		if (iRoomKey == 0) return;		/* 방에 없었음 */

		m_pPlayerManager->Set_PlayerRoom(iClientKey, 0);

		Broadcast_RoomMembers(iRoomKey, Net::NONE_CLIENTKEY);		/* 남은 사람들에게 (나간 사람은 이미 빠짐) */
		m_pMainServer->Add_CustomLog(iClientKey, L"방 나감 (#" + std::to_wstring(iRoomKey) + L")");
		m_isDirty = true;
	}

	void CServerApp::Handle_RequestData(const Net::Event& tEvent)
	{
		uint16_t iLen = tEvent.Get_BodyLen();
		if (iLen != sizeof(Protocol::REQ_DATA)) return;
		Net::Packet<Protocol::REQ_DATA> tPacket{};
		memcpy(&tPacket.tData, tEvent.Get_Body(), sizeof(Protocol::REQ_DATA));

		/* 로비 접속자 목록 요청 : 서버 CPlayer에서 로비에 필요한 것(키, 닉네임, 상태)만 골라 담음 */
		if (tPacket.tData.eDataType == Protocol::DATA_TYPE::LOBBY_PLAYERS)
		{
			Protocol::LOBBY_PLAYERS tBody{};
			tBody.eDataType = Protocol::DATA_TYPE::LOBBY_PLAYERS;

			for (const auto& Pair : m_pPlayerManager->Get_Players())
			{
				const CPlayer* pPlayer = Pair.second.get();
				Protocol::LOBBY_PLAYER& tRow = tBody.arr[tBody.iCount++];
				tRow.iKey	= pPlayer->Get_ClientKey();
				tRow.eState	= static_cast<uint8_t>(pPlayer->Get_State());
				wcsncpy_s(tRow.wszNickname, pPlayer->Get_Nickname().c_str(), _TRUNCATE);

				/* 한 패킷이 꽉 차면 보내고 다시 채움 */
				if (tBody.iCount == Protocol::MAX_ONLINE_PLAYER)
				{
					m_pMainServer->Send_Packet(tEvent.iKey, static_cast<uint16_t>(Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA), tBody);
					tBody.iCount = 0;
				}
			}
			if (tBody.iCount > 0)
				m_pMainServer->Send_Packet(tEvent.iKey, static_cast<uint16_t>(Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA), tBody);
		}

		if (tPacket.tData.eDataType == Protocol::DATA_TYPE::LOBBY_ROOMS)
		{
			Protocol::LOBBY_ROOMS tBody{};
			tBody.eDataType = Protocol::DATA_TYPE::LOBBY_ROOMS;

			const auto& Rooms = m_pRoomManager->Get_Rooms();
			for (const auto& Pair : Rooms)
			{
				const CRoom* pRoom = Pair.second.get();
				if (pRoom == nullptr) continue;
				auto& CurBody = tBody.arr[tBody.iCount++];
				CurBody.isPlaying = pRoom->Get_IsPlaying();
				CurBody.iPlayerNumber = pRoom->Get_CurPlayerNum();
				CurBody.iRoomKey = pRoom->Get_Key();
				wcsncpy_s(CurBody.wszRoomName, pRoom->Get_Name(), Protocol::MAX_ROOMNAME_BUF);

				/* 한 패킷이 꽉 차면 보내고 다시 채움 */
				if (tBody.iCount == Protocol::MAX_ROOM_PACKET_COUNT)
				{
					m_pMainServer->
						Send_Packet(tEvent.iKey, static_cast<uint16_t>(Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA), tBody);
					tBody.iCount = 0;
				}
			}
			if(tBody.iCount > 0)
				m_pMainServer->Send_Packet(tEvent.iKey, static_cast<uint16_t>(Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA), tBody);
		}
	}
}