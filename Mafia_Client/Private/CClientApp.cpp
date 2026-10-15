#include "pch.h"
#include "CClientApp.h"

#include "ConsoleUI.h"
#include "CScene.h"
#include "CIntro.h"
#include "CLogin.h"
#include "CLobby.h"
#include "CRoom.h"
#include "CGame.h"

namespace MafiaClient
{
	CClientApp::CClientApp()
		: m_tInitData{}, m_isRunning{ true }
		, m_pScene{ nullptr }, m_eCurScene{ MafiaClient::SCENE::END }, m_eNextScene{ MafiaClient::SCENE::INTRO }
		, m_pClient{ nullptr }
	{
	}
	CClientApp::~CClientApp()
	{
	}

	void CClientApp::Destory()
	{
		/* Scene → 연결 → Winsock 순서로 정리 */
		if (m_pScene) m_pScene->Exit();
		m_pScene.reset();
		m_pClient.reset();

		Net::Net_Release();

		delete m_pInstance;
		m_pInstance = nullptr;
	}

	HRESULT CClientApp::Initialize(const Init_Data& tData)
	{
		m_tInitData = tData;

		if (Net::Net_Init() == false) return E_FAIL;
		ConsoleUI::Init(L"Mafia Client");

		/* 첫 Scene(INTRO)을 바로 만들어 둠 */
		Apply_SceneChange();
		return m_pScene ? S_OK : E_FAIL;
	}

	void CClientApp::Pre_Update()
	{
		if (m_pClient) m_pClient->Pre_Update();

		Apply_SceneChange();
		if (m_pScene) m_pScene->Pre_Update();
	}

	void CClientApp::Update()
	{
		if (m_pClient) m_pClient->Update();

		if (m_pScene) m_pScene->Update();
	}

	void CClientApp::Late_Update()
	{
		if (m_pClient) m_pClient->Late_Update();

		if (m_pScene) m_pScene->Late_Update();
	}

	void CClientApp::Render()
	{
		if (m_pScene) m_pScene->Render();
	}

	void CClientApp::Set_Connection(std::unique_ptr<Net::CClient> pClient, const std::wstring& strNickname, const std::wstring& strServer)
	{
		m_pClient = std::move(pClient);
		m_strNickname = strNickname;
		m_strServer = strServer;
	}

	void CClientApp::Disconnect()
	{
		m_pClient.reset();
	}

	std::unique_ptr<CScene> CClientApp::Create_Scene(SCENE eScene)
	{
		switch (eScene)
		{
		case SCENE::INTRO:	return CIntro::Create();
		case SCENE::LOGIN:	return CLogin::Create();
		case SCENE::LOBBY:	return CLobby::Create();
		case SCENE::ROOM:	return CRoom::Create();
		case SCENE::GAME:	return CGame::Create();
		default:			return nullptr;
		}
	}

	void CClientApp::Apply_SceneChange()
	{
		if (m_eNextScene == m_eCurScene) return;

		std::unique_ptr<CScene> pNext = Create_Scene(m_eNextScene);
		if (pNext == nullptr) { m_eNextScene = m_eCurScene; return; }	/* 만들기 실패 → 현재 Scene 유지 */

		if (m_pScene) m_pScene->Exit();
		m_pScene = std::move(pNext);
		m_eCurScene = m_eNextScene;
		m_pScene->Enter();
	}
}
