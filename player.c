#include "player.h"
#include "ringmaster.h"

int checkpotato(int socket_fd,int client_fd,int neighbor_fd,int num_players,int ID,potato_t *potato){
  //message to shut down the game
  if(potato->num_hops==0){
    return 1;
  }
  //change the message on potato
  potato->num_hops--;
  char ID_char[4];
  sprintf(ID_char,"%d",ID);
  ID_char[3]=0;
  if(strlen(potato->IDlist)==0){
    strncat(potato->IDlist,ID_char,strlen(ID_char));
  }
  else{
    strncat(potato->IDlist,",",1);
    strncat(potato->IDlist,ID_char,strlen(ID_char));
  }
  //continue the game
  if(potato->num_hops==0){
    //it: sends potato to ringmaster
    printf("I'm it\n");
    send(socket_fd,potato,sizeof(potato_t),0);
  }
  else{
    //sends potato to neighbor
    int random =rand()%2;
    //send to left
    if(random==0){
      send(client_fd,potato,sizeof(potato_t),0);
      if(ID==0){
	printf("Sending potato to %d\n",num_players-1);
      }
      else{
	printf("Sending potato to %d\n",ID-1);
      }
    }
    //send to right
    else{
      send(neighbor_fd,potato,sizeof(potato_t),0);
      if(ID==num_players-1){
	printf("Sending potato to %d\n",0);
      }
      else{
	printf("Sending potato to %d\n",ID+1);
      }
    }
  }
  return 0;
}

void startgame(int socket_fd,int client_fd,int neighbor_fd,int num_players,int ID){
  potato_t potato;
  srand((unsigned int) time(NULL) + ID);
  while(1){
    memset(&potato,0,sizeof(potato_t));
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(socket_fd,&readfds);
    FD_SET(client_fd,&readfds);
    FD_SET(neighbor_fd,&readfds);
    int max_fd = socket_fd;
    if(client_fd>socket_fd){
      max_fd = client_fd;
    }
    if(neighbor_fd>max_fd){
      max_fd = neighbor_fd;
    }
    //selecct fd                                                                
    if(select(max_fd+1,&readfds,NULL,NULL,NULL)==-1){
      fprintf(stderr,"select fails\n");
    }
    //receive message/potato from ringmaster
    if(FD_ISSET(socket_fd,&readfds)){
      recv(socket_fd,&potato,sizeof(potato_t), MSG_WAITALL);
      if(checkpotato(socket_fd,client_fd,neighbor_fd,num_players,ID,&potato)){
	break;
      }
    }
    //receive potato from neighbor
    if(FD_ISSET(client_fd,&readfds)){
      recv(client_fd,&potato,sizeof(potato_t), MSG_WAITALL);
      if(checkpotato(socket_fd,client_fd,neighbor_fd,num_players,ID,&potato)){
	break;
      }
    }
    if(FD_ISSET(neighbor_fd,&readfds)){
      recv(neighbor_fd,&potato,sizeof(potato_t), MSG_WAITALL);
      if(checkpotato(socket_fd,client_fd,neighbor_fd,num_players,ID,&potato)){
	break;
      }
    }
  }
}

