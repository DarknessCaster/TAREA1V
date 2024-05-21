all:
  g++ receptor.cpp -o recibir -lwiringPi
  g++ emisor.cpp -o enviar -lwiringPi

Enviar:
  sudo ./enviar

Recibir:
  sudo ./recibir &

todo:
  sudo ./recibir &
  sudo ./enviar
