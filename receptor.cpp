#include <wiringPi.h>
#include <stdio.h>
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

// PROTOTIPOS
bool desempaquetar(Protocolo&proto);
void procesarBit(bool level);
void cb_receptor(void);

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
    desempaquetar(proto);
    while(nbytes < proto.LNG) // NBYTES MENOR A LONGITUD DE DATA??a
        delay(300); 
    switch(proto.CMD){
        case 1:
            
            break;
        case 2:
            desempaquetar(proto, proto.LNG+2);
            guardarMensaje((char*)proto.DATA);
            break;
        case 3:
            
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
    // int fcs_recibido = fcs(proto.FRAMES, proto.LNG+1);
    // if (fcs_recibido != proto.FCS){                     //filtro 2, correspondiente a la comparacion de los fcs 
    //     return false;
    // } 
    return true; // Desempaquetado correctamente
}

void cb_receptor(void){
  bool level = digitalRead(RX_PIN);
  //  printf("%d",level);
  if (transmissionStarted){
    processBit(level);
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
        }
        
        // Incrementar el contador de bytes y finalizar la transmisión actual
        nbytes++;
        transmissionStarted = false;
    }
    
    // Incrementar el contador de bits
    nbits++;
}

