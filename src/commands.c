#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include "sys/types.h"
#include "sys/wait.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include <pthread.h>

#include "commands.h"
#include "built_in.h"

static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};


#define CLIENT_PATH "tpf_unix_sock.client"
#define SERVER_PATH "tpf_unix_sock.server"
/*
void* make_Server(void* threadid){
  int num = 0;
  char** commands = (char**)threadid;

  for(;commands[num]!=NULL;num++);

  struct single_command sc[8] = {num, commands};

  int server_sock, rc, len, client_sock;

  int backlog = 10;  

  struct sockaddr_un server_sockaddr;
  struct sockaddr_un client_sockaddr;

  memset(&server_sockaddr,0,sizeof(struct sockaddr_un));
  memset(&client_sockaddr,0,sizeof(struct sockaddr_un));

          //make server
  server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if(server_sock == -1){
    printf("SOCKET ERROR\n");
    exit(1);
  }
  printf("make server\n");

  server_sockaddr.sun_family = AF_UNIX;
  strcpy(server_sockaddr.sun_path, SERVER_PATH);
  len = sizeof(server_sockaddr);

  unlink(SERVER_PATH);
  rc = bind(server_sock, (struct sockaddr *) &server_sockaddr, len);
  if(rc == -1){
    printf("BIND ERROR\n");
    close(server_sock);
    exit(1);
  }
  printf("server bind\n");

  rc = listen(server_sock, backlog);
  if(rc == -1){
    printf("LISTEN ERROR\n");
    close(server_sock);
    exit(1);
  }
  printf("listening...\n");


  pthread_t thread[4];
  int t_status;
  rc = pthread_create(&thread[2], NULL, make_Client,(void*)com->argv);//
  printf("createing thread\n");
          //last argument : a single argument that may be passed to start_routine
          //must void type, make_client's variable
  if(rc < 0){
    printf("CREATING THREAD ERROR\n");
    exit(1);
  }
  printf("create thread\n");

  client_sock = accept(server_sock,(struct sockaddr*)&client_sockaddr, &len);
  if(client_sock == -1){
    printf("ACCEPT ERROR\n");
    close(server_sock);
    close(client_sock);
    exit(1);
  }
  printf("accepting\n");

  pthread_join(thread[2], (void**)&t_status);

  pid_t pid2;
  int status2;

  int in = dup(STDIN_FILENO);

  pid2 = fork();

  if(pid2 == 0){

          // printf("(com+1)->argv : %s\n", (com+1)->argv[0]);
    printf("receive data\n");
    dup2(client_sock, STDIN_FILENO); 
    close(client_sock);         

    path_res((com+1)->argv[0],(com+1)->argv);
            // evaluate_command(1,(&com+1)); // second command?
    fprintf(stderr, "%s: command not found\n", (com+1)->argv[0]);
    close(client_sock);
    close(server_sock);
    close(STDIN_FILENO);
    return 0;
  }
  else if(pid2 != 0){
    printf("parent running\n");
            // pthread_exit(thread[2]);

    close(client_sock);
    close(server_sock);
    wait(&status2);
    printf("finish\n");
    return 0;
  }  

}
*/

void* make_Client(void* threadid){ 
// threadid : create()'s last arguemnt'
  int client_sock, rc, len, server_sock;

  int num = 0;
  char** commands = (char**)threadid;

  for(;commands[num]!=NULL;num++);

  //   printf("num : %d\n", num);  
  // for(int i = 0 ; i < num;i++)
  //   printf("commands : %s\n", commands[i]);

    struct single_command sc[8] = {num, commands};

  // printf("sc->argc : %d\n", sc->argc);
  // for(int i = 0 ; i < sc->argc;i++)
  //   printf("sr->argv : %s\n", sc->argv[i]);
  //struct single_command* com = *commands + ti;
  // sc->argc = num;
  // sc->argv = (char**)threadid;

  struct sockaddr_un server_sockaddr;
  struct sockaddr_un client_sockaddr;

  memset(&server_sockaddr,0,sizeof(struct sockaddr_un));
  memset(&client_sockaddr,0,sizeof(struct sockaddr_un));

    //make client socket
  client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
  // printf("make client\n");
  if(client_sock == -1){
    printf("SOCKET ERROR\n");
    exit(1); 
  } 

  client_sockaddr.sun_family = AF_UNIX;
  strcpy(client_sockaddr.sun_path, CLIENT_PATH);
  len = sizeof(client_sockaddr);

  unlink(CLIENT_PATH);
  rc = bind(client_sock, (struct sockaddr *) &client_sockaddr, len);
  // printf("bind\n");
  if(rc == -1){
    printf("BIND ERROR\n");
    close(client_sock);
    exit(1);
  }
  // printf("client binding\n");

  server_sockaddr.sun_family = AF_UNIX;
  strcpy(server_sockaddr.sun_path, SERVER_PATH);
  rc = connect(client_sock, (struct sockaddr*) &server_sockaddr,len);
  // printf("connecting\n");
  if(rc == -1){
   printf("CONNECT ERROR\n");
   close(client_sock);
   exit(1);
 }
 // printf("client connect\n");

 // printf("start dup\n");
 // printf("%d\n",STDOUT_FILENO);

  int out = dup(STDOUT_FILENO);  //out = standard output

  dup2(client_sock, 1); // stdout = client_sock, out = stdout
  evaluate_command(1, &sc);
  close(client_sock);

  dup2(out,1); // out = client_sock = stdout
  close(STDOUT_FILENO);

  pthread_exit(NULL);
  exit(0);
}



