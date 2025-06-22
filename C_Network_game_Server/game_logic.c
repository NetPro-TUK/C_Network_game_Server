#include <time.h>
// 공용 헤더 파일
#include "entity.h"
#include "protocol.h"
#include "log.h"
#include "net_utils.h"
// 서버 헤더 파일
#include "game_logic.h"
#include "net_server.h"

uint32_t defender_owner_id = 0;
uint32_t current_score;

// game_logi.c 에서만 사용
static int attacker_speed = 1;              // 전역 속도 관리 변수
static int ammo = 20;                       // 방어자의 초기 총알 수
static int reloading = 0;                   // 방어자 재장전 상태
static uint64_t reload_start_time_ms = 0;   // 재장전 시작 시간

// 클라이언트의 JOIN 요청을 처리 (클라에서 역할 선택 시)
void handle_join(SOCKET client_fd, PayloadJoin* payload) {
    EntityType type;

    // payload.role 값에 따라 역할 결정
    if (payload->role == 1)
        type = ENTITY_DEFENDER;
    else if (payload->role == 2)
        type = ENTITY_ATTACKER;
    else {
        LOG_WARN("잘못된 역할 요청");
        return;
    }

    // Defender 중복 가입 방지
    if (type == ENTITY_DEFENDER && defender_owner_id != 0) {
        printf("Server> 이미 방어자가 존재합니다. 요청 거부.\n");

        MsgHeader h = { .type = MSG_GAME_EVENT, .length = htonl(sizeof(PayloadGameEvent)) };
        PayloadGameEvent event = { .event_type = PLAYER_REJECTED };
        send_full(client_fd, &h, sizeof(h));
        send_full(client_fd, &event, sizeof(event));

        closesocket(client_fd);
        return;
    }

    // 새 클라이언트 ID 생성 및 엔티티 등록
    uint32_t newClientId = generate_client_id();
    Entity* ent = create_entity(type, newClientId, client_fd);
    if (!ent) {
        LOG_ERROR("엔터티 생성 실패");
        return;
    }

    // 초기 위치 설정: Y는 화면 중앙, X는 역할에 따라
    ent->y = SCREEN_HEIGHT / 2;
    if (type == ENTITY_DEFENDER) {
        ent->x = 75;
        defender_owner_id = newClientId;

    }
    else {  // ENTITY_ATTACKER
        ent->x = 1;
    }

    printf("Server> JOIN: client_fd=%d → entity_id=%u [%s]\n", client_fd, ent->entity_id,
        type == ENTITY_DEFENDER ? "DEFENDER" : "ATTACKER");

    // JOIN_ACK 메시지 전송 (서버 -> 클라)
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
void handle_shooting_event(SOCKET client_fd, PayloadShootingEvent* payload) {
    uint32_t shooter_id = ntohl(payload->shooterId);
    uint32_t bullet_id = ntohl(payload->bulletId);

    // 발사자 엔티티 조회
    Entity* shooter = get_entity_by_id(shooter_id);
    if (!shooter) {
        LOG_WARN("Shooter not found");
        return;
    }

    // 방어자 발사일 경우 탄약 및 재장전 상태 검사
    if (shooter->type == ENTITY_DEFENDER) {
        if (reloading) {
            LOG_INFO("Defender is reloading.");
            return;
        }
        if (ammo <= 0) {
            // 탄약 없음 알림 전송
            LOG_INFO("Defender has no ammo left.");
            MsgHeader header = { .type = MSG_GAME_EVENT, .length = htonl(sizeof(PayloadGameEvent)) };
            PayloadGameEvent p = { .event_type = OUT_OF_AMMO, .entityId = htonl(shooter_id) };
            send_full(client_fd, &header, sizeof(header));
            send_full(client_fd, &p, sizeof(p));
            return;
        }
        ammo--;  // 탄약 소모
    }

    // 총알 엔티티 생성 및 초기 상태 설정
    Entity* bullet = create_entity(ENTITY_BULLET, shooter->owner_client_id, client_fd);
    if (!bullet) {
        LOG_WARN("Bullet creation failed: entity limit reached");
        return;
    }
    bullet->entity_id = bullet_id;
    bullet->x = shooter->x;
    bullet->y = shooter->y;
    bullet->vx = payload->dirX;
    bullet->vy = payload->dirY;
    bullet->alive = 1;

    LOG_INFO("Bullet created");
}


// 재장전 요청 처리: 재장전 플래그 설정 및 타이머 시작
void handle_reload_request() {
    if (ammo == 20 || reloading) return;
    reloading = 1;
    reload_start_time_ms = current_time_ms();  // 타이머 시작
    LOG_INFO("Reload started");
}

// 현재 시간을 밀리초 단위로 반환하는 함수
uint64_t current_time_ms() {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

// 랜덤으로 공격자들을 이동시키는 함수
void auto_move_attackers() {
    static int attacker_speed = 1;  // 속도 초기값 설정
    for (int i = 0; i < entityCount; ++i) {
        Entity* e = &entityArr[i];
        if (!e->alive) continue;

        if (e->type == ENTITY_ATTACKER) {
            e->x += attacker_speed;  // 속도만큼 이동

            if (e->x >= SCREEN_WIDTH) {
                e->x = 1;
                attacker_speed++;  // 벽에 도달할 때마다 속도 증가
                if (attacker_speed > 5) attacker_speed = 5;  // 속도 최대 제한
            }

            // Y축 랜덤 이동
            int dir = (rand() % 3) - 1;
            e->y += dir;
            if (e->y < 1) e->y = 1;
            if (e->y >= SCREEN_HEIGHT - 1) e->y = SCREEN_HEIGHT - 2;
        }
    }
}


// 매 틱마다 게임 상태 업데이트: 이동, 충돌, 브로드캐스트 등
void game_tick() {
    // 0) 난수 시드 초기화 (최초 호출 시)
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned)time(NULL));
        seeded = 1;
    }

    // 방어자 재장전 완료 체크 및 알림
    if (reloading) {
        uint64_t now = current_time_ms();
        if (now - reload_start_time_ms >= 3000) {
            ammo = 20;
            reloading = 0;
            LOG_INFO("Reload complete");

            // 모든 클라이언트에 RELOAD_COMPLETE 이벤트 전송
            MsgHeader header = { .type = MSG_GAME_EVENT, .length = htonl(sizeof(PayloadGameEvent)) };
            PayloadGameEvent payload = { .event_type = RELOAD_COMPLETE, .entityId = htonl(defender_owner_id) };
            for (int i = 1; i < numOfClnt; ++i) {
                send_full(sockArr[i], &header, sizeof(header));
                send_full(sockArr[i], &payload, sizeof(payload));
            }
        }
    }
    // 탄약 부족 시 OUT_OF_AMMO 반복 알림
    if (ammo <= 0) {
        MsgHeader header = { .type = MSG_GAME_EVENT, .length = htonl(sizeof(PayloadGameEvent)) };
        PayloadGameEvent payload = { .event_type = OUT_OF_AMMO, .entityId = htonl(defender_owner_id) };
        for (int i = 1; i < numOfClnt; ++i) {
            send_full(sockArr[i], &header, sizeof(header));
            send_full(sockArr[i], &payload, sizeof(payload));
        }
    }

    // 게임이 시작되었을 때만 처리
    if (game_started) {
        // 1-1) 엔티티 이동 및 범위 벗어난 엔티티 제거
        for (int i = 0; i < entityCount; ++i) {
            Entity* e = &entityArr[i];
            if (!e->alive) continue;
            e->x += e->vx;
            e->y += e->vy;
            // 화면 바깥 벗어나면 죽음 처리 및 제거 알림
            if (e->x < 0 || e->x >= SCREEN_WIDTH || e->y < 0 || e->y >= SCREEN_HEIGHT) {
                mark_entity_dead(e->entity_id);
                LOG_INFO("Entity %d out of bounds → dead", e->entity_id);

                if (e->type == ENTITY_BULLET) {
                    PayloadGameEvent ev = { .event_type = ENTITY_REMOVE, .entityId = htonl(e->entity_id) };
                    MsgHeader hdr = { .type = MSG_GAME_EVENT, .length = htonl(sizeof(ev)) };
                    broadcast_all(&hdr, sizeof(hdr));
                    broadcast_all(&ev, sizeof(ev));
                }
            }
        }
        // 1-2) 충돌 검사 및 처리(점수, 게임 오버 등)
        check_collision();
        // 1-3) 모든 클라이언트에 상태 업데이트 전송
        send_state_update();
    }
}

