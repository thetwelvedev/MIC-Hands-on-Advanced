import streamlit as st
import pandas as pd
import numpy as np
from matplotlib.colors import LinearSegmentedColormap
import plotly.express as px
import pickle
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.ensemble import RandomForestClassifier

# ----------------------------------------------------------
# Configuração da Página e CSS Customizado
# ----------------------------------------------------------
st.set_page_config(
    page_title="Dashboard de monitoramento Equipe Arkham",
    page_icon="😷",
    layout="wide",
    initial_sidebar_state="expanded"
)

st.markdown("""
<style>
    /* Variáveis de cores - Paleta suave e profissional */
    :root {
        --primary: #3a86ff;
        --primary-light: #e1ebff;
        --primary-dark: #0043a9;
        --secondary: #38b000;
        --secondary-light: #e3f5e1;
        --accent: #ff9e00;
        --accent-light: #fff4e1;
        --danger: #ef476f;
        --danger-light: #fde1e7;
        --text-dark: #2b2d42;
        --text-medium: #555b6e;
        --text-light: #8d99ae;
        --background: #f8f9fa;
        --card: #ffffff;
        --border: #e9ecef;
        --shadow: rgba(0, 0, 0, 0.05);
    }
    /* Reset e estilos base */
    html, body, [class*="css"] {
        font-family: 'Inter', -apple-system, BlinkMacSystemFont, sans-serif;
        color: var(--text-dark);
    }
    /* Estilo geral da página */
    .main {
        background-color: var(--background);
        background-image: linear-gradient(to bottom, #f8f9fa, #ffffff);
    }
    /* Cabeçalho principal */
    .main-header {
        font-size: 2.5rem;
        color: var(--primary-dark);
        text-align: center;
        margin-bottom: 1.5rem;
        font-weight: 800;
        letter-spacing: -0.025em;
        padding-bottom: 1rem;
        border-bottom: 3px solid var(--primary);
        background: linear-gradient(90deg, var(--primary-dark) 0%, var(--primary) 100%);
        -webkit-background-clip: text;
        -webkit-text-fill-color: transparent;
        background-clip: text;
    }
    /* Subtítulos */
    .sub-header {
        font-size: 1.8rem;
        color: var(--primary-dark);
        margin-top: 2rem;
        margin-bottom: 1.5rem;
        font-weight: 700;
        letter-spacing: -0.01em;
        border-bottom: 2px solid var(--primary-light);
        padding-bottom: 0.75rem;
    }
    /* Seções */
    .section {
        background-color: var(--card);
        padding: 1.8rem;
        border-radius: 16px;
        margin-bottom: 1.8rem;
        border: 1px solid var(--border);
        box-shadow: 0 4px 20px var(--shadow);
        transition: transform 0.2s ease, box-shadow 0.2s ease;
    }
    .section:hover {
        box-shadow: 0 8px 30px rgba(0, 0, 0, 0.08);
    }
    /* Cards de métricas */
    .metric-card {
        background-color: var(--card);
        padding: 1.8rem;
        border-radius: 16px;
        box-shadow: 0 4px 20px var(--shadow);
        text-align: center;
        transition: all 0.3s ease;
        border: 1px solid var(--border);
        height: 100%;
        display: flex;
        flex-direction: column;
        justify-content: center;
    }
    .metric-card:hover {
        transform: translateY(-5px);
        box-shadow: 0 12px 30px rgba(0, 0, 0, 0.1);
    }
    .metric-value {
        font-size: 2.5rem;
        font-weight: 800;
        color: var(--primary);
        margin: 0.5rem 0;
        letter-spacing: -0.03em;
    }
    .metric-label {
        font-size: 1.1rem;
        color: var(--text-medium);
        margin-top: 0.5rem;
        font-weight: 500;
    }
    /* Gráficos */
    .stPlotlyChart {
        background-color: var(--card);
        border-radius: 16px;
        padding: 1.2rem;
        box-shadow: 0 4px 20px var(--shadow);
        border: 1px solid var(--border);
        transition: transform 0.2s ease, box-shadow 0.2s ease;
    }
    .stPlotlyChart:hover {
        box-shadow: 0 8px 30px rgba(0, 0, 0, 0.08);
    }
    /* Sidebar */
    .sidebar .sidebar-content {
        background-color: var(--primary-light);
        background-image: linear-gradient(180deg, var(--primary-light) 0%, rgba(255, 255, 255, 0.95) 100%);
    }
    /* Botões */
    .stButton>button {
        background-color: var(--primary);
        color: white;
        border-radius: 8px;
        border: none;
        padding: 0.6rem 1.2rem;
        font-weight: 600;
        transition: all 0.2s;
        box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
    }
    .stButton>button:hover {
        background-color: var(--primary-dark);
        box-shadow: 0 4px 10px rgba(0, 0, 0, 0.15);
        transform: translateY(-2px);
    }
    /* Seletores */
    .stSelectbox>div>div {
        background-color: white;
        border-radius: 8px;
        border: 1px solid var(--border);
    }
    /* Tabs */
    .stTabs [data-baseweb="tab-list"] {
        gap: 2px;
        background-color: var(--primary-light);
        border-radius: 12px;
        padding: 5px;
    }
    .stTabs [data-baseweb="tab"] {
        border-radius: 8px;
        padding: 10px 16px;
        background-color: transparent;
    }
    .stTabs [aria-selected="true"] {
        background-color: white;
        color: var(--primary-dark);
        font-weight: 600;
        box-shadow: 0 2px 5px rgba(0, 0, 0, 0.05);
    }
    /* Tabelas */
    .dataframe {
        border-collapse: collapse;
        width: 100%;
        border-radius: 12px;
        overflow: hidden;
        box-shadow: 0 4px 6px rgba(0, 0, 0, 0.05);
    }
    .dataframe th {
        background-color: var(--primary-light);
        color: var(--primary-dark);
        padding: 12px;
        text-align: left;
        font-weight: 600;
    }
    .dataframe td {
        padding: 10px;
        border-top: 1px solid var(--border);
    }
    .dataframe tr:nth-child(even) {
        background-color: #f8fafc;
    }
    /* Ícones e indicadores */
    .positive-indicator {
        color: var(--secondary);
        font-weight: 600;
    }
    .negative-indicator {
        color: var(--danger);
        font-weight: 600;
    }
    /* Cartão informativo */
    .info-card {
        background-color: var(--primary-light);
        border-left: 4px solid var(--primary);
        padding: 1rem;
        border-radius: 8px;
        margin-bottom: 1rem;
    }
    .warning-card {
        background-color: var(--accent-light);
        border-left: 4px solid var(--accent);
        padding: 1rem;
        border-radius: 8px;
        margin-bottom: 1rem;
    }
    .danger-card {
        background-color: var(--danger-light);
        border-left: 4px solid var(--danger);
        padding: 1rem;
        border-radius: 8px;
        margin-bottom: 1rem;
    }
    /* Animações e transições */
    .animate-fade-in {
        animation: fadeIn 0.5s ease-in;
    }
    @keyframes fadeIn {
        from { opacity: 0; }
        to { opacity: 1; }
    }
    /* Footer */
    .footer {
        text-align: center;
        margin-top: 3rem;
        padding: 2rem;
        background: linear-gradient(135deg, var(--primary-dark) 0%, var(--primary) 100%);
        color: white;
        border-radius: 16px;
    }
    .footer a {
        color: white;
        text-decoration: underline;
    }
    /* Expander personalizado */
    .streamlit-expanderHeader {
        font-weight: 600;
        color: var(--primary-dark);
    }
    /* Barra de progresso personalizada */
    .progress-container {
        width: 100%;
        height: 8px;
        background-color: #e9ecef;
        border-radius: 4px;
        margin-top: 10px;
        overflow: hidden;
    }
    .progress-bar {
        height: 100%;
        border-radius: 4px;
        transition: width 0.5s ease;
    }
    /* Badges */
    .badge {
        display: inline-block;
        padding: 0.25rem 0.5rem;
        border-radius: 9999px;
        font-size: 0.75rem;
        font-weight: 600;
        text-transform: uppercase;
        letter-spacing: 0.05em;
    }
    .badge-primary {
        background-color: var(--primary-light);
        color: var(--primary-dark);
    }
    .badge-secondary {
        background-color: var(--secondary-light);
        color: var(--secondary);
    }
    .badge-accent {
        background-color: var(--accent-light);
        color: var(--accent);
    }
    .badge-danger {
        background-color: var(--danger-light);
        color: var(--danger);
    }
</style>
""", unsafe_allow_html=True)

