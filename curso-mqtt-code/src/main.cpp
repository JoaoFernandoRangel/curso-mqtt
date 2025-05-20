#include "NTPClient.h"
#include "PubSubClient.h"
#include "WiFi.h"
#include "WiFiClient.h"
#include "WiFiUdp.h"
#include "cJSON.h"
#include "conf.h"
#include "lib_fs.h"
#include <Arduino.h>

void altera_valor_de_chave(std::string &json, std::string key,
                           std::string value);
void le_arquivo_e_imprime(std::string path, HardwareSerial *serial);
void le_arquivo_e_altera(std::string path, std::string addition);
std::string gera_json_para_impressão(std::string key0, std::string key1,
                                     std::string key2, std::string key3,
                                     std::string key4);
void thingsBoardTask(void *pvParameters);
void connectToWifi();
void manageMQTT();
void reconnectMQTT();
void manageWiFi();
void callback(char *topic, byte *payload, unsigned int length);

WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP _ntpUDP;
NTPClient _timeClient(_ntpUDP, "pool.ntp.org", -10800);

const char *mqtt_server = "demo.thingsboard.io";

void setup() {
  myFS.begin();
  Serial.begin(115200);
  xTaskCreatePinnedToCore(thingsBoardTask, "thingsBoardTask", 10000, NULL, 1,
                          NULL, 1); // Executa no núcleo APP (Core 1)
}

void loop() { vTaskSuspend(NULL); }

// Task que executa o conteúdo original do loop() no núcleo APP (Core 1)
void thingsBoardTask(void *pvParameters) {
  connectToWifi();
  _timeClient.begin();
  _timeClient.update();
  manageMQTT();
  while (true) {
    manageWiFi();
    if (!client.connected()) {
      manageMQTT();
    } else {
      client.loop();
      _timeClient.update();
    }
    vTaskDelay(pdMS_TO_TICKS(10)); // Pequeno delay para não ocupar 100% da CPU
  }
}

void connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi");
}

void manageMQTT() {
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  reconnectMQTT();
}

void manageWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWifi();
  }
}

void reconnectMQTT() {
  int contadorMQTT = 0;
  while (!client.connected() && contadorMQTT < 15) {
    Serial.print("Tentando conectar ao MQTT...");
    if (client.connect("ESP32Client", tbId, NULL)) {
      Serial.println("Conectado");
      client.subscribe("v1/devices/me/rpc/request/+");
      contadorMQTT = 0;
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tente novamente em 5 segundos");
      delay(5000);
      contadorMQTT++;
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  // Converta o payload para uma string
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
}

void le_arquivo_e_imprime(std::string path, HardwareSerial *serial) {
  std::string dados = myFS.readFile(path.c_str());
  serial->printf("O valor do arquivo é: %s\n", dados.c_str());
}

void le_arquivo_e_altera(std::string path, std::string addition) {
  std::string file = myFS.readFile(path.c_str());
  file += addition;
  myFS.writeFile(path.c_str(), file.c_str());
}

std::string gera_json_para_impressão(std::string key0, std::string key1,
                                     std::string key2, std::string key3,
                                     std::string key4) {
  cJSON *root = cJSON_CreateObject();
  cJSON *key0_json = cJSON_CreateString(key0.c_str());
  cJSON *key1_json = cJSON_CreateString(key1.c_str());
  cJSON *key2_json = cJSON_CreateString(key2.c_str());
  cJSON *key3_json = cJSON_CreateString(key3.c_str());
  cJSON *key4_json = cJSON_CreateString(key4.c_str());

  cJSON_AddItemToObject(root, "key0", key0_json);
  cJSON_AddItemToObject(root, "key1", key1_json);
  cJSON_AddItemToObject(root, "key2", key2_json);
  cJSON_AddItemToObject(root, "key3", key3_json);
  cJSON_AddItemToObject(root, "key4", key4_json);

  cJSON_Delete(root);
  std::string ret = cJSON_Print(root);

  return ret;
}

void altera_valor_de_chave(std::string &json, std::string key,
                           std::string value) {
  cJSON *root = cJSON_Parse(json.c_str());
  if (root == NULL) {
    Serial.println("Erro ao analisar o JSON");
    return;
  }

  cJSON *key_json = cJSON_GetObjectItem(root, key.c_str());
  if (key_json != NULL) {
    cJSON_SetValuestring(key_json, value.c_str());
  } else {
    Serial.printf("Chave %s não encontrada no JSON\n", key.c_str());
  }

  json = cJSON_Print(root);

  cJSON_Delete(root);
}