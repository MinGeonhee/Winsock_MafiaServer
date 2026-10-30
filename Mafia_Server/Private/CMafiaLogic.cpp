#include "pch.h"
#include "CMafiaLogic.h"
#include "CServerApp.h"
#include "CRoom.h"
#include <random>
#include <algorithm>

namespace MafiaServer
{
	/* 직업 덱 : 앞에서부터 인원 수만큼 잘라 섞어서 나눠 줌 (중요한 직업일수록 앞) */
	static constexpr Protocol::JOB JOB_DECK[] =
	{
		Protocol::JOB::MAFIA, Protocol::JOB::SNOOP, Protocol::JOB::BODYGUARD, Protocol::JOB::PRESIDENT, Protocol::JOB::MEDIUM,	/* 5인 기본 */
		Protocol::JOB::SOLDIER,																									/* 6인 */
		Protocol::JOB::BETRAYER,																								/* 7인 */
	};
	static_assert(std::size(JOB_DECK) == Protocol::MAX_ROOM_PLAYER, "방 최대 인원만큼 직업이 있어야 함");

	CMafiaLogic::CMafiaLogic()
		: m_iRoomKey{Protocol::NONE_ROOMKEY}
		, m_eCurPhase{Protocol::GAME_PHASE::NIGHT}
		, m_iCurStateTime{}
		, m_iCurStateLen{}
		, m_iDay{}
		, m_iAccusedSeat{-1}
		, m_iExecutedSeat{-1}
		, m_arraySeatInfo{}
		, m_eEndGameResult{Protocol::ENDGAME_RESULT::NONE}
		, m_tUseSkillData{}
		, m_iMafiaSeat{}
	{
		m_arraySeatInfo.fill(nullptr);
	}	  


	CMafiaLogic::~CMafiaLogic()
	{
		for (auto* p : m_arraySeatInfo)
			if (p)
				delete p;
	}

