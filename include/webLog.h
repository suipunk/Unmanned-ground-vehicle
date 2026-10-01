#pragma once

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include "AsyncFsWebServer.h"

#define FILESYSTEM LittleFS

extern AsyncFsWebServer server;

class MultiPrint : public Print {
    public:
        size_t write(uint8_t c) override;
        size_t write(const uint8_t *buffer, size_t size) override;
 
    private:
        String _buffer;
        void flushLine();
};
 
extern MultiPrint multiLog;
 
void setupWebLog();
 
void loopWebLog();
 