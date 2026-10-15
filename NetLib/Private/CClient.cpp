#include "pch.h"
#include "CClient.h"



namespace Net
{
	CClient::CClient()
		: m_pAddress{ nullptr }, m_pSocket{ nullptr }, m_iKey{ NONE_CLIENTKEY }, m_ClientEventQueue{}
	{
	}
	CClient::~CClient()
	{
	}

	HRESULT CClient::Initialize(ClientKey iClientKey,std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket)
	{
		m_pAddress = std::move(pAddress);
		m_pSocket = std::move(pSocket);

		if (m_pAddress == nullptr || m_pSocket == nullptr) return E_FAIL;

		m_iKey = iClientKey;

		return S_OK;
	}

	HRESULT CClient::Initialize(const Init_Data& tData)
	{
		if (tData.eNetProtocalType == NET_PROTOCOL_TYPE::END ||
			tData.eProtocalType == PROTOCAL_TYPE::END ||
			tData.iEnterServerPortNum == 0 || 
			tData.szEnterServerIP == nullptr ) return E_FAIL;
		
		/* TCP 家南 积己 */
		m_pSocket = Net::CSocket::Create(Net::TYPE::CLIENT, tData.eProtocalType, tData.eNetProtocalType);
		if (m_pSocket == nullptr) return E_FAIL;

		/* Connect */
		std::unique_ptr<CAddress> pServerAddress = Net::CAddress::Create(Net::TYPE::CLIENT, tData.eNetProtocalType,tData.iEnterServerPortNum, tData.szEnterServerIP);
		if (pServerAddress == nullptr) return E_FAIL;


		/* Connect 楷搬 夸没窍扁 */
		Net::Event tEvent{};
		tEvent.eEventType = Net::SERVER_EVENT_TYPE::Connect;
		tEvent.pAddress = std::move(pServerAddress);
		m_ClientEventQueue.Push_Event(std::move(tEvent));
	
		return S_OK;
	}

	void CClient::Handle_ClientEventQueue()
	{
		for (;;)
		{
			Event tEvent{};
			if (m_ClientEventQueue.Pop_Event(tEvent) == false) break;
			switch (tEvent.eEventType)
			{
			case Net::SERVER_EVENT_TYPE::Connect:			Handle_Connect(std::move(tEvent));		 break;
			case Net::SERVER_EVENT_TYPE::Receive_Packet:	break;
			case Net::SERVER_EVENT_TYPE::Send_Packet:		break;
			case Net::SERVER_EVENT_TYPE::Disconnect:		break;
			default: break;
			}
		}
	}

	void CClient::Handle_Connect(Net::Event&& tEvent)
	{
		if (m_pSocket == nullptr) return;

		if (m_pSocket->Connect(tEvent.pAddress.get()) == false)	return;

		m_pAddress = Net::CAddress::Create_Empty();
		if (m_pSocket->Get_LocalAddress(m_pAddress.get()) == false) return;
	}


	std::unique_ptr<CClient> CClient::Create(ClientKey iClientKey,std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket)
	{
		std::unique_ptr<CClient> pClient{ new CClient };

		if (FAILED(pClient->Initialize(iClientKey,std::move(pAddress), std::move(pSocket))))
			return nullptr;

		return pClient;
	}

	std::unique_ptr<CClient> CClient::Create(const Init_Data& tData)
	{
		std::unique_ptr<CClient> pClient{ new CClient };

		if (FAILED(pClient->Initialize(tData)))
			return nullptr;

		return pClient;
	}


	uint16_t CClient::Get_PortNum() const
	{
		return m_pAddress ? m_pAddress->Get_PortNum() : 0;
	}

	std::string CClient::Get_IP() const
	{
		return m_pAddress ? m_pAddress->Get_IP() : std::string();
	}


	void CClient::Pre_Update()
	{
		Handle_ClientEventQueue();
	}

	void CClient::Update()
	{
	}

	void CClient::Late_Update()
	{
	}

	void CClient::Render()
	{

	}

}