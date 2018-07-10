class Time {
  public : static volatile long overflow_count;
  public : volatile int register_count = 0;
};

struct Spot {
  int index;
  bool isOccupied;
  Time startTime;
};

class Duration {
   private : Time start;
   
   public : Duration(Time start) {
      this-> start = start;
   }

   public : Time getStart() {
      return start;
   }

    
};

const int SIZE = 7;

Spot spots[SIZE];

void setup() {
  // put your setup code here, to run once:

  for (int i = 0; i < SIZE; i++) {
      spots[i] = Spot();
      spots[i].index = i;
      spots[i].isOccupied = false;
      spots[i].startTime = Time();
  }

  //enable the overflow counter interrupt

}

void loop() {
  // put your main code here, to run repeatedly
  //POLL FOR TIMERS?
}


void onSpotSelected(int index) {

    Time t = Time();
    //assign time.register count to the current register count
    spots[index].startTime = t;
    spots[index].isOccupied = true;

    
}

void onSpotRemoved(int index) {

    Time t = Time();
    //assign current time to t
    //calculate difference in time between 2 Time structs
    //calculate price to charge
    //output price
}

long getTimeMillis() {

   long l = Time::overflow_count;
   
}



//program interrupt logic for overflow

//program button handling


