#include "pch.h"
#include "CGameManager.h"

#include "CServer.h"

namespace MafiaServer
{
	CGameManager::CGameManager()
		: m_pMainServer{nullptr}
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


		return S_OK;	
	}

	void CGameManager::Start_Accept()
	{
	}

	void CGameManager::Pre_Update()
	{

	}
	void CGameManager::Update()
	{
	}
	void CGameManager::Late_Update()
	{
	}
	void CGameManager::Render()
	{
	}
}