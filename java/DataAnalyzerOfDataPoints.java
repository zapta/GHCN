import data.DataAnalyzer;
import data.DataRecord;
import data.DataRecord.Type;
import data.StationRecord;

import java.io.PrintStream;
import java.util.HashMap;
import java.util.Map;

public class DataAnalyzerOfDataPoints extends DataAnalyzer {
  private final static PrintStream out = System.out;

  private static class AnnualData {
    private int tavgCount;
    private int tMaxCount;
    private int tMinCount;
    private int prcpCount;
  }

  private Map<Integer, AnnualData> dataMap = new HashMap();

  @Override
  public void onStationStart(StationRecord station) {
    out.printf("*** %s\n", station);
  }

  @Override
  public void onDataRecord(StationRecord station, DataRecord data) {
    AnnualData annualData = dataMap.get(data.year);
    if (annualData == null) {
      annualData = new AnnualData();
      dataMap.put(data.year, annualData);
    }

    for (Float value : data.values) {
      if (value != null) {
        switch (data.type) {
          case TAVG:
            annualData.tavgCount++;
            break;
          case TMAX:
            annualData.tMaxCount++;
            break;
          case TMIN:
            annualData.tMinCount++;
            break;
          case PRCP:
            annualData.prcpCount++;
            break;
        }
      }
    }
  }

  public void dumpResults(PrintStream ps) {
    ps.print("year, #tAvg, #tMax, #tMin, #prcp\n");
    final int[] years = computeYearRange(dataMap.keySet());
    for (int year : years) {
      final AnnualData annualData = dataMap.get(year);
      if (annualData == null) {
        ps.printf("%4d,\n", year);
      } else {
        ps.printf("%4d, %6d, %6d, %6d, %6d\n", year,
          annualData.tavgCount, annualData.tMaxCount, annualData.tMinCount, annualData.prcpCount );
      }
    }
  }

  public void chartResults() {
    out.println("Chart results no implemented");

//    final int[] years = computeYearRange(dataMap.keySet());
//    final float[] values = new float[years.length];
//    for (int i = 0; i < years.length; i++) {
//      final AnnualData annualData = dataMap.get(years[i]);
//      if (annualData != null) {
//        values[i] = ((float) annualData.sum) / annualData.count;
//      }
//
//    }
//    Chart.plot(years, values);

  }
}
