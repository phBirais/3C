/**
   Projeto: Carteirinha Charging Center com ESP01 e Arduino UNO

   Resumo: O funcionamento básico deste projeto pode ser descrito como uma central com interface humano máquina (IHM).
   Esta interface será composta de um display para mostrar ao usuário as informações e um teclado que será responsável por inserir as informações no sistema.
   A central será controlada por um microcontrolador ATmega-328 inserido a um Arduino contendo acesso a rede através de um módulo Wi-ﬁ ESP01

*/
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include "rgb_lcd.h"
#include <Keypad.h>
//String email = "eber.lima10@gmail.com";
rgb_lcd lcd;
const int colorR = 127;
const int colorG = 255;
const int colorB = 212;

/**********************************************************/
//a cada 24 colunas ele troca de linha sozinho
char array0[] = "Seja Bem Vindo  "; //16
char array1[] = "Sist de Recarga   "; // 18
char array2[] = "Aprox o Cartao  "; //16
char array3[] = "Escolha Funcao Desejada"; //23
char array4[] = "Digite o Valor Desejado"; //23
char array5[] = "A - carregar ; B - Saldo ; C - Transferir ; D - Sair"; //52
int tim = 200; //the value of delay time
/*********************************************************/

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the symbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {22, 24, 26, 28}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {30, 32, 34, 36}; //connect to the column pinouts of the keypad

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

#define SS_PIN 53
#define RST_PIN 5

#define SerialWifi Serial3
uint32_t msg = 0; //numero de creditos enviados no email
int conectado = 0;
uint32_t addcred = 0;
uint32_t SA = 0;
int h = 0; //linha da matriz
int j = 0;

MFRC522 mfrc522(SS_PIN, RST_PIN);
//Matriz de pessoas e códigos cadastrados
String matriz[5][4] = {"Pedro", "EA 35 91 ED", "0", "phbirais@gmail.com",
                       "Eber", "2B C5 CC 46", "3", "eber.lima10@gmail.com",
                       "Bruno", "D6 1A DB 4F", "40", "brunomatsumura@gmail.com",
                       "William", "i", "10", "phbirais@gmail.com",
                       "Random", "71 69 E2 1E", "1", "eber.lima10@gmail.com",
                      };

char st[20];
char EscolheFuncao;
char FuncaoEscolhida;
char ConfirmaValor;
bool find = false;
char ValorRecargaDigitado;
int ValorRecargaInt;
String ValorRecargaString;
//vetor dos créditos de cada pessoa
uint32_t credito[5];


static bool espCheckPresence(uint16_t timeout, uint8_t retries);
static void espBegin();
static void wifiSetup();
static void handleServerClient();
static void wifiStartServer();
static void wifiSendData(uint32_t value);
static String sendModemCommand(String cmd, uint16_t timeout, uint8_t retries = 1);
static void serverProccessClientCommand();

String graph = "<div><iframe width=\"600\" height=\"371\" seamless frameborder=\"0\" scrolling=\"no\" src=\"https://docs.google.com/spreadsheets/d/e/2PACX-1vR8s42u9PIdtwFOw-40zyTslirCnkRh3qC8VLIpW0g0hfNXG_YMYwXO9wqkZMWlw2mI-HxiabLMg6jz/pubchart?oid=249031932&amp;format=interactive\"></iframe></div>";

uint32_t lastDataSent = millis();
uint16_t dataInterval = 60000;
String SSID = "UFABC";
String PW = "85265";

