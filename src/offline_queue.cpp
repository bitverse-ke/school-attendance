#include "offline_queue.h"
#include "config.h"
#include <LittleFS.h>

namespace queue {

static bool       mounted = false;
static String     currentLine;       // line being iterated
static uint32_t   iterPos = 0;       // read cursor

bool begin() {
    if (!LittleFS.begin(true /* formatOnFail */)) {
        Serial.println(F("[Q] LittleFS mount FAILED"));
        return false;
    }
    mounted = true;
    Serial.printf("[Q] Mounted. Pending bytes: %u\n", pendingBytes());
    return true;
}

bool enqueue(const String& jsonLine) {
    if (!mounted) return false;
    File f = LittleFS.open(LOG_FILE, "a");
    if (!f) return false;
    f.print(jsonLine);
    f.print('\n');
    f.close();
    return true;
}

size_t pendingBytes() {
    if (!mounted) return 0;
    File f = LittleFS.open(LOG_FILE, "r");
    if (!f) return 0;
    size_t sz = f.size();
    f.close();
    return sz;
}

size_t approxCount() {
    size_t bytes = pendingBytes();
    if (bytes == 0) return 0;
    // avg event ~120 bytes in our schema
    return bytes / 120;
}

void clear() {
    if (!mounted) return;
    LittleFS.remove(LOG_FILE);
    iterPos = 0;
    currentLine = "";
}

bool tryNext(String& out) {
    if (!mounted) return false;
    if (iterPos == 0 && currentLine.length() > 0) {
        out = currentLine;
        return true;  // repeat caller
    }
    File f = LittleFS.open(LOG_FILE, "r");
    if (!f) return false;
    f.seek(iterPos);
    if (!f.available()) {
        f.close();
        return false;
    }
    currentLine = f.readStringUntil('\n');
    iterPos = f.position();
    f.close();
    out = currentLine;
    return true;
}

void rewind() {
    iterPos = 0;
    currentLine = "";
}

void commit() {
    if (!mounted) return;
    // Truncate everything up through iterPos by rewriting from iterPos onward.
    File f = LittleFS.open(LOG_FILE, "r");
    if (!f) return;
    f.seek(iterPos);
    String rest = f.readString();
    f.close();

    LittleFS.remove(LOG_FILE);
    File w = LittleFS.open(LOG_FILE, "w");
    if (!w) return;
    w.print(rest);
    w.close();

    iterPos = 0;
}

} // namespace