nutrition_palette = ["#3a86ff", "#38b000", "#ff9e00", "#9d4edd", "#ef476f", "#073b4c"]
nutrition_cmap = LinearSegmentedColormap.from_list("nutrition_cmap", ["#3a86ff", "#ef476f"])

# ----------------------------------------------------------
# Cabeçalho e Introdução
# ----------------------------------------------------------
st.markdown(
    f'<div class="main-header animate-fade-in">{nutrition_icons["main"]} Análise de Desnutrição Infantil no Brasil</div>',
    unsafe_allow_html=True
)

st.markdown("""
<div class="section animate-fade-in">
    <div style="display: flex; align-items: center; margin-bottom: 1rem;">
        <div style="background-color: #e1ebff; border-radius: 50%; width: 40px; height: 40px; display: flex; justify-content: center; align-items: center; margin-right: 1rem;">
            <span style="font-size: 1.5rem;">📊</span>
        </div>
        <h3 style="color: #0043a9; margin: 0; font-size: 1.5rem;">Sobre este Estudo</h3>
    </div>
    <p style="font-size: 1.05rem; line-height: 1.6; color: #555b6e;">
        Esta plataforma apresenta uma análise abrangente da desnutrição infantil no Brasil,
        identificando padrões regionais, determinantes socioeconômicos e fatores de infraestrutura
        que impactam o desenvolvimento das crianças brasileiras.
    </p>
    <div class="info-card">
        <div style="display: flex; align-items: flex-start;">
            <div style="font-size: 1.5rem; margin-right: 0.75rem;">ℹ️</div>
            <div>
                <p style="margin: 0; font-weight: 600; color: #0043a9;">Por que este estudo é importante?</p>
                <p style="margin-top: 0.5rem; margin-bottom: 0; color: #555b6e;">
                    A desnutrição infantil continua sendo um desafio significativo para a saúde pública no Brasil,
                    com impactos duradouros no desenvolvimento físico e cognitivo das crianças.
                    Compreender os fatores que contribuem para este problema é essencial para desenvolver
                    políticas públicas eficazes e intervenções direcionadas.
                </p>
            </div>
        </div>
    </div>
</div>
""", unsafe_allow_html=True)

