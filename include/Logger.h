#pragma once

class Logger
{
public:
    void begin();

    void info(const char* msg);
    void warning(const char* msg);
    void error(const char* msg);
};