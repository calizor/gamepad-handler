#include <winsock2.h>
#include <iostream>
#include "Protocol.h"
#include <windows.h> // Важно добавить перед ViGEm для корректных типов данных
#include "include/ViGEm/Client.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "ViGEmClient.lib") // Линк библиотеки

int main() {
    // --- 1. Инициализация ViGEm ---
    const auto client = vigem_alloc();
    if (client == nullptr) {
        std::cerr << "Uh, not enough memory to do that?!" << std::endl;
        return -1;
    }

    const auto retval = vigem_connect(client);
    if (!VIGEM_SUCCESS(retval)) {
        std::cerr << "ViGEm Bus connection failed with error code: 0x" << std::hex << retval << std::endl;
        return -1;
    }

    // Создаем цель (виртуальный Xbox 360)
    const auto pad = vigem_target_x360_alloc();
    const auto add_res = vigem_target_add(client, pad);
    if (!VIGEM_SUCCESS(add_res)) {
        std::cerr << "Target add failed: 0x" << std::hex << add_res << std::endl;
        return -1;
    }

    std::cout << "Virtual Xbox 360 Controller connected!" << std::endl;

    // --- 2. Инициализация Сети ---
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; 
    serverAddr.sin_port = htons(8888);

    bind(udpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    std::cout << "Waiting for mobile data on port 8888..." << std::endl;

    GamepadPacket packet;
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    bool confirmed = false;

    // Структура отчета геймпада
    XUSB_REPORT report;

    while (true) {
        int bytesReceived = recvfrom(udpSocket, (char*)&packet, sizeof(packet), 0, 
                                     (sockaddr*)&clientAddr, &clientAddrLen);
        
        if (bytesReceived > 0) {
            if (!confirmed) {
                // Отправляем подтверждение (IP сервера)
                const char* localIP = "127.0.0.1"; // Для простоты, или используй свой код получения IP
                sendto(udpSocket, localIP, (int)strlen(localIP), 0, (sockaddr*)&clientAddr, clientAddrLen);
                std::cout << "Client connected: " << inet_ntoa(clientAddr.sin_addr) << std::endl;
                confirmed = true; 
            }

            // --- 3. Передача данных в виртуальный геймпад ---
            XUSB_REPORT_INIT(&report);

            /* ВАЖНО: ViGEm ожидает SHORT (от -32768 до 32767).
               Если твой пакет шлет float (0.0 - 1.0) или байты (0-255), 
               тебе нужно их масштабировать. 
               Ниже пример, если в пакете уже приходят SHORT.
            */
            
            report.sThumbLX = (SHORT)packet.leftStickX;
            report.sThumbLY = -(SHORT)packet.leftStickY;
            report.sThumbRX = (SHORT)packet.rightStickX;
            report.sThumbRY = -(SHORT)packet.rightStickY;

            // Если в протоколе есть кнопки, они добавляются так:
            // if (packet.buttonA) report.wButtons |= XUSB_GAMEPAD_A;

            vigem_target_x360_update(client, pad, report);
        }
    }

    // --- 4. Очистка ресурсов ---
    vigem_target_remove(client, pad);
    vigem_target_free(pad);
    vigem_disconnect(client);
    vigem_free(client);

    closesocket(udpSocket);
    WSACleanup();
    return 0;
}