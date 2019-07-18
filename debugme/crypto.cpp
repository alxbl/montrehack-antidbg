#include "crypto.h"
#include "common.h"
#include <Wincrypt.h>

#define array_len(arr) (sizeof(arr) / sizeof((arr)[0]))


buf_t* balloc(size_t sz) {
    auto out = new buf_t();
    out->data = new byte_t[sz];
    out->size = sz;
    return out;
}

void bfree(buf_t* buf) {
    memset(buf->data, 0, buf->size);
    delete[] buf->data;
    delete buf;
}

buf_t* md5(byte_t* b, size_t len)
{
    // This leaks memory, but for the challenge, it doesn't matter.
    auto out = balloc(16);

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
    CryptHashData(hHash, b, (DWORD)len, 0);
    CryptGetHashParam(hHash, HP_HASHVAL, out->data, (DWORD*)&out->size, 0);
    // Cleanup
    CryptReleaseContext(hProv, 0);
    CryptDestroyHash(hHash);

    return out;
}

// https://github.com/tarequeh/DES/blob/master/des.h
#define ENCRYPTION_MODE 1
#define DECRYPTION_MODE 0

#pragma pack(push, 1)
typedef struct {
    unsigned char k[8];
    unsigned char c[4];
    unsigned char d[4];
} key_set;
#pragma pack(pop)

void generate_key(unsigned char* key);
void generate_sub_keys(unsigned char* main_key, key_set* key_sets);
void process_message(unsigned char* message_piece, unsigned char* processed_piece, key_set* key_sets, int mode);

// Code adapted from https://github.com/tarequeh/DES/blob/master/run_des.c
buf_t* des(byte_t* buf, buf_t* key, size_t len, int mode) {
    size_t block_count = 0, number_of_blocks;
    byte_t* data_block = (unsigned char*)malloc(8 * sizeof(char));
    key_set* key_sets = (key_set*)malloc(17 * sizeof(key_set));

    unsigned char* processed_block = (unsigned char*)malloc(8 * sizeof(char));
    int padding = 0;
    auto cur = buf;
    auto left = len;
    
    memset(processed_block, 0, 8);
    memset(data_block, 0, 8);
    memset(key_sets, 0, 17 * sizeof(key_set));

    generate_sub_keys(key->data, key_sets);

    number_of_blocks = len / 8 + ((len % 8) == 0 ? 1 : 0);

    auto out = balloc(number_of_blocks * 8); auto dst = out->data;
    while (left) {
        block_count++;
        memcpy((byte_t*)data_block, cur, min(left, 8));
        cur += 8; left -= min(left, 8);
        if (block_count == number_of_blocks) {
            if (mode == ENCRYPTION_MODE) {
                padding = 8 - len % 8;
                if (padding < 8) { // Fill empty data block bytes with padding
                    memset((data_block + 8 - padding), (unsigned char)padding, padding);
                }

                process_message(data_block, processed_block, key_sets, mode);
                memcpy(dst, processed_block, 8); dst += 8;

                if (padding == 8) { // Write an extra block for padding
                    memset(data_block, (unsigned char)padding, 8);
                    process_message(data_block, processed_block, key_sets, mode);
                    memcpy(dst, processed_block, 8); dst += 8;
                }
            }
            else {
                process_message(data_block, processed_block, key_sets, mode);
                padding = processed_block[7];

                if (padding < 8) {
                    memcpy(dst, processed_block, 8-padding); dst += 8;
                }
            }
        }
        else {
            process_message(data_block, processed_block, key_sets, mode);
            memcpy(dst, processed_block, 8); dst += 8;
        }
        memset(data_block, 0, 8);
    }

    // Free up memory
    memset(processed_block, 0, 8);
    memset(data_block, 0, 8);
    memset(key_sets, 0, 17 * sizeof(key_set));
    free(data_block);
    free(processed_block);
    free(key_sets);
    return out;
}

// Some helper functions to make the challenge code easier to read.
buf_t* decrypt(buf_t* key, byte_t* buf, size_t len) {
    return des(buf, key, len, DECRYPTION_MODE);
}

buf_t* encrypt(buf_t* key, byte_t* buf, size_t len) {
    return des(buf, key, len, ENCRYPTION_MODE);
}

