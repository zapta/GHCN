import data.LocalFileCache;
import data.Processor;

import java.io.PrintStream;

public class Main {
  private final static PrintStream out = System.out;

  public static void main(String[] args) throws Exception {
    final LocalFileCache cache = new LocalFileCache();
    final HotDaysDataHandler dataHandler = new HotDaysDataHandler(1945, 2017, 35);
    Processor.process(cache, new StationsSelectorByRadius(37.389444f, -122.081944f, 200), dataHandler);
    dataHandler.dumpResults();
  }
}
