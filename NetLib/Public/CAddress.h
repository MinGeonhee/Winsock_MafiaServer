#pragma once


namespace Net
{
	class CSocket;

	class CAddress
	{
		friend class Net::CSocket;   // CSocket만 접근 허용
	private:
		CAddress();
	public:
		virtual ~CAddress();
	private:
		HRESULT	Initialize(TYPE eType, NET_PROTOCOL_TYPE eNetProtocalType, uint16_t  iPortNum , const char* szIP);
		void			Addr_Set(const sockaddr_storage& tAddr);
	public:
		uint16_t		Get_PortNum() const;
		std::string		Get_IP()	  const;
	private:
		const SOCKADDR* Get_Addr() const { return reinterpret_cast<const SOCKADDR*>(&m_addr); }
		int				Get_Size() const;
	public:
		static std::unique_ptr<CAddress> Create(TYPE eType, NET_PROTOCOL_TYPE eNetProtocalType, uint16_t iPortNum, const char* szIP = nullptr);
		static std::unique_ptr<CAddress> Create_Empty();
	private:
		NET_PROTOCOL_TYPE	m_eNetProtocalType;
		sockaddr_storage	m_addr; //IPv4, IPv6 둘중 하나를 담을 포인터
	};
}