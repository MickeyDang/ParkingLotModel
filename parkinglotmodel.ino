
//TODO cancel button
//TODO LCD screen
//TODO make better serial statements (time stamps)

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
volatile bool inputValid = 0;


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

  for (int i = 0; i < SIZE; i++) {
      spots[i] = Spot();
      spots[i].index = i;
      spots[i].isOccupied = 0;
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
  
  //enable all pins we will be writing to
  pinMode(spots[0].pinId, OUTPUT);
  pinMode(spots[1].pinId, OUTPUT);
  pinMode(spots[2].pinId, OUTPUT);
  pinMode(spots[3].pinId, OUTPUT);
  pinMode(spots[4].pinId, OUTPUT);
  pinMode(spots[5].pinId, OUTPUT);
  pinMode(spots[6].pinId, OUTPUT);
  
  cli();

  blinkQueue = LinkedList();

  //enable overflow, compareA, compareB
  TIMSK1 = 0x7;

  TCCR1A &= ~(1 << 0);
  TCCR1A &= ~(1 << 1);
  TCCR1B &= ~(1 << 3);
  TCCR1B &= ~(1 << 2);
  TCCR1B |= (1 << 1);
  TCCR1B |= (1 << 0);
 
  OCR1A = 30000;
  
  TCNT1 = 0;
  sei();
  
}

void loop() {
  // put your main code here, to run repeatedly
  if (blinkLED && pinNumber != -1) {
     doBlink(pinNumber);
  }
}

void doBlink (int pinId) {
   //DELAY_TIME
   Serial.println("begin blink");
   resetReadyToBlink();
   digitalWrite(pinId, HIGH);
   delay(100);
   digitalWrite(pinId, LOW);
   delay(100);
   Serial.println("done blink");
}

ISR (TIMER1_OVF_vect) {
  overflow_count++;
}

ISR (TIMER1_COMPA_vect) {

  if (blinkQueue.head != NULL && spots[blinkQueue.head->index].isOccupied == 1) {
    
    if (overflow_count >= blinkQueue.head->timeToFire.overflowCount) {
      Serial.println(blinkQueue.head->index);
      
      int index = blinkQueue.head->index;
      Time* nextTime = convertIndexToTime(index);
      
      Node* n = new Node();
      n->timeToFire = (*nextTime);
      n->index = index;
      
      blinkQueue.modifyList(n, blinkQueue.head);
      blinkQueue.deleteHead();
    
      OCR1A = blinkQueue.head->timeToFire.registerCount;
      setReadyToBlink(spots[index].pinId);

    }
  } else if (blinkQueue.head != NULL){
    blinkQueue.deleteHead();
    OCR1A = blinkQueue.head->timeToFire.registerCount;
  }   
}

ISR (TIMER1_COMPB_vect) {
//  logic here
}

Time* convertIndexToTime(int i) {

  //convert index to next blink in millis
  //#1 goes 1 per seconds, #2 goes 1 per 2 seconds...
  long millisecondsLater = getTimeMillis() + (i + 1) * MILLIS_IN_SECOND;
  long clksLater = millisecondsLater * SCALE_FACTOR / MILLIS_PER_CLK;

  long ovfs = clksLater / CLK_PER_OVF; //will automatically floor the result
  long remainder = clksLater % CLK_PER_OVF;

 
  //assign to time object
  Time* aTime = new Time();
  aTime->overflowCount = ovfs;
  aTime->registerCount = remainder;
  
  return aTime;
}


void setReadyToBlink(int pinId) {
    blinkLED = 1;
    pinNumber = pinId;
}

void resetReadyToBlink() {
    blinkLED = 0;
    pinNumber = -1;
}

void onSpotSelected(int index, int rC) {
    
    Time tS = Time();
    tS.overflowCount = overflow_count;
    tS.registerCount = rC;
    
    spots[index].startTime = tS;
    spots[index].isOccupied = 1;   

    Time* nextTime = convertIndexToTime(index);

    Serial.println(nextTime->overflowCount);
    Serial.println(overflow_count);
    
    Serial.println("line");
    Node* n = new Node();
    n->timeToFire = (*nextTime);
    n->index = index;

    blinkQueue.modifyList(n, blinkQueue.head);
}

