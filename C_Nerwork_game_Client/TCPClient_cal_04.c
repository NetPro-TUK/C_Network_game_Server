#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#include "protocol.h"
#include "net_utils.h"
#include "log.h"

void ErrorHandling(char *message);
#define MAX_PACKET_SIZE  120

int main(void)
{
	WSADATA wsaData;
	SOCKET hSocket;
	SOCKADDR_IN servAdr;
	
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!"); 

	hSocket = socket(PF_INET, SOCK_STREAM, 0);   
	if(hSocket == INVALID_SOCKET)
		ErrorHandling("socket() error");
	
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servAdr.sin_port = htons(9000);
	 
	// TCP 연결 요청...
	int ret;
	ret = connect(hSocket, (SOCKADDR*)&servAdr, sizeof(servAdr));
	if (ret == SOCKET_ERROR) {
		printf("<ERROR> Client. connect() 실행 오류.\n");
		closesocket(hSocket);
		printf("Client> close socket...\n");
		WSACleanup();
		return 0;
	}
	else {
		printf("Client> connection established...\n");
	}
	
	// 1. send/recv 를 통해 계산 입력받고 서버로 전달하고 결과 수령...
	char msg[MAX_PACKET_SIZE];
	int opndCnt, rcvSum, rcvTotal, result, flag;
	flag = 1;
	while (flag)
	{
		// 1. 사용자 연산 자료 입력....(피연산자 수, 피연산자, 연산자)		
		printf("Client: 피연산자 수 입력: ");
		scanf("%d", &opndCnt);
		msg[0] = (char)opndCnt;
		for (int i = 0; i < opndCnt; i++) {
			printf("- %d 번째 피연산자 입력: ", i+1);
			scanf("%d", &msg[1+i*sizeof(int)]);
		}
		rewind(stdin);
		printf("> 연산자를 입력하세요: ");
		scanf("%c", &msg[1+opndCnt*sizeof(int)]);
		send(hSocket, msg, 2+opndCnt*sizeof(int), 0);

		// 2. 결과 수신 및 출력		
		rcvSum = 0; // 수신 누적치. 
		rcvTotal = sizeof(result); // 수신 목표치.
		while(rcvSum < rcvTotal) {
			ret = recv(hSocket, &msg[rcvSum], rcvTotal-rcvSum, 0);
			if (ret <= 0) {
				printf("<ERROR> recv 오류.\n");
				flag = 0;
				break;
			}
			else {
				rcvSum = rcvSum + ret;
				printf("Client> recv %d bytes. sum=%d, total=%d\n", ret, rcvSum, rcvTotal);
			}
		}
		if (flag == 1) {
			result = *((int*)msg);
			printf("Client> 서버로부터 수신된 결과 = %d\n", result);
		}
	}

	closesocket(hSocket);
	printf("Client> close socket...\n");
	WSACleanup();
	return 0;
}

void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}