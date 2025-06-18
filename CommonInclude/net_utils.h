#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <stdint.h>  // C 표준 정수형
#include <stddef.h>  // size_t 정의
#include <string.h>  // memcpy, memset
#include <winsock2.h>  // Windows 소켓
typedef int ssize_t;  // ssize_t 정의 (음수 포함)

    // send()를 반복 호출하여 전체 버퍼 전송을 보장
    // sockfd: 소켓 디스크립터
    // buf: 전송할 데이터 버퍼
    // len: 전송할 바이트 수
    static  ssize_t send_full(int sockfd, const void* buf, size_t len) {
        const char* ptr = (const char*)buf;
        size_t total = 0;
        while (total < len) {
            ssize_t sent = send(sockfd, ptr + total, len - total, 0);
            if (sent <= 0) return sent;  // 오류 또는 연결 종료
            total += (size_t)sent;
        }
        return (ssize_t)total;
    }

    // recv()를 반복 호출하여 전체 버퍼 수신을 보장
    static  ssize_t recv_full(int sockfd, void* buf, size_t len) {
        char* ptr = (char*)buf;
        size_t total = 0;
        while (total < len) {
            ssize_t recvd = recv(sockfd, ptr + total, len - total, 0);
            if (recvd <= 0) return recvd;  // 오류 또는 연결 종료
            total += (size_t)recvd;
        }
        return (ssize_t)total;
    }

#endif /* NET_UTILS_H */