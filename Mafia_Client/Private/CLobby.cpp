#include "pch.h"
#include "CLobby.h"
#include "CClientApp.h"


namespace MafiaClient
{
	CLobby::CLobby()
		: CScene(), m_tView{}, m_isDirty{ true } , m_vecPlayerInfos{}
	{
		m_eType = MafiaClient::SCENE::LOBBY;
	}
	CLobby::~CLobby()
	{
	}

	HRESULT CLobby::Initialize()
	{
		if (FAILED(__super::Initialize())) return E_FAIL;

		m_tView.strNickname = m_pClientApp->Get_Nickname();
		m_tView.strServer	= m_pClientApp->Get_ServerText();
		m_tView.vecMembers	= { m_pClientApp->Get_Nickname() };		/* 서버에서 목록 받기 전까지는 나만 */

		/* TODO: 서버에서 방 목록 받으면 교체 (지금은 화면 확인용 더미) */
		m_tView.vecRooms = {
			{ 1, L"초보만 오세요", 3, 7, L"대기 중", false },
			{ 2, L"빠른 한 판",    7, 7, L"게임 중 · 낮 2일차", true },
		};


		return S_OK;
	}

	void CLobby::Enter()
	{
		__super::Enter();
		Request_PlayerInfos();

		ConsoleUI::Set_CursorVisible(false);
		ConsoleUI::Clear();
		m_isDirty = true;
	}

	void CLobby::Exit()
	{
		m_vecPlayerInfos.clear();
	}

	void CLobby::Pre_Update()
	{
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
				/* TODO: 서버에 입장 요청 → 허락 오면 ROOM. 지금은 바로 이동 */
				if (iRoomCount > 0) m_pClientApp->Change_Scene(MafiaClient::SCENE::ROOM);
				break;
			case 'R':
				Request_PlayerInfos();		/* 새로고침 */
				break;
			case VK_ESCAPE:
				m_pClientApp->Disconnect();
				m_pClientApp->Change_Scene(MafiaClient::SCENE::CONNECT);
				break;
			default: break;
			}
		}
	}

	void CLobby::Update()
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
			Request_PlayerInfos();
	}

	void CLobby::Late_Update()
	{
	}

	void CLobby::Render()
	{
		if (m_isDirty == false && ConsoleUI::Need_Redraw() == false) return;
		m_isDirty = false;

		ConsoleUI::Render_Lobby(m_tView);
	}

	void CLobby::On_Packet(const Net::Event& tEvent)
	{
		Protocol::PACKET_TYPE ePacketType = static_cast<Protocol::PACKET_TYPE>(tEvent.Get_PacketType());
		
		switch (ePacketType)
		{
		case Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA:
		{
			const Protocol::PROVIDE_DATA* pData = reinterpret_cast<const Protocol::PROVIDE_DATA*>(tEvent.Get_Body());
			if (pData == nullptr) break;

			/* 플레이어 정보라면 */
			if (pData->eDataType == Protocol::DATA_TYPE::PLAYER_INFOS)
			{
				if (tEvent.Get_BodyLen() != sizeof(Protocol::PLAYER_INFOS)) break;		/* 크기 안 맞으면 버림 */
				const Protocol::PLAYER_INFOS* pInfo = reinterpret_cast<const Protocol::PLAYER_INFOS*>(tEvent.Get_Body());
				if (pInfo == nullptr) break;

				const uint16_t iCount = (std::min)(pInfo->iCount, static_cast<uint16_t>(Protocol::MAX_ONLINE_PLAYER));	/* 서버 값도 범위 검사 */
				size_t iCurSize = m_vecPlayerInfos.size();
				m_vecPlayerInfos.resize(iCurSize + iCount);
				memcpy(m_vecPlayerInfos.data() + iCurSize, pInfo->arr, iCount * sizeof(Protocol::PlayerInfo));

				Apply_PlayerInfos();
			}

			break;
		}
		default:	break;
		}
	}

	void CLobby::Request_PlayerInfos()
	{
		if (m_pClientApp->Get_Client() == nullptr) return;

		m_ullLastRequest = GetTickCount64();	/* 여기서 기록 → 자동 · 수동(R) 어느 쪽이든 그 시점부터 다시 셈 */
		m_vecPlayerInfos.clear();		/* 새 목록을 받을 거라 비움 (안 비우면 새로고침마다 같은 사람이 쌓임) */

		Protocol::REQ_DATA tPacket{};
		tPacket.eDataType = Protocol::DATA_TYPE::PLAYER_INFOS;
		m_pClientApp->Get_Client()->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_REQ_DATA), tPacket);
	}

	void CLobby::Apply_PlayerInfos()
	{
		static const wchar_t* szState[] = { L"로비", L"방", L"게임" };

		m_tView.vecPlayers.clear();
		for (Protocol::PlayerInfo& tInfo : m_vecPlayerInfos)
		{
			tInfo.wszNickname[Protocol::MAX_NICNAME_BUF - 1] = L'\0';	/* 서버 값도 끝은 막아 둠 */
			const std::wstring strState = (tInfo.eState < 3) ? szState[tInfo.eState] : L"?";
			m_tView.vecPlayers.push_back({ tInfo.iKey, tInfo.wszNickname, strState });
		}
		m_isDirty = true;
	}

	std::unique_ptr<CLobby> CLobby::Create()
	{
		std::unique_ptr<CLobby> pScene{ new CLobby() };
		if (FAILED(pScene->Initialize()))
			return nullptr;
		return pScene;
	}

	void CLobby::Register_NetEventCallBack()
	{
		m_pClientApp->Register_DefaultNetEvents();
		m_pClientApp->Register_NetEvents(Net::NET_EVENT_TYPE::Receive_Packet, [this](const Net::Event& tEvent) {On_Packet(tEvent); });
	}
}
