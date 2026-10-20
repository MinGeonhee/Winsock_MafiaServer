#pragma once
#include <string>
#include <memory>
#include "Enum.h"
#include "Struct.h"

namespace Protocol
{
	class CPlayer
	{
	private:
		explicit CPlayer();
		CPlayer(const CPlayer& cCopy) = delete;
		CPlayer(CPlayer&& cMove) = delete;
	public:
		virtual ~CPlayer();
	private:
		HRESULT	Initialize(uint64_t iKey, const std::wstring& strNickname);
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
	public:
		static std::unique_ptr<CPlayer>	Create(uint64_t iKey, const std::wstring& strNickname);

		/* 받은 PlayerInfo로 만들기 (클라). 받은 값은 믿지 않음 : 닉네임 끝 널 문자, 상태 범위 확인 */
		static std::unique_ptr<CPlayer>	Create(const PlayerInfo& tInfo);
	private:
		uint64_t				m_iKey;
		std::wstring			m_strNickname;
		PLAYER_STATE			m_eState;
		PlayerRecord			m_tRecord;
	};
}
