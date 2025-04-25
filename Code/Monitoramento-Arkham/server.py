from flask import Flask, request, jsonify
from flask_cors import CORS
import time
from threading import Lock
import json

app = Flask(__name__)
CORS(app) 

# Estrutura para armazenar os dados mais recentes
latest_data = {
    "temperature": 0.0,
    "bpm": 0,
    "avg_bpm": 0,
    "spo2": 0,
    "has_finger": False,
    "timestamp": 0
}
data_lock = Lock()  

@app.route('/api/data', methods=['POST'])
def receive_data():
    global latest_data
    
    try:
        data = request.get_json()
        
        with data_lock:
            latest_data = {
                "temperature": float(data.get('temperature', 0.0)),
                "bpm": int(data.get('bpm', 0)),
                "avg_bpm": int(data.get('avg_bpm', 0)),
                "spo2": int(data.get('spo2', 0)),
                "has_finger": bool(data.get('has_finger', False)),
                "timestamp": time.time()
            }
        
        print("Dados recebidos:", latest_data)
        return jsonify({"status": "success"}), 200
    
    except Exception as e:
        print("Erro ao processar dados:", str(e))
        return jsonify({"status": "error", "message": str(e)}), 400

@app.route('/api/latest', methods=['GET'])
def get_latest_data():
    with data_lock:
        return jsonify(latest_data)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)