#include "pch.h"
#include "CLogin.h"
#include "CClientApp.h"
#include "ConsoleUI.h"


namespace MafiaClient
{
	CLogin::CLogin()
		: CScene()
	{
		m_eType = MafiaClient::SCENE::LOGIN;
	}
	CLogin::~CLogin()
	{
	}

	HRESULT CLogin::Initialize()
	{
		return S_OK;
	}

	void CLogin::Enter()
	{
		const Net::CClient::Init_Data& tDefault = m_pClientApp->Get_InitData().tClientData;
		std::wstring strError{};

		/* 접속 성공 or Esc 할 때까지 반복 (입력 화면은 블로킹) */
		for (;;)
		{
			ConsoleUI::LoginResult tLogin = ConsoleUI::Run_Login(ConsoleUI::Widen(tDefault.szEnterServerIP), tDefault.iEnterServerPortNum, strError);
			if (tLogin.isOk == false)
			{
				m_pClientApp->Quit();
				return;
			}

			ConsoleUI::Show_Connecting(tLogin.strIP, tLogin.iPort);

			Net::CClient::Init_Data tData{ tDefault };
			tData.szEnterServerIP		= tLogin.strIP.c_str();		/* tLogin이 살아있는 동안만 유효 → Create 안에서만 사용 */
			tData.iEnterServerPortNum	= tLogin.iPort;

			std::unique_ptr<Net::CClient> pClient = Net::CClient::Create(tData);
			if (pClient == nullptr)
			{
				strError = L"서버에 연결할 수 없습니다. IP와 포트를 확인하세요.";
				continue;
			}

			const std::wstring strServer = ConsoleUI::Widen(tLogin.strIP) + L":" + std::to_wstring(tLogin.iPort);
			m_pClientApp->Set_Connection(std::move(pClient), tLogin.strNickname, strServer);

			/* Client 에서 Server에게 닉네임 전송하기 (요청만 쌓임 → 이번 프레임 Late_Update에서 실제 전송) */
			Protocol::C_TO_S_LOGIN tLoginPacket{};
			wcsncpy_s(tLoginPacket.wszNicName, tLogin.strNickname.c_str(), _TRUNCATE);
			m_pClientApp->Get_Client()->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_LOGIN), tLoginPacket);
			m_pClientApp->Change_Scene(MafiaClient::SCENE::LOBBY);

			return;
		}
	}

	void CLogin::Exit()
	{
	}

	void CLogin::Pre_Update()
	{
	}

	void CLogin::Update()
	{
	}

	void CLogin::Late_Update()
	{
	}

	void CLogin::Render()
	{
	}

	std::unique_ptr<CLogin> CLogin::Create()
	{
		std::unique_ptr<CLogin> pScene{ new CLogin() };
		if (FAILED(pScene->Initialize()))
			return nullptr;
		return pScene;
	}
}
