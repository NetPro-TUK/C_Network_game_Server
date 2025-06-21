#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>  // C ǥ�� ������ ����

// �޽��� Ÿ�� ����
typedef enum {
    MSG_STATE_UPDATE = 1,   // ���� ������Ʈ
    MSG_ACTION_EVENT = 2,   // �׼� �̺�Ʈ
    MSG_JOIN = 3,           // ���� ���ÿ�
    MSG_GAME_EVENT = 4,     // ���� �̺�Ʈ (��: ���� ����, ���� ��)
    MSG_JOIN_ACK = 5,       // Ŭ�󿡰� ��ƼƼ ID ���޿�
    MSG_READY = 6,          // Ŭ���̾�Ʈ �� ����: �غ� �Ϸ� ��ȣ
} MsgType;

// ���� �̺�Ʈ Ÿ�� ����
typedef enum {
    GAME_OVER = 1,
    GAME_WIN = 2,
    PLAYER_REJECTED = 3,
    GAME_START = 4,          // ���� �� Ŭ���̾�Ʈ: ���� ���� ��ȣ
    ENTITY_REMOVE = 5,
    RELOAD_REQUEST = 6       // ���� �� Ŭ���̾�Ʈ: ���ε� ��û
} GameEventType;

// ���� �̺�Ʈ ���̷ε� ����ü
typedef struct {
    GameEventType event_type;  // 1: ���� ����, 2: �¸� ��, 3: ����� �ߺ� ��û �ź�
    uint32_t     entityId;     // ��Ʈ��ũ ����Ʈ ����
} PayloadGameEvent;

// �޽��� ��� ����ü
typedef struct {
    uint32_t length;    // ���̷ε� ���� (��Ʈ��ũ ����Ʈ ����)
    MsgType  type;      // �޽��� Ÿ��
} MsgHeader;

// ���� ���� ���̷ε� (Ŭ�� -> ����)
typedef struct {
    int role;           // 1: �����(DEFENDER), 2: ������(ATTACKER)
} PayloadJoin;

typedef struct {
    uint32_t entityId;     // ������ �ο��� ����Ƽ ID
    int role;              // ENTITY_DEFENDER �Ǵ� ENTITY_ATTACKER
} PayloadJoinAck;

// ���� ������Ʈ ���̷ε� ����ü
typedef struct {
    uint32_t entityId;  // ��ƼƼ ID (��Ʈ��ũ ����Ʈ ����)
    int    x, y;        // ��ġ ��ǥ
    int role;           // ENTITY_DEFENDER �Ǵ� ENTITY_ATTACKER
} PayloadStateUpdate;

// �׼� �̺�Ʈ ���̷ε� ����ü
typedef struct {
    uint32_t shooterId;     // �߻��� ��ƼƼ ID (��Ʈ��ũ ����Ʈ ����)
    uint32_t bulletId;      // �Ѿ� ��ƼƼ ID (��Ʈ��ũ ����Ʈ ����)
    int    dirX, dirY;      // ���� ���� (��: -1, 0, 1) 
} PayloadActionEvent;


#endif /* PROTOCOL_H */