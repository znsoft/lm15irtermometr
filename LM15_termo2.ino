#include <SPI.h>
//#include <lcd_lm15.h>
#include <LM15SGFNZ07SPI.h>
#include <IRremote.h>
#include <math.h>
#include <util/delay.h> 


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
//boolean ShowGraph;
//boolean ShowLow;
double filtercoef;
//int sensVal;           // for raw sensor values from http://playground.arduino.cc/Main/Smooth
//double filterVal;       // this determines smoothness  - .0001 is max  1 is off (no smoothing)
double smoothedVal;     // this holds the last loop value just use a unique variable for every different sensor that needs smoothing
//double smoothedVal2;
decode_results results;
LM15SGFNZ07SPI lcd(9,12,10); //( byte RS, byte RESET, byte CS)
//*  the 13 pin is always SCLK 
//*   and 11 pin always SDATA for fast SPI 
// [SDATA,SCLK,RS,RESET,CS][красный, белый , зеленый черный коричневый] {синий = gnd, желтый = 3.3В}
/*  new
 PIN LCD	PIN ARDUINO
 1	  CS       10   красный
 2	  RESET     12    белый
 3	  RS        9    зеленый 
 4	13  SCLK     13(const)   черный 
 5	11  SDATA    11(const)   коричневый
 6	3.3V
 7	GND               синий
 8	3.3V              желтый
 9	GND
 10	3.3V
 */
#define ScreenSizeX 101
#define ScreenSizeY 80
#define CenterY 40
//#define Soundpin 7
#define laserGndpin 7
#define laserPWMpin 6
int graph[ScreenSizeX + 1];
unsigned char pos;
double lowtemp = 100.0;
double hightemp = -100.0;
double amp,ang = 0.0;
//double angle;
unsigned char mode = 0;
int iamp,imin,imax,angl;
int g360,centr,pwm = 0;
boolean calibrate = false;

volatile unsigned int tachBuf; 

unsigned long tachValue; 

//ISR(TIMER1_CAPT_vect) 
//{ 
//  TCNT1 = 0; 
//  tachBuf = ICR1 + 120; 
//} 


void setup()
{
    pinMode(laserGndpin, OUTPUT);
    digitalWrite(laserGndpin, LOW);
    pinMode(laserPWMpin, OUTPUT);
  //digitalWrite(laserPWMpin, LOW);
analogWrite(laserPWMpin, pwm);
  filtercoef = 0.0001;
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
//  pinMode(Soundpin, OUTPUT);

  //Sound();
 // initfreqmeter();
}


void TIM_Init(void){
  TIMSK1=(1<<ICIE1);
  TCCR1A=(0<<COM1A1)|(0<<COM1A0)|(0<<WGM11)|(0<<WGM10); 
  TCCR1B=(1<<ICNC1)|(1<<ICES1)|(0<<WGM13)|(0<<WGM12)|(0<<CS12)|(0<<CS11)|(1<<CS10); 
  TCNT1 = 0; 
}

void initfreqmeter(void){

  pinMode(8, INPUT);
  //Вход частотомера для импульсов TTL
//  pinMode(6, OUTPUT);
  //выход ШИМ для тестирования частотомера 
//  TCCR0B = TCCR0B & 0b11111000 | 1;
  //частота ШИМ 62500Гц analogWrite(6, 128); 
//  analogWrite(6, 128); 
  //Запустить ШИМ 
//  digitalWrite(8, HIGH); 
  // включить подтяжку входа 
  //Serial.begin(115200); 
  TIM_Init();
  //_delay_ms(300); 
}

unsigned long mesurefreq(void){
cli(); 
tachValue = 16000000/tachBuf;
sei();
return tachValue; 
}

void freqMeter(void){
  unsigned long frq = mesurefreq();
  printdouble((double)(frq), 1, 4, 1, RED, BLACK);
  printLCD("Hz", 8, 4, 1, GREEN, BLACK);
  printdouble((double)tachBuf, 1, 5, 1, BLUE, BLACK);
}


void mydelay(long mili){
  int i;
for(;mili--;)i++;

}

void mytone(int vol,unsigned int freq,unsigned int len){
  boolean b = false;
unsigned int l = len;
  cli(); 
  for(;l>0;l--)
  for(;len>0;len--){
    
    b = !b;
    //analogWrite(Soundpin, b?vol:0);
//      digitalWrite(Soundpin, b);
    mydelay(freq);
  }
  //analogWrite(Soundpin, 0);
//  digitalWrite(Soundpin, LOW);
  sei();

}

