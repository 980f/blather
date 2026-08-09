#include <iostream>
#include <bit>
#include <limits>

/**
 * This project exists to investigate the Collatz sequence.
 * The definition of that sequence used herein uses the base 2 representation of integers to describe the process.
 *
 * The Collatz sequence is defined on odd numbers where the successor is the prior plus the prior shifted up once with a 1 shifted in and that sum is then shifted down until the lsb is a 1.
 * It is conjectured that for any starting point one eventually (finite number of steps) gets to 1, whose successor is 1.
 *
 * Since the number is always odd we can drop the lsb from physical storage. The '1 shifted in' also does not need to be physical although we still shift in a 1 as that is the lsb that was not stored.
 *
 *
 */
using Int = unsigned int;
// const unsigned wordwith = std::numeric_limits<Int>::digits;

using std::countl_zero;
using std::countr_zero;

const unsigned numberOfOdds = 1000; //to size storage and limit the range of study

struct CollatzEval {
/**
 * we don't want just the series values, we want to see how wide the ALU and storage needs to be.
 */
  struct TrackElement {
    Int value;
    //how many steps did it take to get from value to 1?
    unsigned steps;
    //number of zeroes clipped during the step
    unsigned zeroes; //never more than number of bits in unsigned times number of steps.
    //the number of added bits, either 1 or 2, could use a boolean.
    unsigned msbs;
    //we are sharing the structure as both final record and intermediate storage during determination of sequece length.
    bool resolved;

    TrackElement(): value(0), steps(0), zeroes(0), msbs(0), resolved(false) {
    }

    void setForOne() {
      value = 1;
      steps = 1;
      zeroes = 2;
      msbs = 2;
    }

    void step(const Int prior) {
      Int successor = prior + (1 | (prior << 1)); // 3 * prior + 1 coded as for an FPGA
      zeroes = countr_zero(successor);
      msbs = countl_zero(prior) - countl_zero(successor);
      //todo: check for overflow of Int type, for now we just won't get anywhere near it by limiting the range of study.
      value = successor >> zeroes;
      resolved = value == 1;
      ++steps;
    }
  };

  TrackElement tracker[numberOfOdds];

  static unsigned oddness(unsigned ordinal) {
    return ordinal << 1 | 1;
  }

  static unsigned ordinal(unsigned oddish) {
    return oddish >> 1;
  }

  void run() {
    // the zeroeth element is the "1" that terminates sequences.
    tracker[0].setForOne();

    unsigned testee = 1; //an ordinal
    while (testee < numberOfOdds) {
      TrackElement& current(tracker[testee]);
      if (current.resolved) {
        ++testee;
        continue;
      }
      current.step(oddness(testee));
      auto lookahead = ordinal(current.value);
      do {
        auto trackahead = &tracker[lookahead];
        trackahead->step(lookahead);
        if (trackahead->resolved) { //we got to "1"
        } else { //must look at more steps
          //record for back tracking
          trackahead->steps = 1;
        }
      } while (!current.resolved);
    }
  }
};

void trace(const Int starter) {
  CollatzEval::TrackElement tracker;
  printf("\nSeries for %u", starter);
  printf("\nsteps\tvalue\tzeroes\tmsbs\tresolved");
  tracker.value = starter;
  do {
    tracker.step(tracker.value);
    printf("\n%u\t%x\t%u\t%u\t%u", tracker.steps, tracker.value, tracker.zeroes, tracker.msbs, tracker.resolved);
  } while (!tracker.resolved);
  printf("\n\n");
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("usage:\n\t0\tvalue\t prints the series started at the value");
    return 0;
  }
  auto which = atoi(argv[1]);
  switch (which) {
  default:
    printf("Command %d is not implemented", which);
    return 1;
  case 0:
    //process one number
    if (argc > 2) {
      auto query = atoi(argv[2]);
      trace(query);
    } else {
      return 1;
    }
    return 0;
  case 1:
    printf("We will investigate at most %u numbers (%u max value)", numberOfOdds, CollatzEval::oddness(numberOfOdds - 1));
    CollatzEval eval;
    eval.run();
    return 0;
  }

  return 0;
}
