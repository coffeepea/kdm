#ifndef KDM_LOG_H
#define KDM_LOG_H

#define log_info(fmt, ...)  kdm_log_intrnl(stdout, "[INFO]" fmt __VA_OPT__(,) __VA_ARGS__)
#define log_warn(fmt, ...)  kdm_log_intrnl(stdout, "[WARN]" fmt __VA_OPT__(,) __VA_ARGS__)
#define log_error(fmt, ...) kdm_log_intrnl(stderr, "[ERROR]" fmt __VA_OPT__(,) __VA_ARGS__)

void kdm_log_intrnl(FILE* out, const char* format, ...);

#endif
