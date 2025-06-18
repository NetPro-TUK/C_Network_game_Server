#ifndef LOG_H
#define LOG_H

#include <stdio.h>  // printf �Լ�
#include <time.h>   // time, localtime ���� �Լ�

    // �α� ���� ����
    // LOG_LEVEL_DEBUG: ����� �޽���
    // LOG_LEVEL_INFO: ������ �޽���
    // LOG_LEVEL_WARN: ��� �޽���
    // LOG_LEVEL_ERROR: ���� �޽���
    typedef enum {
        LOG_LEVEL_DEBUG,
        LOG_LEVEL_INFO,
        LOG_LEVEL_WARN,
        LOG_LEVEL_ERROR
    } LogLevel;

    // �α� ������ ���ڿ��� ��ȯ�ϴ� �Լ�
    static const char* log_level_name(LogLevel level) {
        switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        default:              return "ERROR";
        }
    }

    // �α� �޽��� ��� �Լ�
    static inline void log_message(LogLevel level, const char* msg, const char* file, int line) {
        time_t t = time(NULL);
        struct tm tm_info;
        localtime_s(&tm_info, &t);


        // �ð� ������: YYYY-MM-DD HH:MM:SS
        char time_buf[20];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);

        // �α� ���: [�ð�][LEVEL][����:����] �޽���
        printf("[%s][%s][%s:%d] \n%s\n",
            time_buf,
            log_level_name(level),
            file,
            line,
            msg);
    }

    // ���� ��ũ�� ����
#define LOG_DEBUG(msg) log_message(LOG_LEVEL_DEBUG, msg, __FILE__, __LINE__)
#define LOG_INFO(msg)  log_message(LOG_LEVEL_INFO,  msg, __FILE__, __LINE__)
#define LOG_WARN(msg)  log_message(LOG_LEVEL_WARN,  msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) log_message(LOG_LEVEL_ERROR, msg, __FILE__, __LINE__)
#endif /* LOG_H */