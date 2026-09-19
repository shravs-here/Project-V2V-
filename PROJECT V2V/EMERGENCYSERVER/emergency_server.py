
from flask import Flask, request, jsonify
from datetime import datetime
from zoneinfo import ZoneInfo
import sqlite3
import os
import time

app = Flask(__name__)

# ============================================================
# CONFIGURATION
# ============================================================

DB_FILE = "accident_log.db"
IST = ZoneInfo("Asia/Kolkata")

server_start_time = time.time()


# ============================================================
# DATABASE
# ============================================================

def get_db():
    conn = sqlite3.connect(DB_FILE)
    conn.row_factory = sqlite3.Row
    return conn


def init_db():

    conn = get_db()

    conn.execute("""
        CREATE TABLE IF NOT EXISTS accidents (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            vehicle_id TEXT,
            accident_type TEXT,
            severity TEXT,
            latitude REAL,
            longitude REAL,
            timestamp TEXT,
            status TEXT DEFAULT 'NEW'
        )
    """)

    conn.commit()
    conn.close()


init_db()


# ============================================================
# HELPER FUNCTIONS
# ============================================================

def get_current_time():
    return datetime.now(IST).strftime(
        "%Y-%m-%d %H:%M:%S"
    )


def get_accident_count():

    conn = get_db()

    row = conn.execute(
        "SELECT COUNT(*) AS count FROM accidents"
    ).fetchone()

    conn.close()

    return row["count"]


def get_today_count():

    today = datetime.now(IST).strftime("%Y-%m-%d")

    conn = get_db()

    row = conn.execute(
        """
        SELECT COUNT(*) AS count
        FROM accidents
        WHERE timestamp LIKE ?
        """,
        (today + "%",)
    ).fetchone()

    conn.close()

    return row["count"]


# ============================================================
# HOME
# ============================================================

@app.route("/")
def home():

    return """
    <!DOCTYPE html>
    <html>

    <head>
        <title>V2V Emergency Network</title>

        <meta name="viewport"
              content="width=device-width, initial-scale=1.0">

        <style>

            * {
                box-sizing: border-box;
            }

            body {
                margin: 0;
                font-family: Arial, Helvetica, sans-serif;
                background: #080b12;
                color: white;
                min-height: 100vh;

                display: flex;
                align-items: center;
                justify-content: center;
            }

            .container {
                text-align: center;
                max-width: 700px;
                padding: 40px;
            }

            .logo {
                font-size: 70px;
                margin-bottom: 10px;
            }

            h1 {
                font-size: 42px;
                margin: 10px 0;
            }

            p {
                color: #9ca3af;
                font-size: 18px;
            }

            .status {
                display: inline-flex;
                align-items: center;
                gap: 10px;

                background: #10251a;
                color: #4ade80;

                padding: 10px 18px;
                border-radius: 30px;

                margin: 20px 0;
            }

            .dot {
                width: 10px;
                height: 10px;
                background: #22c55e;
                border-radius: 50%;
            }

            .button {
                display: inline-block;

                margin-top: 25px;
                padding: 16px 30px;

                background: #dc2626;
                color: white;

                text-decoration: none;

                border-radius: 10px;

                font-size: 17px;
                font-weight: bold;
            }

            .button:hover {
                background: #ef4444;
            }

        </style>

    </head>

    <body>

        <div class="container">

            <div class="logo">🚨</div>

            <h1>V2V Emergency Network</h1>

            <p>
                Vehicle-to-Vehicle Accident Response System
            </p>

            <div class="status">
                <span class="dot"></span>
                Emergency Server Online
            </div>

            <br>

            <a class="button" href="/dashboard">
                Open Emergency Command Center
            </a>

        </div>

    </body>

    </html>
    """


# ============================================================
# ACCIDENT RECEIVER
# ============================================================

