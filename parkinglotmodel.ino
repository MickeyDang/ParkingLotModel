volatile int analogReader=0; //this is the variable that will be changed if an interrupt is detected
int interruptPin = 2;
int analogPin = A0;
const int MILLIS_PER_CLK = 4;
const float MILLIS_IN_SECOND = (float) 1000;
const int SCALE_FACTOR = 1000;
const long CLK_PER_OVF = 65535;

int stepNumber = 1;
bool isIn=0;
int parkingEntryDigits = 3;
bool parkingNumber[parkingEntryDigits] = {0,0,0}
bool inputValid = false;


class Time {
   public : volatile long overflowCount;
   public : volatile long registerCount = 0;

   public : bool isEarlier(Time t) {
      return (this->overflowCount * CLK_PER_OVF + this-> registerCount) < (t.overflowCount * CLK_PER_OVF + t.registerCount); 
   }
};


struct Spot {
  int index;
  bool isOccupied;
  Time startTime;
};

struct Node {
   Node* nextNode;
   Node* prevNode;
   Time timeToFire;
   int index;
};

class LinkedList {
  public : Node* head; 

  void modifyList(Node* newNode, Node* focusNode) {
    if (focusNode->timeToFire.isEarlier(newNode->timeToFire)) {
        newNode->prevNode = focusNode->prevNode;
        focusNode->prevNode->nextNode = newNode;
        focusNode->prevNode = newNode;
        newNode->nextNode = focusNode;
    } else {
      modifyList(newNode, focusNode->nextNode);
    }
  }

  void deleteHead() {
      
     head = head->nextNode;
     delete head->prevNode;
        
  }
  
};


LinkedList blinkQueue;

volatile long overflow_count;
volatile bool blinkLED;


const int SIZE = 7;
Spot spots[SIZE];


void setup() {
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(interruptPin),pauseCount,RISING); //Interrupt 0 is mapped to pin 2, signal an interrupt on a change to pin 2

  cli();

  blinkQueue = LinkedList();
  for (int i = 0; i < SIZE; i++) {
      spots[i] = Spot();
      spots[i].index = i;
      spots[i].isOccupied = false;
      spots[i].startTime = Time();
  }

  //enable overflow, compareA, compareB

  TIMSK1 = 0x7;

  TCCR1A &= ~(1 << 0);
  TCCR1A &= ~(1 << 1);
  TCCR1B &= ~(1 << 3);
  TCCR1B &= ~(1 << 2);
  TCCR1B |= (1 << 1);
  TCCR1B |= (1 << 0);
  
  OCR1B = 50000;
  OCR1A = 40000;
  
  TCNT1 = 0;
  sei();
  
}

void loop() {
  // put your main code here, to run repeatedly
  analogReader = analogRead(analogPin); // reads analogPin (A0)
  //POLL FOR TIMERS?
  

   if (blinkLED) {
      //find the LED Pin
      //set to high
      //delay
      //set to low
      //change linkedlist
   }
   
   float seconds = (double) getTimeMillis() / (float) 1000;
   Serial.println("seconds since start: " + String(seconds));
   delay(1000);

}

ISR (TIMER1_OVF_vect) {
  overflow_count++;
}

ISR (TIMER1_COMPA_vect) {
  
  int i = blinkQueue.head->index;
  blink(i);
  Time nextTime = convertIndexToTime(i);
  
  Node* n = new Node();
  n->timeToFire = nextTime;
  n->index = i;
  
  blinkQueue.modifyList(n, blinkQueue.head);
  blinkQueue.deleteHead();
  
}

ISR (TIMER1_COMPB_vect) {
//  logic here
}

Time convertIndexToTime(int i) {
  Time aTime = Time();
  return aTime;
}

void blink (int id) {
  
}

void onSpotSelected(int index, int rC) {

    Time tS = Time();
    tS.overflowCount = overflow_count;
    tS.registerCount = rC;
    
    spots[index].startTime = tS;
    spots[index].isOccupied = true;   

    
}

void onSpotRemoved(int index, int rC) {
  
    Time tF = Time();
    tF.overflowCount = overflow_count;
    tF.registerCount = rC;

    Spot s = spots[index];
    Time tS = s.startTime;

    long timeMillis = convertClockToMillis(differenceInTimeCLK(tS, tF));
    float seconds = (double) timeMillis / MILLIS_IN_SECOND;

    spots[index].isOccupied = false; // removes
    
    //output the time in seconds
    //calculate price to charge
    //output price
}

long getTimeMillis() {
  
   long remainder = TCNT1;
   long clockCycle = overflow_count * CLK_PER_OVF + remainder;
   
   return convertClockToMillis(clockCycle);
}

long convertClockToMillis(long clk) {
   long myMillis =  clk * MILLIS_PER_CLK / SCALE_FACTOR;
   return myMillis;
}

long differenceInTimeCLK(Time t1, Time t2) {

    long oc = t2.overflowCount - t1.overflowCount;
    long rc = t2.registerCount - t1.registerCount;
    return oc * CLK_PER_OVF + rc;
}

//program interrupt logic for overflow

//program button handling


int getParkingNumber(){
   return parkingNumber[0] + parkingNumber[1] * 2  + parkingNumber[2] * 4;
}


void pauseCount(){
  Serial.println("Interrupt triggered");

  if(analogReader < 5 && analogReader > 0){// 1 - 4
   //weird...

  }else if(analogReader == 0){
   //weird...
   
  }else if(analogReader < 10 && analogReader > 0){ // 9 - 1

    parkingNumber[0] = !parkingNumber[0];
    stepNumber = 2;
    
  }else if(analogReader < 40 && analogReader > 32){ // 39-33

    parkingNumber[1] = !parkingNumber[1];
    stepNumber = 2;

  }else if(analogReader < 51 && analogReader > 43){ // 50-44
   
    parkingNumber[2] = !parkingNumber[2];
    stepNumber = 2;

  }else if(analogReader < 185 && analogReader > 175){ // 184-176 //IN IN IN IN 
    if(stepNumber == 2){
      isIn = true;
      stepNumber = 3;  
      
    }
    
  }else if(analogReader < 252 && analogReader > 240){ // 251-241  //OUT OUT OUT 
    if(stepNumber == 2){
      isIn = false;
      stepNumber = 3;  
    }
    
  }else if(analogReader < 860 && analogReader > 850){ // 859-851
    if(stepNumber == 3){
      if(isIn){
        onSpotSelected(getParkingNumber(), TCNT1);  
      }else{
        onSpotRemoved(getParkingNumber(), TCNT1);  
      }
      //resets:
      stepNumber = 1;
    }
  }else if(analogReader < 911 && analogReader > 899){ // 910-900
    
  }else{ // 1024
    //normal do nothing
  }
  
}

