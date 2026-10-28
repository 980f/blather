#include <iostream>
#include <bit>
#include <bitset>
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
const unsigned wordwith = std::numeric_limits<Int>::digits;

using std::countl_zero;
using std::countr_zero;

const unsigned numberOfOdds = 100'000; //to size storage and limit the range of study

struct CollatzEval {
/**
 * we don't want just the series values, we want to see how wide the ALU and storage needs to be.
 */
  struct TrackElement {
    unsigned link; //used for optimization, tracking most recent predecessor in present investigation.
    Int value;
    //how many steps did it take to get from value to 1?
    unsigned steps;
    //number of zeroes clipped during the step
    unsigned zeroes; //never more than number of bits in unsigned times number of steps.
    //the number of added bits, either 1 or 2, could use a boolean.
    unsigned msbs;
    //
    unsigned width;
    //we are sharing the structure as both final record and intermediate storage during determination of sequece length.
    bool resolved;

    TrackElement(): link(0), value(0), steps(0), zeroes(0), msbs(0), width(0), resolved(false) {
    }

    /** prime the investigation loop terminator logic */
    void setForOne() {
      link = 0;
      value = 1;
      steps = 1;
      zeroes = 2;
      msbs = 2;
      width = 1;
    }

    /** compute follower of @param prior */
    void step(const Int prior) {
      Int successor = prior + (1 | (prior << 1)); // 3 * prior + 1 coded as for an FPGA
      zeroes = countr_zero(successor);
      msbs = countl_zero(prior) - countl_zero(successor);
      //todo: check for overflow of Int type, for now we just won't get anywhere near it by limiting the range of study.
      value = successor >> zeroes;
      width = wordwith - countl_zero(value);
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

  Int run(int query) {
    // the zeroeth element is the "1" that terminates sequences.
    tracker[0].setForOne();

    for (unsigned testee = 1; testee < query; ++testee) {
      TrackElement *current = &tracker[testee];
      if (current->resolved) { //then we already evaluated it as a side effect of eval'ing some lesser number.
        continue;
      }
      printf("\r%d",testee);
      current->link = ~0; //mark end for track-back loop
      auto scanner = testee;
      while (scanner < numberOfOdds) { //this is an abnormal exit, array bounds check. The normal exit is a break deep in the guts of the loop.
        current = &tracker[scanner];
        if (current->steps == 0) { //if never evaluated
          current->step(oddness(scanner));
        }
        if (current->resolved) { //we hit a known element
          //time to back track
          unsigned accumulator = current->steps; //we have intersected an intermediate point on some sequence, which also might be a next-to-the-end one.
          while (current->link != ~0) { //we will not loop any times if the testee is one away from one. Those instances are of the form 10101010101, which is all 1's in base 4.
            auto trackback = &tracker[current->link];
            trackback->steps = ++accumulator;
            trackback->resolved = true; //we are resolved when the end of the sequence has been reached, not just the guys 1 before the end.
            current = trackback;
          }
          break; //loop normal exit is here
        }
        auto lookahead = ordinal(current->value);
        if (lookahead >= numberOfOdds) {
          printf("\nLooked too far ahead to %u starting from %u", lookahead, testee);
          while (current->link != ~0) {
            printf("\n\t%u (%u)", current->link, oddness(current->link));
            current = &tracker[current->link];
          }
          return testee;
        }
        auto trackahead = &tracker[lookahead];
        trackahead->link = scanner;
        scanner = lookahead;
      }
    }
    return 0;
  }
};

void trace(const Int starter) {
  CollatzEval::TrackElement tracker;
  printf("\nSeries for %u", starter);
  printf("\nsteps\t           value\t0's\tmsbs\td\tresolved");
  tracker.value = starter;
  do {
    tracker.step(tracker.value);
    printf("\n%u\t% 20d\t%u\t%u\t%d\t%u\t", tracker.steps, tracker.value, tracker.zeroes, tracker.msbs, tracker.msbs - tracker.zeroes, tracker.width);
    std::cout << std::bitset<wordwith>(tracker.value);
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
  case 0: //trace
    //process one number
    if (argc > 2) {
      auto query = std::stoi(argv[2], nullptr, 0);
      trace(query);
    } else {
      return 1;
    }
    return 0;
  case 1: //all
    printf("\nWe will investigate at most %u numbers (%u max value)", numberOfOdds, CollatzEval::oddness(numberOfOdds - 1));
    if (argc > 2) {
      auto query = std::stoi(argv[2], nullptr, 0);
      CollatzEval eval;
      auto errorAt = eval.run(query);
      if (errorAt) {
        printf("\nEnded early due to %u getting too large for the tracking array", errorAt);
      }
      //todo: dump the array as html
      for (unsigned testee = 1; testee < query; ++testee) {
        CollatzEval::TrackElement *current = &eval.tracker[testee];
        if (current->resolved) { //then we already evaluated it as a side effect of eval'ing some lesser number.
          printf("\n\t%d\t%d", CollatzEval::oddness(testee),current->steps);
        }
      }
      return 0;
    } else {
      return 1;
    }
  }

  return 0;
}
