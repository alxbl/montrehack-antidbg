#include "common.h"
#include "crypto.h"
#ifdef _DEBUG
const char* plaintext[] = {
    "FLAG-{n0pping_1n_th3_90s}       ",
    "FLAG-{h3aps_of_l1ez_4wa1t}      ",
    "FLAG-{U_k4nT_brk_m3_50_3aZy}    ",
    "FLAG-{G3tT1nG_5eri0us_n0w}      ",
    "FLAG-{sUbv3rt_th3_KrNlz}        ",
};
const uint64_t raw_keys[]{ KEY1, KEY2, KEY3, KEY4, KEY5 };


// This code is used to encrypt the plaintext (padded to 32 byte strings) and validate the raw_keys.
void encrypt_flags() {
    printf("Encrypted plaintext:\n");
    for (auto i = 0; i < (sizeof(raw_keys) / sizeof(raw_keys[0])); ++i) {
        auto key = md5((byte_t*)& raw_keys[i], 8);
        
        auto c = encrypt(key, (byte_t*)plaintext[i], LEN);
        printf("\"");
        for (auto j = 0; j < c->size; ++j) printf("\\x%02x", c->data[j]);
        printf("\", // ");
        printf("%d: %s | KEY=%016Ix\n", i, plaintext[i], raw_keys[i]);
    }

    printf("`flags` Decryption validation:\n");
    for (auto i = 0; i < (sizeof(raw_keys) / sizeof(raw_keys[0])); ++i) {
        auto key = md5((byte_t*)& raw_keys[i], 8);
        auto c = decrypt(key, (byte_t*)flags[i], LEN);
        printf("\"");
        for (auto j = 0; j < LEN - 8; ++j) printf("%c", c->data[j]);
        printf("\",\n");
    }
}

#endif