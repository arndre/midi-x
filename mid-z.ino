#include <MIDI.h>

// Created and binds the MIDI interface to the default hardware Serial port
MIDI_CREATE_DEFAULT_INSTANCE();
int MIDI_CHANNEL = 16;

int isRunning = -1;

int ledOutput[] = {9, 10, 11, 12, 13};

int tempo = 100;

int min = 30;   //midi note
int max = 50;   //midi note

bool debug = false;

int min_default = 30;   //midi note (incl. empty note = 34, 35)
int max_default = 53;   //midi note (incl. empty note = 48, 51)
int max_extended = 64;   //midi note, empty note = 63, 64

int repeats = 2;

int velocity = 69;

int button1 = 0;   // sequence mode
int button2 = 0;   // sequence mode
int button3 = 0;   // repeats [1 or 2]
int button4 = 0;   // extend note range

int arpMode = 0; // analog 6

const int seqSteps = 8;
int sequences[][seqSteps] = {
  //  1     2     3     4     5     6     7    8
  {A3,   A2,   A1,   A0,   -1,   -1,   -1,  -1},
  {A0,   A1,   A2,   A3,   -1,   -1,   -1,  -1},
  {A3,   A3,   A2,   A1,   A1,   A0,   A0,  A0},
  {A0,   A1,   A2,   A3,   A2,   A1,   -1,  -1},
};

int currentInput = -1;

const int arpSteps = 8;
int arp[][arpSteps] = {
  //  1     2     3     4     5     6     7     8
  { 0,   -1,   -1,   -1,   -1,   -1,   -1,   -1  },
  { 0,   12,   -1,   -1,   -1,   -1,   -1,   -1  },
  { 0,    3,    7,   12,   -1,   -1,   -1,   -1  },
  { 0,    3,    7,   12,   15,   12,   7,     3   },

  { 0,    3,    7,    3,   -1,   -1,   -1,   -1  },
  { 0,    3,    0,    7,   3,    12,    7,    3  },
};


// this is not binary, thus is the led pattern for each note
int ledNumPattern[][5] = {
  {0, 0, 0, 0, 0},   // 0

  {1, 0, 0, 0, 0},   // 1
  {1, 1, 0, 0, 0},   // 2
  {1, 1, 1, 0, 0},   // 3
  {1, 1, 1, 1, 0},   // 4
  {1, 1, 1, 1, 1},   // 5

  {0, 1, 1, 1, 1},   // 6
  {0, 0, 1, 1, 1},   // 7
  {0, 0, 0, 1, 1},   // 8
  {0, 0, 0, 0, 1},   // 9
  {0, 0, 0, 0, 0},   // 10


  {0, 0, 1, 0, 0},   // 11
  {0, 1, 0, 1, 0},   // 12
  {1, 0, 0, 0, 1},   // 13
  {0, 1, 1, 1, 0},   // 14
  {1, 1, 1, 1, 1}   // 15

};



void setup() {

  for (int i = 0; i < 5; i++) {
    pinMode(ledOutput[i], OUTPUT);
  }


  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP );

  if (debug) {

    Serial.begin(9600);
  } else {
    Serial.begin(31250);
  }



  min = min_default;
  max = max_default;


}

void loop() {


  getTempo();

  readButtons();

  // if not button is active, but system was running before, send stop
  if (needToStop()) {
    isRunning = false;
    sendStop();
    silenceAllNotes();
  }

  // if at least one button is active, but system was not running before, send start
  if (needToStart()) {
    isRunning = true;
    sendStart();
    silenceAllNotes();
  }


  // play mode
  if (isRunning == 1) {

    arpMode = analogRead(A6);
    arpMode = map(arpMode, 0, 1020, 0, 5);

    if (button3 == 1) {
      repeats = 2;
    }
    else {
      repeats = 1;
    }

    if (button4 == 1) {
      max = max_extended;
    }
    else {
      max = max_default;
    }

    if (button1 && !button2) {
      Play(0);
    }

    if (button1 && button2) {
      Play(1);
    }




  }
  else {
    delay(100);
  }


}

