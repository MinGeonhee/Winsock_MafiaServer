#include "pch.h"
#include "CServerApp.h"

#include "CServer.h"
#include "ConsoleUI.h"
#include "ConsoleUI_Server.h"
#include "CPlayerManager.h"

namespace MafiaServer
{
	CServerApp::CServerApp()
		: m_pMainServer{nullptr} , m_isDirty{false} , m_pPlayerManager{}
	{
	}
	CServerApp::~CServerApp()
	{
	}

	CPlayerManager::CPlayerManager()
	{
	}

	CPlayerManager::~CPlayerManager()
	{
	}

	void CServerApp::Destory()
	{
		m_pMainServer.reset();

		Net::Net_Release();
	}

	HRESULT CServerApp::Initialize(const Init_Data& tData)
	{
		Net::Net_Init();

		/* Server 만들고 초기화 */
		m_pMainServer = Net::CServer::Create(tData.tServer_InitData);
		if (m_pMainServer == nullptr) return E_FAIL;
		Register_NetEvents();


		/* Player Manager 만들기 */
		m_pPlayerManager = CPlayerManager::Get_Instance();
		if (FAILED(m_pPlayerManager->Initialize(tData.tPlayerManager_InitData))) return E_FAIL;

		return S_OK;
	}


	void CServerApp::Pre_Update()
	{
		m_pMainServer->Pre_Update();


	}

	void CServerApp::Update()
	{
		Key_Input();

		m_pMainServer->Update();


	}

	void CServerApp::Late_Update()
	{
		m_pMainServer->Late_Update();
	}

	void CServerApp::Render()
	{
		if (m_isDirty == false && ConsoleUI::Need_Redraw(*m_pMainServer) == false) return;

		ConsoleUI::Render_ServerMain(*m_pMainServer);

		m_isDirty = false;
	}


	void CServerApp::Key_Input()
	{
		/* 받아온 Key Input 처내기 */
		for (const KEY_EVENT_RECORD& tKey : ConsoleUI::Poll_Keys())
		{
			/* 클라이언트 접속 받기 On Off */
			if (tKey.wVirtualKeyCode == VK_F5) Allow_Accept();
		}
	}

	void CServerApp::Allow_Accept()
	{
		m_pMainServer->Set_Accept(!m_pMainServer->Is_Accept());
		m_isDirty = true;
	}

