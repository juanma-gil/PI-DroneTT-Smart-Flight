import json
import socket
import time as t
# Read the JSON file

with open('res/coordinates.json') as file:
    json_data = json.load(file)
    # Convert JSON to string
    json_str = json.dumps(json_data)

# TCP server IP and port
server_ip = '192.168.1.196'
server_port = 5001

# Create a TCP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

try:
    # Connect to the server
    print("Connecting...")
    sock.connect((server_ip, server_port))
    print("Connected")
    # Send the JSON data
    t.sleep(5)
    sock.sendall(json_str.encode('ascii'))

    while True:
        response = sock.recv(1024)  # Adjust the buffer size as needed
        if not response:
            break
        print("Received response:", response.decode('ascii'))

finally:
    # Close the socket
    sock.close()
