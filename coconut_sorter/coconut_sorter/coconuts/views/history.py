# views/history.py
import tkinter as tk
from tkinter import ttk
from datetime import datetime, timedelta
import numpy as np
import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from data_manager import get_data_by_date_range, aggregate_by_date, export_to_csv

class HistoryView:
    def __init__(self, parent):
        self.history_view = tk.Frame(parent, bg="white")
        self.current_records = []
        self.build_ui()

    def build_ui(self):
        """Build data history UI"""
        # Date range section
        date_frame = tk.Frame(self.history_view, bg="#ecf0f1")
        date_frame.pack(fill=tk.X, padx=10, pady=10)

        date_label = tk.Label(
            date_frame,
            text="Data History",
            font=("Arial", 14, "bold"),
            bg="#ecf0f1"
        )
        date_label.pack(side=tk.LEFT, padx=10, pady=10)

        # Date picker frame
        picker_frame = tk.Frame(date_frame, bg="white")
        picker_frame.pack(side=tk.LEFT, padx=10, pady=5)

        tk.Label(picker_frame, text="From:", bg="white").pack(side=tk.LEFT, padx=5)
        self.date_from = tk.Entry(picker_frame, width=15)
        self.date_from.pack(side=tk.LEFT, padx=5)
        self.date_from.insert(0, (datetime.now() - timedelta(days=7)).strftime("%Y-%m-%d"))

        tk.Label(picker_frame, text="To:", bg="white").pack(side=tk.LEFT, padx=5)
        self.date_to = tk.Entry(picker_frame, width=15)
        self.date_to.pack(side=tk.LEFT, padx=5)
        self.date_to.insert(0, datetime.now().strftime("%Y-%m-%d"))

        self.pull_btn = tk.Button(
            picker_frame,
            text="PULL DATA",
            command=self.pull_data,
            bg="#27ae60",
            fg="white",
            font=("Arial", 10, "bold"),
            padx=20,
            pady=5
        )
        self.pull_btn.pack(side=tk.LEFT, padx=10)

        # Graph section
        graph_frame = tk.Frame(self.history_view, bg="white")
        graph_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        graph_title = tk.Label(
            graph_frame,
            text="Display Data",
            font=("Arial", 12, "bold"),
            bg="white"
        )
        graph_title.pack(anchor="w", padx=10, pady=5)

        self.history_fig, self.history_ax = plt.subplots(figsize=(10, 3), dpi=100)
        self.history_ax.set_title("Coconut Sorting Trends", fontsize=11, fontweight="bold")
        self.history_ax.set_xlabel("Date")
        self.history_ax.set_ylabel("Count")
        self.history_ax.grid(True, alpha=0.3)

        self.history_canvas = FigureCanvasTkAgg(self.history_fig, master=graph_frame)
        self.history_canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

        # Table section
        table_frame = tk.Frame(self.history_view, bg="white")
        table_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        table_title = tk.Label(
            table_frame,
            text="Data History",
            font=("Arial", 12, "bold"),
            bg="white"
        )
        table_title.pack(anchor="w", padx=5, pady=5)

        # Treeview for table
        columns = ('Date', 'Time', 'Malauhog', 'Malakanin', 'Malakatad')
        self.history_tree = ttk.Treeview(table_frame, columns=columns, height=8, show='headings')

        for col in columns:
            self.history_tree.heading(col, text=col)
            self.history_tree.column(col, width=100, anchor='center')

        self.history_tree.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Scrollbar
        scrollbar = ttk.Scrollbar(table_frame, orient=tk.VERTICAL, command=self.history_tree.yview)
        self.history_tree.configure(yscroll=scrollbar.set)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        # Export button
        self.export_btn = tk.Button(
            self.history_view,
            text="📥 Export to CSV",
            command=self.export_csv,
            bg="#3498db",
            fg="white",
            font=("Arial", 11, "bold"),
            padx=20,
            pady=8
        )
        self.export_btn.pack(pady=10)

    def pull_data(self):
        """Pull and display historical data"""
        try:
            start_str = self.date_from.get()
            end_str = self.date_to.get()

            start_date = datetime.strptime(start_str, "%Y-%m-%d").date()
            end_date = datetime.strptime(end_str, "%Y-%m-%d").date()

            self.current_records = get_data_by_date_range(start_date, end_date)
            self.display_graph()
            self.display_table()
        except Exception as e:
            print(f"Error loading data: {e}")

    def display_graph(self):
        """Display historical data as line graph"""
        aggregated = aggregate_by_date(self.current_records)

        if not aggregated:
            self.history_ax.clear()
            self.history_ax.set_title("No data available for selected range")
            self.history_canvas.draw_idle()
            return

        dates = sorted(aggregated.keys())
        malauhog_counts = [aggregated[d]['Malauhog'] for d in dates]
        malakatad_counts = [aggregated[d]['Malakatad'] for d in dates]
        malakanin_counts = [aggregated[d]['Malakanin'] for d in dates]

        self.history_ax.clear()
        self.history_ax.plot(dates, malauhog_counts, marker='o', label='Malauhog', color='#27ae60')
        self.history_ax.plot(dates, malakatad_counts, marker='s', label='Malakatad', color='#2980b9')
        self.history_ax.plot(dates, malakanin_counts, marker='^', label='Malakanin', color='#8e44ad')

        self.history_ax.set_title("Coconut Sorting Trends", fontsize=11, fontweight="bold")
        self.history_ax.set_xlabel("Date")
        self.history_ax.set_ylabel("Count")
        self.history_ax.legend()
        self.history_ax.grid(True, alpha=0.3)
        self.history_fig.tight_layout()
        self.history_canvas.draw_idle()

    def display_table(self):
        """Display historical data in table"""
        # Clear existing items
        for item in self.history_tree.get_children():
            self.history_tree.delete(item)

        # Aggregate by date and time
        data_dict = {}
        for record in self.current_records:
            key = (record['date'], record['time'])
            if key not in data_dict:
                data_dict[key] = {'Malauhog': 0, 'Malakatad': 0, 'Malakanin': 0}
            data_dict[key][record['type']] += 1

        # Sort by date and time descending
        sorted_data = sorted(data_dict.items(), reverse=True)

        # Insert into table
        for (date, time), counts in sorted_data:
            self.history_tree.insert('', 'end', values=(
                date,
                time,
                counts.get('Malauhog', 0),
                counts.get('Malakanin', 0),
                counts.get('Malakatad', 0)
            ))

    def export_csv(self):
        """Export current records to CSV"""
        try:
            if export_to_csv(self.current_records):
                print("✓ Data exported to coconut_data.csv")
            else:
                print("✗ Export failed")
        except Exception as e:
            print(f"Error exporting: {e}")

    def get_frame(self):
        """Return the frame widget"""
        return self.history_view
