#pragma once
#include <cstdint>
#include <cstddef>

namespace Protocol
{
	using		ROOM_KEY = uint64_t;
	constexpr	ROOM_KEY NONE_ROOMKEY = 0;

	/* 닉네임 제한 길이 10글자 */
	constexpr size_t MAX_NICKNAME_LEN  = 10;
	constexpr size_t MAX_NICNAME_BUF   = 16;
	constexpr size_t MAX_ONLINE_PLAYER = 20;     // 한 패킷에 담는 최대 인원

	/* 방 이름 제한 길이 20 글자 */
	constexpr size_t   MAX_ROOMNAME_LEN = 20;
	constexpr size_t   MAX_ROOMNAME_BUF = 32;
	constexpr uint16_t MAX_ROOM_PLAYER	= 7;
	constexpr uint16_t MIN_START_PLAYER	= 2;		// 게임 시작 최소 인원 (테스트용 2, 실제는 7로)
	constexpr uint32_t START_COUNTDOWN_MS = 5000;	// 시작 승인 후 실제 게임까지 (서버 판정 · 클라 표시 공용)

	constexpr uint16_t MAX_ROOM_PACKET_COUNT = 10;
	 
	/* 채팅 제한 */
	constexpr uint16_t MAX_CHAT_LEN = 100;
	constexpr uint16_t MAX_CHAT_BUF = 128;

	/* 마피아 직업관련 */
	enum class JOB : uint16_t
	{
		UNKNOWN = 0,    // 모름 (남의 직업을 아직 모를 때)
		CITIZEN,		// 시민
		MAFIA,			// 마피아
		END
	};

	enum class ENDGAME_RESULT : uint8_t
	{
		NONE	= 0,	/* 아직 판가름 나지 않음 */
		MAFIA ,			/* 마피아팀의 승리 */
		CITIZEN,		/* 시민팀의 승리 */
		END,
	};

	enum class GAME_PHASE : uint8_t
	{
		NIGHT = 0,		/* 밤 */
		LOADING_DAY,	/* 아침되기전 잠깐의 시간 */
		DAY,			/* 낮 */
		VOTE,			/* 투표 시간 */
		ALLOCUTION,		/* 최후의 변론 시간 */
		LAST_VOTE,		/* 최종 찬반 투표 */
		LOADING_NIGHT,	/* 밤 되기전 잠깐의 시간 */
		GAME_RESULT,	/* 밤 되기전 잠깐의 시간 */
		END
	};

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
		C_TO_S_REQ_DATA	= 0,				// 데이터 요청
		S_TO_C_PROVIDE_DATA,				// 데이터 받기
		C_TO_S_LOGIN,						// 로그인 => 닉네임 이이디 비밀번호
		S_TO_C_LOGIN,						// 로그인 => 닉네임 이이디 비밀번호
		S_TO_C_LOGIN_FAILED,				// 로그인 실패 => 사유
		C_TO_S_CREATE_ROOM,					// 방 만들기 요청
		C_TO_S_ENTER_ROOM,					// 방 들어가기 요청
		C_TO_S_EXIT_ROOM,					// 방 나가기   요청 (본문 없음)
		S_TO_C_ENTER_ROOM,					// 방 입장 성공 (방을 만든 사람도 이걸 받음)
		S_TO_C_ROOM_MEMBERS,				// 방 자리 바뀜 (누가 들어오거나 나감) → 방에 남은 사람 전원에게
		C_TO_S_CHAT,						// 클라->서버 채팅 보내기
		C_TO_S_STARTGAME,					// 클라->서버 게임시작가능여부 판단 패킷
		S_TO_C_STARTGAME_RESULT,			// 서버->클라 게임시작가능여부 판단 체크 후 유무 패킷
		S_TO_C_STARTGAME,					// 서버->클라 실제 게임 시작 (5초 뒤 서버가 재확인 후, 사람마다 자기 직업 포함)
		S_TO_C_CHANGEPHASE,					// 서버->클라 페이지 교체
		C_TO_S_VOTE,						// 클라->서버 투표 (VOTE 단계, 고른 자리)
		C_TO_S_LAST_VOTE,					// 클라->서버 최수의 투표 (LAST_VOTE 단계)
		S_TO_C_VOTE_RESULT,					// 서버->클라 투표 결과 (처형 대상 자리, 없으면 NONE_SEAT) → 방 전원
		S_TO_C_LASTVOTE_RESULT,				// 클라->서버 찬반 투표 (LAST_VOTE 단계)
		S_TO_C_GAME_END,					// 클라->서버 찬반 투표 (LAST_VOTE 단계)
		END
	};

	enum class DATA_TYPE : uint16_t
	{
		LOBBY_PLAYERS = 0,		// 로비 접속자 목록
		LOBBY_ROOMS,			// 로비 방 목록
		CHAT_LOG,				// 방 채팅 한 줄 (LOBBY_ROOMS와 번호가 겹치면 받는 쪽에서 구분 못 함)
		GAME_VOTE,				// 투표 단일 결과
		GAME_LASTVOTE,			// 찬반 투표 한 단일결과
		END
	};

	enum class STARTGAME_RESULT : uint16_t
	{
		OK = 0,			// 게임시작 성공
		SERVER_ERROR,	// 단순 서버 소켓 송신 에러
		NOT_FULL,		// 인원이 전부 차지 않았을 경우
		CANCELED,		// 카운트다운 중 인원이 빠져 시작 취소 (방 전원)
		END,
	};

}
