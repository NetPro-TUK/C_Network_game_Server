#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>  // C 표준 정수형 정의

    // 메시지 타입 정의
    // MSG_STATE_UPDATE: 상태 업데이트 메시지
    // MSG_ACTION_EVENT: 액션 이벤트 메시지
    typedef enum {
        MSG_STATE_UPDATE = 1,
        MSG_ACTION_EVENT = 2
    } MsgType;

    // 메시지 헤더 구조체
    // length: 페이로드 길이 (바이트 단위)
    // type: 메시지 타입 (MsgType)
    typedef struct {
        uint32_t length; /* payload 길이 */
        MsgType  type;   /* 메시지 타입 */
    } MsgHeader;

    // 상태 업데이트 페이로드 구조체
    // entityId: 엔티티 고유 ID
    // x, y: 위치 좌표
    typedef struct {
        uint32_t entityId;
        float    x, y;
    } PayloadStateUpdate;

    // 액션 이벤트 페이로드 구조체
    // shooterId: 발사자 엔티티 ID
    // bulletId: 총알 엔티티 ID
    // dirX, dirY: 발사 방향 (단위 벡터)
    typedef struct {
        uint32_t shooterId;
        uint32_t bulletId;
        float    dirX, dirY;
    } PayloadActionEvent;

#endif /* PROTOCOL_H */