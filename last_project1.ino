#include <Wire.h>
#include "MAX30105.h"  // Nabız sensörümüzün kütüphanesi
#include <WiFi.h>
#include <WebServer.h> //web sunucu oluşturmak için eklendi

const char *ssid = "Kablonet Netmaster-2DC3-G";
const char *password = "Evcil321*";   // ESP32'nin internet ağına bağlanması için gerekli bilgiler

MAX30105 particleSensor; // MAX30105 particleSensor ifadesi, MAX30105 modelinde bir parçacık sensörünü temsil eden bir nesne (object) oluşturuyor. Bu nesne, MAX30105 sensörünü kontrol etmek, verileri okumak ve sensörle etkileşimde bulunmak için kullanılır.
WebServer server(80); //  ifadesi ise bir web sunucusu nesnesi oluşturur. Bu nesne, ESP8266 veya ESP32 gibi WiFi yetenekli mikrodenetleyiciler üzerinde bir HTTP sunucusu başlatmak ve yönetmek için kullanılır. Bu sunucu, cihazınıza gelen HTTP isteklerini işler ve belirli URL'lere yanıtlar gönderir.

void setup()
{
  Serial.begin(115200);   // baud değerimizi bu olarak ayarladık çıktıların düzgün gözükmesi için.
  Serial.println("Başlatılıyor...");

  // Sensörün başlatıldığında bir hata varsa bu gelecektir.
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) // Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 bulunamadı. Gücü veya bağlantıları kontrol edin. ");
    while (1);
  }

  // Plotterda testere dişi şeklinde düzgün grafiklerin oluşması için yapılan ayarlamalar. 
  byte ledBrightness = 0x1F; // Options: 0=Off to 255=50mA
  byte sampleAverage = 8;    // Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 3;          // Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  int sampleRate = 100;      // Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411;      // Options: 69, 118, 215, 411
  int adcRange = 4096;       // Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); // Sensörün konfigüre edilebileceği bazı seçenekler.

  // WiFi'ye bağlanma kısmı
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) // bağlanmaya çalışırken 500 ms gecikmelerle . koyar
  {
    delay(500); 
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Bağlanıyor "); // Bağlantılar oluştuğunda serial monitörde gördüğümüz mesajlar
  Serial.println(ssid);
  Serial.print("IP adresi: ");
  Serial.println(WiFi.localIP());

  // Set up the server  //Bu kod, bir WebServer nesnesi olan server üzerinde belirli HTTP yol (path) ve metod kombinasyonları için işleyici fonksiyonları atanmıştır. Bu HTTP yolları, belirli bir URL'ye yapılan HTTP GET isteklerini temsil eder. İşleyici fonksiyonlar, belirli bir URL'ye yapılan isteğe yanıt olarak çalışacak kodu içerir.
  server.on("/", HTTP_GET, handleRoot); // Bu ifade, kök dizin ("/") için bir HTTP GET isteği alındığında handleRoot adlı bir işleyici fonksiyonunun çağrılacağını belirtir. Yani, cihaza yapılan ana URL isteği için belirli bir işlevsellik sağlar.
  server.on("/irValue", HTTP_GET, handleIrValue);  // Bu ifade, "/irValue" dizini için bir HTTP GET isteği alındığında handleIrValue adlı bir işleyici fonksiyonunun çağrılacağını belirtir. Bu, belirli bir sensör değerini veya başka bir bilgiyi içeren bir sayfayı sunabilir.
  server.on("/smoothedRed", HTTP_GET, handleRedValue);  // Bu ifade, "/smoothedRed" dizini için bir HTTP GET isteği alındığında handleRedValue adlı bir işleyici fonksiyonunun çağrılacağını belirtir. Bu da belirli bir sensör değerini içeren başka bir sayfayı sunabilir.
  server.begin();  // web sunucusunu başlatır.
}

