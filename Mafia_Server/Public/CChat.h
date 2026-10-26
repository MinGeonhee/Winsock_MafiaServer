#pragma once


namespace MafiaServer
{
	/* Chatting 기록을 모아둘 수 있는 */
	class CChat
	{
	public:
		struct Init_Data
		{
			SYSTEMTIME			tTime{};							/* 채팅 친 시간 */
			Net::ClientKey		iChatterKey{Net::NONE_CLIENTKEY};	/* 채팅 친 사람 */
			std::wstring		wstrText{};							/* 채팅 내역 */
		};
	private:
		explicit CChat();
		CChat(const CChat& cCopy) = delete;
		CChat(CChat&& cMove) = delete;
	public:
		virtual ~CChat();
	private:
		HRESULT	Initialize(const CChat::Init_Data& tData);

		const SYSTEMTIME			Get_Time()		 const { return m_tTime; }
		const Net::ClientKey		Get_ChatterKey() const { return m_iChatterKey; }
		const std::wstring&			Get_Text()		 const { return m_wstrText; }

	public:
		static std::unique_ptr<CChat>	Create(const Init_Data& tData);
	private:
		SYSTEMTIME			m_tTime;			/* 채팅을 작성한 시간 */
		Net::ClientKey		m_iChatterKey;		/* 채팅을 친 사람의 클라이언트 키 */
		std::wstring		m_wstrText;			/* 채팅 내역 */
	};
}