@app.route("/accident", methods=["POST"])
def accident():

    data = request.get_json(silent=True)

    if not data:

        return jsonify({
            "status": "error",
            "message": "Invalid JSON data"
        }), 400


    vehicle_id = data.get(
        "vehicle_id",
        "UNKNOWN"
    )

    accident_type = data.get(
        "type",
        "ACCIDENT"
    )

    severity = data.get(
        "severity",
        "UNKNOWN"
    )

    latitude = data.get(
        "latitude"
    )

    longitude = data.get(
        "longitude"
    )

    timestamp = get_current_time()


    # ========================================================
    # SAVE ACCIDENT
    # ========================================================

    conn = get_db()

    cursor = conn.execute(
        """
        INSERT INTO accidents
        (
            vehicle_id,
            accident_type,
            severity,
            latitude,
            longitude,
            timestamp,
            status
        )
        VALUES (?, ?, ?, ?, ?, ?, ?)
        """,
        (
            vehicle_id,
            accident_type,
            severity,
            latitude,
            longitude,
            timestamp,
            "NEW"
        )
    )

    accident_id = cursor.lastrowid

    conn.commit()
    conn.close()


    # ========================================================
    # SERVER LOG
    # ========================================================

    print()
    print("==============================================")
    print("🚨 ACCIDENT ALERT RECEIVED")
    print("==============================================")
    print("Accident ID :", accident_id)
    print("Vehicle     :", vehicle_id)
    print("Type        :", accident_type)
    print("Severity    :", severity)
    print("Latitude    :", latitude)
    print("Longitude   :", longitude)
    print("Time        :", timestamp)
    print("==============================================")
    print()


    return jsonify({

        "status": "received",

        "message":
            "Emergency alert received successfully",

        "accident_id":
            accident_id

    })


# ============================================================
# DASHBOARD DATA API
# ============================================================

@app.route("/api/dashboard")
def dashboard_data():

    conn = get_db()


    # Latest accident
    latest = conn.execute(
        """
        SELECT *
        FROM accidents
        ORDER BY id DESC
        LIMIT 1
        """
    ).fetchone()


    # Accident history
    rows = conn.execute(
        """
        SELECT *
        FROM accidents
        ORDER BY id DESC
        LIMIT 100
        """
    ).fetchall()


    conn.close()


    accidents = []

    for row in rows:

        accidents.append({
            "id": row["id"],
            "vehicle_id": row["vehicle_id"],
            "type": row["accident_type"],
            "severity": row["severity"],
            "latitude": row["latitude"],
            "longitude": row["longitude"],
            "timestamp": row["timestamp"],
            "status": row["status"]
        })


    latest_data = None

    if latest:

        latest_data = {
            "id": latest["id"],
            "vehicle_id": latest["vehicle_id"],
            "type": latest["accident_type"],
            "severity": latest["severity"],
            "latitude": latest["latitude"],
            "longitude": latest["longitude"],
            "timestamp": latest["timestamp"],
            "status": latest["status"]
        }


    return jsonify({

        "total_accidents":
            get_accident_count(),

        "today_accidents":
            get_today_count(),

        "latest":
            latest_data,

        "accidents":
            accidents

    })


# ============================================================
# ACKNOWLEDGE ACCIDENT
# ============================================================

@app.route("/api/accident/<int:accident_id>/acknowledge",
           methods=["POST"])
def acknowledge_accident(accident_id):

    conn = get_db()

    cursor = conn.execute(
        """
        UPDATE accidents
        SET status = 'ACKNOWLEDGED'
        WHERE id = ?
        """,
        (accident_id,)
    )

    conn.commit()

    updated = cursor.rowcount

    conn.close()


    if updated == 0:

        return jsonify({
            "status": "error",
            "message": "Accident not found"
        }), 404


    return jsonify({
        "status": "success",
        "message": "Accident acknowledged"
    })


# ============================================================
# DASHBOARD
# ============================================================

