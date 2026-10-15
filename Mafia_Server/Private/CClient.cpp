#include "pch.h"
#include "CClient.h"


namespace MafiaServer
{
	CClient::CClient()
		: m_pAddress{nullptr}, m_pSocket{nullptr} , m_iKey{0}
	{
	}
	CClient::~CClient()
	{
	}


	HRESULT CClient::Initialize(ClientKey iKey,std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket)
	{
		m_pAddress	= std::move(pAddress);
		m_pSocket	= std::move(pSocket);

		if (m_pAddress == nullptr || m_pSocket == nullptr) return E_FAIL;

		m_iKey = iKey;

		return S_OK;
	}

	std::unique_ptr<CClient> MafiaServer::CClient::Create(ClientKey iKey,std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket)
	{
		std::unique_ptr<CClient> pClient{ new CClient };

		if (FAILED(pClient->Initialize(iKey,std::move(pAddress), std::move(pSocket))))
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
}