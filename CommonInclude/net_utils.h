#ifndef NET_UTILS_H
#define NET_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>  // C ǥ�� ������
#include <stddef.h>  // size_t ����
#include <string.h>  // memcpy, memset
#ifdef _WIN32
#include <winsock2.h>  // Windows ����
    typedef int ssize_t;  // ssize_t ����
#else
#include <sys/socket.h>  // socket �Լ�
#include <unistd.h>      // close �Լ�
#include <arpa/inet.h>   // htonl, ntohl
#endif

    // send()�� �ݺ� ȣ���Ͽ� ��ü ���� ������ ����
    // sockfd: ���� ��ũ����
    // buf: ������ ������ ����
    // len: ������ ����Ʈ ��
    static inline ssize_t send_full(int sockfd, const void* buf, size_t len) {
        const uint8_t* ptr = (const uint8_t*)buf;
        size_t total = 0;
        while (total < len) {
            ssize_t sent = send(sockfd, ptr + total, len - total, 0);
            if (sent <= 0) return sent;  // ���� �Ǵ� ���� ����
            total += (size_t)sent;
        }
        return (ssize_t)total;
    }

    // recv()�� �ݺ� ȣ���Ͽ� ��ü ���� ������ ����
    static inline ssize_t recv_full(int sockfd, void* buf, size_t len) {
        uint8_t* ptr = (uint8_t*)buf;
        size_t total = 0;
        while (total < len) {
            ssize_t recvd = recv(sockfd, ptr + total, len - total, 0);
            if (recvd <= 0) return recvd;  // ���� �Ǵ� ���� ����
            total += (size_t)recvd;
        }
        return (ssize_t)total;
    }

    //// float Ÿ���� ��Ʈ��ũ ����Ʈ ����(uint32_t)�� ��ȯ
    //static inline uint32_t htonf(float f) {
    //    uint32_t p;
    //    memcpy(&p, &f, sizeof(p));     // float ��Ʈ ���� ����
    //    return htonl(p);                // ȣ��Ʈ���Ʈ��ũ ����Ʈ ����
    //}

    //// ��Ʈ��ũ ����Ʈ ����(uint32_t)�� float�� ��ȯ
    //static inline float ntohf(uint32_t p) {
    //    p = ntohl(p);                  // ��Ʈ��ũ��ȣ��Ʈ ����Ʈ ����
    //    float f;
    //    memcpy(&f, &p, sizeof(p));    // ��Ʈ ���� ����
    //    return f;
    //}

#ifdef __cplusplus
}
#endif

#endif /* NET_UTILS_H */