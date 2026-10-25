#pragma once
#include "CScene.h"
#include "ConsoleUI.h"


namespace MafiaClient
{
	class CScene_Room : public CScene
	{
	protected:
		CScene_Room();
	public:
		virtual ~CScene_Room();
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
		void	Apply_RoomInfo();		/* m_tRoomInfo → 화면용 m_tView */
		void	On_Packet(const Net::Event& tEvent);
	public:
		/* 입장 응답(S_TO_C_ENTER_ROOM)을 받아서 만듦 : 방 정보는 이 Scene이 들고 갱신함 */
		static std::unique_ptr<CScene_Room> Create(const Protocol::S_TO_C_ENTER_ROOM& tRoomInfo);
	private:
		Protocol::S_TO_C_ENTER_ROOM	m_tRoomInfo{};
		static constexpr size_t		MAX_CHAT_LEN{ 60 };
		ConsoleUI::RoomScreenView	m_tView;
		bool						m_isDirty;
	};
}
