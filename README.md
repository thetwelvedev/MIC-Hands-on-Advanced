# <img src="/Pictures/logo-arkham.png" alt="Logo do Maloca das iCoisas" width="100"> Monitoramento de Pacientes em Situação de Risco <img src="/Pictures/logo-maloca.png" alt="Logo do Parceiro" width="100"></div>


**Componentes**: [Leonardo Castro](https://github.com/thetwelvedev), [Arthur Ramos](https://github.com/ArthurRamos26) e [Lucas Gabriel](https://github.com/lucasrocha777)

## Índice
- [Índice](#índice)
- [Etapas do Hands-on Avançado](#etapas-do-hands-on-avançado)
- [Organograma](#organograma)
- [Projeto Montado](#projeto-montado)
- [Documentos](#documentos)
  - [Big Picture](#big-picture)
  - [Documento de Requisitos Funcionais](#documento-de-requisitos-funcionais)
  - [Documento de Progresso](#documento-de-progresso)
  - [Slide do Pitch](#slide-do-pitch)
  - [Esquema de Conexões](#esquema-de-conexões)
    - [Esquema de Ligação](#esquema-de-ligação)
    - [Pinagem](#pinagem)

## Etapas do Hands-on Avançado

- [x] Sprint 0
- [x] Sprint 1
- [x] Sprint 2
- [x] Sprint 3
- [x] Apresentação - dia 25/04

## Organograma
![Organograma](/Pictures/organograma-arkham.png)

## Projeto Montado
![Prototipo](/Pictures/Proto-v3.jpg)

## Documentos

### Big Picture
![Big Picture](/Pictures/big-picture-arkham.png)

### Documento de Requisitos Funcionais
[Acesse aqui](/Docs/Requisitos%20Funcionais_Maloca.Equipe_Arkhamdocx.pdf)

### Documento de Progresso
[Acesse aqui](/Docs/Documento%20de%20progresso%20Equipe%20Arkham.docx.pdf)

### Slide do Pitch

[Acesse aqui](/Slide/Slide%20-%20Monitoramento%20de%20Pacientes%20de%20Risco.pdf)

[Veja completo](https://www.canva.com/design/DAGlCGJqn5o/cYMYZnWsSQK9ZPAXMBt4eg/edit?utm_content=DAGlCGJqn5o&utm_campaign=designshare&utm_medium=link2&utm_source=sharebutton)

### Esquema de Conexões

#### Esquema de Ligação

![Esquema de conexão](/Pictures/Esquema%20de%20Conexão.png)

#### Pinagem

**Conexões do MAX30102**  

| **Componente**       | **Pino ESP32** | **Descrição**                          |
|----------------------|---------------|----------------------------------------|
| **VIN (VCC)**        | 3.3V          | Alimentação (3.3V do ESP32)            |
| **GND**              | GND           | Terra (compartilhado com o ESP32)      |
| **SDA**              | GPIO21        | Dados I²C (barramento compartilhado)   |
| **SCL**              | GPIO22        | Clock I²C (barramento compartilhado)   |


**Conexões do CJMU-30205**  

| **Componente**       | **Pino ESP32** | **Descrição**                          |
|----------------------|---------------|----------------------------------------|
| **VCC**              | 3.3V          | Alimentação (3.3V do ESP32)            |
| **GND**              | GND           | Terra (compartilhado com o ESP32)      |
| **SDA**              | GPIO21        | Dados I²C (barramento compartilhado)   |
| **SCL**              | GPIO22        | Clock I²C (barramento compartilhado)   |
| **A0**               | GND           | Configura endereço I²C (padrão)        |
| **A1**               | GND           | Configura endereço I²C (padrão)        |
| **A2**               | GND           | Configura endereço I²C (padrão)        |


**Conexões do Display OLED (I²C)**  

| **Componente**       | **Pino ESP32** | **Descrição**                          |
|----------------------|---------------|----------------------------------------|
| **VCC**              | 3.3V          | Alimentação (3.3V do ESP32)            |
| **GND**              | GND           | Terra (compartilhado com o ESP32)      |
| **SDA**              | GPIO21        | Dados I²C (barramento compartilhado)   |
| **SCL**              | GPIO22        | Clock I²C (barramento compartilhado)   |

## Códigos

### Código do Circuito
```arduino
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "heartRate.h"

// Configurações de WiFi
const char* ssid = "Nome da rede Wifi";
const char* password = "Senha";
const char* serverUrl = "http://SEU_IP_LOCAL:5000/api/data";

// Endereço I2C do MAX30205
const uint8_t MAX30205_ADDRESS = 0x48;

// Definições do display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Objeto do sensor de batimento
MAX30105 particleSensor;

// Buffers para leitura dos dados
#define MAX_BRIGHTNESS 255
#define BUFFER_SIZE 100
uint32_t irBuffer[BUFFER_SIZE];  
uint32_t redBuffer[BUFFER_SIZE]; 
int32_t spo2;                    
int8_t validSPO2;                
int32_t heartRate;               
int8_t validHeartRate;           

// Variáveis para cálculo de BPM
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0; 
float beatsPerMinute;
int beatAvg;

// Variáveis para temporização
unsigned long lastDataSend = 0;
const unsigned long SEND_INTERVAL = 2000; 

// Variável de calibração da temperatura
float calibrationOffset = 0.0;

// Protótipos de função
float readTemperature();
bool checkTempSensor();
void calibrateSensor();
void updateDisplay(float temperature, bool alert, int bpm, int avgBpm, int spo2, bool hasFinger);
void setupHeartSensor();
void readHeartRateAndSpO2();
void scanI2C();
void sendDataToServer(float temp, int bpm, int avgBpm, int spo2, bool hasFinger);
void connectToWiFi();
void reconnectWiFiIfNeeded();

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); 

  // Inicializa o display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha na inicialização do OLED"));
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Inicializando...");
  display.display();

  // Conecta ao WiFi
  connectToWiFi();

  // Verifica dispositivos I2C
  scanI2C();

  // Verifica sensor de temperatura
  if (!checkTempSensor()) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("ERRO: Sensor temp");
    display.display();
    while (true);
  }

  calibrateSensor();     
  setupHeartSensor();   

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Sensores prontos!");
  display.display();
  delay(1000);
}

void loop() {
  // Verifica e reconecta WiFi se necessário
  reconnectWiFiIfNeeded();

  float rawTemp = readTemperature();
  float calibratedTemp = rawTemp + calibrationOffset;
  bool overTemp = calibratedTemp >= 38.0; // Febre se >= 38 °C

  readHeartRateAndSpO2(); 

  bool hasFinger = particleSensor.getIR() > 50000;

  // Envia dados periodicamente
  if (millis() - lastDataSend > SEND_INTERVAL) {
    sendDataToServer(calibratedTemp, heartRate, beatAvg, spo2, hasFinger);
    lastDataSend = millis();
  }

  // Exibe informações no serial
  Serial.print("Temp (raw): ");
  Serial.print(rawTemp, 2);
  Serial.print(" C | Calibrada: ");
  Serial.print(calibratedTemp, 2);
  Serial.print(" C | ");
  Serial.println(overTemp ? "ALERTA: FEBRE!" : "Normal");

  Serial.print("BPM: ");
  Serial.print(heartRate);
  Serial.print(" | Média: ");
  Serial.print(beatAvg);
  Serial.print(" | SpO2: ");
  Serial.print(spo2);
  Serial.print("% | Pulso: ");
  Serial.println(hasFinger ? "Sim" : "Não");
  Serial.println("------------------------");

  updateDisplay(calibratedTemp, overTemp, heartRate, beatAvg, spo2, hasFinger);

  delay(10); 
}

// Função para ler temperatura do MAX30205
float readTemperature() {
  Wire.beginTransmission(MAX30205_ADDRESS);
  Wire.write(0x00);
  if (Wire.endTransmission(false) != 0) {
    Serial.println("Erro na comunicação com MAX30205");
    return -100;
  }

  if (Wire.requestFrom(MAX30205_ADDRESS, 2) != 2) {
    Serial.println("Dados insuficientes");
    return -100;
  }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  
  // Verifica se os bytes são plausíveis
  if (msb == 0xFF && lsb == 0xFF) {
    Serial.println("Leitura inválida (0xFFFF)");
    return -100;
  }

  int16_t raw = (msb << 8) | lsb;
  return raw * 0.00390625; // Conversão para °C
}

// Verifica se o sensor de temperatura está presente
bool checkTempSensor() {
  Wire.beginTransmission(MAX30205_ADDRESS);
  byte error = Wire.endTransmission();
  if (error == 0) {
    Serial.println("MAX30205 detectado!");
    return true;
  } else {
    Serial.print("MAX30205 não encontrado! Erro: ");
    Serial.println(error);
    return false;
  }
}

// Calibra o sensor de temperatura
void calibrateSensor() {
  Serial.println("Calibrando temperatura... Aguarde 10s");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Calibrando sensor");
  display.println("Aguarde 10s...");
  display.display();
  delay(10000);

  float sum = 0;
  int validReadings = 0;
  int readings = 20;
  
  for (int i = 0; i < readings; i++) {
    float t = readTemperature();
    if (t != -100 && t > -50 && t < 100) { // Filtra valores absurdos
      sum += t;
      validReadings++;
      Serial.print("Leitura ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(t, 2);
    }
    delay(200);
  }

  if (validReadings == 0) {
    Serial.println("ERRO: Nenhuma leitura válida do sensor!");
    calibrationOffset = 0;
    return;
  }

  float avg = sum / validReadings;
  
  // Se a média estiver muito baixa (como -31.5), ajustamos para ~36°C
  if (avg < -20) {
    calibrationOffset = 36.0 - avg;
    Serial.println("Ajuste para leituras muito baixas");
  } else {
    // Caso contrário, calibramos para temperatura ambiente (~25°C)
    calibrationOffset = 25.0 - avg;
  }

  Serial.print("Média das leituras: ");
  Serial.print(avg, 2);
  Serial.print(" C | Offset calculado: ");
  Serial.print(calibrationOffset, 2);
  Serial.println(" C");
}

// Configura o sensor de batimentos cardíacos
void setupHeartSensor() {
  Serial.println("Iniciando sensor MAX30102...");

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 não detectado. Verifique a conexão.");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("ERRO: Sensor cardiaco");
    display.display();
    while (1);
  }

  // Configura o sensor com os melhores parâmetros
  byte ledBrightness = 60; 
  byte sampleAverage = 4;  
  byte ledMode = 2;        
  int sampleRate = 100;    
  int pulseWidth = 411;    
  int adcRange = 4096;     

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  particleSensor.enableDIETEMPRDY(); // Habilita o sensor de temperatura interna

  Serial.println("MAX30102 configurado!");
}

// Lê a frequência cardíaca e SpO2 do sensor
void readHeartRateAndSpO2() {
  long irValue = particleSensor.getIR();
  bool hasFinger = irValue > 50000;

  // Detecta batimentos cardíacos
  if (checkForBeat(irValue) {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      // Calcula a média dos batimentos
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  // Se não há dedo, zera os valores
  if (!hasFinger) {
    heartRate = 0;
    beatAvg = 0;
    spo2 = 0;
    validHeartRate = 0;
    validSPO2 = 0;
    return;
  }

  // Atualiza os buffers para cálculo de SpO2
  static uint32_t lastUpdate = 0;
  static int bufferIndex = 0;

  if (millis() - lastUpdate > 15) { // Coleta dados a cada 15ms
    lastUpdate = millis();
    
    redBuffer[bufferIndex] = particleSensor.getRed();
    irBuffer[bufferIndex] = irValue;
    
    bufferIndex++;
    
    if (bufferIndex == BUFFER_SIZE) {
      bufferIndex = 0;
      
      // Calcula SpO2 e BPM
      maxim_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_SIZE, redBuffer, 
                                           &spo2, &validSPO2, &heartRate, &validHeartRate);
      
      // Se o cálculo de BPM falhar, usa o valor do algoritmo de batimento
      if (validHeartRate == 0 && beatAvg > 0) {
        heartRate = beatAvg;
        validHeartRate = 1;
      }
    }
  }
}

// Atualiza o display OLED
void updateDisplay(float temperature, bool alert, int bpm, int avgBpm, int spo2, bool hasFinger) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Monitor de Saude");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Temperatura
  display.setCursor(0, 15);
  display.print("Temp:");
  display.print(temperature, 1);
  display.print((char)247); // ° símbolo
  display.println("C");

  // BPM
  display.setCursor(0, 25);
  display.print("BPM:");
  if (hasFinger) {
    display.print(bpm);
  } else {
    display.print("--");
  }

  // SpO2
  display.setCursor(72, 15);
  display.print("SpO2:");
  if (hasFinger) {
    display.print(spo2);
  } else {
    display.print("--");
  }
  display.print("%");

  // Média BPM
  display.setCursor(72, 25);
  display.print("Avg:");
  if (hasFinger) {
    display.print(avgBpm);
  } else {
    display.print("--");
  }

  // Status do dedo
  display.setCursor(0, 35);
  display.print("Pulso: ");
  display.print(hasFinger ? "Detectado" : "Ausente");

  // WiFi status
  display.setCursor(0, 45);
  display.print("WiFi: ");
  display.print(WiFi.status() == WL_CONNECTED ? "Conectado" : "Desconectado");

  // Alerta de febre
  if (alert) {
    display.setCursor(0, 55);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.print(" ALERTA: FEBRE! ");
    display.setTextColor(SSD1306_WHITE);
  }

  display.display();
}

// Envia dados para o servidor Flask
void sendDataToServer(float temp, int bpm, int avgBpm, int spo2, bool hasFinger) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Cria o objeto JSON para enviar
    String httpRequestData = "{\"temperature\":" + String(temp, 2) + 
                            ",\"bpm\":" + String(bpm) + 
                            ",\"avg_bpm\":" + String(avgBpm) + 
                            ",\"spo2\":" + String(spo2) + 
                            ",\"has_finger\":" + String(hasFinger ? "true" : "false") + 
                            "}";
    
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    
    int httpResponseCode = http.POST(httpRequestData);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Dados enviados com sucesso!");
      Serial.println("Resposta do servidor: " + response);
    } else {
      Serial.print("Erro no envio: ");
      Serial.println(httpResponseCode);
    }
    
    http.end();
  } else {
    Serial.println("WiFi desconectado - dados não enviados");
  }
}

// Conecta ao WiFi
void connectToWiFi() {
  Serial.println("Conectando ao WiFi...");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Conectando WiFi...");
  display.display();
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.display();
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi conectado");
    Serial.println("Endereço IP: ");
    Serial.println(WiFi.localIP());
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi conectado!");
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.display();
    delay(1000);
  } else {
    Serial.println("Falha na conexão WiFi");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Falha no WiFi!");
    display.display();
  }
}

// Verifica e reconecta WiFi se necessário
void reconnectWiFiIfNeeded() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado - tentando reconectar...");
    WiFi.disconnect();
    delay(100);
    connectToWiFi();
  }
}

// Scaneia dispositivos I2C
void scanI2C() {
  Serial.println("Scanning I2C...");
  byte error, address;
  int nDevices = 0;

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Dispositivo encontrado: 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
  }

  if (nDevices == 0) {
    Serial.println("Nenhum dispositivo I2C encontrado!");
  }
}  
```

### Código do servidor 
```python
from flask import Flask, request, jsonify
from flask_cors import CORS
import time
from threading import Lock

app = Flask(__name__)
CORS(app)

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
        return jsonify({"status": "success"}), 200
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 400

@app.route('/api/latest', methods=['GET'])
def get_latest_data():
    with data_lock:
        return jsonify(latest_data)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True) 
```
### Código do Dashboard
```python
import streamlit as st
import requests
import time
import plotly.graph_objects as go
from datetime import datetime
import numpy as np
from pykalman import KalmanFilter

# === CONFIGURAÇÕES ===
FLASK_SERVER = "http://IP_LOCAL:5000"
UPDATE_INTERVAL = 1
MAX_RETRIES = 3

# === INICIALIZAÇÃO DO HISTÓRICO ===
if 'data_history' not in st.session_state:
    st.session_state.data_history = {
        'time': [],
        'temperature': [],
        'bpm': [],
        'filtered_bpm': [],
        'avg_bpm': [],
        'spo2': [],
        'ecg': []
    }

# === FUNÇÕES AUXILIARES ===
def fetch_data():
    for _ in range(MAX_RETRIES):
        try:
            response = requests.get(f"{FLASK_SERVER}/api/latest", timeout=2)
            if response.status_code == 200:
                data = response.json()
                data.setdefault('temperature', 0.0)
                data.setdefault('bpm', 0)
                data.setdefault('avg_bpm', 0)
                data.setdefault('spo2', 0)
                data.setdefault('has_finger', False)
                data.setdefault('ecg', generate_ecg_signal(data['bpm'] if data['has_finger'] else 0))
                return data
        except Exception as e:
            st.warning(f"Erro ao buscar dados: {str(e)}")
            time.sleep(1)
    return None

def generate_ecg_signal(bpm):
    if bpm == 0:
        return 0
    heart_rate = bpm / 60.0
    t = np.linspace(0, 1.0/heart_rate, 500)
    p_wave = 0.25 * np.sin(2 * np.pi * 5 * t) * (t < 0.2/heart_rate)
    qrs = 1.5 * np.sin(2 * np.pi * 15 * t) * ((t >= 0.2/heart_rate) & (t < 0.25/heart_rate))
    t_wave = 0.3 * np.sin(2 * np.pi * 2 * t) * ((t >= 0.3/heart_rate) & (t < 0.45/heart_rate))
    ecg = p_wave + qrs + t_wave
    return ecg[-1] + 0.05 * np.random.randn()

def moving_average(data, window=5):
    if len(data) < window:
        return data
    return np.convolve(data, np.ones(window)/window, mode='valid')

def kalman_filter(signal):
    if len(signal) < 2:
        return signal
    kf = KalmanFilter(initial_state_mean=signal[0], 
                      initial_state_covariance=1,
                      transition_matrices=[1],
                      observation_matrices=[1],
                      observation_covariance=4,
                      transition_covariance=0.1)
    filtered_state_means, _ = kf.filter(signal)
    return filtered_state_means.flatten()

# === SIDEBAR ===
with st.sidebar:
    st.image("logo-maloca.png", use_container_width=True)
    st.image("logo-arkham.png", use_container_width=True)
    st.header("Opções de Visualização")
    show_temp = st.checkbox("Temperatura", True)
    show_bpm = st.checkbox("Frequência Cardíaca", True)
    show_spo2 = st.checkbox("Oxigenação Sanguínea", True)
    show_ecg = st.checkbox("ECG", True)
    st.markdown("---")
    st.header("Filtros de Sinal")
    apply_ma = st.checkbox("Média Móvel")
    apply_kalman = st.checkbox("Filtro de Kalman")

# === LAYOUT PRINCIPAL ===
st.title('Monitor de Saúde em Tempo Real')
col1, col2, col3, col4 = st.columns(4)
temp_ph = col1.empty()
bpm_ph = col2.empty()
avg_bpm_ph = col3.empty()
spo2_ph = col4.empty()

chart_temp = st.empty() if show_temp else None
chart_bpm = st.empty() if show_bpm else None
chart_spo2 = st.empty() if show_spo2 else None
chart_ecg = st.empty() if show_ecg else None

# === LOOP PRINCIPAL ===
while True:
    data = fetch_data()
    if data:
        current_time = datetime.now().strftime("%H:%M:%S")

        st.session_state.data_history['time'].append(current_time)
        st.session_state.data_history['temperature'].append(data['temperature'])
        st.session_state.data_history['bpm'].append(data['bpm'] if data['has_finger'] else None)
        st.session_state.data_history['avg_bpm'].append(data['avg_bpm'])
        st.session_state.data_history['spo2'].append(data['spo2'])
        st.session_state.data_history['ecg'].append(data['ecg'])

        for key in st.session_state.data_history:
            max_len = 100 if key == 'ecg' else 30
            st.session_state.data_history[key] = st.session_state.data_history[key][-max_len:]

        # Aplica filtros ao BPM
        bpm_data = [x for x in st.session_state.data_history['bpm'] if x is not None]
        filtered_bpm = bpm_data

        if apply_ma:
            filtered_bpm = moving_average(filtered_bpm, window=5)
        if apply_kalman:
            filtered_bpm = kalman_filter(filtered_bpm)

        st.session_state.data_history['filtered_bpm'] = list(filtered_bpm) + [None] * (len(st.session_state.data_history['bpm']) - len(filtered_bpm))

        # === ATUALIZA MÉTRICAS ===
        temp_ph.metric("Temperatura (°C)", f"{data['temperature']:.1f}")
        bpm_ph.metric("BPM", str(data['bpm']) if data['has_finger'] else "--")
        avg_bpm_ph.metric("Média BPM", str(data['avg_bpm']) if data['has_finger'] else "--")
        spo2_ph.metric("SpO2 (%)", str(data['spo2']) if data['has_finger'] else "--")

        # === GRÁFICOS ===
        if show_temp:
            fig_temp = go.Figure()
            fig_temp.add_trace(go.Scatter(x=st.session_state.data_history['time'], y=st.session_state.data_history['temperature'], name="Temperatura", line=dict(color='orange')))
            fig_temp.update_layout(title="Temperatura Corporal", xaxis_title="Tempo", yaxis_title="°C")
            chart_temp.plotly_chart(fig_temp, use_container_width=True)

        if show_bpm:
            fig_bpm = go.Figure()
            fig_bpm.add_trace(go.Scatter(x=st.session_state.data_history['time'], y=st.session_state.data_history['bpm'], name="BPM Original", line=dict(color='blue')))
            fig_bpm.add_trace(go.Scatter(x=st.session_state.data_history['time'], y=st.session_state.data_history['filtered_bpm'], name="BPM Filtrado", line=dict(color='green', dash='dot')))
            fig_bpm.update_layout(title="Frequência Cardíaca", xaxis_title="Tempo", yaxis_title="BPM")
            chart_bpm.plotly_chart(fig_bpm, use_container_width=True)

        if show_spo2:
            fig_spo2 = go.Figure()
            fig_spo2.add_trace(go.Scatter(x=st.session_state.data_history['time'], y=st.session_state.data_history['spo2'], name="SpO2", line=dict(color='purple')))
            fig_spo2.update_layout(title="Oxigenação Sanguínea", xaxis_title="Tempo", yaxis_title="SpO2 (%)")
            chart_spo2.plotly_chart(fig_spo2, use_container_width=True)

        if show_ecg:
            fig_ecg = go.Figure()
            fig_ecg.add_trace(go.Scatter(y=st.session_state.data_history['ecg'], name="ECG", line=dict(color='red')))
            fig_ecg.update_layout(title="Eletrocardiograma (ECG)", xaxis_title="Amostras", yaxis_title="Voltagem")
            chart_ecg.plotly_chart(fig_ecg, use_container_width=True)
    else:
        st.error("Erro ao conectar com o servidor de dados.")

    time.sleep(UPDATE_INTERVAL)

```
## Configuração e Execução

### Pré-requisitos

#### Hardware:
- ESP32 com sensores conectados conforme esquema
- Computador com porta USB

#### Software:
- Python 3.8+
- Arduino IDE
- Bibliotecas Python:
```bash
pip install streamlit flask flask-cors requests plotly numpy pykalman
```

### Passo a passo

1. Configurar Ambiente:
```bash
python -m venv venv
# Linux/Mac:
source venv/bin/activate
# Windows:
venv\Scripts\activate
pip install -r requirements.txt
```

2. Executar Serviços:

- Em um terminal:
```bash
python server.py
```

- Em outro terminal:
```bash
streamlit run dashboard-tempo-real.py
```
3. Programar ESP32:
- Instale as bibliotecas necessárias na Arduino IDE:
  - Adafruit SSD1306
  - Adafruit GFX
  - MAX30105 by SparkFun
  - WiFi(No ESP32 vem por padrão)

- Carregue o código monitoramento-com-dashboard.ino

- Configure com suas credenciais WiFi
