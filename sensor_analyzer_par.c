#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include "sensor_types.h"

#define MAX_ANOMALIAS 100000
#define MAX_SENSORES 20
#define NUM_THREADS 16

int total_alertas = 0;
float consumo_energia = 0.0;
int qtd_anomalias = 0;
pthread_mutex_t mutex_anomalias;

FILE *abrir_arquivo(const char *caminho, const char *modo)
{
    FILE *arquivo = fopen(caminho, modo);
    if (arquivo == NULL)
    {
        printf("Erro ao abrir %s\n", caminho);
        exit(EXIT_FAILURE);
    }
    return arquivo;
}

int contar_linhas(FILE *arquivo)
{
    int linhas = 0;
    int c;
    while ((c = fgetc(arquivo)) != EOF)
        if (c == '\n')
            linhas++;
    rewind(arquivo);
    return linhas;
}

SensorTipo parse_tipo(char *tipo_str)
{
    if (strcmp(tipo_str, "temperatura") == 0)
        return TEMPERATURA;
    else if (strcmp(tipo_str, "umidade") == 0)
        return UMIDADE;
    else if (strcmp(tipo_str, "energia") == 0)
        return ENERGIA;
    else if (strcmp(tipo_str, "corrente") == 0)
        return CORRENTE;
    else if (strcmp(tipo_str, "pressao") == 0)
        return PRESSAO;
    else
        return -1;
}

Status parse_status(char *status_str)
{
    if (strcmp(status_str, "OK") == 0)
        return OK;
    else if (strcmp(status_str, "ALERTA") == 0)
        return ALERTA;
    else if (strcmp(status_str, "CRITICO") == 0)
        return CRITICO;
    else
        return -1;
}

void parse(TSensor *struct_sensor, FILE *arquivo, int qtd_linhas, TStatSensor *stats)
{
    char buffer[256];

    for (int i = 0; i < qtd_linhas; i++)
    {
        if (fgets(buffer, sizeof(buffer), arquivo) == NULL)
        {
            fprintf(stderr, "Erro ao ler linha %d: %s\n", i, strerror(errno));
            break;
        }

        int sensor_id;
        char datetime_str[20];
        char tipo_str[12];
        float valor;
        char status_str[8];

        sscanf(buffer, "sensor_%d %19[0-9: -] %s %f status %s",
               &sensor_id, datetime_str, tipo_str, &valor, status_str);

        struct_sensor[i].sensor_id = sensor_id;
        strncpy(struct_sensor[i].datetime_str, datetime_str, 20);
        struct_sensor[i].tipo = parse_tipo(tipo_str);
        struct_sensor[i].dados = valor;
        struct_sensor[i].status = parse_status(status_str);

        int idx = sensor_id - 1;
        if (struct_sensor[i].tipo == TEMPERATURA)
        {
            stats[idx].soma += valor;
            stats[idx].soma_quadrados += valor * valor;
            stats[idx].contagem++;
        }

        if (struct_sensor[i].tipo == ENERGIA)
            consumo_energia += valor;

        if (struct_sensor[i].status == ALERTA || struct_sensor[i].status == CRITICO)
            total_alertas++;
    }
}

void calcular_estatisticas(TStatSensor *stats)
{
    for (int i = 0; i < MAX_SENSORES; i++)
    {
        if (stats[i].contagem > 0)
        {
            stats[i].media = stats[i].soma / stats[i].contagem;
            stats[i].desvio = sqrt(stats[i].soma_quadrados / stats[i].contagem - stats[i].media * stats[i].media);
        }
    }
}

void calcular_anomalias(TSensor *struct_sensor, TStatSensor *stats, TSensor *anomalias, int inicio, int fim)
{
    TSensor *anomalias_local = malloc((fim - inicio) * sizeof(TSensor));
    if (anomalias_local == NULL)
    {
        fprintf(stderr, "Erro ao alocar buffer local de anomalias\n");
        return;
    }
    int qtd_local = 0;

    for (int i = inicio; i < fim; i++)
    {
        if (struct_sensor[i].tipo == TEMPERATURA)
        {
            int idx = struct_sensor[i].sensor_id - 1;
            if (struct_sensor[i].dados >= stats[idx].media + stats[idx].desvio * 3 ||
                struct_sensor[i].dados <= stats[idx].media - stats[idx].desvio * 3)
            {
                anomalias_local[qtd_local++] = struct_sensor[i];
            }
        }
    }

    if (qtd_local > 0)
    {
        pthread_mutex_lock(&mutex_anomalias);
        for (int i = 0; i < qtd_local; i++)
        {
            anomalias[qtd_anomalias + i] = anomalias_local[i];
        }
        qtd_anomalias += qtd_local;
        pthread_mutex_unlock(&mutex_anomalias);
    }

    free(anomalias_local);
}

