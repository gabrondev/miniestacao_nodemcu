#include <Wire.h>  //INCLUSÃO DE BIBLIOTECA
#include <LiquidCrystal_I2C.h> //INCLUSÃO DE BIBLIOTECA
#include <ESP8266WiFi.h>
#include <Adafruit_BMP280.h>
#define BMP280_I2C_ADDRESS  0x76 // Define o endereço I2C do sensor: 0x76 ou 0x77 (0x77 é o endereço padrão da biblioteca)

const char* SSID = "SSID";
const char* SENHA = "SENHA";

int pinoAnalogicoMHRD = A0;
int pinoEnableMHRD = 13;
int valorSensorChuva = 0;
String estaChovendo;

LiquidCrystal_I2C lcd(0x3F, 16, 2);
Adafruit_BMP280  bmp280;
WiFiServer server(80);
 
void setup()
{ 
  // Inicialização da comunicação serial
  Serial.begin(115200);
 
  // Inicialização do LCD e limpeza de qualquer impressão anterior
  Wire.begin(D2, D1); // Alterando comunicação serial para o display LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // Inicialização do sensor BMP280
  Wire.begin(D4, D3);  // Alterando comunicação serial para o BMP280
  if(bmp280.begin(BMP280_I2C_ADDRESS) == 0){
    Wire.begin(D2, D1);
    lcd.setCursor(0,0);
    lcd.print("Erro no BMP280");
    lcd.setCursor(0,1);
    lcd.println("Averigue a falha");

    while(bmp280.begin(BMP280_I2C_ADDRESS) == 0) {
      delay(100000);
      Serial.print(".");
    }
  }
  Wire.begin(D2, D1);
  lcd.clear();

  // Inicialização do sensor de chuva MH-RD
  pinMode(pinoEnableMHRD, OUTPUT);

  // Inicialização da comunicação Wi-fi
  delay(10);
  Serial.println("");
  Serial.println("");
  Serial.print("Conectando a ");
  Serial.print(SSID);

  // Loop para se conectar à rede
  WiFi.begin(SSID, SENHA);
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");

      lcd.setCursor(0,0);
      lcd.print("Rede " + String(SSID));
      lcd.setCursor(0,1);
      lcd.print("Conectando   ");
      lcd.setCursor(10,1);
      delay(500);
      lcd.print(".");
      delay(500);
      lcd.print(".");
      delay(500);
      lcd.print(".");
      delay(500);
  }

  Serial.println("");
  Serial.print("Conectado a rede sem fio ");
  Serial.println(SSID);
  server.begin();
  Serial.println("Servidor iniciado");
  
  Serial.print("IP para se conectar ao NodeMCU: ");
  Serial.print("http://");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Conectado!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());

  Wire.begin(D4, D3); // Habilitando BMP280 novamente
}
 
void loop()
{
  WiFiClient client = server.available(); // Verifica se algum cliente está conectado no servidor

  // Se não existir cliente conectado, reexecuta o processo até que algum cliente se conecte ao servidor
  if (!client) {
    return;
  }
  
  Serial.println("Novo cliente se conectou!");
  while(!client.available()){
  }

  String request = client.readStringUntil('\r'); // Faz a leitura da primeira linha da requisição
  Serial.println(request); // Escreve a requisição na serial
  client.flush(); // Aguarda até que todos os dados de saída sejam enviados ao cliente

  float temperatura = bmp280.readTemperature();
  float pressao = (bmp280.readPressure())/100;
  float altitude = bmp280.readAltitude();

  valorSensorChuva = analogRead(pinoAnalogicoMHRD);
  valorSensorChuva = constrain(valorSensorChuva, 150, 440); 
  valorSensorChuva = map(valorSensorChuva, 150, 440, 1023, 0);
  
  if (valorSensorChuva>= 20){
    estaChovendo = "Está chovendo";
    digitalWrite(pinoEnableMHRD, HIGH);
  } else {
    estaChovendo = "Não está chovendo";
    digitalWrite(pinoEnableMHRD, LOW); 
  }

  // Comunicando ao servidor que será enviada uma página HTML
  client.println("HTTP/1.1 200 OK"); // Escreve para o cliente a versão do http
  client.println("Content-Type: text/html; charset=UTF-8"); // Escreve para o cliente o tipo de conteúdo(texto/html)
  client.println("");

  // Montagem da página HTML
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<head>");
  client.println("<title>Estação Meteorológica</title>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>"); // Meta tag para tornar a página responsiva
  client.println("<style>");
  client.println("body { font-family: 'Fira Sans', sans-serif; background-color: #B4BDFF; text-align: center; }");
  client.println("h1 { color: #333; }");
  client.println("p { font-size: 20px; }");
  client.println(".sensor-info { background-color: #fff; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.2); width: 90%; max-width: 600px; margin: 0 auto; }"); // Ajuste a largura máxima para se adaptar a diferentes tamanhos de tela
  client.println(".material-icons { font-family: 'Material Icons'; font-size: 24px; vertical-align: middle; }");
  client.println("</style>");
  client.println("<link rel='stylesheet' href='https://fonts.googleapis.com/icon?family=Material+Icons'>");
  client.println("<link rel='stylesheet' href='https://fonts.googleapis.com/css?family=Fira+Sans'>"); // Link para o Google Fonts
  client.println("</head>");
  client.println("<body>");
  client.println("<b><h1 style='color: white;'>Estação Meteorológica</h1></b>");
  client.println("<p style='color: white;'>Próxima atualização em <span id='countdown'>15</span> segundos</p>");
  client.println("<script>");
  client.println("function atualizarPagina() {");
  client.println("const countdownElement = document.getElementById('countdown');");
  client.println("let segundosRestantes = 15;");
  client.println("const atualizacaoInterval = setInterval(function() {");
  client.println("segundosRestantes--;");
  client.println("if (segundosRestantes === -1) {");
  client.println("clearInterval(atualizacaoInterval);");
  client.println("location.reload(); // Recarrega a página");
  client.println("} else {");
  client.println("countdownElement.textContent = segundosRestantes;");
  client.println("}");
  client.println("}, 1000); // Atualiza a cada 1 segundo");
  client.println("}");
  client.println("function atualizarDataHora() {");
  client.println("const dataHoraElement = document.getElementById('data-hora');");
  client.println("const agora = new Date();");
  client.println("const dataHoraFormatada = agora.toLocaleString('pt-BR');");
  client.println("dataHoraElement.textContent = dataHoraFormatada;");
  client.println("}");
  client.println("window.addEventListener('load', function() {");
  client.println("atualizarPagina();");
  client.println("setInterval(atualizarDataHora, 1000); // Atualiza a data e hora a cada 1 segundo");
  client.println("});");
  client.println("</script>");
  client.println("<div class='sensor-info'>");
  client.println("<p><i class='material-icons'>thermostat</i><b>Temperatura:</b> " + String(temperatura) + " ºC</p>");
  client.println("<p><i class='material-icons'>compress</i><b>Pressão:</b> " + String(pressao) + " hPa</p>");
  client.println("<p><i class='material-icons'>height</i><b>Altitude:</b> " + String(altitude) + " m</p>");
  client.println("<p><i class='material-icons'>cloud</i><b>Chuva:</b> " + estaChovendo + "</p>");
  client.println("</div>");
  client.println("<p id='data-hora' style='color: white;'></p>");
  client.println("</body>");
  client.println("</html>");

  Serial.println("Cliente desconectado");
  Serial.println("");
}
