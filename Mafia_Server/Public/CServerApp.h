#pragma once
#include "CPlayerManager.h"
#include "CRoomManager.h"

namespace Net
{
	class CServer;
}

namespace MafiaServer
{
	class CServerApp
	{
	public:
		struct Init_Data
		{
			Net::CServer::Init_Data		tServer_InitData{};
			CPlayerManager::Init_Data	tPlayerManager_InitData{};
			CRoomManager::Init_Data		tRoomManager_InitData{};
		};;
	private:
		explicit CServerApp();
		virtual ~CServerApp();
		CServerApp(const CServerApp& cCopy) = delete;
		CServerApp(CServerApp&& cMove) = delete;
	public:
		static CServerApp* Get_Instance()
		{
			if (m_pInstance == nullptr)
			{
				m_pInstance = new CServerApp();
			}
			return m_pInstance;
		}
		void	Destory();
		/* 기본으로 TCP 소켓 생성 */
		HRESULT Initialize(const Init_Data& tData);
	public:
		/* 서버로 들어오는 요청 받기 */
		void	Pre_Update();
		void	Update();
		void	Late_Update();
		void	Render();
	private:
		void	Key_Input();
		void	Allow_Accept();
	private:
		/* ---- NetLib 콜백 (m_pMainServer->Pre_Update 안에서 불림 = 로직 스레드) ---- */
		void	Register_NetEvents();
		void	On_Connect(const Net::Event& tEvent);
		void	On_RejectConnect(const Net::Event& tEvent);
		void	On_ReceivePacket(const Net::Event& tEvent);
		void	On_SendPacket(const Net::Event& tEvent);
		void	On_Disconnect(const Net::Event& tEvent);
	private:
		void	Handle_RequestData(const Net::Event& tEvent);
		void	Handle_CreateRoom(const Net::Event& tEvent);
		void	Handle_EnterRoom(const Net::Event& tEvent);
		void	Fill_Seats(const CRoom* pRoom, Protocol::ROOM_SEAT (&arrSeats)[Protocol::MAX_ROOM_PLAYER]);	/* 자리 7칸 채우기 (입장 / 자리 변경 공용) */
		void	Send_EnterRoom(Net::ClientKey iClientKey, Protocol::ROOM_KEY iRoomKey);						/* 방 정보 + 자리 7칸 보내기 */
		void	Broadcast_RoomMembers(Protocol::ROOM_KEY iRoomKey, Net::ClientKey iExceptKey);				/* 방 사람들에게 바뀐 자리 알리기 */
		void	Leave_Room(Net::ClientKey iClientKey);														/* Esc / 끊김 공용 */
	/* Mangaer 링크 */
	public:
		CPlayerManager* Get_PlayerManager() { return m_pPlayerManager; }
		CRoomManager*	Get_RoomManager()   { return m_pRoomManager; }

	/* 패킷 보내기 : NetLib 서버의 Send_Packet 연결 (Room 등에서 CServerApp::Get_Instance()->Send_Packet(...)으로 사용)
	   실제 전송은 이번 프레임 Late_Update에서 (보내기 목록에 쌓기만 함) */
	public:
		bool	Send_Packet(Net::ClientKey iKey, Protocol::PACKET_TYPE eType, const void* pBody, uint16_t usLen)
		{
			if (m_pMainServer == nullptr) return false;
			return m_pMainServer->Send_Packet(iKey, static_cast<uint16_t>(eType), pBody, usLen);
		}
		template<class T>
		bool	Send_Packet(Net::ClientKey iKey, Protocol::PACKET_TYPE eType, const T& tBody)
		{
			return Send_Packet(iKey, eType, &tBody, static_cast<uint16_t>(sizeof(T)));
		}
	private:
		inline static CServerApp* m_pInstance{ nullptr };
	private:
		/* Player Manager */
		CPlayerManager*				 m_pPlayerManager;
		CRoomManager*				 m_pRoomManager;

		/* 서버 관리 */
		std::unique_ptr<Net::CServer> m_pMainServer;
		bool						  m_isDirty;	// 바뀐 게 있을 때만 다시 그림

		/* 이번 프레임 시각 (ms, PC 켜진 뒤 경과) : 프레임 시작에 한 번 읽고 이 프레임 내내 같은 값 사용 */
		ULONGLONG					  m_ullNow{ 0 };
	};
}

