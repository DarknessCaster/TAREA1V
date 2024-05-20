#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// MACROS
#define DELAY_PIN 0  // Pin utilizado para interrupciones
#define TX_PIN 2  // Pin de transmisión (salida)

// VARIABLES GLOBALES
volatile bool transmissionStarted = false;

// PROTOTIPOS
void cb(void);
void startTransmission();

int main() {
    // Inicializa WiringPi
    if (wiringPiSetup() == -1) {
        printf("Error al inicializar WiringPi\n");
        exit(1);
    }

    // Configura el pin como salida
    pinMode(TX_PIN, OUTPUT);

    // Configura la interrupción
    if (wiringPiISR(DELAY_PIN, INT_EDGE_RISING, &cb) < 0) {
        printf("No se puede iniciar la función de interrupción\n");
        return -1;
    }

    // Prueba de escritura en el pin
    digitalWrite(TX_PIN, HIGH);
    printf("Escribiendo HIGH en el pin %d\n", TX_PIN);
    delay(1000);  // Espera 1 segundo

    digitalWrite(TX_PIN, LOW);
    printf("Escribiendo LOW en el pin %d\n", TX_PIN);
    delay(1000);  // Espera 1 segundo

    // Inicia la transmisión
    startTransmission();
    while (transmissionStarted) {
        delay(100);
    }

    return 0;
}

void cb(void) {
    if (transmissionStarted) {
        digitalWrite(TX_PIN, HIGH);  // O cualquier lógica que desees probar
        delay(100);
        digitalWrite(TX_PIN, LOW);
        transmissionStarted = false;
    }
}

void startTransmission() {
    transmissionStarted = true;
}
