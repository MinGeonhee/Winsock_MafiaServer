#pragma once
#include <cstdint>
#include <cstddef>

namespace Protocol
{
	/* 닉네임 제한 크기 10글자 */
	constexpr size_t MAX_NICKNAME_LEN  = 10;
	constexpr size_t MAX_NICNAME_BUF   = 16;
	constexpr size_t MAX_ONLINE_PLAYER = 20;     // 한 패킷에 담는 최대 인원
	enum class CHECK_NAME : uint16_t
	{
		OK,
		EMPTY,
		OVER,
		INVALID_CHARACTER,
		END
	};

	/* 플레이어가 지금 어디 있는지 */
	enum class PLAYER_STATE : uint16_t
	{
		LOBBY = 0,
		ROOM,
		GAME,
		END
	};

	enum class PACKET_TYPE : uint16_t
	{
		C_TO_S_LOGIN				 = 0, // 로그인 => 닉네임 이이디 비밀번호
		S_TO_C_LOGIN				 = 1, // 로그인 => 닉네임 이이디 비밀번호
		S_TO_C_LOGIN_FAILED			 = 2, // 로그인 실패 => 사유
		C_TO_S_REQ_DATA				 = 3, // 데이터 요청
		S_TO_C_PROVIDE_DATA			 = 4, // 데이터 받기
		END
	};

	enum class DATA_TYPE : uint16_t
	{
		PLAYER_INFOS = 0,
		END
	};

}
