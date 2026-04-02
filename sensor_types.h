#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

#include <stdint.h>
#include <stdio.h>

typedef enum { TEMPERATURA, UMIDADE, ENERGIA, CORRENTE, PRESSAO } SensorTipo;
typedef enum { OK, ALERTA, CRITICO } Status;

typedef struct { 
      int sensor_id;          // id do sensor (1-20)
      char datetime_str[20];  // data e hora no formato "AAAA-MM-DD HH:MM:SS"
      SensorTipo tipo;        // tipo do sensor (enum)
      float dados;            // valor medido pelo sensor
      Status status;          // status da leitura (enum): OK, ALERTA ou CRITICO
} TSensor;

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
    int inicio;
    int fim;
    TSensor *sensor_struct;
    TStatSensor *stats;
    TSensor *anomalias;
    char *caminho_arquivo;
    long file_offset;
} Args;

#endif
