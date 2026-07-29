#ifndef WEB_SERVER_H
#define WEB_SERVER_H

// 启动用于上传电子书的轻量级 Web 服务器
void start_webserver(void);

// 初始化 SPIFFS 文件系统
void spiffs_init(void);

#endif // WEB_SERVER_H