void setup()
{
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.setRGB(colorR, colorG, colorB);

  lcd.clear();
  lcd.setCursor(16, 0);
  for (int positionCounter0 = 0; positionCounter0 < 16 ; positionCounter0++)
  {
    lcd.scrollDisplayLeft();
    lcd.print(array0[positionCounter0]);
    delay(tim);
  }
  delay(600);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sist de recarga");
  delay(1000);
  lcd.setCursor(0, 1);
  lcd.print("aguarde........");


  Serial.begin(115200); // Inicia a serial
  SPI.begin();    // Inicia  SPI bus
  mfrc522.PCD_Init(); // Inicia MFRC522

  Serial.println("Bem-vindo ao 3C");


  //Transforma a coluna de creditos da matriz string em coluna de inteiros
  for (h = 0; h < 5; h++)
  {
    for (j = 2; j < 3; j++)
    {
      SA = matriz[h][j].toInt(); //saldo antes de carregar

    }
  }

  Serial.begin(115200);
  SerialWifi.begin(115200);
  SPI.begin();    // Inicia  SPI bus
  mfrc522.PCD_Init(); // Inicia MFRC522
  Serial.println("Bem-vindo ao 3C");
  //tranf
  for (int h = 0; h < 5; h++) {
    for (int j = 2; j < 3; j++) {
      credito[h] = matriz[h][j].toInt();
    }
  }
  //checa esp
  if (!espCheckPresence(10, 500))
  {
    Serial.println("Nao achei bixo");
    while (1);
  }

  delay(1000);
  //Configura Wifi e Esp
  wifiSetup();
}

void loop()
{

  find = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Aproxime o seu");
  lcd.setCursor(0, 1);
  lcd.print("cartao ao leitor");
  // Busca novos cartões
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // Escolhe um
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  //Mostra UID na serial
  Serial.print("UID da tag :");//Não colocar no display
  String conteudo = "";
  byte letra;

  for (byte i = 0; i < mfrc522.uid.size; i++) //Não printar no display
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  //coloca o codigo em letra maiuscula
  conteudo.toUpperCase();

  //varre a matriz de cadstro para encontar a TAG
  for (h = 0; h < 5; h++) {
    for (j = 1; j < 2; j++) {
      if (conteudo.substring(1) == matriz[h][j]) {
        //se conteudo estiver na matriz, printa o nome da pessoa associada a tag
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Acesso liberado!");
        lcd.setCursor(0, 1);
        lcd.print("Bem vindo, " + matriz[h][j - 1]);
        lcd.setRGB(0, 255, 0);
        delay(1700);
        lcd.setRGB(216, 191, 216);

        //escolher entre opções
        RotinaEscolha();
        find = true;
        delay(1000);


      }
      break;
    }
  }
  //se não achar na matriz, acesso negado
  if (find == false) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Acesso Negado!");
    lcd.setCursor(0, 1);
    lcd.print("Nao Cadastrado");
    Serial.println("\nCarteirinha não cadastrada");
    Serial.println("Acesso Negado!"); //voltar para tela inicial
    Serial.println("Leitura");
    Serial.println(conteudo.substring(1));

    lcd.setRGB(255, 0, 0);
    delay(1500);
    lcd.setRGB( 216, 191, 216);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Aproxime o seu");
    lcd.setCursor(0, 1);
    lcd.print("cartao ao leitor");
    delay(1500);
  }
  else
  {
    //escolher entre opções
  }

}



static String sendModemCommand(String cmd, uint16_t timeout, uint8_t retries)
{
  /* Cria uma string onde a resposta será armazenada */
  String response = "";
  uint8_t ret = retries;

  /* Tenta enviar o comando. Se não obtiver sucesso, tentará retries vezes */
  do {
    Serial.println("Sending: " + cmd);
    SerialWifi.print(cmd);
    uint32_t currentTime = millis();

    /* Após enviar o comando, aguarda pelo OK. Se ocorrer o timeout, ele desiste
       e vai para a próxima tentativa*/
    while (millis() < (currentTime + timeout))
    {
      while (SerialWifi.available()) response += (char) SerialWifi.read();
      /* Quebra o loop se o OK for encontrado */
      if (response.indexOf("OK") != -1) break;
    }
    Serial.println("Response " + String(retries - ret) + ": " + response);
    if (response.indexOf("OK") != -1) break;
    response = "";
  } while (ret-- > 1);

  Serial.println(response);
  return response;
}

static bool espCheckPresence(uint16_t timeout, uint8_t retries)
{
  /* Cria uma string onde a resposta será armazenada */
  String response = "";

  /* Envia a string AT diversas vezes, até conseguir uma resposta
     OK ou acabar o tempo/número de tentativas*/
  do {
    SerialWifi.print("AT\r\n");
    uint32_t currentTime = millis();
    while (millis() < (currentTime + timeout))
    {
      while (SerialWifi.available()) response += (char) SerialWifi.read();
      if (response.indexOf("OK") != -1) return true;
    }
    Serial.println(response);
    response = "";
  } while (retries-- > 1);

  return false;
}

