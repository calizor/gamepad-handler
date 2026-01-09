#include <winsock2.h>
#include <iostream>
#include "Protocol.h"

#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Создаем ОДИН сокет для приема и отправки
    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; 
    serverAddr.sin_port = htons(8888);

    bind(udpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    std::cout << "Waiting for mobile..." << std::endl;

    GamepadPacket packet;
    sockaddr_in clientAddr; // Сюда запишется адрес того, кто прислал пакет
    int clientAddrLen = sizeof(clientAddr);

    bool confirmed = false;
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    hostent* host = gethostbyname(hostname);
    char* localIP = inet_ntoa(*(in_addr*)*host->h_addr_list);

    std::cout << "My Server IP is: " << localIP << std::endl;

    while (true) {
        int bytesReceived = recvfrom(udpSocket, (char*)&packet, sizeof(packet), 0, 
                                     (sockaddr*)&clientAddr, &clientAddrLen);
        
        if (bytesReceived > 0) {
            // 1. Если это первое сообщение — отвечаем клиенту
            if (!confirmed) {
                // В качестве "ответа с IP" можно отправить пустой пакет или структуру.
                // Клиент и так увидит твой IP, когда получит пакет.
                const char* reply = "ACK"; 
                sendto(udpSocket, localIP, strlen(localIP), 0, (sockaddr*)&clientAddr, clientAddrLen);
                
                std::cout << "Client connected. Response sent.  " << inet_ntoa(clientAddr.sin_addr) << std::endl;
                confirmed = true; 
            }

            // 2. Обработка данных геймпада
            std::cout << "StickL X: " << packet.leftStickX << " StickL Y: " << packet.leftStickY << std::endl;
            std::cout << "StickR X: " << packet.rightStickX << " StickR Y: " << packet.rightStickY << std::endl;
        }
    }

    closesocket(udpSocket);
    WSACleanup();
    return 0;
}