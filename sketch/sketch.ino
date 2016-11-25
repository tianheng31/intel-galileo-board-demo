#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* DEBUG设置为1能看到从串口输出的调试信息，设置为0禁止 */
#define DEBUG 1
/* 收发的信息限制在32字节以内 */
#define BUFFERSIZE 32
/* 端口2000接收从Node.js发来的信息 */
#define SKETCH_UDP_PORT 2000
/* 端口2010发送信息到Node.js */
#define NODEJS_UDP_PORT 2010

// 当且仅当错误发生时调用本函数
void printError(char *str)
{
  Serial.print("ERROR: ");
  Serial.println(str);
}

// for the UDP server
struct sockaddr_in servAddr, clntAddr;
socklen_t clen = sizeof(clntAddr);
int servSock;
char msg_buffer[BUFFERSIZE + 1];
fd_set rdevents;
struct timeval tv;
int rc;
// for sending data
char data[BUFFERSIZE + 1];

int keyPin = 13;
int state = 0;

/******************************/
int RS = 12; // 高电平选择数据寄存器，低电平选择指令寄存器
int RW = 11; // 高电平进行读操作，低电平进行写操作
int DB[] = {3, 4, 5, 6, 7, 8, 9, 10}; // 定义开发板与数据总线相连的引脚管脚
int EN = 2;
char defaultMessage[] = "WELCOME  TO  USEWEATHER FORECAST";
/******************************/

// 发送UDP数据报
void sendUDPMessage(char *message)
{
  struct sockaddr_in serv_addr; /* AF_INET对应地址结构为sockaddr_in */
  int sockfd, slen = sizeof(serv_addr);

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) /* 创建一个socket */
  {
    printError("socket");
    return;
  }

  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET; /* sin_family字段总设置为AF_INET */
  serv_addr.sin_port = htons(NODEJS_UDP_PORT); /* 端口号 */

  // 使用回环地址
  if (inet_aton("127.0.0.1", &serv_addr.sin_addr) == 0)
  {
    printError("inet_aton() failed.");
    close(sockfd);
    return;
  }

  if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    printError("sendto()");

  close(sockfd);
}

// 初始化UDP数据报服务器
int initializeUDPServer()
{
  if ((servSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    printError("socket");
  else if (DEBUG) Serial.println("Server : Socket() successful.");

  bzero(&servAddr, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(SKETCH_UDP_PORT); /* local port */
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* s_addr是IPv4地址，这里设置为监听所有地址 */
  if (bind(servSock, (struct sockaddr* )&servAddr, sizeof(servAddr)) == -1)
    printError("bind");
  else if (DEBUG) Serial.println("Server : bind() successful.");

  memset(msg_buffer, 0, sizeof(msg_buffer));
}

void collect(char *data)
{
  int temp = digitalRead(keyPin);
  if (temp == HIGH)
  {
    state = !state;

    if (state == 1)
    {
      strcpy(data, "101010100"); // 北京
    }

    if (state == 0)
    {
      strcpy(data, "101050101"); // 哈尔滨
    }
  }
  delay(500);
}

void LCD_Command_Write(int value)
{
  int i = 0;
  for (i = DB[0]; i <= RS; i++)
  {
    digitalWrite(i, value & 01);
    value >>= 1;
  }
  digitalWrite(EN, LOW);
  delayMicroseconds(1);
  digitalWrite(EN, HIGH);
  delayMicroseconds(1);
  digitalWrite(EN, LOW);
  delayMicroseconds(1);
}

void LCD_Data_Write(int value)
{
  int i = 0;
  digitalWrite(RS, HIGH);
  digitalWrite(RW, LOW);
  for (i = DB[0]; i <= DB[7]; i++)
  {
    digitalWrite(i, value & 01);
    value >>= 1;
  }
  digitalWrite(EN, LOW);
  delayMicroseconds(1);
  digitalWrite(EN, HIGH);
  delayMicroseconds(1);
  digitalWrite(EN, LOW);
  delayMicroseconds(1);
}

void LCD_Display(char *LCD_BUFFER)
{
  LCD_Command_Write(0x01); // 清屏指令
  delay(10);

  for (int i = 0; i < 16; i++)
  {
    LCD_Data_Write(LCD_BUFFER[i]);
  }
  delay(10);

  LCD_Command_Write(0xc0);
  delay(10);

  for (int i = 16; i < 32; i++)
  {
    LCD_Data_Write(LCD_BUFFER[i]);
  }
  delay(1000);
}

void setup() {
  Serial.begin(9600); // 打开串口，设置波特率为9600bps
  delay(3000);

  initializeUDPServer(); /* 初始化UDP服务器 */

  /******************************/
  for (int i = EN; i <= RS; i++) { /* 初始化LCD1602 */
    pinMode(i, OUTPUT);
  }
  delay(100);
  LCD_Command_Write(0x38);
  delay(64);
  LCD_Command_Write(0x38);
  delay(50);
  LCD_Command_Write(0x38);
  delay(20);
  LCD_Command_Write(0x06);
  delay(20);
  LCD_Command_Write(0x0E);
  delay(20);
  LCD_Command_Write(0x01);
  delay(100);
  LCD_Command_Write(0x80);
  delay(20);
  /******************************/

  /******************************/
  pinMode(keyPin, INPUT); /* 初始化引脚模式 */
  /******************************/

  LCD_Display(defaultMessage);
}

void loop() {
  FD_ZERO(&rdevents);
  FD_SET(servSock, &rdevents);
  // 设置阻塞时间为5秒
  tv.tv_sec = 5;
  tv.tv_usec = 0;

  rc = select(servSock + 1, &rdevents, NULL, NULL, &tv);
  if (rc == -1) {
    if (DEBUG) {
      Serial.println("Error in Select.");
    }
  }
  else if (rc == 0) {
    // 超时
    memset(data, 0, sizeof(data));
    collect(data);
    sendUDPMessage(data);
  }
  else {
    // 检查UDP服务器是否从Node.js中收到消息
    if (FD_ISSET(servSock, &rdevents)) {
      if (recvfrom(servSock, msg_buffer, BUFFERSIZE, 0, (struct sockaddr*)&clntAddr, &clen) == -1)
      {
        printError("recvfrom()");
        return;	/* 退出loop() */
      }
      if (DEBUG) {
        int msgLength = strlen(msg_buffer);
        Serial.print("Received packet from: ");
        Serial.println(inet_ntoa(clntAddr.sin_addr));
        Serial.print(msgLength);
        Serial.println(" bytes.");
      }
      Serial.println(msg_buffer);
      LCD_Display(msg_buffer);
      // 清零消息缓冲区
      memset(msg_buffer, 0, sizeof(msg_buffer));
    }
  }
}