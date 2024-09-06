import tkinter as tk
from tkinter import scrolledtext
import threading
import telnetlib
import time

# Telnet connection details
INCOMING_HOST = "192.168.4.1"  # Replace with your ESP32's IP address
INCOMING_PORT = 23             # Incoming data stream port
OUTGOING_PORT = 24             # Outgoing data stream port
BUFFER_SIZE = 8192             # Buffer size for reading Telnet data

# Timestamp parsing function
def parse_timestamp(line):
    try:
        # Extract the timestamp part from the log line format: "[<timestamp>] IN/OUT: <data>"
        timestamp_str = line.split("]")[0].strip("[")
        return int(timestamp_str)
    except (ValueError, IndexError):
        return 0

# Thread function to read incoming data
def read_incoming_data(text_widget):
    try:
        with telnetlib.Telnet(INCOMING_HOST, INCOMING_PORT) as tn:
            while True:
                data = tn.read_until(b'\n', timeout=1).decode('utf-8')
                if data:
                    add_data_to_text_widget(text_widget, data, "IN")
    except Exception as e:
        print(f"Error reading incoming data: {e}")

# Thread function to read outgoing data
def read_outgoing_data(text_widget):
    try:
        with telnetlib.Telnet(INCOMING_HOST, OUTGOING_PORT) as tn:
            while True:
                data = tn.read_until(b'\n', timeout=1).decode('utf-8')
                if data:
                    add_data_to_text_widget(text_widget, data, "OUT")
    except Exception as e:
        print(f"Error reading outgoing data: {e}")

# Function to add data to the text widget with synchronization
def add_data_to_text_widget(text_widget, data, direction):
    text_widget.insert(tk.END, data)
    text_widget.see(tk.END)  # Auto-scroll to the latest data

# Main application class
class SerialMonitorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32 Bluetooth Serial Monitor")
        self.root.geometry("1200x600")

        # Create frames for side-by-side display
        self.incoming_frame = tk.Frame(root)
        self.outgoing_frame = tk.Frame(root)
        self.incoming_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.outgoing_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        # Create scrolling text widgets
        self.incoming_text = scrolledtext.ScrolledText(self.incoming_frame, wrap=tk.WORD, height=30, width=60, bg="#f0f0f0")
        self.outgoing_text = scrolledtext.ScrolledText(self.outgoing_frame, wrap=tk.WORD, height=30, width=60, bg="#f0f0f0")

        # Add labels
        tk.Label(self.incoming_frame, text="Incoming Data", font=("Arial", 12, "bold")).pack(anchor="nw")
        tk.Label(self.outgoing_frame, text="Outgoing Data", font=("Arial", 12, "bold")).pack(anchor="nw")

        # Pack the text areas
        self.incoming_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.outgoing_text.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        # Start data reading threads
        threading.Thread(target=read_incoming_data, args=(self.incoming_text,), daemon=True).start()
        threading.Thread(target=read_outgoing_data, args=(self.outgoing_text,), daemon=True).start()

# Start the GUI application
if __name__ == "__main__":
    root = tk.Tk()
    app = SerialMonitorApp(root)
    root.mainloop()
