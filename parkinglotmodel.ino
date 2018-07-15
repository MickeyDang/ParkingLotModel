//hardware variables
volatile int analogReader = 0; //this is the variable that will be changed if an interrupt is detected
int interruptPin = 2;
int analogPin = A0;

//constants
const int MILLIS_PER_CLK = 4;
const float MILLIS_IN_SECOND = (float) 1000;
const int SCALE_FACTOR = 1000;
const long CLK_PER_OVF = 65535;
const int DELAY_TIME = 250;

//dashboard and onboaring controls
volatile int stepNumber = 1;
volatile bool isIn = 0;
const int PARKING_ENTRY_DIGITS = 3;
volatile bool parkingNumber[PARKING_ENTRY_DIGITS] = {0,0,0};
volatile bool inputValid = false;


//defined classes and structures
class Time {
   public : volatile long overflowCount;
   public : volatile long registerCount = 0;

   public : bool isEarlier(Time t) {
      return (this->overflowCount * CLK_PER_OVF + this-> registerCount) < (t.overflowCount * CLK_PER_OVF + t.registerCount); 
   }

   public : String getTime() {
      return "OVF: " + String(overflowCount) + "and " + String(TCNT1) + " cycles";
   }
};


struct Spot {
  int index;
  int pinId;
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
  public : Node* head = NULL; 

   void modifyList(Node* newNode, Node* focusNode) {
    if (head == NULL) {
       head = newNode;
    } else if (!focusNode->timeToFire.isEarlier(newNode->timeToFire)) {
        newNode->prevNode = focusNode->prevNode;
       
        //handles head edge case
        if (focusNode->prevNode != NULL) {
            focusNode->prevNode->nextNode = newNode;
        } else {
            head = newNode;
        }
        
        focusNode->prevNode = newNode;
        newNode->nextNode = focusNode;
    } else if (focusNode->nextNode == NULL) {
        focusNode->nextNode = newNode;
        newNode->prevNode = focusNode;
    }else {
      modifyList(newNode, focusNode->nextNode);
    }
  }

  void deleteHead() {
     if (head->nextNode != NULL) {
        head = head->nextNode;
        delete head->prevNode;
     } else {
         head = NULL;
     } 
  }
};


//global variables
LinkedList blinkQueue;

//state variables
volatile long overflow_count;
volatile bool blinkLED;
volatile int pinNumber;

//parking lot model
const int SIZE = 7;
Spot spots[SIZE];

void setup() {
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(interruptPin), onButtonClicked, RISING); //Interrupt 0 is mapped to pin 2, signal an interrupt on a change to pin 2

  //enable all pins we will be writing to
  
  cli();

  blinkQueue = LinkedList();
  for (int i = 0; i < SIZE; i++) {
      spots[i] = Spot();
      spots[i].index = i;
      spots[i].isOccupied = false;
      spots[i].startTime = Time();
  }

  //connect each spot to their arbitrary led pin id
  spots[0].pinId = 13;
  spots[1].pinId = 12;
  spots[2].pinId = 11;
  spots[3].pinId = 10;
  spots[4].pinId = 9;
  spots[5].pinId = 7;
  spots[6].pinId = 6;

  //enable overflow, compareA, compareB
  TIMSK1 = 0x7;

  TCCR1A &= ~(1 << 0);
  TCCR1A &= ~(1 << 1);
  TCCR1B &= ~(1 << 3);
  TCCR1B &= ~(1 << 2);
  TCCR1B |= (1 << 1);
  TCCR1B |= (1 << 0);
 
  OCR1A = 40000;
  
  TCNT1 = 0;
  sei();
  
}

void loop() {
  // put your main code here, to run repeatedly
  
//  if (blinkLED && pinNumber != -1) {
//     blink(pinNumber);
//     resetReadyToBlink();
//  }
   //debug
}

ISR (TIMER1_OVF_vect) {
  overflow_count++;
}

