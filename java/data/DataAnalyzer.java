package data;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Set;

/**
 * Base class for user provided object that performs the necesary data analysis, typically in a form
 * of aggregation of data.
 */
public class DataAnalyzer {
  /**
   * Called once when a station that passed the filtering is ready to be analysed. It follows
   * by zero or more calls to onDataRecord() and exactly one call to onStationEnd().
   */
  public void onStationStart(StationRecord station) {
  }

  /**
   * Called for each DataRecord that passed the user provider DataSelector.
   */
  public void onDataRecord(StationRecord station, DataRecord data) {
  }

  /**
   * Called when there are no more data records for the current station.
   */
  public void onStationEnd(StationRecord station) {
  }

  /**
   * Given a set of years, return a sorted array with all the years between the min and max
   * years in the set. Useful to generate annual analysis results from aggregated data in a Map.
   */
  protected int[] computeYearRange(Set<Integer> yearsSet) {
    if (yearsSet.isEmpty()) {
      return new int[0];
    }
    final List<Integer> sortedYears = new ArrayList(yearsSet);
    Collections.sort(sortedYears);
    final int minYear = sortedYears.get(0);
    final int maxYear = sortedYears.get(sortedYears.size() - 1);
    final int[] result = new int[maxYear - minYear + 1];
    for (int i = 0; i < result.length; i++) {
      result[i] = minYear + i;
    }
    return result;
  }
}
