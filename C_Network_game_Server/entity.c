// ���� ��� ����
#include "entity.h"

#include <string.h>

Entity entityArr[MAX_ENTITY];
int entityCount = 0;
static uint32_t nextEntityId = 1;

void init_entity_system() {
    memset(entityArr, 0, sizeof(entityArr));
    entityCount = 0;
    nextEntityId = 1;
}

// ��ƼƼ ���� �Լ�
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

// ��ƼƼ ID�� ��ƼƼ�� ã�� �Լ�
Entity* get_entity_by_id(uint32_t id) {
    for (int i = 0; i < entityCount; ++i) {
        if (entityArr[i].entity_id == id && entityArr[i].alive)
            return &entityArr[i];
    }
    return NULL;
}

// ��ƼƼ ���� ������Ʈ �Լ�
void update_entity_state(uint32_t id, int x, int y, int vx, int vy) {
    Entity* ent = get_entity_by_id(id);
    if (ent) {
        ent->x = x;
        ent->y = y;
        ent->vx = vx;
        ent->vy = vy;
    }
}

// ��ƼƼ�� ���� ���·� ǥ���ϴ� �Լ�
void mark_entity_dead(uint32_t id) {
    Entity* ent = get_entity_by_id(id);
    if (ent) {
        ent->alive = 0;
    }
}