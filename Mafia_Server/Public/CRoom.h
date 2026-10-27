#pragma once

namespace Net
{
	class CServer;
}


namespace MafiaServer
{
	class CChatting;
	class CMafiaLogic;
	class CServerApp;

	class CRoom
	{
	public:
		enum class STATE
		{
			WATING = 0,  // 게임 대기 상태
			LOADING,	 // 시작 후 로딩 상태
			GAME,		 // 게임 중
			END,
		};

		enum class ENTER_RESULT
		{
			OK,
			FULL_ROOM,
			START_GAME,
			ID_ERROR,
			END,
		};
		struct Init_Data
		{
			Protocol::ROOM_KEY iRoomKey{};				//Room Key
			Net::ClientKey     iRoomMasterClientKey{};	// Room Master Client Key
			std::wstring	   wstrRoomName{};			//Room Name;
		};
	private:
		CRoom();
		HRESULT Initialize(const Init_Data& tData);
	public:
		virtual ~CRoom();
	public:
		const wchar_t*	   Get_Name()				const { return m_wszRoomName; }
		Protocol::ROOM_KEY Get_Key()				const { return m_iRoomKey; }
		uint16_t		   Get_CurPlayerNum()		const { return m_iCurPlayerNum; }
		bool			   Get_IsPlaying()			const;
		Net::ClientKey	   Get_MasterKey()			const { return m_iRoomMasterClientKey; }
		/* 자리 7칸 (인덱스 = 자리 번호 - 1, 빈자리 = NONE_CLIENTKEY) */
		const std::array<Net::ClientKey, Protocol::MAX_ROOM_PLAYER>& Get_PlayerKeys() const { return m_arrayPlayerClientKeys; }
		bool			   Has_Player(Net::ClientKey iKey) const
		{
			return iKey != Net::NONE_CLIENTKEY && std::find(m_arrayPlayerClientKeys.begin(), m_arrayPlayerClientKeys.end(), iKey) != m_arrayPlayerClientKeys.end();
		}
		bool				Can_Start() const ;
		/* 시작 승인 : 로딩(카운트다운) 시작. START_COUNTDOWN_MS 뒤 Update에서 게임으로 */
		void				Start_Loading(ULONGLONG ullNow);
	public:
		/* 방 참여 신청 받기 */
		ENTER_RESULT	Enter(Net::ClientKey iEnterPlayerClientKey);
		void			Exit(Net::ClientKey iExitPlayerClientKey);

		/* 매 프레임 : 시각은 CServerApp이 한 번 읽어 Pre_Update / Update에 같은 값으로 내려줌. 게임 중이면 게임 로직에 그대로 전달 */
		void			Pre_Update(ULONGLONG ullNow);
		void			Update(ULONGLONG ullNow);
		void			LateUpdate(ULONGLONG ullNow);

		/* 채팅 관련 */
		void			Chatting(const SYSTEMTIME& tTime, Net::ClientKey iChatterKey, const std::wstring& wstrText, Net::CServer* pMainServere);

		/* 방안의 모든 사람에게 패킷보내기 */
		void			SendPacket_All(Protocol::PACKET_TYPE tType, const void* pBody, uint16_t tBodyLen, Net::CServer* pMainServer) const;

	private:
		void			Change_Master();
		void			Start_Game(ULONGLONG ullNow);		/* 로딩 끝 : 게임 상태 + 게임 로직 생성 */
	public:
		static std::unique_ptr<CRoom> Create(const Init_Data& tData);
	private:
		wchar_t				m_wszRoomName[Protocol::MAX_ROOMNAME_BUF];
		CServerApp*			m_pServerApp;
		STATE				m_eCurState;
		Protocol::ROOM_KEY	m_iRoomKey; //방 전용 키
		Net::ClientKey		m_iRoomMasterClientKey;

		uint16_t			m_iCurPlayerNum;
		std::array< Net::ClientKey , Protocol::MAX_ROOM_PLAYER > m_arrayPlayerClientKeys;
		std::unique_ptr<CChatting>		m_pChatting;
		std::unique_ptr<CMafiaLogic>	m_pLogic;		/* 게임 중일 때만 (게임 시작에 만들고 끝나면 reset) */
		ULONGLONG						m_ullLoadingStart{ 0 };	/* 로딩(카운트다운) 시작 시각 */
	};
}

