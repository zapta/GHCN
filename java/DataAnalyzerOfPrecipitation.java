import data.DataAnalyzer;
import data.DataRecord;
import data.StationRecord;

import java.io.PrintStream;
import java.util.*;

public class DataAnalyzerOfPrecipitation extends DataAnalyzer {
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
    if (data.type != DataRecord.Type.PRCP) {
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
    ps.print("year, percp inch\n");
    final int[] years = computeYearRange(dataMap.keySet());
    for (int year : years) {
      final AnnualData annualData = dataMap.get(year);
      if (annualData == null) {
        ps.printf("%4d,\n", year);
      } else {
        ps.printf("%4d, %5.2f, %7d\n", year,
          annualData.count == 0 ? 0f : 365.0f * (annualData.sum / annualData.count) / 25.4, annualData.count);
      }
    }
  }
}
