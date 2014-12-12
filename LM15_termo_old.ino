#include <SPI.h>
#include <lcd_lm15.h>
//#include <LM15SGFNZ07.h>
#include <IRremote.h>
#include <math.h>

//пульт от тюнера manli
#define KEY_MUTE 0x61D6C837
#define KEY_SOURCE 0x61D68877
#define KEY_FM 0x61D638C7
#define KEY_POWER 0x61D648B7
#define KEY_1 0x61D6807F
#define KEY_2 0x61D640BF
#define KEY_3 0x61D6C03F
#define KEY_STEREO 0x61D6B04F
#define KEY_4 0x61D620DF
#define KEY_5 0x61D6A05F
#define KEY_6 0x61D6609F
#define KEY_SHOT 0x61D69867
#define KEY_7 0x61D6E01F
#define KEY_8 0x61D610EF
#define KEY_9 0x61D6906F
#define KEY_FULLSCR 0x61D608F7
#define KEY_BACK 0x61D650AF
#define KEY_0 0x61D600FF
#define KEY_UP 0x61D6D02F
#define KEY_PLUS 0x61D630CF
#define KEY_T 0x61D6D827
#define KEY_REC 0x61D6B847
#define KEY_DOWN 0x61D6A857
#define KEY_MINUS 0x61D618E7
#define KEY_STOP 0x61D6708F
#define KEY_PAUSE 0x61D67887
#define KEY_RW 0x61D6F00F
#define KEY_FW 0x61D658A7



int RECV_PIN = 2;
IRrecv irrecv(RECV_PIN);
boolean ShowGraph;
boolean ShowLow;

decode_results results;
//LM15SGFNZ07 lcd(8,9,10,11,12); 
// [SDATA,SCLK,RS,RESET,CS][красный, белый , зеленый черный коричневый] {синий = gnd, желтый = 3.3В}
/*  new
 PIN LCD	PIN ARDUINO
 1	10  CS       10
 2	8  RESET     8
 3	6  RS        9
 4	13  SCLK     13
 5	11  SDATA    11
 6	3.3V
 7	GND
 8	3.3V
 9	GND
 10	3.3V
 */
#define ScreenSizeX 101
#define ScreenSizeY 80
#define CenterY 40
int graph[ScreenSizeX + 1];
unsigned char pos;
double lowtemp = 100.0;
double hightemp = -100.0;
double amp;
unsigned char mode = 0;
int iamp,imin,imax;

void setup()
{
  //  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver
  // irrecv.blink13(true);
  pinMode(RECV_PIN + 1, OUTPUT);
  digitalWrite(RECV_PIN + 1, LOW);
  pinMode(RECV_PIN + 2, OUTPUT);
  digitalWrite(RECV_PIN + 2, HIGH);
  prepareDisp();
  pos = 0;
//  GraphShiftY = 200;
}

void prepareDisp(){
  initLCD_LM15(32);
  Redraw0();
}


void Redraw0(){
  Redraw1();
  printLCD("min", 0, 2, 1, GREEN, BLACK);
  printLCD("max", 0, 3, 1, BLUE, BLACK);
  printLCD("C ", 10, 2, 1, GREEN, BLACK);
  printLCD("C ", 10, 3, 1, BLUE, BLACK);

}

void Redraw1(){
  fillLCD_LM15(0,0);
  printLCD("Temperature", 0, 0, 1, ORANGE, BLACK);
  printLCD("C ", 10, 1, 1, WHITE, BLACK);
}


void storeNewTemp(double t){
  drawPixelLCD_LM15(graph[pos],pos, BLACK);
  graph[pos]=(unsigned char)(iamp);
  drawPixelLCD_LM15(graph[pos] - graph[0] + 40, pos, RED);
  if(++pos>=ScreenSizeX){pos = 0;fillLCD_LM15(0,0);}
}

double Thermister(int RawADC) {
  double Temp;
  // See http://en.wikipedia.org/wiki/Thermistor for explanation of formula
  Temp = log(((10240000/RawADC) - 10000));
  Temp = 1 / (0.001129148 + (0.000234125 * Temp) + (0.0000000876741 * Temp * Temp * Temp));
  Temp = Temp - 273.15;           // Convert Kelvin to Celcius
  return Temp;
}


void printTemp(void) {
  printdouble(amp, 4, 1, 2, WHITE, BLACK);
}

void printLowHigh(void) {
  if(lowtemp>amp){
    lowtemp = amp;
  }
  if(hightemp<amp){
    hightemp = amp;
  }
}

void printlh(void){
    printdouble(lowtemp, 4, 2, 1, GREEN, BLACK);
    printdouble(hightemp, 4, 3, 1, BLUE, BLACK);

}

void printdouble(double t, unsigned char x, unsigned char y, unsigned int size, unsigned int color_font, unsigned int color){
  char buffer[5];
  dtostrf(t, 5, 1, buffer);
  printLCD(buffer, x, y, size, color_font, color);
}

void printLCD(char * t, unsigned char x, unsigned char y, unsigned int size, unsigned int color_font, unsigned int color){
  setCursorLCD_LM15(y*14, x*8);
  textLCD_LM15(t, color_font, color);
}

void oscilograph(void){
  
  for(pos = 0;pos < ScreenSizeX;pos++)  graph[pos]=analogRead(1);
  fillLCD_LM15(0,0);
  for(pos = 0;pos < ScreenSizeX;pos++) {
    drawPixelLCD_LM15(((graph[pos]-graph[0])>>3)+40, pos, GREEN);
    
  }
}


void loop()
{
    iamp = analogRead(0);
    amp = Thermister(iamp);  // Read sensor
    printLowHigh();

  switch(mode){
  case 0:
    printTemp();
    printlh();
    delay(1000);
    break;
  case 1:
    printTemp();
    storeNewTemp(amp);
    delay(1000);
    break;
  case 2:
    oscilograph();
    //delay(1000);
    break;   
  case 3:
    oscilograph();
    delay(1000);
    break;

  }

  if (irrecv.decode(&results)) {
    switch(results.value){
    case KEY_SHOT:
      Redraw1();
      mode = 1;
      break;
    case KEY_T:
      Redraw0();
      mode = 0;
      break;
    case KEY_REC:
      Redraw1();
      mode = 1;
      break;
    case KEY_SOURCE:
      mode= (mode+1)%4;
      
      break;
    case KEY_STOP:
      mode= 2;
      
      break;
    case KEY_PAUSE:
      mode= 3;
      
      break;
    case KEY_PLUS:
      //slowG += 2;
      break;
    case KEY_MINUS:
    //  slowG -= 2;
      break;
    case KEY_UP:
//      GraphShiftY--;
      break;
    case KEY_DOWN:
  //    GraphShiftY++;
      break;
    }

    irrecv.resume(); // Receive the next value
  }

//  gtime++; 
}






