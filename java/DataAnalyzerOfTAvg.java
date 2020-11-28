import data.DataAnalyzer;
import data.DataRecord;
import data.StationRecord;

import java.io.PrintStream;
import java.util.HashMap;
import java.util.Map;

public class DataAnalyzerOfTAvg extends DataAnalyzer {
  private final static PrintStream out = System.out;

  private static class AnnualData {
    private int count;
    private float sum;
  }

  private Map<Integer, AnnualData> dataMap = new HashMap();

  @Override
  public void onStationStart(StationRecord station) {
    out.printf("*** %s\n", station);
  }

  @Override
  public void onDataRecord(StationRecord station, DataRecord data) {
    if (data.type != DataRecord.Type.TAVG) {
      throw new RuntimeException("Unexpected data type: " + data.type);
    }

    AnnualData annualData = dataMap.get(data.year);
    if (annualData == null) {
      annualData = new AnnualData();
      dataMap.put(data.year, annualData);
    }

    for (Float value : data.values) {
      if (value != null) {
        annualData.count++;
        annualData.sum += value;
      }
    }
  }

  public void dumpResults(PrintStream ps) {
    ps.print("year, tavg\n");
    final int[] years = computeYearRange(dataMap.keySet());
    for (int year : years) {
      final AnnualData annualData = dataMap.get(year);
      if (annualData == null) {
        ps.printf("%4d,\n", year);
      } else {
        ps.printf("%4d, %2.2f, %7d\n", year,
          annualData.count == 0 ? 0f : annualData.sum / annualData.count, annualData.count);
      }
    }
  }

  public void chartResults() {

    final int[] years = computeYearRange(dataMap.keySet());
    final float[] values = new float[years.length];
    for (int i = 0; i < years.length; i++) {
      final AnnualData annualData = dataMap.get(years[i]);
      if (annualData != null) {
        values[i] = ((float) annualData.sum) / annualData.count;
      }

    }
    Chart.plot(years, values);

  }
}
