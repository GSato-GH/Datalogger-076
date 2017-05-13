    /* Datalogger 
 *
 *  Uso:
 *  - Armazenamento serial em EEPROM de informações enviadas por um LDR
 *  Gerenciamento via comunicação serial e algumas funcionalidades por teclado matricial
 *  
 *  Código de comunicação serial baseado no implementado pelo professor Thiago F. Tavares - FEEC-Unicamp
 *
 *  Guilherme Sato e Marco Aurélio de Oliveira Nona
 *  
 *  1S - 2017
 */

/* stdio.h contem rotinas para processamento de expressoes regulares */
#include <stdio.h>  
#include <Wire.h>
#include "TimerOne.h"
#define at24c16_address 0x50
#define read_address 0

/* Rotina auxiliar para comparacao de strings */
int str_cmp(char *s1, char *s2, int len) {
  /* Compare two strings up to length len. Return 1 if they are
   *  equal, and 0 otherwise.
   */
  int i;
  for (i=0; i<len; i++) {
    if (s1[i] != s2[i]) return 0;
    if (s1[i] == '\0') return 1;
  }
  return 1;
}

/* Processo de bufferizacao. Caracteres recebidos sao armazenados em um buffer. Quando um caractere
 *  de fim de linha ('\n') e recebido, todos os caracteres do buffer sao processados simultaneamente.
 */

/* Buffer de dados recebidos */
#define MAX_BUFFER_SIZE 31
typedef struct {
  char data[MAX_BUFFER_SIZE];
  unsigned int tam_buffer;
} serial_buffer;

/* Teremos somente um buffer em nosso programa, O modificador volatile
 *  informa ao compilador que o conteudo de Buffer pode ser modificado a qualquer momento. Isso
 *  restringe algumas otimizacoes que o compilador possa fazer, evitando inconsistencias em
 *  algumas situacoes (por exemplo, evitando que ele possa ser modificado em uma rotina de interrupcao
 *  enquanto esta sendo lido no programa principal).
 */
serial_buffer Buffer;

/* Todas as funcoes a seguir assumem que existe somente um buffer no programa e que ele foi
 *  declarado como Buffer. Esse padrao de design - assumir que so existe uma instancia de uma
 *  determinada estrutura - se chama Singleton (ou: uma adaptacao dele para a programacao
 *  nao-orientada-a-objetos). Ele evita que tenhamos que passar o endereco do
 *  buffer como parametro em todas as operacoes (isso pode economizar algumas instrucoes PUSH/POP
 *  nas chamadas de funcao, mas esse nao eh o nosso motivo principal para utiliza-lo), alem de
 *  garantir um ponto de acesso global a todas as informacoes contidas nele.
 */

/* Limpa buffer */
void buffer_clean() {
  Buffer.tam_buffer = 0;
}

/* Adiciona caractere ao buffer */
int buffer_add(char c_in) {
  if (Buffer.tam_buffer < MAX_BUFFER_SIZE) {
    Buffer.data[Buffer.tam_buffer++] = c_in;
    return 1;
  }
  return 0;
}


/* Flags globais para controle de processos da interrupcao */
int flag_check_command = 0;
char buffer_keyboard[3];


/* Rotinas de interrupcao */

/* Ao receber evento da UART */
void serialEvent() {
  char c;
  while (Serial.available()) {
    c = Serial.read();
    if (c=='\n') {
      buffer_add('\0'); /* Se recebeu um fim de linha, coloca um terminador de string no buffer */
      flag_check_command = 1;
    } else {
     buffer_add(c);
    }
  }
}

/* Verificação de qual tecla foi pressionada */
void return_line_column(int line, int column){
  int i;

/* Armazenamento das teclas pressionadas
 * Retorna números da linha e coluna */
    for(i=0; i<2;i++){
       buffer_keyboard [i] = buffer_keyboard[i+1];
    }
    if(line == 1 && column == 1){
      buffer_keyboard[2] = '1';
    }
    if(line == 1 && column == 2){
      buffer_keyboard[2] = '2';
    }
    if(line == 1 && column == 3){
      buffer_keyboard[2] = '3';
    }
    if(line == 2 && column == 1){
      buffer_keyboard[2] = '4';
    }
    if(line == 4 && column == 1){
      buffer_keyboard[2] = '*';
    }
    if(line == 4 && column == 3){
      buffer_keyboard[2] = '#';
    }
    flag_check_command = 1;
}

