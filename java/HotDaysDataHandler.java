import data.DataRecord;
import data.Processor;
import data.Processor.DataHandler;
import data.StationRecord;

import java.io.PrintStream;
import data.Processor.DataHandler;

public  class HotDaysDataHandler implements DataHandler {
  private final static PrintStream out = System.out;

  private final int minYear;
  private final int maxYear;
  private final int tempC;
  private final int numYears;

  // For each year in [minYear, maxYear]
  private final int[] totalDataPoints;
  private int[] hotDataPoints;

  public HotDaysDataHandler(int minYear, int maxYear, int tempC) {
    this.minYear = minYear;
    this.maxYear = maxYear;
    this.tempC = tempC;
    this.numYears = maxYear - minYear + 1;
    totalDataPoints = new int[numYears];
    hotDataPoints = new int[numYears];
  }

  @Override
  public void onDataRecord(StationRecord station, DataRecord data) {
    if (data.type == DataRecord.Type.TMAX && data.year >= minYear && data.year <= maxYear) {
      final int i = data.year - minYear;
      for (Float value : data.values) {
        if (value != null) {
          totalDataPoints[i]++;
          if (value > tempC) {
            hotDataPoints[i]++;
          }
        }
      }
    }
  }

  public void dumpResults() {
    for (int year = minYear; year <= maxYear; year++) {
      final int i = year - minYear;
      out.printf("%d, %5.2f  \n", year, (365.0 * hotDataPoints[i]) / totalDataPoints[i]);
    }
  }
}
