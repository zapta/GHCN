import data.DataProcessor;
import data.DataProcessor.StationSelector;
import data.DataProcessor.DataSelector;
import data.DataRecord;
import data.DataRecord.Type;
import data.LocalFileCache;
import geo.GeoPoint;

import java.io.PrintStream;

public class Main {
  private final static PrintStream out = System.out;

  final static GeoPoint OKLAHOMA_CITY = new GeoPoint(35.482222f, -97.535f);

  final static GeoPoint NEW_YORK = new GeoPoint(40.7127f, -74.0059f);
  
  final static GeoPoint LOS_ANGELES = new GeoPoint(34.05f, -118.25f);

  private static class ChartLoaderTask extends Thread {
    @Override
    public void run() {
      Chart.printTest("Chart class loaded");
    }
  }

  public static void main(String[] args) throws Exception {

    // Loading the charting classes take time so do it it.
    new ChartLoaderTask().start();

    final LocalFileCache cache = new LocalFileCache("/tmp/ghcn_cache");

    //final StationSelector stationsSelector = new StationsSelectorByRadius(NEW_YORK, Units.milesToKm(100));
    //final StationSelector stationsSelector = new StationsSelectorByRadius(LOS_ANGELES, Units.milesToKm(100));
    final StationSelector stationsSelector = new StationSelectorUsStates("OK");
    //final StationSelector stationsSelector = new StationSelectorUsStates("CA");

    //final DataProcessor.DataSelector dataSelector = new DataSelectorByTypeAndYearRange(DataRecord.Type.TMAX, 1900, 2017);
    //final DataSelector dataSelector = new DataSelectorByTypeAndYearRange(DataRecord.Type.PRCP, 1800, 2100);
    //final DataSelector dataSelector = new DataSelectorByTypeAndYearRange(1800, 2100, Type.TAVG );
    final DataSelector dataSelector = new DataSelectorByTypeAndYearRange(1800, 2100,
        Type.TAVG, Type.TMAX, Type.TMIN, Type.PRCP);

    //final DataAnalyzerOfHotDays dataAnalyzer = new DataAnalyzerOfHotDays(Units.farenheitToCelcius(95f));
    //final DataAnalyzerOfPrecipitation dataAnalyzer = new DataAnalyzerOfPrecipitation();
    //final DataAnalyzerOfTAvg dataAnalyzer = new DataAnalyzerOfTAvg();
    final DataAnalyzerOfDataPoints dataAnalyzer = new DataAnalyzerOfDataPoints();

    final DataProcessor processor = new DataProcessor();
    processor.process(cache, stationsSelector, dataSelector, dataAnalyzer);

    dataAnalyzer.dumpResults(out);
    PrintStream fileOut = new PrintStream("output.csv", "UTF-8");

    dataAnalyzer.dumpResults(fileOut);
    fileOut.close();
    out.println("Results written to output.csv");

    dataAnalyzer.chartResults();

    out.println("Main() done.");
  }
}
