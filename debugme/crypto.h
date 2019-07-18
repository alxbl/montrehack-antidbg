#pragma once

#include <string>
#include <cstdint>

typedef uint8_t byte_t;

typedef struct buf_ {
	byte_t*  data;
	size_t size;
} buf_t;

buf_t* balloc(size_t sz);

void  bfree(buf_t* buf);

buf_t* md5(byte_t* b, size_t len);
buf_t* decrypt(buf_t* key, byte_t* buf, size_t len);
buf_t* encrypt(buf_t* key, byte_t* buf, size_t len);