void Sound(){
  /*mytone(255,5,100);
   mytone(255,17,100);
   mytone(255,9,100);
   mytone(255,40,100);
   */
}

void prepareDisp(){
  lcd.init_lcd();
  lcd.contrast_lcd(0x11c); //setting new contrast
  delay(100);
  //initLCD_LM15(32);
  Redraw0();
}


void Redraw0(){
  Redraw1();
  printLCD("min", 0, 1, 1, GREEN, BLACK);
  printLCD("max", 0, 2, 1, BLUE, BLACK);
  printLCD("C ", 10, 1, 1, GREEN, BLACK);
  printLCD("C ", 10, 2, 1, BLUE, BLACK);

}

void Redraw1(){

  lcd.clear_lcd(BLACK);
  printLCD("Temperature", 0, 0, 1, ORANGE, BLACK);
  printLCD("C ", 6, 3, 2, WHITE, BLACK);
}


void storeNewTemp(double t){
  lcd.pixel_lcd(pos, graph[pos], BLACK);
  graph[pos]=(unsigned char)(iamp);
  lcd.pixel_lcd(pos, graph[pos] - graph[0] + 40, RED);
  if(++pos>=ScreenSizeX){
    pos = 0;
    lcd.clear_lcd(BLACK);
  }
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
  printdouble(amp, 0, 3, 2, WHITE, BLACK);
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
  printdouble(lowtemp, 4, 1, 1, GREEN, BLACK);
  printdouble(hightemp, 4, 2, 1, BLUE, BLACK);

}

void printdouble(double t, unsigned char x, unsigned char y, unsigned int size, unsigned int color_font, unsigned int color){
  char buffer[50];
  dtostrf(t, 5, 1, buffer);
  printLCD(buffer, x, y, size, color_font, color);
}

void printLCD(char * t, unsigned char x, unsigned char y, unsigned int size, unsigned int color_font, unsigned int color){
  //setCursorLCD_LM15(y*14, x*8);
  //textLCD_LM15(t, color_font, color);
  lcd.str_lcd(t,x+1, y+1, size, color_font, color);
}

void oscilograph(void){

  for(pos = 0;pos < ScreenSizeX;pos++)  graph[pos]=analogRead(1);
  lcd.clear_lcd(BLACK);
  for(pos = 0;pos < ScreenSizeX;pos++) {
    lcd.pixel_lcd(pos,((graph[pos]-graph[0])>>3)+40, GREEN);

  }
}

void oscilograph1(void){


  int i,u = 0;
  for(i = 0;i < 8;i++) u += analogRead(1);
  u = (u >> 3) - centr;
  double acc = (double)u / 5.0;
  ang+=acc;
  //  printLCD("calibrating...", 0, 0, 1, ORANGE, BLACK);

  printdouble(acc, 1, 0, 2, WHITE, BLACK);
  printdouble(ang, 1, 1, 2, WHITE, BLACK);
  printdouble((double)centr, 4, 4, 1, RED, BLACK);
  printdouble((double)analogRead(1), 4, 5, 1, GREEN, BLACK);

}

void autocalibrate(void){
  printLCD("calibrating...", 0, 0, 1, ORANGE, BLACK);
  printdouble((double)analogRead(1), 1, 4, 1, WHITE, BLACK);
  int i,u,v = analogRead(1);
  //    for(i = 0;i < 100;i++) v=(v+analogRead(1));
  for(i = 0;i < 100;i++) if (v != analogRead(1)) return;
  calibrate = true;
  centr = v;
  g360 = centr<<1;
  lcd.clear_lcd(BLACK);

}

double smoothtest(double filt,double angle){

  double oldacc = smoothedVal;  
  for(pos = 0;pos < ScreenSizeX;pos++)  graph[pos]=analogRead(1);
  for(pos = 0;pos < ScreenSizeX;pos++)  {
    smooth(graph[pos]>>1, filt);
  }

  //printdouble(smoothedVal, 3, 0, 1, WHITE, BLACK);
  //printdouble(filt*1000.0, 3, 3, 1, WHITE, BLACK);
  oldacc -= smoothedVal;
  printdouble(oldacc, 2, 4, 1, BLUE | GREEN, BLACK);

  angl =  (int)(oldacc) + angl;
  if(angl>100)angl = 100;
  if(angl<-100)angl = -100;

  printdouble((double)angl, 3, 2, 1, RED, BLACK);
  return angle;
}


