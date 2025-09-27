#include <WiFi.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define pino_compressor 23
#define pino_sensor_temperatura 21
#define pino_ventilador_baixo 18
#define pino_ventilador_alto 19



const char* ssid = "SSID_REDE"; //ssid da rede
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
int ultimaTrocaCompressor = 0;
bool arCondicionadoLigado = false;

bool trocarEstadoCompressor(int agora){
  if (agora - ultimaTrocaCompressor > intervalo_compressor){
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





void refrigerar(int agora){
  if(!compressorLigado){
    if(trocarEstadoCompressor(agora)){
      ligarCompressor();
    }
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
            client.print("<!DOCTYPE html>");
            client.print("<html lang=\"pt-BR\">");
            client.print("<head>");
            client.print("<meta charset=\"UTF-8\">");
            client.print("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
            client.print("<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css2?family=Material+Symbols+Sharp:opsz,wght,FILL,GRAD@20..48,100..700,0..1,-50..200\" />");

            // configuração de estilo do site
            client.print("<style type=\"text/css\">* { margin: 0; padding: 0; color: white; }  body{ background-color: black; background-size: cover;");
            client.print("font-family: Arial, Helvetica, sans-serif; }  .main { display: flex; flex-direction: column; justify-content: space-between;");
            client.print("background-color: #131819; height: 100%; }  .display #on-off{ position: absolute; padding: 5%; font-weight: normal; }");
            client.print(".display .temperatura{ display: flex; justify-content: center; align-items: center; margin-top: 30px; }");
            client.print(".display .temperatura #temp{ font-size: 150px; font-weight: bold; }  .display .temperatura #cesius{ position: absolute;");
            client.print("font-size: 30px; margin-bottom: 75px; margin-left: 200px; font-weight: normal; }  .display #info-estado{"); 
            client.print("position: absolute; font-size: 25px; margin-bottom: -75px; margin-left: 300px; font-weight: normal; }");
            client.print(".display .textInfo-box{ display: flex; flex-direction: column; margin-left: 25%; margin-bottom: 20px; }");
            client.print(".display .textInfo{ font-size: 1.5rem; font-weight: normal; }  .display .textInfo-box .textInfo .refresh{position: absolute;");
            client.print("margin-left: 15px;color: blue;} .estado, .ajusteTemp{ padding: 5px; height: 100px; display: flex; justify-content: space-evenly;");
            client.print("align-items: center; text-align: center; background-color: black; border-bottom: 2px solid #131819; }  .estado .icon-box { width: 50%; }");
            client.print(".estado .icon-box.right-border{ border-right: 1px solid #131819; }  .estado .icon-box.left-border{ border-left: 1px solid #131819; }");
            client.print(".estado .icon, .ajusteTemp .icon{ font-size: 100px; }  .estado .icon-box .text-estado{ position: absolute; margin: 40px 0; font-size: 30px;");
            client.print("display: none; }  .estado .icon-box .text-estado.text-quente{ background: linear-gradient(75deg, red 3.5%, yellow 95%); background-clip: text;");
            client.print("-webkit-background-clip: text; -webkit-text-fill-color: transparent; }  .estado .icon-box .text-estado.text-frio{ margin-left: 25px;");
            client.print("background: linear-gradient(75deg, white 3.5%, blue 95%); background-clip: text; -webkit-background-clip: text; -webkit-text-fill-color: transparent; }");
            client.print(".estado .icon.frio:hover + .text-frio{ display: inline; }  .estado .icon.quente:hover + .text-quente{ display: inline; }  .ajusteTemp .info-temp{");
            client.print("font-size: 24px; }.ajust-left{ margin-left: 25px; }  .ajust-right{ margin-right: 25px; }  .ajusteTemp .aumentar .text-temp, .ajusteTemp .diminuir ");
            client.print(".text-temp{ position: absolute; margin: 40px 0; font-size: 30px; display: none; }  .ajusteTemp .icon.icon-aumentar:hover + .text-aumentar{");
            client.print("display: inline; }  .ajusteTemp .icon.icon-diminuir:hover + .text-diminuir{ display: inline; }  .velocidades{ display: flex; justify-content: space-evenly;");
            client.print("align-items: center; height: 135px; font-size: 24px; background-color: black; }  .velocidades .radio, .velocidades .ajust-radio{ margin-right: 10px;");
            client.print("cursor: pointer; } @media screen and (max-width: 450px){ .display #info-estado{ display: none; }  .display{ padding-top: 20px; }  .display .textInfo-box{");
            client.print("margin-left: 0; }  .display .textInfo{ font-size: 1.2rem; }  .estado .icon.frio:hover + .text-frio, .estado .icon.quente:hover + .text-quente, .ajusteTemp");
            client.print(".icon.icon-aumentar:hover + .text-aumentar, .ajusteTemp .icon.icon-diminuir:hover + .text-diminuir { display: none; pointer-events: none; }");
            client.print(".velocidades{ flex-direction: column; justify-content: space-between; height: auto; }  .velocidades .radio{ margin-right: 50px; }  .velocidades");
            client.print(".ajust-radio { margin: 35px 15px 35px 0; } }  @media screen and (min-width:450px) and (max-width: 600px){ .display{ padding-top: 20px; }  .display");
            client.print(".textInfo-box{ margin-left: 25px; }  .display .textInfo{ font-size: 1.2rem; }  .estado .icon.frio:hover + .text-frio, .estado .icon.quente:hover +");
            client.print(".text-quente, .ajusteTemp .icon.icon-aumentar:hover + .text-aumentar,.ajusteTemp .icon.icon-diminuir:hover + .text-diminuir {display: none;");
            client.print("pointer-events: none;}.velocidades{flex-direction: column;justify-content: space-between;height: auto;}.velocidades .radio{margin-right: 175px;}");
            client.print(".velocidades .ajust-radio {margin: 35px 15px 35px 0;}}@media screen and (min-width: 600px) and (max-width: 900px){.display .textInfo-box{");
            client.print("margin-left: 25px;}.ajusteTemp .icon.icon-aumentar:hover + .text-aumentar,.ajusteTemp .icon.icon-diminuir:hover + .text-diminuir {display: none;");
            client.print("pointer-events: none;}}@media screen and (min-width: 900px) and (max-width: 1100px){.display .textInfo-box{margin-left: 100px;}}");
            client.print("@media screen and (min-width: 1100px) and (max-width: 1300px){.display .textInfo-box{margin-left: 150px;}}</style>");

            client.print("<title>Controle Ar Condicionado</title>");
            client.print("</head>");

            // o conteúdo do cabeçalho HTTP
            client.print("<body>");
            client.print("<div class=\"main\">");
            client.print("<div class=\"display\">");
            
            
            if (arCondicionadoLigado)
            {
              client.print("<h1 id=\"on-off\">Ligado</h1>");
            }
            else
            {
              client.print("<h1 id=\"on-off\">Desligado</h1>");
            }

            client.print("<div class=\"temperatura\">");
            client.print("<h1 id=\"temp\">");
            if (arCondicionadoLigado)
            {
              client.print(temperaturaDesejada);
            }
            else
            {
              client.print("--");
            }
            client.print("</h1>");
            client.print("<h1 id=\"celsius\">°C</h1>");
            client.print("<hr>");
            client.print("<h1 class=\"textInfo\">Modo de Ventilação: ");
            
            if(arCondicionadoLigado)
            {
              if (potenciaVentiladorDesejada == 1)
              {
                client.print("Baixa");
              }else if(potenciaVentiladorDesejada == 2)
              {
                client.print("Alta");
              }else if(potenciaVentiladorDesejada == 0)
              {
                client.print("Desligado");
              }
            }
            else
            {
              client.print("Desligado");
            }

            client.print("</h1>");
            client.print("</div>"); //DIV TEMPERATURA
            client.print("</div>"); //DIV DISPLAY
            client.print("</div>"); //DIV MAIN

            client.print("<div class=\"ajusteTemp\">"); 

            client.print("<div style='text-align:center; margin:20px;'>");
            client.print("<a href=\"/ligar-desligar\" style='font-size:24px; color:white; text-decoration:none; background:#444; padding:10px 20px; border-radius:8px;'>");
            client.print(arCondicionadoLigado ? "Desligar" : "Ligar");
            client.print("</a>");
            client.print("</div>");

            client.print("<div style='text-align:center; margin:20px;'>");
            client.print("<a href=\"/info\" style='font-size:24px; color:white; text-decoration:none; background:#444; padding:10px 20px; border-radius:8px;'>");
            client.print("Info.");
            client.print("</a>");
            client.print("</div>");


            client.print("<div class=\"aumentar\">");
            client.print("<a href=\"/mais\">");
            client.print("<span class=\"material-symbols-sharp icon ajust-left icon-aumentar\">");
            client.print("add");
            client.print("</span>");
            client.print("<span class=\"text-temp text-aumentar\">Aumentar</span>");
            client.print("</a>");
            client.print("</div>");

            //client.print("<div class=\"info-temp\"></div>");//div info-temp abre e fecha

            client.print("<div class=\"diminuir\">"); //div botao diminuir
            client.print("<a href=\"/menos\">");
            client.print("<span class=\"material-symbols-sharp icon ajust-right icon-diminuir\">");
            client.print("remove");
            client.print("</span>");
            client.print("<span class=\"text-temp text-diminuir\">Diminuir</span>");
            client.print("</a>");
            client.print("</div>"); //div botao diminuir off

            client.print("</div>"); //finaliza div 

            client.print("<div class=\"velocidades\">"); //v1
            client.print("<a href=\"/v1\" class=\"radio\">");
            client.print("<input class=\"ajust-radio\" type=\"radio\" name=\"radio\" id=\"radio1\">");
            client.print("Velocidade Baixa.");
            client.print("</a>");
            //v2
            client.print("<a href=\"/v2\" class=\"radio\">");
            client.print("<input class=\"ajust-radio\" type=\"radio\" name=\"radio\" id=\"radio2\" >");
            client.print("Velocidade Alta.");
            client.print("</a>");
            //v0
            client.print("<a href=\"/v0\" class=\"radio\">");
            client.print("<input class=\"ajust-radio\" type=\"radio\" name=\"radio\" id=\"radio2\" >");
            client.print("Ventilador desligado.");
            client.print("</a>");



            client.print("</div>"); //div velocidades

            client.print("</body>");
            client.print("</html>");

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
        }

        if(currentLine.endsWith("GET /v1"))
        {
          potenciaVentiladorDesejada = 1;
        }

        if(currentLine.endsWith("GET /v0")){
          potenciaVentiladorDesejada = 0;
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
  }



  delay(500);
}
