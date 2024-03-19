int pin = 23; // Pin por el que vamos a controlar el encedido y apagado del led


void setup() {
  pinMode(pin, OUTPUT);   // declaramos el pin como salida ( OUTPUT )
}

void loop() {

  //Inicio de la S en código morse
  digitalWrite (pin,HIGH);  // Encendemos el led
  delay (50);	 // Pausa de 50ms
  digitalWrite (pin,LOW);   // Apagamos el led
  delay (50);	 // Pausa de 50ms
  digitalWrite (pin,HIGH);  // Encendemos el led
  delay (50);	 // Pausa de 50ms
  digitalWrite (pin,LOW);   // Apagamos el led
  delay (50);	 // Pausa de 50ms
  digitalWrite (pin,HIGH);  // Encendemos el led
  delay (50);	 // Pausa de 50ms
  digitalWrite (pin,LOW);   // Apagamos el led
  delay (50);	 // Pausa de 50ms
  //Fin de la S en código morse

  //Inicio de la O en código morse
  digitalWrite (pin,HIGH);  // Encendemos el led
  delay (500);	 // Pausa de 500ms
  digitalWrite (pin,LOW);   // Apagamos el led
  delay (500);	 // Pausa de 500ms
  digitalWrite (pin,HIGH);  // Encendemos el led
  delay (500);	 // Pausa de 500ms
  digitalWrite (pin,LOW);   // Apagamos el led
  delay (500);	 // Pausa de 500ms
  digitalWrite (pin,HIGH);  // Encendemos el led
  delay (500);	 // Pausa de 500ms
  digitalWrite (pin,LOW);   // Apagamos el led
  delay (500);	 // Pausa de 500ms
  //Fin de la S en código morse

  //Inicio de la S en código morse
  digitalWrite (pin,HIGH);  // Encendemos el led
  delay (50);	 // Pausa de 50ms
  digitalWrite (pin,LOW);   // Apagamos el led
  delay (50);	 // Pausa de 50ms
  digitalWrite (pin,HIGH);  // Encendemos el led
  delay (50);	 // Pausa de 50ms
  digitalWrite (pin,LOW);   // Apagamos el led
  delay (50);	 // Pausa de 50ms
  digitalWrite (pin,HIGH);  // Encendemos el led
  delay (50);	 // Pausa de 50ms
  digitalWrite (pin,LOW);   // Apagamos el led
  delay (2000);	 // Pausa de 50ms
  //Fin de la S en código morse
}