ISR (TIMER1_COMPA_vect) {

  if (spots[blinkQueue.head->index].isOccupied) {
    if (overflow_count >= blinkQueue.head->timeToFire.overflowCount) {
      int i = blinkQueue.head->index;
      Time nextTime = convertIndexToTime(i);
      
      Node* n = new Node();
      n->timeToFire = nextTime;
      n->index = i;
      
      blinkQueue.modifyList(n, blinkQueue.head);
      blinkQueue.deleteHead();
    
      OCR1A = blinkQueue.head->timeToFire.registerCount;
      setReadyToBlink(spots[i].pinId);
    }
  } else {
    blinkQueue.deleteHead();
    OCR1A = blinkQueue.head->timeToFire.registerCount;
  }
  
     
}

ISR (TIMER1_COMPB_vect) {
//  logic here
}

Time convertIndexToTime(int i) {

  //convert index to next blink in millis
  //#1 goes 1 per seconds, #2 goes 1 per 2 seconds...
  long millisecondsLater = (i + 1) * MILLIS_IN_SECOND;
  long clkLater = millisecondsLater * MILLIS_PER_CLK / SCALE_FACTOR;

  //will automatically floor the result
  long ovfs = clkLater / CLK_PER_OVF; 
  long remainder = clkLater % CLK_PER_OVF;

  //assign to time object
  Time aTime = Time();
  aTime.overflowCount = ovfs;
  aTime.registerCount = remainder;
  
  return aTime;
}

void blink (int pinId) {
   digitalWrite(pinId, HIGH);
   delay(DELAY_TIME);
   digitalWrite(pinId, LOW);
   
}

void setReadyToBlink(int pinId) {
    blinkLED = true;
    pinNumber = pinId;
}

void resetReadyToBlink() {
    blinkLED = false;
    pinNumber = -1;
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

int getParkingNumber(){
   return parkingNumber[0] + parkingNumber[1] * 2  + parkingNumber[2] * 4;
}

//testing functions below
void test(int value) {
  if (value <= 5) {
    Serial.println("btn 2^0");
  } else if(value < 10 && value > 6){ // 9 - 1
    Serial.println("btn 2^1");
  } else if(value < 40 && value > 32){ // 39-33
    Serial.println("btn 2^2");
  } else if(value < 51 && value > 43){ // 50-44
    Serial.println("btn in");
  }else if(value < 185 && value > 175){ // 184-176 
    Serial.println("btn out");
  } else if(value < 252 && value > 240){ // 251-241  
    Serial.println("btn go");
  } else if(value < 860 && value > 850){ // 859-851
    Serial.println("btn cancel. Not implemeneted");
  } else if(value < 911 && value > 899){ // 910-900
    //not implemented
  } else { // 1024
    //normal do nothing
    Serial.println("default value. It can't find a button");
  }
}

void onButtonClicked(){
  analogReader = analogRead(analogPin);
  int value = analogReader;
  Serial.println("interrupt found voltage of: " + String(value));
//  test(value);

//    if(analogReader < 10 && analogReader > 0){ // 9 - 1
//    //button press on first button
//    parkingNumber[0] = !parkingNumber[0];
//    stepNumber = 2;
//    
//  } else if(analogReader < 40 && analogReader > 32){ // 39-33
//    //button press on second button
//    parkingNumber[1] = !parkingNumber[1];
//    stepNumber = 2;
//
//  } else if(analogReader < 51 && analogReader > 43){ // 50-44
//    //button press on third button
//    parkingNumber[2] = !parkingNumber[2];
//    stepNumber = 2;
//
//  }else if(analogReader < 185 && analogReader > 175){ // 184-176 
//    //in button press
//    if(stepNumber == 2){
//      isIn = true;
//      stepNumber = 3;  
//    }
//    
//  } else if(analogReader < 252 && analogReader > 240){ // 251-241  
//    //out button press
//    if(stepNumber == 2){
//      isIn = false;
//      stepNumber = 3;  
//    }
//    
//  } else if(analogReader < 860 && analogReader > 850){ // 859-851
//    //go button press
//    if(stepNumber == 3){
//      if(isIn){
//        onSpotSelected(getParkingNumber(), TCNT1);  
//      }else{
//        onSpotRemoved(getParkingNumber(), TCNT1);  
//      }
//      //resets:
//      stepNumber = 1;
//    }
//    
//  } else if(analogReader < 911 && analogReader > 899){ // 910-900
//    //todo add cancel button 
//  } else { // 1024
//    //normal do nothing
//  }
  
}

