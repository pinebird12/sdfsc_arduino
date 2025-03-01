#include "Queue.h"
using namespace std;

class HRMonitor {
public:

  int avgWindow;
  Queue<float> que;
  Queue<int> tmp;
  int inQueue = 0;
  int inputWire;
  float currentBPM = 0;
  float* windowBuffer;
  float conVal;

  HRMonitor(int bufferSize, int input) {
    avgWindow = bufferSize;
    que = Queue<float>(bufferSize);
    inputWire = input;
    windowBuffer = new float[bufferSize];
    for (int i = 0; i < bufferSize; i++){
      windowBuffer[i] = 0.0;
    }
    for (int i = 0; i < bufferSize; i++){
      Serial.println(windowBuffer[i]);
    }
    conVal = (1.0 / bufferSize);
  }

  ~HRMonitor() {
    delete[] windowBuffer; // Ensure to free allocated memory
  }

  void update() {
    if (inQueue == avgWindow) { // If the queue is full, empty it and get a HR
      float last = 0;
      float current = que.pop();
      float next = que.peek();
      double numPushed = 0;
      for (int i = 1; i < avgWindow; i++) { // pop values in the queue, and save
                                            // if you ever get a local maxima
        last = current;
        current = que.pop();
        next = que.peek();
        if ((last < current) && (current > next)) {
            tmp.push(i);
            numPushed = numPushed + 1;
          }
      }
      double avg = 0;
      last = 0;
      for (int i = 0; i < numPushed; i++) {
        if (last == 0) {
          last = tmp.pop();
        } else {
          avg = avg + (tmp.peek() - last);
          last = tmp.pop();
        }
      }
      tmp.clear();
      avg = avg / numPushed;
      Serial.println(avg);
      currentBPM = avg;
      que.clear();
      inQueue = 0;
    }
    float signal = analogRead(inputWire);
    if ((signal > 1000) || (signal < 0)) {
      return;
    } else {
      que.push(this_convolve(signal));
      inQueue++;
    }
  }

  int getBPM() {
    return currentBPM;
  }

private:
  float this_convolve(float signal) {
    float avg = 0;
    if ((signal > 1000) || (signal < 0)) {
      Serial.println("We shouldn't be here");
      return 500;
    }
    for (int i = 0; i < avgWindow; i++) {
      avg = avg + (conVal * windowBuffer[i]);
    }
    for (int i = 0; i < avgWindow - 1; i++) {
      windowBuffer[i] = windowBuffer[i + 1];
    }
    windowBuffer[avgWindow - 1] = signal;
    return avg;
  }
};
