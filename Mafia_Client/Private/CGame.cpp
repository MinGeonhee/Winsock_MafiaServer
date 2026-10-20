#include "pch.h"
#include "CGame.h"
#include "CClientApp.h"
#include "ConsoleUI.h"


namespace MafiaClient
{
	CGame::CGame()
		: CScene(), m_isDirty{ true }
	{
		m_eType = MafiaClient::SCENE::GAME;
	}
	CGame::~CGame()
	{
	}

	HRESULT CGame::Initialize()
	{
		if (FAILED(__super::Initialize())) return E_FAIL;

		return S_OK;
	}

	void CGame::Enter()
	{
		__super::Enter();

		ConsoleUI::Clear();
		m_isDirty = true;
	}

	void CGame::Exit()
	{
	}

	void CGame::Pre_Update()
	{
		for (const KEY_EVENT_RECORD& tKey : ConsoleUI::Poll_Keys())
		{
			if (tKey.wVirtualKeyCode == VK_ESCAPE)
				m_pClientApp->Change_Scene(MafiaClient::SCENE::ROOM);
		}
	}

	void CGame::Update()
	{
	}

	void CGame::Late_Update()
	{
	}

	void CGame::Render()
	{
		if (m_isDirty == false && ConsoleUI::Need_Redraw() == false) return;
		m_isDirty = false;

		/* TODO: ConsoleUI::Render_Game(...) 만들어지면 교체 */
		ConsoleUI::Write(L"\x1b[H\x1b[2J\n  게임 (준비 중)\n\n  [Esc] 돌아가기\n");
	}

	std::unique_ptr<CGame> CGame::Create()
	{
		std::unique_ptr<CGame> pScene{ new CGame() };
		if (FAILED(pScene->Initialize()))
			return nullptr;
		return pScene;
	}

	void CGame::Register_NetEventCallBack()
	{
		/* 기존 Default CallBack함수등록 */
		m_pClientApp->Register_DefaultNetEvents();
	}
}
