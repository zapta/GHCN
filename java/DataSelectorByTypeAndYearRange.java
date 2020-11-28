import data.DataProcessor.DataSelector;
import data.DataRecord;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

public class DataSelectorByTypeAndYearRange extends DataSelector {
  //public final DataRecord.Type type;
  private final int minYear;
  private final int maxYear;
  private final Set<DataRecord.Type> types;


  public DataSelectorByTypeAndYearRange(int minYear, int maxYear, DataRecord.Type... types) {
    this.minYear = minYear;
    this.maxYear = maxYear;
    this.types = new HashSet(Arrays.asList(types));
  }

  @Override
  public boolean onDataRecord(DataRecord data) {
    return  types.contains(data.type) && data.year >= minYear && data.year <= maxYear;
  }
}
