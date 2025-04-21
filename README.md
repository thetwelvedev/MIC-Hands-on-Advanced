# <img src="https://upload.wikimedia.org/wikipedia/commons/8/87/Arduino_Logo.svg" alt="Arduino Logo" width="40"> Monitoramento de Pacientes em Situação de Risco

**Componentes**: [Leonardo Castro](https://github.com/thetwelvedev), [Arthur Ramos](https://github.com/ArthurRamos26) e [Lucas Gabriel](https://github.com/lucasrocha777)

## Índice
- [ Monitoramento de Pacientes em Situação de Risco](#-monitoramento-de-pacientes-em-situação-de-risco)
  - [Índice](#índice)
  - [Etapas do Hands-on Avançado](#etapas-do-hands-on-avançado)
  - [Organograma](#organograma)
  - [Projeto Montado](#projeto-montado)
  - [Documentos](#documentos)
    - [Big Picture](#big-picture)
    - [Documento de Requisitos Funcionais](#documento-de-requisitos-funcionais)
    - [Esquema de Conexões](#esquema-de-conexões)
      - [Esquema de Ligação](#esquema-de-ligação)
      - [Pinagem](#pinagem)

## Etapas do Hands-on Avançado

- [x] Sprint 0
- [x] Sprint 1
- [x] Sprint 2
- [x] Sprint 3
- [ ] Apresentação - dia 25/04

## Organograma
![Organograma](/Pictures/organograma-arkham.png)

## Projeto Montado
![Prototipo](/Pictures/Proto-v1.jpeg)

## Documentos

### Big Picture
![Big Picture](/Pictures/big-picture-arkham.png)

### Documento de Requisitos Funcionais
[Acesse aqui](/Docs/Requisitos%20Funcionais_Maloca.Equipe_Arkhamdocx.pdf)

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

---

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

---

**Conexões do Display OLED (I²C)**  

| **Componente**       | **Pino ESP32** | **Descrição**                          |
|----------------------|---------------|----------------------------------------|
| **VCC**              | 3.3V          | Alimentação (3.3V do ESP32)            |
| **GND**              | GND           | Terra (compartilhado com o ESP32)      |
| **SDA**              | GPIO21        | Dados I²C (barramento compartilhado)   |
| **SCL**              | GPIO22        | Clock I²C (barramento compartilhado)   |

<!-- ## Códigos

### Código do Circuito

### Código do Dashboard
-->