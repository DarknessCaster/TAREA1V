all:
	g++ receptor.cpp -o recibir -lwiringPi
	g++ emisor.cpp -o enviar -lwiringPi
	g++ delay.cpp -o delay -lwiringPi

Enviar:
	sudo ./enviar

Recibir:
	sudo ./recibir

Delay:
	sudo ./delay &

todo:
	sudo ./delay &
	sudo ./recibir &
	sudo ./enviar
