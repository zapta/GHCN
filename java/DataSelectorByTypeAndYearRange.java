import data.DataProcessor.DataSelector;
import data.DataRecord;

public class DataSelectorByTypeAndYearRange extends DataSelector {
  public final DataRecord.Type type;
  public final int minYear;
  public final int maxYear;

  public DataSelectorByTypeAndYearRange(DataRecord.Type type, int minYear, int maxYear) {
    this.type = type;
    this.minYear = minYear;
    this.maxYear = maxYear;
  }

  @Override
  public boolean onDataRecord(DataRecord data) {
    return data.type == type && data.year >= minYear && data.year <= maxYear;
  }
}
