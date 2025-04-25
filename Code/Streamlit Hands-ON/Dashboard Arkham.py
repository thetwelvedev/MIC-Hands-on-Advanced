import streamlit as st
import numpy as np
import plotly.graph_objects as go
from datetime import datetime, timedelta
import time

# Configuração da página
st.set_page_config(layout="wide", page_title="Monitor ECG")

# CSS personalizado para melhorar a aparência
st.markdown("""
    <style>
    .stApp {
        background-color: #f0f2f6;
    }
    .ecg-container {
        background-color: white;
        border-radius: 10px;
        padding: 20px;
        box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
        margin-bottom: 20px;
    }
    </style>
""", unsafe_allow_html=True)

# Título da aplicação
st.title("📉 Monitoramento ECG em Tempo Real")

# Parâmetros controláveis pela sidebar
with st.sidebar:
    st.header("Configurações do ECG")
    update_interval = st.slider("Intervalo de atualização (ms)", 50, 1000, 100)
    heart_rate = st.slider("Frequência cardíaca (BPM)", 40, 120, 72)
    amplitude = st.slider("Amplitude do sinal", 0.5, 3.0, 1.5)
    noise_level = st.slider("Nível de ruído", 0.0, 0.5, 0.1)

# Função para simular um sinal ECG mais realista
def generate_ecg_signal(length=500, heart_rate=72, amplitude=1.5, noise=0.1):
    # Gerar um sinal ECG básico com complexos QRS
    t = np.linspace(0, 2 * np.pi * (length / 100), length)
    
    # Componentes do ECG
    p_wave = 0.1 * np.sin(1.5 * t) * (np.sin(0.05 * t) ** 2)
    qrs_complex = amplitude * np.sin(30 * t) * np.exp(-5 * (t % (2 * np.pi / (heart_rate / 60))))
    t_wave = 0.2 * np.sin(0.8 * t) * (np.sin(0.05 * t) ** 10)
    
    # Combinar componentes e adicionar ruído
    ecg = p_wave + qrs_complex + t_wave + noise * np.random.randn(length)
    
    return ecg

# Inicializar buffer de ECG
if 'ecg_buffer' not in st.session_state:
    st.session_state.ecg_buffer = generate_ecg_signal(500, heart_rate, amplitude, noise_level)

# Container principal para o ECG
ecg_placeholder = st.empty()

# Atualização contínua do ECG
while True:
    # Gerar novo segmento de ECG
    new_segment = generate_ecg_signal(50, heart_rate, amplitude, noise_level)
    
    # Atualizar buffer (deslocamento e adição de novos dados)
    st.session_state.ecg_buffer = np.roll(st.session_state.ecg_buffer, -len(new_segment))
    st.session_state.ecg_buffer[-len(new_segment):] = new_segment
    
    # Criar figura do Plotly
    fig = go.Figure()
    
    # Adicionar trace do ECG
    fig.add_trace(go.Scatter(
        y=st.session_state.ecg_buffer,
        line=dict(color='#2e7d32', width=2),
        name='Sinal ECG',
        hoverinfo='none'
    ))
    
    # Configurações do layout
    fig.update_layout(
        title="Eletrocardiograma (ECG) em Tempo Real",
        xaxis=dict(
            title="Tempo (ms)",
            showgrid=True,
            gridcolor='lightgray',
            zeroline=False
        ),
        yaxis=dict(
            title="Amplitude (mV)",
            showgrid=True,
            gridcolor='lightgray',
            zeroline=True,
            zerolinecolor='black'
        ),
        plot_bgcolor='white',
        paper_bgcolor='white',
        height=400,
        margin=dict(l=40, r=40, t=60, b=40),
        showlegend=False
    )
    
    # Adicionar linha de zero
    fig.add_hline(y=0, line_dash="dot", line_color="gray")
    
    # Mostrar o gráfico no placeholder
    with ecg_placeholder.container():
        st.plotly_chart(fig, use_container_width=True)
    
    # Intervalo de atualização
    time.sleep(update_interval / 1000)