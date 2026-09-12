/*
 * offline_queue.h — LittleFS-backed event queue
 *
 * Each event is one JSON line appended to /events.log.
 * On flush: read all, send each, rewrite file with unsent remainder.
 * Handles WiFi outages gracefully for outdoor deployment.
 */
#ifndef OFFLINE_QUEUE_H
#define OFFLINE_QUEUE_H

#include <Arduino.h>

namespace queue {
    bool        begin();
    bool        enqueue(const String& jsonLine);   // append
    size_t      pendingBytes();                    // size on disk
    size_t      approxCount();                     // rough count by line breaks
    void        clear();                           // wipe log
    // Iterate: call tryNext() repeatedly; when it returns false, no more.
    bool        tryNext(String& outJsonLine);
    void        rewind();                          // restart iteration from top
    void        commit();                          // remove last-consumed line
}

#endif