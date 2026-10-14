// Mafia_Server.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include "pch.h"
#include "CGameManager.h"
#include "ConsoleUI.h"

using namespace std;
using namespace MafiaServer;


void Accept(Net::CSocket* pSocket)
{

}

int main()
{
	ConsoleUI::Init(L"Mafia Server");

	CGameManager::Init_Data tData{};
	CGameManager* m_pCGameManager =  CGameManager::Get_Instance();

	/* Server에서 사용할 Port번호 설정 (bind 실패 시 다시 입력) */
	std::wstring strError;
	while (true)
	{
		tData.iUsingPortNumber = ConsoleUI::Run_StartupScreen(1234, strError);
		if (SUCCEEDED(m_pCGameManager->Initialize(tData))) break;

		strError = std::to_wstring(tData.iUsingPortNumber) + L"번 포트를 열 수 없습니다. (이미 사용 중일 수 있음)";
	}
	/* 윈도우 방화벽 설정 관련 쓰기 */

	ConsoleUI::Write(std::wstring(L"\n  ") + ConsoleUI::Color::Green + L"● " + ConsoleUI::Color::Reset
		+ std::to_wstring(tData.iUsingPortNumber) + L"번 포트에서 접속 대기 중\n");

	while (true)
	{
		m_pCGameManager->Pre_Update();
		m_pCGameManager->Update();
		m_pCGameManager->Late_Update();
		m_pCGameManager->Render();
	}

}
