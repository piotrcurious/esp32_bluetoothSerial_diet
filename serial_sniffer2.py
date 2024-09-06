import tkinter as tk
from tkinter import scrolledtext
import threading
import telnetlib

# Telnet connection details
INCOMING_HOST = "192.168.4.1"  # Replace with your ESP32's IP address
INCOMING_PORT = 23             # Incoming data stream port
OUTGOING_PORT = 24             # Outgoing data stream port

# Buffer to hold incoming and outgoing data with timestamps
incoming_buffer = []
outgoing_buffer = []

# Timestamp extraction function
def extract_timestamp(data):
    try:
        return int(data.split("]")[0].strip("["))
    except (ValueError, IndexError):
        return 0

# Insert data into the correct buffer based on the direction
def add_data_to_buffer(buffer, data, direction):
    timestamp = extract_timestamp(data)
    buffer.append((timestamp, f"{direction}: {data.strip()}"))

# Read incoming data from Telnet
def read_incoming_data():
    global incoming_buffer
    try:
        with telnetlib.Telnet(INCOMING_HOST, INCOMING_PORT) as tn:
            while True:
                data = tn.read_until(b'\n', timeout=1).decode('utf-8')
                if data:
                    add_data_to_buffer(incoming_buffer, data, "IN")
    except Exception as e:
        print(f"Error reading incoming data: {e}")

# Read outgoing data from Telnet
def read_outgoing_data():
    global outgoing_buffer
    try:
        with telnetlib.Telnet(INCOMING_HOST, OUTGOING_PORT) as tn:
            while True:
                data = tn.read_until(b'\n', timeout=1).decode('utf-8')
                if data:
                    add_data_to_buffer(outgoing_buffer, data, "OUT")
    except Exception as e:
        print(f"Error reading outgoing data: {e}")

# Update the display with synchronized data
def update_display(incoming_text, outgoing_text, slider):
    global incoming_buffer, outgoing_buffer
    last_sync_timestamp = 0

    while True:
        # Sort buffers by timestamps
        combined = sorted(incoming_buffer + outgoing_buffer, key=lambda x: x[0])

        # Clear the text areas before inserting new data
        incoming_text.delete(1.0, tk.END)
        outgoing_text.delete(1.0, tk.END)

        # Display data with padding to keep the timeline clear
        for timestamp, data in combined:
            if "IN" in data:
                incoming_text.insert(tk.END, data + '\n')
                # Add padding to outgoing if needed
                if last_sync_timestamp != timestamp:
                    outgoing_text.insert(tk.END, '\n')
            elif "OUT" in data:
                outgoing_text.insert(tk.END, data + '\n')
                # Add padding to incoming if needed
                if last_sync_timestamp != timestamp:
                    incoming_text.insert(tk.END, '\n')
            last_sync_timestamp = timestamp

        # Adjust the slider range and update position
        slider.config(command=lambda *args: sync_scroll(incoming_text, outgoing_text, *args))
        slider.set(incoming_text.yview()[0])

        incoming_text.update_idletasks()
        outgoing_text.update_idletasks()

        # Update every second
        incoming_buffer.clear()
        outgoing_buffer.clear()
        incoming_text.after(1000)

# Sync scroll positions of the two text areas using the slider
def sync_scroll(incoming_text, outgoing_text, *args):
    incoming_text.yview(*args)
    outgoing_text.yview(*args)

# Main application class
class SerialMonitorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32 Bluetooth Serial Monitor")
        self.root.geometry("1200x600")

        # Create frames for layout
        self.top_frame = tk.Frame(root)
        self.bottom_frame = tk.Frame(root)
        self.top_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        self.bottom_frame.pack(side=tk.BOTTOM, fill=tk.X)

        # Create incoming and outgoing text areas
        self.incoming_text = scrolledtext.ScrolledText(self.top_frame, wrap=tk.WORD, height=30, width=60, bg="#f0f0f0")
        self.outgoing_text = scrolledtext.ScrolledText(self.top_frame, wrap=tk.WORD, height=30, width=60, bg="#f0f0f0")

        # Labels for each stream
        tk.Label(self.top_frame, text="Incoming Data", font=("Arial", 12, "bold")).pack(side=tk.LEFT, anchor="nw")
        tk.Label(self.top_frame, text="Outgoing Data", font=("Arial", 12, "bold")).pack(side=tk.RIGHT, anchor="ne")

        # Pack the text areas
        self.incoming_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.outgoing_text.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        # Create a scrollbar slider to sync scrolling
        self.slider = tk.Scale(self.bottom_frame, from_=0, to=1000, orient=tk.HORIZONTAL, length=600, command=lambda val: sync_scroll(self.incoming_text, self.outgoing_text, 'moveto', val))
        self.slider.pack(side=tk.BOTTOM)

        # Start threads to read data
        threading.Thread(target=read_incoming_data, daemon=True).start()
        threading.Thread(target=read_outgoing_data, daemon=True).start()
        threading.Thread(target=update_display, args=(self.incoming_text, self.outgoing_text, self.slider), daemon=True).start()

# Start the GUI application
if __name__ == "__main__":
    root = tk.Tk()
    app = SerialMonitorApp(root)
    root.mainloop()