// https://github.com/tarequeh/DES/blob/master/des.c
int initial_key_permutaion[] = { 57, 49,  41, 33,  25,  17,  9,
                                 1, 58,  50, 42,  34,  26, 18,
                                10,  2,  59, 51,  43,  35, 27,
                                19, 11,   3, 60,  52,  44, 36,
                                63, 55,  47, 39,  31,  23, 15,
                                 7, 62,  54, 46,  38,  30, 22,
                                14,  6,  61, 53,  45,  37, 29,
                                21, 13,   5, 28,  20,  12,  4 };

int initial_message_permutation[] = { 58, 50, 42, 34, 26, 18, 10, 2,
                                        60, 52, 44, 36, 28, 20, 12, 4,
                                        62, 54, 46, 38, 30, 22, 14, 6,
                                        64, 56, 48, 40, 32, 24, 16, 8,
                                        57, 49, 41, 33, 25, 17,  9, 1,
                                        59, 51, 43, 35, 27, 19, 11, 3,
                                        61, 53, 45, 37, 29, 21, 13, 5,
                                        63, 55, 47, 39, 31, 23, 15, 7 };

int key_shift_sizes[] = { -1, 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 };

int sub_key_permutation[] = { 14, 17, 11, 24,  1,  5,
                                 3, 28, 15,  6, 21, 10,
                                23, 19, 12,  4, 26,  8,
                                16,  7, 27, 20, 13,  2,
                                41, 52, 31, 37, 47, 55,
                                30, 40, 51, 45, 33, 48,
                                44, 49, 39, 56, 34, 53,
                                46, 42, 50, 36, 29, 32 };

int message_expansion[] = { 32,  1,  2,  3,  4,  5,
                             4,  5,  6,  7,  8,  9,
                             8,  9, 10, 11, 12, 13,
                            12, 13, 14, 15, 16, 17,
                            16, 17, 18, 19, 20, 21,
                            20, 21, 22, 23, 24, 25,
                            24, 25, 26, 27, 28, 29,
                            28, 29, 30, 31, 32,  1 };

int S1[] = { 14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
             0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
             4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
            15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13 };

int S2[] = { 15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
             3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
             0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
            13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9 };

int S3[] = { 10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
            13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
            13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
             1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12 };

int S4[] = { 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
            13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
            10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
             3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14 };

int S5[] = { 2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
            14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
             4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
            11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3 };

int S6[] = { 12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
            10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
             9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
             4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13 };

int S7[] = { 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
            13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
             1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
             6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12 };

int S8[] = { 13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
             1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
             7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
             2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11 };

int right_sub_message_permutation[] = { 16,  7, 20, 21,
                                    29, 12, 28, 17,
                                     1, 15, 23, 26,
                                     5, 18, 31, 10,
                                     2,  8, 24, 14,
                                    32, 27,  3,  9,
                                    19, 13, 30,  6,
                                    22, 11,  4, 25 };

int final_message_permutation[] = { 40,  8, 48, 16, 56, 24, 64, 32,
                                    39,  7, 47, 15, 55, 23, 63, 31,
                                    38,  6, 46, 14, 54, 22, 62, 30,
                                    37,  5, 45, 13, 53, 21, 61, 29,
                                    36,  4, 44, 12, 52, 20, 60, 28,
                                    35,  3, 43, 11, 51, 19, 59, 27,
                                    34,  2, 42, 10, 50, 18, 58, 26,
                                    33,  1, 41,  9, 49, 17, 57, 25 };

