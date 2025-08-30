#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

/*Proyecto de simulacion de colas de gasolinera, Realizado por Josmer Azocar y Josue Azocar 
 Asignatura: Tecnicas de programacion I */
 
// --- Definiciones para Compatibilidad (Windows y Linux/macOS) ---
#ifdef _WIN32
#include <windows.h>
void esperar_milisegundos(int ms) { Sleep(ms); }
void limpiar_pantalla() { system("cls"); }
void gotoxy(int x, int y) {
    HANDLE hcon = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD dwPos;
    dwPos.X = x;
    dwPos.Y = y;
    SetConsoleCursorPosition(hcon, dwPos);
}
#else
#include <unistd.h>
void esperar_milisegundos(int ms) { usleep(ms * 1000); }
void limpiar_pantalla() { system("clear"); }
void gotoxy(int x, int y) { printf("\033[%d;%dH", y + 1, x + 1); }
#endif

// --- Funcion de Aleatoriedad Segura para Hilos ---
// Se usa en lugar de rand() para garantizar numeros aleatorios unicos en cada hilo
int safe_rand(unsigned int *seedp) {
    *seedp = (*seedp * 1103515245 + 12345);
    return (unsigned int)(*seedp / 65536) % 32768;
}

// --- Constantes de la Simulacion ---
#define NUM_SURTIDORES 4
#define MAX_COCHES_COLA 5
#define MAX_COLA_ESPERA 10

// --- Estructura para los Argumentos de Hilos ---
// Se utiliza para pasar datos a los hilos de gasolina y al generador de vehiculos
typedef struct {
    int id;
    unsigned int seed;
} HiloArgs;

// --- Variables Globales ---
pthread_mutex_t mtx;
int cola1 = 0, cola2 = 0, cola3 = 0, cola4 = 0; // Colas de surtidores
int cola0 = 0; // Cola de espera general

// Variables para el tiempo
int seg = 0, durar = 0, segundos = 0;
int simulacion_terminada = 0;

// Variables para estadísticas
int atendidos[NUM_SURTIDORES] = {0};
long long total_rango_surtir[NUM_SURTIDORES] = {0};
double promedio_rango_surtir[NUM_SURTIDORES] = {0};
double total_promedio_rango_surtir = 0;

// Variables para los rangos de tiempo
int rango_minimo_llegada, rango_maximo_llegada;
int rango_minimo_surtir, rango_maximo_surtir;

// Archivo para guardar resultados
FILE *archivo;


// --- Funciones de Utilidad ---
int aleatorio(int minimo, int maximo, unsigned int *seed) {
    if (minimo > maximo) return minimo;
    return minimo + safe_rand(seed) % (maximo - minimo + 1);
}

void mostrar_pantalla_bienvenida() {
    limpiar_pantalla();
    gotoxy(27, 4); printf("=========================");
    gotoxy(27, 5); printf(" Simulador de Gasolinera ");
    gotoxy(27, 6); printf("=========================");
    gotoxy(10, 10); printf("Reglas de la Simulacion:");
    gotoxy(12, 12); printf("1. Hay 4 surtidores de gasolina operativos.");
    gotoxy(12, 13); printf("2. Cada surtidor tiene una cola individual con un maximo de %d vehiculos.", MAX_COCHES_COLA);
    gotoxy(12, 14); printf("3. Si todas las colas de surtidor estan llenas, los vehiculos que lleguen");
    gotoxy(14, 15); printf("   pasaran a una cola de espera general (maximo %d vehiculos).", MAX_COLA_ESPERA);
    gotoxy(12, 16); printf("4. Si la cola de espera general tambien esta llena, los vehiculos se iran.");
    gotoxy(12, 17); printf("5. Al finalizar la Simulacion se creara un archivo txt con las estadisticas.");
    gotoxy(28, 20); printf("Pulse Enter para continuar...");
    getchar();
}

