#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <winsock2.h>
#include "protocol.h"

void handle_join(SOCKET client_fd, PayloadJoin* payload);

#endif
