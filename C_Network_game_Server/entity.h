#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <winsock2.h>

#define MAX_ENTITY 256

typedef enum {
    ENTITY_DEFENDER,     // 방어자
    ENTITY_BULLET,     // 총알
    ENTITY_ATTACKER    // 공격자
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

extern Entity entityArr[MAX_ENTITY];
extern int entityCount;

// 엔터티 관련 함수
void    init_entity_system();
Entity* create_entity(EntityType type, uint32_t owner_id, SOCKET sock);
Entity* get_entity_by_id(uint32_t id);
void    update_entity_state(uint32_t id, int x, int y, int vx, int vy);
void    mark_entity_dead(uint32_t id);

extern Entity entityArr[MAX_ENTITY];
extern int entityCount;

#endif