static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;
    }
  }

  return -1; // Not found
}

/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  struct single_command* com = (*commands);

  assert(com->argc != 0);

  if (n_commands == 1) {

    int built_in_pos = is_built_in_command(com->argv[0]);
    if (built_in_pos != -1) {
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
        if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
          fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
        }
      } else {
        fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
        return -1;
      }
    } else if (strcmp(com->argv[0], "") == 0) {
      return 0;
    } else if (strcmp(com->argv[0], "exit") == 0) {
      return 1;
    } else {

      pid_t pid;
      int status;

      pid = fork();

      if(pid == 0){
        // printf("child!\n");
        path_res(com->argv[0],com->argv);
        fprintf(stderr, "%s: command not found\n", com->argv[0]);
        exit(1);
      }
      else if(pid != 0){
        // printf("parent\n");
        wait(&status);
        // printf("finish\n");
        return 0;
      }
    }
  }
  else if(n_commands >= 2){
    int server_sock, rc, len, client_sock;

    int backlog = 10;  

    struct sockaddr_un server_sockaddr;
    struct sockaddr_un client_sockaddr;

    memset(&server_sockaddr,0,sizeof(struct sockaddr_un));
    memset(&client_sockaddr,0,sizeof(struct sockaddr_un));

          //make server
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(server_sock == -1){
      printf("SOCKET ERROR\n");
      exit(1);
    }
    // printf("make server\n");

    server_sockaddr.sun_family = AF_UNIX;
    strcpy(server_sockaddr.sun_path, SERVER_PATH);
    len = sizeof(server_sockaddr);

    unlink(SERVER_PATH);
    rc = bind(server_sock, (struct sockaddr *) &server_sockaddr, len);
    if(rc == -1){
      printf("BIND ERROR\n");
      close(server_sock);
      exit(1);
    }
    // printf("server bind\n");

    rc = listen(server_sock, backlog);
    if(rc == -1){
      printf("LISTEN ERROR\n");
      close(server_sock);
      exit(1);
    }
    // printf("listening...\n");

    pthread_t thread[4];
    int t_status;

    rc = pthread_create(&thread[2], NULL, make_Client,(void*)com->argv);//
    // printf("createing thread\n");
          //last argument : a single argument that may be passed to start_routine
          //must void type, make_client's variable
    if(rc < 0){
      printf("CREATING THREAD ERROR\n");
      exit(1);
    }
    // printf("create thread\n");

    client_sock = accept(server_sock,(struct sockaddr*)&client_sockaddr, &len);
    if(client_sock == -1){
      printf("ACCEPT ERROR\n");
      close(server_sock);
      close(client_sock);
      exit(1);
    }
    // printf("accepting\n");

    pthread_join(thread[2], (void**)&t_status);

    pid_t pid2;
    int status2;

    int in = dup(STDIN_FILENO);

    pid2 = fork();

    if(pid2 == 0){

          // printf("(com+1)->argv : %s\n", (com+1)->argv[0]);
      // printf("receive data\n");
      dup2(client_sock, STDIN_FILENO); 
      close(client_sock);

      path_res((com+1)->argv[0],(com+1)->argv);
            // evaluate_command(1,(&com+1)); // second command?
      fprintf(stderr, "%s: command not found\n", (com+1)->argv[0]);
      dup2(in, STDIN_FILENO);
      close(client_sock);
      close(server_sock);
      close(STDIN_FILENO);
      exit(1);
    }
    else if(pid2 != 0){
      // printf("parent running\n");

      // pthread_cancel(thread[2]);

      close(client_sock);
      close(server_sock);
      wait(&status2);

      // printf("finish\n");
      return 0;
      // exit(1);
    } 
    dup2(in,STDIN_FILENO);
    close(STDIN_FILENO);    
    return 0;
  }
  return 0;
}

void path_res(char* argv0, char* argv1[]){
  char buf[4096];

  execv(argv0,argv1); //execution about absolute path
  
  //putenv("PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin/:/sbin");
  char* varname = getenv("PATH");
  strcpy(buf, varname);


  char *saveptr = NULL;
  char *tok = strtok_r(buf, ":", &saveptr);
  char res[1024];

  while (tok != NULL) {
   strcpy(res,tok);
   strcat(res,"/"); 
   strcat(res, argv0);
   // printf("%s\n",res);
   execv(res,argv1);

   tok = strtok_r(NULL, ":", &saveptr);
 }
}

void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }

    free(argv);
  }

  memset((*commands), 0, sizeof(struct single_command) * n_commands);
}
