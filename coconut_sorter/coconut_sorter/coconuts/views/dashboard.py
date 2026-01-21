# views/dashboard.py
import tkinter as tk
from tkinter import ttk

class DashboardView:
    def __init__(self, parent):
        self.main_view = tk.Frame(parent, bg="white")
        self.build_ui()

    def build_ui(self):
        """Build main dashboard UI"""
        # Left side - Video feed
        left_frame = tk.Frame(self.main_view, bg="white")
        left_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=10, pady=10)

        video_title = tk.Label(
            left_frame,
            text="Camera Feed - Coconut Detection",
            font=("Arial", 14, "bold"),
            bg="white"
        )
        video_title.pack(pady=5)

        # Video label with border
        video_border = tk.Frame(left_frame, bg="#34495e", highlightthickness=2)
        video_border.pack(fill=tk.BOTH, expand=True)

        self.video_label = tk.Label(video_border, bg="black")
        self.video_label.pack(padx=2, pady=2, fill=tk.BOTH, expand=True)

        # Right side - Statistics and status
        right_frame = tk.Frame(self.main_view, bg="#ecf0f1", width=300)
        right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, padx=10, pady=10)
        right_frame.pack_propagate(False)

        # Status section
        status_label = tk.Label(
            right_frame,
            text="STATUS",
            font=("Arial", 12, "bold"),
            bg="#34495e",
            fg="white"
        )
        status_label.pack(fill=tk.X, pady=(5, 10), padx=5)

        self.status = tk.Label(
            right_frame,
            text="Waiting for coconut...",
            font=("Arial", 16, "bold"),
            bg="white",
            fg="#2980b9",
            wraplength=280
        )
        self.status.pack(fill=tk.X, pady=10, padx=5)

        self.process_status = tk.Label(
            right_frame,
            text="",
            font=("Arial", 10),
            bg="white",
            fg="#7f8c8d"
        )
        self.process_status.pack(fill=tk.X, pady=5, padx=5)

        # Divider
        ttk.Separator(right_frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=10, padx=5)

        # Counters section
        counter_label = tk.Label(
            right_frame,
            text="COUNTERS",
            font=("Arial", 12, "bold"),
            bg="#34495e",
            fg="white"
        )
        counter_label.pack(fill=tk.X, pady=(5, 10), padx=5)

        self.counter = tk.Label(
            right_frame,
            text="Total: 0\nCoconuts: 0",
            font=("Arial", 11),
            bg="white",
            justify=tk.LEFT
        )
        self.counter.pack(fill=tk.X, pady=10, padx=5)

        # Types section
        types_label = tk.Label(
            right_frame,
            text="TYPES",
            font=("Arial", 12, "bold"),
            bg="#34495e",
            fg="white"
        )
        types_label.pack(fill=tk.X, pady=(5, 10), padx=5)

        self.type_label = tk.Label(
            right_frame,
            text="Malauhog: 0\nMalakatad: 0\nMalakanin: 0",
            font=("Arial", 11),
            bg="white",
            justify=tk.LEFT,
            fg="#27ae60"
        )
        self.type_label.pack(fill=tk.X, pady=10, padx=5)

    def get_frame(self):
        """Return the frame widget"""
        return self.main_view
