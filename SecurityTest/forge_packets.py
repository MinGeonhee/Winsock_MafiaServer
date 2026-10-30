"""패킷 변조 테스트 : 정상 클라를 거치지 않고 조작한 패킷을 서버에 직접 보냄 (우리 서버 · 로컬 전용)
   서버가 전부 버리고 서버 로그에 "거부 [...] 이유"를 남기는지 확인 (로그는 서버 창을 덤프해서 봄)"""
import socket, struct, time, sys

HOST, PORT = '127.0.0.1', 1234
P = dict(C_LOGIN=2, S_LOGIN=3, CREATE_ROOM=5, ENTER_ROOM=6, S_ENTER_ROOM=8, C_CHAT=10, C_STARTGAME=11, S_STARTGAME=13,
         CHANGEPHASE=14, C_VOTE=15, C_LAST_VOTE=16, C_JOB_ACTION=20)
JOB = ['UNKNOWN', 'MAFIA', 'PRESIDENT', 'SNOOP', 'BODYGUARD', 'MEDIUM', 'SOLDIER', 'BETRAYER']
J = {n: i for i, n in enumerate(JOB)}
NONE_SEAT = -2

def wstr(s, n):
    b = s.encode('utf-16-le')[: (n - 1) * 2]
    return b + b'\0' * (n * 2 - len(b))

class Bot:
    def __init__(self, name):
        self.name = name
        self.s = socket.create_connection((HOST, PORT)); self.s.settimeout(0.05)
        self.buf = b''; self.inbox = []
    def send(self, t, body=b''): self.s.sendall(struct.pack('<HHB', t, len(body), 0) + body)
    def pump(self):
        try:
            d = self.s.recv(65536); self.buf += d
        except (socket.timeout, BlockingIOError): pass
        while len(self.buf) >= 5:
            t, n, net = struct.unpack_from('<HHB', self.buf)
            if len(self.buf) < 5 + n: break
            if not net: self.inbox.append((t, self.buf[5:5 + n]))
            self.buf = self.buf[5 + n:]
    def wait(self, t, sec=10):
        end = time.time() + sec
        while time.time() < end:
            for b in ALL: b.pump()
            for m in self.inbox:
                if m[0] == t: self.inbox.remove(m); return m[1]
        return None

ALL = []
def bot(name):
    b = Bot(name); ALL.append(b)
    b.send(P['C_LOGIN'], wstr(name, 16)); assert b.wait(P['S_LOGIN']) is not None
    return b

def act(b, room, job, target=0, real_len=True, extra=b''):
    body = struct.pack('<QH', room, job) + struct.pack('<i', target)
    if job == J['BETRAYER']: body += b'\0'
    if job == J['PRESIDENT']: body = struct.pack('<QH', room, job) + wstr(extra.decode() if extra else '', 128)
    if not real_len: body = body[:-2]
    b.send(P['C_JOB_ACTION'], body)

def chat(b, room, text): b.send(P['C_CHAT'], struct.pack('<Q', room) + wstr(text, 128))
def vote(b, room, seat): b.send(P['C_VOTE'], struct.pack('<iQ', seat, room))
def last(b, room, yes): b.send(P['C_LAST_VOTE'], struct.pack('<Q?', room, yes))

def step(no, title):
    print(f'[{no:2}] {title}', flush=True); time.sleep(0.8)

# ---- 방 1 (7명) + 방 2 (다른 사람 1명) ----
bots = [bot(f'정상{i + 1}') for i in range(7)]
bots[0].send(P['CREATE_ROOM'], wstr('변조테스트', 32))
room = struct.unpack_from('<Q', bots[0].wait(P['S_ENTER_ROOM']))[0]
for b in bots[1:]:
    b.send(P['ENTER_ROOM'], struct.pack('<Q', room)); assert b.wait(P['S_ENTER_ROOM']) is not None
other = bot('남의방'); other.send(P['CREATE_ROOM'], wstr('다른방', 32))
other_room = struct.unpack_from('<Q', other.wait(P['S_ENTER_ROOM']))[0]

step(1, '게임 시작 전에 능력 패킷 → 게임 중이 아님')
act(bots[1], room, J['MAFIA'], 2)

bots[0].send(P['C_STARTGAME'], struct.pack('<Q', room))
jobs = {}
for i, b in enumerate(bots):
    body = b.wait(P['S_STARTGAME'], 10)
    jobs[JOB[struct.unpack_from('<HH', body)[1]]] = (b, i)
print('직업 :', {k: v[1] + 1 for k, v in jobs.items()}, '(입장 순서 번호, 실제 자리는 섞임)', flush=True)
mafia = jobs['MAFIA'][0]; snoop = jobs['SNOOP'][0]; medium = jobs['MEDIUM'][0]; soldier = jobs['SOLDIER'][0]; citizen = jobs['PRESIDENT'][0]

step(2, '시민(대통령)이 마피아인 척 처치 패킷 → 직업 불일치')
act(citizen, room, J['MAFIA'], 0)
step(3, '패킷 길이 조작 (마피아 구조체에서 2바이트 뺌) → 길이 불일치')
act(mafia, room, J['MAFIA'], 0, real_len=False)
step(4, '없는 직업 번호 99 → 길이 불일치 (모르는 직업)')
mafia.send(P['C_JOB_ACTION'], struct.pack('<QHi', room, 99, 0))
step(5, '머리(10바이트)보다 짧은 패킷 → 머리보다 짧음')
mafia.send(P['C_JOB_ACTION'], b'\x01\x02\x03')
step(6, '탐정이 첫날 밤 조사 → 첫날 밤 조사 불가')
act(snoop, room, J['SNOOP'], 0)
step(7, '영매가 산 사람 직업 보기 → 영매는 죽은 사람만')
act(medium, room, J['MEDIUM'], 0)
step(8, '군인이 능력 패킷 → 능력이 없는 직업 (군인 구조체가 없어 길이 불일치로 먼저 걸림)')
soldier.send(P['C_JOB_ACTION'], struct.pack('<QHi', room, J['SOLDIER'], 0))
step(9, '범위 밖 대상 (자리 99) → 잘못된 대상')
act(mafia, room, J['MAFIA'], 99)
step(10, '마피아 정상 처치 1번 + 같은 밤 두 번째 → 이번 밤 이미 사용')
for t in range(7):
    act(mafia, room, J['MAFIA'], t)		# 마피아 자기 자리는 "마피아끼리 처치 불가", 첫 정상 대상 1번만 받아들여짐
step(11, '밤에 시민이 채팅 → 지금 단계엔 채팅 불가')
chat(citizen, room, '밤인데 채팅')
step(12, '밤에 투표 / 찬반 → 투표 거부 · 찬반 거부')
vote(citizen, room, 0); last(citizen, room, True)
step(13, '남의 방 번호로 채팅 (방2 사람이 방1 키로) → 방에 없는 사람의 채팅')
chat(other, room, '남의 방에 끼어들기')
step(14, '없는 방 번호로 투표 → 없는 방')
vote(citizen, 987654, 0)
time.sleep(1.0)
time.sleep(3)
print('끝', flush=True)
