#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "built_in.h"
#include "utils.h"

#include "signal_handlers.h"

#define SIGINT 2
#define SIGSTP 20

int main()
{
  char buf[8096];
  putenv("PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin");
 
  catch_sigint(SIGINT);
  catch_sigstp(SIGSTP);

  while (1) {
    //catch_sigint(SIGINT);
    fgets(buf, 8096, stdin);

    struct single_command commands[512];
    int n_commands = 0;
    mysh_parse_command(buf, &n_commands, &commands);

    int ret = evaluate_command(n_commands, &commands);

    free_commands(n_commands, &commands);
    n_commands = 0;

    if (ret == 1) {
      break;
    }
  }

  return 0;
}
