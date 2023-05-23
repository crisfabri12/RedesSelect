#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <errno.h>

#define SERVER_PORT 5400
#define MAX_CLIENTS 32

int main(int argc, char *argv[]) {
    int i, len, rc, on = 1;
    int listen_sd, max_sd, new_sd;
    int desc_ready, end_server = 0;
    int close_conn;
    char buffer[80];
    struct sockaddr_in6 addr;
    struct timeval timeout;
    fd_set master_set, working_set;

    // Crear un socket de flujo IPv6
    listen_sd = socket(AF_INET6, SOCK_STREAM, 0);
    if (listen_sd < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Permitir que el descriptor de socket sea reutilizable
    rc = setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
    if (rc < 0) {
        perror("setsockopt() failed");
        close(listen_sd);
        exit(EXIT_FAILURE);
    }

    // Establecer el socket en modo no bloqueante
    rc = ioctl(listen_sd, FIONBIO, (char *)&on);
    if (rc < 0) {
        perror("ioctl() failed");
        close(listen_sd);
        exit(EXIT_FAILURE);
    }

    // Vincular el socket a la dirección y puerto
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    memcpy(&addr.sin6_addr, &in6addr_any, sizeof(in6addr_any));
    addr.sin6_port = htons(SERVER_PORT);
    rc = bind(listen_sd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc < 0) {
        perror("bind() failed");
        close(listen_sd);
        exit(EXIT_FAILURE);
    }

    // Establecer el tamaño de la cola de escucha
    rc = listen(listen_sd, MAX_CLIENTS);
    if (rc < 0) {
        perror("listen() failed");
        close(listen_sd);
        exit(EXIT_FAILURE);
    }

    // Inicializar el conjunto de descriptores de archivo maestro
    FD_ZERO(&master_set);
    max_sd = listen_sd;
    FD_SET(listen_sd, &master_set);

    // Inicializar la estructura timeval a 3 minutos
    timeout.tv_sec = 3 * 60;
    timeout.tv_usec = 0;

    // Bucle principal para esperar conexiones entrantes o datos en los sockets conectados
    while (!end_server) {
        // Copiar el conjunto de descriptores de archivo maestro al conjunto de trabajo
        memcpy(&working_set, &master_set, sizeof(master_set));

        // Llamar a select() y esperar 3 minutos a que se complete
        printf("\n-----------------------------------------------------------------------\n");
        printf("Waiting on select()...\n");
        rc = select(max_sd + 1, &working_set, NULL, NULL, &timeout);

        // Verificar si hubo un error en select()
        if (rc < 0) {
            perror("select() failed");
            break;
        }

        // Si no hubo actividad en los sockets, continuamos esperando
        if (rc == 0) {
            printf("No activity on any socket. Waiting...\n");
            continue;
        }

        // Recorrer todos los descriptores de archivo para verificar si hay actividad en alguno de ellos
        for (i = 0; i <= max_sd; i++) {
            if (FD_ISSET(i, &working_set)) {
                // Verificar si hay actividad en el descriptor de archivo de escucha (nueva conexión entrante)
                if (i == listen_sd) {
                    printf("New connection incoming...\n");
                    // Aceptar la nueva conexión entrante
                    new_sd = accept(listen_sd, NULL, NULL);
                    if (new_sd < 0) {
                        perror("accept() failed");
                        end_server = 1;
                    } else {
                        FD_SET(new_sd, &master_set);
                        if (new_sd > max_sd) {
                            max_sd = new_sd;
                        }
                    }
                } else {
                    // Hay actividad en un descriptor de archivo conectado (datos recibidos)
                    printf("Receiving data from client...\n");
                    memset(buffer, 0, sizeof(buffer));
                    rc = recv(i, buffer, sizeof(buffer), 0);
                    if (rc < 0) {
                        perror("recv() failed");
                    } else if (rc == 0) {
                        printf("Client closed the connection\n");
                    } else {
                        printf("Received message from client: %s\n", buffer);
                        // Aquí puedes realizar cualquier otra acción necesaria con los datos recibidos
                    }
                }
            }
        }
    }

    // Cerrar todos los descriptores de archivo
    for (i = 0; i <= max_sd; i++) {
        if (FD_ISSET(i, &master_set)) {
            close(i);
        }
    }

    return 0;
}
