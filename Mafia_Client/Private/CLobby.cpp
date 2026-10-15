#include "pch.h"
#include "CLobby.h"
#include "CClientApp.h"


namespace MafiaClient
{
	CLobby::CLobby()
		: CScene(), m_tView{}, m_isDirty{ true }
	{
		m_eType = MafiaClient::SCENE::LOBBY;
	}
	CLobby::~CLobby()
	{
	}

	HRESULT CLobby::Initialize()
	{
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
		ConsoleUI::Set_CursorVisible(false);
		ConsoleUI::Clear();
		m_isDirty = true;
	}

	void CLobby::Exit()
	{
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
			case VK_ESCAPE:
				m_pClientApp->Disconnect();
				m_pClientApp->Change_Scene(MafiaClient::SCENE::LOGIN);
				break;
			default: break;
			}
		}
	}

	void CLobby::Update()
	{
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

	std::unique_ptr<CLobby> CLobby::Create()
	{
		std::unique_ptr<CLobby> pScene{ new CLobby() };
		if (FAILED(pScene->Initialize()))
			return nullptr;
		return pScene;
	}
}
