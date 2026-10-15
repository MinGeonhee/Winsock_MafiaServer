#pragma once

namespace MafiaServer
{
	class CClient
	{
	private:
		explicit CClient();
		CClient(const CClient& cCopy) = delete;
		CClient(CClient&& cMove) = delete;
	public:
		virtual ~CClient();
	private:
		HRESULT	Initialize(ClientKey iKey,std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket);
	public:
		ClientKey Get_ClientKey() { return m_iKey; }
		uint16_t	Get_PortNum()const;
		std::string Get_IP() const;

	public:
		static	std::unique_ptr<CClient> Create(ClientKey iKey,std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket);
	private:
		ClientKey						m_iKey;
		std::unique_ptr<Net::CAddress>  m_pAddress;
		std::unique_ptr<Net::CSocket>   m_pSocket;
	};
}