void readButtons() {
  button1 = digitalRead(2);
  button2 = digitalRead(3);
  button3 = digitalRead(4);
  button4 = digitalRead(5);
}


bool needToStop() {
  if (button1 == 0 && isRunning) {
    return true;
  }
  return false;
}

bool needToStart() {
  if (button1 == 1 && isRunning == false) {
    return true;
  }
  return false;
}

void Play(int sequence) {

  for (int i = 0; i < seqSteps; i++) {
    int input = sequences[sequence][i];
    if (input != -1 ) {
      currentInput = input;
      PlayIt(input);
    }
  }
}


void showNumberLed() {

  if (currentInput <= 0) {
    return;
  }

  int value = analogRead(currentInput);
  value = map(value, 0, 1020, min, max) - (min + 3); // +3 to start with 0 0 0 0 0

  for (int i = 0; i < 5; i++) {

    if (ledNumPattern[value % 15][i] == 1) {
      digitalWrite(ledOutput[i], true);
    } else {
      digitalWrite(ledOutput[i], false);
    }
  }
}



void PlayIt(int input) {
  int value = analogRead(input);
  value = map(value, 0, 1020, min, max);

  if ( value >= max - 2) {

    // skip
    return;
  }


  if (value <= min + 2 ) {



    for (int i = 0; i < arpSteps; i++) {
      if (arp[arpMode][i] >= 0) {
        sendClockSignalAndNoteDuration(12, 12);
      }
    }
    return;
  }


  // play normal
  if (value < max && value > min + 2)
  {

    if (debug) {

      Serial.print(input);
      Serial.print("\t");
      Serial.print(value);
      Serial.println();
    }

    for (int i = 0; i < arpSteps; i++) {
      if (arp[arpMode][i] >= 0) {
        playNote(value + arp[arpMode][i], velocity);
      }
    }
    return;
  }



}

void getTempo() {
  tempo = analogRead(A4);
  tempo = map(tempo, 0, 1023, 1300, 5);
}

void sendClockSignalAndNoteDuration(int parts, int total) {

  for (int i = 0; i < parts; i++) {
    showNumberLed();

    delay(tempo / total);
    sendClock();
  }
}

void playNote(int pitch, int velocity) {


  //  if(debug) {
  //
  //    Serial.print("note:");
  //    Serial.print(pitch);
  //    Serial.println();
  //  }

  getTempo();
  button2 = digitalRead(3);
  int gate = analogRead(A5);

  gate = map(gate, 0, 1020, 2, 9) ;

  for (int i = 0; i < repeats; i++) {

    //sendCmd(0x9F, pitch, velocity);  // send the actual note

    MIDI.sendNoteOn(pitch, 127, MIDI_CHANNEL);

    gate = gate / repeats;

    sendClockSignalAndNoteDuration(gate, 12); // midi clock

    MIDI.sendNoteOn(pitch, 0, MIDI_CHANNEL);
    //sendCmd(0x9F, pitch, 0x00);  // send note off
    sendClockSignalAndNoteDuration(12 / repeats - gate, 12); // midi clock
  }
}


void sendClock() {

//  //    Serial.write(248);
//  unsigned long currentMillis = millis();
//  if (currentMillis - prevMillis > interval) {
//    // save the last time.
//    prevMillis = currentMillis;
//    MIDI.SendRealtime(clock);

  
    MIDI.sendRealTime(midi::Clock);


  }

  void sendStart() {
    if (debug == false)
      Serial.write(250);
  }

  void sendStop() {
    if (debug == false)
      Serial.write(252);
  }

  void silenceAllNotes() {
    for (int i = 0; i < 127; i++) {
      MIDI.sendNoteOff(i, 0, MIDI_CHANNEL);     // Stop the note
      //sendCmd(0x9F, i, 0);  // send note off to all notes
    }
  }


  // plays a MIDI note. Doesn't check to see that cmd is greater than 127, or that
  // data values are less than 127:
  void sendCmd(int cmd, int pitch, int velocity) {
    if (debug == false)  {

      Serial.write(cmd);
      Serial.write(pitch);
      Serial.write(velocity);
    }
  }