/* Inicializa as configurações de WiFi. */
static void wifiSetup()
{
  String response;
  /* Envia o comando de conectar ao WiFi */
  response = sendModemCommand("AT+CWJAP=\"" + SSID + "\",\"" + PW + "\"\r\n", 10000);

  /* Se foi possível conectar, mostra o IP */
  if (response.indexOf("OK") != -1)
  {
    sendModemCommand("AT+CIFSR\r\n", 1000);
  }

  /* Configura o ESP como WiFi Station */
  sendModemCommand("AT+CWMODE=1\r\n", 1000);

  const int colorR = 216;
  const int colorG = 191;
  const int colorB = 216;
  lcd.setRGB(colorR, colorG, colorB);
}


static void wifiSendData(uint32_t msg, String email)
{
  /* Prepara a requisição GET para o PushingBox com o valor desejado*/
  String data = "GET /pushingbox?devid=v12BC0951E24E2C2";
  data += "&email=" + (email);
  data += "&msg=" + String(msg);
  //data += "&credit=" + String(credit);

  data += " HTTP/1.1\r\nHost: api.pushingbox.com\r\n\r\n";

  /* Conecta ao servidor */
  sendModemCommand("AT+CIPSTART=\"TCP\",\"api.pushingbox.com\",80\r\n", 2500);
  delay(500);

  /* Descreve o tamanho dos dados para envio */
  sendModemCommand("AT+CIPSEND=" + String(data.length()) + "\r\n", 2000);
  delay(500);

  /* Envia os dados */
  SerialWifi.print(data);
  Serial.println("Data sent.");
}




void mensageminicial()
{
  lcd.setRGB(colorR, colorG, colorB);
  lcd.clear();
  lcd.setCursor(16, 0);
  for (int positionCounter2 = 0; positionCounter2 < 21 ; positionCounter2++)
  {
    lcd.scrollDisplayLeft();
    lcd.print(array2[positionCounter2]);
    delay(tim);
  }
  Serial.println("Aproxime o seu cartao do leitor...");
  Serial.println();
}



void RotinaEscolha() {

  EscolherFuncao();
  FuncaoSelecionada();


}

void EscolherFuncao()
{
  int cont;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("opcoes possiveis:");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("A-carga/B-saldo");
  lcd.setCursor(0, 1);
  lcd.print("C-trasf/D-sair");


  cont = 0;
  EscolheFuncao = " ";
  /*while (EscolheFuncao != 'A' or 'B' or 'C' or 'D' or '2')
    {
    EscolheFuncao = customKeypad.getKey();
    Serial.println(EscolheFuncao);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Funcao Desejada:");
    Serial.println("Funcao Desejada:");
    lcd.setCursor(0, 1);
    lcd.print("A- Carregar");
    Serial.println("A- Carregar");
    if (EscolheFuncao == 'A' or EscolheFuncao == 'B' or EscolheFuncao == 'C' or EscolheFuncao == 'D')
    {
      Serial.println(EscolheFuncao);
      break;
    }
    delay (2000);
    EscolheFuncao = customKeypad.getKey();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Funcao Desejada:");
    Serial.println("Funcao Desejada:");
    lcd.setCursor(0, 1);
    lcd.print("B- Consult Saldo");
    Serial.println("B- Consult Saldo");
    if (EscolheFuncao == 'A' or EscolheFuncao == 'B' or EscolheFuncao == 'C' or EscolheFuncao == 'D')
    {
      Serial.println(EscolheFuncao);
      break;
    }
    delay (2000);
    EscolheFuncao = customKeypad.getKey();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Funcao Desejada:");
    Serial.println("Funcao Desejada:");
    lcd.setCursor(0, 1);
    lcd.print("C- Transf Saldo");
    Serial.println("C- Transf Saldo");
    if (EscolheFuncao == 'A' or EscolheFuncao == 'B' or EscolheFuncao == 'C' or EscolheFuncao == 'D')
    {
      Serial.println(EscolheFuncao);
      break;
    }
    delay (2000);
    EscolheFuncao = customKeypad.getKey();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Funcao Desejada:");
    Serial.println("Funcao Desejada:");
    lcd.setCursor(0, 1);
    lcd.print("D- Sair");
    Serial.println("D- Sair");
    if (EscolheFuncao == 'A' or EscolheFuncao == 'B' or EscolheFuncao == 'C' or EscolheFuncao == 'D')
    {
      Serial.println(EscolheFuncao);
      break;
    }
    delay (2000);
    cont++;
    if (cont >= 3)
    {
      EscolheFuncao = '2';
      Serial.println(EscolheFuncao);
      break;
    }

    }
  */

  while (EscolheFuncao != 'A' or 'B' or 'C' or 'D' or '2')
  {
    EscolheFuncao = customKeypad.getKey();
    Serial.println(EscolheFuncao);
    if (EscolheFuncao == 'A' or EscolheFuncao == 'B' or EscolheFuncao == 'C' or EscolheFuncao == 'D')
    {
      Serial.println(EscolheFuncao);
      break;
    }
    delay(50);
  }
}