void pantalla_de_carga() {
    limpiar_pantalla();
    printf("Iniciando simulacion...");
    esperar_milisegundos(1500);
    limpiar_pantalla();
}

// --- Hilos de la Simulacion ---

// Hilo que maneja el tiempo de la simulacion
void* cronometro(void* arg) {
    int duracionseg = *(int*)arg;
    while(seg <= duracionseg) {
        pthread_mutex_lock(&mtx);
        durar = seg / 60;
        segundos = seg % 60;
        pthread_mutex_unlock(&mtx);
        esperar_milisegundos(1000);
        seg++;
    }
    pthread_mutex_lock(&mtx);
    simulacion_terminada = 1;
    pthread_mutex_unlock(&mtx);
    return NULL;
}

// Hilo para mostrar el estado de las colas en pantalla
void* mostrar_colas(void* arg) {
    int colas[NUM_SURTIDORES];
    int c0, dur, segs, terminado;
    char coche_char = 219; // Caracter ASCII para representar un coche

    do {
        pthread_mutex_lock(&mtx);
        colas[0] = cola1; colas[1] = cola2; colas[2] = cola3; colas[3] = cola4;
        c0 = cola0; dur = durar; segs = segundos; terminado = simulacion_terminada;
        pthread_mutex_unlock(&mtx);

        limpiar_pantalla();
        gotoxy(30, 2); printf("... SIMULANDO ...");
        gotoxy(60, 2); printf("Tiempo: %02d:%02d", dur, segs);

        // Dibuja los 4 surtidores
        for (int i = 0; i < NUM_SURTIDORES; i++) {
            int x_pos = (i % 2 == 0) ? 5 : 45;
            int y_pos = (i < 2) ? 6 : 10;
            
            gotoxy(x_pos, y_pos);
            printf("Surtidor %d: [", i + 1);
            for (int j = 0; j < MAX_COCHES_COLA; j++) {
                if (j < colas[i]) printf("%c ", coche_char);
                else printf("  ");
            }
            printf("] (%d/%d)", colas[i], MAX_COCHES_COLA);
        }

        // Dibuja la cola de espera general
        gotoxy(15, 16);
        printf("Cola de Espera General: [");
        for (int i = 0; i < MAX_COLA_ESPERA; i++) {
            if (i < c0) printf("%c ", coche_char);
            else printf("  ");
        }
        printf("] (%d/%d)", c0, MAX_COLA_ESPERA);

        fflush(stdout);
        esperar_milisegundos(500);

    } while (!terminado);
    return NULL;
}

// Hilo que genera nuevos vehiculos que llegan a la gasolinera
void* generador_vehiculos(void* arg) {
    HiloArgs *args = (HiloArgs*)arg;
    unsigned int *seed = &args->seed;
    int terminado;

    do {
        int tiempo_llegada = aleatorio(rango_minimo_llegada, rango_maximo_llegada, seed);
        esperar_milisegundos(tiempo_llegada);

        pthread_mutex_lock(&mtx);
        terminado = simulacion_terminada;
        if (!terminado) {
            int colas_actuales[] = {cola1, cola2, cola3, cola4};
            int min_cola_idx = -1;
            int min_cola_val = MAX_COCHES_COLA;

            for (int i = 0; i < NUM_SURTIDORES; i++) {
                if (colas_actuales[i] < min_cola_val) {
                    min_cola_val = colas_actuales[i];
                    min_cola_idx = i;
                }
            }
            
            if (min_cola_idx != -1 && min_cola_val < MAX_COCHES_COLA) {
                if (min_cola_idx == 0) cola1++;
                else if (min_cola_idx == 1) cola2++;
                else if (min_cola_idx == 2) cola3++;
                else if (min_cola_idx == 3) cola4++;
            } else if (cola0 < MAX_COLA_ESPERA) {
                cola0++;
            }
        }
        pthread_mutex_unlock(&mtx);

    } while (!terminado);
    return NULL;
}

