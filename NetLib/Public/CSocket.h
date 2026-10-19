#pragma once



namespace Net{

	class CAddress;

	class CSocket
	{
		enum READY : BYTE
		{
			NONE = 0,
			BIND = 1 << 0,
			LISTEN = 1 << 1,
			ACTIVE = 1 << 2, 
		};
	private:
		explicit CSocket();
	public:
		virtual ~CSocket();
	private:
		HRESULT Initialize(TYPE eType, PROTOCAL_TYPE eProtocalType, NET_PROTOCOL_TYPE eNetProtocalType); // 초기화 함수
	public:
		//Server용
		bool Bind(const CAddress* pAddress); //해당 소켓을 Address 안에 있는 Port번호와 바인딩해주는 함수
		bool Listen(int iMaxClientNum = SOMAXCONN);
		bool Accept(OUT CAddress* pAddress, OUT CSocket* pSocket);
		//Client용
		bool			Connect(const CAddress* pAddress);
		//공용
		int				Get_LastError() { return m_iLastError; }

		const SOCKET	Get_Socket() const { return m_pSocket; }

		bool			Get_LocalAddress(CAddress* pOutAddress);

		TYPE			Get_Type()		   { return m_eType; }
		PROTOCAL_TYPE	Get_ProtocalType() { return m_eProtocalType; }
	public:
		static std::unique_ptr<CSocket> Create(TYPE eType, PROTOCAL_TYPE eProtocalType,NET_PROTOCOL_TYPE eNetProtocalType);
		static std::unique_ptr<CSocket> Create_Empty();
	private:
		/* 체크용 함수 */
		BYTE				m_eReady;
		TYPE				m_eType;
		PROTOCAL_TYPE		m_eProtocalType;
		SOCKET				m_pSocket; //윈도우 소켓
		int					m_iLastError;
	};

}

