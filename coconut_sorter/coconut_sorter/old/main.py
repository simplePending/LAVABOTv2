# main.py
import cv2
import tkinter as tk
from PIL import Image, ImageTk
from vision import detect_coconut
from sound_analyzer import analyze_tap
from serial_comm import send, read_sensor

class CoconutSorter:
    def __init__(self, root):
        self.root = root
        self.root.title("Coconut Sorting System")
        self.root.geometry("800x600")

        self.total = 0
        self.coconut_count = 0
        self.malauhog_count = 0
        self.malakatad_count = 0
        self.malakanin_count = 0

        self.video = cv2.VideoCapture(1)

        # Video feed
        self.label = tk.Label(root)
        self.label.pack()

        # Status display
        self.status = tk.Label(root, text="Waiting for coconut...", font=("Arial", 24), fg="blue")
        self.status.pack(pady=10)

        # Counters
        self.counter = tk.Label(root, text="Total: 0 | Coconuts: 0", font=("Arial", 16))
        self.counter.pack(pady=5)

        # Type breakdown
        self.type_label = tk.Label(
            root, 
            text="Malauhog: 0 | Malakatad: 0 | Malakanin: 0", 
            font=("Arial", 14),
            fg="darkgreen"
        )
        self.type_label.pack(pady=5)

        # Process status
        self.process_status = tk.Label(root, text="", font=("Arial", 12), fg="gray")
        self.process_status.pack(pady=5)

        self.last_state = "NONE"
        self.processing = False
        self.update()

    def update(self):
        ret, frame = self.video.read()
        if not ret:
            self.root.after(100, self.update)
            return

        result = detect_coconut(frame)

        # If coconut detected and not already processing
        if result == "COCONUT" and self.last_state != "COCONUT" and not self.processing:
            self.processing = True
            self.total += 1
            self.coconut_count += 1

            self.status.config(
                text=f"Coconut #{self.coconut_count} detected!",
                fg="orange"
            )
            self.process_status.config(text="Waiting for tap from Arduino...")
            self.root.update()

            # ✅ Wait for Arduino to finish sensor + delay + tap
            tap_done = self.wait_for_tap_done()

            if not tap_done:
                self.process_status.config(text="Tap timeout – skipped")
                self.processing = False
                return

            # ✅ Analyze sound ONLY ONCE
            self.process_status.config(text="Analyzing sound...")
            self.root.update()
            tap_type = analyze_tap()

            # ✅ Update counts
            if tap_type == "MALAUHOG":
                self.malauhog_count += 1
                self.status.config(text="MALAUHOG Coconut", fg="green")
            elif tap_type == "MALAKATAD":
                self.malakatad_count += 1
                self.status.config(text="MALAKATAD Coconut", fg="blue")
            else:
                self.malakanin_count += 1
                self.status.config(text="MALAKANIN Coconut", fg="purple")

            # ✅ Send result to Arduino
            send(tap_type)

            self.process_status.config(text=f"Sorted as {tap_type} ✓")

            # ✅ Update UI
            self.counter.config(
                text=f"Total: {self.total} | Coconuts: {self.coconut_count}"
            )
            self.type_label.config(
                text=f"Malauhog: {self.malauhog_count} | "
                    f"Malakatad: {self.malakatad_count} | "
                    f"Malakanin: {self.malakanin_count}"
            )

            self.processing = False

        elif result == "NON_COCONUT" and self.last_state != "NON_COCONUT":
            self.total += 1
            self.status.config(text=f"Non-coconut object detected", fg="red")
            self.process_status.config(text="Rejected")

        self.last_state = result

        # Display video feed
        frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        img = Image.fromarray(frame)
        imgtk = ImageTk.PhotoImage(img)
        self.label.imgtk = imgtk
        self.label.configure(image=imgtk)

        self.root.after(150, self.update)

    def wait_for_tap_done(self, timeout=5000):
        start = self.root.tk.call('clock', 'milliseconds')

        while True:
            msg = read_sensor()

            if msg == "TAP_DONE":
                return True

            now = self.root.tk.call('clock', 'milliseconds')
            if now - start > timeout:
                return False

            self.root.update()

    def on_close(self):
        self.video.release()
        send("CLOSE")  # Signal Arduino to clean up
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = CoconutSorter(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()