	void CServerApp::Register_NetEvents()
	{
		/* std::function은 멤버 함수를 바로 못 담음 (어느 객체의 함수인지 모름) → this를 잡은 람다로 연결 */
		m_pMainServer->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Connect,			[this](const Net::Event& tEvent) { On_Connect(tEvent); });
		m_pMainServer->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Reject_Connect,	[this](const Net::Event& tEvent) { On_RejectConnect(tEvent); });
		m_pMainServer->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Receive_Packet,	[this](const Net::Event& tEvent) { On_ReceivePacket(tEvent); });
		m_pMainServer->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Send_Packet,		[this](const Net::Event& tEvent) { On_SendPacket(tEvent); });
		m_pMainServer->Resigster_CallBackNetEvent(Net::NET_EVENT_TYPE::Disconnect,		[this](const Net::Event& tEvent) { On_Disconnect(tEvent); });
	}

	void CServerApp::On_Connect(const Net::Event& tEvent)
	{
		/* tEvent.iKey : 새로 들어온 클라이언트 (CPlayer는 로그인 패킷을 받은 뒤에 생성) */
		m_isDirty = true;
	}

	void CServerApp::On_RejectConnect(const Net::Event& tEvent)
	{
		m_isDirty = true;
	}

	void CServerApp::On_ReceivePacket(const Net::Event& tEvent)
	{
		/* TODO: 헤더 꺼내서 Protocol::PACKET_TYPE 별로 switch (C_TO_S_LOGIN 등) */
		Protocol::PACKET_TYPE tType = static_cast<Protocol::PACKET_TYPE>(tEvent.Get_PacketType());

		switch (tType)
		{
		case Protocol::PACKET_TYPE::C_TO_S_LOGIN:
		/* Client -> Server에게 본인 아이디를 줌 */
		{
			if (tEvent.Is_NetPacket()) return;

			uint16_t iLen = tEvent.Get_BodyLen();
			if (iLen != sizeof(Protocol::C_TO_S_LOGIN)) break;
			Net::Packet<Protocol::C_TO_S_LOGIN> tPacket{};
			memcpy(&tPacket.tData, tEvent.Get_Body(),iLen);

			/* 닉네임 체크하기 */
			Protocol::CHECK_NAME eCheck =  Protocol::Is_VailedNickName(tPacket.tData.wszNicName);

			switch (eCheck)
			{
			case Protocol::CHECK_NAME::OK:
			{
				/* OK 라면 Client 번호 연결 시켜주기 */
				Protocol::S_TO_C_LOGIN tLoginBody{};
				tLoginBody.iClientKey = tEvent.iKey;
				m_pMainServer->Send_Packet(tEvent.iKey, static_cast<uint16_t>(Protocol::PACKET_TYPE::S_TO_C_LOGIN), tLoginBody);

				if (m_pPlayerManager->LogIn_Player(tEvent.iKey, tPacket.tData.wszNicName) == false) break;

				break;
			}
			case Protocol::CHECK_NAME::EMPTY:
			case Protocol::CHECK_NAME::OVER:
			case Protocol::CHECK_NAME::INVALID_CHARACTER:
			{
				/* 로그인 실패 이유 보내주기 */
				Protocol::S_TO_C_LOGIN_FAILED tLoginFailedBody{};
				tLoginFailedBody.eFailedType = eCheck;
				m_pMainServer->Send_Packet(tEvent.iKey, static_cast<uint16_t>(Protocol::PACKET_TYPE::S_TO_C_LOGIN_FAILED), tLoginFailedBody);
				break;
			}
			default:break;
			}
			break;
		}
		case Protocol::PACKET_TYPE::C_TO_S_REQ_DATA:
		{
			Handle_RequestData(tEvent);
			break;
		}
		case Protocol::PACKET_TYPE::END:			break;
		default:									break;
		}
	}

	void CServerApp::On_SendPacket(const Net::Event& tEvent)
	{
	}

	void CServerApp::On_Disconnect(const Net::Event& tEvent)
	{
		/* TODO: tEvent.iKey의 CPlayer 정리 (방에서 빼기 등) */
		if (m_pPlayerManager->LogOut_Player(tEvent.iKey) == false) return;
		m_isDirty = true;
	}

	void CServerApp::Handle_RequestData(const Net::Event& tEvent)
	{

		uint16_t iLen = tEvent.Get_BodyLen();
		if (iLen != sizeof(Protocol::REQ_DATA)) return;
		Net::Packet<Protocol::REQ_DATA> tPacket{};
		memcpy(&tPacket.tData, tEvent.Get_Body(), sizeof(Protocol::REQ_DATA));

		/* 플레이어 목록 요청 */
		if (tPacket.tData.eDataType == Protocol::DATA_TYPE::PLAYER_INFOS)
		{
			std::vector<Protocol::PlayerInfo> vec = std::move(m_pPlayerManager->Get_PlayerInfos());
			size_t iPre{ 0 };
			/* 최대값을 나눠서 진행 */
			while (iPre < vec.size())
			{
				/* Info 복사 */
				Protocol::PLAYER_INFOS tBody{};
				tBody.eDataType = Protocol::DATA_TYPE::PLAYER_INFOS;
				tBody.iCount = static_cast<uint16_t>((std::min)(vec.size() - iPre, Protocol::MAX_ONLINE_PLAYER));
				memcpy(tBody.arr , vec.data() + iPre , tBody.iCount * sizeof(Protocol::PlayerInfo));
				m_pMainServer->Send_Packet(tEvent.iKey, static_cast<uint16_t>(Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA),tBody);
				iPre += tBody.iCount;
			}
		}
	}
}