#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <winsock2.h>

#define MAX_ENTITY 256

typedef enum {
    ENTITY_DEFENDER,     // �����
    ENTITY_BULLET,     // �Ѿ�
    ENTITY_ATTACKER    // ������
} EntityType;


typedef struct {
	uint32_t entity_id;         // ��ƼƼ ���� ID
	EntityType type;            // ��ƼƼ Ÿ�� (�����, �Ѿ�, ������ ��)
	uint32_t owner_client_id;   // ������ Ŭ���̾�Ʈ ID
	int x, y;                   // ��ġ ��ǥ
	int vx, vy;					// �̵� ���� (��: -1, 0, 1)
    int alive;					// 1: �������, 0: ����
	SOCKET sock; 			    // ��ƼƼ�� ����� ���� (�ʿ��)
} Entity;

extern Entity entityArr[MAX_ENTITY];
extern int entityCount;

// ����Ƽ ���� �Լ�
void    init_entity_system();
Entity* create_entity(EntityType type, uint32_t owner_id, SOCKET sock);
Entity* get_entity_by_id(uint32_t id);
void    update_entity_state(uint32_t id, int x, int y, int vx, int vy);
void    mark_entity_dead(uint32_t id);

extern Entity entityArr[MAX_ENTITY];
extern int entityCount;

#endif
