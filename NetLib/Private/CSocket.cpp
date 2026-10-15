#include "pch.h"
#include "Net_Header.h"

namespace Net
{

	CSocket::CSocket()
		: m_eType{ TYPE::END }, m_pSocket{ INVALID_SOCKET }, m_iLastError{ 0 },
		m_eReady{ READY::NONE } , m_eProtocalType{PROTOCAL_TYPE::END }
	{
	}

	CSocket::~CSocket()
	{
		closesocket(m_pSocket);
	}

	HRESULT CSocket::Initialize(TYPE eType, PROTOCAL_TYPE eProtocalType, NET_PROTOCOL_TYPE eNetProtocalType)
	{

		if (static_cast<INT32>(eType) < 0 || eType >= TYPE::END) return E_FAIL;
		if (static_cast<INT32>(eType) < 0 || eProtocalType >= PROTOCAL_TYPE::END) return E_FAIL;
		if (static_cast<INT32>(eType) < 0 || eNetProtocalType >= NET_PROTOCOL_TYPE::END) return E_FAIL;

		m_eType = eType;
		m_eProtocalType = eProtocalType;

		/* TCP / UDP 프로토콜 타입에 맞게 socket 생성 */
		m_pSocket = socket(eNetProtocalType == NET_PROTOCOL_TYPE::IPv4 ? AF_INET : AF_INET6, 
			m_eProtocalType == PROTOCAL_TYPE::TCP ? SOCK_STREAM : SOCK_DGRAM, static_cast<int>(m_eProtocalType));
		if (m_pSocket == INVALID_SOCKET)
		{
			m_iLastError = WSAGetLastError();
			return E_FAIL;
		}

		return S_OK;
	}


#pragma region  Server Type 전용 함수

	bool CSocket::Bind(const CAddress* pAddress)
	{
		/* 순서가 Bind이상이라면 */
		if (m_eType != TYPE::SERVER)	return false;
		if (m_eReady >= READY::BIND)	return false;
		if (pAddress == nullptr)		return false;


		/* 나중에 받아올 주소값  CAddress 구현후 다시오자 */
		if (bind(m_pSocket, pAddress->Get_Addr(), pAddress->Get_Size()))
		{
			m_iLastError = WSAGetLastError();
			return false;
		}


		m_eReady |= BIND;

		return true;
	}

	bool CSocket::Listen(int iMaxClientNum)
	{
		if (m_eType != TYPE::SERVER)				return false;
		if ((m_eReady & READY::BIND) == false)		return false;

		if (listen(m_pSocket, iMaxClientNum) == SOCKET_ERROR)
		{
			m_iLastError = WSAGetLastError();
			return false;
		}

		m_eReady |= LISTEN;

		return true;
	}

	bool CSocket::Accept(OUT CAddress* pOutAddress, OUT CSocket* pOutSocket)
	{
		if (m_eType != TYPE::SERVER) return false;

		sockaddr_storage addr{};
		int iLen = sizeof(sockaddr_storage);

		SOCKET pClient = accept(m_pSocket, reinterpret_cast<SOCKADDR*>(&addr), &iLen);
		if(pClient == INVALID_SOCKET)
		{
			m_iLastError = WSAGetLastError();
			return false;
		}

		if (pOutSocket)
		{
			pOutSocket->m_pSocket = pClient;
			pOutSocket->m_eType = TYPE::CLIENT;
			pOutSocket->m_eReady = READY::NONE;
		}

		if (pOutAddress)
		{
			pOutAddress->Addr_Set(addr);
		}


		m_eReady |= READY::ACTIVE;


		return true;
	}


#pragma endregion

#pragma region Client Type 전용 함수

	bool CSocket::Connect(const CAddress* pAddress)
	{
		if (m_eType != TYPE::CLIENT)	return false;
		if (pAddress == nullptr)		return false;

		// 서버와 연결해주는 기능
		if (connect(m_pSocket, pAddress->Get_Addr(), pAddress->Get_Size()) == SOCKET_ERROR )
		{
			m_iLastError = WSAGetLastError();
			return false;
		}

		return true;
	}

#pragma endregion

#pragma region 공용


#pragma endregion

	bool CSocket::Get_LocalAddress(CAddress* pOutAddress)
	{
		if (pOutAddress == nullptr) return false;

		sockaddr_storage tAddr{};
		int iLen = sizeof(tAddr);
		if (getsockname(m_pSocket, reinterpret_cast<SOCKADDR*>(&tAddr), &iLen) == SOCKET_ERROR)
		{
			m_iLastError = WSAGetLastError();
			return false;
		}

		pOutAddress->Addr_Set(tAddr);   // CSocket은 CAddress의 friend라 private 함수 호출 가능

		return true;
	}

	std::unique_ptr<CSocket> CSocket::Create(TYPE eType, PROTOCAL_TYPE eProtocalType, NET_PROTOCOL_TYPE eNetProtocalType)
	{
		std::unique_ptr<CSocket> pSocket(new CSocket());

		/* 지역변수 처럼 써도되네 */
		if (FAILED(pSocket->Initialize(eType, eProtocalType, eNetProtocalType)))
			return nullptr;

		return pSocket;
	}

	std::unique_ptr<CSocket> CSocket::Create_Empty()
	{
		return std::unique_ptr<CSocket>(new CSocket());
	}
}