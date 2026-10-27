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

	/* 룸 제작 의뢰 */
	struct C_TO_S_CREATE_ROOM
	{
		wchar_t wszRoomName[Protocol::MAX_ROOMNAME_BUF]{}; /* 룸 이름 */
	};

	/* 방 자리 한 칸 (iKey 0 = 빈자리) */
	struct ROOM_SEAT
	{
		uint64_t	iKey{ 0 };
		wchar_t		wszNickname[Protocol::MAX_NICNAME_BUF]{};
	};

	/* 방 입장 성공 : 방 정보 + 자리 7칸 (배열 순서 = 자리 번호) */
	struct S_TO_C_ENTER_ROOM
	{
		ROOM_KEY	iRoomKey{};
		uint64_t	iMasterKey{ 0 };
		wchar_t		wszRoomName[Protocol::MAX_ROOMNAME_BUF]{};
		ROOM_SEAT	arrSeats[Protocol::MAX_ROOM_PLAYER]{};
	};

	/* 방 들어가기 요청 */
	struct C_TO_S_ENTER_ROOM
	{
		ROOM_KEY	iRoomKey{};
	};

	/* 방 자리 바뀜 : 방장 + 자리 7칸 (빈자리는 iKey 0) */
	struct S_TO_C_ROOM_MEMBERS
	{
		uint64_t	iMasterKey{ 0 };
		ROOM_SEAT	arrSeats[Protocol::MAX_ROOM_PLAYER]{};
	};

	struct C_TO_S_STARTGAME
	{
		Protocol::ROOM_KEY iRoomKey;
	};

	struct S_TO_C_STARTGAME
	{
		/* 게임 시작 실패 이유 Text로 넣어서 보내줄 예정 */
		STARTGAME_RESULT	eResult{ STARTGAME_RESULT::END};
	};

	/* 로비 접속자 */
	struct LOBBY_PLAYER
	{
		uint64_t		iKey{ 0 };
		wchar_t			wszNickname[Protocol::MAX_NICNAME_BUF]{};
		uint8_t			eState{ 0 };
	};
	
	/* 로비 룸 */
	struct LOBBY_ROOM
	{
		ROOM_KEY iRoomKey{};								// 룸 키 번호
		uint16_t iPlayerNumber{};							// 해당 룸안에 플레이어 숫자
		bool	 isPlaying{false};							// 게임 중인지 아닌지
		wchar_t	 wszRoomName[Protocol::MAX_ROOMNAME_BUF]{};	// 룸 이름
	};


	struct C_TO_S_CHAT
	{
		ROOM_KEY	iRoomKey{ Protocol::NONE_ROOMKEY };
		wchar_t		wszText[MAX_CHAT_BUF]{};	/* 채팅 내용 */
	};

	struct REQ_DATA
	{
		DATA_TYPE eDataType{};
	};

	struct PROVIDE_DATA abstract
	{
		DATA_TYPE eDataType{};
	};

	struct LOBBY_PLAYERS : PROVIDE_DATA
	{
		uint16_t		iCount{ 0 };
		LOBBY_PLAYER	arr[MAX_ONLINE_PLAYER]{};
	};

	struct LOBBY_ROOMS : PROVIDE_DATA
	{
		uint16_t		iCount{ 0 };
		LOBBY_ROOM		arr[MAX_ROOM_PACKET_COUNT]{};
	};

	/* 채팅 로그 보내기 */
	struct CHAT_LOG : PROVIDE_DATA
	{
		SYSTEMTIME	tTime{};					/* 시간 */
		wchar_t		wszName[MAX_NICNAME_BUF];	/* 보낸 사람 닉네임 */
		wchar_t		wszText[MAX_CHAT_BUF]{};	/* 채팅 내용 */
	};

#pragma pack(pop)

}