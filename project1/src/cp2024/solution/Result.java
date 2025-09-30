package cp2024.solution;

public class Result {
  private final boolean value;
  private final int position;

  public Result(int position2, boolean value2) {
    value = value2;
    position = position2;
  }

  public boolean getValue() { return value; }
  public int getPosition() { return position; }
}