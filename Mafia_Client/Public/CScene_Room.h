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
		void	Show_Notice(const std::wstring& strText);	/* 안내 줄에 빨간 알림 (NOTICE_MS 뒤 자동으로 사라짐) */
		void	Apply_Phase(Protocol::GAME_PHASE ePhase, uint16_t iDay, uint32_t iRemainMs);	/* 서버가 알려준 게임 단계 → 원탁 가운데 표시 */
		void	Update_PhaseTime();							/* 남은 시간 표시 (초가 바뀔 때만 다시 그림) */
	public:
		/* 입장 응답(S_TO_C_ENTER_ROOM)을 받아서 만듦 : 방 정보는 이 Scene이 들고 갱신함 */
		static std::unique_ptr<CScene_Room> Create(const Protocol::S_TO_C_ENTER_ROOM& tRoomInfo);
	private:
		Protocol::S_TO_C_ENTER_ROOM	m_tRoomInfo{};
		static constexpr size_t		MAX_CHAT_LEN{ 60 };
		ConsoleUI::RoomScreenView	m_tView;
		bool						m_isDirty;

		static constexpr ULONGLONG	NOTICE_MS{ 3000 };
		ULONGLONG					m_ullNoticeEnd{ 0 };	/* 알림이 사라질 시각 */

		/* 게임 시작 카운트다운 (표시용 : 실제 시작 판정은 서버) */
		ULONGLONG					m_ullStartAt{ 0 };		/* 게임이 시작될 시각 (0 = 카운트다운 아님) */
		int							m_iLastCount{ 0 };		/* 마지막으로 띄운 초 (같은 초를 두 번 띄우지 않게) */

		/* 게임 중 (서버가 알려준 값 그대로 : 클라는 판정하지 않음) */
		Protocol::JOB				m_eJob{ Protocol::JOB::UNKNOWN };		/* 내 직업 */
		Protocol::GAME_PHASE		m_ePhase{ Protocol::GAME_PHASE::END };	/* 지금 단계 */
		ULONGLONG					m_ullPhaseEnd{ 0 };		/* 이번 단계가 끝나는 시각 (내 시계 기준, 표시용) */
		int							m_iLastRemainSec{ -1 };	/* 마지막으로 그린 남은 초 */
	};
}