	HRESULT CMafiaLogic::Initialize(const CMafiaLogic::Init_Data& tData)
	{
		if (tData.pRoom == nullptr) return E_FAIL;
		m_pRoom = tData.pRoom;

		/* 앉은 자리 */
		std::vector<INT32> vecSeats;
		for (INT32 i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
			if (tData.arrayParticipant[i]) vecSeats.push_back(i);
		if (vecSeats.size() < Protocol::MIN_START_PLAYER) return E_FAIL;

		/* 덱을 인원 수만큼 잘라서 섞기 → 마피아는 항상 들어감 */
		std::vector<Protocol::JOB> vecDeck(std::begin(JOB_DECK), std::begin(JOB_DECK) + vecSeats.size());
		static std::mt19937 s_Random{ std::random_device{}() };
		std::shuffle(vecDeck.begin(), vecDeck.end(), s_Random);

		/* 앉은 자리 순서대로 한 장씩 */
		for (size_t i = 0; i < vecSeats.size(); ++i)
		{
			m_arraySeatInfo[vecSeats[i]] = new Seat_Info;
			m_arraySeatInfo[vecSeats[i]]->eJob = vecDeck[i];
			if (vecDeck[i] == Protocol::JOB::MAFIA) m_iMafiaSeat = vecSeats[i];
		}

		/* 첫 State(밤)부터 시작 */
		m_iDay = 0;
		Start_Phase(Protocol::GAME_PHASE::NIGHT, tData.ullNow);
		return S_OK;
	}

	void CMafiaLogic::Pre_Update(ULONGLONG ullNow)
	{
		/* 이번 State 제한 시간이 지났으면 다음 State로 */
		if (ullNow - m_iCurStateTime >= m_iCurStateLen)
			Change_Phase(ullNow);
	}

	void CMafiaLogic::Update(ULONGLONG ullNow)
	{
		/*  */

	}

	void CMafiaLogic::Late_Update(ULONGLONG ullNow)
	{

	}

	ULONGLONG CMafiaLogic::Get_Remain(ULONGLONG ullNow) const
	{
		const ULONGLONG ullElapsed = ullNow - m_iCurStateTime;
		return ullElapsed >= m_iCurStateLen ? 0 : m_iCurStateLen - ullElapsed;
	}

	bool CMafiaLogic::Can_Chat(int iSeat) const
	{
		if (!Check_SeatRange(iSeat)) return false;
		if (m_arraySeatInfo[iSeat]->isAlive == false) return false;	/* 죽은 사람은 못 침 */

		switch (m_eCurPhase)
		{
		case Protocol::GAME_PHASE::NIGHT:		return Is_NightChatMember(iSeat);	/* 밤 : 마피아 + 접선한 배신자 (받는 사람도 마피아만, CRoom::Chatting) */
		case Protocol::GAME_PHASE::DAY:			return true;
		case Protocol::GAME_PHASE::ALLOCUTION:	return iSeat == m_iAccusedSeat;
		default:								return false;
		}
	}

	Protocol::JOB CMafiaLogic::Get_Job(INT32 iSeat) const
	{
		if (!Check_SeatRange(iSeat)) return Protocol::JOB::UNKNOWN;	/* 빈자리 */
		
		return m_arraySeatInfo[iSeat]->eJob;
	}

	bool CMafiaLogic::Check_SeatRange(INT32 iSeat) const
	{
		if (iSeat < 0) return false;
		if (iSeat >= Protocol::MAX_ROOM_PLAYER) return false;
		if (m_arraySeatInfo[iSeat] == nullptr)	return false;

		return true;
	}

	void CMafiaLogic::Kill(INT32 iSeat, INT32 iKillerSeat)
	{
		/* Seat 범위 체크 */
		if (Check_SeatRange(iSeat) == false) return;
		if (Check_SeatRange(iKillerSeat) == false) return;

		/* 죽인 사람 or 죽을 사람 체크 */
		if (m_arraySeatInfo[iSeat]->isAlive == false)		return;
		if (m_arraySeatInfo[iKillerSeat]->isAlive == false) return;

		/* killer조건 Mafia 등 늘어날 예정 */
		if (!Is_Mafia(iKillerSeat)) return;

		/* 죽음 */
		m_arraySeatInfo[iSeat]->isAlive = false;
	}

	void CMafiaLogic::Arrest(INT32 iSeat)
	{
		/* Seat 범위 체크 */
		if (Check_SeatRange(iSeat) == false) return;
		m_arraySeatInfo[iSeat]->isAlive = false;
	}

	/* Game End 조건 */
	Protocol::ENDGAME_RESULT CMafiaLogic::Judge_EndGame()
	{
		/* 살아 있는 사람만 팀별로 셈 (빈자리 = nullptr) */
		INT32 iMafia{ 0 }, iTeam{ 0 }, iCitizen{ 0 };	/* 마피아 / 마피아 팀(배신자 포함) / 시민 팀 */
		for (INT32 i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
		{
			if (m_arraySeatInfo[i] == nullptr) continue;
			if (m_arraySeatInfo[i]->isAlive == false) continue;

			if (Is_Mafia(i)) ++iMafia;
			Is_MafiaTeam(i) ? ++iTeam : ++iCitizen;
		}

		if (iMafia == 0)		 return Protocol::ENDGAME_RESULT::CITIZEN;	/* 마피아가 잡히면 끝 (배신자가 남아도) */
		if (iTeam >= iCitizen)	 return Protocol::ENDGAME_RESULT::MAFIA;	/* 같아지면 투표로 절대 못 이김 (동점 = 연행 없음) */
		
		return Protocol::ENDGAME_RESULT::NONE;
	}

	INT32 CMafiaLogic::Get_SeatVoteCount(INT32 iSeat) const /* iSeat자리가 투표받은 숫자 */
	{
		if (!Check_SeatRange(iSeat)) return Protocol::NONE_SEAT;
		if (m_arraySeatInfo[iSeat]->isAlive == false) return Protocol::NONE_SEAT;
		
		INT32 iCount{ 0 };
		for (INT32 i = 0; i < Protocol::MAX_ROOM_PLAYER ; ++i)
		{
			if (m_arraySeatInfo[i] == nullptr) continue;;
			if (m_arraySeatInfo[i]->isAlive == false) continue;
			if (m_arraySeatInfo[i]->iVoteSeat == Protocol::NONE_VOTE) continue;
			if (m_arraySeatInfo[i]->iVoteSeat == Protocol::NONE_SEAT) continue;

			/* 해당 자리가 투표한 투표 자리숫자가 Seat와 같다면*/
			if (m_arraySeatInfo[i]->iVoteSeat == iSeat) iCount++;
		}

		return iCount;
	}

	INT32 CMafiaLogic::Get_LastVote(INT32 iSeat)
	{
		if (Check_SeatRange(iSeat) == false)			return Protocol::NONE_SEAT;
		if (m_arraySeatInfo[iSeat] == nullptr)			return Protocol::NONE_SEAT;
		if (m_arraySeatInfo[iSeat]->isAlive == false)	return Protocol::NONE_SEAT; /* 죽어있는 표는 없는 표 */

		return m_arraySeatInfo[iSeat]->iVoteSeat;
	}

	INT32 CMafiaLogic::Get_LastVoteResult()
	{
		uint8_t iYes{};
		uint8_t iNo{};

		for (uint8_t i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
		{
			if (m_arraySeatInfo[i] == nullptr) continue;
			if (m_arraySeatInfo[i]->isAlive == false) continue;
			if (m_arraySeatInfo[i]->iVoteSeat == Protocol::YES) ++iYes;		/* 안 누른 사람(NONE_VOTE)은 집계 제외 */
			else if (m_arraySeatInfo[i]->iVoteSeat == Protocol::NO) ++iNo;
		}

		INT32 iReturn{};
		if (iYes > iNo)
		{
			iReturn = m_iAccusedSeat;
		}
		else iReturn = -1;

		return iReturn;
	}


	bool CMafiaLogic::Vote(INT32 iVoter, INT32 iTarget)
	{
		if (m_eCurPhase != Protocol::GAME_PHASE::VOTE)				return false;
		if (!Check_SeatRange(iVoter))								return false;
		if (m_arraySeatInfo[iVoter]->isAlive == false)				return false;

		/* 무효표가 아닌경우 iTarget의 범위 · 생존 확인 (무효표 = NONE_SEAT 그대로 기록 → 집계에서 빠짐) */
		if (iTarget != Protocol::NONE_SEAT)
		{
			if (!Check_SeatRange(iTarget))							return false;
			if (m_arraySeatInfo[iTarget]->isAlive == false)			return false;
			if (iTarget == iVoter)									return false;
		}

		m_arraySeatInfo[iVoter]->iVoteSeat = iTarget;
		return true;
	}

	bool CMafiaLogic::LastVote(INT32 iVoter, bool isAgree)
	{
		if (m_eCurPhase != Protocol::GAME_PHASE::LAST_VOTE) return false;
		if (!Check_SeatRange(iVoter)) return false;
		if (m_arraySeatInfo[iVoter]->isAlive == false) return false;

		if (m_iAccusedSeat == iVoter) return false;									 /* 자기 자신은 안 됨 */
		if (m_arraySeatInfo[iVoter]->iVoteSeat != Protocol::NONE_VOTE) return false; /* 이미 투표를 진행했다면 패스 */

		/* 서버에 저장되어 있는 최후의 변론인한테 찬반투표 */
		m_arraySeatInfo[iVoter]->iVoteSeat = isAgree ? Protocol::YES : Protocol::NO;

		return true;
	}


	INT32 CMafiaLogic::Get_MostVoteSeat() const
	{
		std::array<INT32, Protocol::MAX_ROOM_PLAYER> arrayCount{};
		arrayCount.fill(0);
		for (INT32 i = 0; i < Protocol::MAX_ROOM_PLAYER ; ++i)
		{
			if (m_arraySeatInfo[i] == nullptr) continue;
			if (m_arraySeatInfo[i]->isAlive == false) continue;
			if (m_arraySeatInfo[i]->iVoteSeat == Protocol::NONE_VOTE) continue;
			if (!Check_SeatRange(m_arraySeatInfo[i]->iVoteSeat)) continue; /* 무효표도 포함해서 처리 */
			arrayCount[m_arraySeatInfo[i]->iVoteSeat]++;
		}

		/* 최다 득표자 자리 번호 찾기 (동점 · 0표 = -1) */
		INT32 iBest{ -1 }, iBestCount{ 0 };
		bool isTie{ false };
		for (INT32 i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
		{
			if (arrayCount[i] > iBestCount)							{ iBest = i; iBestCount = arrayCount[i]; isTie = false; }
			else if (arrayCount[i] == iBestCount && iBestCount > 0)	isTie = true;
		}
		return isTie ? -1 : iBest;
	}

	void CMafiaLogic::Use_Skill(INT32 iUseSeat, const Protocol::JOB_ACTION& tAction)
	{
		if (!Check_SeatRange(iUseSeat)) return;
		if (m_arraySeatInfo[iUseSeat]->isAlive == false) return;

		/* 패킷에 적힌 직업 = 실제 내 직업이어야 함 (조작 방지) */
		Protocol::JOB eJob = m_arraySeatInfo[iUseSeat]->eJob;
		if (tAction.eJob != eJob) return;

		switch (eJob)
		{
			/* 마피아의 경우 상대를 죽인다 */
		case Protocol::JOB::MAFIA:
		{
			const INT32 iTargetSeat = static_cast<const Protocol::MAFIA_ACTION&>(tAction).tTarget.iSeat;
			if (m_eCurPhase != Protocol::GAME_PHASE::NIGHT)		return;
			if (!Check_SeatRange(iTargetSeat))					return;
			if (m_arraySeatInfo[iTargetSeat]->isAlive == false) return;
			if (Is_Mafia(iTargetSeat))							return;	/* 자기 자신 포함 */
			if (m_tUseSkillData.iMafiaKillTarget != Protocol::NONE_SEAT)		return;	/* 이미 능력사용 */

			/* 추후 판정을위한 데이터 */
			m_tUseSkillData.iMafiaKillTarget = iTargetSeat;

			return;
		}
		case Protocol::JOB::PRESIDENT:	/* 대통령 : 밤마다 공지 1번 → 방 전원 */
		{
#pragma region 대통령
			if (m_eCurPhase != Protocol::GAME_PHASE::NIGHT)			return;
			if (m_arraySeatInfo[iUseSeat]->isUsePhaseAbility)		return;	/* 이번 밤 이미 씀 */
			const Protocol::PRESIDENT_ACTION& tPresidentAction = static_cast<const Protocol::PRESIDENT_ACTION&>(tAction);

			Protocol::PRESIDENT_NOTICE tNotice{};
			tNotice.eDataType = Protocol::DATA_TYPE::PRESIDENT_NOTICE;
			wcsncpy_s(tNotice.tNotice.wszText, tPresidentAction.tNotice.wszText, _TRUNCATE);	/* 클라가 끝(\0)을 안 넣어 보내도 잘라서 끝냄 */
			if (tNotice.tNotice.wszText[0] == L'\0')				return;	/* 빈 공지 */

			m_arraySeatInfo[iUseSeat]->isUsePhaseAbility = true;
			m_pRoom->Send_ToAll(Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA, tNotice);
			break;
#pragma endregion
		}
		case Protocol::JOB::SNOOP:	/* 탐정 : 마피아인지만 → 본인에게 */
		{
#pragma region 탐정
			const INT32 iTargetSeat = static_cast<const Protocol::SNOOP_ACTION&>(tAction).tTarget.iSeat;
			if (m_eCurPhase != Protocol::GAME_PHASE::NIGHT)						return;
			if (m_iDay < 1)														return;	/* 첫날 밤(0일차)엔 조사 불가 : 둘째 밤부터 */
			if (!Check_SeatRange(iTargetSeat) || iTargetSeat == iUseSeat)		return;
			if (m_arraySeatInfo[iTargetSeat]->isAlive == false)					return;
			if (m_arraySeatInfo[iUseSeat]->isUsePhaseAbility)					return;	/* 이번 밤 이미 씀 */

			m_arraySeatInfo[iUseSeat]->isUsePhaseAbility = true;

			Protocol::SNOOP_RESULT tResult{};
			tResult.eDataType		= Protocol::DATA_TYPE::SNOOP_RESULT;
			tResult.tTarget.iSeat	= iTargetSeat;
			tResult.isMafia			= Is_Mafia(iTargetSeat);		/* 배신자는 마피아로 안 나옴 */
			m_pRoom->Send_ToSeat(iUseSeat, Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA, tResult);
			return;
#pragma endregion
		}
		case Protocol::JOB::BODYGUARD:	/* 경호원 : 기록만 (판정은 밤 → 낮) */
		{
#pragma region 경호원
			const INT32 iTargetSeat = static_cast<const Protocol::BODYGUARD_ACTION&>(tAction).tTarget.iSeat;
			if (m_eCurPhase != Protocol::GAME_PHASE::NIGHT)			return;
			if (!Check_SeatRange(iTargetSeat))						return;
			if (m_arraySeatInfo[iTargetSeat]->isAlive == false)		return;
			if (m_arraySeatInfo[iUseSeat]->isUsePhaseAbility)		return;	/* 이번 밤 이미 씀 */

			m_arraySeatInfo[iUseSeat]->isUsePhaseAbility = true;

			m_tUseSkillData.iBodyguardTarget = iTargetSeat;
			return;
#pragma endregion
		}
		case Protocol::JOB::MEDIUM:		/* 영매 : 죽은 사람 직업 → 본인에게 */
		{
#pragma region 영매사

			const INT32 iTargetSeat = static_cast<const Protocol::MEDIUM_ACTION&>(tAction).tTarget.iSeat;
			if (m_eCurPhase != Protocol::GAME_PHASE::NIGHT)			return;
			if (!Check_SeatRange(iTargetSeat))						return;
			if (m_arraySeatInfo[iTargetSeat]->isAlive)				return;	/* 죽은 사람만 */
			if (m_arraySeatInfo[iUseSeat]->isUsePhaseAbility)		return;	/* 이번 밤 이미 씀 */

			m_arraySeatInfo[iUseSeat]->isUsePhaseAbility = true;

			Protocol::MEDIUM_RESULT tResult{};
			tResult.eDataType		= Protocol::DATA_TYPE::MEDIUM_RESULT;
			tResult.tTarget.iSeat	= iTargetSeat;
			tResult.eJob			= m_arraySeatInfo[iTargetSeat]->eJob;
			m_pRoom->Send_ToSeat(iUseSeat, Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA, tResult);
			return;
#pragma endregion
		}
		case Protocol::JOB::BETRAYER:	/* 배신자 : 밤마다 1번 (직업 확인은 게임 중 1회 / 접촉 시도는 매일) → 본인에게 */
		{
#pragma region 배신자
			const Protocol::BETRAYER_ACTION& tBetrayerAction = static_cast<const Protocol::BETRAYER_ACTION&>(tAction);
			const INT32 iTargetSeat = tBetrayerAction.tTarget.iSeat;
			Seat_Info* pMe = m_arraySeatInfo[iUseSeat];
			if (m_eCurPhase != Protocol::GAME_PHASE::NIGHT)					return;
			if (pMe->isUsePhaseAbility)										return;	/* 이번 밤 이미 씀 */
			if (!Check_SeatRange(iTargetSeat) || iTargetSeat == iUseSeat)	return;
			if (m_arraySeatInfo[iTargetSeat]->isAlive == false)				return;
			if (!tBetrayerAction.isContact && m_tUseSkillData.isBetRayerFindJob) return;	/* 직업 확인은 1회 */
			if (tBetrayerAction.isContact && m_tUseSkillData.isBetRayerFindMafia) return;	/* 이미 접선함 */

			Protocol::BETRAYER_RESULT tResult{};
			tResult.eDataType = Protocol::DATA_TYPE::BETRAYER_RESULT;
			tResult.tTarget.iSeat = iTargetSeat;
			tResult.isContact = tBetrayerAction.isContact;

			/* 직업확인 */
			if (!tBetrayerAction.isContact)
			{
				m_tUseSkillData.isBetRayerFindJob = true;
				tResult.eJob = m_arraySeatInfo[iTargetSeat]->eJob;

			}
			/* 마피아 컨택트일 경우 */
			else
			{
				pMe->isUsePhaseAbility = true;
				tResult.isSucceeded   = Is_Mafia(iTargetSeat);					/* 접촉 : 대상이 마피아면 성공 */
				tResult.tTarget.iSeat = tBetrayerAction.tTarget.iSeat;			/* 마피아 자리 넣어주기 */

				/* 접선 성공 : 마피아에게도 알림 (tTarget = 배신자 자리) → 이후 밤 채팅 같이 */
				if (tResult.isSucceeded)
				{
					m_tUseSkillData.isBetRayerFindMafia = true;

					Protocol::BETRAYER_RESULT tToMafia = tResult;
					tToMafia.tTarget.iSeat = iUseSeat;
					tToMafia.eJob		   = Protocol::JOB::BETRAYER;
					for (INT32 i = 0; i < Protocol::MAX_ROOM_PLAYER; ++i)
						if (Is_Mafia(i)) m_pRoom->Send_ToSeat(i, Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA, tToMafia);
				}
			}

			m_pRoom->Send_ToSeat(iUseSeat, Protocol::PACKET_TYPE::S_TO_C_PROVIDE_DATA, tResult);
			return;
#pragma endregion
		}
		case Protocol::JOB::SOLDIER:	/* 군인 : 밤 행동 없음 (판정은 밤 → 낮) */
		default:
			return;
		}
	}

	void CMafiaLogic::Change_Phase(ULONGLONG ullNow)
	{
		/* 현재 State 증가 */
		uint16_t iState = static_cast<uint16_t>(m_eCurPhase);
		++iState;
		if (iState >= static_cast<uint16_t>(Protocol::GAME_PHASE::GAME_RESULT)) iState = 0;

		/* 밤-> 낮으로 넘어갈때 쌓인 로직관리 */
		if (m_eCurPhase == Protocol::GAME_PHASE::NIGHT)
		{
			Protocol::S_TO_C_NIGHT_RESULT tNightResult{};
			const INT32 iTarget = m_tUseSkillData.iMafiaKillTarget;

			/* 마피아가 골랐을 때만 판정 : 경호원 → 군인 → 처치 순 */
			if (Check_SeatRange(iTarget))
			{
				if (iTarget == m_tUseSkillData.iBodyguardTarget)
					tNightResult.isBlocked = true;
				else if (Get_Job(iTarget) == Protocol::JOB::SOLDIER && !m_tUseSkillData.isSoldierUsed)
				{
					m_tUseSkillData.isSoldierUsed = true;
					tNightResult.isSoldierSurvived = true;
					tNightResult.iSoldierSeat	   = iTarget;
				}
				else
				{
					Kill(iTarget, m_iMafiaSeat);
					if (m_arraySeatInfo[iTarget]->isAlive == false) tNightResult.iKilledSeat = iTarget;	/* 진짜 죽었을 때만 */
				}
			}
			if (m_pRoom) m_pRoom->Send_ToAll(Protocol::PACKET_TYPE::S_TO_C_NIGHT_RESULT, tNightResult);
			m_isFxNext = tNightResult.isBlocked || tNightResult.isSoldierSurvived || tNightResult.iKilledSeat != Protocol::NONE_SEAT;	/* 조용한 밤은 연출 없음 */

			/* 죽었으면 승패 판정 */
			if (tNightResult.iKilledSeat != Protocol::NONE_SEAT)
			{
				m_eEndGameResult = Judge_EndGame();
				if (m_eEndGameResult != Protocol::ENDGAME_RESULT::NONE)
					iState = static_cast<uint16_t>(Protocol::GAME_PHASE::GAME_RESULT);
			}
		}

		/* 투표 끝 : 집계. 지목된 사람이 없으면 최후 변론 · 찬반 건너뛰고 바로 밤으로 */
		if (m_eCurPhase == Protocol::GAME_PHASE::VOTE)
		{
			m_iAccusedSeat = Get_MostVoteSeat();
			if (m_iAccusedSeat < 0) iState = static_cast<uint16_t>(Protocol::GAME_PHASE::LOADING_NIGHT);
			else if (Get_Job(m_iAccusedSeat) == Protocol::JOB::PRESIDENT)	/* 대통령은 연행할 수 없으니 변론 · 찬반 없이 바로 면책 (Room이 찬반 결과처럼 알림) */
			{
				m_iExecutedSeat		= m_iAccusedSeat;
				m_isPresidentSaved	= true;
				m_isFxNext			= true;
				iState = static_cast<uint16_t>(Protocol::GAME_PHASE::LOADING_NIGHT);
			}
		}

		/* 마지막 투표 찬반 투표 */
		if (m_eCurPhase == Protocol::GAME_PHASE::LAST_VOTE)
		{
			m_iExecutedSeat = Get_LastVoteResult();
			m_isFxNext = (m_iExecutedSeat != -1);		/* 연행(머그샷) · 면책(연단) 연출 */

			/* 대통령은 투표로 연행할 수 없음 : 살려 두고 Room이 "대통령 확정"을 알림 */
			if (m_iExecutedSeat != -1 && Get_Job(m_iExecutedSeat) == Protocol::JOB::PRESIDENT)
				m_isPresidentSaved = true;
			/* -1이 아닌 경우 처형 */
			else if (m_iExecutedSeat != -1)
			{
				/* 죽인 뒤 해당 */
				Arrest(m_iExecutedSeat);
				m_eEndGameResult = Judge_EndGame();		/* Room이 Get_Winner로 읽어 감 */
				const Protocol::ENDGAME_RESULT eResult = m_eEndGameResult;

				/* 게임이 끝났다면 결과 창으로 이동*/
				if (eResult != Protocol::ENDGAME_RESULT::NONE && eResult != Protocol::ENDGAME_RESULT::END)
				{
					iState = static_cast<uint16_t>(Protocol::GAME_PHASE::GAME_RESULT);
				}
			}
		}

		Start_Phase(static_cast<Protocol::GAME_PHASE>(iState), ullNow);
	}

	void CMafiaLogic::Start_Phase(Protocol::GAME_PHASE ePhase, ULONGLONG ullNow)
	{
		m_eCurPhase = ePhase;
		m_iCurStateTime = ullNow;								/* 이 State가 시작된 시각 */
		if (m_eCurPhase == Protocol::GAME_PHASE::LOADING_DAY) ++m_iDay;	/* 동틀 때마다 하루 (아침 로딩 화면부터 새 일차) */
		/* 새 투표 · 새 찬반 : 기록 지우기 */
		if (m_eCurPhase == Protocol::GAME_PHASE::VOTE || m_eCurPhase == Protocol::GAME_PHASE::LAST_VOTE)
			for (Seat_Info* pSeat : m_arraySeatInfo)
				if (pSeat) pSeat->Reset_Vote();

		/* 추후 DB연결해서 사용하고싶긴해 */
		switch (m_eCurPhase)
		{
		case Protocol::GAME_PHASE::NIGHT:
			/* 새 밤 : 능력 대상 초기화 */
			m_tUseSkillData.Reset();
			for (Seat_Info* pSeat : m_arraySeatInfo)
				if (pSeat) pSeat->isUsePhaseAbility = false;
			m_iCurStateLen = 30'000;  /* 30초 */
			break;
		case Protocol::GAME_PHASE::LOADING_DAY:
			m_iCurStateLen = m_isFxNext ? 10'000 : 5'000;	/* 밤 습격 연출이 있으면 10초 (연출 뒤 여유), 없으면 5초 */
			m_isFxNext = false;
			break;
		case Protocol::GAME_PHASE::DAY:
			m_iCurStateLen = 30'000; /* 30초 */
			break;
		case Protocol::GAME_PHASE::VOTE:
			m_iAccusedSeat = -1;
			m_iExecutedSeat = -1;
			m_isPresidentSaved = false;
			m_iCurStateLen = 15'000; /* 15초 */
			break;
		case Protocol::GAME_PHASE::ALLOCUTION:
			m_iCurStateLen = 15'000; /* 15초 */
			break;
		case Protocol::GAME_PHASE::LAST_VOTE:
			m_iCurStateLen = 10'000; /* 10초 */
			break;
		case Protocol::GAME_PHASE::LOADING_NIGHT:
			m_iCurStateLen = m_isFxNext ? 10'000 : 5'000;	/* 연행 · 면책 연출이 있으면 10초, 없으면 5초 */
			m_isFxNext = false;
			break;
		default:
			break;
		}

	}

	std::unique_ptr<CMafiaLogic> CMafiaLogic::Create(const CMafiaLogic::Init_Data& tData)
	{
		std::unique_ptr<CMafiaLogic> pMafiaLogic{new CMafiaLogic()};

		if (FAILED(pMafiaLogic->Initialize(tData)))
			return nullptr;

		return pMafiaLogic;
	}

}

