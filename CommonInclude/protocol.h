#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>  // C ǥ�� ������ ����

    // �޽��� Ÿ�� ����
    typedef enum {
		MSG_STATE_UPDATE = 1,   // ���� ������Ʈ
		MSG_ACTION_EVENT = 2,   // �׼� �̺�Ʈ
        MSG_JOIN = 3            // ���� ���ÿ�
    } MsgType;

    // �޽��� ��� ����ü
    typedef struct {
		uint32_t length;    // ���̷ε� ���� (��Ʈ��ũ ����Ʈ ����)
		MsgType  type;      // �޽��� Ÿ��
    } MsgHeader;

	// ���� ���� ���̷ε� (Ŭ�� -> ����)
    typedef struct {
        int role;           // 1: �����(PLAYER), 2: ������(ATTACKER)
    } PayloadJoin;

    // ���� ������Ʈ ���̷ε� ����ü
    typedef struct {
		uint32_t entityId;  // ��ƼƼ ID (��Ʈ��ũ ����Ʈ ����)
		int    x, y;        // ��ġ ��ǥ
    } PayloadStateUpdate;

    // �׼� �̺�Ʈ ���̷ε� ����ü
    typedef struct {
		uint32_t shooterId;     // �߻��� ��ƼƼ ID (��Ʈ��ũ ����Ʈ ����)
		uint32_t bulletId;      // �Ѿ� ��ƼƼ ID (��Ʈ��ũ ����Ʈ ����)
		int    dirX, dirY;      // ���� ���� (��: -1, 0, 1) 
    } PayloadActionEvent;

#endif /* PROTOCOL_H */