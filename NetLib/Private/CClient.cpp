#include "pch.h"
#include "CClient.h"



namespace Net
{
	ClientKey CClient::m_iKey{ 1 };

	CClient::CClient()
		: m_pAddress{ nullptr }, m_pSocket{ nullptr }
	{
	}
	CClient::~CClient()
	{
	}


	HRESULT CClient::Initialize(std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket)
	{
		m_pAddress = std::move(pAddress);
		m_pSocket = std::move(pSocket);

		if (m_pAddress == nullptr || m_pSocket == nullptr) return E_FAIL;

		m_iKey = m_iKey++;

		return S_OK;
	}

	HRESULT CClient::Initialize(const Init_Data& tData)
	{
		if (tData.eNetProtocalType == NET_PROTOCOL_TYPE::END || tData.eProtocalType == PROTOCAL_TYPE::END) return E_FAIL;
		
		/* TCP 家南 积己 */
		m_pSocket = Net::CSocket::Create(Net::TYPE::CLIENT, tData.eProtocalType, tData.eNetProtocalType);
		if (m_pSocket == nullptr) return E_FAIL;

		/* 惑措规 IP 林家.. */
		m_pAddress = Net::CAddress::Create(Net::TYPE::SERVER, tData.eNetProtocalType,);
		if (m_pAddress == nullptr) return E_FAIL;

		m_pSocket->Connect(m_pAddress.get());

		return S_OK;
	}

	std::unique_ptr<CClient> CClient::Create(std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket)
	{
		std::unique_ptr<CClient> pClient{ new CClient };

		if (FAILED(pClient->Initialize(std::move(pAddress), std::move(pSocket))))
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

}