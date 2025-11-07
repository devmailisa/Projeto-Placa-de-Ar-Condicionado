#include <WiFi.h>
#include <DallasTemperature.h>
#include <OneWire.h>

/*************** Pinos *****************/
#define pino_compressor        23
#define pino_sensor_temperatura 21
#define pino_ventilador_baixo  18
#define pino_ventilador_alto   19

/*************** Wi-Fi *****************/
const char* ssid= "ssid";
const char* password= "senha";
WiFiServer server(80);

/*************** Sensor Temp **********/
OneWire onewire(pino_sensor_temperatura);
DallasTemperature sensors(&onewire);
DeviceAddress endereco_sensor_temperatura;

/*************** Controle *************/
float  temperaturaDesejada = 20.0f;
const  float HISTER = 0.5f;               // histerese (±0,5 °C)
bool   arCondicionadoLigado = false;

// 0=off, 1=baixo, 2=alto
uint8_t potenciaVentiladorDesejada = 0;

/*************** Compressor ***********/
// Relé com lógica invertida: LOW=liga, HIGH=desliga
bool compressorLigado = false;

const uint32_t INTERVALO_COMPRESSOR = 90000UL; // 90 s
uint32_t ultimaTrocaCompressor = 0;

inline bool podeTrocarEstado(uint32_t agora) {
  return (uint32_t)(agora - ultimaTrocaCompressor) >= INTERVALO_COMPRESSOR;
}

void ligarCompressor() {
  digitalWrite(pino_compressor, LOW);
  compressorLigado = true;
  ultimaTrocaCompressor = millis();
}

void desligarCompressor() {
  digitalWrite(pino_compressor, HIGH);
  compressorLigado = false;
}

/*************** Ventiladores *********/
void ventiladorDesligado() {
  digitalWrite(pino_ventilador_baixo, HIGH);
  digitalWrite(pino_ventilador_alto, HIGH);
}
void ventiladorBaixo() {
  digitalWrite(pino_ventilador_baixo, LOW);
  digitalWrite(pino_ventilador_alto, HIGH);
}
void ventiladorAlto() {
  digitalWrite(pino_ventilador_baixo, HIGH);
  digitalWrite(pino_ventilador_alto, LOW);
}

/*************** Temperatura **********/
float lerTemperatura() {
  sensors.requestTemperatures();
  float temp = sensors.getTempC(endereco_sensor_temperatura);
  if (temp == DEVICE_DISCONNECTED|| temp == 85.0f) return NAN;
  return temp;
}

void diminuirTemperatura() {
  if (temperaturaDesejada > 15.0f) temperaturaDesejada -= 1.0f;
}
void aumentarTemperatura() {
  if (temperaturaDesejada < 35.0f) temperaturaDesejada += 1.0f;
}

/*************** Modo **********/
void desligarArCondicionado(uint32_t /*agora*/) {
  // Desligar imediato é sempre seguro
  desligarCompressor();
  ventiladorDesligado();
}

void refrigerar(uint32_t agora) {
  if (!compressorLigado && podeTrocarEstado(agora)) {
    ligarCompressor();
  }
  
  potenciaVentiladorDesejada = 2;
}

