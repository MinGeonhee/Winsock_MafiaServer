#include "pch.h"
#include "CAddress.h"
#include <iostream>

namespace Net
{

	CAddress::CAddress()
		: m_addr{} , m_eNetProtocalType{NET_PROTOCOL_TYPE::NET_PROTOCOL_TYPE_END}
	{
	}

	CAddress::~CAddress()
	{
	}

	HRESULT CAddress::Initialize(TYPE eType, NET_PROTOCOL_TYPE eNetProtocalType, uint16_t  iPortNum, const char* szIP)
	{
		if (eType < 0 || eType >= TYPE::TYPE_END) return E_FAIL;
		if (eNetProtocalType < 0 || eNetProtocalType >= NET_PROTOCOL_TYPE::NET_PROTOCOL_TYPE_END) return E_FAIL;

		m_eNetProtocalType = eNetProtocalType;

		/* 4,6 경우에 따라서 나누기 */
		if (m_eNetProtocalType == NET_PROTOCOL_TYPE::IPv4)
		{
			SOCKADDR_IN* pIPv4 = reinterpret_cast<SOCKADDR_IN*>(&m_addr);
			pIPv4->sin_family = AF_INET;
			pIPv4->sin_addr.s_addr = htonl(INADDR_ANY);
			pIPv4->sin_port = htons(iPortNum);

			if (eType == TYPE::CLIENT)
			{
				if (inet_pton(AF_INET, szIP, &pIPv4->sin_addr) != 1)
				{
					int i = WSAGetLastError();
					std::cout << WSAGetLastError() << std::endl;
					return E_FAIL;
				}
			}
		}
		else if (m_eNetProtocalType == NET_PROTOCOL_TYPE::IPv6)
		{
			SOCKADDR_IN6* pIPv6 = reinterpret_cast<SOCKADDR_IN6*>(&m_addr);
			pIPv6->sin6_family = AF_INET6;
			pIPv6->sin6_addr = in6addr_any;
			pIPv6->sin6_port = htons(iPortNum);

			if (eType == TYPE::CLIENT)
			{
				if (inet_pton(AF_INET6, szIP, &pIPv6->sin6_addr) != 1)
				{
					std::cout << WSAGetLastError() << std::endl;
					return E_FAIL;
				}
			}
		}

		return S_OK;
	}

	void CAddress::Addr_Set(const sockaddr_storage& tAddr)
	{
		/* Addr_Set으로불 */
		m_addr = tAddr;
		tAddr.ss_family == AF_INET ? this->m_eNetProtocalType = NET_PROTOCOL_TYPE::IPv4 :
									 this->m_eNetProtocalType = NET_PROTOCOL_TYPE::IPv6;
	}


	int CAddress::Get_Size() const
	{
		return (m_addr.ss_family == AF_INET)
			? sizeof(sockaddr_in)
			: sizeof(sockaddr_in6);
	}


	uint16_t	CAddress::Get_PortNum() const
	{
		return m_eNetProtocalType == Net::NET_PROTOCOL_TYPE::IPv4 ?
			ntohs(reinterpret_cast<const SOCKADDR_IN*>(&m_addr)->sin_port) :
			ntohs(reinterpret_cast<const SOCKADDR_IN6*>(&m_addr)->sin6_port);
	}


	std::string	CAddress::Get_IP()	  const
	{
		char szIp[INET6_ADDRSTRLEN]{};

		if (m_eNetProtocalType == Net::NET_PROTOCOL_TYPE::IPv4)
		{
			const SOCKADDR_IN* p = reinterpret_cast<const SOCKADDR_IN*>(&m_addr);
			inet_ntop(AF_INET,&p->sin_addr,szIp,sizeof(szIp));
		}	
		if (m_eNetProtocalType == Net::NET_PROTOCOL_TYPE::IPv6)
		{
			const SOCKADDR_IN6* p = reinterpret_cast<const SOCKADDR_IN6*>(&m_addr);
			inet_ntop(AF_INET6, &p->sin6_addr, szIp, sizeof(szIp));
		}

		return std::string(szIp);
	}

	std::unique_ptr<CAddress> CAddress::Create(TYPE eType, NET_PROTOCOL_TYPE eNetProtocalType, uint16_t iPortNum, const char* szIP)
	{
		std::unique_ptr<CAddress> pAddress(new CAddress());

		if (FAILED((pAddress->Initialize(eType, eNetProtocalType, iPortNum, szIP))))
			return nullptr;

		return pAddress;
	}

	std::unique_ptr<CAddress> CAddress::Create_Empty()
	{
		return std::unique_ptr<CAddress>(new CAddress());
	}

}