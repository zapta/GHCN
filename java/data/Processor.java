package data;

import com.sun.istack.internal.Nullable;

import java.io.*;
import java.util.ArrayList;
import java.util.List;

public class Processor {

  private final static PrintStream out = System.out;

  public static class StationsFileReader {


    @Nullable
    private BufferedReader reader;

    @Nullable
    private String textLine;


    public StationsFileReader open(File stationsFile) throws Exception {
      reader = new BufferedReader(new FileReader(stationsFile));
      return this;
    }

    public void close() throws IOException {
      if (reader != null) {
        textLine = null;
        reader.close();
      }
    }

    public boolean readNext() throws IOException {
      for (; ; ) {
        textLine = reader.readLine();
        // End of file
        if (textLine == null) {
          return false;
        }
        // Apply filter
        if (!StationRecord.isAcceptedTextLine(textLine)) {
          continue;
        }
        // We have a good station record.
        return true;
      }
    }

    public StationRecord parseTextLine() {
      return StationRecord.parseFromTextLine(textLine);
    }
  }


  public static class DataFileReader {

    // File's path in the local file system.
    private final File file;

    @Nullable
    private BufferedReader reader;

    @Nullable
    private String textLine;

    private DataFileReader(File file) {
      this.file = file;
    }


    public static DataFileReader forFile(File file) throws Exception {
      return new DataFileReader(file).open();
    }

    private DataFileReader open() throws FileNotFoundException {
      reader = new BufferedReader(new FileReader(file));
      return this;
    }

    public void close() throws IOException {
      if (reader != null) {
        textLine = null;
        reader.close();
      }
    }

    public boolean readNext() throws IOException {
      for (; ; ) {
        textLine = reader.readLine();
        // End of file
        if (textLine == null) {
          return false;
        }
        // Apply filter
        if (!DataRecord.isAcceptedTextLine(textLine)) {
          continue;
        }
        // We have a good station record.
        return true;
      }
    }

    public DataRecord parseTextLine() {
      return DataRecord.parseFromTextLine(textLine);
    }
  }


  public static void process(LocalFileCache cache, StationSelector stationSelector, DataHandler dataHandler) throws Exception {
    final List<StationRecord> selectedStations = selectStations(cache, stationSelector);
    processData(cache, selectedStations, dataHandler);
  }

  /**
   * Read station file and select the station that match the station selector.
   */
  private static List<StationRecord> selectStations(LocalFileCache cache, StationSelector stationSelector) throws Exception {
    int stationsCount = 0;
    cache.cacheStationsListFile();
    final StationsFileReader stationsReader = new StationsFileReader().open(cache.stationsListLocalFile());
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
   * Read station file and select the station that match the station selector.
   */
  private static void processData(LocalFileCache cache, List<StationRecord> stationRecords, DataHandler dataHandler) throws Exception {
    cache.cacheStationsFilesByRecords(stationRecords);
    for (StationRecord station : stationRecords) {
      final DataFileReader reader = DataFileReader.forFile(cache.stationDataLocalFile(station.id));
      while (reader.readNext()) {
        final DataRecord data = reader.parseTextLine();
        dataHandler.onDataRecord(station, data);
      }
      reader.close();
    }
  }

  public interface StationSelector {
    boolean onStation(StationRecord station);
  }

  public interface DataHandler {
    void onDataRecord(StationRecord station, DataRecord data);
  }
}