/*************** Setup **********/
void setup() {
  pinMode(pino_compressor, OUTPUT);
  pinMode(pino_ventilador_baixo, OUTPUT);
  pinMode(pino_ventilador_alto, OUTPUT);

  digitalWrite(pino_compressor, HIGH);     // compressor inicialmente desligado
  digitalWrite(pino_ventilador_baixo, HIGH);
  digitalWrite(pino_ventilador_alto, HIGH);

  Serial.begin(115200);
  delay(50);

  sensors.begin();
  uint8_t qtd = sensors.getDeviceCount();
  Serial.print(qtd, DEC);
  Serial.println(" sensor(es) de temperatura encontrado(s).");

  if (!sensors.getAddress(endereco_sensor_temperatura, 0)) {
    Serial.println("Sensor de temperatura NAO encontrado!");
  } else {
    sensors.setResolution(endereco_sensor_temperatura, 12);
  }

  // Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.println("Conectando...");
  }
  Serial.print("Conectado a ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

/*************** Loop **********/
void loop() {
  float    temperaturaAtual = lerTemperatura();
  uint32_t agora            = millis();

  if (!isnan(temperaturaAtual) && arCondicionadoLigado) {
    // Controle com histerese
    if (temperaturaAtual > temperaturaDesejada + HISTER) {
      refrigerar(agora);
    } else if (temperaturaAtual < temperaturaDesejada - HISTER && compressorLigado) {
      // desligar é sempre permitido
      desligarCompressor();
      potenciaVentiladorDesejada = 0;
    }

    // Aplica velocidade de ventilador
    if (potenciaVentiladorDesejada == 2) ventiladorAlto();
    else if (potenciaVentiladorDesejada == 1) ventiladorBaixo();
    else ventiladorDesligado();

  } else {
    // Sensor falhou (NaN) OU ar desligado → tudo desligado
    desligarArCondicionado(agora);
  }

  // --- Servidor HTTP ---
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Novo cliente conectado.");
    String currentLine = "";
    String firstLine   = "";
    bool requestEnded  = false;

    // Lê o request até a linha em branco (fim dos headers)
    uint32_t t0 = millis();
    while (client.connected() && !requestEnded && (millis() - t0 < 2000)) {
      if (client.available()) {
        char c = client.read();
        // Serial.write(c); // opcional: espelhar requisição no serial
        if (c == '\n') {
          // Primeira linha do request (ex.: "GET /v1 HTTP/1.1")
          if (firstLine.length() == 0) firstLine = currentLine;

          // Linha em branco indica fim do header
          if (currentLine.length() == 0) {
            requestEnded = true;
            break;
          }
          currentLine = "";
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    // Processa rota com a primeira linha do request
    if (firstLine.startsWith("GET /ligar-desligar ")) {
      arCondicionadoLigado = !arCondicionadoLigado;
      // Se desligar, respeite clique do usuário para manter ventilador off
      if (!arCondicionadoLigado) {
        potenciaVentiladorDesejada = 0;
      }
    } else if (firstLine.startsWith("GET /mais ")) {
      aumentarTemperatura();
    } else if (firstLine.startsWith("GET /menos ")) {
      diminuirTemperatura();
    } else if (firstLine.startsWith("GET /v2 ")) {
      potenciaVentiladorDesejada = 2;
    } else if (firstLine.startsWith("GET /v1 ")) {
      potenciaVentiladorDesejada = 1;
    } else if (firstLine.startsWith("GET /info ")) {
      Serial.print("Temperatura Desejada: ");
      Serial.print(temperaturaDesejada);
      Serial.print("°C | Temperatura do ambiente: ");
      if (isnan(temperaturaAtual)) Serial.print("N/A");
      else                         Serial.print(temperaturaAtual);
      Serial.println("°C");
      Serial.print("Compressor: ");
      Serial.println(compressorLigado ? "Ligado" : "Desligado");
      Serial.print("Tempo decorrido (ms): ");
      Serial.println(agora);
      Serial.println();
    }

    // Resposta HTTP (página)
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html; charset=UTF-8");
    client.println("Connection: close");
    client.println();
    client.print("<!DOCTYPE html><html lang=\"pt-BR\"><head>");
    client.print("<meta charset=\"UTF-8\">");
    client.print("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    client.print("<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css2?family=Material+Symbols+Sharp:opsz,wght,FILL,GRAD@20..48,100..700,0..1,-50..200\"/>");
    client.print("<title>Controle Ar Condicionado</title>");

    // ===== CSS Responsivo =====
    client.print("<style>");
    client.print(":root{--bg:#000;--panel:#131819;--text:#fff;--muted:#8aa0a3;--btn:#444;--radius:12px;");
    client.print("--fs-xxl:clamp(3rem,10vw,9rem);--fs-xl:clamp(1.25rem,3.5vw,2rem);--fs-lg:clamp(1rem,2.8vw,1.5rem);");
    client.print("--fs-md:clamp(0.95rem,2.2vw,1.15rem);--fs-sm:clamp(0.85rem,1.9vw,1rem);--gap:clamp(12px,2.5vw,20px);}");
    client.print("*{box-sizing:border-box;}html,body{height:100%;}body{margin:0;font-family:Arial,Helvetica,sans-serif;color:var(--text);background:var(--bg);}");
    client.print(".main{min-height:100vh;display:flex;flex-direction:column;gap:var(--gap);background:var(--panel);padding:clamp(12px,3vw,24px);}");
    client.print(".display{display:flex;flex-direction:column;gap:clamp(8px,2vw,12px);}#on-off{font-size:var(--fs-lg);font-weight:600;margin:0;color:var(--muted);}");
    client.print(".temperatura{display:flex;align-items:flex-end;justify-content:center;gap:clamp(8px,2.5vw,16px);padding:clamp(8px,2.5vw,16px);border-radius:var(--radius);background:#0c1011;}");
    client.print("#temp{font-size:var(--fs-xxl);line-height:1;font-weight:800;margin:0;}#celsius{font-size:clamp(1.2rem,4vw,2rem);margin:0 0 clamp(8px,1.8vw,12px) 0;color:var(--muted);font-weight:600;}");
    client.print(".textInfo{font-size:var(--fs-lg);text-align:center;margin:clamp(8px,2vw,16px) 0 0 0;font-weight:500;}");
    client.print(".ajusteTemp{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:var(--gap);} .btn{display:flex;align-items:center;justify-content:center;gap:10px;");
    client.print("padding:clamp(10px,3vw,14px);background:var(--btn);border-radius:var(--radius);text-decoration:none;color:var(--text);font-size:var(--fs-lg);font-weight:600;min-height:52px;transition:transform .08s ease,opacity .15s ease;user-select:none;-webkit-tap-highlight-color:transparent;}");
    client.print(".btn:active{transform:scale(.98);opacity:.9;} .icon{font-size:clamp(28px,6vw,40px);}");
    client.print(".velocidades{display:flex;flex-wrap:wrap;gap:var(--gap);justify-content:center;align-items:center;background:#0b0f10;padding:clamp(10px,3vw,16px);border-radius:var(--radius);}");
    client.print("input[type=radio]{position:absolute;opacity:0;pointer-events:none;} .radio{display:flex;align-items:center;gap:10px;padding:clamp(10px,2.8vw,14px) clamp(14px,4vw,18px);border-radius:var(--radius);text-decoration:none;color:var(--text);background:#1a2224;font-size:var(--fs-md);min-height:48px;transition:background .15s ease,transform .08s ease;}");
    client.print(".radio:active{transform:scale(.98);} .radio:hover{background:#213034;} .velocidades a{text-decoration:none;} .row{display:flex;flex-wrap:wrap;gap:var(--gap);align-items:center;justify-content:center;} .grow{flex:1 1 220px;}");
    client.print("@media(max-width:860px){.ajusteTemp{grid-template-columns:repeat(2,minmax(0,1fr));}}");
    client.print("@media(max-width:480px){.ajusteTemp{grid-template-columns:1fr;}.textInfo{font-size:var(--fs-md);}}");
    client.print("</style>");
    client.print("</head>");

    // ===== BODY =====
    client.print("<body><div class=\"main\"><div class=\"display\">");

    // ON/OFF
    client.print("<h1 id=\"on-off\">");
    client.print(arCondicionadoLigado ? "Ligado" : "Desligado");
    client.print("</h1>");

    // TEMPERATURA
    client.print("<div class=\"temperatura\"><h1 id=\"temp\">");
    if (arCondicionadoLigado && !isnan(temperaturaAtual)) {
      client.print(temperaturaDesejada, 0);
    } else {
      client.print("--");
    }
    client.print("</h1><h1 id=\"celsius\">°C</h1></div>");

    // MODO DE VENTILAÇÃO
    client.print("<h1 class=\"textInfo\">Modo de Ventilação: ");
    if (arCondicionadoLigado) {
      if (potenciaVentiladorDesejada == 1)      client.print("Baixa");
      else if (potenciaVentiladorDesejada == 2) client.print("Alta");
      else                                      client.print("Desligado");
    } else {
      client.print("Desligado");
    }
    client.print("</h1></div>");

    // AÇÕES PRINCIPAIS
    client.print("<div class=\"ajusteTemp\">");
    // Ligar/Desligar
    client.print("<a class=\"btn\" href=\"/ligar-desligar\"><span class=\"material-symbols-sharp icon\">power_settings_new</span><span>");
    client.print(arCondicionadoLigado ? "Desligar" : "Ligar");
    client.print("</span></a>");
    // Info
    client.print("<a class=\"btn\" href=\"/info\"><span class=\"material-symbols-sharp icon\">info</span><span>Info</span></a>");
    // Aumentar
    client.print("<a class=\"btn\" href=\"/mais\"><span class=\"material-symbols-sharp icon\">add</span><span>Aumentar</span></a>");
    // Diminuir
    client.print("<a class=\"btn\" href=\"/menos\"><span class=\"material-symbols-sharp icon\">remove</span><span>Diminuir</span></a>");
    client.print("</div>");

    // VELOCIDADES
    client.print("<div class=\"velocidades\">");
    // V1
    client.print("<a href=\"/v1\" class=\"radio grow\"><input class=\"ajust-radio\" type=\"radio\" name=\"radio\" id=\"radio1\">");
    client.print("<span class=\"material-symbols-sharp\">air</span><span>Velocidade Baixa</span></a>");
    // V2
    client.print("<a href=\"/v2\" class=\"radio grow\"><input class=\"ajust-radio\" type=\"radio\" name=\"radio\" id=\"radio2\">");
    client.print("<span class=\"material-symbols-sharp\">air</span><span>Velocidade Alta</span></a>");
    client.print("</div>");

    client.print("</div></body></html>");
    client.println();

    client.stop();
    Serial.println("Desconectado.");
  }
}
