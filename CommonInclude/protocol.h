#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>  // C 표준 정수형 정의

// 메시지 타입 정의
typedef enum {
    MSG_STATE_UPDATE    = 1,  // 클라이언트↔서버: 엔티티 위치/상태 업데이트
    MSG_SHOOTING_EVENT  = 2,  // 클라이언트→서버: 발사(슈팅) 이벤트
    MSG_JOIN            = 3,  // 클라이언트→서버: 역할(Defender/Attacker) 선택 요청
    MSG_GAME_EVENT      = 4,  // 클라이언트↔서버: 게임 이벤트 (시작, 종료, 점수 등)
    MSG_JOIN_ACK        = 5,  // 서버→클라이언트: JOIN 요청에 대한 응답 (엔티티 ID 전달)
    MSG_READY           = 6   // 클라이언트→서버: 준비 완료 신호
} MsgType;

// 게임 이벤트 세부 타입 정의 (PayloadGameEvent.event_type 필드)
typedef enum {
    GAME_OVER       = 1,  // 서버→클라이언트: 게임 오버 알림
    GAME_WIN        = 2,  // 사용되지 않음 
    PLAYER_REJECTED = 3,  // 서버→클라이언트: 방어자 중복 요청 거부
    GAME_START      = 4,  // 서버→클라이언트: 게임 시작 알림
    ENTITY_REMOVE   = 5,  // 서버→클라이언트: 특정 엔티티(총알/플레이어) 제거
    RESPAWN_REQUEST = 6,  // 클라이언트→서버: 공격자 리스폰 요청
    OUT_OF_AMMO     = 7,  // 서버→클라이언트: 탄약 부족 알림 (방어자 전용)
    RELOAD_REQUEST  = 8,  // 클라이언트→서버: 재장전 요청 (방어자 전용)
    RELOAD_COMPLETE = 9,  // 서버→클라이언트: 재장전 완료 알림
    SCORE_UPDATE    = 10  // 서버→클라이언트: 점수 갱신 알림
} GameEventType;

// PayloadGameEvent: MSG_GAME_EVENT 전송 시 사용하는 페이로드 구조체
typedef struct {
    GameEventType event_type;  // 이벤트 종류
    uint32_t      entityId;    // 관련 엔티티 ID (네트워크 바이트 순서)
} PayloadGameEvent;

// MsgHeader: 모든 메시지 최상단에 붙는 헤더 구조체
typedef struct {
    uint32_t length;  // 뒤따르는 페이로드 길이 (네트워크 바이트 순서)
    MsgType  type;    // 메시지 종류
} MsgHeader;

// PayloadJoin: MSG_JOIN 요청 시 클라이언트→서버 전송
typedef struct {
    int role;  // 1: DEFENDER, 2: ATTACKER
} PayloadJoin;

// PayloadJoinAck: MSG_JOIN_ACK 응답 시 서버→클라이언트 전송
typedef struct {
    uint32_t entityId; // 부여된 엔티티 ID (네트워크 바이트 순서)
    int      role;     // 요청된 역할 상수
} PayloadJoinAck;

// PayloadStateUpdate: MSG_STATE_UPDATE 전송 시 사용하는 페이로드

typedef struct {
    uint32_t entityId; // 업데이트할 엔티티 ID (네트워크 바이트 순서)
    int      x, y;     // 화면 좌표
    int      role;     // ENTITY_DEFENDER 또는 ENTITY_ATTACKER
} PayloadStateUpdate;

// PayloadShootingEvent: MSG_SHOOTING_EVENT 전송 시 사용하는 페이로드
typedef struct {
    uint32_t shooterId;  // 발사자 엔티티 ID (네트워크 바이트 순서)
    uint32_t bulletId;   // 생성될 총알 엔티티 ID (네트워크 바이트 순서)
    int      dirX, dirY; // 발사 방향 벡터 (-1,0,1)
} PayloadShootingEvent;



#endif /* PROTOCOL_H */