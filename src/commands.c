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

// export PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin/:/sbin;
//char PATH[4096]="/usr/local/bin:/usr/bin:/bin:/usr/sbin/:/sbin";

//ing
#define CLIENT_PATH "tpf_unix_sock.client"
#define SERVER_PATH "tpf_unix_sock.server"

struct sockaddr_un server_sockaddr;
struct sockaddr_un client_sockaddr;


void* make_Client(void* threadid){ // threadid : create()'s last arguemnt'
  int client_sock, rc, len;
  int num = 0;
  char** commands = (char**)threadid;

  while(commands != NULL)
    num++;

  struct single_comand* sc = (char**)threadid + num;
  //struct single_command* com = *commands + ti;
  // sc->argc = num;
  // sc->argv = (char**)threadid;
  

  memset(&server_sockaddr,0,sizeof(struct sockaddr_un));
  memset(&client_sockaddr,0,sizeof(struct sockaddr_un));

  //make client socket
  client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if(client_sock == -1){
    printf("SOCKET ERROR\n");
    exit(1);
  }

  client_sockaddr.sun_family = AF_UNIX;
  strcpy(client_sockaddr.sun_path, CLIENT_PATH);
  len = sizeof(client_sockaddr);

  unlink(CLIENT_PATH);
  rc = bind(client_sock, (struct sockaddr *) &client_sockaddr, len);
  if(rc == -1){
    printf("BIND ERROR\n");
    close(client_sock);
    exit(1);
  }

  server_sockaddr.sun_family = AF_UNIX;
  strcpy(server_sockaddr.sun_path, SERVER_PATH);
  rc = connect(client_sock, (struct sockaddr*) &server_sockaddr,len);
  if(rc == -1){
    printf("CONNECT ERROR\n");
    close(client_sock);
    exit(1);
  }

  int out = dup(stdout);  //out = standard output

  dup2(client_sock, stdout); // stdout = client_sock, out = stdout
  evaluate_command(1, sc);
  close(client_sock);

  dup2(out,stdout); // out = client_sock = stdout
  close(client_sock);
  // dup2(client_sock, stdout);
  // close(client_sock);

  pthread_exit(NULL);
}
//


static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands 
  = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;
    }
  }

  return -1; // Not found
}

/*
 * Description: 
 Currently this function only handles single built_in commands.
  You should modify this structure to launch process 
  and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  if (n_commands > 0) {
    struct single_command* com = (*commands);

    assert(com->argc != 0);

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
      char* res;

      printf("%d\n",n_commands);
      
      pid = fork();
      

      if(pid == 0){    //child
        putenv("PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin");
        if(n_commands == 1)
          path_res(com->argv[0],com->argv);

        //ing
        else if(n_commands == 2){
          //make server socket
          int server_sock, rc, len, client_sock;
          int backlog = 10; 

          struct sockaddr_un server_sockaddr;
          struct sockaddr_un client_sockaddr;

          memset(&server_sockaddr,0,sizeof(struct sockaddr_un));
          memset(&client_sockaddr,0,sizeof(struct sockaddr_un));

          server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
          if(server_sock == -1){
            printf("SOCKET ERROR\n");
            exit(1);
          }

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

          rc = listen(server_sock, backlog);
          if(rc == -1){
            printf("LISTEN ERROR\n");
            close(server_sock);
            exit(1);
          }
          printf("listening...\n");

          // client_sock = accept(server_sock,(struct sockaddr*)&client_sockaddr, &len);
          // if(client_sock == -1){
          //   printf("ACCEPT ERROR: %d\n", sock_errno());
          //   close(server_sock);
          //   close(clent_sock);
          //   exit(1);
          // }

          pthread_t thread;
          int status;

          rc = pthread_create(&thread, NULL, make_Client,(void*)com->argv);//
          printf("createing thread\n");
          //last argument : a single argument that may be passed to start_routine
          //must void type, make_client's variable
          if(rc < 0){
            printf("CREATING THREAD ERROR\n");
            exit(-1);
          }

          client_sock = accept(server_sock,(struct sockaddr*)&client_sockaddr, &len);
          printf("accepting\n");
          if(client_sock == -1){
            printf("ACCEPT ERROR\n");
            close(server_sock);
            close(client_sock);
            exit(1);
          }

          pthread_join(thread, (void**)&status);

          int in = dup(stdin);

          dup2(client_sock, stdin);
          path_res(com->argv[0],com->argv);
          close(client_sock);
          dup2(in, stdin);

          // int out = dup(stdout);  //out = standard output

          // dup2(client_sock, stdout); // stdout = client_sock, out = stdout
          // evaluate_command(1, sc);
          // close(client_sock);

          // dup2(out,stdout); // out = client_sock = stdout
          // close(client_sock);
        }
        //

        //execv(res, com->argv);
        //execv(com->argv[0],com->argv);
        fprintf(stderr, "%s: command not found\n", com->argv[0]);
        printf("child running\n");
        exit(1);
      }
      else if(pid!=0){   //parent
        // int status;
        
        printf("parent running\n");
        wait(&status);
        printf("finish\n");
        
        // putenv("PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin/:/sbin");
        // char* varname = getenv("PATH");
        // printf("%s\n",varname);
        return 0;
      }
    }
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
     printf("%s\n",res);
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
