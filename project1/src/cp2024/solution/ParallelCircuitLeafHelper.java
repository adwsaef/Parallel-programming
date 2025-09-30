package cp2024.solution;

import cp2024.circuit.CircuitNode;
import cp2024.circuit.LeafNode;

public class ParallelCircuitLeafHelper extends ParallelCircuitHelper {
  private boolean cancelled = false;

  public ParallelCircuitLeafHelper(CircuitNode currentCircuit2,
                                   ParallelCircuitHelper from, int position2,
                                   Thread masterThread2) {

    super(currentCircuit2, from, position2, masterThread2);
  }

  public synchronized void solved(boolean res) {
    value = res;
    notifyAll();
  }

  public synchronized void cancel() {
    cancelled = true;
    notifyAll();
  }

  @Override
  public synchronized void initCalculation() {
    initMaster();
  } // I don't have to init anything on leaves, just set masterThread if root is leave.

  @Override
  public synchronized void process() {
    // Leaf helper can face interruption from both sides:
    // external interruption while processing getValue - ALL calculations must
    // be stopped. parent interrupted me because my result is meaningless.

    Thread t = new Thread(() -> {
      try {
        solved(((LeafNode)currentCircuit).getValue());
      } catch (InterruptedException e) {
        cancel();
      }
    });
    t.start();

    try {
      // wait throws Interrupted Exception only if my thread got interrupted.
      // Only my parent can interrupt me, and it means that I should stop
      // calculating my Circuit node value.

      // This loop will be left when
      // value != null - t succeeded
      // cancelled = true - getValue got cancelled
      // wait thrown exception - only when parent interrupted me and my result
      // is pointless.
      while (value == null && !cancelled)
        wait();

    } catch (InterruptedException e) {
      t.interrupt();
      return;
    }

    // At this point cancelled or value is processed.

    if (!cancelled) {
      if (parent != null)
        parent.newResult(new Result(position, value));
    } else {
      // Cancel all (starting from root)
      masterThread.interrupt();
    }
  }
}
