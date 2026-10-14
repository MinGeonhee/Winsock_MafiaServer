#ifndef PCH_H
#define PCH_H

/* 서버 헤더 */
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#include <cstdint>

#include <memory>

#include <string>

#include "Enum.h"
#include "Struct.h"

#endif //PCH_H
