#include <winsock2.h>
#include <iostream>
#include <thread>
#include <atomic>
#include "include/ViGEm/Client.h"
#include "Protocol.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "ViGEmClient.lib")

// --- 1. RAII Обертки для автоматического управления ресурсами ---
class ViGEmSession {
    PVIGEM_CLIENT client = nullptr;
    PVIGEM_TARGET pad = nullptr;
public:
    ViGEmSession() {
        client = vigem_alloc();
        if (!client || !VIGEM_SUCCESS(vigem_connect(client))) 
            throw std::runtime_error("ViGEm init failed");
        
        pad = vigem_target_x360_alloc();
        if (!VIGEM_SUCCESS(vigem_target_add(client, pad)))
            throw std::runtime_error("ViGEm target add failed");
    }

    ~ViGEmSession() {
        if (client) {
            if (pad) {
                vigem_target_remove(client, pad);
                vigem_target_free(pad);
            }
            vigem_disconnect(client);
            vigem_free(client);
        }
        std::cout << "[RAII] ViGEm resources released." << std::endl;
    }

    void update(XUSB_REPORT& report) { vigem_target_x360_update(client, pad, report); }
};

class WinsockSession {
    SOCKET udpSocket = INVALID_SOCKET;
public:
    WinsockSession(int port) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        bind(udpSocket, (sockaddr*)&addr, sizeof(addr));

        // Устанавливаем таймаут для recvfrom, чтобы поток мог проверять флаг остановки
        DWORD timeout = 500000; // 500ms
        setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    }

    ~WinsockSession() {
        if (udpSocket != INVALID_SOCKET) closesocket(udpSocket);
        WSACleanup();
        std::cout << "[RAII] Network resources released." << std::endl;
    }

    SOCKET getSocket() { return udpSocket; }
};

// --- 2. Логика работы программы ---
int main() {
    try {
        WinsockSession network(8888);
        ViGEmSession controller;
        std::atomic<bool> running{ true };
        

        // Поток для обработки сетевых данных
        std::thread networkThread([&]() {
            GamepadPacket packet;
            sockaddr_in clientAddr;
            int addrLen = sizeof(clientAddr);
            XUSB_REPORT report;
            bool is_first = true;

            std::cout << "Worker thread started..." << std::endl;

            while (running) {
                int bytes = recvfrom(network.getSocket(), (char*)&packet, sizeof(packet), 0, (sockaddr*)&clientAddr, &addrLen);
                if (is_first){
                    sendto(network.getSocket(), (const char *)&packet, sizeof(packet), 0, (const sockaddr*)&clientAddr, addrLen);
                    is_first = false;
                }

                if (bytes > 0) {
                    XUSB_REPORT_INIT(&report);
                    report.sThumbLX = (SHORT)packet.leftStickX;
                    report.sThumbLY = -(SHORT)packet.leftStickY;
                    report.sThumbRX = (SHORT)packet.rightStickX;
                    report.sThumbRY = -(SHORT)packet.rightStickY;
                    controller.update(report);
                }
            }
            std::cout << "Worker thread stopping..." << std::endl;
        });

        std::cout << "Server is RUNNING. Press ENTER to stop safely." << std::endl;
        std::cin.get(); // Ждем ввода от пользователя

        running = false;       // Сигнал потоку остановиться
        networkThread.join();  // Ждем завершения потока

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0; // Все деструкторы вызовутся автоматически здесь
}