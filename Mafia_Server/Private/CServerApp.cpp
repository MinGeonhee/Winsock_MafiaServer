#include "pch.h"
#include "CServerApp.h"

#include "CServer.h"
#include "ConsoleUI.h"
#include "ConsoleUI_Server.h"

namespace MafiaServer
{
	CServerApp::CServerApp()
		: m_pMainServer{nullptr} , m_isDirty{false}
	{
	}
	CServerApp::~CServerApp()
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
		m_pMainServer = Net::CServer::Create(tData.tServerData);


		return S_OK;
	}

	void CServerApp::Pre_Update()
	{
		m_pMainServer->Pre_Update();


	}

	void CServerApp::Update()
	{
		Key_Input();

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
}