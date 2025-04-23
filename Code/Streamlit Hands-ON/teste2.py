import streamlit as st
import pandas as pd
import matplotlib.pyplot as plt
from firebase_admin import credentials, db, initialize_app
import firebase_admin
from firebase_admin import credentials, db

if not firebase_admin._apps:
    cred = credentials.Certificate('D:\Faculdade\Maloca das iCoisas\Hands-ON Advanced\serviceAccountKey.json')  # seu arquivo Firebase
    firebase_admin.initialize_app(cred, {
        'databaseURL': 'https://monitoramento-bpm-e-temp-default-rtdb.firebaseio.com'  # atualize com sua URL
    })

ref = db.reference('/')

# Função de filtro de média móvel
def media_movel(series, janela=5):
    return series.rolling(window=janela, min_periods=1).mean()

# Layout do Dashboard
st.title("Dashboard de Monitoramento de Pacientes")
st.sidebar.title("Configurações")
paciente_id = st.sidebar.selectbox("Escolha o Paciente:", ['Paciente1', 'Paciente2', 'Paciente3'])
janela_filtro = st.sidebar.slider("Tamanho da Janela do Filtro de Média Móvel", 1, 20, 5)

# Acessar dados do paciente
paciente_ref = ref.child('pacientes').child(paciente_id)
dados = paciente_ref.get()

if dados:
    df = pd.DataFrame(dados).T  # transpor para facilitar
    df['frequencia_cardiaca'] = pd.to_numeric(df['frequencia_cardiaca'], errors='coerce')
    df['oxigenacao'] = pd.to_numeric(df['oxigenacao'], errors='coerce')
    df['temperatura'] = pd.to_numeric(df['temperatura'], errors='coerce')
    df.index = pd.to_datetime(df.index)

    # Aplicar o filtro de média móvel na temperatura
    df['temperatura_filtrada'] = media_movel(df['temperatura'], janela=janela_filtro)

    st.subheader(f"Sinais Vitais - {paciente_id}")

    # Gráfico comparativo: Temperatura original x Filtrada
    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(df.index, df['temperatura'], label='Temperatura Original', color='red', alpha=0.6)
    ax.plot(df.index, df['temperatura_filtrada'], label='Temperatura Filtrada', color='green', linewidth=2)
    ax.set_title("Comparação: Temperatura Original vs Filtrada")
    ax.set_ylabel("Temperatura (°C)")
    ax.legend()
    st.pyplot(fig)

    # Outras métricas visuais
    st.metric("Última Temperatura", f"{df['temperatura'].iloc[-1]:.1f} °C")
    st.metric("Última Frequência Cardíaca", f"{df['frequencia_cardiaca'].iloc[-1]:.0f} BPM")
    st.metric("Último Nível de Oxigenação", f"{df['oxigenacao'].iloc[-1]:.0f} %")
else:
    st.warning("Nenhum dado encontrado para este paciente.")
