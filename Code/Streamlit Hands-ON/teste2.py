import streamlit as st
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime, timedelta
import time

st.set_page_config(layout="wide",
                   initial_sidebar_state="expanded",
                   page_icon="D:\Faculdade\Maloca das iCoisas\Hands-ON Advanced\logo_Arkham.jpg",
                   )

# CSS personalizado
st.markdown("""
    <style>
    .stApp {
        background-color: #e8f5e9;
    }
    .header {
        background-color: white;
        padding: 1rem;
        border-radius: 30px;
        margin-bottom: 1rem;
    }
    .header-content {
        display: flex;
        justify-content: space-between;
        align-items: center;
    }
    .title-container {
        text-align: center;
        margin-top: 1rem;
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
    .footer {
        background-color: white;
        padding: 1rem;
        border-radius: 30px;
        margin-top: 2rem;
        text-align: center;
        font-size: 0.9rem;
        color: #666;
    }
    </style>
""", unsafe_allow_html=True)

# Função de média móvel
def media_movel(series, janela=5):
    return series.rolling(window=janela, min_periods=1).mean()

# Simular ECG (visual, não real)
def simular_ecg(tamanho=500):
    t = np.linspace(0, 3 * np.pi, tamanho)
    ecg = np.sin(1.5 * t) + 0.1 * np.random.randn(tamanho)
    ecg[::50] += 2  # "pico" tipo batimento
    return ecg

# Sidebar para páginas
st.sidebar.image("D:\Faculdade\Maloca das iCoisas\Hands-ON Advanced\logo_Arkham.jpg")
pagina = st.sidebar.radio("Navegar para:", ["Monitoramento em Tempo Real", "Médias dos Dados", "Sobre o Projeto"])
st.sidebar.image("D:\Faculdade\Maloca das iCoisas\Hands-ON Advanced\logo_maloca.jpg")


# DataFrame Global
if 'dados' not in st.session_state:
    st.session_state.dados = pd.DataFrame(columns=['tempo', 'frequencia_cardiaca', 'temperatura', 'oxigenacao'])
    st.session_state.tempo_atual = datetime.now()

