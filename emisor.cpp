#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocolo.h"

// VARIABLES GLOBALES
Protocolo proto;
int msg_enviados = 0;
int msg_prueba = 0;
char nombre_arch[20];
bool transmissionStarted = false; // Indica si la transmision empieza o no.
volatile int nbits = 0; // contador de bits enviados
volatile int nbytes = 0; // contador de bytes enviados
int nones = 0; // contador de 1s en el byte enviado

// PROTOTIPOS
int empaquetar(Protocolo &proto);
int fcs(BYTE * arr, int tam);
void cb_emisor(void);
void startTransmission();

int main(){
    //INICIA WIRINGPI
    if(wiringPiSetup() == -1)
    return -1;

    // CONFIGURA INTERRUPCION PIN CLOCK
    if (wiringPiISR(DELAY_PIN_E, INT_EDGE_RISING, &cb_emisor) < 0) {
        printf("No se puede iniciar la función de interrupción\n");
        return -1;
    }

    //CONFIGURA PINES DE ENTRADA SALIDA
    pinMode(TX_PIN, OUTPUT);
    
    //INICIA MENU
    do {
        printf("\n================== MENU ==================\n");
        printf("1. Enviar mensaje de prueba\n");
        printf("2. Enviar mensaje de texto\n");
        printf("3. Buscar archivo txt\n");
        printf("4. Ver cantidad de mensajes enviados\n");
        printf("5. Listar archivos de texto del receptor\n");
        printf("6. Crear archivo y registrar un mensaje\n"); //  *** PUNTAJES EXTRAS: 2.
        printf("7. Cerrar programa receptor\n");
        printf("==============================================\n");
        printf("Ingrese la opcion deseada: ");
        scanf("%c", &proto.CMD);
        printf("%x", proto.CMD);
        getchar(); 
        switch (proto.CMD) {
            case '1':
                printf("\nIngrese el mensaje a enviar (15 caracteres maximo):");
                scanf(" %[^\n]s", proto.DATA);
                empaquetar(proto);
                startTransmission();
                while(transmissionStarted)
                    delay(2000);
                if(msg_prueba == 10){
                    msg_prueba = 0;
                    break;
                }   
                break;
            case '2':
                printf("\nIngrese el mensaje de texto a enviar (15 caracteres maximo):");
                scanf(" %[^\n]s", proto.DATA);
                empaquetar(proto);
                startTransmission();
                while(transmissionStarted)
                    delay(2000);
                break;
            case '3':
                printf("\nIngrese el nombre del archivo de texto a mostrar:");
                scanf(" %[^\n]s", proto.DATA);
                empaquetar(proto);
                startTransmission();
                while(transmissionStarted)
                    delay(2000);
                break;
            case '4':
                printf("Mensajes enviados: %d", msg_enviados); // *** Esto tambien debe ir en el receptor.            
                break;
            case '5':
                empaquetar(proto);
                // Ejecutar emisor
                // *** En receptor ejecutar funcion (funcion pendiente);
                msg_enviados++;                     
                break;
            case '6':
                printf("\nIngrese el mensaje a enviar (15 caracteres maximo):");
                scanf(" %[^\n]s", proto.DATA);
                empaquetar(proto);
                // Ejecutar emisor
                msg_enviados++;
                break;
            case '7':
                printf("\nComunicacion finalizada");
                empaquetar(proto);
                // Ejecutar emisor
                break;
            default:
                printf("Opción inválida. Por favor, ingrese una opción válida.\n");
                break;
        }
    } while (proto.CMD != 7); // se cierra el emisor si proto.CMD = 7.
    return 0;
}

// IMPLEMENTACIONES
/*  Nombre de la función: empaquetar
 *  Tipo de función: int
 *  Parámetros: Protocolo &proto
 *  Descripción de la función: Esta función empaqueta los datos de la estructura Protocolo.
 *                             Calcula la longitud de los datos y prepara el paquete (FRAMES)
 *                             copiando los datos en el campo FRAMES y calcula el campo de verificación de trama (FCS).
 *                             Luego retorna la longitud total del paquete.
 */
