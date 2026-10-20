#pragma once
#include <string>
#include "Enum.h"
#include "Struct.h"

namespace Protocol
{
	/* 전적 (나중에 직업별 승수, 연승 등 추가) */
	struct PlayerRecord
	{
		uint32_t	iPlayCount{ 0 };
		uint32_t	iWinCount{ 0 };
	};

	/* 로그인한 플레이어 한 명 (서버 · 클라 공용)
	   - 담는 것 : 누구인지(키, 닉네임), 지금 어디 있는지(상태), 전적
	   - 담지 않는 것 : 직업 · 투표 같은 "이번 판" 정보 → 게임(CGame)이 관리
	   - 연결(소켓)은 모름 : 키로만 Net::CClient와 이어짐 */
	class CPlayer
	{
	public:
		CPlayer() = default;
		CPlayer(uint64_t iKey, const std::wstring& strNickname);
	public:
		uint64_t				Get_Key() const			{ return m_iKey; }
		const std::wstring&		Get_Nickname() const	{ return m_strNickname; }
		PLAYER_STATE			Get_State() const		{ return m_eState; }
		const PlayerRecord&		Get_Record() const		{ return m_tRecord; }

		void					Set_State(PLAYER_STATE eState) { m_eState = eState; }
		void					Add_Result(bool isWin);
	public:
		/* 네트워크 변환 : CPlayer는 wstring이 있어 그대로 못 보내므로 고정 크기 PlayerInfo로 바꿔서 주고받음 */
		PlayerInfo				To_Info() const;
		static CPlayer			From_Info(const PlayerInfo& tInfo);
	private:
		uint64_t		m_iKey{ 0 };
		std::wstring	m_strNickname{};
		PLAYER_STATE	m_eState{ PLAYER_STATE::LOBBY };
		PlayerRecord	m_tRecord{};
	};
}