void generate_sub_keys(unsigned char* main_key, key_set* key_sets) {
    int i, j;
    int shift_size;
    unsigned char shift_byte, first_shift_bits, second_shift_bits, third_shift_bits, fourth_shift_bits;

    for (i = 0; i < 8; i++) {
        key_sets[0].k[i] = 0;
    }

    for (i = 0; i < 56; i++) {
        shift_size = initial_key_permutaion[i];
        shift_byte = 0x80 >> ((shift_size - 1) % 8);
        shift_byte &= main_key[(shift_size - 1) / 8];
        shift_byte <<= ((shift_size - 1) % 8);

        key_sets[0].k[i / 8] |= (shift_byte >> i % 8);
    }

    for (i = 0; i < 3; i++) {
        key_sets[0].c[i] = key_sets[0].k[i];
    }

    key_sets[0].c[3] = key_sets[0].k[3] & 0xF0;

    for (i = 0; i < 3; i++) {
        key_sets[0].d[i] = (key_sets[0].k[i + 3] & 0x0F) << 4;
        key_sets[0].d[i] |= (key_sets[0].k[i + 4] & 0xF0) >> 4;
    }

    key_sets[0].d[3] = (key_sets[0].k[6] & 0x0F) << 4;


    for (i = 1; i < 17; i++) {
        for (j = 0; j < 4; j++) {
            key_sets[i].c[j] = key_sets[i - 1].c[j];
            key_sets[i].d[j] = key_sets[i - 1].d[j];
        }

        shift_size = key_shift_sizes[i];
        if (shift_size == 1) {
            shift_byte = 0x80;
        }
        else {
            shift_byte = 0xC0;
        }

        // Process C
        first_shift_bits = shift_byte & key_sets[i].c[0];
        second_shift_bits = shift_byte & key_sets[i].c[1];
        third_shift_bits = shift_byte & key_sets[i].c[2];
        fourth_shift_bits = shift_byte & key_sets[i].c[3];

        key_sets[i].c[0] <<= shift_size;
        key_sets[i].c[0] |= (second_shift_bits >> (8 - shift_size));

        key_sets[i].c[1] <<= shift_size;
        key_sets[i].c[1] |= (third_shift_bits >> (8 - shift_size));

        key_sets[i].c[2] <<= shift_size;
        key_sets[i].c[2] |= (fourth_shift_bits >> (8 - shift_size));

        key_sets[i].c[3] <<= shift_size;
        key_sets[i].c[3] |= (first_shift_bits >> (4 - shift_size));

        // Process D
        first_shift_bits = shift_byte & key_sets[i].d[0];
        second_shift_bits = shift_byte & key_sets[i].d[1];
        third_shift_bits = shift_byte & key_sets[i].d[2];
        fourth_shift_bits = shift_byte & key_sets[i].d[3];

        key_sets[i].d[0] <<= shift_size;
        key_sets[i].d[0] |= (second_shift_bits >> (8 - shift_size));

        key_sets[i].d[1] <<= shift_size;
        key_sets[i].d[1] |= (third_shift_bits >> (8 - shift_size));

        key_sets[i].d[2] <<= shift_size;
        key_sets[i].d[2] |= (fourth_shift_bits >> (8 - shift_size));

        key_sets[i].d[3] <<= shift_size;
        key_sets[i].d[3] |= (first_shift_bits >> (4 - shift_size));

        for (j = 0; j < 48; j++) {
            shift_size = sub_key_permutation[j];
            if (shift_size <= 28) {
                shift_byte = 0x80 >> ((shift_size - 1) % 8);
                shift_byte &= key_sets[i].c[(shift_size - 1) / 8];
                shift_byte <<= ((shift_size - 1) % 8);
            }
            else {
                shift_byte = 0x80 >> ((shift_size - 29) % 8);
                shift_byte &= key_sets[i].d[(shift_size - 29) / 8];
                shift_byte <<= ((shift_size - 29) % 8);
            }

            key_sets[i].k[j / 8] |= (shift_byte >> j % 8);
        }
    }
}