int main(int argc, char *argv[]){
  //check command
  if(argc!=3){
    fprintf(stderr,"correct format: ./player <machine_name> <port_num>\n");
    return EXIT_FAILURE;
  }
  const char *hostname =argv[1];
  const char *port = argv[2];

  //set socket of players
  int socket_fd;
  int status;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
   if (status != 0) {
     fprintf(stderr,"Error: cannot get address info for host（%s,%s)\n",hostname,port);
    return EXIT_FAILURE;
  }
  socket_fd = socket(host_info_list->ai_family,host_info_list->ai_socktype,host_info_list->ai_protocol);
  if (socket_fd == -1) {
    fprintf(stderr,"Error: cannot create socket（%s,%s)\n",hostname,port);
    return EXIT_FAILURE;
  }  
  //conncet to ringmaster                                    
  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    fprintf(stderr,"Error: cannot connect to socket（%s,%s)\n",hostname,port);
    return EXIT_FAILURE;
  }

  //send initial message to ringmster
  char local_hostname[256];
  memset(local_hostname,0,256);
  if(gethostname(local_hostname, 256)==-1){
    fprintf(stderr,"cannot get the host name\n");
  }
  send(socket_fd,local_hostname,strlen(local_hostname),0);

  
  //receive initial message from ringmaster             
  int num_players;
  recv(socket_fd, &num_players, sizeof(int), 0);
  int ID;
  recv(socket_fd, &ID, sizeof(int), 0);
  char neighbor_hostname[256];
  recv(socket_fd, neighbor_hostname, 256, 0);
  int neighbor_port_int;
  if(ID==0){
    neighbor_port_int=30000+num_players-1;
  }
  else{
    neighbor_port_int=30000+ID-1;
    }
  char neighbor_port[6];
  sprintf(neighbor_port,"%d",neighbor_port_int);
  neighbor_port[5]=0;
  printf("Connected as player %d out of %d total players\n",ID ,num_players);

  //set socket as server
  int server_fd;
  int status_s;
  struct addrinfo host_info_s;
  struct addrinfo *host_info_list_s;
  const char *hostname_s = NULL;
  int port_int =ID+30000;
  char port_s[6];
  sprintf(port_s,"%d",port_int);
  port_s[5]=0;
  memset(&host_info_s, 0, sizeof(host_info_s));
  host_info_s.ai_family   = AF_UNSPEC;
  host_info_s.ai_socktype = SOCK_STREAM;
  host_info_s.ai_flags    = AI_PASSIVE;
  status_s = getaddrinfo(hostname_s, port_s, &host_info_s, &host_info_list_s);
  if (status_s != 0) {
    fprintf(stderr,"Error:cannot get address info for host (%s,%s)\n",hostname_s,port_s);
    return EXIT_FAILURE;
  }
  server_fd = socket(host_info_list_s->ai_family, host_info_list_s->ai_socktype, host_info_list_s->ai_protocol);
  if (server_fd == -1) {
    fprintf(stderr,"Error: cannot create socket (%s,%s)\n",hostname_s,port_s);
    return EXIT_FAILURE;
  }
  int yes = 1;
  status_s = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status_s = bind(server_fd, host_info_list_s->ai_addr, host_info_list_s->ai_addrlen);
  if (status_s == -1) {
    fprintf(stderr,"Error: cannot bind socket (%s,%s)\n",hostname_s,port_s);
    return EXIT_FAILURE;
  }
  status_s = listen(server_fd, 100);
  if (status == -1) {
    fprintf(stderr,"Error: cannot listen on socket (%s,%s)\n",hostname_s,port_s);
    return EXIT_FAILURE;
  }
  
  //set socket as client
  int client_fd;
  int status_c;
  struct addrinfo host_info_c;
  struct addrinfo *host_info_list_c;
  memset(&host_info_c, 0, sizeof(host_info_c));
  host_info_c.ai_family   = AF_UNSPEC;
  host_info_c.ai_socktype = SOCK_STREAM;
  status_c = getaddrinfo(neighbor_hostname, neighbor_port, &host_info_c, &host_info_list_c);
   if (status_c != 0) {
    fprintf(stderr,"Error: cannot get address info for host (%s,%s)\n",neighbor_hostname,neighbor_port);
    return EXIT_FAILURE;
  }
  client_fd = socket(host_info_list_c->ai_family,host_info_list_c->ai_socktype,host_info_list_c->ai_protocol);
  if (client_fd == -1) {
    fprintf(stderr,"Error: cannot create socket (%s,%s)\n",neighbor_hostname,neighbor_port);
    return EXIT_FAILURE;
  }

  //connection between players
  int operation;
  int neighbor_fd;
  int ack=1;
  for(int i=0;i<2;i++){
    //operation one:accept==0,connect==1
    recv(socket_fd,&operation,sizeof(int),0);
    if(operation==0){
      //accept connection from another neighbor
      struct sockaddr_storage socket_addr;
      socklen_t socket_addr_len = sizeof(socket_addr);
      neighbor_fd = accept(server_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
      if (neighbor_fd == -1) {
	fprintf(stderr,"Error: cannot accept connection on socket to neighbor\n");
	exit(EXIT_FAILURE);
      }
      //send ack
      send(socket_fd,&ack,sizeof(int),0);
    }
    if(operation==1){
      //conncet to neighbor                                          
      status_c = connect(client_fd, host_info_list_c->ai_addr, host_info_list_c->ai_addrlen);
      if (status_c == -1) {
	fprintf(stderr,"Error: cannot connect to socket (%s,%s)\n",neighbor_hostname,neighbor_port);
	return EXIT_FAILURE;
      }
      //send ack                                                    
      send(socket_fd,&ack,sizeof(int),0);
    }
  }

  //wait for receiving message
  startgame(socket_fd,client_fd,neighbor_fd,num_players,ID);

  freeaddrinfo(host_info_list_c);
  close(client_fd);
  freeaddrinfo(host_info_list_s);
  close(server_fd);
  freeaddrinfo(host_info_list);
  close(socket_fd);
  return EXIT_SUCCESS;
}

