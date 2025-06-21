#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>  // C 표준 정수형 정의

// 메시지 타입 정의
typedef enum {
    MSG_STATE_UPDATE = 1,   // 상태 업데이트
    MSG_ACTION_EVENT = 2,   // 액션 이벤트
    MSG_JOIN = 3,           // 역할 선택용
    MSG_GAME_EVENT = 4,     // 게임 이벤트 (예: 게임 시작, 종료 등)
    MSG_JOIN_ACK = 5,       // 클라에게 엔티티 ID 전달용
    MSG_READY = 6,          // 클라이언트 → 서버: 준비 완료 신호
} MsgType;

// 게임 이벤트 타입 정의
typedef enum {
    GAME_OVER = 1,
    GAME_WIN = 2,
    PLAYER_REJECTED = 3,
    GAME_START = 4,          // 서버 → 클라이언트: 게임 시작 신호
    ENTITY_REMOVE = 5,
    RELOAD_REQUEST = 6       // 서버 → 클라이언트: 리로드 요청
} GameEventType;

// 게임 이벤트 페이로드 구조체
typedef struct {
    GameEventType event_type;  // 1: 게임 오버, 2: 승리 등, 3: 방어자 중복 요청 거부
    uint32_t     entityId;     // 네트워크 바이트 순서
} PayloadGameEvent;

// 메시지 헤더 구조체
typedef struct {
    uint32_t length;    // 페이로드 길이 (네트워크 바이트 순서)
    MsgType  type;      // 메시지 타입
} MsgHeader;

// 역할 선택 페이로드 (클라 -> 서버)
typedef struct {
    int role;           // 1: 방어자(DEFENDER), 2: 공격자(ATTACKER)
} PayloadJoin;

typedef struct {
    uint32_t entityId;     // 서버가 부여한 엔터티 ID
    int role;              // ENTITY_DEFENDER 또는 ENTITY_ATTACKER
} PayloadJoinAck;

// 상태 업데이트 페이로드 구조체
typedef struct {
    uint32_t entityId;  // 엔티티 ID (네트워크 바이트 순서)
    int    x, y;        // 위치 좌표
    int role;           // ENTITY_DEFENDER 또는 ENTITY_ATTACKER
} PayloadStateUpdate;

// 액션 이벤트 페이로드 구조체
typedef struct {
    uint32_t shooterId;     // 발사자 엔티티 ID (네트워크 바이트 순서)
    uint32_t bulletId;      // 총알 엔티티 ID (네트워크 바이트 순서)
    int    dirX, dirY;      // 방향 벡터 (예: -1, 0, 1) 
} PayloadActionEvent;


#endif /* PROTOCOL_H */