#pragma once
#include "CScene.h"


namespace MafiaClient
{
	class CScene_Game : public CScene
	{
	protected:
		CScene_Game();
	public:
		virtual ~CScene_Game();
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
		static std::unique_ptr<CScene_Game> Create();
	private:
		bool	m_isDirty;
	};
}
