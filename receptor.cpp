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
void crearArchivo(char cadena[]);
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
            error_FCS = desempaquetar(proto);
            crearArchivo((char *)proto.DATA);
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
/*  Nombre de la función: desempaquetar
 *  Tipo de función: bool
 *  Parámetros: Protocolo &proto
 *  Descripción de la función: Esta función desempaqueta el FRAME (data empaquetado) y extrae 
 *                             el comando, la longitud de datos, los datos y el FCS.
 *                             Verifica un error del mensaje mediante la comparación del FCS calculado.
 */
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

/*  Nombre de la función: cb_receptor
 *  Tipo de función: void
 *  Parámetros: Ninguno.
 *  Descripción de la función: Función callback del receptor, encargada de leer el estado del pin RX 
 *                             y procesar los bits recibidos. Inicia la transmisión al detectar un bit de inicio.
 */
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

/*  Nombre de la función: procesarBit
 *  Tipo de función: void
 *  Parámetros: bool level.
 *  Descripción de la función: Esta función procesa cada bit recibido durante la transmisión,
 *                             desempaquetando los bytes de datos y verificando la paridad.
 */
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

/*  Nombre de la función: guardarMensaje
 *  Tipo de función: void
 *  Parámetros: char cadena[].
 *  Descripción de la función: Esta función guarda un mensaje en un archivo llamado "mensajes.txt".
 *                             Abre el archivo en modo de adición, escribe el mensaje y lo cierra.
 *                             Si el archivo no existe lo crea.
 */
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

/*  Nombre de la función: crearArchivo
 *  Tipo de función: void
 *  Parámetros: char cadena[].
 *  Descripción de la función: Esta función crea un archivo de texto en la carpeta actual con el nombre proporcionado.
 *                             Si el archivo ya existe, muestra un mensaje de error. Si no existe, lo crea.
 */
void crearArchivo(char cadena[]){ // Crea un archivo de texto en la carpeta actual a partir de un nombre entregado, si el archivo ya existe devuelve mensaje de error.
    FILE *lectura = fopen(strcat((cadena), ".txt"), "r"); // Se concatena el ".txt" al final del nombre entregado e intenta abrir el archivo. 
    if(lectura != NULL){ // Si el intento de apertura del archivo no devuelve NULL significa que si existe.
        printf("\n El archivo %s ya existe en nuestros registros.", cadena);
    }else{
        fopen(cadena, "a+");
        printf("\n El archivo %s fue creado con exito.", cadena);
    }
}

/*  Nombre de la función: mostrarArchivo
 *  Tipo de función: void
 *  Parámetros: char cadena[].
 *  Descripción de la función: Esta función muestra el contenido de un archivo cuyo nombre es ingresado por el usuario.
 *                             Si el archivo no existe, muestra un mensaje de error. Si el archivo existe pero está vacío, indica que está vacío.
 */
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