# Página 1: Monitoramento em Tempo Real
if pagina == "Monitoramento em Tempo Real":
    st.title("Monitoramento em Tempo Real - Paciente")
    janela_filtro = st.sidebar.slider("Tamanho da Janela do Filtro de Média Móvel", 1, 20, 5)
    atualizar_cada = st.sidebar.slider("Atualizar a cada (segundos)", 1, 10, 2)

    grafico_container = st.empty()
    metricas_container = st.empty()
    ecg_container = st.empty()

    # Função para simular novo dado
    def novo_dado(prev):
        if prev is None:
            return {
                'frequencia_cardiaca': np.random.randint(75, 85),
                'temperatura': np.random.uniform(36.3, 36.7),
                'oxigenacao': np.random.uniform(97, 99)
            }
        return {
            'frequencia_cardiaca': np.clip(prev['frequencia_cardiaca'] + np.random.normal(0, 1), 60, 100),
            'temperatura': np.clip(prev['temperatura'] + np.random.normal(0, 0.05), 34, 37.5),
            'oxigenacao': np.clip(prev['oxigenacao'] + np.random.normal(0, 0.2), 94, 100)
        }

    # Simular novo ponto
    prev = st.session_state.dados.iloc[-1] if not st.session_state.dados.empty else None
    novo = novo_dado(prev)
    st.session_state.tempo_atual += timedelta(seconds=atualizar_cada)
    novo['tempo'] = st.session_state.tempo_atual
    st.session_state.dados = pd.concat([st.session_state.dados, pd.DataFrame([novo])], ignore_index=True)

    # Gráfico temperatura com média móvel
    dados = st.session_state.dados.copy()
    dados['temperatura_filtrada'] = media_movel(dados['temperatura'], janela=janela_filtro)

    with grafico_container.container():
        fig, ax = plt.subplots(figsize=(10, 4))
        ax.plot(dados['tempo'], dados['temperatura'], label='Temperatura Original', color='red', alpha=0.5)
        ax.plot(dados['tempo'], dados['temperatura_filtrada'], label='Temperatura Filtrada', color='green', linewidth=2)
        ax.set_ylabel("Temperatura (°C)")
        ax.set_title("Temperatura Corporal")
        ax.legend()
        st.pyplot(fig)

    # Métricas
    with metricas_container.container():
        st.metric("🌡Temperatura", f"{dados['temperatura'].iloc[-1]:.1f} °C")
        st.metric("📉Frequência Cardíaca", f"{dados['frequencia_cardiaca'].iloc[-1]:.0f} BPM")
        st.metric("🔴Oxigenação", f"{dados['oxigenacao'].iloc[-1]:.0f} %")

        st.subheader("Alertas")

        alertas = []
        ultima = dados.iloc[-1]

        if ultima['frequencia_cardiaca'] < 60 or ultima['frequencia_cardiaca'] > 100:
            alertas.append("⚠️ Frequência Cardíaca fora do normal!")

        if ultima['temperatura'] < 35 or ultima['temperatura'] > 37.5:
            alertas.append("🌡️ Temperatura fora do intervalo esperado!")

        if ultima['oxigenacao'] < 95:
            alertas.append("🫁 Nível de Oxigênio abaixo do ideal!")

        if alertas:
            for alerta in alertas:
                st.error(alerta)
        else:
            st.success("✅ Todos os sinais estão dentro dos parâmetros normais.")

    # ECG
    with ecg_container.container():
        ecg = simular_ecg(100)
        if 'ecg_buffer' not in st.session_state:
            st.session_state.ecg_buffer = ecg
        else:
            st.session_state.ecg_buffer = np.roll(st.session_state.ecg_buffer, -len(ecg))
            st.session_state.ecg_buffer[-len(ecg):] = ecg

        fig_ecg, ax_ecg = plt.subplots(figsize=(10, 2))
        ax_ecg.plot(st.session_state.ecg_buffer, color='blue')
        ax_ecg.set_title("Gráfico ECG")
        ax_ecg.axis("off")
        st.pyplot(fig_ecg)


# Página 2: Médias dos Dados
elif pagina == "Médias dos Dados":
    st.title("📊 Médias dos Sinais Vitais Registrados")

    if not st.session_state.dados.empty:
        df = st.session_state.dados
        st.write("### Médias")
        col1, col2, col3 = st.columns(3)
        col1.metric("Temperatura Média", f"{df['temperatura'].mean():.2f} °C")
        col2.metric("Frequência Média", f"{df['frequencia_cardiaca'].mean():.1f} BPM")
        col3.metric("Oxigenação Média", f"{df['oxigenacao'].mean():.1f} %")

        # Garantir os tipos corretos e remover linhas com valores inválidos
        df_filtrado = df[['tempo', 'temperatura', 'frequencia_cardiaca', 'oxigenacao']].copy()
        df_filtrado['tempo'] = pd.to_datetime(df_filtrado['tempo'], errors='coerce')
        df_filtrado['temperatura'] = pd.to_numeric(df_filtrado['temperatura'], errors='coerce')
        df_filtrado['frequencia_cardiaca'] = pd.to_numeric(df_filtrado['frequencia_cardiaca'], errors='coerce')
        df_filtrado['oxigenacao'] = pd.to_numeric(df_filtrado['oxigenacao'], errors='coerce')

        df_filtrado = df_filtrado.dropna()  # remove valores inválidos
        df_filtrado = df_filtrado.set_index('tempo')

    else:
        st.info("Nenhum dado ainda foi registrado.")
if pagina == "Sobre o Projeto":
    st.title("📚 Sobre o Projeto de monitoramento equipe Arkham")
