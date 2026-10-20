#include "pch.h"
#include "CLogin.h"
#include "CClientApp.h"


namespace MafiaClient
{
	CLogin::CLogin()
		: CScene(), m_eStep{ STEP::INPUT }, m_strError{}, m_strNickname{}
	{
		m_eType = MafiaClient::SCENE::LOGIN;
	}
	CLogin::~CLogin()
	{
	}

	HRESULT CLogin::Initialize()
	{
		if (FAILED(__super::Initialize())) return E_FAIL;

		return S_OK;
	}

	void CLogin::Enter()
	{
		__super::Enter();		/* 연결이 이미 있으므로 여기서 내 패킷 등록까지 끝남 */

		m_eStep = STEP::INPUT;
	}

	void CLogin::Exit()
	{
	}

	void CLogin::Pre_Update()
	{
	}

	void CLogin::Update()
	{
		/* 연결이 없으면(끊김) App이 CONNECT로 보내므로 여기서는 아무것도 안 함 */
		if (m_pClientApp->Get_Client() == nullptr) return;

		switch (m_eStep)
		{
		case STEP::INPUT:		Do_Input();	break;		/* 이 단계만 블로킹 (입력 화면이라 괜찮음) */
		case STEP::WAIT_LOGIN:				break;		/* 응답은 On_Packet으로 옴 */
		}
	}

	void CLogin::Late_Update()
	{
	}

	void CLogin::Render()
	{
	}

	void CLogin::Do_Input()
	{
		/* 로고 아래에 접속한 서버와 내 번호 표시 */
		const Net::ClientKey iKey = m_pClientApp->Get_Client()->Get_ClientKey();
		const std::wstring strInfo = L"서버 " + m_pClientApp->Get_ServerText() + (iKey != Net::NONE_CLIENTKEY ? L"  ·  #" + std::to_wstring(iKey) : L"");

		ConsoleUI::LoginResult tLogin = ConsoleUI::Run_Login(strInfo, m_strNickname, m_strError, Protocol::MAX_NICKNAME_LEN);
		if (tLogin.isOk == false)
		{
			/* Esc : 접속 화면으로 (콜백 안이 아니므로 바로 끊어도 됨) */
			m_pClientApp->Disconnect();
			m_pClientApp->Change_Scene(MafiaClient::SCENE::CONNECT);
			return;
		}

		m_strNickname = tLogin.strNickname;
		Send_Login(m_strNickname);
		ConsoleUI::Show_Status(L"⋯ 로그인 확인 중");
		m_eStep = STEP::WAIT_LOGIN;			/* 응답은 프레임이 돌면서 받음 */
	}

	void CLogin::Send_Login(const std::wstring& strNickname)
	{
		/* 요청만 쌓임 → 이번 프레임 Late_Update에서 실제 전송 */
		Protocol::C_TO_S_LOGIN tPacket{};
		wcsncpy_s(tPacket.wszNicName, strNickname.c_str(), _TRUNCATE);
		m_pClientApp->Get_Client()->Send_Packet(static_cast<uint16_t>(Protocol::PACKET_TYPE::C_TO_S_LOGIN), tPacket);
	}

	/* ---------------- 패킷 ---------------- */
	void CLogin::Register_NetEventCallBack()
	{
		/* 기존 Default CallBack함수등록 (끊김 정리 등) 먼저 */
		m_pClientApp->Register_DefaultNetEvents();

		/* 받은 패킷은 로그인 화면이 직접 처리 */
		if (Net::CClient* pClient = m_pClientApp->Get_Client())
			pClient->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Receive_Packet, [this](const Net::Event& tEvent) { On_Packet(tEvent); });
	}

	void CLogin::On_Packet(const Net::Event& tEvent)
	{
		switch (static_cast<Protocol::PACKET_TYPE>(tEvent.Get_PacketType()))
		{
		case Protocol::PACKET_TYPE::S_TO_C_LOGIN:			On_LoginSuccess(tEvent);	break;
		case Protocol::PACKET_TYPE::S_TO_C_LOGIN_FAILED:	On_LoginFailed(tEvent);		break;
		default: break;
		}
	}

	void CLogin::On_LoginSuccess(const Net::Event& tEvent)
	{
		if (m_eStep != STEP::WAIT_LOGIN) return;

		m_pClientApp->Set_Nickname(m_strNickname);
		m_pClientApp->Change_Scene(MafiaClient::SCENE::LOBBY);
	}

	void CLogin::On_LoginFailed(const Net::Event& tEvent)
	{
		if (m_eStep != STEP::WAIT_LOGIN) return;

		Protocol::S_TO_C_LOGIN_FAILED tPacket{};
		tPacket.eFailedType = Protocol::CHECK_NAME::END;		/* 크기가 안 맞으면 "알 수 없는 이유"로 */
		if (tEvent.Get_BodyLen() == sizeof(tPacket))
			memcpy(&tPacket, tEvent.Get_Body(), sizeof(tPacket));

		/* 실패 이유(enum) → 화면 문구 */
		switch (tPacket.eFailedType)
		{
		case Protocol::CHECK_NAME::EMPTY:				m_strError = L"닉네임을 입력하세요.";									break;
		case Protocol::CHECK_NAME::OVER:				m_strError = L"닉네임은 " + std::to_wstring(Protocol::MAX_NICKNAME_LEN) + L"글자까지 가능합니다.";	break;
		case Protocol::CHECK_NAME::INVALID_CHARACTER:	m_strError = L"닉네임은 한글, 영문, 숫자, _ 만 쓸 수 있습니다.";		break;
		default:										m_strError = L"로그인에 실패했습니다.";								break;
		}

		/* 연결은 그대로 두고 닉네임만 다시 입력 */
		m_eStep = STEP::INPUT;
	}

	std::unique_ptr<CLogin> CLogin::Create()
	{
		std::unique_ptr<CLogin> pScene{ new CLogin() };
		if (FAILED(pScene->Initialize()))
			return nullptr;
		return pScene;
	}
}
