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

		/* 서버 연결이 끊겼으면 여기서 정리 (콜백 안에서 지우지 않음) */
		if (m_isLostConnection)
		{
			Disconnect();
			Change_Scene(MafiaClient::SCENE::LOGIN);
		}

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

		Register_NetEvents();
	}

	void CClientApp::Disconnect()
	{
		m_pClient.reset();
		m_isLostConnection = false;
	}

	void CClientApp::Register_NetEvents()
	{
		if (m_pClient == nullptr) return;

		/* std::function은 멤버 함수를 바로 못 담음 (어느 객체의 함수인지 모름) → this를 잡은 람다로 연결 */
		m_pClient->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Receive_Packet,	[this](const Net::Event& tEvent) { On_ReceivePacket(tEvent); });
		m_pClient->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Send_Packet,		[this](const Net::Event& tEvent) { On_SendPacket(tEvent); });
		m_pClient->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Disconnect,		[this](const Net::Event& tEvent) { On_Disconnect(tEvent); });
	}

	void CClientApp::On_ReceivePacket(const Net::Event& tEvent)
	{
		Protocol::PACKET_TYPE eType = static_cast<Protocol::PACKET_TYPE>(tEvent.Get_PacketType());

		switch (eType)
		{
		case Protocol::PACKET_TYPE::S_TO_C_LOGIN:

			break;
		default:	break;
		}

	}

	void CClientApp::On_SendPacket(const Net::Event& tEvent)
	{
	}

	void CClientApp::On_Disconnect(const Net::Event& tEvent)
	{
		/* 여기서 m_pClient.reset() 하면 안 됨: 지금 m_pClient의 함수 안에서 불리는 중 → 자기 자신이 지워짐
		   표시만 하고, m_pClient->Pre_Update()가 끝난 뒤에 정리 */
		m_isLostConnection = true;
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
