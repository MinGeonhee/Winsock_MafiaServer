#pragma once

namespace Net
{
	class CClient
	{
	public:
		struct Init_Data
		{
			std::string			strEnterIP{ Net::DEFAULT_SERVER_IP };
			PROTOCAL_TYPE		eProtocalType{ Net::PROTOCAL_TYPE::TCP };
			NET_PROTOCOL_TYPE	eNetProtocalType{ Net::NET_PROTOCOL_TYPE::IPv4 };
		};
	private:
		explicit CClient();
		CClient(const CClient& cCopy) = delete;
		CClient(CClient&& cMove) = delete;
	public:
		virtual ~CClient();
	private:
		HRESULT	Initialize(std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket);
		HRESULT	Initialize(const Init_Data& tData);
	public:
		ClientKey	Get_ClientKey() { return m_iKey; }
		uint16_t	Get_PortNum()const;
		std::string Get_IP() const;
		
	public:

	public:
		static	std::unique_ptr<CClient> Create(std::unique_ptr<Net::CAddress> pAddress, std::unique_ptr<Net::CSocket> pSocket);

		/* 실제 클라이언트에서 사용하는 클래스로서의 역할 */
		static	std::unique_ptr<CClient> Create(const Init_Data& tData);
	private:
		static ClientKey				m_iKey;
		std::unique_ptr<Net::CAddress>  m_pAddress;
		std::unique_ptr<Net::CSocket>   m_pSocket;
	};

}