void FuncaoSelecionada()
{
  switch (EscolheFuncao)
  {
    case 'A':
      {
        EscolheFuncao = ' ';
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Funcao Escolhida");
        Serial.println("Funcao Escolhida");
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Recarregar");
        Serial.println("Recarregar");
        delay (10);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Valor desejado:");
        Serial.println("Valor:");
        Serial.println("Apos digitar conferir o valor");
        //delay(2000);
        Serial.println("Para confirmar aperte *");
        //delay(2000);
        Serial.println("Para cancelar aperte #");
        //delay(2000);
        Serial.println("Para limpar aperte D");
        delay(500);
        ConfirmaValor = ' ';
        ValorRecargaString = ' ';

        //while (ConfirmaValor != '*' ) // não pode ser essa variável
        while (ValorRecargaDigitado != '*' )
        {
          // ConfirmaValor = customKeypad.getKey();
          ValorRecargaDigitado = customKeypad.getKey();

          //ValorRecargaString.concat(String(ValorRecargaInt < 10 ? " 0" ,));
          if (ValorRecargaDigitado != 0 && ValorRecargaDigitado != '*')
          {
            //ValorRecargaInt = ValorRecargaDigitado.toInt
            ValorRecargaString.concat(ValorRecargaDigitado);
            delay(50);
          }
          Serial.println("Valor:" + ValorRecargaString);
          lcd.setCursor(0, 0);
          lcd.print("Valor desejado:");
          lcd.setCursor(0, 1);
          lcd.print(ValorRecargaString);
        }
        Serial.println("Valor Aceito");
        Serial.println(ValorRecargaString);
        addcred = ValorRecargaString.toInt();
        Serial.println(addcred);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Insira o cartao");
        delay(3000);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Carregamento ok");
        lcd.setCursor(0, 1);
        lcd.print("email enviado");
        delay(2500);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Creditos add:");
        lcd.setCursor(0, 1);
        lcd.print(addcred);
        delay(2200);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Valor total(R$):");
        lcd.setCursor(0, 1);
        lcd.print(addcred * 4.65);
        delay(2200);
        SA = credito[h] + addcred;
        Serial.println(SA);

        Serial.println(matriz[h][j + 2]);
        credito[h] = SA;
        delay(1000);

        wifiSendData(SA, (matriz[h][j + 2]));
        ValorRecargaString = ' ';
        ValorRecargaDigitado = " ";
        RotinaEscolha();
      }
    case 'B':
      {
        EscolheFuncao = ' ';
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Funcao Escolhida");
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Consultar Saldo");
        delay (1000);

        SA = credito[h];

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Seu saldo e de:");
        lcd.setCursor(0, 1);
        lcd.print(SA);
        Serial.print(SA);
        Serial.println(matriz[h][j + 2]);
        delay(1000);
        RotinaEscolha();

      }
    case 'C':
      {
        break;
      }
    case 'D':
      {
        EscolheFuncao = ' ';
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Funcao Escolhida");
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Sair");
        delay (1000);
        lcd.clear();
        {
          break;
        }
      }
    case '2':
      {
        /*
          EscolheFuncao = ' ';
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Opcao Invalida");
          delay (1000);
          {
          lcd.clear();
          break;
          }
        */
      }
  }
}
