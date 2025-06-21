#ifndef NET_SERVER_H
#define NET_SERVER_H

#include <winsock2.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_CLIENT  64

extern SOCKET sockArr[MAX_CLIENT];
extern WSAEVENT eventArr[MAX_CLIENT];
extern int numOfClnt;	// 생성된 클라이언트 개수
bool game_started;
bool server_game_over;

// 서버 초기화 및 포트 바인딩
int		init_server_socket(int port);

// 새로운 클라이언트 수락 및 이벤트 배열 등록
void	accept_new_client(SOCKET serverSock);

// 클라이언트로부터 메시지 수신 및 타입별 처리
int		recv_and_dispatch(int clientIndex);

// 특정 클라이언트에게 데이터 전송
void	send_to_client(SOCKET sock, const void* buf, int len);

// 모든 클라이언트에게 데이터 브로드캐스트
void	broadcast_all(const void* buf, int len);

// 클라이언트 종료 처리
void remove_client_at(int index);

// 클라이언트 종료 시 방어자 소켓 초기화
void reset_defender_if_match(SOCKET closingSock);

// 클라이언트 ID 생성 함수
uint32_t generate_client_id();

#endif
