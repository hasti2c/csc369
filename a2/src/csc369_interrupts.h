/**
 * CSC369 Assignment 2
 *
 * @file Defines the public interface for controlling "simulated interrupts".
 */
#ifndef CSC369_INTERRUPTS_H
#define CSC369_INTERRUPTS_H
#include <signal.h>
#include <stdio.h>

/**
 * How frequently this process will be interrupted.
 */
#define CSC369_INTERRUPTS_SIGNAL_INTERVAL 200

/**
 * Enum specifying the state of interrupts.
 */
typedef enum
{
  CSC369_INTERRUPTS_DISABLED = 0,
  CSC369_INTERRUPTS_ENABLED = 1,
} CSC369_InterruptsState;

/**
 * Enum specifying the verbosity of outputs (logging) produced by interrupts.
 */
typedef enum
{
  CSC369_INTERRUPTS_QUIET = 0,
  CSC369_INTERRUPTS_VERBOSE = 1,
} CSC369_InterruptsOutput;

/**
 * Initialize the CSC369 interrupt library.
 *
 * This must be called before using other functions in this header.
 *
 * @return 0 on success, -1 otherwise.
 */
void
CSC369_InterruptsInit(void);

/**
 * Set whether interrupts should be enabled or disabled.
 *
 * @return The state of interrupts before the call to this function.
 */
CSC369_InterruptsState
CSC369_InterruptsSet(CSC369_InterruptsState state);

/**
 * Enable interrupts.
 *
 * @return The state of interrupts before the call to this function.
 */
CSC369_InterruptsState
CSC369_InterruptsEnable(void);

/**
 * Disable interrupts.
 *
 * @return The state of interrupts before the call to this function.
 */
CSC369_InterruptsState
CSC369_InterruptsDisable(void);

/**
 * @return whether interrupts are enabled (1) or not (0).
 */
int
CSC369_InterruptsAreEnabled(void);

/**
 * Set the verbosity of logging.
 */
void
CSC369_InterruptsSetLogLevel(CSC369_InterruptsOutput level);

/**
 * Print to stdout safely (i.e., without being interrupted).
 *
 * This function can be called as if it were printf.
 */
int
CSC369_InterruptsPrintf(const char* fmt, ...);

#endif // CSC369_INTERRUPTS_H
