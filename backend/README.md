# Attendance System — Backend Documentation

Flask + SQLite backend for an RFID-based attendance system.  
Receives data from an ESP32 (RFID scanner + DHT11 sensor) and serves status to a tablet.

---

## Setup

### 1. Install dependencies
```bash
pip install -r requirements.txt
```

`requirements.txt` contains:
```
flask>=3.0.0
flask-cors>=4.0.0
python-dotenv>=1.0.0
```

### 2. Create your `.env` file

```bash
echo "" > .env
```

```env
API_KEY=YourSuperSecretRandomString123
DB_FILE=system.db
MAX_PAYLOAD=2048
PORT=5000
```

> **Never commit `.env` to git.** It contains your secret API key.

### 3. Run the server
```bash
python app.py
```

The server starts on `http://0.0.0.0:5000` and is reachable by any device on the same LAN.  
The SQLite database (`system.db`) is created automatically on first run.

---

## Authentication

Three endpoints require an `X-API-Key` header on every request:  
`/scan`, `/manual`, `/environment`

The key must match the `API_KEY` value in your `.env`.

```
X-API-Key: YourSuperSecretRandomString123
```

`/status` is public — no key needed (tablet polling).

---

## Database tables

### `users`
Registered RFID cards. Populated manually or via a separate admin tool.

| Column | Type | Description |
|---|---|---|
| `uid` | TEXT (PK) | RFID card UID, uppercase hex, 8–14 chars |
| `name` | TEXT | Full name of the cardholder |
| `id_number` | INTEGER | Student/employee ID number |
| `photo_path` | TEXT | Optional path to photo file |

**Add a user manually:**
```bash
sqlite3 system.db "INSERT INTO users (uid, name, id_number, photo_path) \
  VALUES ('A1B2C3D4', 'Juan dela Cruz', 20240001, NULL);"
```

### `attendance`
Log of every entry, both RFID and manual.

| Column | Type | Description |
|---|---|---|
| `timestamp` | TEXT | UTC ISO-8601, e.g. `2026-06-09T23:50:18+00:00` |
| `name` | TEXT | Person's name |
| `id_number` | INTEGER | ID number |
| `entry_type` | TEXT | `RFID` or `Manual` |
| `purpose` | TEXT | Reason for visit (manual entries only) |

### `environment`
DHT11 sensor readings from the ESP32.

| Column | Type | Description |
|---|---|---|
| `timestamp` | TEXT | UTC ISO-8601 |
| `temperature` | REAL | °C, accepted range: −50.0 to 100.0 |
| `humidity` | REAL | %, accepted range: 0.0 to 100.0 |

---

## API Endpoints

### `POST /scan`
Called by the ESP32 when an RFID card is tapped.  
Looks up the UID in `users`, logs an RFID attendance entry, and returns the user's data.

**Headers:**
```
Content-Type: application/json
X-API-Key: <your key>
```

**Request body:**
```json
{
  "uid": "A1B2C3D4"
}
```

**UID rules:** 8–14 characters, hex only (`A–F`, `0–9`), case-insensitive (normalized to uppercase internally).

**Responses:**

| Status | Body | Meaning |
|---|---|---|
| `200 OK` | `{ "name": "...", "id_number": 123, "photo_path": null }` | Card found, entry logged |
| `400 Bad Request` | `{ "error": "Bad Request" }` | Missing `uid` field |
| `400 Bad Request` | `{ "error": "Invalid UID" }` | UID fails hex/length validation |
| `401 Unauthorized` | `{ "error": "Unauthorized" }` | Wrong or missing API key |
| `404 Not Found` | `{ "error": "Not Found" }` | UID not registered in `users` table |
| `500` | `{ "error": "Server Error" }` | Database error |

**ESP32 example (Arduino/C++):**
```cpp
HTTPClient http;
http.begin("http://192.168.1.100:5000/scan");
http.addHeader("Content-Type", "application/json");
http.addHeader("X-API-Key", "YourSuperSecretRandomString123");

String body = "{\"uid\":\"" + uidString + "\"}";
int code = http.POST(body);
```

---

### `POST /manual`
Called when someone fills in the manual entry form on the tablet.  
Logs a manual attendance entry directly to the `attendance` table.

**Headers:**
```
Content-Type: application/json
X-API-Key: <your key>
```

**Request body:**
```json
{
  "name": "Juan dela Cruz",
  "id": 20240001,
  "purpose": "Library visit"
}
```

> Note: the field is `id`, not `id_number`.