# ----------------------------------------------------------
# Sidebar para Filtros e Controles
# ----------------------------------------------------------
with st.sidebar:
    st.image("C:\Users\ALUNOS MALOCA\Documents\Streamlit Hands-ON\Brasão_da_UFRR.png")
    st.title(f"Controle de Dados")
    st.markdown('<div style="border-bottom: 1px solid #e9ecef; margin-bottom: 20px;"></div>', unsafe_allow_html=True)

    uploaded_file = st.file_uploader("Carregar arquivo de dados", type=["csv"])

    st.markdown(
        '<p style="font-size: 0.9rem; color: #555b6e; font-weight: 500; margin-bottom: 0.5rem;">Filtros de Análise</p>',
        unsafe_allow_html=True
    )

    salas = st.multiselect(
        "Salas",
        options=['Norte', 'Sul', 'Sudeste', 'Centro-Oeste', 'Nordeste'],
        default=['Norte'],
        help="Escolha uma ou mais regiões para análise"
    )

    st.markdown('<p style="font-size: 0.9rem; color: #555b6e; margin-bottom: 0.25rem;">Faixa Etária (meses)</p>',
                unsafe_allow_html=True)
    faixa_etaria = st.select_slider(
        "",
        options=[0, 12, 24, 36, 48, 60],
        value=(0, 60),
        format_func=lambda x: f"{x} meses"
    )

    tipo_domicilio = st.multiselect(
        "Tipo de Domicílio",
        options=['Casa', 'Apartamento', 'Habitação em casa de cômodos'],
        default=['Casa', 'Apartamento']
    )

    acesso_alimentos = st.radio(
        "Acesso a Alimentos Básicos",
        options=["Todos", "Sim, sempre", "Sim, quase sempre", "Sim, às vezes"],
        index=0
    )

    st.markdown('<div style="border-bottom: 1px solid #e9ecef; margin: 20px 0;"></div>', unsafe_allow_html=True)

    st.markdown("""
    <div style="background-color: #e1ebff; padding: 15px; border-radius: 12px; margin-bottom: 20px;">
        <p style="font-weight: 600; color: #0043a9; margin-bottom: 8px;">Sobre os Indicadores</p>
        <ul style="margin: 0; padding-left: 20px; color: #555b6e; font-size: 0.9rem;">
            <li>Índice de Desenvolvimento: Média dos scores de alimentos, saúde e infraestrutura</li>
            <li>Score Alimentos: Disponibilidade de alimentos básicos</li>
            <li>Score Infraestrutura: Condições adequadas de moradia</li>
        </ul>
    </div>
    """, unsafe_allow_html=True)

    if st.button("Atualizar Análise", key="update_btn"):
        st.success("Análise atualizada com sucesso!")

    st.markdown(
        '<div style="margin-top: 30px; text-align: center; font-size: 0.8rem; color: #8d99ae;">Versão 3.0.0</div>',
        unsafe_allow_html=True
    )

