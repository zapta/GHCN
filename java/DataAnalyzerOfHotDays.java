import data.DataAnalyzer;
import data.DataRecord;
import data.StationRecord;

import java.io.PrintStream;
import java.util.*;

public class DataAnalyzerOfHotDays extends DataAnalyzer {

  private final static PrintStream out = System.out;

  private static class AnnualData {
    private int totalCount;
    private float hotCount;
  }

  private Map<Integer, AnnualData> dataMap = new HashMap();

  private final int tempC;

  public DataAnalyzerOfHotDays(int tempC) {
    this.tempC = tempC;
  }

  @Override
  public void onDataRecord(StationRecord station, DataRecord data) {
    if (data.type != DataRecord.Type.TMAX) {
      throw new RuntimeException("Unexpected data type: " + data.type);
    }

    AnnualData annualData = dataMap.get(data.year);
    if (annualData == null) {
      annualData = new AnnualData();
      dataMap.put(data.year, annualData);
    }

    for (Float value : data.values) {
      if (value != null) {
        annualData.totalCount++;

        if (value > tempC) {
          annualData.hotCount++;
        }
      }
    }

  }

  public void dumpResults(PrintStream ps) {
    ps.print("year, hot days\n");
    final int[] years = computeYearRange(dataMap.keySet());
    for (int year : years) {
      final AnnualData annualData = dataMap.get(year);
      if (annualData == null) {
        ps.printf("%4d,\n", year);
      } else {
        ps.printf("%4d, %5.2f, %7d\n", year,
          annualData.totalCount == 0 ? 0f : 365.0f * ((float)annualData.hotCount / annualData.totalCount), annualData.totalCount);
      }
    }
  }

  public void chartResults() {
    final int[] years = computeYearRange(dataMap.keySet());
    final float[] values = new float[years.length];
    for (int i = 0; i < years.length; i++) {

      final AnnualData annualData = dataMap.get(years[i]);
      if (annualData != null) {
        values[i] = 365.0f * ((float) annualData.hotCount / annualData.totalCount);
      }

    }
    Chart.plot(years, values);

  }


}
