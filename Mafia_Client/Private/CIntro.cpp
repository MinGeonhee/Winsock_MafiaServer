#include "pch.h"
#include "CIntro.h"
#include "CClientApp.h"
#include "ConsoleUI.h"


namespace MafiaClient
{
	CIntro::CIntro()
		: CScene()
	{
		m_eType = MafiaClient::SCENE::INTRO;
	}
	CIntro::~CIntro()
	{
	}

	HRESULT CIntro::Initialize()
	{
		if (FAILED(__super::Initialize())) return E_FAIL;

		return S_OK;
	}

	void CIntro::Enter()
	{
		__super::Enter();

		/* 로고 애니메이션 → 아무 키 입력까지 여기서 블로킹 (네트워크 연결 전이라 막아도 문제 없음) */
		ConsoleUI::Play_Intro();
		m_pClientApp->Change_Scene(MafiaClient::SCENE::CONNECT);
	}

	void CIntro::Exit()
	{
	}

	void CIntro::Pre_Update()
	{
	}

	void CIntro::Update()
	{
	}

	void CIntro::Late_Update()
	{
	}

	void CIntro::Render()
	{
	}

	std::unique_ptr<CIntro> CIntro::Create()
	{
		std::unique_ptr<CIntro> pScene{ new CIntro() };
		if (FAILED(pScene->Initialize()))
			return nullptr;
		return pScene;
	}
	void CIntro::Register_NetEventCallBack()
	{
		/* 기존 Default CallBack함수등록 */
		m_pClientApp->Register_DefaultNetEvents();
	}
}
