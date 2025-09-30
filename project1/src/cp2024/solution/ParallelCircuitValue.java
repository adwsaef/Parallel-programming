package cp2024.solution;

import cp2024.circuit.CircuitNode;
import cp2024.circuit.CircuitValue;

public class ParallelCircuitValue implements CircuitValue {
  private final ParallelCircuitHelper root;
  private final Thread masterThread;

  public ParallelCircuitValue(CircuitNode currentCircuit) {
    // constructor is fast - it can be performed here.

    root = ParallelCircuitHelper.createCorrectObject(currentCircuit, null, 0,
                                                     null);

    masterThread = new Thread(() -> {
      root.initCalculation();
      root.process();
    });

    masterThread.start();
  }

  // Thread.interrupt is safe function to call from different threads - function
  // doesn't have to be synchronized.
  public void interruptMasterThread() { masterThread.interrupt(); }

  @Override
  synchronized public boolean getValue() throws InterruptedException {

    masterThread.join();

    Boolean res = root.getValue();

    if (res == null)
      throw new InterruptedException();

    return res;
  }
}
