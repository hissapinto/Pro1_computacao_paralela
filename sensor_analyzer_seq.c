#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "sensor_types.h"

#define MAX_ANOMALIAS 100000
#define MAX_SENSORES 20

int total_alertas = 0;
float consumo_energia = 0.0;

FILE *abrir_arquivo(const char *caminho, const char *modo) {
      FILE *arquivo = fopen(caminho, modo);
      if (arquivo == NULL)
      {
            printf("Erro ao abrir %s\n", caminho);
            exit(EXIT_FAILURE);
      }
      return arquivo;
}

int contar_linhas(FILE *arquivo) {
      int linhas = 0;
      char c;
      while ((c = fgetc(arquivo)) != EOF)
            if (c == '\n') linhas++;
      rewind(arquivo); // volta o cursor pro inicio do arquivo
      return linhas;
}

SensorTipo parse_tipo(char *tipo_str) {
      if (strcmp(tipo_str, "temperatura") == 0) return TEMPERATURA;
      else if (strcmp(tipo_str, "umidade") == 0) return UMIDADE;
      else if (strcmp(tipo_str, "energia") == 0) return ENERGIA;
      else if (strcmp(tipo_str, "corrente") == 0) return CORRENTE;
      else if (strcmp(tipo_str, "pressao") == 0) return PRESSAO;
      else return -1;
}

Status parse_status(char *status_str) {
      if (strcmp(status_str, "OK") == 0) return OK;
      else if (strcmp(status_str, "ALERTA") == 0) return ALERTA;
      else if (strcmp(status_str, "CRITICO") == 0) return CRITICO;
      else return -1;
}

//Revebe ponteiro para os dados, end arquivo, qtd linha e ponteiro p vetor de status
void parse(TSensor *struct_sensor, FILE *arquivo, int qtd_linhas, TStatSensor *stats) {
      char buffer[256];

      for (int i = 0; i < qtd_linhas; i++) {
            if (fgets(buffer, sizeof(buffer), arquivo) == NULL) {
                  printf(stderr, "Erro ao ler linha %d: %s\n", i, strerror(errno));
                  break;
            }

            int sensor_id; //id
            char datetime_str[20]; //data
            char tipo_str[12]; //tipo do sensor (enum)
            float valor; //valor medido
            char status_str[8]; //status (enum)

            // Ex leitura: sensor_001 2026-03-01 00:00:00 temperatura 21.1 status OK
            // o %19[0-9: -] é para ler a data e hora. 19 pq a data e hora tem 19 chars
            // e o [0-9: -] significa "aceita dígitos, dois pontos, espaço e hífen"
            sscanf(buffer, "sensor_%d %19[0-9: -] %s %f status %s",
                  &sensor_id, datetime_str, tipo_str, &valor, status_str);

            //Atribuições no vetor de dados
            struct_sensor[i].sensor_id = sensor_id;
            strncpy(struct_sensor[i].datetime_str, datetime_str, 20);
            struct_sensor[i].tipo = parse_tipo(tipo_str);
            struct_sensor[i].dados = valor;
            struct_sensor[i].status = parse_status(status_str);

            int idx = sensor_id - 1;
            if (struct_sensor[i].tipo == TEMPERATURA) {
                  stats[idx].soma += valor;
                  stats[idx].soma_quadrados += valor * valor;
                  stats[idx].contagem++;
            }

            if (struct_sensor[i].tipo == ENERGIA)
                  consumo_energia += valor; //var global

            if (struct_sensor[i].status == ALERTA || struct_sensor[i].status == CRITICO)
                  total_alertas++;
      }
}

void calcular_estatisticas(TStatSensor *stats) {
      for (int i = 0; i < MAX_SENSORES; i++)
      {
            stats[i].media = stats[i].soma / stats[i].contagem;
            stats[i].desvio = sqrt(stats[i].soma_quadrados / stats[i].contagem - stats[i].media * stats[i].media);
      }
}

//argc = qtd args | *argv[] = vetor de args | o nome do programa é o arg 0
int main(int argc, char *argv[]) {
      if (argc < 2) {
            printf("Uso: %s <arquivo.log>\n", argv[0]);
            return 1;
      }

      FILE *arquivo = abrir_arquivo(argv[1], "r");
      int qtd_linhas = contar_linhas(arquivo);

      //Uma alocação por linha do log/leitura
      TSensor *dados = malloc(qtd_linhas * sizeof(TSensor));
      if (dados == NULL) {
            perror("Erro de alocacao\n");
            return 1;
      }

      TStatSensor stats[MAX_SENSORES] = {0};

      parse(dados, arquivo, qtd_linhas, stats);
      calcular_estatisticas(stats);

      fclose(arquivo);
      free(arquivo);
      free(dados);
      return 0;
}
