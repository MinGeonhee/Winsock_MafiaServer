#include "pch.h"
#include "CScene_Room.h"
#include "CClientApp.h"
#include "ConsoleUI.h"


namespace MafiaClient
{
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
		/* 막지 않는 입력 : 매 프레임 들어온 키만 처리 (그동안에도 패킷을 받아 화면이 바뀜) */
		for (const KEY_EVENT_RECORD& tKey : ConsoleUI::Poll_Keys(true))
		{
			if (tKey.wVirtualKeyCode == VK_ESCAPE)
			{
				/* 서버에 나간다고 알리고 로비로 (응답 기다리지 않음) */
				if (Net::CClient* pClient = m_pClientApp->Get_Client())
					pClient->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_EXIT_ROOM), nullptr, 0);
				m_pClientApp->Change_Scene(MafiaClient::SCENE::LOBBY);
				return;
			}
			if (tKey.wVirtualKeyCode == VK_F5)
			{
				/* TODO: 방장이면 게임 시작 요청 */
				continue;
			}

			switch (ConsoleUI::Edit_Line(m_tView.strInput, tKey, MAX_CHAT_LEN))
			{
			case ConsoleUI::LINE_INPUT::SUBMIT:
			{
				/* 빈 Text가 가지 못하도록 체크 */
				if (m_tView.strInput.empty()) break;

				Protocol::C_TO_S_CHAT tBody{};
				tBody.iRoomKey  = m_tRoomInfo.iRoomKey;
				wcsncpy_s(tBody.wszText, m_tView.strInput.c_str(), _TRUNCATE);
				m_pClientApp->Get_Client()->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_CHAT),tBody);
				m_tView.strInput.clear();
				break;
			}
			case ConsoleUI::LINE_INPUT::CHANGED:
				m_isDirty = true;
				break;
			default: break;
			}
		}
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
			tView.isMaster	= (tSeat.iKey == tInfo.iMasterKey);
		}
		m_isDirty = true;
	}

	void CScene_Room::Update()
	{
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
		case Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA:
		{
			const Protocol::PROVIDE_DATA* pData = reinterpret_cast<const Protocol::PROVIDE_DATA*>(tEvent.Get_Body());
			if (pData == nullptr) break;
			
			if (pData->eDataType == Protocol::DATA_TYPE::CHAT_LOG)
			{
				Protocol::CHAT_LOG tChat{};
				memcpy(&tChat, tEvent.Get_Body() , sizeof(tChat));                   // 복사본에서
				tChat.wszName[Protocol::MAX_NICNAME_BUF - 1] = L'\0';                // 끝 막기
				tChat.wszText[Protocol::MAX_CHAT_BUF - 1] = L'\0';

				m_tView.vecChats.push_back({ tChat.wszName, tChat.wszText, false , false });
				m_isDirty = true;
			}

			break;
		}

		default: break;
		}
	}
}
