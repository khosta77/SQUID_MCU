#pragma once

#include <cstdint>

class KeyController {
public:
    static void setKey(uint8_t motorNum, bool state);
    static void clearAll();
    static bool isKeySet(uint8_t motorNum);
};
