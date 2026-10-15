#include "pch.h"
#include "CScene.h"
#include "CClientApp.h"

namespace MafiaClient
{
	CScene::CScene()
		: m_pClientApp{CClientApp::Get_Instance()} , m_eType{MafiaClient::SCENE::END}
	{

	}
}