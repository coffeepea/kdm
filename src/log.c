#include <log.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>

void kdm_log_intrnl(FILE* out, const char* format, ...) {
  time_t t = time(NULL);
  struct tm* tm = localtime(&t);

  char time_str[64];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm);

  va_list args;
  va_start(args, format);

  fprintf(out, "%s - ", time_str);
  vfprintf(out, format, args);

  va_end(args);
}
