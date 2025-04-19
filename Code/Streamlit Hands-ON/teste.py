import streamlit as st
import matplotlib.pyplot as plt
import numpy as np
import time
from datetime import datetime
import firebase_admin
from firebase_admin import credentials, db
import os
from PIL import Image

# Configuração inicial do Firebase (substitua com suas credenciais)
if not firebase_admin._apps:
    cred = credentials.Certificate("C:\\Users\\ALUNOS MALOCA\\Documents\\Dashboad_Arkham\\serviceAccountKey.json")  # Arquivo de credenciais do Firebase
    firebase_admin.initialize_app(cred, {
        'databaseURL': 'https://monitoramento-bpm-e-temp-default-rtdb.firebaseio.com'  # URL do seu banco de dados Firebase
    })

# Configuração da página
st.set_page_config(page_title="Monitoramento de Paciente", page_icon="🩺", layout="wide")

# Função para carregar imagens (substitua com seus próprios caminhos de imagem)
def load_image(image_path):
    try:
        return Image.open(image_path)
    except:
        return None

# Carregar logos (substitua com seus próprios arquivos)
logo_projeto = load_image("logo_projeto.png")
logo_faculdade = load_image("logo_faculdade.png")
logo_curso = load_image("logo_curso.png")

# CSS personalizado para o fundo verde e estilo
st.markdown("""
    <style>
    .stApp {
        background-color: #e8f5e9;
    }
    .header {
        background-color: white;
        padding: 1rem;
        border-radius: 10px;
        margin-bottom: 2rem;
        display: flex;
        justify-content: space-between;
        align-items: center;
    }
    .logo-container {
        display: flex;
        align-items: center;
        gap: 20px;
    }
    .metric-card {
        background-color: white;
        border-radius: 10px;
        padding: 1.5rem;
        box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
        text-align: center;
    }
    .metric-title {
        font-size: 1rem;
        color: #666;
        margin-bottom: 0.5rem;
    }
    .metric-value {
        font-size: 2rem;
        font-weight: bold;
        color: #2e7d32;
    }
    .metric-unit {
        font-size: 1rem;
        color: #666;
    }
    </style>
    """, unsafe_allow_html=True)

# Cabeçalho com logos
col1, col2, col3 = st.columns([1, 2, 1])
with col1:
    if logo_projeto:
        st.image(logo_projeto, width=100)
with col2:
    st.title("Monitoramento de Paciente")
with col3:
    if logo_faculdade and logo_curso:
        st.image([logo_faculdade, logo_curso], width=100)

# Divisor
st.markdown("---")

# Função para simular dados de ECG
def generate_ecg_data(heart_rate=72, duration=5, fs=250):
    t = np.linspace(0, duration, int(duration * fs))
    
    # Parâmetros do ECG
    hr = heart_rate
    rr = 60 / hr  # Intervalo RR em segundos
    
    # Componentes do ECG
    p_wave = 0.25 * np.sin(2 * np.pi * 0.7 * t)
    qrs_complex = 1.5 * np.sin(2 * np.pi * 15 * t) * np.exp(-20 * (t % rr))
    t_wave = 0.3 * np.sin(2 * np.pi * 0.3 * t)
    
    # Combina os componentes
    ecg = p_wave + qrs_complex + t_wave
    
    # Adiciona ruído
    noise = 0.05 * np.random.normal(size=len(t))
    ecg += noise
    
    return t, ecg

# Função para obter dados do Firebase
def get_firebase_data():
    try:
        ref = db.reference('/paciente')
        data = ref.get()
        return data or {
            'heart_rate': 72,
            'spo2': 98,
            'temperature': 36.5,
            'timestamp': datetime.now().isoformat()
        }
    except:
        # Retorna dados simulados se houver erro na conexão
        return {
            'heart_rate': np.random.randint(60, 100),
            'spo2': np.random.randint(95, 100),
            'temperature': round(np.random.normal(36.5, 0.5), 1),
            'timestamp': datetime.now().isoformat()
        }

# Layout principal
col1, col2, col3 = st.columns(3)

# Atualização em tempo real
placeholder = st.empty()

while True:
    # Obter dados do Firebase
    data = get_firebase_data()
    
    with placeholder.container():
        # Métricas
        col1, col2, col3 = st.columns(3)
        
        with col1:
            st.markdown("""
            <div class="metric-card">
                <div class="metric-title">Frequência Cardíaca</div>
                <div class="metric-value">{}</div>
                <div class="metric-unit">bpm</div>
            </div>
            """.format(data['heart_rate']), unsafe_allow_html=True)
        
        with col2:
            st.markdown("""
            <div class="metric-card">
                <div class="metric-title">Oxigênio no Sangue</div>
                <div class="metric-value">{}</div>
                <div class="metric-unit">% SpO2</div>
            </div>
            """.format(data['spo2']), unsafe_allow_html=True)
        
        with col3:
            st.markdown("""
            <div class="metric-card">
                <div class="metric-title">Temperatura</div>
                <div class="metric-value">{}</div>
                <div class="metric-unit">°C</div>
            </div>
            """.format(data['temperature']), unsafe_allow_html=True)
        
        # Gráfico de ECG
        st.markdown("### Eletrocardiograma (ECG)")
        t, ecg = generate_ecg_data(data['heart_rate'])
        fig, ax = plt.subplots(figsize=(10, 4))
        ax.plot(t, ecg, color='green')
        ax.set_xlabel('Tempo (s)')
        ax.set_ylabel('Amplitude')
        ax.grid(True)
        st.pyplot(fig)
        
        # Data e hora da última atualização
        st.caption(f"Última atualização: {datetime.fromisoformat(data['timestamp']).strftime('%d/%m/%Y %H:%M:%S')}")
    
    # Atualizar a cada 2 segundos
    time.sleep(2)