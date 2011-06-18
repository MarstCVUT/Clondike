#ifndef DIRECTOR_MSGS_H
#define DIRECTOR_MSGS_H

extern const char* DIRECTOR_CHANNEL_NAME;

/* Message types */
enum {
  /* User space requests */
  DIRECTOR_REGISTER_PID, /* User helper daemon initially registers itself and its PID to the kernel */
  DIRECTOR_SEND_GENERIC_USER_MESSAGE, /* Informs requests transmission of a generic user message to peer */
  /* Generic ack */
  DIRECTOR_ACK,

  /* Commands */
  DIRECTOR_CHECK_NPM, /* Checks if a non-preemptive migration should be performed */
  DIRECTOR_CHECK_FULL_NPM, /* Checks if a non-preemptive migration should be performed (pass in args&envp) */
  DIRECTOR_NODE_CONNECTED, /* Informs director, that DN has successfuly connected to CN */
  DIRECTOR_NODE_DISCONNECTED, /* Informs director that a peer node connection was closed */
  DIRECTOR_IMMIGRATION_REQUEST, /* Informs director, some associated CN wants to immigrate task to this node */
  DIRECTOR_IMMIGRATION_CONFIRMED, /* Informs director about successful immigration */
  DIRECTOR_TASK_EXIT, /* Informs, that a task has finished */
  DIRECTOR_TASK_FORK, /* Informs, that a task has forked */
  DIRECTOR_GENERIC_USER_MESSAGE, /* Informs about newly arrived generic user message */
  DIRECTOR_EMIGRATION_FAILED, /* Informs director about failed emigration request (could be npm or ppm, director should know which one based on provided pid) */
  DIRECTOR_MIGRATED_HOME, /* Informs director that a task was migrated home */

  /* Responses */
  DIRECTOR_NPM_RESPONSE, /* Response on non-preemptive migration check */
  DIRECTOR_NODE_CONNECT_RESPONSE, /* Informs director, that a DN connected to current CCN */
  DIRECTOR_IMMIGRATION_REQUEST_RESPONSE, /* Accept/reject respons on immigration request */

  __DIRECTOR_MSG_MAX
};

#define DIRECTOR_MSG_MAX (__DIRECTOR_MSG_MAX - 1)

/* Message attributes */
enum {
  DIRECTOR_A_PID, /* process pid */
  DIRECTOR_A_REMOTE_PID, /* remote pid */
  DIRECTOR_A_PPID, /* process parent pid */
  DIRECTOR_A_UID, /* user id */
  DIRECTOR_A_TASK_TYPE, /* Task type (1=guest/0=shadow)*/
  DIRECTOR_A_NAME, /* Name of anything */
  DIRECTOR_A_LENGTH, /* 32 bit length */
  DIRECTOR_A_INDEX, /* 32 bit index */
  DIRECTOR_A_SLOT_INDEX, /* 32 bit index */
  DIRECTOR_A_SLOT_TYPE, /* 32 bit (1=core, 0=detached) */
  DIRECTOR_A_ADDRESS, /* Address of remote end point in a protocol specific format */
  DIRECTOR_A_AUTH_DATA,
  DIRECTOR_A_USER_DATA, /* Generic user data */
  DIRECTOR_A_ARGS,
  DIRECTOR_A_ARG,
  DIRECTOR_A_ENVS,
  DIRECTOR_A_ENV,
  DIRECTOR_A_REASON, /* 32 bit length */  
  DIRECTOR_A_DECISION, /* 32 bit length */  
  DIRECTOR_A_DECISION_VALUE, /* 32 bit length */  
  DIRECTOR_A_EXIT_CODE, /* 32 bit length */  
  DIRECTOR_A_ERRNO, /* error code, in case some error occured */
  DIRECTOR_A_RUSAGE,

  __DIRECTOR_ATTR_MAX
};

#define DIRECTOR_ATTR_MAX __DIRECTOR_ATTR_MAX

#endif
