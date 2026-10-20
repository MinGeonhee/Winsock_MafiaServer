#pragma once
#include "CScene.h"


namespace MafiaClient
{
	class CRoom : public CScene
	{
	protected:
		CRoom();
	public:
		virtual ~CRoom();
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
		static std::unique_ptr<CRoom> Create();
	private:
		bool	m_isDirty;
	};
}
