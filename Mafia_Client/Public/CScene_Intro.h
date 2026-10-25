#pragma once
#include "CScene.h"


namespace MafiaClient
{
	class CScene_Intro : public CScene
	{
	protected:
		CScene_Intro();
	public:
		virtual ~CScene_Intro();
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
		static std::unique_ptr<CScene_Intro> Create();

		// CScene을(를) 통해 상속됨
	};
}
