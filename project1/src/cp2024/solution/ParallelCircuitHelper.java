package cp2024.solution;

import cp2024.circuit.*;
import java.util.*;

public class ParallelCircuitHelper {
  protected final CircuitNode currentCircuit;
  protected final int position;
  protected final ParallelCircuitHelper parent;
  protected Thread masterThread;
  protected Boolean value = null;

  private Boolean[] childResult; // null - not known, true, false
  private final Deque<Result> pending = new ArrayDeque<>();
  private final Deque<Thread> childThread = new ArrayDeque<>();

  public ParallelCircuitHelper(CircuitNode currentCircuit2,
                               ParallelCircuitHelper from, int position2,
                               Thread masterThread2) {
    currentCircuit = currentCircuit2;
    position = position2;
    parent = from;
    masterThread = masterThread2;
  }

  synchronized public void initCalculation() {

    initMaster(); // Master is Thread working on root.

    try {
      // Object may not be constructed on non leaves.
      assert (currentCircuit.getType() != NodeType.LEAF);

      CircuitNode[] children = currentCircuit.getArgs();
      childResult = new Boolean[children.length];

      for (int i = 0; i < childResult.length; ++i) {

        // If interrupted by parent stop.
        if (Thread.currentThread().isInterrupted())
          throw new InterruptedException();

        Thread tmp = getThread(children, i);

        // Save information about thread responsible for tree node, before
        // running it.
        childThread.add(tmp);
        tmp.start();
      }

    } catch (InterruptedException e) {

      // my parent must have interrupted me - terminate_children.
      terminateChildrenProcess();
    }
  }

  public void initMaster(){
    if (masterThread == null) masterThread = Thread.currentThread();
  }

  public static ParallelCircuitHelper
  createCorrectObject(CircuitNode currentCircuit, ParallelCircuitHelper from,
                      int position, Thread masterThread) {
    return currentCircuit.getType() != NodeType.LEAF
        ? new ParallelCircuitHelper(currentCircuit, from, position,
                                    masterThread)
        : new ParallelCircuitLeafHelper(currentCircuit, from, position,
                                        masterThread);
  }

  private Thread getThread(CircuitNode[] children, int i) {

    // ParallelCircuitHelper constructor is fast, it can be performed here.
    ParallelCircuitHelper child =
        createCorrectObject(children[i], this, i, masterThread);
    return new Thread(() -> {
      child.initCalculation();
      child.process();
    });
  }

  synchronized public void process() {
    try {
      decide(); // maybe the value is known initially for example (LX node if
                // x=0).
      while (value == null) {
        while (pending.isEmpty())
          wait();

        Result newResult = pending.removeLast();
        childResult[newResult.getPosition()] = newResult.getValue();
        decide();

        if (Thread.currentThread().isInterrupted())
          throw new InterruptedException();
      }
      if (parent != null) // value may not be NULL
        parent.newResult(new Result(position, value));

      notifyAll(); // possibly someone is waiting in get_value.

      // All children calculations is meaningless - end it as soon as possible.
      terminateChildrenProcess();

    } catch (InterruptedException e) {

      notifyAll(); // possibly someone is waiting in get_value.

      // my parent must have interrupted me - terminate_children.
      terminateChildrenProcess();
    }
  }

  synchronized protected void newResult(Result newResult) {
    pending.add(newResult);
    notifyAll();
  }

  synchronized public Boolean getValue() { return value; }

  private void terminateChildrenProcess() {
    // if child process is already dead, interruption will not do anything.
    for (Thread t : childThread)
      t.interrupt();
  }

  private void decide() { // set value if it is already known.

    if (currentCircuit.getType() == NodeType.IF) {
      if (childResult[1] != null && childResult[2] != null &&
          childResult[1] == childResult[2]) {
        value = childResult[1]; // if second and third are known, and same,
                                // result is known.
      } else {

        if (childResult[0] == null) // Otherwise first node must be known.
          return;

        if (childResult[0])
          value = childResult[1]; // If it is null it doesn't change anything.

        if (!childResult[0])
          value = childResult[2];
      }
      return;
    }

    int unknown =
        (int)Arrays.stream(childResult).filter(Objects::isNull).count();
    int trues =
        (int)Arrays.stream(childResult).filter(Boolean.TRUE::equals).count();
    int falses = childResult.length - unknown - trues;

    if (currentCircuit.getType() == NodeType.NOT) {
      if (unknown == 0)
        value = trues == 0;
    }

    if (currentCircuit.getType() == NodeType.AND) {
      if (falses > 0) {
        value = false;
      } else {
        if (unknown == 0)
          value = true;
      }
    }

    if (currentCircuit.getType() == NodeType.OR) {
      if (trues > 0) {
        value = true;
      } else {
        if (unknown == 0)
          value = false;
      }
    }

    if (currentCircuit.getType() == NodeType.GT) {
      int x = ((ThresholdNode)currentCircuit).getThreshold();

      if (trues >= x + 1) {
        value = true;
      } else {
        if (unknown == 0 || trues + unknown < x + 1)
          value = false;
      }
    }

    if (currentCircuit.getType() == NodeType.LT) {
      int x = ((ThresholdNode)currentCircuit).getThreshold();

      if (trues + unknown <= x - 1) {
        value = true;
      } else {
        if (unknown == 0 || trues > x - 1)
          value = false;
      }
    }

    // Current Node may not be leaf.

  }
}
