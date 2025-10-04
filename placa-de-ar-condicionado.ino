#include <WiFi.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define pino_compressor 23
#define pino_sensor_temperatura 21
#define pino_ventilador_baixo 18
#define pino_ventilador_alto 19



const char* ssid = "SUA_REDE"; //ssid da rede
const char* password =  "SENHA"; //senha da rede
WiFiServer server(80);


OneWire onewire(pino_sensor_temperatura);
DallasTemperature sensor(&onewire);

float lerTemperatura() {
  sensor.requestTemperatures();
  DeviceAddress sensor1Address;
  sensor.getAddress(sensor1Address, 0);
  float temperatura = sensor.getTempC(sensor1Address);
  return temperatura;
}

float temperaturaDesejada = 20;

void diminuirTemperatura(){
  if(temperaturaDesejada > 15){
    temperaturaDesejada--;
  }
}

void aumentarTemperatura(){
  if(temperaturaDesejada < 35){
    temperaturaDesejada++;
  }
}





/*************************COMPRESSOR*************************/
//O relé do compressor tem a lógica invertida pois ele ativa no nível lógico LOW;
//E desativa com o nível lógico HIGH.

bool compressorLigado = false;

void ligarCompressor(){
  digitalWrite(pino_compressor, LOW); 
  compressorLigado = true;
}

void desligarCompressor(){
  digitalWrite(pino_compressor, HIGH);
  compressorLigado = false;
}



const int intervalo_compressor = 90000; //milissegundos
int ultimaTrocaCompressor = -90000;
bool arCondicionadoLigado = false;

bool trocarEstadoCompressor(int agora){
  if (agora - ultimaTrocaCompressor >= intervalo_compressor){
    ultimaTrocaCompressor = agora;
    return true;
  }

  return false;
}







/*************************VENTILADORES*************************/
float potenciaVentiladorDesejada = 0; 

void ventiladorDesligado(){
  digitalWrite(pino_ventilador_baixo, HIGH);
  digitalWrite(pino_ventilador_alto, HIGH);
}

void ventiladorBaixo(){
  digitalWrite(pino_ventilador_baixo, LOW);
  digitalWrite(pino_ventilador_alto, HIGH);
}

void ventiladorAlto(){
  digitalWrite(pino_ventilador_baixo, HIGH);
  digitalWrite(pino_ventilador_alto, LOW);
}

/*********************ON/OFF*********************/

void desligarArCondicionado(int agora){
  if(trocarEstadoCompressor(agora)){
    desligarCompressor();
  }
  
  ventiladorDesligado();
}

//Verifica se o usuário clicou no estado de "Ventilação desligada" naquele instante
bool clicou = false;


void refrigerar(int agora){
  if(!compressorLigado){
    if(trocarEstadoCompressor(agora)){
      ligarCompressor();
    }
  }

  if(potenciaVentiladorDesejada == 0 and !clicou){
    potenciaVentiladorDesejada = 2;
  }
}




void setup() {
  pinMode(pino_compressor, OUTPUT);
  pinMode(pino_ventilador_baixo, OUTPUT);
  pinMode(pino_ventilador_alto, OUTPUT);

  digitalWrite(pino_compressor, HIGH);
  digitalWrite(pino_ventilador_baixo, HIGH);
  digitalWrite(pino_ventilador_alto, HIGH);
  Serial.begin(115200);

  //Inicia a conexão WiFi
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Conectando...");
  }
  Serial.print("Conectado a ");
  Serial.println(ssid);
  Serial.println(WiFi.localIP());
  server.begin();
  //Para estabelecer a conexão, é necessário abrir um browser e digitar o endereço IP correspondente
}

