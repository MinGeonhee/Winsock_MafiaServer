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

	inline bool Is_VailedNickName( const std::wstring& wstrNicName )
	{
		if (wstrNicName.empty()) return false;
		if (wstrNicName.size() > Protocol::MAX_NICKNAME_LEN) return false;

		for (wchar_t wsz : wstrNicName)
			if (Is_ValidNickChar(wsz))
				return false;
		return true;
	}

}