/* Funcoes internas ao void main() */
int sensorValue = 0;

void writeEEPROM(int deviceaddress, unsigned int eeaddress, byte data ) {
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress));
  Wire.write(data);
  Wire.endTransmission();
  delay(5);
}

byte readEEPROM(int deviceaddress, unsigned int eeaddress )  {
  byte data = 0xF;
 
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress));
  Wire.endTransmission();
 
  Wire.requestFrom(deviceaddress,1);
 
  if (Wire.available()){
    data = Wire.read();
  }
  return data;
}

/* Variáveis globais do controle da EEPROM */
int address = 0;
int EEPROM_value = 0;
int counter = 0;
int ledPin = 0;
int led_command = 0;
int counter_memoria = 0;
int memoria_automatica = 0;
void setup() {
  /* Inicializacao */
  buffer_clean();
  flag_check_command = 0;
  Serial.begin(9600);
  Wire.begin();
  Timer1.initialize(250000);
  Timer1.attachInterrupt(temporizador);
  writeEEPROM(at24c16_address, address, 0);
  pinMode(2, OUTPUT);
  
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);

  pinMode(8, INPUT);
  pinMode(9, INPUT);
  pinMode(10, INPUT);
  digitalWrite(8, HIGH);
  digitalWrite(9, HIGH);
  digitalWrite(10, HIGH);
  Serial.println("Ready to use");
}

/* Gerenciamento de contadores */
void temporizador() {
/* Led piscante indicativo de funcionamento */
  if(led_command == 1){
    counter --;
    if (counter > 0){
     ledPin = !ledPin;
      digitalWrite(13, ledPin);
    }
    else {
      digitalWrite(13, LOW);
      led_command = 0;
    }
  }
/* Informação, via terminal, de operação do modo de gravação automática */
  if (memoria_automatica == 1){
      counter_memoria++;
      if(counter_memoria == 4){
       //address++;
       counter_memoria = 0;
       buffer_add('R');
       buffer_add('E');
       buffer_add('C');
       buffer_add('O');
       buffer_add('R');
       buffer_add('D');
       flag_check_command = 1;
      }
  }
  else{
    counter_memoria = 0;
  }
}

