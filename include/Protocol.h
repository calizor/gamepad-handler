#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>

#pragma pack(push, 1)
struct GamepadPacket {
    int16_t leftStickX;
    int16_t leftStickY;
    int16_t rightStickX; // Добавлено
    int16_t rightStickY; // Добавлено
    uint16_t buttons;
};
#pragma pack(pop)

#endif