// Hilo para cada surtidor, que atiende a los vehiculos
void* gasolina(void* arg) {
    HiloArgs *args = (HiloArgs*)arg;
    int id_surtidor = args->id;
    unsigned int *seed = &args->seed;
    int terminado;

    do {
        int hay_coche = 0;
        pthread_mutex_lock(&mtx);
        terminado = simulacion_terminada;
        if (id_surtidor == 0 && cola1 > 0) hay_coche = 1;
        if (id_surtidor == 1 && cola2 > 0) hay_coche = 1;
        if (id_surtidor == 2 && cola3 > 0) hay_coche = 1;
        if (id_surtidor == 3 && cola4 > 0) hay_coche = 1;
        pthread_mutex_unlock(&mtx);

        if (hay_coche && !terminado) {
            int tiempo_surtir = aleatorio(rango_minimo_surtir, rango_maximo_surtir, seed);
            esperar_milisegundos(tiempo_surtir);

            pthread_mutex_lock(&mtx);
            if (!simulacion_terminada) {
                 if (id_surtidor == 0 && cola1 > 0) {
                    cola1--; atendidos[0]++; total_rango_surtir[0] += tiempo_surtir;
                } else if (id_surtidor == 1 && cola2 > 0) {
                    cola2--; atendidos[1]++; total_rango_surtir[1] += tiempo_surtir;
                } else if (id_surtidor == 2 && cola3 > 0) {
                    cola3--; atendidos[2]++; total_rango_surtir[2] += tiempo_surtir;
                } else if (id_surtidor == 3 && cola4 > 0) {
                    cola4--; atendidos[3]++; total_rango_surtir[3] += tiempo_surtir;
                }
            }
            pthread_mutex_unlock(&mtx);
        } else {
            esperar_milisegundos(100);
        }
        
        pthread_mutex_lock(&mtx);
        terminado = simulacion_terminada;
        pthread_mutex_unlock(&mtx);

    } while (!terminado);
    return NULL;
}

// Hilo que mueve vehiculos de la cola de espera a los surtidores
void* cola_espera(void* arg) {
    int terminado;
    do {
        pthread_mutex_lock(&mtx);
        terminado = simulacion_terminada;
        if (!terminado && cola0 > 0) {
            if (cola1 < MAX_COCHES_COLA) { cola0--; cola1++; }
            else if (cola2 < MAX_COCHES_COLA) { cola0--; cola2++; }
            else if (cola3 < MAX_COCHES_COLA) { cola0--; cola3++; }
            else if (cola4 < MAX_COCHES_COLA) { cola0--; cola4++; }
        }
        pthread_mutex_unlock(&mtx);
        esperar_milisegundos(500);
    } while (!terminado);
    return NULL;
}

