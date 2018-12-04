package data;

import java.io.PrintStream;
import java.util.ArrayList;
import java.util.List;

/**
 * A framework class that orchestrates the analysis sessions of GHCN data.
 */
public class DataProcessor {

  private final static PrintStream out = System.out;

  /**
   * User provided filtering of stations. Only stations for which this returns true are
   * included in the analsys. Useful to restrict the analysis to a region or another
   * subset of stations.
   */
  public static abstract class StationSelector {
    public abstract boolean onStation(StationRecord station);
  }

  /**
   * User provided filtering of station data records. Only data records for which this returns
   * true are sent to the DataAnalyzer.
   */
  public static abstract class DataSelector {
    public abstract boolean onDataRecord(DataRecord data);
  }

  /**
   * This is the main method of the data processor. It performs the filtering and analysis based
   * on the user's filters and analyser passed to it.
   *
   * @param cache           the local file cache to use. It's responsible for fetching and
   *                        caching on local disk the necessary GHCN files.
   * @param stationSelector user provided criteria for stations to include in the analysis.
   * @param dataSelector    user provided criteria for data records to include in the analysis.
   * @param dataAnalyzer    user provided data analyser.
   */
  public void process(LocalFileCache cache, StationSelector stationSelector, DataSelector
      dataSelector, DataAnalyzer dataAnalyzer) throws Exception {
    final List<StationRecord> selectedStations = selectStations(cache, stationSelector);
    processData(cache, selectedStations, dataSelector, dataAnalyzer);
  }

  /**
   * Reads the station records from the station file and performs the station filtering.  If the
   * station file is not available, it is fetched and cached locally.
   */
  private List<StationRecord> selectStations(LocalFileCache cache, StationSelector
      stationSelector) throws Exception {
    int stationsCount = 0;
    cache.cacheStationsListFile();
    final StationsFileReader stationsReader = new StationsFileReader().open(cache
        .stationsListLocalFile());
    final List<StationRecord> result = new ArrayList<>();
    while (stationsReader.readNext()) {
      stationsCount++;
      final StationRecord stationRecord = stationsReader.parseTextLine();
      if (stationSelector.onStation(stationRecord)) {
        result.add(stationRecord);
      }
    }
    stationsReader.close();
    out.printf("Iterated over %d stations\n", stationsCount);
    return result;
  }

  /**
   * Read the station files that passed filtering, performs the data filtering and pass the data
   * records to the user provided analyzer.
   * If a station file is not available locally, it is fetched and cached on a local disk.
   */
  private  void processData(LocalFileCache cache, List<StationRecord> stationRecords,
                                  DataSelector dataSelector, DataAnalyzer dataAnalyzer) throws Exception {
    // This fetches and caches the missing station files. May take some time.
    cache.cacheStationsFilesByRecords(stationRecords);

    // All station files are here, start analysing.
    for (StationRecord station : stationRecords) {
      dataAnalyzer.onStationStart(station);
      final DataFileReader reader =
          new DataFileReader().open(cache.stationDataLocalFile(station.id));
      while (reader.readNext()) {
        final DataRecord data = reader.parseTextLine();
        if (dataSelector.onDataRecord(data)) {
          dataAnalyzer.onDataRecord(station, data);
        }
      }
      dataAnalyzer.onStationEnd(station);
      reader.close();
    }
  }
}
