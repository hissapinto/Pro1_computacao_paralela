#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

#include <stdint.h>

typedef enum { TEMPERATURA, UMIDADE, ENERGIA, CORRENTE, PRESSAO } SensorTipo;
typedef enum { OK, ALERTA, CRITICO } Status;

typedef struct { 
      // Vai guardar o id do sensor (0-19)
      int sensor_id; // 4 bytes

      char datetime_str[20];

      // Vai guardar o tipo do sensor
      SensorTipo tipo; // 4 bytes

      // Vai guardar dos dados extraidos do sensor
      float dados; // 4 bytes

      // Vai guardar o status do sensor
      Status status; // 4 bytes
} TSensor; // 24 bytes total

typedef struct {
      // Soma dos valores dos sensores
      // Exemplo: sensor_004 teve 3 leituras de temperatura: 21.1, 22.0, 20.3
      // soma = 21.1 + 22.0 + 20.3 = 63.4
      float soma; // Vai ser útil na média

      // Aqui é a soma de cada valor ao quadrado
      float soma_quadrados; // Vai ser útil no desvio padrão

      // Aqui é quantas leituras
      int contagem; // Vai ser útil na média e desvio padrão

      float media;
      float desvio;
} TStatSensor;

typedef struct {
      int sensor_id;
      char datetime[20];
      float valor;
} TAnomalia;

#endif