// Hilo que espera el final de la simulacion para mostrar y guardar los resultados
void* continuacion(void* arg) {
    while(1) {
        int terminado;
        pthread_mutex_lock(&mtx);
        terminado = simulacion_terminada;
        pthread_mutex_unlock(&mtx);
        if (terminado) break;
        esperar_milisegundos(1000);
    }
    
    esperar_milisegundos(1000); 
    limpiar_pantalla();
    printf("--- Resultados Finales de la Simulacion ---\n\n");
    printf("Tiempo de la Simulacion: %02d:%02d \n", durar, segundos);

    int total_atendidos_final = 0;
    for (int i = 0; i < NUM_SURTIDORES; i++) {
        total_atendidos_final += atendidos[i];
    }
    printf("Numero total de atendidos: %d \n", total_atendidos_final);
    printf("Numero de vehiculos en cola de espera: %d \n\n", cola0);

    double suma_promedios = 0;
    int surtidores_activos = 0;
    for (int i = 0; i < NUM_SURTIDORES; i++) {
        if (atendidos[i] > 0) {
            promedio_rango_surtir[i] = (double)(total_rango_surtir[i] / 1000.0) / atendidos[i];
            suma_promedios += promedio_rango_surtir[i];
            surtidores_activos++;
        } else {
            promedio_rango_surtir[i] = 0;
        }
        printf("Tiempo promedio surtidor %d: %.2f seg (%d atendidos)\n", i + 1, promedio_rango_surtir[i], atendidos[i]);
    }
    
    if (surtidores_activos > 0) {
        total_promedio_rango_surtir = suma_promedios / surtidores_activos;
    }
    printf("Tiempo promedio general: %.2f seg\n", total_promedio_rango_surtir);

    archivo = fopen("Datos_de_Simulacion.txt", "w");
    if (archivo != NULL) {
        fprintf(archivo, "Tiempo de la Simulacion: %02d:%02d\n", durar, segundos);
        fprintf(archivo, "Numero total de atendidos: %d\n", total_atendidos_final);
        fprintf(archivo, "Numero de vehiculos en cola de espera: %d\n\n", cola0);
        for (int i = 0; i < NUM_SURTIDORES; i++) {
            fprintf(archivo, "Tiempo promedio surtidor %d: %.2f seg (%d atendidos)\n", i + 1, promedio_rango_surtir[i], atendidos[i]);
        }
        fprintf(archivo, "Tiempo promedio general: %.2f seg\n", total_promedio_rango_surtir);
        fclose(archivo);
        printf("\nResultados guardados en 'Datos_de_Simulacion.txt'\n");
    } else {
        printf("\nError al guardar archivo.\n");
    }
    return NULL;
}


