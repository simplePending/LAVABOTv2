# data_manager.py
import json
import os
from datetime import datetime
import csv

DATA_FILE = "coconut_data.json"

def load_data():
    """Load all historical data"""
    if os.path.exists(DATA_FILE):
        try:
            with open(DATA_FILE, 'r') as f:
                return json.load(f)
        except:
            return []
    return []

def save_data(data):
    """Save historical data"""
    with open(DATA_FILE, 'w') as f:
        json.dump(data, f, indent=2)

def add_record(tap_type):
    """Add a new sorting record"""
    data = load_data()
    record = {
        "date": datetime.now().strftime("%Y-%m-%d"),
        "time": datetime.now().strftime("%H:%M"),
        "type": tap_type,
        "timestamp": datetime.now().isoformat()
    }
    data.append(record)
    save_data(data)
    return record

def get_data_by_date_range(start_date, end_date):
    """Get records within date range"""
    data = load_data()
    filtered = []
    for record in data:
        rec_date = datetime.strptime(record['date'], "%Y-%m-%d").date()
        if start_date <= rec_date <= end_date:
            filtered.append(record)
    return filtered

def aggregate_by_date(records):
    """Aggregate records by date"""
    aggregated = {}
    for record in records:
        date = record['date']
        tap_type = record['type']
        
        if date not in aggregated:
            aggregated[date] = {
                "Malauhog": 0,
                "Malakatad": 0,
                "Malakanin": 0
            }
        
        if tap_type in aggregated[date]:
            aggregated[date][tap_type] += 1
    
    return aggregated

def export_to_csv(records, filename="coconut_data.csv"):
    """Export records to CSV"""
    if not records:
        return False
    
    try:
        with open(filename, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['Date', 'Time', 'Type'])
            for record in records:
                writer.writerow([record['date'], record['time'], record['type']])
        return True
    except Exception as e:
        print(f"Error exporting CSV: {e}")
        return False
