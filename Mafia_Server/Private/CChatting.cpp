#include "pch.h"
#include "CChatting.h"
#include "CChat.h"

namespace MafiaServer
{
	CChatting::CChatting()
		: m_iMaxChatLen{}, m_iSaveChattingSize{}
	{
	}


	CChatting::~CChatting()
	{
	}

	HRESULT	CChatting::Initialize(const Init_Data& tData)
	{
		m_iMaxChatLen = tData.iMaxChatLen;
		m_iSaveChattingSize = tData.iSaveChattingSize;

		return S_OK;
	}

	bool CChatting::Check_Chat(const std::wstring& wstrText)
	{
		/* 채팅 Check */
		if (wstrText.empty())					return false;
		if (wstrText.size() > m_iMaxChatLen)	return false;

		return true;
	}

	bool CChatting::Chatting(const SYSTEMTIME& tTime, Net::ClientKey iChatterKey, const std::wstring& wstrText)
	{
		/* 채팅 유효성 검사 */
		if (Check_Chat(wstrText) == false) return false;

		CChat::Init_Data tData{ tTime, iChatterKey, wstrText };
		std::unique_ptr<CChat> pNewChat{CChat::Create(tData)};
		if (pNewChat == nullptr) return false;
		m_dequeChats.push_back(std::move(pNewChat));

		if (m_dequeChats.size() >= m_iSaveChattingSize) 
			m_dequeChats.pop_front();

		return true;
	}

	std::unique_ptr<CChatting> CChatting::Create(const Init_Data& tData)
	{
		std::unique_ptr<CChatting> pChatting{new CChatting()};

		if (FAILED(pChatting->Initialize(tData))) return nullptr;


		return pChatting;
	}
}
