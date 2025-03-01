#include "Queue.h"
using namespace std;

class HRMonitor {
public:

  int avgWindow;
  Queue<float> que;
  Queue<int> tmp;
  int inQueue = 0;
  int inputWire;
  int currentBPM = 0;
  int* windowBuffer;
  float conVal;

  HRMonitor(int bufferSize, int input) {
    avgWindow = bufferSize;
    que = new Queue<int>(bufferSize);
    inputWire = input;
    windowBuffer = new int[bufferSize];
    for (int i = 0; i < bufferSize; i++){
      windowBuffer[i] = 0;
    }
    conVal = 1 / bufferSize;
  }


  void update() {
    if (inQueue == windowBuffer) { // If the queue is full, empty it and get a HR
      float last = 0;
      float current = que.pop();
      float next = que.peek();
      int numPushed = 0;
      for (int i = 1; i < avgWindow; i++) { // pop values in the queue, and save
                                            // if you ever get a local maxima
        last = current;
        current = que.pop();
        next = que.peek();
        if ((last < current) && (current > next)) {
            tmp.push(i);
            numPushed++;
          }
      }
      float avg = 0;
      last = 0;
      for (int i = 0; i < numPushed; i++) {
        if (last == 0) {
          last = tmp.pop();
        } else {
          avg = avg + (tmp.peek() - last);
          last = tmp.pop();
        }
      }
      avg = avg / numPushed;
      currentBPM = avg;
      que.clear();
      inQueue = 0;
    }
    int signal = analogRead(inputWire);
    // convolve buffer and cycle, appending to queue
    que.push(this_convolve(signal));
    inQueue++;
  }

  int getBPM() {
    return currentBPM;
  }

private:
  float this_convolve(int signal) {
    float avg = 0;
    for (int i = 0; i < avgWindow; i++) {
      avg = avg + (conVal * windowBuffer[i]);
    }
    que.push(avg);
    for (int i = 0; i < avgWindow - 1; i++) {
      windowBuffer[i] = windowBuffer[i + 1];
    }
    windowBuffer[avgWindow - 1] = signal;
    return avg;
  }
};
