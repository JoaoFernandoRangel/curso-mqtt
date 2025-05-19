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
                                     std::string key2);

void setup() {
  myFS.begin();
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
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
                                     std::string key2) {
  cJSON *root = cJSON_CreateObject();
  cJSON *key0_json = cJSON_CreateString(key0.c_str());
  cJSON *key1_json = cJSON_CreateString(key1.c_str());
  cJSON *key2_json = cJSON_CreateString(key2.c_str());

  cJSON_AddItemToObject(root, "key0", key0_json);
  cJSON_AddItemToObject(root, "key1", key1_json);
  cJSON_AddItemToObject(root, "key2", key2_json);

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