# views/monitoring.py
import tkinter as tk
import numpy as np
import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

class MonitoringView:
    def __init__(self, parent):
        self.monitoring_view = tk.Frame(parent, bg="white")
        self.monitoring_active = False
        self.build_ui()

    def build_ui(self):
        """Build monitoring UI"""
        title = tk.Label(
            self.monitoring_view,
            text="Real-Time Sound Acoustic Analysis",
            font=("Arial", 14, "bold"),
            bg="white"
        )
        title.pack(pady=10)

        # Matplotlib figure for sound waveform
        self.fig, self.ax = plt.subplots(figsize=(10, 4), dpi=100)
        self.ax.set_title("Live Microphone Input", fontsize=12, fontweight="bold")
        self.ax.set_ylim(-0.5, 0.5)
        self.ax.set_xlabel("Time (samples)")
        self.ax.set_ylabel("Amplitude")
        self.ax.grid(True, alpha=0.3)
        self.line, = self.ax.plot(np.zeros(1000))

        self.canvas = FigureCanvasTkAgg(self.fig, master=self.monitoring_view)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        # Control buttons
        button_frame = tk.Frame(self.monitoring_view, bg="white")
        button_frame.pack(fill=tk.X, padx=10, pady=10)

        self.monitor_btn = tk.Button(
            button_frame,
            text="▶ Start Monitoring",
            command=self.toggle_monitoring,
            bg="#27ae60",
            fg="white",
            font=("Arial", 12, "bold"),
            padx=20,
            pady=10
        )
        self.monitor_btn.pack(side=tk.LEFT, padx=5)

        self.monitor_status = tk.Label(
            button_frame,
            text="Status: Stopped",
            font=("Arial", 11),
            bg="white",
            fg="#e74c3c"
        )
        self.monitor_status.pack(side=tk.LEFT, padx=20)

    def toggle_monitoring(self):
        """Toggle monitoring on/off"""
        self.monitoring_active = not self.monitoring_active
        if self.monitoring_active:
            self.monitor_btn.config(text="⏹ Stop Monitoring", bg="#e74c3c")
            self.monitor_status.config(text="Status: Recording...", fg="#27ae60")
        else:
            self.monitor_btn.config(text="▶ Start Monitoring", bg="#27ae60")
            self.monitor_status.config(text="Status: Stopped", fg="#e74c3c")

    def is_active(self):
        """Check if monitoring is active"""
        return self.monitoring_active

    def update_line(self, signal):
        """Update the waveform display"""
        self.line.set_ydata(signal)
        self.line.set_xdata(range(len(signal)))
        self.ax.set_xlim(0, len(signal))
        self.canvas.draw_idle()

    def get_frame(self):
        """Return the frame widget"""
        return self.monitoring_view
