# Winsock Mafia Server (개발 중)

Winsock TCP와 C++로 제작 중인 7인 CMD 마피아 게임 서버입니다.
먼저 만든 [콘솔 채팅 서버](https://github.com/MinGeonhee/Winsock_ChattingServer)를 확장해, 방(Room) 단위로 게임 상태를 관리하고 규칙 판정은 서버가 담당하는 구조로 설계하고 있습니다.

## 구조

| 모듈 | 역할 |
|---|---|
| `NetLib` | 정적 라이브러리. Winsock 소켓·주소 래퍼, 헤더(타입+길이) 기반 패킷 송수신 |
| `Mafia_Server` | CGameManager(업데이트 루프), CServer(리슨 소켓·Accept), CClient(접속자 1명) |
| `Mafia_Client` | 클라이언트 (예정) |
| `ConsoleUI` | 콘솔 출력 헬퍼 |

## 진행 상황

- [x] NetLib 소켓 래퍼, 헤더 기반 패킷 송수신
- [x] `unique_ptr` 기반 소유권 구조
- [ ] 이벤트 큐 연결, 수신 스레드 / 접속 종료 처리
- [ ] Room / Game 상태 머신 (대기 → 직업 배정 → 낮 → 투표 → 밤 → 판정)
- [ ] 클라이언트
