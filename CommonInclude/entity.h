#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <winsock2.h>

#define MAX_ENTITY 256

typedef enum {
	ENTITY_ATTACKER = 0,	// 공격자
    ENTITY_DEFENDER = 1,    // 방어자
    ENTITY_BULLET	= 2,    // 총알
} EntityType;


typedef struct {
	uint32_t entity_id;         // 엔티티 고유 ID
	EntityType type;            // 엔티티 타입 (방어자, 총알, 공격자 등)
	uint32_t owner_client_id;   // 소유자 클라이언트 ID
	int x, y;                   // 위치 좌표
	int vx, vy;					// 이동 방향 (예: -1, 0, 1)
    int alive;					// 1: 살아있음, 0: 죽음
	SOCKET sock; 			    // 엔티티가 연결된 소켓 (필요시)
} Entity;

extern Entity entityArr[MAX_ENTITY];	// 엔티티 배열
extern int entityCount;					// 현재 생성된 엔티티 개수

// 엔티티 시스템 초기화: 배열 클리어 및 카운터 리셋
void init_entity_system(void);

// 엔티티 생성: 타입, 소유자 ID, 소켓 지정 후 Entity 포인터 반환
Entity* create_entity(EntityType type, uint32_t owner_id, SOCKET sock);

// ID로 엔티티 검색 (활성 상태만)
Entity* get_entity_by_id(uint32_t id);

// 소켓으로 엔티티 검색
Entity* find_entity_by_sock(SOCKET sock);

// 엔티티 상태 업데이트: 위치 및 속도 갱신
void update_entity_state(uint32_t id, int x, int y, int vx, int vy);

// 엔티티를 죽음 상태로 표시
void mark_entity_dead(uint32_t id);

#endif
