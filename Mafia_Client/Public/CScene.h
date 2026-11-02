#pragma once

namespace MafiaClient
{
	class CClientApp;

	class CScene
	{
	protected:
		CScene();
	public:
		virtual ~CScene() { m_pClientApp = nullptr; }
	protected:
		virtual HRESULT	Initialize();
		virtual void	Register_NetEventCallBack() = 0;
	public:
		virtual void Enter();		/* Scene이 화면에 올라올 때 1번 */
		virtual void Exit() = 0;		/* Scene이 내려갈 때 1번 */

		virtual void Pre_Update() = 0;	/* 입력 / 네트워크 */
		virtual void Update() = 0;		/* 로직 */
		virtual void Late_Update() = 0;	/* Scene 전환 판단 */
		virtual void Render() = 0;		/* 바뀐 게 있을 때만 그리기 */ 

	public:
		MafiaClient::SCENE Get_Type() const { return m_eType; }
	protected:
		MafiaClient::SCENE	m_eType;
		CClientApp*			m_pClientApp;
	};
}
