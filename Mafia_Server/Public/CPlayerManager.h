#pragma once


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
		/* 들고있는 Player 정보를 vector에 담아서 보내기  */
		std::vector<Protocol::PlayerInfo> Get_PlayerInfos();
	public:
		bool	LogIn_Player(std::unique_ptr<Protocol::CPlayer> pAddPlayer);
		bool	LogIn_Player(uint64_t iKey, const std::wstring& strNickname);
		bool	LogOut_Player(uint64_t iKey);

		/* 로그인한 플레이어 찾기 (없으면 nullptr) : 오래 들고 있지 말고 쓸 때마다 키로 찾기 */
		const Protocol::CPlayer*	Find_Player(uint64_t iKey) const;
	private:
		inline static CPlayerManager* m_pInstance{ nullptr };
	private:
		std::unordered_map< Net::ClientKey, std::unique_ptr<Protocol::CPlayer> > m_umapPlayers;
	};

}
