#pragma once
#include <string>
#include "Enum.h"

namespace Protocol
{
	/* ±ÛÀÚ ±æÀÌ Á¦ÇÑ ÁÖ±â */
	inline bool Is_ValidNickChar( wchar_t wsz )
	{
		return (wsz >= L'°¡' && wsz  <= L'ÆR')
			|| (wsz >= L'a' && wsz <= L'z')
			|| (wsz >= L'A' && wsz <= L'Z')
			|| (wsz >= L'0' && wsz <= L'9')
			|| wsz == L'_';
	}

	inline  CHECK_NAME Is_VailedNickName( const std::wstring& wstrNicName )
	{
		if (wstrNicName.empty()) return CHECK_NAME::EMPTY;
		if (wstrNicName.size() > Protocol::MAX_NICKNAME_LEN) return CHECK_NAME::OVER;

		for (wchar_t wsz : wstrNicName)
			if (Is_ValidNickChar(wsz) == false)
				return CHECK_NAME::INVALID_CHARACTER;

		return CHECK_NAME::OK;
	}

}