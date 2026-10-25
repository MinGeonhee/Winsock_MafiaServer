#pragma once
#include "CScene.h"
#include "ConsoleUI.h"


namespace MafiaClient
{
	/* 서버 접속 : IP / 포트 입력 → connect → 성공하면 LOGIN (로그인은 연결된 뒤에 따로) */
	class CScene_Connect : public CScene
	{
	protected:
		CScene_Connect();
	public:
		virtual ~CScene_Connect();
	protected:
		HRESULT	Initialize() override;
		void	Register_NetEventCallBack() override;
	public:
		void Enter() override;
		void Exit() override;
		void Pre_Update() override;
		void Update() override;
		void Late_Update() override;
		void Render() override;
	private:
		bool	Try_Connect(const ConsoleUI::ConnectResult& tResult);
	public:
		static std::unique_ptr<CScene_Connect> Create();
	private:
		std::wstring	m_strError;		/* 다음 입력 화면에 빨간 줄로 띄울 사유 */

		/* 실패 후 다시 띄울 때 직전 입력값 유지 */
		std::wstring	m_strLastIP;
		uint16_t		m_iLastPort{ 0 };
	};
}
