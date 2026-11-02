// Mafia_Client.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include "pch.h"
#include "CClientApp.h"
using namespace std;

int main()
{
	MafiaClient::CClientApp::Init_Data tData{};
	tData.tClientData.szEnterServerIP = Net::DEFAULT_SERVER_IP;
	tData.tClientData.iEnterServerPortNum = 1234;

	MafiaClient::CClientApp* pApp = MafiaClient::CClientApp::Get_Instance();

	if (FAILED(pApp->Initialize(tData)))
	{
		pApp->Destory();
		return 0;
	}

	while (pApp->Is_Running())
	{
		pApp->Pre_Update();
		pApp->Update();
		pApp->Late_Update();
		pApp->Render();

		Sleep(16);	/* 약 60프레임. 콘솔이라 CPU 100% 안 먹게 */
	}

	pApp->Destory();	/* 안에서 Net_Release 호출 */
}