// 활성 엔티티 상태를 모든 클라이언트에 브로드캐스트
void send_state_update() {
    for (int i = 0; i < entityCount; ++i) {
        Entity* ent = &entityArr[i];
        if (!ent->alive) continue;

        // 메시지 헤더 및 페이로드 구성
        PayloadStateUpdate payload = { .entityId = htonl(ent->entity_id), .x = ent->x, .y = ent->y, .role = ent->type };
        MsgHeader header = { .type = MSG_STATE_UPDATE, .length = htonl(sizeof(payload)) };

        // 각 클라이언트에 전송
        for (int c = 1; c < numOfClnt; ++c) {
            send_full(sockArr[c], &header, sizeof(header));
            send_full(sockArr[c], &payload, sizeof(payload));
        }

        // 디버그 로그 추가
        printf("[SERVER_LOG] SEND STATE_UPDATE → id=%u, x=%d, y=%d, role=%d\n",
            ent->entity_id, ent->x, ent->y, ent->type);
    }
}

// 모든 엔티티 간 충돌을 검사하고 처리
void check_collision() {
    for (int i = 0; i < entityCount; ++i) {
        Entity* a = &entityArr[i];
        if (!a->alive) continue;

        for (int j = i + 1; j < entityCount; ++j) {
            Entity* b = &entityArr[j];
            if (!b->alive) continue;

            int dx = abs(a->x - b->x);
            int dy = abs(a->y - b->y);
            // 충돌 반경 내 진입 시 처리
            if (dx <= COLLISION_RADIUS && dy <= COLLISION_RADIUS) {
                // 총알 vs 공격자 충돌 처리 로직
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
                    // 내부 상태 반영: 둘 다 죽음 처리
                    mark_entity_dead(attacker->entity_id);
                    mark_entity_dead(bullet->entity_id);
                    // 점수 증가 및 전파
                    current_score++;          
                    PayloadGameEvent score_ev = { .event_type = SCORE_UPDATE, .entityId = htonl(current_score) };
                    MsgHeader score_hdr = { .type = MSG_GAME_EVENT, .length = htonl(sizeof(score_ev)) };
                    broadcast_all(&score_hdr, sizeof(score_hdr));
                    broadcast_all(&score_ev, sizeof(score_ev));

                    // ENTITY_REMOVE 이벤트 전파 (공격자, 총알)
                    PayloadGameEvent ev; ev.event_type = ENTITY_REMOVE;
                    MsgHeader hdr = { .type = MSG_GAME_EVENT, .length = htonl(sizeof(ev)) };
                    // ▶ 공격자 삭제 알림
                    ev.entityId = htonl(attacker->entity_id);
                    broadcast_all(&hdr, sizeof(hdr));
                    broadcast_all(&ev, sizeof(ev));
                    // ▶ 총알 삭제 알림
                    ev.entityId = htonl(bullet->entity_id);
                    broadcast_all(&hdr, sizeof(hdr));
                    broadcast_all(&ev, sizeof(ev));
                }
                // 방어자 vs 공격자 충돌 시 게임 오버 처리
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

// 게임 오버 검사 및 알림 전송
void check_game_over() {
    int playerAlive = 0;
    // 남아있는 방어자 유무 확인
    for (int i = 0; i < entityCount; ++i) {
        Entity* ent = &entityArr[i];
        if (ent->alive && ent->type == ENTITY_DEFENDER) {
            playerAlive = 1;
            break;
        }
    }
    // 방어자 없으면 게임 종료 이벤트 전송
    if (!playerAlive) {
        LOG_WARN("GAME OVER: All players are dead");

        // 클라이언트에게 게임 종료 알림
        PayloadGameEvent payload;
        payload.event_type = 1;

        MsgHeader header;
        header.length = htonl(sizeof(payload));
        header.type = MSG_GAME_EVENT;

        for (int i = 1; i < numOfClnt; ++i) {
            send_full(sockArr[i], &header, sizeof(header));
            send_full(sockArr[i], &payload, sizeof(payload));
        }
    }
    // 서버 종료 플래그 설정
    server_game_over = true;
}