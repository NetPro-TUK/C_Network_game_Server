#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <stdint.h>  // C 표준 정수형
#include <stddef.h>  // size_t 정의
#include <string.h>  // memcpy, memset
#include <winsock2.h>  // Windows 소켓
typedef int ssize_t;  // ssize_t 정의 (음수 포함)

// send()를 반복 호출하여 전체 버퍼를 전송
static ssize_t send_full(int sockfd, const void* buf, size_t len) {
    const char* ptr = (const char*)buf;
    size_t total = 0;
    while (total < len) {
        ssize_t sent = send(sockfd, ptr + total, len - total, 0);
        if (sent > 0) {
            total += (size_t)sent;
        }
        else {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) {
                // 아직 보낼 수 있는 버퍼가 확보되지 않았으니 잠시 쉬고 재시도
                Sleep(1);
                continue;
            }
            // 연결 종료(0) 또는 그 외 오류
            return sent;
        }
    }
    return (ssize_t)total;
}

// recv()를 반복 호출하여 전체 버퍼를 수신
static ssize_t recv_full(int sockfd, void* buf, size_t len) {
    char* ptr = (char*)buf;
    size_t total = 0;
    while (total < len) {
        ssize_t recvd = recv(sockfd, ptr + total, len - total, 0);
        if (recvd > 0) {
            total += (size_t)recvd;
        }
        else {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) {
                // 아직 데이터가 다 안 왔으니 잠시 쉬고 재시도
                Sleep(1);
                continue;
            }
            // 연결 종료(0) 또는 다른 오류
            return recvd;
        }
    }
    return (ssize_t)total;
}

#endif /* NET_UTILS_H */