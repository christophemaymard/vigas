/*
    error.c --
    Error logging 
*/

#ifdef LOGERROR

#include <stdio.h>
#include <stdarg.h>

#include "build/cmd_sdl2/error.h"
#include "build/cmd_sdl2/main.h"

static FILE *error_log;
#endif

void error_init(void)
{
#ifdef LOGERROR
  error_log = fopen("error.log","w");
#endif
}

void error_shutdown(void)
{
#ifdef LOGERROR
  if(error_log) fclose(error_log);
#endif
}

void error(const char *format, ...)
{
#ifdef LOGERROR
  if (log_error)
  {
    va_list ap;
    va_start(ap, format);
    if(error_log) vfprintf(error_log, format, ap);
    va_end(ap);
  }
#endif
}
