#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static const char *logFile = "/home/linaro/share/exa_log.txt";

static FILE *fpLog = NULL;

void OpenLog()
{
    if(fpLog == NULL)
        fpLog = fopen(logFile, "w");
}

void CloseLog()
{
    if(fpLog)
        fclose(fpLog);
    fpLog = NULL;
}

void LogString(const char *str)
{
    if(fpLog == NULL)
        return;

    if(str == NULL || str[0] == 0)
        return;

    if(strlen(str) > 1024*10)
        return;

    fwrite(str, 1, strlen(str), fpLog);

    fflush(fpLog);
}

static char tmp[1024*10];
void LogText(const char *fmt, ...)
{
    va_list args;

    if(fpLog == NULL)
        return;

    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end (args);

    LogString(tmp);
}

