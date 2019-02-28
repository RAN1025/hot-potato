#include "ringmaster.h"
#include "player.h"

void commandcheck(int port_num, int num_players, int num_hops){
  if(port_num<1024||port_num>65535){
    fprintf(stderr,"The number of port should be greater than 1024 and less than 65535\n");
    exit(EXIT_FAILURE);
  }
  if(num_players<2){
    fprintf(stderr,"The number of players must be greater than 1\n");
    exit(EXIT_FAILURE);
  }
  if(num_hops<0||num_hops>512){
    fprintf(stderr,"The number of hops must be greater than or equal to zero and less than or equal to 512\n");
    exit(EXIT_FAILURE);
  }
}

void connectplayers(int master_fd, int num_players, playerinfo_t *information){
  for (int i=0;i<num_players;i++){
    //connection
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int players_fd;
    players_fd = accept(master_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (players_fd == -1) {
      fprintf(stderr,"Error: cannot accept connection on socket to ringmaster\n");
      exit(EXIT_FAILURE);
    }   
    //receive initial message from players
    char player_hostname[256] = {0};
    recv(players_fd, player_hostname, 256, 0);
    //store information of players
    memset(&(information[i]), 0, sizeof(playerinfo_t));
    information[i].ID=i;
    information[i].player_fd=players_fd;
    information[i].listen_port=30000+i;
    strcpy(information[i].hostname, player_hostname);
    printf("Player %d is ready to play\n",i);
  }
}

void sendmessage(int num_players, playerinfo_t *information){
  for (int i=0;i<num_players;i++){
    //send initial message to players
    send(information[i].player_fd,&num_players,sizeof(int),0);
    send(information[i].player_fd,&information[i].ID,sizeof(int),0);
    //send neighbor hostname
    if(i==0){
      send(information[i].player_fd,information[num_players-1].hostname,256,0);
    }
    else{
      send(information[i].player_fd,information[i-1].hostname,256,0);
    }
  }
}

void checkconnection(int num_players,playerinfo_t *information){
  int operation;
  int ack;
  for(int i=0;i<num_players-1;i++){
    operation=0;
    send(information[i].player_fd,&operation,sizeof(int),0);
    operation=1;
    send(information[i+1].player_fd,&operation,sizeof(int),0);
    //wait for ack
    recv(information[i].player_fd,&ack,sizeof(int),0);
    recv(information[i+1].player_fd,&ack,sizeof(int),0);
  }
  operation=0;
  send(information[num_players-1].player_fd,&operation,sizeof(int),0);
  operation=1;
  send(information[0].player_fd,&operation,sizeof(int),0);
  //wait for ack                                                                                            
  recv(information[num_players-1].player_fd,&ack,sizeof(int),0);
  recv(information[0].player_fd,&ack,sizeof(int),0);
}

void sendpotato(int num_hops, int num_players, playerinfo_t *information, potato_t potato){
  //send potato
  srand((unsigned int)time(NULL) + num_players);
  int random = rand()%num_players;
  send(information[random].player_fd,&potato,sizeof(potato_t),0);
  printf("Ready to start the game, sending potato to player %d\n",random);
}

int main(int argc, char *argv[]){
  //check command
  if(argc!=4){
    fprintf(stderr,"correct format: ./ringmaster <port_num> <num_players> <num_hops>\n");
    return EXIT_FAILURE;
  }
  int port_num = atoi(argv[1]);
  int num_players = atoi(argv[2]);
  int num_hops = atoi(argv[3]);
  commandcheck(port_num,num_players,num_hops);
  
  //initial output
  printf("Potato Ringmaster\n");
  printf("Players = %d\n",num_players);
  printf("Hops = %d\n",num_hops);

  //players' information
  playerinfo_t information[num_players];

  //set socket of ringmaster
  int master_fd;
  int status;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = NULL;
  const char *port     = argv[1];
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;
  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    fprintf(stderr,"Error:cannot get address info for host (%s,%s)\n",hostname,port);
    return EXIT_FAILURE;
  }
  master_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
  if (master_fd == -1) {
    fprintf(stderr,"Error: cannot create socket (%s,%s)\n",hostname,port);
    return EXIT_FAILURE;
  }
  int yes = 1;
  status = setsockopt(master_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(master_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    fprintf(stderr,"Error: cannot bind socket (%s,%s)\n",hostname,port);
    return EXIT_FAILURE;
  }
  status = listen(master_fd, 100);
  if (status == -1) {
    fprintf(stderr,"Error: cannot listen on socket (%s,%s)\n",hostname,port);
    return EXIT_FAILURE;
  }

  //connect to players
  connectplayers(master_fd,num_players,information);
  //send message to players
  sendmessage(num_players, information);

  //check connection between players
  checkconnection(num_players,information);
  
  //create potato to start game
  potato_t potato;
  memset(&potato,0,sizeof(potato_t));
  potato.num_hops = num_hops;
 
  //end game
  if(potato.num_hops==0){
    //send message to shut the game down
    for(int i=0;i<num_players;i++){
      send(information[i].player_fd,&potato,sizeof(potato_t),0);
    }
    freeaddrinfo(host_info_list);
    close(master_fd);
    return EXIT_SUCCESS;
  }

  //send potato
  sendpotato(num_hops,num_players,information,potato);

  //receive potato
  while(1){
    fd_set readfds;
    FD_ZERO(&readfds);
    for(int i=0;i<num_players;i++){
      FD_SET(information[i].player_fd,&readfds);
    }
    int max_fd=information[0].player_fd;
    for(int i=0;i<num_players;i++){
      if(information[i].player_fd>max_fd){
	max_fd=information[i].player_fd;
      }
    }
    //select fd                                                                           
    if(select(max_fd+1,&readfds,NULL,NULL,NULL)==-1){
      fprintf(stderr,"select fails\n");
    }
    for(int i=0;i<num_players;i++){
      if(FD_ISSET(information[i].player_fd,&readfds)){
        recv(information[i].player_fd,&potato,sizeof(potato_t), MSG_WAITALL);
      }
    }
    if(potato.num_hops==0){
      printf("Trace of potato:\n");
      printf("%s\n",potato.IDlist);
      //send message to shut the game down                                                            
      for(int i=0;i<num_players;i++){
	send(information[i].player_fd,&potato,sizeof(potato_t),0);
      }
      break;
    }
  }

  freeaddrinfo(host_info_list);
  close(master_fd);
  return EXIT_SUCCESS;
}
