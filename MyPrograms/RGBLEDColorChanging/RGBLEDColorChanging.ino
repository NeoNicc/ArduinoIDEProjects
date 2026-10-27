#define BLUE 3
#define GREEN 5
#define RED 6

void setup() {
  // put your setup code here, to run once:
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);

  digitalWrite(RED, HIGH);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);
}

int redValue;
int greenValue;
int blueValue;

int delayTime = 100;

void loop() {
  // put your main code here, to run repeatedly:
  redValue = 255;
  greenValue = 0;
  blueValue = 0;

  for (int i = 0; i < 255; i++) {
    redValue -= 1;

    if(i % 5 == 0) {
      if(greenValue <= 150) {
        greenValue += 20;
      } 
    }

    blueValue += 1;

    analogWrite(RED, redValue);
    analogWrite(GREEN, greenValue);
    analogWrite(BLUE, blueValue);
    delay(delayTime);
  }

  for (int i = 0; i < 255; i++) {
    redValue += 1;

    greenValue -= 1;

    blueValue -= 1;

    analogWrite(RED, redValue);
    analogWrite(GREEN, greenValue);
    analogWrite(BLUE, blueValue);

    delay(delayTime);
  }
}
