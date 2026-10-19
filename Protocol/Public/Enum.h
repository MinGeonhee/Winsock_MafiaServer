#pragma once

namespace Protocol
{
	/* 닉네임 제한 크기 10글자 */
	constexpr size_t MAX_NICKNAME_LEN  = 10;
	constexpr size_t MAX_NICNAME_BUF   = 16;


	enum class PACKET_TYPE
	{
		C_TO_S_LOGIN = 0, // 로그인 => 닉네임 이이디 비밀번호
		S_TO_C_LOGIN = 1, // 로그인 => 닉네임 이이디 비밀번호
		END
	};
}
