#pragma once

namespace MafiaServer
{
	/* 전적 (나중에 직업별 승수, 연승 등 추가) : 서버만 들고 있음 */
	struct PlayerRecord
	{
		uint32_t	iPlayCount{ 0 };
		uint32_t	iWinCount{ 0 };
	};

	/* 서버가 들고 있는 진짜 플레이어. 패킷 모양은 모름 (보낼 때 CServerApp이 필요한 것만 골라 담음) */
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
		Protocol::PLAYER_STATE	Get_State() const		{ return m_eState; }
		const PlayerRecord&		Get_Record() const		{ return m_tRecord; }

		void					Set_State(Protocol::PLAYER_STATE eState) { m_eState = eState; }
		void					Add_Result(bool isWin);
	public:
		static std::unique_ptr<CPlayer>	Create(uint64_t iKey, const std::wstring& strNickname);
	private:
		uint64_t				m_iKey;
		std::wstring			m_strNickname;
		Protocol::PLAYER_STATE	m_eState;
		PlayerRecord			m_tRecord;
	};
}
