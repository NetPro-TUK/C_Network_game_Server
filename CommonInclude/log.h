#ifndef LOG_H
#define LOG_H

#include <stdio.h>  // printf 함수
#include <time.h>   // time, localtime 관련 함수

    // 로그 레벨 정의
    // LOG_LEVEL_DEBUG: 디버그 메시지
    // LOG_LEVEL_INFO: 정보성 메시지
    // LOG_LEVEL_WARN: 경고 메시지
    // LOG_LEVEL_ERROR: 오류 메시지
    typedef enum {
        LOG_LEVEL_DEBUG,
        LOG_LEVEL_INFO,
        LOG_LEVEL_WARN,
        LOG_LEVEL_ERROR
    } LogLevel;

    // 로그 레벨을 문자열로 변환하는 함수
    static const char* log_level_name(LogLevel level) {
        switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        default:              return "ERROR";
        }
    }

    // 로그 메시지 출력 함수
    static inline void log_message(LogLevel level, const char* msg, const char* file, int line) {
        time_t t = time(NULL);
        struct tm tm_info;
        localtime_s(&tm_info, &t);


        // 시간 포맷팅: YYYY-MM-DD HH:MM:SS
        char time_buf[20];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);

        // 로그 출력: [시간][LEVEL][파일:라인] 메시지
        printf("[%s][%s][%s:%d] \n%s\n",
            time_buf,
            log_level_name(level),
            file,
            line,
            msg);
    }

    // 편의 매크로 정의
#define LOG_DEBUG(msg) log_message(LOG_LEVEL_DEBUG, msg, __FILE__, __LINE__)
#define LOG_INFO(msg)  log_message(LOG_LEVEL_INFO,  msg, __FILE__, __LINE__)
#define LOG_WARN(msg)  log_message(LOG_LEVEL_WARN,  msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) log_message(LOG_LEVEL_ERROR, msg, __FILE__, __LINE__)
#endif /* LOG_H */