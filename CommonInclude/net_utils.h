#ifndef NET_UTILS_H
#define NET_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>  // C 표준 정수형
#include <stddef.h>  // size_t 정의
#include <string.h>  // memcpy, memset
#ifdef _WIN32
#include <winsock2.h>  // Windows 소켓
    typedef int ssize_t;  // ssize_t 정의
#else
#include <sys/socket.h>  // socket 함수
#include <unistd.h>      // close 함수
#include <arpa/inet.h>   // htonl, ntohl
#endif

    // send()를 반복 호출하여 전체 버퍼 전송을 보장
    // sockfd: 소켓 디스크립터
    // buf: 전송할 데이터 버퍼
    // len: 전송할 바이트 수
    static inline ssize_t send_full(int sockfd, const void* buf, size_t len) {
        const uint8_t* ptr = (const uint8_t*)buf;
        size_t total = 0;
        while (total < len) {
            ssize_t sent = send(sockfd, ptr + total, len - total, 0);
            if (sent <= 0) return sent;  // 오류 또는 연결 종료
            total += (size_t)sent;
        }
        return (ssize_t)total;
    }

    // recv()를 반복 호출하여 전체 버퍼 수신을 보장
    static inline ssize_t recv_full(int sockfd, void* buf, size_t len) {
        uint8_t* ptr = (uint8_t*)buf;
        size_t total = 0;
        while (total < len) {
            ssize_t recvd = recv(sockfd, ptr + total, len - total, 0);
            if (recvd <= 0) return recvd;  // 오류 또는 연결 종료
            total += (size_t)recvd;
        }
        return (ssize_t)total;
    }

    //// float 타입을 네트워크 바이트 오더(uint32_t)로 변환
    //static inline uint32_t htonf(float f) {
    //    uint32_t p;
    //    memcpy(&p, &f, sizeof(p));     // float 비트 패턴 복사
    //    return htonl(p);                // 호스트→네트워크 바이트 오더
    //}

    //// 네트워크 바이트 오더(uint32_t)를 float로 변환
    //static inline float ntohf(uint32_t p) {
    //    p = ntohl(p);                  // 네트워크→호스트 바이트 오더
    //    float f;
    //    memcpy(&f, &p, sizeof(p));    // 비트 패턴 복사
    //    return f;
    //}

#ifdef __cplusplus
}
#endif

#endif /* NET_UTILS_H */