#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>

/** 初始化信号处理器，注册SIGINT、SIGTERM、SIGQUIT等信号处理 */
void signal_handler_init(void* server_instance);


#endif /* SIGNAL_HANDLER_H */