void screentest(void){
  //lcd.clear_lcd(BLACK);
  for(int y = 0;y < ScreenSizeY ; y++) 
    for(int x = 0;x < ScreenSizeX ; x++)
      lcd.pixel_lcd(x,y, (x ^ y)*analogRead(1) );

}

void demo(void){
  
  //----------------mandelbrot--------------
int px,py;// = 1234;
//int py = 456;
int pz=7,deep=29;//,dp=11;
float xf=1.8f,yf=0.001f;

  printLCD("start...", 0, 0, 1, ORANGE, BLACK);


	int xs,k,x,y,xi=0,yi=0,zi,zc,yc,xt,yt;

	float b,a,r,t,i,s;

	xs=ScreenSizeX;y=ScreenSizeY;yc=y/2;

	zc=pz*pz*pz;

	zi=zc+1;

	xi=xs/2;

	yi=yc;

	s=(float)zc/5.0f;

	xf=(float)px/s+xf;

	yf=(float)py/s+yf;

	px=xf*10000;py=yf*10000;

	px=0;py=0;


	for(;y--;b=(float)(y-yi)/zi-yf)for(x=xs;x--;){

			a=(float)(x-xi)/zi-xf;

			r=i=0;

			for(k=deep;--k&&4>r*r+i*i;i=t){	t=2.0f*r*i+b;r=r*r-i*i+a;}//main interation deep loop

//-----------------------------color pallete work-----------------------------
      lcd.pixel_lcd(x,y, k*1234);
			}
//}
  
    printLCD("end...", 0, 1, 1, ORANGE, BLACK);
mytone(255,100,55000);
//  for(int y = 0;y < ScreenSizeY ; y++) 
//    for(int x = 0;x < ScreenSizeX ; x++)

}


double smooth(int data, double filterVal){


  if (filterVal > 1){      // check to make sure param's are within range
    filterVal = .99;
  }
  else if (filterVal <= 0){
    filterVal = 0;
  }

  smoothedVal = (data * (1.0 - filterVal)) + (smoothedVal  *  filterVal);

  return smoothedVal;
}
//------------------------------------------------------------------------
void loop()
{
  iamp = analogRead(0);
  amp = Thermister(iamp);  // Read sensor
  printLowHigh();

  desktop();
  takeControlsKey();
  //  gtime++; 
}

void desktop(void){
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
    if(!calibrate){
      autocalibrate();
    }
    else{    
      oscilograph1();
    }
    //delay(300);
    break;
  case 4:
    screentest();
    break;
  case 5:
    ang = smoothtest(filtercoef,ang);

    break;
  case 6:
    freqMeter();
    break;
  case 7:
    demo();
    break;

  }


}


void takeControlsKey(void){
  if (irrecv.decode(&results)) {
    switch(results.value){
    case KEY_SHOT:
      Redraw1();
      mode = 1;
      Sound();
      break;
    case KEY_T:
      Redraw0();
      mode = 0;
      Sound();
      break;
    case KEY_REC:
      Redraw1();
      mode = 1;
      Sound();
      break;
    case KEY_SOURCE:
      mode= (mode+1)%8;
      pos = 0;
      lcd.clear_lcd(BLACK);      
      Sound();
      break;
    case KEY_STOP:
      mode= 2;
      Sound();
      break;
    case KEY_PAUSE:
      mode= 3;
      pos = 0;
      calibrate = false;
      ang = 0.0;
      lcd.clear_lcd(BLACK);
      Sound();
      break;
    case KEY_FULLSCR:
      mode= 4;
      pos = 0;
      lcd.clear_lcd(BLACK);
      Sound();      
      break;
    case KEY_POWER:
      Sound();
      mode= 5;
      ang = 1.0;
      lcd.clear_lcd(BLACK);
      break;
    case KEY_STEREO:
      mode= 6;
      lcd.clear_lcd(BLACK);
      break;
    case KEY_MUTE:
      mode= 7;
      lcd.clear_lcd(BLACK);
      break;
    case KEY_PLUS:
      filtercoef += 0.0001;
      break;
    case KEY_MINUS:
      filtercoef -= 0.0001;
      break;
    case KEY_UP:
      pwm+=10;
            //if(pwm>100)pwm = 100;
    analogWrite(laserPWMpin, pwm);
  printdouble((double)pwm, 1, 4, 1, GREEN, BLACK);

      break;
    case KEY_DOWN:
      pwm-=10;
      if(pwm<0)pwm = 0;
      analogWrite(laserPWMpin, pwm);
  printdouble((double)pwm, 1, 4, 1, GREEN, BLACK);

      break;
    }

    irrecv.resume(); // Receive the next value
  }
}








