#include "pch.h"
#include "CChat.h"
#include "CServerApp.h"


namespace MafiaServer
{

	CChat::CChat()
		: m_tTime{}, m_iChatterKey{ Net::NONE_CLIENTKEY }, m_wstrText{}
	{

	}

	CChat::~CChat()
	{
		m_wstrText.clear();
	}

	HRESULT CChat::Initialize(const CChat::Init_Data& tData)
	{
		
		m_tTime			= tData.tTime;
		m_iChatterKey	= tData.iChatterKey;
		m_wstrText		= tData.wstrText;

		return S_OK;
	}

	std::unique_ptr<CChat> CChat::Create(const CChat::Init_Data& tData)
	{
		std::unique_ptr<CChat> pChat{ new CChat() };

		if (FAILED(pChat->Initialize(tData))) return nullptr;

		return pChat;
	}

}