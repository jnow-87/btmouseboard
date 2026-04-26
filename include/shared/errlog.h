#ifndef ERRLOG_H
#define ERRLOG_H

// NOTE this header expects some log macros to be defined


/* macros */
#define goto_err(label, msg)	{ ERROR(msg); goto label; }


#endif // ERRLOG_H
