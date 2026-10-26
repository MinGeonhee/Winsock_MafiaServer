#include "pch.h"
#include "CPlayer.h"

namespace MafiaServer
{
	CPlayer::CPlayer()
		: m_iClientKey{ Net::NONE_CLIENTKEY }, m_iRoomKey{ 0 }, m_strNickname{}, m_eState{ Protocol::PLAYER_STATE::LOBBY }, m_tRecord{}
	{
	}

	CPlayer::~CPlayer()
	{
	}

	HRESULT CPlayer::Initialize(uint64_t iKey, const std::wstring& strNickname)
	{
		if (iKey == 0 || strNickname.empty()) return E_FAIL;

		m_iClientKey = iKey;
		m_strNickname = strNickname;
		m_eState = Protocol::PLAYER_STATE::LOBBY;

		return S_OK;
	}

	void CPlayer::Add_Result(bool isWin)
	{
		++m_tRecord.iPlayCount;
		if (isWin) ++m_tRecord.iWinCount;
	}

	std::unique_ptr<CPlayer> CPlayer::Create(uint64_t iKey, const std::wstring& strNickname)
	{
		std::unique_ptr<CPlayer> pPlayer{ new CPlayer() };

		if (FAILED(pPlayer->Initialize(iKey, strNickname)))
			return nullptr;

		return pPlayer;
	}
}
