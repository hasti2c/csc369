#include <assert.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/time.h>

#include "csc369_interrupts.h"
#include "csc369_thread.h"

#define UNUSED(x) (void)(x)

// The type of signal to use for delivering "interrupts"
#define CSC369_INTERRUPTS_SIGNAL_TYPE SIGALRM

// Whether we should log debugging information to stdout
int interrupts_log_level = CSC369_INTERRUPTS_QUIET;

/**
 * Ask the operating system to set an alarm for some time (i.e., SIG_INTERVAL)
 * in the future.
 */
void
ScheduleAlarmSignal(void)
{
  struct itimerval val;
  val.it_interval.tv_sec = 0;
  val.it_interval.tv_usec = 0;
  val.it_value.tv_sec = 0;
  val.it_value.tv_usec = CSC369_INTERRUPTS_SIGNAL_INTERVAL;

  int ret = setitimer(ITIMER_REAL, &val, NULL);
  assert(!ret);
}

/**
 * Handle a signal from the operating system.
 */
void
HandleSignal(int sig, siginfo_t* sip, void* contextVP)
{
  UNUSED(sig);
  UNUSED(sip);
  assert(!CSC369_InterruptsAreEnabled());

  static int first = 1;
  static struct timeval start, end, diff = { 0, 0 };

  if (interrupts_log_level) {
    int ret = gettimeofday(&end, NULL);
    assert(!ret);

    if (first) {
      first = 0;
    } else {
      timersub(&end, &start, &diff);
    }

    ucontext_t* context = (ucontext_t*)contextVP;
    start = end;
    printf("%s: context at %10p, time diff = %ld us\n",
           __func__,
           (void*)context,
           diff.tv_sec * 1000000 + diff.tv_usec);
  }

  // Set up the next interrupt
  ScheduleAlarmSignal();
  // Yield to "preempt" the current thread and switch to another
  CSC369_ThreadYield();
}

void
CSC369_InterruptsInit(void)
{
  // Ensure this function is only called once
  static int init = 0;
  assert(!init);
  init = 1;

  struct sigaction action;
  action.sa_handler = NULL;
  action.sa_sigaction = HandleSignal;

  // Block alarm signals while the interrupt handler is running. This will avoid
  // recursive interrupts where an interrupt occurs before the previous
  // interrupt handler has finished running.
  int error = sigemptyset(&action.sa_mask);
  assert(!error);

  // Use sa_sigaction as handler instead of sa_handler
  action.sa_flags = SA_SIGINFO;
  if (sigaction(CSC369_INTERRUPTS_SIGNAL_TYPE, &action, NULL)) {
    perror("Setting up signal handler");
    assert(0);
  }
  ScheduleAlarmSignal();
}

CSC369_InterruptsState
CSC369_InterruptsSet(CSC369_InterruptsState state)
{
  sigset_t mask, omask;

  // Create a signal set with only CSC369_INTERRUPTS_SIGNAL_TYPE
  int ret = sigemptyset(&mask);
  assert(!ret);
  ret = sigaddset(&mask, CSC369_INTERRUPTS_SIGNAL_TYPE);
  assert(!ret);

  // Based on state, block or unblock signals of CSC369_INTERRUPTS_SIGNAL_TYPE
  if (state) {
    ret = sigprocmask(SIG_UNBLOCK, &mask, &omask);
  } else {
    ret = sigprocmask(SIG_BLOCK, &mask, &omask);
  }
  assert(!ret);
  return (sigismember(&omask, CSC369_INTERRUPTS_SIGNAL_TYPE) ? 0 : 1);
}

CSC369_InterruptsState
CSC369_InterruptsEnable(void)
{
  return CSC369_InterruptsSet(CSC369_INTERRUPTS_ENABLED);
}

CSC369_InterruptsState
CSC369_InterruptsDisable(void)
{
  return CSC369_InterruptsSet(CSC369_INTERRUPTS_DISABLED);
}

int
CSC369_InterruptsAreEnabled(void)
{
  sigset_t mask;
  int ret = sigprocmask(0, NULL, &mask);
  assert(!ret);
  return (sigismember(&mask, CSC369_INTERRUPTS_SIGNAL_TYPE) ? 0 : 1);
}

void
CSC369_InterruptsSetLogLevel(CSC369_InterruptsOutput level)
{
  interrupts_log_level = level;
}

int
CSC369_InterruptsPrintf(const char* fmt, ...)
{
  int const prev_state = CSC369_InterruptsDisable();

  va_list args;
  va_start(args, fmt);
  int ret = vprintf(fmt, args);
  va_end(args);

  CSC369_InterruptsSet(prev_state);
  return ret;
}
