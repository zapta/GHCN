import data.DataRecord;
import geo.GeoPoint;
import data.LocalFileCache;
import data.DataProcessor;
import data.DataProcessor.DataSelector;
import data.DataProcessor.StationSelector;

import java.io.PrintStream;
import java.io.PrintWriter;

public class Main {
  private final static PrintStream out = System.out;

  final static GeoPoint OKLAHOMA_CITY = new GeoPoint(35.482222f,-97.535f);


  public static void main(String[] args) throws Exception {
    final LocalFileCache cache = new LocalFileCache();

    //final StationSelector stationsSelector = new StationsSelectorByRadius(OKLAHOMA_CITY, 100*1.6);
    final StationSelector stationsSelector = new StationSelectorUsStates("OK");

    final DataProcessor.DataSelector dataSelector = new DataSelectorByTypeAndYearRange(DataRecord.Type.TMAX, 1900, 2017);
    //final DataSelector dataSelector = new DataSelectorByTypeAndYearRange(DataRecord.Type.PRCP, 1800, 2100);

    // 35C = 95F
    final DataAnalyzerOfHotDays dataAnalyzer = new DataAnalyzerOfHotDays(35);
    //final DataAnalyzerOfPrecipitation dataAnalyzer = new DataAnalyzerOfPrecipitation();

    final DataProcessor processor = new DataProcessor();
    processor.process(cache, stationsSelector, dataSelector, dataAnalyzer);

    dataAnalyzer.dumpResults(out);
    PrintStream fileOut = new PrintStream("output.csv", "UTF-8");

    dataAnalyzer.dumpResults(fileOut);
    fileOut.close();
    out.println("Results written to output.csv");

    dataAnalyzer.chartResults();
  }
}