/* Ao apertar uma tecla do teclado matricial */
void loop() {
      for (int pin = 4; pin<8; pin++) {
      digitalWrite(4, HIGH);
      digitalWrite(5, HIGH);
      digitalWrite(6, HIGH);
      digitalWrite(7, HIGH);
      digitalWrite(pin, LOW);
      //Verifica se alguma tecla da coluna 1 foi pressionada
      if (digitalRead(8) == LOW)
      {
        return_line_column(pin-3, 1);
        while(digitalRead(8) == LOW){}
      }
  
      //Verifica se alguma tecla da coluna 2 foi pressionada    
      if (digitalRead(9) == LOW)
      {
        return_line_column(pin-3, 2);
        while(digitalRead(9) == LOW){};
      }
     
      //Verifica se alguma tecla da coluna 3 foi pressionada
      if (digitalRead(10) == LOW)
      {
        return_line_column(pin-3, 3);
        while(digitalRead(10) == LOW){}
      }
    
   delay(10);
 }
  
/* Variáveis de controle do sensor
 * Divisão por 4 para caber na memória */
  sensorValue = analogRead(A1);
  sensorValue = sensorValue/4;
  int x, y, N;
  char out_buffer[31];
  int flag_write = 0;
  int mem_status = 0;

  /* A flag_check_command permite separar a recepcao de caracteres
   *  (vinculada a interrupca) da interpretacao de caracteres. Dessa forma,
   *  mantemos a rotina de interrupcao mais enxuta, enquanto o processo de
   *  interpretacao de comandos - mais lento - nao impede a recepcao de
   *  outros caracteres. Como o processo nao 'prende' a maquina, ele e chamado
   *  de nao-preemptivo.
   */

/* Verficação de palavras sinalizadoras de ação recebidas via serial */ 
  if (flag_check_command == 1) {
/* Recebe "PING" e retorna "PONG" */
    if (str_cmp(Buffer.data, "PING", 4) ) {
      sprintf(out_buffer, "PONG\n");
      flag_write = 1;
    }

/* Recebe "ID" e retorna a identificação do datalloger */
    if (str_cmp(Buffer.data, "ID", 2) ) {
      sprintf(out_buffer, "Datalogger do Bill e Marcao\n");
      flag_write = 1;
    }
/* Recebe "MEASURE" e retorna o valor atual do LDR */
    if(str_cmp(Buffer.data, "MEASURE", 7) ) {
      sprintf(out_buffer, "%d\n", sensorValue);
      flag_write = 1;
    }
/* Recebe "SUM" e dois números e retorna a soma desses dois númeroes */ 
    if (str_cmp(Buffer.data, "SUM", 3) ) {
      sscanf(Buffer.data, "%*s %d %d", &x, &y);
      sprintf(out_buffer, "SUM = %d\n", x+y);
      flag_write = 1;
    }
/* Recebe "RECORD" e armazena na EEPROM o valor atual medido pelo LDR */
    if(str_cmp(Buffer.data, "RECORD", 6 ) ) {
       address++;
       writeEEPROM(at24c16_address, read_address, address); 
       writeEEPROM(at24c16_address, address, sensorValue); 
       buffer_clean();
    }
/* Recebe "MEMSTATUS" e retorna a quantidade de dados armazenados na EEPROM */
    if(str_cmp(Buffer.data, "MEMSTATUS", 9) ) {
       mem_status = readEEPROM(at24c16_address, read_address);
       sprintf(out_buffer, "Elements: %d\n", mem_status);
       flag_write = 1;
    }
/* Recebe "RESET" e apaga os dados na EEPROM */
    if(str_cmp(Buffer.data, "RESET", 5) ) {
      address = 0;
      writeEEPROM(at24c16_address, read_address, address); 
      buffer_clean();
    }
/* Recebe "GET" e um número e retorna o valor armazenado na EEPROM referente à posição informada */
    if(str_cmp(Buffer.data, "GET", 3) ) {
      sscanf(Buffer.data, "%*s %d", &N);
      EEPROM_value = readEEPROM(at24c16_address, N);
      sprintf(out_buffer, "%d\n", EEPROM_value);
      flag_write = 1;
    }
/* Ações de acordo com comandos enviados via teclado matricial
 * Se "#1*" aciona led indicativo de funcionamento
 * Se "#2*" armazena na EEPROM o valor atual medido pelo LDR
 * Se "#3*" ativa o modo de armazenamento automático na EEPROM
 * Se "#4*" desativa o modo de armazenamento automático
*/
    if(str_cmp(buffer_keyboard,"#1*", 3)){
      led_command = 1;
      counter = 10;
      buffer_keyboard[0] = 0; 
    }
    if(str_cmp(buffer_keyboard,"#2*", 3)){
       address++;
       writeEEPROM(at24c16_address, read_address, address); 
       writeEEPROM(at24c16_address, address, sensorValue);
       buffer_keyboard[0] = 0; 
    }
    if(str_cmp(buffer_keyboard,"#3*", 3)){
      memoria_automatica = 1;
      buffer_keyboard[0] = 0; 
    }
    if(str_cmp(buffer_keyboard,"#4*", 3)){
      memoria_automatica = 0;
      buffer_keyboard[0] = 0; 
    }
    else{
      buffer_clean();
    }
    flag_check_command = 0;
  }

  /* Posso construir uma dessas estruturas if(flag) para cada funcionalidade
   *  do sistema. Nesta a seguir, flag_write e habilitada sempre que alguma outra
   *  funcionalidade criou uma requisicao por escrever o conteudo do buffer na
   *  saida UART.
   */
  if (flag_write == 1) {
    Serial.write(out_buffer);
    buffer_clean();
    Buffer.data[0] = "\0";
    flag_write = 0;
  }
  
}
