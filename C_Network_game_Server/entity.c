// 공용 헤더 파일
#include "entity.h"
#include <string.h>

Entity entityArr[MAX_ENTITY];       // 엔티티 배열: 최대 MAX_ENTITY 개수까지 저장
int entityCount = 0;                // 현재 생성된 엔티티 수
static uint32_t nextEntityId = 1;   // 다음 생성될 엔티티에 부여할 고유 ID

// 엔티티 시스템 초기화 함수
void init_entity_system() {
    memset(entityArr, 0, sizeof(entityArr));
    entityCount = 0;
    nextEntityId = 1;
}

// 엔티티 생성 함수
Entity* create_entity(EntityType type, uint32_t owner_id, SOCKET sock) {
    if (entityCount >= MAX_ENTITY) return NULL;

    Entity* ent = &entityArr[entityCount++];
    ent->entity_id = nextEntityId++;
    ent->type = type;
    ent->owner_client_id = owner_id;
    ent->x = ent->y = 0;
    ent->vx = ent->vy = 0;
    ent->alive = 1;
    ent->sock = sock;
    return ent;
}

// ID로 엔티티 검색
Entity* get_entity_by_id(uint32_t id) {
    for (int i = 0; i < entityCount; ++i) {
        if (entityArr[i].entity_id == id && entityArr[i].alive)
            return &entityArr[i];
    }
    return NULL;
}

// 해당 클라이언트 소켓으로 엔티티 검색
Entity* find_entity_by_sock(SOCKET sock) {
    for (int i = 0; i < entityCount; ++i) {
        if (entityArr[i].alive && entityArr[i].sock == sock)
            return &entityArr[i];
    }
    return NULL;
}

// 엔티티 상태 업데이트 함수
void update_entity_state(uint32_t id, int x, int y, int vx, int vy) {
    Entity* ent = get_entity_by_id(id);
    if (ent) {
        ent->x = x;
        ent->y = y;
        ent->vx = vx;
        ent->vy = vy;
    }
}

// 엔티티를 사망 상태로 표시하는 함수
void mark_entity_dead(uint32_t id) {
    Entity* ent = get_entity_by_id(id);
    if (ent) {
        ent->alive = 0;
    }
}