void imprimir_estatisticas(TStatSensor *stats, TSensor *anomalias, int qtd_anomalias, struct timespec start)
{
    struct timespec end;
    float maior_desvio = stats[0].desvio;
    int idx_sensor = 0;
    int qtd_sensores_impressos = 0;
    int resposta = 0;

    printf("---------- Estatísticas gerais: ----------\n");
    printf("\nO consumo de energia é: %.2f\n", consumo_energia);
    printf("total de alertas: %d\n\n", total_alertas);

    printf("---------- Estatísticas dos sensores de temperatura: ----------\n");
    for (int i = 0; i < MAX_SENSORES; i++)
    {
        if (!isnan(stats[i].media))
        {
            if (idx_sensor == 0)
            {
                maior_desvio = stats[i].desvio;
                idx_sensor = i + 1;
            }
            if (stats[i].desvio > maior_desvio)
            {
                maior_desvio = stats[i].desvio;
                idx_sensor = i + 1;
            }
            printf("Sensor %d:\n  Média: %.2f\n  Desvio padrão: %.2f\n\n", i, stats[i].media, stats[i].desvio);
            qtd_sensores_impressos++;
        }
        if (qtd_sensores_impressos > 10)
            break;
    }
    printf("\nO maior desvio padrão pertence ao Sensor %d, sendo: %.2f\n", idx_sensor, maior_desvio);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_spent = (end.tv_sec - start.tv_sec) +
                        (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("\nTempo de execução: %f segundos\n", time_spent);

    do
    {
        printf("\nDeseja visualizar as anomalias detectadas? Digite:\n- 1 ver as 10 primeiras\n- 2 para ver todas\n- 3 para não ver.\n");
        scanf("%d", &resposta);
    } while (resposta != 1 && resposta != 2 && resposta != 3);

    if (resposta == 2)
    {
        for (int j = 0; j < qtd_anomalias; j++)
        {
            printf("Anomalia %d | sensor %d: temperatura: %.2f no dia: %s\n", (j + 1), anomalias[j].sensor_id, anomalias[j].dados, anomalias[j].datetime_str);
        }
    }
    else if (resposta == 1)
    {
        for (int j = 0; j < 10 && j < qtd_anomalias; j++)
        {
            printf("Anomalia %d | sensor %d: temperatura: %.2f no dia: %s\n", (j + 1), anomalias[j].sensor_id, anomalias[j].dados, anomalias[j].datetime_str);
        }
    }
}

void *thread_anomalias_func(void *arg)
{
    Args *args = (Args *)arg;
    calcular_anomalias(args->sensor_struct, args->stats, args->anomalias, args->inicio, args->fim);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Uso: %s <arquivo.log>\n", argv[0]);
        return 1;
    }

    pthread_mutex_init(&mutex_anomalias, NULL);

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    FILE *arquivo = abrir_arquivo(argv[1], "r");
    int qtd_linhas = contar_linhas(arquivo);

    TSensor *dados = malloc(qtd_linhas * sizeof(TSensor));
    if (dados == NULL)
    {
        perror("Erro de alocacao\n");
        return 1;
    }

    TStatSensor stats[MAX_SENSORES] = {0};
    TSensor *anomalias = calloc(MAX_ANOMALIAS, sizeof(TSensor));
    if (anomalias == NULL)
    {
        perror("Erro ao alocar anomalias");
        return 1;
    }

    //Sequencial
    parse(dados, arquivo, qtd_linhas, stats);
    calcular_estatisticas(stats);

    //Paralelo
    int chunk_size = qtd_linhas / NUM_THREADS;
    pthread_t threads[NUM_THREADS];
    Args *argumentos[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++)
    {
        argumentos[i] = malloc(sizeof(Args));
        if (argumentos[i] == NULL)
        {
            perror("Erro ao alocar memória");
            return 1;
        }

        argumentos[i]->inicio = i * chunk_size;
        argumentos[i]->fim = (i == NUM_THREADS - 1) ? qtd_linhas : (i + 1) * chunk_size;
        argumentos[i]->anomalias = anomalias;
        argumentos[i]->sensor_struct = dados;
        argumentos[i]->stats = stats;

        if (pthread_create(&threads[i], NULL, thread_anomalias_func, argumentos[i]) != 0)
        {
            perror("Erro ao criar a thread");
            return 1;
        }
    }

    for (int j = 0; j < NUM_THREADS; j++)
    {
        pthread_join(threads[j], NULL);
        free(argumentos[j]);
    }

    imprimir_estatisticas(stats, anomalias, qtd_anomalias, start);

    pthread_mutex_destroy(&mutex_anomalias);
    fclose(arquivo);
    free(dados);
    free(anomalias);
    return 0;
}