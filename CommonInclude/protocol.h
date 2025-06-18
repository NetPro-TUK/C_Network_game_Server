#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>  // C ǥ�� ������ ����

    // �޽��� Ÿ�� ����
    // MSG_STATE_UPDATE: ���� ������Ʈ �޽���
    // MSG_ACTION_EVENT: �׼� �̺�Ʈ �޽���
    typedef enum {
        MSG_STATE_UPDATE = 1,
        MSG_ACTION_EVENT = 2
    } MsgType;

    // �޽��� ��� ����ü
    // length: ���̷ε� ���� (����Ʈ ����)
    // type: �޽��� Ÿ�� (MsgType)
    typedef struct {
        uint32_t length; /* payload ���� */
        MsgType  type;   /* �޽��� Ÿ�� */
    } MsgHeader;

    // ���� ������Ʈ ���̷ε� ����ü
    // entityId: ��ƼƼ ���� ID
    // x, y: ��ġ ��ǥ
    typedef struct {
        uint32_t entityId;
        float    x, y;
    } PayloadStateUpdate;

    // �׼� �̺�Ʈ ���̷ε� ����ü
    // shooterId: �߻��� ��ƼƼ ID
    // bulletId: �Ѿ� ��ƼƼ ID
    // dirX, dirY: �߻� ���� (���� ����)
    typedef struct {
        uint32_t shooterId;
        uint32_t bulletId;
        float    dirX, dirY;
    } PayloadActionEvent;

#endif /* PROTOCOL_H */