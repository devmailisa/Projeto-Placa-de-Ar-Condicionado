# Sistema IoT para Controle de Ar-Condicionado com ESP32 e HTTP

Este projeto faz parte do **Programa Institucional de Bolsas de Iniciação Científica (PIDETEC-EAD)** – Edital nº10/2025.  
Seu objetivo é desenvolver um **protótipo IoT** para monitorar e controlar variáveis de experimentos com estudantes de Educação a Distância (EaD).  

O sistema proposto utiliza um **ESP32** para controlar o funcionamento de um ar-condicionado, integrando **sensores de temperatura** e **interface web** hospedada na própria placa.

---

## 🔧 Hardware Utilizado
- **ESP32**
- **Sensor de temperatura DS18B20** (pino GPIO 21)
- **3 relés** para acionamento:
  - Compressor → GPIO 23  
  - Ventilador Baixa → GPIO 19  
  - Ventilador Alta → GPIO 18  

---

## 🌡️ Lógica de Funcionamento
1. O usuário define a **temperatura desejada** pela página web do ESP32.  
2. O sistema lê a **temperatura ambiente** via DS18B20.  
3. As condições de controle são:
   - Se **temperatura ambiente > desejada** e o **compressor** não mudou de estado nos últimos **90 s**:  
     - Liga compressor (GPIO 23)  
     - Liga ventilador alta (GPIO 18)  
   - Se a temperatura for **normalizada**:  
     - Mantém apenas ventilador baixa (GPIO 19)  
     - Se o compressor estiver ligado e o seu estado não tiver sido alterado em **menos de 90s**,deve ser desligado.

⚡ O atraso de 90 segundos evita ciclos rápidos de liga/desliga, protegendo o compressor.

---

## 🌐 Interface Web
O ESP32 hospeda uma **página HTTP** que permite:  
- Ligar/desligar o sistema  
- Ajustar a temperatura de referência  
- Visualizar a temperatura atual  

A comunicação é feita via **servidor HTTP local**, sem necessidade de serviços externos.

---

## 🚀 Como Usar
1. Faça upload do código para o ESP32 via Arduino IDE ou PlatformIO.  
2. Conecte o ESP32 à rede Wi-Fi configurada.  
3. Acesse o IP do ESP32 no navegador para abrir a interface.  
4. Configure a temperatura e controle o ar-condicionado remotamente.  

---

## 📚 Objetivos Educacionais
- Exploração prática de conceitos de **IoT, automação e controle**.  
- Experimentos remotos com monitoramento de variáveis ambientais.  
- Aprendizagem ativa em disciplinas de Engenharia e áreas correlatas.

---

## 🔮 Próximos Passos
- Integração com banco de dados para registro histórico de temperaturas.  
- Adição de sensores extras (umidade, corrente elétrica, vibração).  

---
