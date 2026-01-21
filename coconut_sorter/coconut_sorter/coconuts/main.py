# main.py
import cv2
import tkinter as tk
from PIL import Image, ImageTk
import numpy as np
import threading
import sounddevice as sd
from collections import deque

# ===== Matplotlib backend MUST be first =====
import matplotlib
matplotlib.use("TkAgg")

from vision import detect_coconut
from sound_analyzer import analyze_tap
from serial_comm import send, read_sensor, init_serial
from data_manager import add_record

# Import views
from views.dashboard import DashboardView
from views.monitoring import MonitoringView
from views.history import HistoryView


class CoconutSorter:
    def __init__(self, root):
        self.root = root
        self.root.title("Coconut Sorting System Dashboard")
        self.root.geometry("1200x800")

        # === INIT SERIAL SAFELY ===
        init_serial()

        # === COUNTERS ===
        self.total = 0
        self.coconut_count = 0
        self.malauhog_count = 0
        self.malakatad_count = 0
        self.malakanin_count = 0

        # === CAMERA ===
        self.video = cv2.VideoCapture(1)
        self.video.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        self.video.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

        # === STATE ===
        self.last_state = "NONE"
        self.processing = False
        self.audio_buffer = deque(maxlen=8820)  # 0.2 sec at 44100 Hz

        # === CREATE DASHBOARD ===
        self.create_dashboard()

        # === START LOOPS ===
        self.update_camera()
        self.monitoring_update()

    # ================= DASHBOARD LAYOUT =================
    def create_dashboard(self):
        """Create main dashboard with navigation"""
        # Top navigation bar
        nav_frame = tk.Frame(self.root, bg="#2c3e50", height=60)
        nav_frame.pack(fill=tk.X, side=tk.TOP)
        nav_frame.pack_propagate(False)

        title_label = tk.Label(
            nav_frame,
            text="🥥 COCONUT SORTING SYSTEM",
            font=("Arial", 18, "bold"),
            bg="#50e466",
            fg="white"
        )
        title_label.pack(side=tk.LEFT, padx=20, pady=10)

        # Navigation buttons
        btn_main = tk.Button(
            nav_frame,
            text="📊 Main Dashboard",
            command=self.show_main_view,
            bg="#3498db",
            fg="white",
            font=("Arial", 11, "bold"),
            relief=tk.FLAT,
            padx=15,
            pady=8
        )
        btn_main.pack(side=tk.LEFT, padx=5)

        btn_monitoring = tk.Button(
            nav_frame,
            text="🔊 Monitoring",
            command=self.show_monitoring_view,
            bg="#27ae60",
            fg="white",
            font=("Arial", 11, "bold"),
            relief=tk.FLAT,
            padx=15,
            pady=8
        )
        btn_monitoring.pack(side=tk.LEFT, padx=5)

        btn_history = tk.Button(
            nav_frame,
            text="📈 Data History",
            command=self.show_history_view,
            bg="#e74c3c",
            fg="white",
            font=("Arial", 11, "bold"),
            relief=tk.FLAT,
            padx=15,
            pady=8
        )
        btn_history.pack(side=tk.LEFT, padx=5)

        # Content area
        self.content_frame = tk.Frame(self.root)
        self.content_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        # Create views
        self.dashboard = DashboardView(self.content_frame)
        self.monitoring = MonitoringView(self.content_frame)
        self.history = HistoryView(self.content_frame)

        # Show main view by default
        self.show_main_view()

    # ================= VIEW MANAGEMENT =================
    def show_main_view(self):
        """Switch to main dashboard view"""
        self.monitoring.monitoring_view.pack_forget()
        self.history.history_view.pack_forget()
        self.dashboard.main_view.pack(fill=tk.BOTH, expand=True)

    def show_monitoring_view(self):
        """Switch to monitoring view"""
        self.dashboard.main_view.pack_forget()
        self.history.history_view.pack_forget()
        self.monitoring.monitoring_view.pack(fill=tk.BOTH, expand=True)

    def show_history_view(self):
        """Switch to history view"""
        self.dashboard.main_view.pack_forget()
        self.monitoring.monitoring_view.pack_forget()
        self.history.history_view.pack(fill=tk.BOTH, expand=True)
        self.history.pull_data()  # Auto-load default date range

    # ================= CAMERA LOOP (FAST) =================
    def update_camera(self):
        """Update camera feed and detect coconuts"""
        ret, frame = self.video.read()
        if ret:
            result = detect_coconut(frame)

            # Draw detection result on frame
            h, w = frame.shape[:2]
            if result == "COCONUT":
                color = (0, 255, 0)  # Green
                text = "COCONUT DETECTED ✓"
            elif result == "NON_COCONUT":
                color = (0, 165, 255)  # Orange
                text = "NON-COCONUT"
            else:
                color = (100, 100, 100)  # Gray
                text = "Searching..."

            cv2.putText(frame, text, (20, 40), cv2.FONT_HERSHEY_SIMPLEX,
                       1, color, 2)

            if result == "COCONUT" and not self.processing and self.last_state != "COCONUT":
                self.processing = True
                self.total += 1
                self.coconut_count += 1

                self.dashboard.status.config(
                    text=f"Coconut #{self.coconut_count} detected!",
                    fg="#27ae60"
                )
                self.dashboard.process_status.config(text="Waiting for tap...")

                # 🚀 PROCESS IN BACKGROUND THREAD
                threading.Thread(
                    target=self.process_coconut,
                    daemon=True
                ).start()

            self.last_state = result

            frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            img = ImageTk.PhotoImage(Image.fromarray(frame))
            self.dashboard.video_label.configure(image=img)
            self.dashboard.video_label.image = img

        self.root.after(15, self.update_camera)  # ~60 FPS

    # ================= BACKGROUND PROCESS =================
    def process_coconut(self):
        """Process coconut in background thread"""
        tap_done = self.wait_for_tap_done()
        if not tap_done:
            self.root.after(0, lambda: self.dashboard.process_status.config(
                text="Tap timeout – skipped"
            ))
            self.processing = False
            return

        self.root.after(0, lambda: self.dashboard.process_status.config(
            text="Analyzing sound..."
        ))

        tap_type, signal = analyze_tap()

        self.root.after(0, lambda: self.update_after_analysis(tap_type, signal))

    # ================= UI UPDATE =================
    def update_after_analysis(self, tap_type, signal):
        """Update UI after tap analysis"""
        if tap_type == "MALAUHOG":
            self.malauhog_count += 1
            self.dashboard.status.config(text="🟢 MALAUHOG Coconut", fg="#27ae60")
        elif tap_type == "MALAKATAD":
            self.malakatad_count += 1
            self.dashboard.status.config(text="🔵 MALAKATAD Coconut", fg="#2980b9")
        else:
            self.malakanin_count += 1
            self.dashboard.status.config(text="🟣 MALAKANIN Coconut", fg="#8e44ad")

        # Save to data history
        add_record(tap_type)

        send(tap_type)

        self.dashboard.counter.config(
            text=f"Total: {self.total}\nCoconuts: {self.coconut_count}"
        )

        self.dashboard.type_label.config(
            text=f"Malauhog: {self.malauhog_count}\n"
                 f"Malakatad: {self.malakatad_count}\n"
                 f"Malakanin: {self.malakanin_count}"
        )

        self.dashboard.process_status.config(text=f"✓ Sorted as {tap_type}")
        self.processing = False

    # ================= REAL-TIME MONITORING =================
    def monitoring_update(self):
        """Update real-time monitoring graph"""
        if self.monitoring.is_active():
            try:
                # Record small chunk of audio
                audio_chunk = sd.rec(int(0.1 * 44100), samplerate=44100,
                                    channels=1, blocking=False)
                sd.wait()

                if audio_chunk is not None and len(audio_chunk) > 0:
                    signal = audio_chunk[:, 0]
                    self.audio_buffer.extend(signal)

                    if len(self.audio_buffer) > 100:
                        self.monitoring.update_line(np.array(list(self.audio_buffer)))
            except Exception as e:
                print(f"Monitoring error: {e}")

        self.root.after(100, self.monitoring_update)

    # ================= PLOT UPDATE =================
    def update_plot(self, signal):
        """Update the waveform plot"""
        # This would update the main dashboard plot if needed
        pass

    # ================= ARDUINO WAIT =================
    def wait_for_tap_done(self, timeout=5000):
        start = self.root.tk.call('clock', 'milliseconds')

        while True:
            msg = read_sensor()
            if msg == "TAP_DONE":
                return True

            now = self.root.tk.call('clock', 'milliseconds')
            if now - start > timeout:
                return False

    def on_close(self):
        self.video.release()
        send("CLOSE")
        self.root.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    app = CoconutSorter(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()
