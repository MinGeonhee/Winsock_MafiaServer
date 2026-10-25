#pragma once
#include "CPlayer.h"


namespace MafiaServer
{

	class CPlayerManager
	{
	public:
		struct Init_Data
		{
		};
	private:
		explicit CPlayerManager();
		virtual ~CPlayerManager();
		CPlayerManager(const CPlayerManager& cCopy) = delete;
		CPlayerManager(CPlayerManager&& cMove) = delete;
	public:
		static CPlayerManager* Get_Instance()
		{
			if (m_pInstance == nullptr)
			{
				m_pInstance = new CPlayerManager();
			}
			return m_pInstance;
		}
		void	Destory();
		HRESULT Initialize(const Init_Data& tData);
	public:
		/* 로그인한 플레이어 전체 (읽기 전용) : 무엇을 보낼지는 받는 쪽(CServerApp)이 고름 */
		const std::unordered_map<Net::ClientKey, std::unique_ptr<CPlayer>>& Get_Players() const { return m_umapPlayers; }
	public:
		bool	LogIn_Player(std::unique_ptr<CPlayer> pAddPlayer);
		bool	LogIn_Player(uint64_t iKey, const std::wstring& strNickname);
		bool	LogOut_Player(uint64_t iKey);

		/* 로그인한 플레이어 찾기 (없으면 nullptr) : 오래 들고 있지 말고 쓸 때마다 키로 찾기 */
		const CPlayer*	Find_Player(uint64_t iKey) const;

		/* 플레이어 상태 바꾸기 (로비 ↔ 방 ↔ 게임). 없는 키면 false */
		bool			Set_PlayerState(uint64_t iKey, Protocol::PLAYER_STATE eState);
	private:
		inline static CPlayerManager* m_pInstance{ nullptr };
	private:
		std::unordered_map< Net::ClientKey, std::unique_ptr<CPlayer> > m_umapPlayers;
	};

}
