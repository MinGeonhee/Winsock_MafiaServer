#include "pch.h"
#include "CPlayerManager.h"

namespace  MafiaServer
{
	HRESULT CPlayerManager::Initialize(const Init_Data& tData)
	{
		return S_OK;
	}


	bool CPlayerManager::LogIn_Player(std::unique_ptr<CPlayer> pAddPlayer)
	{
		if (pAddPlayer == nullptr) return false;

		uint64_t iClientKey = pAddPlayer->Get_Key();
		auto iter = m_umapPlayers.find(iClientKey);
		if (iter != m_umapPlayers.end()) return false;

		m_umapPlayers[iClientKey] = std::move(pAddPlayer);

		return true;
	}

	bool CPlayerManager::LogIn_Player(uint64_t iKey, const std::wstring& strNickname)
	{
		if (strNickname.empty() || iKey == Net::NONE_CLIENTKEY ) return false;
		return LogIn_Player(std::move(CPlayer::Create(iKey, strNickname)));
	}

	bool CPlayerManager::LogOut_Player(uint64_t iKey)
	{
		auto iter = m_umapPlayers.find(iKey);
		if (iter == m_umapPlayers.end()) return false;
		m_umapPlayers.erase(iter);

		return true;
	}

	bool CPlayerManager::Set_PlayerState(uint64_t iKey, Protocol::PLAYER_STATE eState)
	{
		auto iter = m_umapPlayers.find(iKey);
		if (iter == m_umapPlayers.end()) return false;

		iter->second->Set_State(eState);
		return true;
	}

	const CPlayer* CPlayerManager::Find_Player(uint64_t iKey) const
	{
		auto iter = m_umapPlayers.find(iKey);
		return iter != m_umapPlayers.end() ? iter->second.get() : nullptr;
	}
}