void loop()
{
  server.handleClient();  //  Bu ifade, web sunucusunun istemcilerle iletişim kurmasını ve gelen HTTP isteklerini işlemesini sağlar.
  delay(2);

  //float irValue = (particleSensor.getIR() * 0.0008);
  //Serial.println(irValue);  // serial plotterda gösterme kodu
  // BU 2 VERİNİN AYRI AYRI PLOTTERDA BAKILMASI DAHA SAĞLIKLI SONUÇLAR VERECEKTİR.
  //float smoothedRed = (particleSensor.getRed() * 0.00085);
  //Serial.println(smoothedRed);  // serial plotterda gösterme kodu
}

void handleRoot()  //  Bu fonksiyon, belirli bir web sayfasını oluşturur ve bu sayfada MAX30105 parçacık sensöründen alınan IR (infrared) ve düzeltilmiş kırmızı (smoothed red) değerleri gösterir.
{
  char msg[1500];
  float irValue = (particleSensor.getIR() * 0.0008);  // Ölçümlerin daha düzgün olması için belirli sayılarla çarpılmıştır.
  float smoothedRed = (particleSensor.getRed() * 0.00085);

  snprintf(msg, 1500,  // HTML kodlarımızın başlangıcı, bu kod içinde ölçüm değerlerimizi istenilen yerlere koyarak, bize ESP32nin verdiği IP adresinden bu verilerin konulduğu internet sayfasına ulaşırız.
           "<html>\
  <head>\
    <meta name='viewport' content='width=device-width, initial-scale=1'>\
    <title>Heart Rate Monitor</title>\
    <style>\
    html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center;}\
    h2 { font-size: 3.0rem; }\
    p { font-size: 3.0rem; }\   
    .units { font-size: 1.2rem; }\
    .dht-labels{ font-size: 1.5rem; vertical-align:middle; padding-bottom: 15px;}\
    </style>\
    <script>\
      function updateValues() {\
        var xhttpIr = new XMLHttpRequest();\
        xhttpIr.onreadystatechange = function() {\
          if (this.readyState == 4 && this.status == 200) {\
            document.getElementById('irValue').innerHTML = 'Heart Rate(BPM): ' + this.responseText;\
          }\
        };\
        xhttpIr.open('GET', '/irValue', true);\
        xhttpIr.send();\
        var xhttpRed = new XMLHttpRequest();\
        xhttpRed.onreadystatechange = function() {\
          if (this.readyState == 4 && this.status == 200) {\
            document.getElementById('smoothedRed').innerHTML = 'SPO2 Percentage: ' + this.responseText;\
          }\
        };\
        xhttpRed.open('GET', '/smoothedRed', true);\
        xhttpRed.send();\
      }\
      setInterval(updateValues, 100);\
    </script>\
  </head>\
  <body>\
      <h2>Heart Rate Monitor</h2>\
      <p id='irValue'></p>\
      <p id='smoothedRed'></p>\
  </body>\
</html>",
           irValue, smoothedRed);

  server.send(200, "text/html", msg);  // tarayıcıya gönderilecek HTML içeriğini belirtir. Bu sayede, belirli bir URL'ye yapılan isteğe karşılık olarak istemciye bir HTML sayfası gönderilir.
}

void handleIrValue()
{
  float irValue = (particleSensor.getIR() * 0.0008);
  server.send(200, "text/plain", String(irValue));  // bu IR(Infrared) değerini HTTP yanıtı olarak gönderir. HTTP yanıtının başlığı "text/plain" olarak belirlenir ve içerik olarak IR değeri kullanılır.
}

void handleRedValue()
{
  float smoothedRed = (particleSensor.getRed() * 0.00085);
  server.send(200, "text/plain", String(smoothedRed));  // bu düzeltilmiş kırmızı değeri(smoothedRed) HTTP yanıtı olarak gönderir. HTTP yanıtının başlığı "text/plain" olarak belirlenir ve içerik olarak düzeltilmiş kırmızı değeri kullanılır.
}
