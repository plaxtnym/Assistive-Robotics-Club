const int trigPin = 9;
const int echoPin = 10;


void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // Ensure trigger pin is low
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

}

void loop() {


  // Send a 10 microsecond high pulse to trigger the sensor
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the echo pin, which returns the duration of the pulse in microseconds
  long duration = pulseIn(echoPin, HIGH);

  // Calculate distance in centimetres
  int distance = duration * 0.034 / 2;

  // Print the distance in cm
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");


  delay(500); // Short delay to allow continuous measurement
}