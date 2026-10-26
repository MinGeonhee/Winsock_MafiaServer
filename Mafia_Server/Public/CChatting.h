#pragma once


namespace Net
{
	class CServer;
}

namespace MafiaServer
{
	class CChat;

	class CChatting
	{
	public:
		struct Init_Data
		{
			uint16_t	iSaveChattingSize{};	/* 보관할 채팅 사이즈  */
			uint16_t	iMaxChatLen{};			/* 채팅 한개의 MaxSize */
		};
	private:
		explicit CChatting();
		CChatting(const CChatting& cCopy) = delete;
		CChatting(CChatting&& cMove) = delete;
	public:
		virtual ~CChatting();
	private:
		HRESULT	Initialize(const CChatting::Init_Data& tData);
		bool	Check_Chat(const std::wstring& wstrText);
	public:
		bool	Chatting(const SYSTEMTIME& tTime, Net::ClientKey iChatterKey, const std::wstring& wstrText);
	public:
		static std::unique_ptr<CChatting>	Create(const Init_Data& tData);
	private:
		std::deque<std::unique_ptr<CChat>> m_dequeChats;
		uint16_t	m_iSaveChattingSize;
		uint16_t	m_iMaxChatLen;
	};
}


