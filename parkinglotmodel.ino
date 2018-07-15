
const int MILLIS_PER_CLK = 4;
const float MILLIS_IN_SECOND = (float) 1000;
const int SCALE_FACTOR = 1000;
const long CLK_PER_OVF = 65535;


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
};


LinkedList blinkQueue;

volatile long overflow_count;
volatile bool blinkLED;


const int SIZE = 7;
Spot spots[SIZE];


void setup() {
  
  Serial.begin(9600);
 
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
//   logic here
}

ISR (TIMER1_COMPB_vect) {
//  logic here
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


