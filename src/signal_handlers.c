#include "signal_handlers.h"

#include <signal.h>

void catch_sigint(int signalNo)
{
  // TODO: File this!
	// signal(signalNo,SIG_DFL);
	signal(signalNo,SIG_IGN);
}

void catch_sigtstp(int signalNo)
{
  // TODO: File this!
}

