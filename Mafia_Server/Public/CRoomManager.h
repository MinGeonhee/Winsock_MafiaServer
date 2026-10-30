#pragma once
#include "CRoom.h"

namespace MafiaServer
{
	class CRoomManager
	{
	public:
		struct Init_Data
		{
		};
	private:
		explicit CRoomManager();
		virtual ~CRoomManager();
		CRoomManager(const CRoomManager& cCopy) = delete;
		CRoomManager(CRoomManager&& cMove) = delete;
	public:
		static CRoomManager* Get_Instance()
		{
			if (m_pInstance == nullptr)
			{
				m_pInstance = new CRoomManager();
			}
			return m_pInstance;
		}
		void	Destory();
		HRESULT Initialize(const Init_Data& tData);
	public:
		const std::unordered_map<Protocol::ROOM_KEY, std::unique_ptr<CRoom>>& Get_Rooms() const { return m_umapRooms; }
	public:
		Protocol::ROOM_KEY	Create_Room(const std::wstring& wstrRoomName, Net::ClientKey iRoomMasterPlayerClientKey  );
		const CRoom*		Find_Room(Protocol::ROOM_KEY iEnterRoomKey);
		CRoom::ENTER_RESULT Enter_Room( Protocol::ROOM_KEY iEnterRoomKey  ,Net::ClientKey iEnterPlayerClientKey );
		/* 이 사람이 있는 방에서 빼기 (비면 방 삭제). 반환 : 나간 방 키 (방에 없었으면 0) */
		Protocol::ROOM_KEY	Exit_Room(Net::ClientKey iExitPlayerClientKey);
		void				Chatting(Protocol::ROOM_KEY iRoomKey, const SYSTEMTIME& tTime, Net::ClientKey iChatterKey, const std::wstring& wstrText, Net::CServer* pMainServer);
		void				Vote(Protocol::ROOM_KEY iRoomKey, Net::ClientKey iVoterKey, INT32 iSeat);
		void				LastVote(Protocol::ROOM_KEY iRoomKey, Net::ClientKey iVoterKey, bool is);
		void				Use_Skill(Net::ClientKey iUserKey, const Protocol::JOB_ACTION& tAction);	/* 방 키는 tAction.iRoomKey */

		/* 게임 시작 승인된 방 : 로딩(카운트다운) 시작 */
		bool				Start_Loading(Protocol::ROOM_KEY iRoomKey, ULONGLONG ullNow);

		/* 매 프레임 : Pre_Update / Update 둘 다 같은 이번 프레임 시각을 모든 방에 전달 (도는 중에 방을 지우지 않음) */
		void				Pre_Update(ULONGLONG ullNow);
		void				Update(ULONGLONG ullNow);
	private:
		inline static CRoomManager* m_pInstance{ nullptr };
	private:
		Protocol::ROOM_KEY	m_iRoomKey;
		std::unordered_map< Protocol::ROOM_KEY , std::unique_ptr<CRoom> > m_umapRooms;
	};
}

