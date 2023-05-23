#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define SERVER_IP "127.0.0.1"  // Dirección IP del servidor
#define SERVER_PORT 5400      // Puerto del servidor

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[80];

    // Crear el socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr)) <= 0) {
        perror("Error al configurar la dirección del servidor");
        exit(EXIT_FAILURE);
    }

    // Conectar al servidor
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al conectar con el servidor");
        exit(EXIT_FAILURE);
    }

    // Enviar datos al servidor
    strcpy(buffer, "Hola servidor");
    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
        perror("Error al enviar datos al servidor");
        exit(EXIT_FAILURE);
    }

    // Recibir respuesta del servidor
    memset(buffer, 0, sizeof(buffer));
    if (recv(sockfd, buffer, sizeof(buffer), 0) < 0) {
        perror("Error al recibsir datos del servidor");
        exit(EXIT_FAILURE);
    }

    printf("Respuesta del servidor: %s\n", buffer);

    // Cerrar el socket
    close(sockfd);

    return 0;
}
