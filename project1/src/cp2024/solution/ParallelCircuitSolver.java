package cp2024.solution;

import cp2024.circuit.Circuit;
import cp2024.circuit.CircuitSolver;
import cp2024.circuit.CircuitValue;
import java.util.Vector;


public class ParallelCircuitSolver implements CircuitSolver {
  private boolean stopped = false;
  private final Vector<ParallelCircuitValue> myObjects = new Vector<>();

  @Override
  synchronized public CircuitValue solve(Circuit root) {
    if (stopped)
      return new IgnoredCircuit();

    myObjects.add(new ParallelCircuitValue(root.getRoot()));

    return myObjects.lastElement();
  }

  @Override
  synchronized public void stop() {
    if (!stopped) {
      stopped = true;

      // Stop all unfinished calculations.

      for (ParallelCircuitValue tmp : myObjects)
        tmp.interruptMasterThread();
    }
  }
}
