#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>
#include <math.h>

#define NUM_THREADS 3
#define NUM_NUMBERS 100
#define NUM_PODS 15

// Estrutura de um nó em uma lista encadeada
typedef struct Node {
    int data;           // Dados armazenados no nó
    struct Node* next;  // Próximo nó na lista
} Node;

// Estrutura de uma fila (queue) baseada em lista encadeada
typedef struct Queue {
    Node* front;        // Ponteiro para o início da fila
    Node* rear;         // Ponteiro para o final da fila
} Queue;

// Estrutura que representa um "Pod" com poder computacional
typedef struct Pod {
    int power;          // Poder computacional do Pod
    char name[20];      // Nome do Pod
} Pod;

// Estrutura que representa o estado de um trabalhador (thread)
typedef struct WorkerState {
    int power;                      // Poder computacional do trabalhador
    int allocatedPods[NUM_PODS];    // Lista de IDs de Pods alocados ao trabalhador
    int numAllocatedPods;           // Número de Pods alocados ao trabalhador
    int numProcessedNumbers;        // Número de números processados pelo trabalhador
} WorkerState;

// Estrutura compartilhada entre as threads
typedef struct SharedData {
    pthread_mutex_t mutex;         // Mutex para garantir exclusão mútua
    WorkerState* workers[NUM_THREADS];  // Lista de estados dos trabalhadores
    Pod pods[NUM_PODS];             // Lista de Pods com poder computacional
    char podNames[NUM_PODS][20];    // Nomes dos Pods
    pthread_barrier_t barrier;      // Barreira para sincronização das threads
} SharedData;

Queue numberQueue;  // Fila para armazenar números a serem processados

