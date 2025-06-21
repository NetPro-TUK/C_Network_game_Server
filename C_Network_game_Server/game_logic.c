// 공용 헤더 파일
#include "entity.h"
#include "protocol.h"
#include "log.h"
#include "net_utils.h"
#include <time.h>
// 서버 헤더 파일
#include "game_logic.h"
#include "net_server.h"

uint32_t defender_owner_id = 0;

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
    if (type == ENTITY_DEFENDER && defender_owner_id != 0) {
        printf("Server> 이미 방어자가 존재합니다. 요청 거부.\n");

        MsgHeader h = { .type = MSG_GAME_EVENT, .length = htonl(sizeof(PayloadGameEvent)) };
        PayloadGameEvent event = { .event_type = PLAYER_REJECTED };
        send_full(client_fd, &h, sizeof(h));
        send_full(client_fd, &event, sizeof(event));

        closesocket(client_fd);
        return;
    }

    // 엔터티 생성
    uint32_t newClientId = generate_client_id();
    Entity* ent = create_entity(type, newClientId, client_fd);
    if (!ent) {
        LOG_ERROR("엔터티 생성 실패");
        return;
    }
    // ▶ 시작 위치 설정: 방어자 x=70, 공격자 x=1, y는 필드 중앙
        ent->y = SCREEN_HEIGHT / 2;  // game_logic.h: SCREEN_HEIGHT = 25
        if (type == ENTITY_DEFENDER) {
            ent->x = 70;
            defender_owner_id = newClientId;

        }
        else {  // ENTITY_ATTACKER
            ent->x = 1;
        }

    printf("Server> JOIN: client_fd=%d → entity_id=%u [%s]\n", client_fd, ent->entity_id,
        type == ENTITY_DEFENDER ? "DEFENDER" : "ATTACKER");

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
// 현재 시간을 밀리초 단위로 반환하는 함수
uint64_t current_time_ms() {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
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
    if (shooter->reloading) {
        LOG_INFO("Shooter is reloading.");
        return;
    }
    if (shooter->ammo <= 0) {
        LOG_INFO("Shooter has no ammo left.");
        return;
    }
    shooter->ammo--;

    // 총알 생성
    Entity* bullet = create_entity(ENTITY_BULLET, shooter->owner_client_id, client_fd);
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

void handle_reload_request(SOCKET client_fd, uint32_t entity_id) {
    Entity* e = get_entity_by_id(entity_id);
    if (!e || e->type != ENTITY_DEFENDER) return;
    if (e->ammo == 20 || e->reloading) return;

    e->reloading = 1;
    e->reload_start_time_ms = current_time_ms();  // 타이머 시작
    LOG_INFO("Reload started by entity %u", entity_id);
}

// 랜덤으로 공격자들을 이동시키는 함수
void auto_move_attackers() {
    for (int i = 0; i < entityCount; ++i) {
        Entity* e = &entityArr[i];
        if (!e->alive) continue;

        if (e->type == ENTITY_ATTACKER) {
            // 1) 수평 이동: 오른쪽으로 1칸, 필드 끝 도달 시 왼쪽으로 감싸기
            e->x++;
            if (e->x >= SCREEN_WIDTH) {
                e->x = 1;
            }

            // 2) 수직 이동: -1, 0, +1 중 랜덤
            int dir = (rand() % 3) - 1;
            e->y += dir;
            if (e->y < 1)                e->y = 1;
            if (e->y >= SCREEN_HEIGHT - 1) e->y = SCREEN_HEIGHT - 2;
        }
    }
}

// 게임 틱 처리 함수
void game_tick() {
    // 0) (서버 한 번만) 난수 시드 초기화 – main() 쪽에 있어도 무방
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned)time(NULL));
        seeded = 1;
    }

    // (추가 기능) 방어자 재장전 상태 갱신
    for (int i = 0; i < entityCount; ++i) {
        Entity* e = &entityArr[i];
        if (e->alive && e->type == ENTITY_DEFENDER && e->reloading) {
            uint64_t now = current_time_ms();
            if (now - e->reload_start_time_ms >= 3000) {
                e->ammo = 20;
                e->reloading = 0;
                LOG_INFO("Reload complete for entity %u", e->entity_id);
            }
        }
    }

    // 1) 게임이 시작된 후에만 엔터티 이동 & 충돌 처리
    if (game_started) {
        // 1-1) 위치 업데이트 (총알 등)
        for (int i = 0; i < entityCount; ++i) {
            Entity* e = &entityArr[i];
            if (!e->alive) continue;
            e->x += e->vx;
            e->y += e->vy;
            if (e->x < 0 || e->x >= SCREEN_WIDTH ||
                e->y < 0 || e->y >= SCREEN_HEIGHT) {
                mark_entity_dead(e->entity_id);
                LOG_INFO("Entity %d out of bounds → dead", e->entity_id);
            }
        }

        // 1-2) 충돌 검사
        check_collision();

        // 1-3) 모든 클라이언트에 상태 브로드캐스트
        send_state_update();
    }
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
        payload.role = ent->type;

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

        // ▶ 디버그 로그 추가
        printf("[SERVER_LOG] SEND STATE_UPDATE → id=%u, x=%d, y=%d, role=%d (%s)\n",
            ent->entity_id,
            ent->x,
            ent->y,
            ent->type,
            ent->type == ENTITY_ATTACKER ? "ATTACKER" :
            ent->type == ENTITY_DEFENDER ? "DEFENDER" :
            ent->type == ENTITY_BULLET ? "BULLET" :
            "UNKNOWN");
    }
}

