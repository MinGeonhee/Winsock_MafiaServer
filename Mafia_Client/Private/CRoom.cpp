#include "pch.h"
#include "CRoom.h"
#include "CClientApp.h"
#include "ConsoleUI.h"


namespace MafiaClient
{
	CRoom::CRoom()
		: CScene(), m_isDirty{ true }
	{
		m_eType = MafiaClient::SCENE::ROOM;
	}
	CRoom::~CRoom()
	{
	}

	HRESULT CRoom::Initialize()
	{
		return S_OK;
	}

	void CRoom::Enter()
	{
		ConsoleUI::Clear();
		m_isDirty = true;
	}

	void CRoom::Exit()
	{
	}

	void CRoom::Pre_Update()
	{
		for (const KEY_EVENT_RECORD& tKey : ConsoleUI::Poll_Keys())
		{
			if (tKey.wVirtualKeyCode == VK_ESCAPE)
				m_pClientApp->Change_Scene(MafiaClient::SCENE::LOBBY);
		}
	}

	void CRoom::Update()
	{
	}

	void CRoom::Late_Update()
	{
	}

	void CRoom::Render()
	{
		if (m_isDirty == false && ConsoleUI::Need_Redraw() == false) return;
		m_isDirty = false;

		/* TODO: ConsoleUI::Render_Room(...) 만들어지면 교체 */
		ConsoleUI::Write(L"\x1b[H\x1b[2J\n  방 (준비 중)\n\n  [Esc] 돌아가기\n");
	}

	std::unique_ptr<CRoom> CRoom::Create()
	{
		std::unique_ptr<CRoom> pScene{ new CRoom() };
		if (FAILED(pScene->Initialize()))
			return nullptr;
		return pScene;
	}
}
