#include "entity.h"
#include "protocol.h"
#include "game_logic.h"
#include "net_server.h"
#include "log.h"
#include "net_utils.h"

int has_player;  // 전역 또는 전역 배열로 추적

// 클라이언트가 JOIN 요청을 보냈을 때 처리하는 함수
void handle_join(SOCKET client_fd, PayloadJoin* payload) {
    EntityType type;

    if (payload->role == 1)
        type = ENTITY_DEFENDER;
    else if (payload->role == 2)
        type = ENTITY_ATTACKER;
    else {
        LOG_WARN("잘못된 역할 요청");
        return;
    }

    // 방어자 중복 체크
    if (type == ENTITY_PLAYER && has_player) {
        printf("Server> 이미 방어자가 존재합니다. 요청 거부.\n");

        MsgHeader h = { .type = MSG_GAME_EVENT, .length = htonl(sizeof(PayloadGameEvent)) };
        PayloadGameEvent event = { .event_type = PLAYER_REJECTED };
        send_full(client_fd, &h, sizeof(h));
        send_full(client_fd, &event, sizeof(event));

        closesocket(client_fd);
        return;
    }

    // 엔터티 생성
    Entity* ent = create_entity(type, client_fd);
    if (!ent) {
        LOG_ERROR("엔터티 생성 실패");
        return;
    }

    if (type == ENTITY_PLAYER) has_player = 1;

    printf("Server> JOIN: client_fd=%d → entity_id=%u [%s]\n", client_fd, ent->entity_id,
        type == ENTITY_DEFENDER ? "PLAYER" : "ATTACKER");

    // 클라이언트에게 entity_id를 응답으로 보냄
    PayloadJoinAck ackPayload;
    ackPayload.entityId = htonl(ent->entity_id);
    ackPayload.role = type;

    MsgHeader header;
    header.length = htonl(sizeof(ackPayload));
    header.type = MSG_JOIN_ACK;

    send_full(client_fd, &header, sizeof(header));
    send_full(client_fd, &ackPayload, sizeof(ackPayload));
}


// 총알 발사 이벤트를 처리하는 함수
void handle_action_event(SOCKET client_fd, PayloadActionEvent* payload) {
    // 발사자 ID를 바탕으로 엔터티 검색
    uint32_t shooter_id = ntohl(payload->shooterId);
    uint32_t bullet_id = ntohl(payload->bulletId);

    Entity* shooter = get_entity_by_id(shooter_id);
    if (!shooter) {
        LOG_WARN("Shooter not found");
        return;
    }

    // 총알 생성
    Entity* bullet = create_entity(ENTITY_BULLET, shooter->owner_client_id);
    if (!bullet) {
        LOG_WARN("Bullet creation failed: entity limit reached");
        return;
    }

    bullet->entity_id = bullet_id;  // 서버가 아닌 클라이언트가 ID 지정한 경우
    bullet->x = shooter->x;
    bullet->y = shooter->y;
    bullet->vx = payload->dirX;
    bullet->vy = payload->dirY;
    bullet->alive = 1;

    LOG_INFO("Bullet created");
}

// 게임 처리하는 함수
void game_tick() {
    for (int i = 0; i < entityCount; ++i) {
        Entity* e = &entityArr[i];
        if (!e->alive) continue;

        // 위치 업데이트
        e->x += e->vx;
        e->y += e->vy;

        // 화면 밖으로 나가면 죽은 것으로 처리
        if (e->x < 0 || e->x >= SCREEN_WIDTH || e->y < 0 || e->y >= SCREEN_HEIGHT) {
            mark_entity_dead(e->entity_id);
            LOG_INFO("Entity %d is out of bounds and marked dead", e->entity_id);
        }
    }

    // 충돌 검사
    check_collision();

    // 상태 전송
    send_state_update();
}

// 엔티티 상태 업데이트를 모든 클라이언트에게 전송하는 함수
void send_state_update() {
    for (int i = 0; i < entityCount; ++i) {
        Entity* ent = &entityArr[i];
        if (!ent->alive) continue;

        // 페이로드 준비
        PayloadStateUpdate payload;
        payload.entityId = htonl(ent->entity_id);
        payload.x = ent->x;
        payload.y = ent->y;

        // 헤더 준비
        MsgHeader header;
        header.length = htonl(sizeof(payload));
        header.type = MSG_STATE_UPDATE;

        // 헤더 + 페이로드 전송
        for (int c = 1; c < numOfClnt; ++c) {
            SOCKET sock = sockArr[c];
            send_full(sock, &header, sizeof(header));
            send_full(sock, &payload, sizeof(payload));
        }
    }
}

// 충돌 체크 함수 (예: 총알과 플레이어 간의 충돌)
void check_collision() {
    for (int i = 0; i < entityCount; ++i) {
        Entity* a = &entityArr[i];
        if (!a->alive) continue;

        for (int j = i + 1; j < entityCount; ++j) {
            Entity* b = &entityArr[j];
            if (!b->alive) continue;

            // 충돌 조건: 같은 위치에 있음
            if (a->x == b->x && a->y == b->y) {
                // 총알 vs 공격자
                if ((a->type == ENTITY_BULLET && b->type == ENTITY_ATTACKER) ||
                    (a->type == ENTITY_ATTACKER && b->type == ENTITY_BULLET)) {

                    mark_entity_dead(a->entity_id);
                    mark_entity_dead(b->entity_id);
                    LOG_INFO("Bullet hit attacker");
                }

                // 방어자 vs 공격자
                else if ((a->type == ENTITY_DEFENDER && b->type == ENTITY_ATTACKER) ||
                    (a->type == ENTITY_ATTACKER && b->type == ENTITY_DEFENDER)) {

                    mark_entity_dead(a->entity_id);
                    mark_entity_dead(b->entity_id);
                    LOG_WARN("Player hit by attacker → Game Over");
                    // 게임 오버
                    check_game_over();
                }
            }
        }
    }
}

// 게임 오버 체크 함수
void check_game_over() {
    int playerAlive = 0;

    for (int i = 0; i < entityCount; ++i) {
        Entity* ent = &entityArr[i];
        if (ent->alive && ent->type == ENTITY_DEFENDER) {
            playerAlive = 1;
            break;
        }
    }

    if (!playerAlive) {
        LOG_WARN("GAME OVER: All players are dead");

        // 클라이언트에게 게임 종료 알림
        PayloadGameEvent payload;
        payload.event_type = 1; // 1 = 게임 오버

        MsgHeader header;
        header.length = htonl(sizeof(payload));
        header.type = MSG_GAME_EVENT;

        for (int i = 1; i < numOfClnt; ++i) {
            send_full(sockArr[i], &header, sizeof(header));
            send_full(sockArr[i], &payload, sizeof(payload));
        }
    }
}