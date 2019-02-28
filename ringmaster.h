#ifndef __RINGMASTER_H__
#define __RINGMASTER_H__

#include "potato.h"

//information record structure for ringmaster
typedef struct _playerinfo_t {
  int ID;
  int player_fd;
  int listen_port;
  char hostname[40];
}playerinfo_t;

//check commands
void commandcheck(int port_num, int num_players, int num_hops);
//connet to all players
void connectplayers(int master_fd, int num_players, playerinfo_t *information);
//send message to players
void sendmessage(int num_players, playerinfo_t *information);
//check connection between players
void checkconnection(int num_players,playerinfo_t *information);
//send potato to players
void sendpotato(int num_hops, int num_players, playerinfo_t *information, potato_t potato);

#endif

