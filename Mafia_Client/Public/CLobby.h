#pragma once
#include "CScene.h"
#include "ConsoleUI.h"


namespace MafiaClient
{
	class CLobby : public CScene
	{
	protected:
		CLobby();
	public:
		virtual ~CLobby();
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
	public:
		static std::unique_ptr<CLobby> Create();
	private:
		ConsoleUI::LobbyView	m_tView;		/* 화면에 그릴 로비 정보 (서버 데이터 오면 여기 채움) */
		bool					m_isDirty;
	};
}
