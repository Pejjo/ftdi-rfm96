#ifndef debugh
#define debugh

/* Uncomment the #define INFRA_DEBUG_ENABLE in makefile to enable debug messages */
#define MSG_EMERG       0 /*Used for emergency messages, usually those that precede a crash*/
#define MSG_ALERT       1 /*A situation requiring immediate action */
#define MSG_CRIT        2 /*Critical conditions, often related to serious hw/sw failures */
#define MSG_ERR         3 /*Used to report error conditions */
#define MSG_WARN        4 /*Warnings about problematic situations that do not, in themselves,
                                                create serious problems with the system */
#define MSG_NOTICE      5 /*Situations that are normal, but still worthy of note. */
#define MSG_INFO        6 /*Informational messages */
#define MSG_DEBUG       7 /*Used for debugging messages */


/******************************************************************************/
/*                                                              Global variables                                                          */
/******************************************************************************/
#ifdef DEBUG_ENABLE
	int currentDebugLevel =  MSG_NOTICE;
//        int currentDebugLevel = MSG_INFO;
//        int     currentDebugLevel = MSG_DEBUG;
#endif


#ifdef DEBUG_ENABLE
        #define DBG(level,...) do { if (level <= currentDebugLevel){  \
                printf("%d: %s: %d: %s(): ",level, __FILE__, __LINE__, __FUNCTION__); \
                printf(__VA_ARGS__); }} while (0)
#else
        #define DBG(level,fmt,...)      ;
#endif

#endif

