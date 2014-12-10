int inSensor = 5;
int ejSensor = 2;
int engineLeft = 3;
int engineRight = 11;
int engineSensor = 7; // this doesn't actually seem to work, unfortunately
int led = 13;
int turnSpeed = 255;
enum tstate {
  STOP = 0,
  EJECT = 1,
  INSERT = 2,
  PARK = 3,
} target = INSERT;
enum cstate {
  STOPPED = 0,
  EJECTED = 1,
  INSERTED = 2,
  PARKED = 3,
  MOVING_LEFT = 4,
  MOVING_RIGHT = 5,
  ERROR = 9,
} curr = STOPPED;
void(*insert)();
void(*eject)();
int printed = 0;

void printStatus() {
  switch(curr) {
    case STOPPED:
      Serial.println("stopped");
      break;
    case EJECTED:
      Serial.println("ejected");
      break;
    case INSERTED:
      Serial.println("inserted");
      break;
    case PARKED:
      Serial.println("parked");
      break;
    case MOVING_LEFT:
      Serial.println("moving left");
      break;
    case MOVING_RIGHT:
      Serial.println("moving right");
      break;
    case ERROR:
      Serial.println("error");
      break;
    default:
      Serial.println("unknown state");
      break;
  }
}

void turnLeft() {
  analogWrite(engineRight, 0);
  analogWrite(engineLeft, turnSpeed);
  curr = MOVING_LEFT;
}

void turnRight() {
  analogWrite(engineLeft, 0);
  analogWrite(engineRight, turnSpeed);
  curr = MOVING_RIGHT;
}

void stopMove() {
  analogWrite(engineLeft, 0);
  analogWrite(engineRight, 0);
  curr = STOPPED;
}

void setup() {
  pinMode(inSensor, INPUT_PULLUP);
  pinMode(ejSensor, INPUT_PULLUP);
  pinMode(led, OUTPUT);
  analogWrite(engineLeft, 0);
  analogWrite(engineRight, 0);
  while(!Serial) {
    ;
  }
  Serial.begin(9600);
  turnLeft();
  while(curr != INSERTED && curr != EJECTED) {
    if(digitalRead(ejSensor) == HIGH) {
      stopMove();
      curr = EJECTED;
      eject = turnLeft;
      insert = turnRight;
    }
    if(digitalRead(inSensor) == HIGH) {
      stopMove();
      curr = INSERTED;
      insert = turnRight;
      eject = turnLeft;
    }
  }
  Serial.println("READY.");
}

void loop() {
  int inSensorState;
  int ejSensorState;
  int c;

  if(target != curr) {
    digitalWrite(led, HIGH);
    printed = 0;
    switch(target) {
      case STOP:
        stopMove();
        break;
      case EJECT:
        eject();
        break;
      case INSERT:
        insert();
        break;
      case PARK:
        if(curr == INSERTED) {
          eject();
        }
        if(curr == EJECTED) {
          insert();
        }
        break;
    }
    if(curr == ERROR) {
      Serial.println("ERROR!");
    }
  } else {
    digitalWrite(led, LOW);
    if(!printed) {
      printed = 1;
      printStatus();
    }
  }

  inSensorState = digitalRead(inSensor);
  ejSensorState = digitalRead(ejSensor);
  if(ejSensorState == HIGH && target == EJECT) {
    stopMove();
  }
  if(inSensorState == HIGH && target == INSERT) {
    stopMove();
  }
  if(inSensorState == LOW && ejSensorState == LOW && target == PARK) {
    stopMove();
  }
  if(curr == STOPPED) {
    if(inSensorState == HIGH) {
      curr = INSERTED;
    } else if(ejSensorState == HIGH) {
      curr = EJECTED;
    } else {
      curr = PARKED;
    }
  }
  if(target == STOP) {
    target = (tstate)curr;
  }
  if(Serial.available() > 0) {
    printed = 0;
    c = Serial.read();
    if(c <0x60) {
      delay(200);
    }
    c |= 0x20;
    switch(c) {
      case 'i':
        target = INSERT;
        break;
      case 'e':
        target = EJECT;
        break;
      case 's':
        target = STOP;
        break;
      case 'p':
        target = PARK;
        break;
      case 'w':
        printStatus();
        printed=1;
      default:
        break;
    }
  }
}