**Field rules:**

| Field | Type | Rules |
|---|---|---|
| `name` | string | 1–50 chars, letters/numbers/spaces/hyphens/dots/commas |
| `id` | integer | Positive whole number |
| `purpose` | string | 1–100 chars, same character rules as name |

**Responses:**

| Status | Body | Meaning |
|---|---|---|
| `201 Created` | `{ "status": "Success" }` | Entry logged |
| `400 Bad Request` | `{ "error": "Bad Request" }` | Missing required field |
| `400 Bad Request` | `{ "error": "Invalid Input: id must be a positive integer" }` | Bad ID value |
| `400 Bad Request` | `{ "error": "Invalid Input" }` | Name or purpose fails validation |
| `401 Unauthorized` | `{ "error": "Unauthorized" }` | Wrong or missing API key |

**Fetch example (JavaScript):**
```js
await fetch("http://localhost:5000/manual", {
  method: "POST",
  headers: {
    "Content-Type": "application/json",
    "X-API-Key": "YourSuperSecretRandomString123"
  },
  body: JSON.stringify({
    name: "Juan dela Cruz",
    id: 20240001,
    purpose: "Library visit"
  })
});
```

---

### `POST /environment`
Called by the ESP32 on each DHT11 sensor reading.  
Stores temperature and humidity with a server-generated UTC timestamp.

**Headers:**
```
Content-Type: application/json
X-API-Key: <your key>
```

**Request body:**
```json
{
  "temperature": 27.5,
  "humidity": 65.0
}
```

**Responses:**

| Status | Body | Meaning |
|---|---|---|
| `201 Created` | `{ "status": "Success" }` | Reading stored |
| `400 Bad Request` | `{ "error": "Bad Request" }` | Missing field |
| `400 Bad Request` | `{ "error": "Invalid sensor values" }` | Value out of accepted range |
| `401 Unauthorized` | `{ "error": "Unauthorized" }` | Wrong or missing API key |

**ESP32 example (Arduino/C++):**
```cpp
HTTPClient http;
http.begin("http://192.168.1.100:5000/environment");
http.addHeader("Content-Type", "application/json");
http.addHeader("X-API-Key", "YourSuperSecretRandomString123");

String body = "{\"temperature\":" + String(temp, 1) + ",\"humidity\":" + String(hum, 1) + "}";
int code = http.POST(body);
```

---

### `GET /status`
Polled by the tablet to get the current system state.  
Returns the latest environment reading and the 5 most recent attendance entries.  
**No API key required.**

**Request:** No body, no headers needed.

**Response `200 OK`:**
```json
{
  "environment": {
    "timestamp": "2026-06-09T23:50:22+00:00",
    "temperature": 27.5,
    "humidity": 65.0
  },
  "recent_attendance": [
    {
      "timestamp": "2026-06-09T23:50:18+00:00",
      "name": "Juan dela Cruz",
      "id_number": 20240001,
      "entry_type": "Manual",
      "purpose": "Library visit"
    }
  ]
}
```

`environment` is `null` if no sensor data has been received yet.  
`recent_attendance` is an empty array `[]` if no entries exist.

**Fetch example (JavaScript):**
```js
const res = await fetch("http://localhost:5000/status");
const data = await res.json();
console.log(data.environment.temperature);
```

---

## Validation rules summary

| Field | Rule |
|---|---|
| `uid` | Hex string, 8–14 chars (`A-F0-9`), case-insensitive |
| `name` | 1–50 chars, `a-zA-Z0-9 -.,` only |
| `id` | Positive integer (`> 0`) |
| `purpose` | 1–100 chars, same as name |
| `temperature` | Float, −50.0 to 100.0 |
| `humidity` | Float, 0.0 to 100.0 |
| Payload size | Max 2048 bytes (configurable via `MAX_PAYLOAD` in `.env`) |

---

## Project structure

```
project/
├── app.py            # Flask backend
├── .env              # Your secrets (never commit this)
├── .env.example      # Template to copy from
├── requirements.txt  # Python dependencies
├── system.db         # SQLite database (auto-created on first run)
└── tester.html       # Browser-based test UI for all endpoints
```

---

## Quick reference — which method for which endpoint

| Endpoint | Method | Who calls it | Key required |
|---|---|---|---|
| `/scan` | POST | ESP32 | Yes |
| `/manual` | POST | Tablet / frontend | Yes |
| `/environment` | POST | ESP32 | Yes |
| `/status` | GET | Tablet | No |