// Função para enfileirar um número na fila
void enqueue(Queue* queue, int data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;

    if (queue->rear == NULL) {
        queue->front = newNode;
        queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
}

// Função para desenfileirar um número da fila
int dequeue(Queue* queue) {
    if (queue->front == NULL) {
        return -1; // Fila vazia
    }

    int data = queue->front->data;
    Node* temp = queue->front;
    queue->front = queue->front->next;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    free(temp);
    return data;
}

// Função para fatorar um número
void factorize(int number, int workerId) {
    printf("Worker %d fatoração de %d: ", workerId, number);
    for (int i = 2; i <= number; i++) {
        while (number % i == 0) {
            printf("%d", i);
            number /= i;
            if (number > 1) {
                printf(" * ");
            }
        }
    }
    printf("\n");
}

// Função para imprimir informações sobre os Pods
void printPods(SharedData* sharedData) {
    printf("\n");
    printf("Pods e Seu Poder de Hardware:\n");
    for (int i = 0; i < NUM_PODS; i++) {
        printf("Pod %d: Poder = %d\n", i, sharedData->pods[i].power);
    }
}

// Função executada pela thread mestre (MASTER)
void* MASTER(void* arg) {
    SharedData* sharedData = (SharedData*)arg;
    srand(time(NULL));

    // Inicializa os Pods com métricas simuladas de hardware
    for (int i = 0; i < NUM_PODS; i++) {
        // Métricas simuladas enviadas pelos Pods indicando seu processador, RAM e latência de resposta.
        int cpu = rand() % 100 + 1;
        int ram_values[] = {128, 256, 512};
        int ram = ram_values[rand() % 3];
        int latency = rand() % 10 + 1;
        int power = (cpu * 100) + (ram * 50) + (latency * 10);
        sharedData->pods[i].power = power;
        sprintf(sharedData->podNames[i], "Pod %d", i);
    }

    printPods(sharedData); // Imprime informações dos Pods e seu hardware

    sharedData->workers[1]->power = 0;
    sharedData->workers[2]->power = 0;

    printf("\n");
    printf("Alocação de Pods para Trabalhadores:\n");
    for (int i = 0; i < NUM_PODS; i++) {
        pthread_mutex_lock(&sharedData->mutex);

        int minPowerIndex = -1;
        int minPower = INT_MAX;

        for (int j = 1; j < NUM_THREADS; j++) {
            if (sharedData->workers[j]->power < minPower) {
                minPower = sharedData->workers[j]->power;
                minPowerIndex = j;
            }
        }

        sharedData->workers[minPowerIndex]->power += sharedData->pods[i].power;
        sharedData->workers[minPowerIndex]->allocatedPods[sharedData->workers[minPowerIndex]->numAllocatedPods++] = i;

        // Imprime a mensagem de alocação
        printf("Pod %d foi alocado para o Trabalhador %d\n", i, minPowerIndex);

        pthread_mutex_unlock(&sharedData->mutex);
    }

    // Gera números aleatórios para processamento
    for (int i = 0; i < NUM_NUMBERS; i++) {
        int randomNumber = rand() % 1000 + 1;
        enqueue(&numberQueue, randomNumber);
    }

    printf("\n");
    printf("Poder Alocado:\n");
    // Imprime as linhas de "Poder" para o Trabalhador 1 e Trabalhador 2
    printf("Trabalhador 1: %d\n", sharedData->workers[1]->power);
    printf("Trabalhador 2: %d\n", sharedData->workers[2]->power);
    printf("\n");

    // Exibe a linha "Números para Processar"
    printf("Números para Processar:\n");
    printf("(");
    Node* currentNode = numberQueue.front;
    while (currentNode != NULL) {
        printf("%d", currentNode->data);
        if (currentNode->next != NULL) {
            printf(", ");
        }
        currentNode = currentNode->next;
    }
    printf(")\n");
    printf("\n");

    // Aguarda todas as threads terminarem antes de prosseguir
    pthread_barrier_wait(&sharedData->barrier);

    return NULL;
}

// Função executada pelo Trabalhador 1
void* WORKER1(void* arg) {
    SharedData* sharedData = (SharedData*)arg;
    int workerId = 1;

    // Aguarda todas as threads terminarem antes de prosseguir
    pthread_barrier_wait(&sharedData->barrier);

    while (1) {
        pthread_mutex_lock(&sharedData->mutex);
        int number = dequeue(&numberQueue);
        pthread_mutex_unlock(&sharedData->mutex);

        if (number == -1) {
            break; // Não há mais números para processar
        }

        factorize(number, workerId); // Fatora o número e exibe o resultado com o ID do trabalhador
        sharedData->workers[workerId]->numProcessedNumbers++;
    }

    return NULL;
}

// Função executada pelo Trabalhador 2
void* WORKER2(void* arg) {
    SharedData* sharedData = (SharedData*)arg;
    int workerId = 2;

    // Aguarda todas as threads terminarem antes de prosseguir
    pthread_barrier_wait(&sharedData->barrier);

    while (1) {
        pthread_mutex_lock(&sharedData->mutex);
        int number = dequeue(&numberQueue);
        pthread_mutex_unlock(&sharedData->mutex);

        if (number == -1) {
            break; // Não há mais números para processar
        }

        factorize(number, workerId); // Fatora o número e exibe o resultado com o ID do trabalhador
        sharedData->workers[workerId]->numProcessedNumbers++;
    }

    return NULL;
}

int main() {
    numberQueue.front = NULL;
    numberQueue.rear = NULL;

    SharedData sharedData;
    pthread_mutex_init(&sharedData.mutex, NULL);

    WorkerState worker1State;
    WorkerState worker2State;

    sharedData.workers[1] = &worker1State;
    sharedData.workers[2] = &worker2State;

    // Inicializa a barreira com o número de threads
    pthread_barrier_init(&sharedData.barrier, NULL, NUM_THREADS);

    pthread_t threads[NUM_THREADS];
    pthread_create(&threads[0], NULL, MASTER, &sharedData);
    pthread_create(&threads[1], NULL, WORKER1, &sharedData);
    pthread_create(&threads[2], NULL, WORKER2, &sharedData);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\n");
    printf("Trabalhador 1 processou %d números.\n", worker1State.numProcessedNumbers);
    printf("Trabalhador 2 processou %d números.\n", worker2State.numProcessedNumbers);
    printf("\n");

    // Destroi a barreira e o mutex
    pthread_barrier_destroy(&sharedData.barrier);
    pthread_mutex_destroy(&sharedData.mutex);

    return 0;
}