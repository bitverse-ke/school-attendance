# Firebase Cloud Functions API contract

The ESP32 firmware POSTs to four Cloud Function endpoints. All requests
are HTTPS, JSON body, with auth headers:

```
X-Device-Id:   <device-id>
X-Device-Key:  <per-device API key>
Content-Type:  application/json
```

The backend validates `X-Device-Key` against `devices/{deviceId}/apiKey`
in Firestore. Rotate via Cloud Function `rotateDeviceKey`.

---

## POST `/deviceHeartbeat`

Periodic liveness ping (every 5 min).

**Request body:**
```json
{
  "type": "heartbeat",
  "deviceId": "gate-north-01",
  "uptimeSec": 86400,
  "rssi": -65,
  "epoch": 1726148400
}
```

**Response 200:**
```json
{ "ok": true, "serverTime": 1726148405 }
```

**Backend action:** Update `devices/{deviceId}`:
- `lastSeen` = now
- `lastRssi` = body.rssi
- `uptimeSec` = body.uptimeSec
- `online` = true

If 3+ heartbeats missed, mark `online=false` and trigger alert.

---

## POST `/deviceEvent`

Single fingerprint verification event. **This is the hot path.**

**Request body:**
```json
{
  "type": "verify",
  "deviceId": "gate-north-01",
  "fingerprintId": 247,
  "score": 142,
  "epoch": 1726148400
}
```

**Response 200:**
```json
{
  "ok": true,
  "studentId": "ADM-2024-247",
  "studentName": "Jane Wanjiku",
  "isLate": false,
  "classId": "grade-5-east"
}
```

**Backend action:**
1. Look up `students/{fingerprintId}` to get `studentId`, `name`, `classId`
2. Look up `classes/{classId}/schedule` to determine if late
3. Append to `attendance_logs/{auto-id}`:
   ```
   { deviceId, fingerprintId, studentId, name, classId,
     epoch, isLate, score }
   ```
4. Update `students/{fingerprintId}`:
   - `lastSeenEpoch`
   - `lastSeenDevice`

**Failure cases:**
- Unknown `fingerprintId` → 200 with `{ ok: true, unknown: true }`
- Stale epoch (>5 min skew) → log warning, still accept
- Invalid device key → 401

---

## POST `/deviceEnroll`

Device-side enrolment mirrored to backend (slot was assigned by admin
in web UI, then user enrolled at the device).

**Request body:**
```json
{
  "type": "enroll",
  "deviceId": "gate-north-01",
  "fingerprintId": 247,
  "studentId": "ADM-2024-247",
  "name": "Jane Wanjiku",
  "epoch": 1726148400
}
```

**Response 200:** `{ "ok": true }`

**Backend action:**
- Create `enrollments/{fingerprintId}`:
  ```
  { deviceId, studentId, name, enrolledAt, enrolledByDevice }
  ```
- Link student: `students/{studentId}/fingerprintId = 247`

---

## POST `/devicePullTemplates`

Pull templates that should be loaded onto this device. Used when a new
student is enrolled via the web UI and needs to be sent down to a
device (e.g. for a multi-gate setup where each gate has its own R307).

**Request body:**
```json
{
  "deviceId": "gate-north-01",
  "sinceEpoch": 1726148400
}
```

**Response 200:**
```json
{
  "templates": [
    { "fingerprintId": 247, "studentId": "ADM-2024-247", "enrolledAt": 1726148400 }
  ]
}
```

> **Note:** This returns only metadata. Actual template download
> requires either:
> - Re-enrolment at the device with the same finger (recommended — keeps templates local-only), OR
> - Encrypted template blob transfer (future enhancement)

For prototype, this endpoint is a stub that returns metadata so the
admin UI knows which students to enrol at which device.

---

## Firestore schema (backend writes these)

```
devices/{deviceId}/
  apiKey:        string      (validated by Cloud Functions)
  location:      string      ("Gate North", "Classroom 5B")
  model:         string      ("esp32-wroom-r307-v1")
  firmwareVer:   string
  lastSeen:      timestamp
  lastRssi:      int
  online:        boolean
  uptimeSec:     int

students/{studentId}/
  name:          string
  classId:       string
  fingerprintId: int         (linked on enrolment)
  lastSeenEpoch: timestamp
  lastSeenDevice:string
  guardian: {
    name, phone, email, consentSigned: bool, consentDate: timestamp
  }

classes/{classId}/
  name: string
  schedule: {
    startMin: int            (minutes from midnight, e.g. 7:30 = 450)
    lateAfterMin: int        (e.g. 7:45 = 465)
  }

enrollments/{fingerprintId}/
  deviceId:      string
  studentId:     string
  enrolledAt:    timestamp
  enrolledByDevice: string

attendance_logs/{auto-id}/
  deviceId:      string
  fingerprintId: int
  studentId:     string
  name:          string
  classId:       string
  epoch:         timestamp
  isLate:        boolean
  score:         int         (R307 confidence)
```

---

## Error handling

- **Network failure:** firmware queues the event to LittleFS, retries on next
  WiFi reconnect + every 5 min heartbeat.
- **HTTP 5xx:** same as network failure (queue).
- **HTTP 4xx:** drop the event, log to serial. Common cause: invalid device
  key after rotation. Operator should re-provision via serial `WIFIRESET`.
- **Clock skew:** backend accepts up to 5 min offset. Beyond that, log
  warning for manual review.

## Rate limits

- Per device: 1 heartbeat / 5 min
- Per device: 1 verification / 1 sec (firmware-enforced by IDLE → result flow)
- Per device: 1 enrolment / 30 sec

Cloud Functions enforce these via per-device counters in Firestore.
