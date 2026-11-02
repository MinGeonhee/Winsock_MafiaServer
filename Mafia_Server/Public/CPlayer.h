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
		Net::ClientKey			Get_ClientKey() const	{ return m_iClientKey; }
		Protocol::ROOM_KEY		Get_RoomKey()	const	{ return m_iRoomKey; }		/* 0 = 방에 없음 */
		const std::wstring&		Get_Nickname()	const	{ return m_strNickname; }
		Protocol::PLAYER_STATE	Get_State()		const	{ return m_eState; }
		const PlayerRecord&		Get_Record()	const	{ return m_tRecord; }

		void					Set_State(Protocol::PLAYER_STATE eState) { m_eState = eState; }
		void					Set_RoomKey(Protocol::ROOM_KEY iRoomKey) { m_iRoomKey = iRoomKey; }
		void					Add_Result(bool isWin);
	public:
		static std::unique_ptr<CPlayer>	Create(uint64_t iKey, const std::wstring& strNickname);
	private:
		Net::ClientKey			m_iClientKey{};		/* 연결 키 = 플레이어 키 */
		Protocol::ROOM_KEY		m_iRoomKey{};		/* 들어가 있는 방 (0 = 로비) */

		std::wstring			m_strNickname;
		Protocol::PLAYER_STATE	m_eState;
		PlayerRecord			m_tRecord;
	};
}
