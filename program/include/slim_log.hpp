#ifndef __SLIM__LOG__HPP
#define __SLIM__LOG__HPP
#include <iostream>
#include <sstream>
#include <initializer_list>
#include <cstdarg>
#include <syslog.h>
#include <uv.h>
namespace slim::log::console
{
    
};
namespace slim::log::system {
    uv_loop_t *log_loop;
    void init(uv_loop_t *loop) {
        log_loop = loop;
    }
    class _base_log_info {
        public:
            uv_work_t request;
            int priority;
            int options = LOG_CONS | LOG_PID;
            std::string message;
            std::string program = "slim";
            int facility = LOG_LOCAL7;
            template<class... Args>
            _base_log_info(const char* level, Args... args) {
                fold_message_args(level, args...);
            }
            ~_base_log_info() {
                std::cout << "destructing\n";
            }
        private:
            template<class... Args>
            void fold_message_args(const char* level, Args... args) {
                std::stringstream messagestream;
                messagestream << level << ": ";
                (messagestream << ... << std::forward<Args>(args));
                message = messagestream.str();
            }
    };
    void write_syslog(uv_work_t* request) {
        _base_log_info *log_info = (_base_log_info*) request->data;
        setlogmask (LOG_UPTO (LOG_DEBUG));
        openlog(log_info->program.c_str(), log_info->options, log_info->facility);
        syslog(log_info->priority, "%s", log_info->message.c_str());
    }
    void done_write_syslog(uv_work_t* request, int status) {
        _base_log_info *log_info = (_base_log_info*) request->data;
        log_info->~_base_log_info();
        delete request;
    }
    struct critical {
        template<class... Args>
        critical(Args... args) {
            _base_log_info* log_info = new _base_log_info("CRITICAL", args...);
            log_info->request.data = (void*) log_info;
            log_info->priority = LOG_CRIT;
            log_info->options = log_info->options | LOG_PERROR;
            uv_queue_work(log_loop, &log_info->request, write_syslog, done_write_syslog);
        }
    };
    struct debug {
        template<class... Args>
        debug(Args... args) {
            _base_log_info* log_info = new _base_log_info("DEBUG", args...);
            log_info->request.data = (void*) log_info;
            log_info->priority = LOG_DEBUG;
            uv_queue_work(log_loop, &log_info->request, write_syslog, done_write_syslog);
        }
    };
    struct error {
        template<class... Args>
        error(Args... args) {
            _base_log_info* log_info = new _base_log_info("ERROR", args...);
            log_info->request.data = (void*) log_info;
            log_info->priority = LOG_ERR;
            uv_queue_work(log_loop, &log_info->request, write_syslog, done_write_syslog);
        }
    };
    struct info {
        template<class... Args>
        info(Args... args) {
            _base_log_info* log_info = new _base_log_info("INFO", args...);
            log_info->request.data = (void*) log_info;
            log_info->priority = LOG_INFO;
            uv_queue_work(log_loop, &log_info->request, write_syslog, done_write_syslog);
        }
    };
    struct notice {
        template<class... Args>
        notice(Args... args) {
            _base_log_info* log_info = new _base_log_info("NOTICE", args...);
            log_info->request.data = (void*) log_info;
            log_info->priority = LOG_NOTICE;
            log_info->options = log_info->options | LOG_PERROR;
            uv_queue_work(log_loop, &log_info->request, write_syslog, done_write_syslog);
        }
    };
    struct warn {
        template<class... Args>
        warn(Args... args) {
            _base_log_info* log_info = new _base_log_info("WARN", args...);
            log_info->request.data = (void*) log_info;
            log_info->priority = LOG_WARNING;
            uv_queue_work(log_loop, &log_info->request, write_syslog, done_write_syslog);
        }
    };
    void handle_libuv_error(const char* message, int error) {
        if(error) {
            critical(message, uv_strerror(error));
            exit(error);
        }
    }
};
#endif