// --- Funcion Principal ---
int main() {
    int duracion, rango_min_llegada_seg, rango_max_llegada_seg, rango_min_surtir_seg, rango_max_surtir_seg;
    
    //BIENVENIDA AL PROGRAMA
    mostrar_pantalla_bienvenida();
    
    //CANTIDAD DE MINUTOS QUE VA A DURAR LA SIMULACION CON VALIDACIONES 1-90m
    do { 
        limpiar_pantalla();
        printf("Inserte la duracion de la simulacion (1-90 minutos): "); 
        scanf("%d", &duracion); 
        if (duracion < 1 || duracion > 90) {
            printf("\nError: La duracion debe estar entre 1 y 90 minutos.\n");
            esperar_milisegundos(2000);
        }
    } while (duracion < 1 || duracion > 90);

    int duracionseg = duracion * 60;
    int limite_segundos = duracionseg / 3;

    //TIEMPO QUE VA A TARDAR EN LLEGAR UN VEHICULO CON VALIDACIONES
    do { 
        limpiar_pantalla();
        printf("Duracion: %d minutos\n\n", duracion);
        printf("Rango de tiempo de LLEGADA de vehiculos (segundos).\n");
        printf("El valor debe estar entre 1 y %d segundos.\n\n", limite_segundos);
        printf("Minimo: "); 
        scanf("%d", &rango_min_llegada_seg);
        if (rango_min_llegada_seg < 1 || rango_min_llegada_seg > limite_segundos) {
            printf("\nError: Valor fuera del rango permitido. Intente de nuevo.\n");
            esperar_milisegundos(2000);
        }
    } while (rango_min_llegada_seg < 1 || rango_min_llegada_seg > limite_segundos);
    
    do { 
        limpiar_pantalla();
        printf("Duracion: %d minutos\n\n", duracion);
        printf("Rango de tiempo de LLEGADA de vehiculos (segundos).\n");
        printf("El valor debe estar entre 1 y %d segundos.\n\n", limite_segundos);
        printf("Minimo de llegada: %d\n", rango_min_llegada_seg);
        printf("Maximo: "); 
        scanf("%d", &rango_max_llegada_seg);
        if (rango_max_llegada_seg < rango_min_llegada_seg || rango_max_llegada_seg > limite_segundos) {
            printf("\nError: El maximo debe ser >= al minimo y no exceder %d.\n", limite_segundos);
            esperar_milisegundos(2000);
        }
    } while (rango_max_llegada_seg < rango_min_llegada_seg || rango_max_llegada_seg > limite_segundos);

	//TIEMPO QUE VA A TARDAR EN SURTIR UN VEHICULO CON VALIDACIONES	
    do { 
        limpiar_pantalla();
        printf("Duracion: %d minutos\n", duracion);
        printf("T. Llegada: [%d-%d] seg\n\n", rango_min_llegada_seg, rango_max_llegada_seg);
        printf("Rango de tiempo para SURTIR (segundos).\n");
        printf("El valor debe estar entre 1 y %d segundos.\n\n", limite_segundos);
        printf("Minimo: "); 
        scanf("%d", &rango_min_surtir_seg);
        if (rango_min_surtir_seg < 1 || rango_min_surtir_seg > limite_segundos) {
            printf("\nError: Valor fuera del rango permitido. Intente de nuevo.\n");
            esperar_milisegundos(2000);
        }
    } while (rango_min_surtir_seg < 1 || rango_min_surtir_seg > limite_segundos);

    do { 
        limpiar_pantalla();
        printf("Duracion: %d minutos\n", duracion);
        printf("T. Llegada: [%d-%d] seg\n", rango_min_llegada_seg, rango_max_llegada_seg);
        printf("T. Surtir Minimo: %d seg\n\n", rango_min_surtir_seg);
        printf("Rango de tiempo para SURTIR (segundos).\n");
        printf("El valor debe estar entre 1 y %d segundos.\n\n", limite_segundos);
        printf("Maximo: "); 
        scanf("%d", &rango_max_surtir_seg);
        if (rango_max_surtir_seg < rango_min_surtir_seg || rango_max_surtir_seg > limite_segundos) {
            printf("\nError: El maximo debe ser >= al minimo y no exceder %d.\n", limite_segundos);
            esperar_milisegundos(2000);
        }
    } while (rango_max_surtir_seg < rango_min_surtir_seg || rango_max_surtir_seg > limite_segundos);

    pantalla_de_carga();
    
    rango_minimo_llegada = rango_min_llegada_seg * 1000;
    rango_maximo_llegada = rango_max_llegada_seg * 1000;
    rango_minimo_surtir = rango_min_surtir_seg * 1000;
    rango_maximo_surtir = rango_max_surtir_seg * 1000;
    
    pthread_mutex_init(&mtx, NULL); // Inicializa el mutex	
    
    pthread_t hilo_crono, hilo_display, hilo_generador, hilo_espera, hilo_resultados;
    pthread_t hilos_gasolina[NUM_SURTIDORES];
    HiloArgs args_gasolina[NUM_SURTIDORES];
    HiloArgs args_generador;

    // Creacion de hilos
    pthread_create(&hilo_crono, NULL, cronometro, &duracionseg);
    pthread_create(&hilo_display, NULL, mostrar_colas, NULL);
    pthread_create(&hilo_espera, NULL, cola_espera, NULL);
    pthread_create(&hilo_resultados, NULL, continuacion, NULL);
    
    args_generador.seed = time(NULL);
    pthread_create(&hilo_generador, NULL, generador_vehiculos, &args_generador);
    
    for (int i = 0; i < NUM_SURTIDORES; i++) {
        args_gasolina[i].id = i;
        args_gasolina[i].seed = time(NULL) + i + 1;
        pthread_create(&hilos_gasolina[i], NULL, gasolina, &args_gasolina[i]);
    }

    // Espera a que todos los hilos terminen
    pthread_join(hilo_crono, NULL);
    pthread_join(hilo_generador, NULL);
    for (int i = 0; i < NUM_SURTIDORES; i++) {
        pthread_join(hilos_gasolina[i], NULL);
    }
    pthread_join(hilo_espera, NULL);
    pthread_join(hilo_display, NULL);
    pthread_join(hilo_resultados, NULL);
    
    pthread_mutex_destroy(&mtx); // Destruye el mutex

    printf("\nPresione Enter para salir...");
    getchar(); getchar();

    return 0;
}