void onSpotRemoved(int index, int rC) {
  
    Time tF = Time();
    tF.overflowCount = overflow_count;
    tF.registerCount = rC;

    Spot s = spots[index];
    Time tS = s.startTime;

    long timeMillis = convertClockToMillis(differenceInTimeCLK(tS, tF));
    float seconds = (double) timeMillis / MILLIS_IN_SECOND;

    spots[index].isOccupied = 0; // removes

    Serial.println("seconds in spot: " + String(seconds));
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
   int p1 = parkingNumber[0] ? 1 : 0;
   int p2 = parkingNumber[1] ? 1 : 0;
   int p3 = parkingNumber[2] ? 1 : 0;
   return p1 + p2 * 2  + p3 * 4;
}


void test(int value){
 Serial.println("interrupt found value of: " + String(value));
 
 Serial.println(value);
  if(value < 1024 && value > 1020){// 1 - 4
     Serial.println("2^0");
  }else if(value < 935 && value > 928){ // 9 - 1
     Serial.println("2^1");
  }else if(value < 860 && value > 850){ // 39-33
     Serial.println("2^2");
  }else if(value < 800 && value > 770){ // 50-44
     Serial.println("IN");
  }else if(value < 740 && value > 720){ // 184-176
     Serial.println("OUT");
    
  }else if(value < 690 && value > 680){ // 251-241
      Serial.println("GO");
   
  }else if(value < 650 && value > 635){ // 859-851
      Serial.println("EXTRA BTN 1");
   
  }else if(value < 610 && value > 600){ // 910-900
      Serial.println("EXTRA BTN 2");
  }else{ // 1024
    //normal do nothing
  } 
}

void onButtonClicked(){
  
  int value = analogRead(analogPin);
  
//  test(value);

  if(value < 1024 && value > 1020){// 1 - 4
    Serial.println("2^0 is toggled");
    //button press on first button
    parkingNumber[0] = !parkingNumber[0];
    stepNumber = 2;
    
  }else if(value < 935 && value > 928){ // 9 - 1
     Serial.println("2^1 is toggled");
    //button press on second button
    parkingNumber[1] = !parkingNumber[1];
    stepNumber = 2;
    
  }else if(value < 860 && value > 850){ // 39-33
     Serial.println("2^2 is toggled");
    //button press on third button
    parkingNumber[2] = !parkingNumber[2];
    stepNumber = 2;
    
  }else if(value < 800 && value > 770){ // 50-44
    
    //in button press
    if(stepNumber == 2){

      Serial.println("in " + String(getParkingNumber()));
      isIn = true;
      stepNumber = 3;  
    }
    
  }else if(value < 740 && value > 720){ // 184-176
     Serial.println("out");
    //out button press
    if(stepNumber == 2){
      isIn = false;
      stepNumber = 3;  
    }
    
  }else if(value < 690 && value > 680){ // 251-241

    //go button press
    if(stepNumber == 3){
      if(isIn){
         Serial.println("onSpotSelected");
        onSpotSelected(getParkingNumber(), TCNT1);  
      }else{
        Serial.println("onSpotRemoved");
        onSpotRemoved(getParkingNumber(), TCNT1);  
      }
      //resets:
      stepNumber = 1;
      parkingNumber[0] = 0; 
      parkingNumber[1] = 0;
      parkingNumber[2] = 0;  
      isIn = 0;
      inputValid = 0;
    }
   
  }else if(value < 650 && value > 635){ // 859-851
//      Serial.println("EXTRA BTN 1");
   
  }else if(value < 610 && value > 600){ // 910-900
//      Serial.println("EXTRA BTN 2");
  }else{ // 1024
    //normal do nothing
  } 
}



