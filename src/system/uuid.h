#pragma once
#include "Arduino.h"
#include "esp_system.h"
#define UUID_BYTE_LEN 16
#define UUID_STR_LEN 37

class UUID
{
public:
    static const String new_id()
    {
        uint8_t id[UUID_BYTE_LEN];
        /* generate idID bytes */
        esp_fill_random(id, UUID_BYTE_LEN);
        /* idid version */
        id[6] = 0x40 | (id[6] & 0xF);
        /* idid variant */
        id[8] = (0x80 | id[8]) & ~0x40;
        static char out[37];
        snprintf(out, UUID_STR_LEN,
                 "%02x%02x%02x%02x-%02x%02x-%02x%02x-"
                 "%02x%02x-%02x%02x%02x%02x%02x%02x",
                 id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7], id[8], id[9], id[10], id[11],
                 id[12], id[13], id[14], id[15]);

        return String(out);
    }

private:
    UUID() { }    
};