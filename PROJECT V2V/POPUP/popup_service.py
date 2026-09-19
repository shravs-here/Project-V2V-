import serial
import threading
import queue
import tkinter as tk
import time
import json
import webbrowser


# ============================================================
# CONFIGURATION
# ============================================================

PORTS = {
    "ESP1": "COM4",   # Accident vehicle
    "ESP2": "COM3"    # Nearby vehicle
}

BAUD_RATE = 115200


# ============================================================
# COLORS
# ============================================================

BG = "#0B1220"
CARD = "#111A2E"
CARD_2 = "#18233A"

TEXT = "#F8FAFC"
SUBTEXT = "#94A3B8"
MUTED = "#64748B"

# ESP1 - local
ORANGE = "#F59E0B"
ORANGE_DARK = "#D97706"
ORANGE_BG = "#2A2110"

# ESP2 - remote
RED = "#EF4444"
RED_DARK = "#DC2626"
RED_BG = "#2A1417"

GREEN = "#22C55E"
GREEN_BG = "#10261A"

BUTTON = "#24324A"
BUTTON_HOVER = "#33445F"

WHITE = "#FFFFFF"


# ============================================================
# GLOBALS
# ============================================================

events = queue.Queue()

serial_connections = {}

root = None

# Keeps track of the ESP1 local popup
local_popup = None


# ============================================================
# SERIAL READER
# ============================================================

def serial_reader(vehicle_name, port):

    while True:

        try:

            ser = serial.Serial(
                port,
                BAUD_RATE,
                timeout=1
            )

            serial_connections[vehicle_name] = ser

            print(
                f"[{vehicle_name}] Connected to {port}"
            )

            while True:

                line = ser.readline().decode(
                    "utf-8",
                    errors="ignore"
                ).strip()

                if not line:
                    continue

                print(
                    f"[{vehicle_name}] {line}"
                )

                events.put(
                    (vehicle_name, line)
                )

        except Exception:

            print(
                f"[{vehicle_name}] Waiting for {port}..."
            )

            serial_connections.pop(
                vehicle_name,
                None
            )

            time.sleep(2)


# ============================================================
# SEND COMMAND
# ============================================================

def send_command(vehicle_name, command):

    ser = serial_connections.get(vehicle_name)

    if ser is None:

        print(
            f"[{vehicle_name}] Cannot send {command}: "
            "serial not connected"
        )

        return

    try:

        ser.write(
            (command + "\n").encode()
        )

        ser.flush()

        print(
            f"[{vehicle_name}] >> {command}"
        )

    except Exception as e:

        print(
            f"[{vehicle_name}] Send error: {e}"
        )


# ============================================================
# WINDOW HELPERS
# ============================================================

def make_window():

    win = tk.Toplevel(root)

    win.configure(
        bg=BG
    )

    win.attributes(
        "-topmost",
        True
    )

    win.resizable(
        False,
        False
    )

    return win


def center_window(win, width, height):

    win.update_idletasks()

    screen_width = win.winfo_screenwidth()
    screen_height = win.winfo_screenheight()

    x = (screen_width - width) // 2
    y = (screen_height - height) // 2

    win.geometry(
        f"{width}x{height}+{x}+{y}"
    )


# ============================================================
# BUTTON
# ============================================================

def make_button(
    parent,
    text,
    command,
    x,
    y,
    width,
    bg=BUTTON
):

    button = tk.Button(
        parent,
        text=text,
        command=command,
        font=("Segoe UI", 11, "bold"),
        fg=WHITE,
        bg=bg,
        activebackground=BUTTON_HOVER,
        activeforeground=WHITE,
        relief="flat",
        bd=0,
        cursor="hand2"
    )

    button.place(
        x=x,
        y=y,
        width=width,
        height=50
    )

    return button


# ============================================================
# INFO BOX
# ============================================================

def info_box(parent, x, y, width, label, value):

    frame = tk.Frame(
        parent,
        bg=CARD_2
    )

    frame.place(
        x=x,
        y=y,
        width=width,
        height=68
    )

    tk.Label(
        frame,
        text=label.upper(),
        font=("Segoe UI", 8, "bold"),
        fg=MUTED,
        bg=CARD_2
    ).place(
        x=13,
        y=8
    )

    tk.Label(
        frame,
        text=str(value),
        font=("Segoe UI", 11, "bold"),
        fg=TEXT,
        bg=CARD_2
    ).place(
        x=13,
        y=30
    )


# ============================================================
# GOOGLE MAPS
# ============================================================

def open_location(latitude, longitude):

    try:

        lat = float(latitude)
        lon = float(longitude)

        url = (
            "https://www.google.com/maps?q="
            f"{lat},{lon}"
        )

        print(
            f"[MAP] Opening: {lat}, {lon}"
        )

        webbrowser.open(
            url
        )

    except Exception as e:

        print(
            f"[MAP] Invalid GPS coordinates: {e}"
        )


