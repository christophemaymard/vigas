#ifndef _ERROR_H_
#define _ERROR_H_

/* Function prototypes */
void error_init(void);
void error_shutdown(void);
void error(const char *format, ...);

#endif /* _ERROR_H_ */

