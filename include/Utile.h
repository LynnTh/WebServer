#ifndef UTILE_H
#define UTILE_H

int socket_bind_listen(int port);
void handle_for_sigpipe();
int setSocketNonBlocking(int fd);
void setSocketNodelay(int fd);

#endif