int empaquetar(Protocolo &proto){
    // if (proto.LNG + 2 > LARGO_DATA + 2){
    //     return -1;
    // } 
    proto.LNG = strlen((const char*)proto.DATA);
    proto.FRAMES[0] = (proto.CMD & 0x0F) | ((proto.LNG & 0x0F) << 4);
    memcpy(&proto.FRAMES[1], proto.DATA, proto.LNG);
    proto.FCS = fcs(proto.FRAMES, proto.LNG+1);
    proto.FRAMES[proto.LNG+1] = (proto.FCS) & 0xFF;
    return proto.LNG +2;
}

/*  Nombre de la función: startTransmission
 *  Tipo de función: void
 *  Parámetros: Ninguno
 *  Descripción de la función: Esta función inicia el proceso de transmisión marcando el estado de transmisión como activo. 
 *                            Es utilizada para comenzar la transmisión de datos desde el emisor.
 */
void startTransmission(){
  transmissionStarted = true;
}

/*  Nombre de la función: fcs
 *  Tipo de función: int
 *  Parámetros: BYTE * arr, int tam.
 *  Descripción de la función: Calcula el numero de bits activos en el FRAME (data empaquetado) 
 *                             (FCS, por sus siglas en inglés) para un array de bytes dado.
 *                             Itera sobre cada byte del FRAME y cuenta la cantidad de bits activos en cada byte.
 *                             Luego retorna el total de bits activos en la trama.
 */
int fcs(BYTE * arr, int tam){
    int sum_bits = 0;
    for(int i=0; i<tam; i++){
        for(int j=0; j<8; j++){
            sum_bits += (arr[i] >> j) & 0x01;
        } 
    }
    return sum_bits;
}

/*  Nombre de la función: cb_emisor
 *  Tipo de función: void
 *  Parámetros: Ninguno.
 *  Descripción de la función: Funcion callback del emisor, lo que hace es transmitir bit a bit
 *                              el FRAME (data empaquetado) a traves de transmision asincrona.
 *                              Esto es con bit de inicio, data y luego bit de paridad.
 */
void cb_emisor(void) {
    if (transmissionStarted) { // Si transmision se inicia...
        // Escribe en el pin TX
        if (nbits == 0) {
            digitalWrite(TX_PIN, 0); // Se envia bit de inicio
        } 
        else if (nbits < 9) {
            // envia bit a bit el dato hasta enviar el byte
            digitalWrite(TX_PIN, (proto.FRAMES[nbytes] >> (nbits-1)) & 0x01); 
            // printf("%d", (p.FRAMES[nbytes] >> (nbits-1)) & 0x01);
        } 
        else if (nbits == 9) {
            // printf("\n");
            // Guardar bits activos (cantidad de 1s) en la variable nones
            nones = (proto.FRAMES[nbytes] & 0x01) + ((proto.FRAMES[nbytes] & 0x02) >> 1) + ((proto.FRAMES[nbytes] & 0x04) >> 2) + 
                    ((proto.FRAMES[nbytes] & 0x08) >> 3) + ((proto.FRAMES[nbytes] & 0x10) >> 4) + ((proto.FRAMES[nbytes] & 0x20) >> 5) + 
                    ((proto.FRAMES[nbytes] & 0x40) >> 6) + ((proto.FRAMES[nbytes] & 0x80) >> 7);

            digitalWrite(TX_PIN, nones % 2 == 0); // Se envia el Bit de paridad PAR 
        } 
        else {
            digitalWrite(TX_PIN, 1); // Canal libre durante 2 clocks
        }
        
        // Actualiza contador de bits
        nbits++;

        // Actualiza contador de bytes
        if (nbits == 11) {
            nbits = 0;
            nbytes++;

            // Finaliza la comunicación
            if (nbytes == proto.LNG+1) {
                transmissionStarted = false;
                nbytes = 0;
                if(proto.CMD == '1'){
                    msg_prueba++;
                }
                msg_enviados++; // contador mensajes enviados
            }
        }
    } 
    else { // Si no ha iniciado la transmision...
        // Canal en reposo
        digitalWrite(TX_PIN, 1);
    }
}
