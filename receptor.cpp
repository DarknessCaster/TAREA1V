#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocolo.h"

// VARIABLES GLOBALES
volatile int nbytes = 0;
volatile int nbits = 0;
Protocolo proto;
bool parity = 0;
int nones = 0;
bool transmissionStarted = false;
bool parityError = 0;
volatile BYTE LNG = 17;
volatile int errores = 0;
bool error_FCS = true;

// PROTOTIPOS
bool desempaquetar(Protocolo&proto);
void procesarBit(bool level);
void cb_receptor(void);
void guardarMensaje(char cadena[]);
void mostrarArchivo(char cadena[]);

int main(){
    //INICIA WIRINGPI
    if(wiringPiSetup() == -1)
    exit(1);

    //CONFIGURA PINES DE ENTRADA SALIDA
    pinMode(RX_PIN, INPUT);

    //CONFIGURA INTERRUPCION PIN CLOCK (PUENTEADO A PIN PWM)
    if(wiringPiISR(DELAY_PIN_R, INT_EDGE_RISING, &cb_receptor) < 0){
        printf("Unable to start interrupt function\n");
    }
    printf("Delay\n");
    while(nbytes < proto.LNG) // NBYTES MENOR A LONGITUD DE DATA??a
        delay(300); 
    error_FCS = desempaquetar(proto);
    switch(proto.CMD){
        case '1':
            for(int i = 0; i<proto.LNG; i++){
                printf("Byte %d: %d\n", i, proto.DATA[i]);
            }
            break;
        case '2':
            guardarMensaje((char*)proto.DATA);
            break;
        case '3':
            mostrarArchivo((char*)proto.DATA);
            break;
        case '4':
            break;
        case '5':
            
            break;
        case '6':
            
            break;
        case '7':
            printf("Cerrando programa...");
            exit(1);
            break;
        default:
            break;
    }
    return 0;
}

// IMPLEMENTACIONES
bool desempaquetar(Protocolo&proto){
    // if (tam != proto.LNG+2){                            //filtro 1, corrrespondiente al largo total del mensaje
    //     return false;
    // }
    proto.CMD = proto.FRAMES[0] & 0x0F;
    proto.LNG = ((proto.FRAMES[0] >> 4) & 0x0F);
    if (proto.LNG > 0 && proto.LNG <= LARGO_DATA){      
        for(int i = 0; i < proto.LNG; i++){
            proto.DATA[i] = proto.FRAMES[i+1] & 0xFF;
        }
    } 
    proto.FCS = proto.FRAMES[proto.LNG+1];
    int fcs_recibido = fcs(proto.FRAMES, proto.LNG+1);
    if (fcs_recibido != proto.FCS){                     //filtro 2, correspondiente a la comparacion de los fcs 
        return false;
    } 
    return true; // Desempaquetado correctamente
}

void cb_receptor(void){
    bool level = digitalRead(RX_PIN);
    //  printf("%d",level);
    if (transmissionStarted){
        procesarBit(level);
    }
    else if(level == 0 && !transmissionStarted){
        transmissionStarted = true;
        nbits = 1;
    }
}

void procesarBit(bool level){
    if (nbits < 9) { // Si estamos recibiendo uno de los primeros 8 bits de datos
        proto.FRAMES[nbytes] |= level << (nbits - 1);
    } 
    else if (nbits == 9) { // Si estamos recibiendo el bit de paridad
        parity = level;
        // Calcular la cantidad de bits 1 en el byte recibido
        nones = (proto.FRAMES[nbytes] & 0x01) + ((proto.FRAMES[nbytes] & 0x02) >> 1) +
                ((proto.FRAMES[nbytes] & 0x04) >> 2) + ((proto.FRAMES[nbytes] & 0x08) >> 3) +
                ((proto.FRAMES[nbytes] & 0x10) >> 4) + ((proto.FRAMES[nbytes] & 0x20) >> 5) +
                ((proto.FRAMES[nbytes] & 0x40) >> 6) + ((proto.FRAMES[nbytes] & 0x80) >> 7);
        
        // Verificar si la paridad recibida coincide con la paridad calculada
        if (parity != (nones % 2 == 0)) {
            parityError = true;
            errores++; // Contador de errores de paridad por cada bit
        }
        
        // Incrementar el contador de bytes y finalizar la transmisión actual
        nbytes++;
        transmissionStarted = false;
    }
    
    // Incrementar el contador de bits
    nbits++;
}

void guardarMensaje(char cadena[]){ // Guarda 
    FILE *archivo;
    // printf("\n La cadena en la funcion es %s", cadena); // Para probar que la cadena este bien en este punto.
    int aux;
    archivo = fopen("mensajes.txt", "a+"); // Abre el archivo "mensajes.txt" en la carpeta actual, si no existe lo crea.
    aux = fgetc(archivo); // Lee el primer caracter del archivo.
        if(aux != EOF){ // Si el archivo esta vacio su primer caracter sera "EOF". Solo cuando se comienza a escribir en el archivo no es necesario un salto de linea.
            fseek(archivo, 0, SEEK_END); // Lleva al inicio del archivo, al primer caracter.
            fprintf(archivo, "\n");
        }
    
        for(int i=0; i<15 ; i++){ // Se hace asi por si hay espacios en la cadena de caracteres, en lugar de solo hacer un fprintf() del string completo.
            if(strcmp(&cadena[i], "\0") == 0){ // Finaliza la escritura de caracteres en el archivo cuando se llega al final del string.
                break;
            }
            fprintf(archivo, "%c", cadena[i]); // Escribe en el archivo la cadena caracter a caracter.
        }
        fclose(archivo);
}

void mostrarArchivo(char cadena[]){ // Muestra el contenido de un archivo cuyo nombre es ingresado por el usuario, si no existe devuelve mensaje de error.
    char aux[15]; 
    FILE *lectura = fopen(strcat((cadena), ".txt"), "r"); // Se concatena el ".txt" al final del nombre entregado e intenta abrir el archivo. 
    if(lectura == NULL){
        printf("\n El archivo %s no existe en nuestros registros.", cadena);
    }else if(fgetc(lectura) == EOF){
        printf("\n El archivo %s existe pero esta vacio.", cadena);
    }else{
        printf("\n Contenido de %s:\n", cadena);
        while(feof(lectura) == 0){ // Mientras que no se halla llegado al final del archivo referenciado por el puntero "lectura" imprime por pantalla cada mensaje en el archivo.
            fgets(aux, 15, lectura);
            printf("%s", aux);
        }
    }
    fclose(lectura);
}