# ============================================================
# RESULT SCREEN
# ============================================================

def show_result(
    win,
    title,
    message,
    accent,
    panel_bg
):

    for widget in win.winfo_children():
        widget.destroy()

    win.configure(
        bg=CARD
    )

    center_window(
        win,
        680,
        430
    )

    tk.Label(
        win,
        text="V2V EMERGENCY SYSTEM",
        font=("Segoe UI", 10, "bold"),
        fg=SUBTEXT,
        bg=CARD
    ).pack(
        pady=(45, 10)
    )

    tk.Label(
        win,
        text=title,
        font=("Segoe UI", 27, "bold"),
        fg=TEXT,
        bg=CARD
    ).pack(
        pady=8
    )

    panel = tk.Frame(
        win,
        bg=panel_bg
    )

    panel.pack(
        padx=40,
        pady=20,
        fill="x"
    )

    tk.Label(
        panel,
        text=message,
        font=("Segoe UI", 11),
        fg=TEXT,
        bg=panel_bg,
        justify="center"
    ).pack(
        pady=25
    )

    make_button(
        win,
        "CLOSE",
        win.destroy,
        240,
        340,
        200,
        bg=accent
    )


# ============================================================
# CLOSE ESP1 LOCAL POPUP
# ============================================================

def close_local_popup():

    global local_popup

    if local_popup is not None:

        try:

            if local_popup.winfo_exists():

                print(
                    "[ESP1] Reset detected -> closing local popup"
                )

                local_popup.destroy()

        except Exception:
            pass

    local_popup = None


# ============================================================
# ESP1 — LOCAL ACCIDENT POPUP
# ============================================================

def show_local_accident_popup():

    global local_popup

    # Prevent duplicate local popups
    if local_popup is not None:

        try:

            if local_popup.winfo_exists():
                return

        except Exception:
            pass

    win = make_window()

    local_popup = win

    # Make sure closing the window manually
    # also clears the popup reference.
    def on_close():

        global local_popup

        local_popup = None

        win.destroy()

    win.protocol(
        "WM_DELETE_WINDOW",
        on_close
    )

    width = 700
    height = 610

    center_window(
        win,
        width,
        height
    )

    # ========================================================
    # HEADER
    # ========================================================

    header = tk.Frame(
        win,
        bg=ORANGE
    )

    header.place(
        x=0,
        y=0,
        width=width,
        height=95
    )

    tk.Label(
        header,
        text="ESP1",
        font=("Segoe UI", 16, "bold"),
        fg=WHITE,
        bg=ORANGE
    ).place(
        x=30,
        y=14
    )

    tk.Label(
        header,
        text="LOCAL ACCIDENT VEHICLE",
        font=("Segoe UI", 10, "bold"),
        fg="#FFF7ED",
        bg=ORANGE
    ).place(
        x=31,
        y=48
    )

    tk.Label(
        header,
        text="⚠",
        font=("Segoe UI", 34),
        fg=WHITE,
        bg=ORANGE
    ).place(
        x=620,
        y=20
    )

    # ========================================================
    # BODY
    # ========================================================

    body = tk.Frame(
        win,
        bg=CARD
    )

    body.place(
        x=0,
        y=95,
        width=width,
        height=515
    )

    tk.Label(
        body,
        text="ACCIDENT DETECTED",
        font=("Segoe UI", 28, "bold"),
        fg=TEXT,
        bg=CARD
    ).place(
        x=35,
        y=25
    )

    tk.Label(
        body,
        text="The vehicle's crash detection system has triggered.",
        font=("Segoe UI", 11),
        fg=SUBTEXT,
        bg=CARD
    ).place(
        x=37,
        y=70
    )

    # ========================================================
    # COUNTDOWN PANEL
    # ========================================================

    countdown_panel = tk.Frame(
        body,
        bg=ORANGE_BG
    )

    countdown_panel.place(
        x=35,
        y=110,
        width=630,
        height=135
    )

    tk.Label(
        countdown_panel,
        text="EMERGENCY ALERT WILL BE SENT AUTOMATICALLY",
        font=("Segoe UI", 10, "bold"),
        fg=ORANGE,
        bg=ORANGE_BG
    ).pack(
        pady=(12, 2)
    )

    countdown_label = tk.Label(
        countdown_panel,
        text="5",
        font=("Segoe UI", 38, "bold"),
        fg=ORANGE,
        bg=ORANGE_BG
    )

    countdown_label.pack()

    tk.Label(
        countdown_panel,
        text="PRESS CANCEL IF THIS IS A FALSE ALARM",
        font=("Segoe UI", 9),
        fg="#FDE68A",
        bg=ORANGE_BG
    ).pack()

    # ========================================================
    # INFO
    # ========================================================

    info_box(
        body,
        35,
        265,
        190,
        "ESP SOURCE",
        "ESP1"
    )

    info_box(
        body,
        235,
        265,
        190,
        "VEHICLE",
        "V2V-001"
    )

    info_box(
        body,
        435,
        265,
        230,
        "STATUS",
        "AWAITING DECISION"
    )

    # ========================================================
    # STATE
    # ========================================================

    state = {
        "finished": False
    }

    # ========================================================
    # CANCEL
    # ========================================================

    def cancel_accident():

        if state["finished"]:
            return

        state["finished"] = True

        send_command(
            "ESP1",
            "CANCEL"
        )

        show_result(
            win,
            "ALERT CANCELLED",
            "The accident alert was cancelled.\n"
            "The vehicle is returning to monitoring.",
            GREEN,
            GREEN_BG
        )

    # ========================================================
    # CONFIRM
    # ========================================================

    def confirm_accident():

        if state["finished"]:
            return

        state["finished"] = True

        send_command(
            "ESP1",
            "CONFIRM"
        )

        show_result(
            win,
            "EMERGENCY CONFIRMED",
            "Emergency response has been triggered.\n"
            "V2V and Internet emergency alerts are being sent.",
            RED,
            RED_BG
        )

    # ========================================================
    # BUTTONS
    # ========================================================

    make_button(
        body,
        "CANCEL — FALSE ALARM",
        cancel_accident,
        35,
        365,
        300,
        bg=BUTTON
    )

    make_button(
        body,
        "CONFIRM EMERGENCY",
        confirm_accident,
        365,
        365,
        300,
        bg=ORANGE_DARK
    )

    # ========================================================
    # COUNTDOWN
    # ========================================================

    def countdown(seconds):

        if not win.winfo_exists():
            return

        if state["finished"]:
            return

        countdown_label.config(
            text=str(seconds)
        )

        if seconds <= 0:

            confirm_accident()

            return

        win.after(
            1000,
            lambda: countdown(
                seconds - 1
            )
        )

    win.after(
        500,
        lambda: countdown(5)
    )


