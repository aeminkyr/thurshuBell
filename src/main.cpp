#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h> // Brian Lough tarafından yazılmış Universal Telegram Bot Kütüphanesi: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

// Ağ kimlik bilgilerinizi buraya yazın
const char *ssid = "***";
const char *password = "****";

// Telegram BOT'unuzu başlatın
#define BOTtoken "*******:******************************" // Bot Token'ınız (Botfather'dan oluşturuabilirsiniz.)

// Bireysel veya grup sohbetinin chat ID'sini öğrenmek için @myidbot'u kullanabilirsiniz.
#define CHAT_ID "-******"

#ifdef ESP8266
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Yeni mesajları her 250ms' de bir kontrol eder.
int botRequestDelay = 250;
unsigned long lastTimeBotRan;

const int D4_PIN = D4;
const int D3_PIN = D3;
const int analogInPin = A0; // ESP8266 Analog Pin ADC0 = A0

int sensorValue = 0; // Analog Pin'in değeri

// D4 ve D3 pinlerini HIGH veya LOW olarak ayarlayan fonksiyon
void setPinsHighLow(bool isD4High, bool isD3High)
{
  digitalWrite(D4_PIN, isD4High ? HIGH : LOW);
  digitalWrite(D3_PIN, isD3High ? HIGH : LOW);
}

// Belirli zaman aralıklarında pin durum değişikliklerini yönetmek için fonksiyon
void handleOpenDoor()
{
  setPinsHighLow(false, true);
  delay(500);
  setPinsHighLow(true, false);
  delay(1000);
  setPinsHighLow(false, false);
}

// Yeni mesajlar alındığında ne yapılacağını yönetmek için fonksiyon
void handleNewMessages(int numNewMessages)
{
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++)
  {
    // İstekte bulunanın chat ID'si
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID)
    {
      // mesaj gelen kullanıcı id si ile önceden belirlediğimiz CHAT_ID eşleşmezse kullanıcıya yetkisiz kullanıcı olduğunu belirtir.
      bot.sendMessage(chat_id, "Yetkisiz kullanıcı", "");
      continue;
    }

    // Alınan mesajı yazdır
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/basla")
    {
      String welcome = "Merhaba, " + from_name + ".\n";
      welcome += "Kontrol için aşağıdaki komutları kullanabilirsiniz.\n\n";
      welcome += "/kapiyi_ac komutuyla bina kapısını açabilirsiniz.\n";
      welcome += "/durum komutuyla diafon durumunu görebilirsiniz. \n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/kapiyi_ac")
    {
      handleOpenDoor();
      bot.sendMessage(chat_id, "Kapı açıldı", "");
    }

    if (text == "/durum")
    {
      bot.sendMessage(chat_id, "Online.", "");
    }
  }
}

void setup()
{
  Serial.begin(115200);

#ifdef ESP8266
  configTime(0, 0, "pool.ntp.org"); // NTP sunucusu üzerinden UTC zamanını aldık.
  client.setTrustAnchors(&cert);    // api.telegram.org api'ı için gerekli ssl sertifijasını ekledik.
#endif

  pinMode(D4_PIN, OUTPUT);
  pinMode(D3_PIN, OUTPUT);
  digitalWrite(D3_PIN, HIGH);

  // Wi-Fi'ye bağlan
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
#ifdef ESP32
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
#endif
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Wi-Fi'ye bağlanılıyor..");
  }
  // ESP32 Yerel IP adresini yazdır
  Serial.println(WiFi.localIP());
}

void loop()
{
  if (millis() > lastTimeBotRan + botRequestDelay)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      // Serial.println("Yeni mesaj alındı");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

  sensorValue = analogRead(analogInPin); // analog değeri okuyup sensorValue'ya atadık.
  // Serial.println(sensorValue);

  if (sensorValue > 200 && sensorValue < 300)
  {
    // Okuduğumuz analog değer 200 - 300 arasında ise bot zil çalıyor mesajı yolluyor..
    bot.sendMessage(CHAT_ID, "Zil çalıyor! Kapıyı açmak için:  /kapiyi_ac ", "");
    // Serial.println(" : Zil çalıyor");
  }
}
