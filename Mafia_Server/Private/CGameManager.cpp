#include "pch.h"
#include "CGameManager.h"

#include "CServer.h"
#include "ConsoleUI.h"

namespace MafiaServer
{
	CGameManager::CGameManager()
		: m_pMainServer{nullptr} , m_isDirty{false}
	{
	}
	CGameManager::~CGameManager()
	{
	}

	void CGameManager::Destory()
	{
		m_pMainServer->Destory();
		m_pMainServer = nullptr;
	}

	HRESULT CGameManager::Initialize(const Init_Data& tData)
	{
		/* Server 만들고 초기화 */
		m_pMainServer = CServer::Get_Instance();
		if (FAILED(m_pMainServer->Initialize(tData.iUsingPortNumber))) return E_FAIL;

		/* 처음 서버를 열면 기본 적으로 클라이언트 연결을 받기 */
		this->m_pMainServer->Set_Accept(true);

		return S_OK;
	}

	void CGameManager::Pre_Update()
	{
		m_pMainServer->Pre_Update();


	}

	void CGameManager::Update()
	{
		Key_Input();

		m_pMainServer->Update();


	}

	void CGameManager::Late_Update()
	{
		m_pMainServer->Late_Update();
	}

	void CGameManager::Render()
	{
		if (m_isDirty == false && ConsoleUI::Need_Redraw() == false) return;


		ConsoleUI::Render_ServerMain(m_pMainServer->Get_PortNum(), m_pMainServer->Is_Accept(),m_pMainServer->Get_ClientView());


		m_isDirty = false;
	}


	void CGameManager::Key_Input()
	{
		/* 받아온 Key Input 처내기 */
		for (const KEY_EVENT_RECORD& tKey : ConsoleUI::Poll_Keys())
		{
			/* 클라이언트 접속 받기 On Off */
			if (tKey.wVirtualKeyCode == VK_F5) Allow_Accept();
		}
	}

	void CGameManager::Allow_Accept()
	{
		m_pMainServer->Set_Accept(!m_pMainServer->Is_Accept());
		m_isDirty = true;
	}
}