void process_message(unsigned char* message_piece, unsigned char* processed_piece, key_set * key_sets, int mode) {
    int i, k;
    int shift_size;
    unsigned char shift_byte;

    unsigned char initial_permutation[8];
    memset(initial_permutation, 0, 8);
    memset(processed_piece, 0, 8);

    for (i = 0; i < 64; i++) {
        shift_size = initial_message_permutation[i];
        shift_byte = 0x80 >> ((shift_size - 1) % 8);
        shift_byte &= message_piece[(shift_size - 1) / 8];
        shift_byte <<= ((shift_size - 1) % 8);

        initial_permutation[i / 8] |= (shift_byte >> i % 8);
    }

    unsigned char l[4], r[4];
    for (i = 0; i < 4; i++) {
        l[i] = initial_permutation[i];
        r[i] = initial_permutation[i + 4];
    }

    unsigned char ln[4], rn[4], er[6], ser[4];

    int key_index;
    for (k = 1; k <= 16; k++) {
        memcpy(ln, r, 4);

        memset(er, 0, 6);

        for (i = 0; i < 48; i++) {
            shift_size = message_expansion[i];
            shift_byte = 0x80 >> ((shift_size - 1) % 8);
            shift_byte &= r[(shift_size - 1) / 8];
            shift_byte <<= ((shift_size - 1) % 8);

            er[i / 8] |= (shift_byte >> i % 8);
        }

        if (mode == DECRYPTION_MODE) {
            key_index = 17 - k;
        }
        else {
            key_index = k;
        }

        for (i = 0; i < 6; i++) {
            er[i] ^= key_sets[key_index].k[i];
        }

        unsigned char row, column;

        for (i = 0; i < 4; i++) {
            ser[i] = 0;
        }

        // 0000 0000 0000 0000 0000 0000
        // rccc crrc cccr rccc crrc cccr

        // Byte 1
        row = 0;
        row |= ((er[0] & 0x80) >> 6);
        row |= ((er[0] & 0x04) >> 2);

        column = 0;
        column |= ((er[0] & 0x78) >> 3);

        ser[0] |= ((unsigned char)S1[row * 16 + column] << 4);

        row = 0;
        row |= (er[0] & 0x02);
        row |= ((er[1] & 0x10) >> 4);

        column = 0;
        column |= ((er[0] & 0x01) << 3);
        column |= ((er[1] & 0xE0) >> 5);

        ser[0] |= (unsigned char)S2[row * 16 + column];

        // Byte 2
        row = 0;
        row |= ((er[1] & 0x08) >> 2);
        row |= ((er[2] & 0x40) >> 6);

        column = 0;
        column |= ((er[1] & 0x07) << 1);
        column |= ((er[2] & 0x80) >> 7);

        ser[1] |= ((unsigned char)S3[row * 16 + column] << 4);

        row = 0;
        row |= ((er[2] & 0x20) >> 4);
        row |= (er[2] & 0x01);

        column = 0;
        column |= ((er[2] & 0x1E) >> 1);

        ser[1] |= (unsigned char)S4[row * 16 + column];

        // Byte 3
        row = 0;
        row |= ((er[3] & 0x80) >> 6);
        row |= ((er[3] & 0x04) >> 2);

        column = 0;
        column |= ((er[3] & 0x78) >> 3);

        ser[2] |= ((unsigned char)S5[row * 16 + column] << 4);

        row = 0;
        row |= (er[3] & 0x02);
        row |= ((er[4] & 0x10) >> 4);

        column = 0;
        column |= ((er[3] & 0x01) << 3);
        column |= ((er[4] & 0xE0) >> 5);

        ser[2] |= (unsigned char)S6[row * 16 + column];

        // Byte 4
        row = 0;
        row |= ((er[4] & 0x08) >> 2);
        row |= ((er[5] & 0x40) >> 6);

        column = 0;
        column |= ((er[4] & 0x07) << 1);
        column |= ((er[5] & 0x80) >> 7);

        ser[3] |= ((unsigned char)S7[row * 16 + column] << 4);

        row = 0;
        row |= ((er[5] & 0x20) >> 4);
        row |= (er[5] & 0x01);

        column = 0;
        column |= ((er[5] & 0x1E) >> 1);

        ser[3] |= (unsigned char)S8[row * 16 + column];

        for (i = 0; i < 4; i++) {
            rn[i] = 0;
        }

        for (i = 0; i < 32; i++) {
            shift_size = right_sub_message_permutation[i];
            shift_byte = 0x80 >> ((shift_size - 1) % 8);
            shift_byte &= ser[(shift_size - 1) / 8];
            shift_byte <<= ((shift_size - 1) % 8);

            rn[i / 8] |= (shift_byte >> i % 8);
        }

        for (i = 0; i < 4; i++) {
            rn[i] ^= l[i];
        }

        for (i = 0; i < 4; i++) {
            l[i] = ln[i];
            r[i] = rn[i];
        }
    }

    unsigned char pre_end_permutation[8];
    for (i = 0; i < 4; i++) {
        pre_end_permutation[i] = r[i];
        pre_end_permutation[4 + i] = l[i];
    }

    for (i = 0; i < 64; i++) {
        shift_size = final_message_permutation[i];
        shift_byte = 0x80 >> ((shift_size - 1) % 8);
        shift_byte &= pre_end_permutation[(shift_size - 1) / 8];
        shift_byte <<= ((shift_size - 1) % 8);

        processed_piece[i / 8] |= (shift_byte >> i % 8);
    }
}