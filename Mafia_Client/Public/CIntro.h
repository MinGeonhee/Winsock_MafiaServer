#pragma once
#include "CScene.h"


namespace MafiaClient
{
	class CIntro : public CScene
	{
	protected:
		CIntro();
	public:
		virtual ~CIntro();
	protected:
		HRESULT	Initialize() override;
	public:
		void Enter() override;
		void Exit() override;
		void Pre_Update() override;
		void Update() override;
		void Late_Update() override;
		void Render() override;
	public:
		static std::unique_ptr<CIntro> Create();
	};
}
