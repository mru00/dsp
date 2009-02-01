#pragma once


typedef size_t (*rdfun)(float* buffer, const size_t size);

extern rdfun
input_open(const char* which);

extern void
input_close();
