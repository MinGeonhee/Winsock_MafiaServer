#pragma once
#include "CScene.h"
#include "ConsoleUI.h"


namespace MafiaClient
{
	/* 로그인 : 이미 서버에 연결된 상태에서 닉네임 입력 → C_TO_S_LOGIN → 응답 대기 → 성공하면 LOBBY */
	class CScene_Login : public CScene
	{
		/* 로그인 진행 단계 */
		enum class STEP
		{
			INPUT,			/* 닉네임 입력 → 로그인 패킷 전송 */
			WAIT_LOGIN,		/* 서버 응답(성공 / 실패) 기다리는 중 */
		};
	protected:
		CScene_Login();
	public:
		virtual ~CScene_Login();
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
		void	Do_Input();
		void	Send_Login(const std::wstring& strNickname);

		/* 패킷 (패킷 하나에 함수 하나) */
		void	On_Packet(const Net::Event& tEvent);
		void	On_LoginSuccess(const Net::Event& tEvent);
		void	On_LoginFailed(const Net::Event& tEvent);
	public:
		static std::unique_ptr<CScene_Login> Create();
	private:
		STEP			m_eStep;
		std::wstring	m_strError;		/* 다음 입력 화면에 빨간 줄로 띄울 사유 */
		std::wstring	m_strNickname;	/* 마지막으로 입력한 닉네임 (실패 후 다시 띄울 때 유지, 성공하면 App에 저장) */

		/* 로그인 대기 연출 : 응답이 너무 빨라 화면이 깜빡 넘어가지 않게 최소 시간 동안 "로그인 중" 표시 */
		static constexpr ULONGLONG	LOGIN_MIN_MS		= 1000;		/* 성공해도 이 시간은 보여줌 */
		static constexpr ULONGLONG	LOGIN_TIMEOUT_MS	= 5000;		/* 이 시간 동안 응답이 없으면 실패 처리 */
		ULONGLONG		m_ullWaitStart{ 0 };
		bool			m_isLoginOk{ false };	/* 성공 응답을 받았는지 (받아도 최소 시간까지는 대기) */
	};
}
