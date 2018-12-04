import data.DataProcessor;
import data.StationRecord;
import geo.GeoPoint;

import java.util.HashSet;
import java.util.Set;

public  class StationSelectorUsStates extends DataProcessor.StationSelector {

  private final Set<String> stateCodes;

  public StationSelectorUsStates(String... stateCodes) {
    this.stateCodes = new HashSet<String>();
    for (String state : stateCodes) {
      this.stateCodes.add(state);
    }
  }

  @Override
  public boolean onStation(StationRecord station) {
    return stateCodes.contains(station.state);
  }
}
