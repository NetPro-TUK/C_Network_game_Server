#include "protocol.h"
#include "game_logic.h"
#include "entity.h"

void handle_join(SOCKET client_fd, PayloadJoin* payload) {
    EntityType type;
    if (payload->role == 1)
        type = ENTITY_PLAYER;
    else if (payload->role == 2)
        type = ENTITY_ATTACKER;
    else {
        printf("잘못된 역할: %d\n", payload->role);
        return;
    }

    Entity* ent = create_entity(type, client_fd);
    if (ent) {
        printf("JOIN: client_fd=%d → entity_id=%u [%s]\n",
            client_fd, ent->entity_id,
            type == ENTITY_PLAYER ? "PLAYER" : "ATTACKER");
    }
}
