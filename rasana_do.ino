#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#include <Wire.h>
#define SensorPin A0          //pH meter Analog output to Arduino Analog Input 2
#define Offset -1.60 //Penyimpangan hasil pembacaan sensor dengan nilai yang sebenearnya
#define samplingInterval 20
#define printInterval 800
#define ArrayLenth  40    // panjang array
float pHValue, voltage, b;
int pHArray[ArrayLenth], buf[10], temp;;
int pHArrayIndex = 0;
unsigned long int avgValue;


//masukan username dan password agar andika_doorlock dapat terhubung ke server
const char* ssid = "EduFarm RR#1"; // masukan Nama Wifi nya
const char* password = "rasana#1"; // isi password dari wifi

String moist_value_string;
String moist_value_string2;
String postData;
String postData2;

#include <pgmspace.h>
#include <Wire.h>

//=========DEKLARASI VARIABEL DAN OBJEK UNTUK PEMBACAAN SENSOR======================
#define DoSensorPin  A0    //dissolved oxygen sensor analog output pin to arduino mainboard
#define VREF 3200
#define SCOUNT  30           // jumlah array untuk sampel data

int analogBuffer[SCOUNT];    //store the analog value in the array, readed from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;

float SaturationDoVoltage = 477.6; //577.6 nilai ini disesuaikan dengan kalibrasi
float SaturationDoTemperature = 25.0;
float averageVoltage;
float doValue;
float temperature = 25;    //default temperature is 25^C
const float SaturationValueTab[41] PROGMEM = {      //saturation dissolved oxygen concentrations at various temperatures
  14.46, 14.22, 13.82, 13.44, 13.09,
  12.74, 12.42, 12.11, 11.81, 11.53,
  11.26, 11.01, 10.77, 10.53, 10.30,
  10.08, 9.86,  9.66,  9.46,  9.27,
  9.08,  8.90,  8.73,  8.57,  8.41,
  8.25,  8.11,  7.96,  7.82,  7.69,
  7.56,  7.43,  7.30,  7.18,  7.07,
  6.95,  6.84,  6.73,  6.63,  6.53,
  6.41,
};

void ReadDO()
{
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 30U)  //every 30 milliseconds,read the analog value from the ADC
  {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(DoSensorPin);    //read the analog value and store into the buffer
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT)analogBufferIndex = 0;
  }
  for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
  {
    analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
  }
  averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;//1024.0 = 10 bit (arduino) sedangkan 4059.0= 12 bit (arduino) // baca nilai DO dengan metode median
  doValue = pgm_read_float_near( &SaturationValueTab[0] + (int)(SaturationDoTemperature + 0.5) ) * averageVoltage / SaturationDoVoltage; //calculate the do value, doValue = Voltage / SaturationDoVoltage * SaturationDoValue(with temperature compensation)


}

int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
  {
    bTab[i] = bArray[i];
  }
  int i, j, bTemp;
  /*Proses sorting data*/
  for (j = 0; j < iFilterLen - 1; j++)
  {
    for (i = 0; i < iFilterLen - j - 1; i++)
    {
      if (bTab[i] > bTab[i + 1])
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)bTemp = bTab[(iFilterLen - 1) / 2];
  else bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}


void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);

  WiFi.begin(ssid, password); //--> menghubungkan ke router wifi
  Serial.println("");
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.print("Sudah Terhubung ke : ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  ReadDO();
  moist_value_string = String(doValue);
  //program koneksi ke server
  WiFiClient client;
  HTTPClient http;
  postData = "do=" + moist_value_string;
  postData2 = "wt=" + moist_value_string2;
  http.begin(client, "http://192.168.137.1/rasana/getdata_do.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST(postData);
  String payload = http.getString();
  http.end();  //Close connection
  delay(500);
}
