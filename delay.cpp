#include <wiringPi.h>
#include <stdlib.h>

#define PWM_PIN 1 // Pin de salida de PWM, BCM 18
#define PWM_CLOCK_DIVISOR 192
#define PWM_RANGE 2000

// Configuración del PWM para generar una frecuencia de 500 Hz
// Frecuencia = 19.2 MHz / clock_divisor / range
// 500 Hz = 19.2 MHz / 192 / 200
// Ajusta los valores del divisor y del rango según la frecuencia deseada

int main(){
    if(wiringPiSetup() == -1)
        exit(1);

    // Configura el pin PWM
    pinMode(PWM_PIN, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS); // Modo Mark-Space
    pwmSetRange(PWM_RANGE);
    pwmSetClock(PWM_CLOCK_DIVISOR);
    pwmWrite(PWM_PIN, PWM_RANGE / 2); // 50% del ciclo de trabajo

    while(1) {
        delay(500); // Mantén el programa en funcionamiento
    }

    return 0;
}