# ============================================================
# ESP2 — REMOTE V2V POPUP
# ============================================================

def show_remote_accident_popup(data):

    win = make_window()

    width = 740
    height = 620

    center_window(
        win,
        width,
        height
    )

    # ========================================================
    # HEADER
    # ========================================================

    header = tk.Frame(
        win,
        bg=RED
    )

    header.place(
        x=0,
        y=0,
        width=width,
        height=95
    )

    tk.Label(
        header,
        text="ESP2",
        font=("Segoe UI", 16, "bold"),
        fg=WHITE,
        bg=RED
    ).place(
        x=30,
        y=14
    )

    tk.Label(
        header,
        text="REMOTE V2V VEHICLE",
        font=("Segoe UI", 10, "bold"),
        fg="#FEE2E2",
        bg=RED
    ).place(
        x=31,
        y=48
    )

    tk.Label(
        header,
        text="🚨",
        font=("Segoe UI", 34),
        fg=WHITE,
        bg=RED
    ).place(
        x=655,
        y=20
    )

    # ========================================================
    # BODY
    # ========================================================

    body = tk.Frame(
        win,
        bg=CARD
    )

    body.place(
        x=0,
        y=95,
        width=width,
        height=525
    )

    tk.Label(
        body,
        text="REMOTE ACCIDENT ALERT",
        font=("Segoe UI", 28, "bold"),
        fg=TEXT,
        bg=CARD
    ).place(
        x=35,
        y=25
    )

    tk.Label(
        body,
        text="A nearby vehicle has reported an accident through V2V.",
        font=("Segoe UI", 11),
        fg=SUBTEXT,
        bg=CARD
    ).place(
        x=37,
        y=70
    )

    # ========================================================
    # DATA
    # ========================================================

    vehicle = data.get(
        "vehicle",
        data.get(
            "vehicle_id",
            "V2V-001"
        )
    )

    severity = data.get(
        "severity",
        "Unknown"
    )

    latitude = data.get(
        "latitude",
        None
    )

    longitude = data.get(
        "longitude",
        None
    )

    accident_type = data.get(
        "type",
        "ACCIDENT"
    )

    # ========================================================
    # INFO GRID
    # ========================================================

    info_box(
        body,
        35,
        110,
        205,
        "RECEIVER",
        "ESP2"
    )

    info_box(
        body,
        250,
        110,
        205,
        "ACCIDENT VEHICLE",
        vehicle
    )

    info_box(
        body,
        465,
        110,
        240,
        "SEVERITY",
        severity
    )

    info_box(
        body,
        35,
        190,
        325,
        "ACCIDENT TYPE",
        accident_type
    )

    info_box(
        body,
        375,
        190,
        160,
        "LATITUDE",
        latitude if latitude is not None else "N/A"
    )

    info_box(
        body,
        550,
        190,
        155,
        "LONGITUDE",
        longitude if longitude is not None else "N/A"
    )

    # ========================================================
    # NETWORK STATUS
    # ========================================================

    status = tk.Frame(
        body,
        bg=RED_BG
    )

    status.place(
        x=35,
        y=275,
        width=670,
        height=70
    )

    tk.Label(
        status,
        text="●  ESP-NOW ALERT RECEIVED",
        font=("Segoe UI", 10, "bold"),
        fg=RED,
        bg=RED_BG
    ).place(
        x=18,
        y=12
    )

    tk.Label(
        status,
        text="Emergency information forwarded from the nearby vehicle.",
        font=("Segoe UI", 9),
        fg="#FCA5A5",
        bg=RED_BG
    ).place(
        x=18,
        y=39
    )

    # ========================================================
    # LOCATION
    # ========================================================

    def view_location():

        if latitude is None or longitude is None:

            print(
                "[ESP2] GPS coordinates unavailable."
            )

            return

        open_location(
            latitude,
            longitude
        )

    make_button(
        body,
        "📍  SEE LOCATION",
        view_location,
        35,
        375,
        325,
        bg=RED_DARK
    )

    # ========================================================
    # CLOSE
    # ========================================================

    make_button(
        body,
        "CLOSE ALERT",
        win.destroy,
        380,
        375,
        325,
        bg=BUTTON
    )

    print(
        "[ESP2] Remote V2V popup opened."
    )


