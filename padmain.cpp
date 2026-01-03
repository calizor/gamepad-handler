#include <winsock2.h>
#include <iostream>
#include "Protocol.h"

#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in serverAddr, clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Слушаем все интерфейсы
    serverAddr.sin_port = htons(8888);       // Порт должен совпадать с Android

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    std::cout << "waiting for mobile..." << std::endl;

    GamepadPacket packet;
    while (true) {
        int bytesReceived = recvfrom(serverSocket, (char*)&packet, sizeof(packet), 0, 
                                     (sockaddr*)&clientAddr, &clientAddrLen);
        if (bytesReceived > 0) {
            std::cout << "StickL X: " << packet.leftStickX << " StickL Y: " << packet.leftStickY << std::endl;
            std::cout << "StickR X: " << packet.rightStickX << " StickL Y: " << packet.rightStickY << std::endl;
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
