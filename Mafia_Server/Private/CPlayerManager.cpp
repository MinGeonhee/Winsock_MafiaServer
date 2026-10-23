#include "pch.h"
#include "CPlayerManager.h"

namespace  MafiaServer
{
	HRESULT CPlayerManager::Initialize(const Init_Data& tData)
	{
		return S_OK;
	}


	bool CPlayerManager::LogIn_Player(std::unique_ptr<Protocol::CPlayer> pAddPlayer)
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
		return LogIn_Player(std::move(Protocol::CPlayer::Create(iKey, strNickname)));
	}


	bool CPlayerManager::LogOut_Player(uint64_t iKey)
	{
		auto iter = m_umapPlayers.find(iKey);
		if (iter == m_umapPlayers.end()) return false;
		m_umapPlayers.erase(iter);

		return true;
	}

	const Protocol::CPlayer* CPlayerManager::Find_Player(uint64_t iKey) const
	{
		auto iter = m_umapPlayers.find(iKey);
		return iter != m_umapPlayers.end() ? iter->second.get() : nullptr;
	}

	std::vector<Protocol::PlayerInfo> CPlayerManager::Get_PlayerInfos()
	{
		std::vector<Protocol::PlayerInfo> vec{};

		for (const auto& Pair : m_umapPlayers)
			vec.push_back(Pair.second->To_Info());

		return vec;
	}
}