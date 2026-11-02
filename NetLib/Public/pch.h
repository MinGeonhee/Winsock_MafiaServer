#ifndef PCH_H
#define PCH_H

/* 서버 헤더 */
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#include <cstdint>

#include <memory>

#include <unordered_map>
#include <map>
#include <vector>
#include <queue>
#include <string>

/* Thread */
#include <mutex>
#include <atomic>
#include <thread>

#include "Net_Header.h"

#endif //PCH_H
