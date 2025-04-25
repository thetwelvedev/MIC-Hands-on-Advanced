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