// 충돌 체크 함수
void check_collision() {
    for (int i = 0; i < entityCount; ++i) {
        Entity* a = &entityArr[i];
        if (!a->alive) continue;

        for (int j = i + 1; j < entityCount; ++j) {
            Entity* b = &entityArr[j];
            if (!b->alive) continue;

            int dx = abs(a->x - b->x);
            int dy = abs(a->y - b->y);

            if (dx <= COLLISION_RADIUS && dy <= COLLISION_RADIUS) {
                // --- 총알 vs 공격자 충돌 처리 ---
                Entity* bullet = NULL, * attacker = NULL;
                if (a->type == ENTITY_BULLET && b->type == ENTITY_ATTACKER) {
                    bullet = a;
                    attacker = b;
                }
                else if (b->type == ENTITY_BULLET && a->type == ENTITY_ATTACKER) {
                    bullet = b;
                    attacker = a;
                }

                if (bullet && attacker) {
                    // 1) 서버 내부 상태에서 둘 다 죽음으로 표시
                    mark_entity_dead(attacker->entity_id);
                    mark_entity_dead(bullet->entity_id);
                    LOG_INFO("Bullet hit attacker → removed entities %u(attacker), %u(bullet)",
                        attacker->entity_id, bullet->entity_id);

                    // 2) MSG_GAME_EVENT(ENTITY_REMOVE) 이벤트 페이로드 준비
                    PayloadGameEvent ev;
                    ev.event_type = ENTITY_REMOVE;
                    MsgHeader hdr = {
                        .length = htonl(sizeof(ev)),
                        .type = MSG_GAME_EVENT
                    };

                    // ▶ 공격자 삭제 알림
                    ev.entityId = htonl(attacker->entity_id);
                    broadcast_all(&hdr, sizeof(hdr));
                    broadcast_all(&ev, sizeof(ev));

                    // ▶ 총알 삭제 알림
                    ev.entityId = htonl(bullet->entity_id);
                    broadcast_all(&hdr, sizeof(hdr));
                    broadcast_all(&ev, sizeof(ev));
                }

                // --- 방어자 vs 공격자 충돌 (게임 오버) 처리 ---
                else if ((a->type == ENTITY_DEFENDER && b->type == ENTITY_ATTACKER) ||
                    (b->type == ENTITY_DEFENDER && a->type == ENTITY_ATTACKER)) {
                    mark_entity_dead(a->entity_id);
                    mark_entity_dead(b->entity_id);
                    LOG_WARN("Player hit by attacker → Game Over");
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
    server_game_over = true;
}