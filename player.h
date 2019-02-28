#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "potato.h"

//wait for receiving message
void startgame(int socket_fd,int client_fd,int neighbor_fd,int num_players,int ID);
//check the state of potato and do corresponding operation
int checkpotato(int socket_fd,int client_fd,int neighbor_fd,int num_players,int ID,potato_t *potato);

#endif
