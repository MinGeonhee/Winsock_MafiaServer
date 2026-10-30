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

	/* 게임 페이즈 변경 패킷 */
	struct S_TO_C_CHANGEPHASE
	{
		Protocol::GAME_PHASE	eCurPhase{Protocol::GAME_PHASE::END};	/* 현재 Phase */
		uint16_t				iDay{0};								/* 몇일째 인지 */
		uint64_t				iRemainMs{ 0 };							/* 현재 Phase 제한시간 */
		uint8_t					iAliveMask{ 0 };						/* 비트 i = i번 자리 생존 (화면용 : 죽은 의자 회색 · 투표 후보) */
	};
	
	/* ---------------- 직업 능력 (C_TO_S_JOB_ACTION) ----------------
	   머리(JOB_ACTION)의 eJob을 보고 직업별 구조체로 읽음. 내 자리는 안 보냄 (서버가 보낸 사람 키로 찾음)
	   서버는 eJob == 그 자리의 실제 직업인지 다시 확인 */

	/* 내용물 : 단순 구조체 */
	struct TARGET
	{
		INT32		iSeat{ NONE_SEAT };			/* 대상 자리 */
	};

	struct NOTICE
	{
		wchar_t		wszText[MAX_CHAT_BUF]{};	/* 공지 내용 */
	};

	/* 공통 머리 */
	struct JOB_ACTION abstract
	{
		ROOM_KEY	iRoomKey{ NONE_ROOMKEY };
		JOB			eJob{ JOB::UNKNOWN };		/* 어떤 직업 구조체인지 */
	};

	/* 직업별 */
	struct MAFIA_ACTION : JOB_ACTION			/* 밤 : 처치할 사람 */
	{
		TARGET		tTarget{};
	};

	struct SNOOP_ACTION : JOB_ACTION			/* 밤 : 마피아인지 조사할 사람 */
	{
		TARGET		tTarget{};
	};

	struct BODYGUARD_ACTION : JOB_ACTION		/* 밤 : 지킬 사람 */
	{
		TARGET		tTarget{};
	};

	struct MEDIUM_ACTION : JOB_ACTION			/* 밤 : 직업을 볼 죽은 사람 */
	{
		TARGET		tTarget{};
	};

	struct BETRAYER_ACTION : JOB_ACTION			/* 밤 : 직업 확인(1회) 또는 마피아 접촉 시도 */
	{
		TARGET		tTarget{};
		bool		isContact{ false };			/* false = 직업 확인 / true = 접촉 시도 */
	};

	struct PRESIDENT_ACTION : JOB_ACTION		/* 밤 : 아침에 실릴 공지 */
	{
		NOTICE		tNotice{};
	};

	/* 서버 → 클라 : 밤 끝, 방 전원에게. 밤사이 죽은 자리 (NONE_SEAT = 아무도 안 죽음) */
	struct S_TO_C_NIGHT_RESULT
	{
		bool		isBlocked{ false };			/* 경호원이 막음 */
		bool		isSoldierSurvived{ false };	/* 군인이 1회 버팀 */
		INT32		iSoldierSeat{ NONE_SEAT };	/* 버틴 군인 자리 (방 전원 공개) */
		INT32		iKilledSeat{ NONE_SEAT };
	};


	/* 클라 → 서버 : VOTE 단계에 고른 자리 (0 ~ MAX_ROOM_PLAYER-1, NONE_SEAT = 무효표). 클라는 Enter로 확정할 때 한 번만 보냄 */
	struct C_TO_S_VOTE
	{
		INT32				iVoteSeat{ NONE_SEAT }; /* 투표한 자리숫자 */
		Protocol::ROOM_KEY	iRoomKey{};			/* 내 방번호 */
	};

	/* 서버 → 클라 : VOTE 단계 끝, 방 전원에게. 처형 대상 자리 (NONE_SEAT = 없음 → 바로 밤으로) */
	struct S_TO_C_VOTE_RESULT
	{
		INT32		iSeat{ NONE_SEAT };
	};

	/* 서버 → 클라 : Last Vote 단계 끝 방 전원에게 처형될 대상 자리 와 최종 투표결과 알려주기 */
	struct S_TO_C_LASTVOTE_RESULT
	{
		INT32		iSeat{ NONE_SEAT };
		bool		isPresident{ false };		/* 찬성이 많았지만 대통령이라 연행되지 않음 (iSeat = 그 대통령 자리) */
	};

	/* 클라 → 서버 : LAST_VOTE 단계 찬반 (찬성 = 처형) */
	struct C_TO_S_LAST_VOTE
	{
		Protocol::ROOM_KEY		iRoomKey{Protocol::NONE_ROOMKEY};
		bool					isAgree{ false };
	};


	struct S_TO_C_GAME_END
	{
		ENDGAME_RESULT  eWinner{ ENDGAME_RESULT::NONE };
		JOB             arrJobs[MAX_ROOM_PLAYER]{};     /* 자리별 직업 공개 (빈자리 = UNKNOWN) */
		uint8_t         iAliveMask{ 0 };                /* 마지막 생존 상태 (결과 창에 ☠ 표시용) */
	};

	/* 방 자리 바뀜 : 방장 + 자리 7칸 (빈자리는 iKey 0) */
	struct S_TO_C_ROOM_MEMBERS
	{
		uint64_t	iMasterKey{ 0 };
		ROOM_SEAT	arrSeats[Protocol::MAX_ROOM_PLAYER]{};
	};

	/* 방장 → 서버 : 게임 시작 가능한지 확인 요청 */
	struct C_TO_S_STARTGAME
	{
		Protocol::ROOM_KEY iRoomKey;
	};

	/* 서버 → 클라 : 시작 확인 결과
	   OK       : 방 전원, 클라는 START_COUNTDOWN_MS 카운트다운 표시 (실제 시작은 서버가 S_TO_C_GAME_START로 알림)
	   NOT_FULL : 방장에게만
	   CANCELED : 방 전원, 카운트다운 중 인원이 빠져 시작 취소 */
	struct S_TO_C_STARTGAME_RESULT
	{
		Protocol::STARTGAME_RESULT	eResult{ STARTGAME_RESULT::END};
	};

	/* 서버 → 클라 : 실제 게임 시작 (카운트다운 끝에 서버가 한 번 더 확인한 뒤 먼저 보냄)
	   사람마다 따로 보냄 : eJob은 받는 사람 자기 직업만 */
	struct S_TO_C_STARTGAME
	{
		Protocol::STARTGAME_RESULT	eResult{ STARTGAME_RESULT::END };

		Protocol::JOB				eJob{ JOB::UNKNOWN };
		Protocol::GAME_PHASE		ePhase{ GAME_PHASE::END };	/* 첫 단계 */
		uint16_t					iDay{ 0 };
		uint32_t					iRemainMs{ 0 };				/* 첫 단계 남은 시간 (클라는 자기 시계로 카운트다운 표시) */
		uint64_t					arrSeatKeys[MAX_ROOM_PLAYER]{};	/* 게임 시작 때 섞인 자리 (인덱스 = 자리, 0 = 빈자리) */
		JOB							arrAllJobs[MAX_ROOM_PLAYER]{};	/* [취약 예시] 전원 직업 : 결과 창용이라며 미리 보냄 (화면엔 안 보여도 클라 메모리에 있음) */
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

	struct GAME_LASTVOTE : PROVIDE_DATA
	{
		bool			is{false};
	};

	struct GAME_VOTE : PROVIDE_DATA
	{
		/* 투표된 자리 */
		INT32			iVoteSeat{};
		INT32			iCount{};
	};

	/* ---------------- 직업 능력 결과 (S_TO_C_PROVIDE_DATA, 능력 쓴 본인에게만) ---------------- */
	struct SNOOP_RESULT : PROVIDE_DATA
	{
		TARGET		tTarget{};
		bool		isMafia{ false };
	};

	struct MEDIUM_RESULT : PROVIDE_DATA
	{
		TARGET		tTarget{};
		JOB			eJob{ JOB::UNKNOWN };
	};

	struct BETRAYER_RESULT : PROVIDE_DATA
	{
		TARGET		tTarget{};
		bool		isContact{ false };			/* 접촉 시도였는지 */
		bool		isSucceeded{ false };		/* 접촉 : 대상이 마피아였는지 */
		JOB			eJob{ JOB::UNKNOWN };		/* 직업 확인 : 대상 직업 */
	};

	/* 아침 : 대통령 공지 (방 전원) */
	struct PRESIDENT_NOTICE : PROVIDE_DATA
	{
		NOTICE		tNotice{};
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