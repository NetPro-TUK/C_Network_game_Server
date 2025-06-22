#ifndef NET_SERVER_H
#define NET_SERVER_H

#include <winsock2.h>
#include <stdint.h>
#include <stdbool.h>

// 쿨라이언트 접속 최대 수
#define MAX_CLIENT  20


extern bool game_started;			// 게임 시작						
extern bool server_game_over;		// 게임 종료
extern bool defender_ready;			// 방어자 준비 (방어자, 공격자 최소 한명 이상일 때 시작)
extern bool attacker_ready;			// 공격자 준비

extern SOCKET sockArr[MAX_CLIENT];		// 클라 송수신 소켓
extern WSAEVENT eventArr[MAX_CLIENT];	// 각 이벤트 객체
extern int numOfClnt;					// 생성된 클라이언트 개수
extern uint32_t current_score;			// 방어자 게임 점수
extern uint32_t defender_owner_id;		// 방어자 중복 접속 방지

// 서버 초기화 및 포트 바인딩
int init_server_socket(int port);

// 새로운 클라이언트 수락 및 이벤트 배열 등록
void accept_new_client(SOCKET serverSock);

// 새로 접속한 클라이언트 ID 부여
uint32_t generate_client_id();

// recv 메시지 수신 및 타입별 처리
int recv_and_dispatch(int clientIndex);

// 모든 클라이언트에게 데이터 브로드캐스트
void broadcast_all(const void* buf, int len);

// 클라이언트 종료 처리
void remove_client_at(int index);

// 클라이언트 종료 시 방어자 소켓 초기화
void reset_defender_if_match(SOCKET closingSock);

#endif