# ============================================================
# PROCESS SERIAL EVENTS
# ============================================================

def process_event(vehicle_name, line):

    # ========================================================
    # ESP1 LOCAL
    # ========================================================

    if vehicle_name == "ESP1":

        # Detect ESP32 reset / reboot
        if (
            "rst:" in line
            or "V2V SYSTEM STARTING" in line
            or "SYSTEM READY" in line
        ):

            root.after(
                0,
                close_local_popup
            )

        if "ACCIDENT_SUSPECTED" in line:

            print(
                "[ESP1] >>> LOCAL ACCIDENT POPUP"
            )

            root.after(
                0,
                show_local_accident_popup
            )

    # ========================================================
    # ESP2 REMOTE
    # ========================================================

    if vehicle_name == "ESP2":

        if "REMOTE_ACCIDENT|" in line:

            try:

                # Get everything after REMOTE_ACCIDENT|
                json_data = line.split(
                    "REMOTE_ACCIDENT|",
                    1
                )[1]

                # Find JSON object boundaries.
                # This prevents extra serial text from
                # causing json.loads() to fail.
                start = json_data.find("{")
                end = json_data.rfind("}")

                if start == -1 or end == -1:

                    raise ValueError(
                        "JSON object not found"
                    )

                json_data = json_data[
                    start:end + 1
                ]

                data = json.loads(
                    json_data
                )

                print(
                    f"[ESP2] Parsed accident data: {data}"
                )

            except Exception as e:

                print(
                    f"[ESP2] JSON error: {e}"
                )

                data = {
                    "vehicle_id": "V2V-001",
                    "severity": "Unknown",
                    "type": "ACCIDENT",
                    "latitude": None,
                    "longitude": None
                }

            print(
                "[ESP2] >>> REMOTE V2V ACCIDENT POPUP"
            )

            root.after(
                0,
                lambda d=data:
                show_remote_accident_popup(d)
            )


# ============================================================
# EVENT PROCESSOR
# ============================================================

def event_processor():

    try:

        while True:

            vehicle_name, line = events.get_nowait()

            process_event(
                vehicle_name,
                line
            )

    except queue.Empty:
        pass

    root.after(
        100,
        event_processor
    )


# ============================================================
# START SERIAL THREADS
# ============================================================

def start_serial_threads():

    for vehicle_name, port in PORTS.items():

        thread = threading.Thread(
            target=serial_reader,
            args=(
                vehicle_name,
                port
            ),
            daemon=True
        )

        thread.start()


# ============================================================
# MAIN
# ============================================================

def main():

    global root

    root = tk.Tk()

    root.withdraw()

    root.title(
        "V2V Emergency System"
    )

    start_serial_threads()

    root.after(
        100,
        event_processor
    )

    print(
        "=========================================="
    )

    print(
        "       V2V POPUP SERVICE STARTED"
    )

    print(
        "=========================================="
    )

    print(
        "ESP1 -> COM4 -> LOCAL ACCIDENT POPUP"
    )

    print(
        "ESP2 -> COM3 -> REMOTE V2V POPUP"
    )

    print(
        "Listening for ESP-NOW forwarded alerts..."
    )

    print(
        "=========================================="
    )

    root.mainloop()


if __name__ == "__main__":
    main()