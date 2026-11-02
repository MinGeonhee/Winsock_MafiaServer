#include "pch.h"
#include "CClientApp.h"

#include "ConsoleUI.h"
#include "CScene.h"
#include "CScene_Intro.h"
#include "CScene_Connect.h"
#include "CScene_Login.h"
#include "CScene_Lobby.h"
#include "CScene_Room.h"
#include "CScene_Game.h"

namespace MafiaClient
{
	CClientApp::CClientApp()
		: m_tInitData{}, m_isRunning{ true }
		, m_pScene{ nullptr }, m_eCurScene{ MafiaClient::SCENE::END }, m_queueSceneRequests{}
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
		Change_Scene(MafiaClient::SCENE::INTRO);
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
			Change_Scene(MafiaClient::SCENE::CONNECT);		/* 연결이 없으면 접속 화면부터 */
		}

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

		/* 프레임 맨 끝 (Render까지 다 끝난 뒤)에서만 Scene 교체
		   → 이번 프레임 도중에는 어떤 Scene도 지워지지 않음 */
		Apply_SceneChange();
	}

	void CClientApp::Set_Connection(std::unique_ptr<Net::CClient> pClient, const std::wstring& strServer)
	{
		m_pClient = std::move(pClient);
		m_strNickname.clear();		/* 닉네임은 로그인 성공 후 Set_Nickname */
		m_strServer = strServer;

		Register_DefaultNetEvents();
	}

	void CClientApp::Disconnect()
	{
		m_pClient.reset();
		m_isLostConnection = false;
	}

	void CClientApp::Clear_NetEvents()
	{
		if (m_pClient == nullptr) return;
		m_pClient->Clear_CallBackNetEvent();
	}

	void CClientApp::Register_DefaultNetEvents()
	{
		if (m_pClient == nullptr) return;

		/* std::function은 멤버 함수를 바로 못 담음 (어느 객체의 함수인지 모름) → this를 잡은 람다로 연결 */
		m_pClient->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Receive_Packet,	[this](const Net::Event& tEvent) { On_ReceivePacket(tEvent); });
		m_pClient->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Send_Packet,		[this](const Net::Event& tEvent) { On_SendPacket(tEvent); });
		m_pClient->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Disconnect,		[this](const Net::Event& tEvent) { On_Disconnect(tEvent); });
	}

	void CClientApp::Register_NetEvents(Net::NET_EVENT_TYPE eType, Net::NetEventFuntion func)
	{
		if (m_pClient == nullptr) return;
		m_pClient->Resigster_CallBackNetEvent(eType, func);
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

	std::unique_ptr<CScene> CClientApp::Create_Scene(SCENE eScene, const std::any& anyArg)
	{
		switch (eScene)
		{
		case SCENE::INTRO:	return CScene_Intro::Create();
		case SCENE::CONNECT:return CScene_Connect::Create();
		case SCENE::LOGIN:	return CScene_Login::Create();
		case SCENE::LOBBY:	return CScene_Lobby::Create();
		case SCENE::ROOM:
		{
			/* 방 정보 없이는 방으로 못 감 (타입이 다르면 nullptr → 현재 Scene 유지) */
			const auto* pInfo = std::any_cast<Protocol::S_TO_C_ENTER_ROOM>(&anyArg);
			return pInfo ? CScene_Room::Create(*pInfo) : nullptr;
		}
		case SCENE::GAME:	return CScene_Game::Create();
		default:			return nullptr;
		}
	}

	void CClientApp::Apply_SceneChange()
	{
		/* 한 프레임에 요청이 여러 번 오면 마지막 요청만 적용
		   (중간 Scene을 만들었다 바로 지우면 Enter가 두 번 돌아 화면·등록이 꼬임) */
		if (m_queueSceneRequests.empty()) return;

		SceneRequest tNext = std::move(m_queueSceneRequests.back());
		while (!m_queueSceneRequests.empty()) m_queueSceneRequests.pop();
		const MafiaClient::SCENE eNext = tNext.eScene;

		if (eNext == m_eCurScene) return;

		std::unique_ptr<CScene> pNext = Create_Scene(eNext, tNext.anyArg);
		if (pNext == nullptr) return;	/* 만들기 실패 → 현재 Scene 유지 */

		if (m_pScene) m_pScene->Exit();
		m_pScene = std::move(pNext);
		m_eCurScene = eNext;
		m_pScene->Enter();
	}
}
