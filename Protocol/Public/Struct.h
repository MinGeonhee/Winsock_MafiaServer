#pragma once
#include "Enum.h"
#include "string.h"
#include <stdint.h>
namespace Protocol
{
#pragma pack(push,1)
	/* 클라이언트가 서버한테 */
	struct C_TO_S_LOGIN
	{
		wchar_t wszNicName[Protocol::MAX_NICNAME_BUF];
	};
	/* 로그인 성공하였을 경우 */
	struct S_TO_C_LOGIN
	{
		uint64_t iClientKey{0};
	};
	/* 로그인 실패하였을 경우 */
	struct S_TO_C_LOGIN_FAILED
	{
		Protocol::CHECK_NAME eFailedType{};
	};
#pragma pack(pop)
}