#include <ESP8266WiFi.h>
#include "secrets.h"
#include "ThingSpeak.h" // always include thingspeak header file after other header files and custom macros
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

char ssid[] = SECRET_SSID;   // your network SSID (name) 
char pass[] = SECRET_PASS;   // your network password
WiFiClient client;

unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;


// Configuración del DHT11
#define DHTPIN D4     // Pin conectado al DATA del DHT11
#define DHTTYPE DHT11 // Modelo del sensor (DHT11)

// Configuración del LED
#define LED_PIN D2 // Pin conectado al LED (GPIO2)

DHT dht(DHTPIN, DHTTYPE);

// Variables para calcular el promedio
float sumaTemperatura = 0;
float sumaHumedad = 0;
int contadorLecturas = 0;
int estadoLed = 1;



void setup() {
  Serial.begin(9600);  // Initialize serial
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo native USB port only
  }
  
  WiFi.mode(WIFI_STA); 
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  Serial.println("Inicializando DHT11...");
  dht.begin();

  // Configurar el pin del LED como salida
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // Connect or reconnect to WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print("Conexión fallida. Estado WiFi: ");
      Serial.println(WiFi.status());
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(10000);     
    } 
    Serial.println("\nConnected.");
  }
  
  float humedad = dht.readHumidity();
  float temperatura = dht.readTemperature();

  // Verificar si hay error de lectura
  if (isnan(humedad) || isnan(temperatura)) {
    Serial.println("Error al leer el sensor DHT11");
    analogWrite(LED_PIN, 0); // Apagar LED en caso de error
    return;
  }

  // Sumar valores para el promedio
  sumaTemperatura += temperatura;
  sumaHumedad += humedad;
  contadorLecturas++;
  

  // Encender LED con brillo moderado


  // Si se han realizado 5 lecturas, calcular y mostrar el promedio
  if (contadorLecturas == 5) {
    float promedioTemperatura = sumaTemperatura / 5;
    float promedioHumedad = sumaHumedad / 5;

    Serial.print("Promedio Temperatura: ");
    Serial.println(promedioTemperatura);
    Serial.print("Promedio Humedad: ");
    Serial.println(promedioHumedad);

    ThingSpeak.setField(1, promedioTemperatura);
    ThingSpeak.setField(2, promedioHumedad);



    int responseCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (responseCode == 200) {
      Serial.println("Datos enviados con éxito.");
    } else {
      Serial.println("Error al enviar datos. Código HTTP: " + String(responseCode));
    }

    sumaTemperatura = 0;
    sumaHumedad = 0;
    contadorLecturas = 0;

    delay(15000); // Retraso obligatorio
  }
  float aviso = ThingSpeak.readFloatField(myChannelNumber, 3, myWriteAPIKey);
  if (isnan(aviso)) {
    Serial.println("Error al leer el Field 3.");
  } else if (aviso == 1) {
    Serial.println("Aviso recibido: Parpadeo continuo del LED.");
    while (true) {
      analogWrite(LED_PIN, 125); // Brillo moderado
      delay(200); // LED encendido por 500 ms
      analogWrite(LED_PIN, 0);   // Apagar LED
      delay(200); // LED apagado por 500 ms


      // Leer nuevamente el valor del Field 3 para verificar si se debe continuar
      aviso = ThingSpeak.readFloatField(myChannelNumber, 3, myWriteAPIKey);
      if (isnan(aviso) || aviso != 1) {
        Serial.println("Aviso desactivado: Deteniendo parpadeo del LED.");
        analogWrite(LED_PIN, 0); // Asegurar que el LED está apagado
        
        break; // Salir del bucle
      }
    }
  } else {
    Serial.println("No hay aviso: LED apagado.");
    analogWrite(LED_PIN, 0); // Apagar LED
  }
 
}
