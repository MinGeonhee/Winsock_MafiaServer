#include "pch.h"
#include "CScene_Connect.h"
#include "CClientApp.h"


namespace MafiaClient
{
	CScene_Connect::CScene_Connect()
		: CScene(), m_strError{}, m_strLastIP{}
	{
		m_eType = MafiaClient::SCENE::CONNECT;
	}
	CScene_Connect::~CScene_Connect()
	{
	}

	HRESULT CScene_Connect::Initialize()
	{
		if (FAILED(__super::Initialize())) return E_FAIL;

		return S_OK;
	}

	void CScene_Connect::Enter()
	{
		__super::Enter();

		/* 처음엔 main에서 넘겨준 기본 주소 */
		const Net::CClient::Init_Data& tDefault = m_pClientApp->Get_InitData().tClientData;
		m_strLastIP = ConsoleUI::Widen(tDefault.szEnterServerIP);
		m_iLastPort = tDefault.iEnterServerPortNum;
	}

	void CScene_Connect::Exit()
	{
	}

	void CScene_Connect::Pre_Update()
	{
	}

	void CScene_Connect::Update()
	{
		/* 입력 화면 (Enter / Esc 까지 블로킹. 연결 전이라 막아도 문제 없음) */
		ConsoleUI::ConnectResult tResult = ConsoleUI::Run_ServerConnect(m_strLastIP, m_iLastPort, m_strError);
		if (tResult.isOk == false)
		{
			m_pClientApp->Quit();
			return;
		}
		m_strLastIP = ConsoleUI::Widen(tResult.strIP);
		m_iLastPort = tResult.iPort;

		ConsoleUI::Show_Connecting(tResult.strIP, tResult.iPort);

		const ULONGLONG ullStart = GetTickCount64();
		if (Try_Connect(tResult) == false)
		{
			/* 실패는 Windows 재시도로 이미 약 2초 걸렸으므로 연출 없이 바로 */
			m_strError = L"서버에 연결할 수 없습니다. IP와 포트를 확인하세요.";
			return;		/* 다음 프레임에 다시 입력 화면 */
		}

		/* 성공 : connect가 너무 빨라 화면이 깜빡 넘어가지 않게 최소 1.5초 연출 */
		ConsoleUI::Play_Connecting(tResult.strIP, tResult.iPort, 1500, static_cast<DWORD>(GetTickCount64() - ullStart));

		m_pClientApp->Change_Scene(MafiaClient::SCENE::LOGIN);
	}

	void CScene_Connect::Late_Update()
	{
	}

	void CScene_Connect::Render()
	{
	}

	bool CScene_Connect::Try_Connect(const ConsoleUI::ConnectResult& tResult)
	{
		Net::CClient::Init_Data tData{ m_pClientApp->Get_InitData().tClientData };
		tData.szEnterServerIP		= tResult.strIP.c_str();		/* tResult가 살아있는 동안만 유효 → Create 안에서만 사용 */
		tData.iEnterServerPortNum	= tResult.iPort;

		std::unique_ptr<Net::CClient> pClient = Net::CClient::Create(tData);
		if (pClient == nullptr) return false;

		m_pClientApp->Set_Connection(std::move(pClient), ConsoleUI::Widen(tResult.strIP) + L":" + std::to_wstring(tResult.iPort));
		return true;
	}

	void CScene_Connect::Register_NetEventCallBack()
	{
		/* 기존 Default CallBack함수등록 (접속 화면은 아직 연결 전이라 실제로는 아무것도 안 됨) */
		m_pClientApp->Register_DefaultNetEvents();
	}

	std::unique_ptr<CScene_Connect> CScene_Connect::Create()
	{
		std::unique_ptr<CScene_Connect> pScene{ new CScene_Connect() };
		if (FAILED(pScene->Initialize()))
			return nullptr;
		return pScene;
	}
}
