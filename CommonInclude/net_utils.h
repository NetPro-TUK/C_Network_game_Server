#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <stdint.h>  // C ǥ�� ������
#include <stddef.h>  // size_t ����
#include <string.h>  // memcpy, memset
#include <winsock2.h>  // Windows ����
typedef int ssize_t;  // ssize_t ���� (���� ����)

    // send()�� �ݺ� ȣ���Ͽ� ��ü ���� ������ ����
    // sockfd: ���� ��ũ����
    // buf: ������ ������ ����
    // len: ������ ����Ʈ ��
    static  ssize_t send_full(int sockfd, const void* buf, size_t len) {
        const char* ptr = (const char*)buf;
        size_t total = 0;
        while (total < len) {
            ssize_t sent = send(sockfd, ptr + total, len - total, 0);
            if (sent <= 0) return sent;  // ���� �Ǵ� ���� ����
            total += (size_t)sent;
        }
        return (ssize_t)total;
    }

    // recv()�� �ݺ� ȣ���Ͽ� ��ü ���� ������ ����
    static  ssize_t recv_full(int sockfd, void* buf, size_t len) {
        char* ptr = (char*)buf;
        size_t total = 0;
        while (total < len) {
            ssize_t recvd = recv(sockfd, ptr + total, len - total, 0);
            if (recvd <= 0) return recvd;  // ���� �Ǵ� ���� ����
            total += (size_t)recvd;
        }
        return (ssize_t)total;
    }

#endif /* NET_UTILS_H */