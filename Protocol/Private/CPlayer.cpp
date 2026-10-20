#include "pch.h"
#include "CPlayer.h"

namespace Protocol
{
	CPlayer::CPlayer()
		: m_iKey{ 0 }, m_strNickname{}, m_eState{ PLAYER_STATE::LOBBY }, m_tRecord{}
	{
	}

	CPlayer::~CPlayer()
	{
	}

	HRESULT CPlayer::Initialize(uint64_t iKey, const std::wstring& strNickname)
	{
		if (iKey == 0 || strNickname.empty()) return E_FAIL;

		m_iKey = iKey;
		m_strNickname = strNickname;
		m_eState = PLAYER_STATE::LOBBY;

		return S_OK;
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

	std::unique_ptr<CPlayer> CPlayer::Create(uint64_t iKey, const std::wstring& strNickname)
	{
		std::unique_ptr<CPlayer> pPlayer{ new CPlayer() };

		if (FAILED(pPlayer->Initialize(iKey, strNickname)))
			return nullptr;

		return pPlayer;
	}

	std::unique_ptr<CPlayer> CPlayer::Create(const PlayerInfo& tInfo)
	{
		/* 닉네임 끝에 널 문자가 없을 수도 있으므로 복사하면서 막아 둠 */
		wchar_t wszNick[MAX_NICNAME_BUF]{};
		wcsncpy_s(wszNick, tInfo.wszNickname, _TRUNCATE);

		std::unique_ptr<CPlayer> pPlayer = Create(tInfo.iKey, wszNick);
		if (pPlayer == nullptr) return nullptr;

		if (tInfo.eState < static_cast<uint8_t>(PLAYER_STATE::END))
			pPlayer->m_eState = static_cast<PLAYER_STATE>(tInfo.eState);

		return pPlayer;
	}
}
