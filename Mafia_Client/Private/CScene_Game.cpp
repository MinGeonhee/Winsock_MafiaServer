#include "pch.h"
#include "CScene_Game.h"
#include "CClientApp.h"
#include "ConsoleUI.h"


namespace MafiaClient
{
	CScene_Game::CScene_Game()
		: CScene(), m_isDirty{ true }
	{
		m_eType = MafiaClient::SCENE::GAME;
	}
	CScene_Game::~CScene_Game()
	{
	}

	HRESULT CScene_Game::Initialize()
	{
		if (FAILED(__super::Initialize())) return E_FAIL;

		return S_OK;
	}

	void CScene_Game::Enter()
	{
		__super::Enter();

		ConsoleUI::Clear();
		m_isDirty = true;
	}

	void CScene_Game::Exit()
	{
	}

	void CScene_Game::Pre_Update()
	{
		for (const KEY_EVENT_RECORD& tKey : ConsoleUI::Poll_Keys())
		{
			if (tKey.wVirtualKeyCode == VK_ESCAPE)
				m_pClientApp->Change_Scene(MafiaClient::SCENE::ROOM);
		}
	}

	void CScene_Game::Update()
	{
	}

	void CScene_Game::Late_Update()
	{
	}

	void CScene_Game::Render()
	{
		if (m_isDirty == false && ConsoleUI::Need_Redraw() == false) return;
		m_isDirty = false;

		/* TODO: ConsoleUI::Render_Game(...) 만들어지면 교체 */
		ConsoleUI::Write(L"\x1b[H\x1b[2J\n  게임 (준비 중)\n\n  [Esc] 돌아가기\n");
	}

	std::unique_ptr<CScene_Game> CScene_Game::Create()
	{
		std::unique_ptr<CScene_Game> pScene{ new CScene_Game() };
		if (FAILED(pScene->Initialize()))
			return nullptr;
		return pScene;
	}

	void CScene_Game::Register_NetEventCallBack()
	{
		/* 기존 Default CallBack함수등록 */
		m_pClientApp->Register_DefaultNetEvents();
	}
}
