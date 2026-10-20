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
	private:
		void	On_Packet(const Net::Event& tEvent);
		void	Request_PlayerInfos();		/* 서버에 접속 중인 플레이어 목록 요청 (받은 목록은 비우고 새로 받음) */
		void	Apply_PlayerInfos();		/* 받은 PlayerInfo → 화면용 MemberView로 바꾸고 다시 그리기 */
	public:
		static std::unique_ptr<CLobby> Create();
	private:
		std::vector<Protocol::PlayerInfo>	m_vecPlayerInfos;
		ConsoleUI::LobbyView				m_tView;			/* 화면에 그릴 로비 정보 (서버 데이터 오면 여기 채움) */
		bool								m_isDirty;
	};
}
