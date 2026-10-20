#pragma once
#include "Enum.h"
#include <cstdint>
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

	/* 전적 (나중에 직업별 승수, 연승 등 추가) */
	struct PlayerRecord
	{
		uint32_t	iPlayCount{ 0 };
		uint32_t	iWinCount{ 0 };
	};
	/* 플레이어 한 명 (네트워크로 보내는 공개 정보만, 고정 크기) */
	struct PlayerInfo
	{
		uint64_t		iKey{ 0 };
		wchar_t			wszNickname[Protocol::MAX_NICNAME_BUF]{};
		uint8_t			eState{ 0 };								
	};


	struct REQ_DATA
	{
		DATA_TYPE eDataType{};
	};

	struct PROVIDE_DATA abstract
	{
		DATA_TYPE eDataType{};
	};

	struct PLAYER_INFOS : PROVIDE_DATA
	{
		uint16_t	iCount{ 0 };
		PlayerInfo  arr[MAX_ONLINE_PLAYER]{};
	};

#pragma pack(pop)

}