void loop() 
{
  float temperaturaAtual = lerTemperatura();
  int agora = millis();

  if(arCondicionadoLigado)
  {
    if(temperaturaAtual > temperaturaDesejada){
      refrigerar(agora);
    }else{
      if(compressorLigado && trocarEstadoCompressor(agora)){
        desligarCompressor();
      }
    }

    if(potenciaVentiladorDesejada == 2){
      ventiladorAlto();
    } else if(potenciaVentiladorDesejada == 1){
      ventiladorBaixo();
    } else if(potenciaVentiladorDesejada == 0){
      ventiladorDesligado();
    }
  }else
  {
    desligarArCondicionado(agora);
  }


  WiFiClient client = server.available(); //Avalia se existe um cliente disponível para estabelecer a conexão

  if (client)
  {
    Serial.println("Novo cliente conectado.");
    String currentLine = "";
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        Serial.write(c);
        if (c == '\n')
        {
          if (currentLine.length() == 0) 
          {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print("<!DOCTYPE html><html lang=\"pt-BR\"><head>");
            client.print("<meta charset=\"UTF-8\">");
            client.print("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.print("<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css2?family=Material+Symbols+Sharp:opsz,wght,FILL,GRAD@20..48,100..700,0..1,-50..200\"/>");
            client.print("<title>Controle Ar Condicionado</title>");

            /* ===== CSS Responsivo ===== */
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

            /* ===== BODY ===== */
            client.print("<body><div class=\"main\"><div class=\"display\">");

            /* ON/OFF */
            client.print("<h1 id=\"on-off\">");
            client.print(arCondicionadoLigado ? "Ligado" : "Desligado");
            client.print("</h1>");

            /* TEMPERATURA */
            client.print("<div class=\"temperatura\"><h1 id=\"temp\">");
            if (arCondicionadoLigado) { client.print(temperaturaDesejada); } else { client.print("--"); }
            client.print("</h1><h1 id=\"celsius\">°C</h1></div>");

            /* MODO DE VENTILAÇÃO */
            client.print("<h1 class=\"textInfo\">Modo de Ventilação: ");
            if (arCondicionadoLigado) {
              if (potenciaVentiladorDesejada == 1)      client.print("Baixa");
              else if (potenciaVentiladorDesejada == 2) client.print("Alta");
              else                                      client.print("Desligado");
            } else {
              client.print("Desligado");
            }
            client.print("</h1></div>");

            /* AÇÕES PRINCIPAIS */
            client.print("<div class=\"ajusteTemp\">");
            /* Ligar/Desligar */
            client.print("<a class=\"btn\" href=\"/ligar-desligar\"><span class=\"material-symbols-sharp icon\">power_settings_new</span><span>");
            client.print(arCondicionadoLigado ? "Desligar" : "Ligar");
            client.print("</span></a>");
            /* Info */
            client.print("<a class=\"btn\" href=\"/info\"><span class=\"material-symbols-sharp icon\">info</span><span>Info</span></a>");
            /* Aumentar */
            client.print("<a class=\"btn\" href=\"/mais\"><span class=\"material-symbols-sharp icon\">add</span><span>Aumentar</span></a>");
            /* Diminuir */
            client.print("<a class=\"btn\" href=\"/menos\"><span class=\"material-symbols-sharp icon\">remove</span><span>Diminuir</span></a>");
            client.print("</div>");

            /* VELOCIDADES */
            client.print("<div class=\"velocidades\">");
            /* V1 */
            client.print("<a href=\"/v1\" class=\"radio grow\"><input class=\"ajust-radio\" type=\"radio\" name=\"radio\" id=\"radio1\">");
            client.print("<span class=\"material-symbols-sharp\">air</span><span>Velocidade Baixa</span></a>");
            /* V2 */
            client.print("<a href=\"/v2\" class=\"radio grow\"><input class=\"ajust-radio\" type=\"radio\" name=\"radio\" id=\"radio2\">");
            client.print("<span class=\"material-symbols-sharp\">air</span><span>Velocidade Alta</span></a>");
            /* V0 */
            client.print("<a href=\"/v0\" class=\"radio grow\"><input class=\"ajust-radio\" type=\"radio\" name=\"radio\" id=\"radio3\">");
            client.print("<span class=\"material-symbols-sharp\">air</span><span>Ventilador Desligado</span></a>");
            client.print("</div>");

            client.print("</div></body></html>");
            client.println();


            break;
          } 
          else 
          {
            currentLine = "";
          }
        }else if (c != '\r')
        {
          currentLine += c;
        }

        //CHAMADAS DA INTERAÇÃO DO USUÁRIO
        if (currentLine.endsWith("GET /ligar-desligar")) 
        { 
          arCondicionadoLigado = !arCondicionadoLigado;
        } 

        if (currentLine.endsWith("GET /mais")) 
        { 
          aumentarTemperatura();
        } 

        if (currentLine.endsWith("GET /menos")) 
        { 
          diminuirTemperatura();
        }

        if (currentLine.endsWith("GET /v2"))
        {
          potenciaVentiladorDesejada = 2;
          clicou = false;
        }

        if(currentLine.endsWith("GET /v1"))
        {
          potenciaVentiladorDesejada = 1;
          clicou = false;
        }

        if(currentLine.endsWith("GET /v0")){
          potenciaVentiladorDesejada = 0;
          clicou = true;
        }

        if(currentLine.endsWith("GET /info")){
          Serial.print("Temperatura Desejada: ");
          Serial.print(temperaturaDesejada);
          Serial.print("°C | Temperatura do ambiente: ");
          Serial.print(temperaturaAtual);
          Serial.println("°C");
          Serial.println();
          Serial.print("Compressor: ");
          Serial.println(compressorLigado ? "Ligado" : "Desligado");
          Serial.print("Tempo decorrido: ");
          Serial.println(agora);
          Serial.println();
        }

      }
    }
    client.stop();
    Serial.println("Desconectado.");
  }else{
    Serial.print(".");
  }

}
