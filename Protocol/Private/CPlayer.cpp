#include "pch.h"
#include "CPlayer.h"

namespace Protocol
{
	CPlayer::CPlayer(uint64_t iKey, const std::wstring& strNickname)
		: m_iKey{ iKey }, m_strNickname{ strNickname }, m_eState{ PLAYER_STATE::LOBBY }, m_tRecord{}
	{
	}

	void CPlayer::Add_Result(bool isWin)
	{
		++m_tRecord.iPlayCount;
		if (isWin) ++m_tRecord.iWinCount;
	}

	PlayerInfo CPlayer::To_Info() const
	{
		PlayerInfo tInfo{};
		tInfo.iKey		= m_iKey;
		tInfo.eState	= static_cast<uint8_t>(m_eState);
		wcsncpy_s(tInfo.wszNickname, m_strNickname.c_str(), _TRUNCATE);
		return tInfo;
	}

	CPlayer CPlayer::From_Info(const PlayerInfo& tInfo)
	{
		/* 받은 값은 믿지 않음 : 닉네임 끝 널 문자, 상태 범위 확인 */
		wchar_t wszNick[MAX_NICNAME_BUF]{};
		wcsncpy_s(wszNick, tInfo.wszNickname, _TRUNCATE);

		CPlayer tPlayer{ tInfo.iKey, wszNick };
		if (tInfo.eState < static_cast<uint8_t>(PLAYER_STATE::END))
			tPlayer.m_eState = static_cast<PLAYER_STATE>(tInfo.eState);
		return tPlayer;
	}
}