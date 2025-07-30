#include <WiFi.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define pino_compressor 23
#define pino_sensor_temperatura 21
#define pino_ventilador_baixo 18
#define pino_ventilador_alto 19

const int intervalo_compressor = 90000; //milissegundos

/*************************WiFi*************************/
const char* ssid = "brisa-2044881"; //ssid da rede
const char* password =  "ur7nknzt"; //senha da rede

WiFiServer server(80); //Determina que a esp32 irá iniciar um servido Web que irá operar na porta 80 da rede WiFi

float temperaturaDesejada = 20;
float potenciaVentiladorDesejada = 1; //ventilador no modo baixo

/*
Estado de operacao do ventilador
0 -> desligado
1 -> baixo
2 -> alto
*/

int modoVentilador = 0; 
int ultimaTrocaCompressor = 0;
bool compressorLigado = false;
bool arCondicionadoLigado = false;

OneWire onewire(pino_sensor_temperatura);
DallasTemperature sensor(&onewire);

/*************************TEMPERATURA*************************/

float lerTemperatura() {
  sensor.requestTemperatures();
  DeviceAddress sensor1Address;
  sensor.getAddress(sensor1Address, 0);
  float temperatura = sensor.getTempC(sensor1Address);
  return temperatura;
}

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
void ligarCompressor(){
  digitalWrite(pino_compressor, LOW); 
  compressorLigado = true;
}

void desligarCompressor(){
  digitalWrite(pino_compressor, HIGH);
  compressorLigado = false;
}

bool trocarEstadoCompressor(int agora){
  if (agora - ultimaTrocaCompressor > intervalo_compressor){
    return true;
  }

  return false;
}

/*************************VENTILADORES*************************/
void ventiladorDesligado(){
  digitalWrite(pino_ventilador_baixo, LOW);
  digitalWrite(pino_ventilador_alto, LOW);
  
  modoVentilador = 0;

  Serial.println("Ventilador: Desligado");
}

void ventiladorBaixo(){
  digitalWrite(pino_ventilador_baixo, HIGH);
  digitalWrite(pino_ventilador_alto, LOW);

  modoVentilador = 1;

  Serial.println("Ventilador: Baixo");
}

void ventiladorAlto(){
  digitalWrite(pino_ventilador_baixo, LOW);
  digitalWrite(pino_ventilador_alto, HIGH);

  modoVentilador = 2;

  Serial.println("Ventilador: Alto");
}

/*********************ON/OFF*********************/

void desligarArCondicionado(int agora){
  if(trocarEstadoCompressor(agora)){
    desligarCompressor();
  }

  ventiladorDesligado();
}

void setup() {
  pinMode(pino_compressor, OUTPUT);
  pinMode(pino_ventilador_baixo, OUTPUT);
  pinMode(pino_ventilador_alto, OUTPUT);

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

void loop() {
  if (arCondicionadoLigado){
    float temperaturaEvaporador = lerTemperatura();
    unsigned long tempoDecorrido = millis();

    if(temperaturaEvaporador > temperaturaDesejada && trocarEstadoCompressor(tempoDecorrido)){
      ligarCompressor();
      ultimaTrocaCompressor = tempoDecorrido;
    } else if(temperaturaEvaporador <= temperaturaDesejada && trocarEstadoCompressor(tempoDecorrido)){
      desligarCompressor();
      ultimaTrocaCompressor = tempoDecorrido;
    }

    if(potenciaVentiladorDesejada == 0){
      ventiladorDesligado();
    } else if(potenciaVentiladorDesejada == 1){
      ventiladorBaixo();
    } else if(potenciaVentiladorDesejada == 2){
      ventiladorAlto();
    }

    Serial.print("Temperatura Desejada: ");
    Serial.print(temperaturaDesejada);
    Serial.print("°C | Temperatura do ambiente: ");
    Serial.print(temperaturaEvaporador);
    Serial.println("°C");
    Serial.println();
    Serial.print("Compressor: ");
    Serial.println(compressorLigado ? "Ligado" : "Desligado");
    Serial.print("Tempo decorrido: ");
    Serial.println(tempoDecorrido);
    Serial.println();
  }else{
    desligarArCondicionado(tempoDecorrido);
  }

  WiFiClient client = server.available(); //Avalia se existe um cliente disponível para estabelecer a conexão

  if (client){
    Serial.println("Novo cliente conectado.");
    String currentLine = "";
    while (client.connected()){
      if (client.available()){
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            /* ALTERAR AQUI
            client.print("Click <a href=\"/H\">here</a> to turn the LED on pin 2 on.<br>");
            client.print("Click <a href=\"/L\">here</a> to turn the LED on pin 2 off.<br>");
            */

            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }

        /*********************Botao ligar e desligar**********************/
        if (currentLine.endsWith("GET /ligar-desligar")) { 
          arCondicionadoLigado = !arCondicionadoLigado;
        } 

        /*********************Modo-ventilador**********************/
        if (currentLine.endsWith("GET /ligar-desligar")) { 
          arCondicionadoLigado = !arCondicionadoLigado;
        } 

        

        
      }
    }
    client.stop();
    Serial.println("Cliente Desconectado.");
  }
}
