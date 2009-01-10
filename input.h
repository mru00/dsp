#pragma once


typedef size_t (*rdfun)(buffer_t* buffer, const size_t size);

extern buffer_t*
input_open(const char* which, const size_t size);

extern int
input_read();

extern void
input_close();