@app.route("/dashboard")
def dashboard():

    return """
<!DOCTYPE html>

<html>

<head>

    <meta charset="UTF-8">

    <meta name="viewport"
          content="width=device-width, initial-scale=1.0">

    <title>V2V Emergency Command Center</title>


    <style>

        * {
            box-sizing: border-box;
        }


        body {

            margin: 0;

            font-family:
                Inter,
                Arial,
                Helvetica,
                sans-serif;

            background:
                #070a10;

            color: #f8fafc;
        }


        /* ==================================================
           HEADER
           ================================================== */

        .header {

            height: 75px;

            display: flex;

            align-items: center;

            justify-content: space-between;

            padding:
                0 35px;

            background:
                #0d111a;

            border-bottom:
                1px solid #1f2937;

            position: sticky;

            top: 0;

            z-index: 50;
        }


        .brand {

            display: flex;

            align-items: center;

            gap: 14px;
        }


        .brand-icon {

            width: 43px;
            height: 43px;

            border-radius: 10px;

            display: flex;

            align-items: center;

            justify-content: center;

            background:
                #dc2626;

            font-size: 22px;
        }


        .brand-text h1 {

            margin: 0;

            font-size: 19px;
        }


        .brand-text p {

            margin: 3px 0 0;

            color: #64748b;

            font-size: 12px;
        }


        .server-status {

            display: flex;

            align-items: center;

            gap: 8px;

            color: #4ade80;

            font-size: 13px;

            font-weight: bold;
        }


        .status-dot {

            width: 9px;
            height: 9px;

            border-radius: 50%;

            background: #22c55e;

            box-shadow:
                0 0 10px #22c55e;
        }


        /* ==================================================
           MAIN
           ================================================== */

        .main {

            max-width: 1500px;

            margin: auto;

            padding: 30px;
        }


        .page-title {

            margin-bottom: 25px;
        }


        .page-title h2 {

            margin: 0;

            font-size: 30px;
        }


        .page-title p {

            margin-top: 7px;

            color: #64748b;
        }


        /* ==================================================
           STAT CARDS
           ================================================== */

        .stats {

            display: grid;

            grid-template-columns:
                repeat(3, 1fr);

            gap: 18px;

            margin-bottom: 25px;
        }


        .stat-card {

            background:
                #0d111a;

            border:
                1px solid #1f2937;

            border-radius: 14px;

            padding: 23px;
        }


        .stat-label {

            color: #64748b;

            font-size: 13px;

            text-transform:
                uppercase;

            letter-spacing:
                0.08em;
        }


        .stat-value {

            margin-top: 8px;

            font-size: 34px;

            font-weight: 800;
        }


        .stat-danger {

            color: #ef4444;
        }


        .stat-green {

            color: #4ade80;
        }


        /* ==================================================
           LATEST INCIDENT
           ================================================== */

        .incident {

            background:
                linear-gradient(
                    135deg,
                    #241014,
                    #110c10
                );

            border:
                1px solid #7f1d1d;

            border-radius: 16px;

            padding: 25px;

            margin-bottom: 25px;

            display: none;
        }


        .incident.active {

            display: block;
        }


        .incident-header {

            display: flex;

            justify-content:
                space-between;

            align-items: center;

            margin-bottom: 20px;
        }


        .incident-title {

            display: flex;

            align-items: center;

            gap: 12px;

            color: #f87171;

            font-size: 21px;

            font-weight: 800;
        }


        .new-badge {

            background: #dc2626;

            color: white;

            padding: 5px 10px;

            border-radius: 5px;

            font-size: 11px;

            font-weight: bold;
        }


        .incident-grid {

            display: grid;

            grid-template-columns:
                repeat(4, 1fr);

            gap: 15px;
        }


        .incident-item {

            background:
                rgba(0,0,0,0.25);

            border-radius: 9px;

            padding: 15px;
        }


        .incident-item label {

            display: block;

            color: #64748b;

            font-size: 11px;

            text-transform: uppercase;

            margin-bottom: 7px;
        }


        .incident-item strong {

            font-size: 15px;
        }


        .incident-actions {

            margin-top: 20px;

            display: flex;

            gap: 10px;
        }


        button,
        .map-button {

            border: none;

            padding: 11px 17px;

            border-radius: 8px;

            cursor: pointer;

            font-weight: bold;

            text-decoration: none;

            display: inline-block;
        }


        .ack-button {

            background: #dc2626;

            color: white;
        }


        .map-button {

            background: #1e293b;

            color: white;
        }


        /* ==================================================
           LOG
           ================================================== */

        .log-card {

            background:
                #0d111a;

            border:
                1px solid #1f2937;

            border-radius: 16px;

            overflow: hidden;
        }


        .log-header {

            padding: 20px 22px;

            display: flex;

            align-items: center;

            justify-content:
                space-between;

            border-bottom:
                1px solid #1f2937;
        }


        .log-header h3 {

            margin: 0;

            font-size: 18px;
        }


        .live-label {

            color: #4ade80;

            font-size: 12px;

            font-weight: bold;
        }


        .table-wrapper {

            overflow-x: auto;
        }


        table {

            width: 100%;

            border-collapse:
                collapse;
        }


        th {

            text-align: left;

            padding: 14px 20px;

            color: #64748b;

            font-size: 11px;

            text-transform:
                uppercase;

            letter-spacing:
                0.06em;

            background:
                #090d14;
        }


        td {

            padding: 16px 20px;

            border-top:
                1px solid #17202d;

            font-size: 13px;
        }


        tr:hover {

            background:
                #111827;
        }


        .severity {

            display: inline-block;

            padding: 5px 9px;

            border-radius: 5px;

            font-size: 11px;

            font-weight: bold;
        }


        .severity-moderate {

            background: #422006;

            color: #fbbf24;
        }


        .severity-severe {

            background: #450a0a;

            color: #f87171;
        }


        .severity-critical {

            background: #450a0a;

            color: #fb7185;
        }


        .severity-minor {

            background: #052e16;

            color: #4ade80;
        }


        .status {

            color: #fbbf24;

            font-size: 11px;

            font-weight: bold;
        }


        .status-ack {

            color: #4ade80;
        }


        .empty {

            padding: 60px;

            text-align: center;

            color: #475569;
        }


        /* ==================================================
           NOTIFICATION
           ================================================== */

        #notification {

            position: fixed;

            top: 95px;

            right: 25px;

            width: 370px;

            background:
                #160b0d;

            border:
                1px solid #ef4444;

            border-left:
                5px solid #ef4444;

            border-radius: 12px;

            padding: 18px;

            box-shadow:
                0 15px 50px
                rgba(0,0,0,0.5);

            transform:
                translateX(450px);

            transition:
                transform 0.35s ease;

            z-index: 100;
        }


        #notification.show {

            transform:
                translateX(0);
        }


        .notification-title {

            color: #f87171;

            font-weight: 800;

            font-size: 16px;

            margin-bottom: 8px;
        }


        .notification-text {

            color: #cbd5e1;

            font-size: 13px;

            line-height: 1.6;
        }


        @media(max-width: 900px) {

            .stats {

                grid-template-columns:
                    1fr;
            }

            .incident-grid {

                grid-template-columns:
                    repeat(2, 1fr);
            }
        }


        @media(max-width: 600px) {

            .header {

                padding: 0 15px;
            }

            .main {

                padding: 18px;
            }

            .server-status {

                display: none;
            }

            .incident-grid {

                grid-template-columns:
                    1fr;
            }

            #notification {

                left: 15px;
                right: 15px;

                width: auto;
            }
        }

    </style>

</head>


<body>


<!-- ======================================================
     HEADER
     ====================================================== -->

<header class="header">

    <div class="brand">

        <div class="brand-icon">
            🚨
        </div>

        <div class="brand-text">

            <h1>
                V2V EMERGENCY NETWORK
            </h1>

            <p>
                ACCIDENT RESPONSE COMMAND CENTER
            </p>

        </div>

    </div>


    <div class="server-status">

        <span class="status-dot"></span>

        SERVER ONLINE

    </div>

</header>


<!-- ======================================================
     MAIN
     ====================================================== -->

<main class="main">


    <div class="page-title">

        <h2>
            Emergency Dashboard
        </h2>

        <p>
            Real-time vehicle accident monitoring
        </p>

    </div>


    <!-- ==================================================
         STATISTICS
         ================================================== -->

    <section class="stats">


        <div class="stat-card">

            <div class="stat-label">
                Total Accidents
            </div>

            <div
                id="totalAccidents"
                class="stat-value stat-danger"
            >
                0
            </div>

        </div>


        <div class="stat-card">

            <div class="stat-label">
                Accidents Today
            </div>

            <div
                id="todayAccidents"
                class="stat-value"
            >
                0
            </div>

        </div>


        <div class="stat-card">

            <div class="stat-label">
                System Status
            </div>

            <div
                class="stat-value stat-green"
                style="font-size:24px;"
            >
                ONLINE
            </div>

        </div>


    </section>


    <!-- ==================================================
         ACTIVE INCIDENT
         ================================================== -->

    <section
        id="incident"
        class="incident"
    >

        <div class="incident-header">

            <div class="incident-title">

                🚨

                <span>
                    ACCIDENT DETECTED
                </span>

                <span
                    id="newBadge"
                    class="new-badge"
                >
                    NEW
                </span>

            </div>

        </div>


        <div class="incident-grid">


            <div class="incident-item">

                <label>
                    Accident ID
                </label>

                <strong id="incidentId">
                    —
                </strong>

            </div>


            <div class="incident-item">

                <label>
                    Vehicle
                </label>

                <strong id="incidentVehicle">
                    —
                </strong>

            </div>


            <div class="incident-item">

                <label>
                    Type
                </label>

                <strong id="incidentType">
                    —
                </strong>

            </div>


            <div class="incident-item">

                <label>
                    Severity
                </label>

                <strong id="incidentSeverity">
                    —
                </strong>

            </div>


            <div class="incident-item">

                <label>
                    Latitude
                </label>

                <strong id="incidentLat">
                    —
                </strong>

            </div>


            <div class="incident-item">

                <label>
                    Longitude
                </label>

                <strong id="incidentLon">
                    —
                </strong>

            </div>


            <div class="incident-item">

                <label>
                    Time
                </label>

                <strong id="incidentTime">
                    —
                </strong>

            </div>


            <div class="incident-item">

                <label>
                    Status
                </label>

                <strong id="incidentStatus">
                    NEW
                </strong>

            </div>


        </div>


        <div class="incident-actions">

            <a
                id="mapButton"
                class="map-button"
                href="#"
                target="_blank"
            >
                📍 VIEW LOCATION
            </a>


            <button
                id="ackButton"
                class="ack-button"
            >
                ✓ ACKNOWLEDGE INCIDENT
            </button>

        </div>


    </section>


    <!-- ==================================================
         ACCIDENT LOG
         ================================================== -->

    <section class="log-card">

        <div class="log-header">

            <h3>
                Accident Log
            </h3>

            <span class="live-label">
                ● LIVE
            </span>

        </div>


        <div class="table-wrapper">

            <table>

                <thead>

                    <tr>

                        <th>
                            ID
                        </th>

                        <th>
                            Vehicle
                        </th>

                        <th>
                            Type
                        </th>

                        <th>
                            Severity
                        </th>

                        <th>
                            Location
                        </th>

                        <th>
                            Time
                        </th>

                        <th>
                            Status
                        </th>

                    </tr>

                </thead>


                <tbody id="logBody">

                    <tr>

                        <td
                            colspan="7"
                            class="empty"
                        >
                            Waiting for accident reports...

                        </td>

                    </tr>

                </tbody>

            </table>

        </div>

    </section>


</main>


<!-- ======================================================
     NOTIFICATION
     ====================================================== -->

<div id="notification">

    <div class="notification-title">

        🚨 NEW ACCIDENT REGISTERED

    </div>

    <div
        id="notificationText"
        class="notification-text"
    >
        Emergency alert received.
    </div>

</div>


<script>


let lastAccidentId = null;

let currentIncidentId = null;


// ========================================================
// ESCAPE HTML
// ========================================================

function escapeHTML(value) {

    if (value === null ||
        value === undefined) {

        return "—";
    }

    return String(value)
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
}


// ========================================================
// SEVERITY CLASS
// ========================================================

function severityClass(severity) {

    const value =
        String(severity || "")
        .toLowerCase();

    if (value.includes("critical")) {
        return "severity-critical";
    }

    if (value.includes("severe")) {
        return "severity-severe";
    }

    if (value.includes("moderate")) {
        return "severity-moderate";
    }

    return "severity-minor";
}


// ========================================================
// SHOW NOTIFICATION
// ========================================================

function showNotification(accident) {

    const notification =
        document.getElementById(
            "notification"
        );


    document.getElementById(
        "notificationText"
    ).innerHTML =

        "Vehicle <b>" +
        escapeHTML(
            accident.vehicle_id
        ) +

        "</b> reported a <b>" +

        escapeHTML(
            accident.severity
        ) +

        "</b> accident at " +

        escapeHTML(
            accident.timestamp
        ) + ".";


    notification.classList.add(
        "show"
    );


    setTimeout(() => {

        notification.classList.remove(
            "show"
        );

    }, 7000);
}


// ========================================================
// LOAD DASHBOARD
// ========================================================

async function loadDashboard() {

    try {

        const response =
            await fetch(
                "/api/dashboard"
            );


        const data =
            await response.json();


        // ------------------------------------------------
        // STATISTICS
        // ------------------------------------------------

        document.getElementById(
            "totalAccidents"
        ).textContent =
            data.total_accidents;


        document.getElementById(
            "todayAccidents"
        ).textContent =
            data.today_accidents;


        // ------------------------------------------------
        // LATEST ACCIDENT
        // ------------------------------------------------

        if (data.latest) {

            const accident =
                data.latest;


            currentIncidentId =
                accident.id;


            document.getElementById(
                "incident"
            ).classList.add(
                "active"
            );


            document.getElementById(
                "incidentId"
            ).textContent =
                accident.id;


            document.getElementById(
                "incidentVehicle"
            ).textContent =
                accident.vehicle_id;


            document.getElementById(
                "incidentType"
            ).textContent =
                accident.type;


            document.getElementById(
                "incidentSeverity"
            ).textContent =
                accident.severity;


            document.getElementById(
                "incidentLat"
            ).textContent =
                accident.latitude;


            document.getElementById(
                "incidentLon"
            ).textContent =
                accident.longitude;


            document.getElementById(
                "incidentTime"
            ).textContent =
                accident.timestamp;


            document.getElementById(
                "incidentStatus"
            ).textContent =
                accident.status;


            // Map link
            document.getElementById(
                "mapButton"
            ).href =

                "https://www.google.com/maps?q=" +
                accident.latitude +
                "," +
                accident.longitude;


            // Status styling
            const statusElement =
                document.getElementById(
                    "incidentStatus"
                );


            statusElement.className =
                accident.status ===
                "ACKNOWLEDGED"
                    ? "status-ack"
                    : "";


            // ------------------------------------------------
            // NEW ACCIDENT DETECTION
            // ------------------------------------------------

            if (
                lastAccidentId !== null &&
                accident.id > lastAccidentId
            ) {

                showNotification(
                    accident
                );

                document.getElementById(
                    "newBadge"
                ).style.display =
                    "inline-block";

            }


            lastAccidentId =
                accident.id;
        }


        // ------------------------------------------------
        // LOG TABLE
        // ------------------------------------------------

        const logBody =
            document.getElementById(
                "logBody"
            );


        if (
            !data.accidents ||
            data.accidents.length === 0
        ) {

            logBody.innerHTML = `

                <tr>

                    <td
                        colspan="7"
                        class="empty"
                    >
                        No accidents have been
                        reported yet.

                    </td>

                </tr>

            `;

            return;
        }


        logBody.innerHTML =
            data.accidents.map(
                accident => `

                    <tr>

                        <td>
                            <b>#${escapeHTML(
                                accident.id
                            )}</b>
                        </td>

                        <td>
                            ${escapeHTML(
                                accident.vehicle_id
                            )}
                        </td>

                        <td>
                            ${escapeHTML(
                                accident.type
                            )}
                        </td>

                        <td>

                            <span
                                class="severity
                                ${severityClass(
                                    accident.severity
                                )}"
                            >

                                ${escapeHTML(
                                    accident.severity
                                )}

                            </span>

                        </td>

                        <td>

                            <a
                                href="https://www.google.com/maps?q=${accident.latitude},${accident.longitude}"
                                target="_blank"
                                style="
                                    color:#60a5fa;
                                    text-decoration:none;
                                "
                            >

                                ${escapeHTML(
                                    accident.latitude
                                )},
                                ${escapeHTML(
                                    accident.longitude
                                )}

                            </a>

                        </td>

                        <td>
                            ${escapeHTML(
                                accident.timestamp
                            )}
                        </td>

                        <td>

                            <span
                                class="${
                                    accident.status ===
                                    "ACKNOWLEDGED"
                                        ? "status-ack"
                                        : "status"
                                }"
                            >

                                ${escapeHTML(
                                    accident.status
                                )}

                            </span>

                        </td>

                    </tr>

                `
            ).join("");


    }
    catch (error) {

        console.error(
            "Dashboard error:",
            error
        );
    }
}


// ========================================================
// ACKNOWLEDGE
// ========================================================

document.getElementById(
    "ackButton"
).addEventListener(
    "click",
    async () => {

        if (!currentIncidentId) {
            return;
        }


        try {

            await fetch(
                "/api/accident/" +
                currentIncidentId +
                "/acknowledge",
                {
                    method: "POST"
                }
            );


            document.getElementById(
                "newBadge"
            ).style.display =
                "none";


            loadDashboard();

        }
        catch (error) {

            console.error(
                error
            );
        }

    }
);


// ========================================================
// INITIAL LOAD
// ========================================================

loadDashboard();


// ========================================================
// LIVE UPDATES
// ========================================================

setInterval(
    loadDashboard,
    2000
);


</script>


</body>

</html>
    """


# ============================================================
# START SERVER
# ============================================================

if __name__ == "__main__":

    port = int(
        os.environ.get(
            "PORT",
            5000
        )
    )

    app.run(
        host="0.0.0.0",
        port=port,
        debug=False
    )
