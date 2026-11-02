#include "pch.h"
#include "CScene_Intro.h"
#include "CClientApp.h"
#include "ConsoleUI.h"


namespace MafiaClient
{
	CScene_Intro::CScene_Intro()
		: CScene()
	{
		m_eType = MafiaClient::SCENE::INTRO;
	}
	CScene_Intro::~CScene_Intro()
	{
	}

	HRESULT CScene_Intro::Initialize()
	{
		if (FAILED(__super::Initialize())) return E_FAIL;

		return S_OK;
	}

	void CScene_Intro::Enter()
	{
		__super::Enter();

		/* 로고 애니메이션 → 아무 키 입력까지 여기서 블로킹 (네트워크 연결 전이라 막아도 문제 없음) */
		ConsoleUI::Play_Intro();
		m_pClientApp->Change_Scene(MafiaClient::SCENE::CONNECT);
	}

	void CScene_Intro::Exit()
	{
	}

	void CScene_Intro::Pre_Update()
	{
	}

	void CScene_Intro::Update()
	{
	}

	void CScene_Intro::Late_Update()
	{
	}

	void CScene_Intro::Render()
	{
	}

	std::unique_ptr<CScene_Intro> CScene_Intro::Create()
	{
		std::unique_ptr<CScene_Intro> pScene{ new CScene_Intro() };
		if (FAILED(pScene->Initialize()))
			return nullptr;
		return pScene;
	}
	void CScene_Intro::Register_NetEventCallBack()
	{
		/* 기존 Default CallBack함수등록 */
		m_pClientApp->Register_DefaultNetEvents();
	}
}
