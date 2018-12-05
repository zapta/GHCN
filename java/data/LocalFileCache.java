package data;

import it.sauronsoftware.ftp4j.FTPClient;

import java.io.File;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.*;
import java.util.stream.Collectors;

// Links for data and related docs:
// ftp://ftp.ncdc.noaa.gov/pub/data/ghcn/daily/readme.txt
// ftp://ftp.ncdc.noaa.gov/pub/data/cdo/documentation/GHCND_documentation.pdf
// http://www.montana.edu/atwood/weatherdata/ghcn_readme.pdf
//
// ftp://ftp.ncdc.noaa.gov/pub/data/ghcn/daily/all/USC00045123.dly
// ftp://ftp.ncdc.noaa.gov/pub/data/ghcn/daily/ghcnd-stations.txt

public class LocalFileCache {

  private final static PrintStream out = System.out;

  // NOAA limits number of parallel connections to 2.
  private static final int MAX_FTP_CONNECTIONS = 2;

  private final File cacheDir;

  /**
   * Create a cache instance on an existing and writeable local disk directory.
   */
  public LocalFileCache(String cacheDirPath) throws Exception {
    this.cacheDir = new File(cacheDirPath);
    // TODO: consider to replace with auto creation of the cache directory?
    if (!cacheDir.isDirectory()) {
      throw new RuntimeException("Cache directory [" + cacheDirPath + "] doesn't exist or is not a directory");
    }
  }

  /**
   * Given a station id, return a File for it's file in the cache. The file itself may
   * or may not exist.
   */
  public File stationDataLocalFile(String stationId) throws Exception {
    return new File(cacheDir, stationId + ".dly");
  }

  /**
   * Given a list of station ids, check which ones already have thier data files in the local cache.
   *
   * @return the sublist of station ids that are missing in the cache.
   */
  private List<String> findMissingLocalStationFiles(List<String> stationIds) throws Exception {
    List<String> result = new ArrayList();
    // TODO: reading the cache directory list may be faster than checking each file.
    for (String stationId : stationIds) {
      if (!stationDataLocalFile(stationId).exists()) {
        result.add(stationId);
      }
    }
    out.printf("%d of %d station files are alerady in the local cache\n", stationIds.size() - result.size(), stationIds.size());
    return result;
  }

  /**
   * Given a list of station ids that are missing in the cache, fetch and cache them. This
   * can take some time to complete.
   *
   * <p>This method is called in parallel by multiple loader tasks</p>
   */
  private void fetchStationFiles(String taskId, List<String> stationIds) throws Exception {
    if (stationIds.isEmpty()) {
      return;
    }
    out.printf("[%s] Connecting to ftp.ncdc.noaa.gov to fetch %d station files\n", taskId, stationIds.size());
    FTPClient client = new FTPClient();
    client.connect("ftp.ncdc.noaa.gov");
    client.login("anonymous", "ftp4j");
    int counter = 0;
    for (String missingStationId : stationIds) {
      counter++;
      out.printf("[%s] [%d/%d] Loading %s.dly\n", taskId, counter, stationIds.size(), missingStationId);
      client.download("pub/data/ghcn/daily/all/" + missingStationId + ".dly",
          stationDataLocalFile(missingStationId));
    }
    out.printf("[%s] Closing connection to ftp.ncdc.noaa.gov\n", taskId);
    client.disconnect(true);
  }


  /**
   * Given a list of station ids, make all of them having their data file cached on local disk.
   * This method fetched the missing data files
   */
  public void cacheStationsFilesByIds(List<String> stationIds) throws Exception {
    // Find the subset of the list that is not yet in cached.
    final List<String> missingStationIds = findMissingLocalStationFiles(stationIds);
    if (missingStationIds.isEmpty()) {
      return;
    }

    // We need to fetch the missing files. Will fetch from NOAA via FTP using the max allowed
    // number of connections.

    // Split the list of missing stations to numConnections sublists. We don't care if
    // some lists are empty.
    ArrayList<ArrayList<String>> lists = new ArrayList<ArrayList<String>>();
    for (int i = 0; i < MAX_FTP_CONNECTIONS; i++) {
      lists.add(new ArrayList<>());
    }
    for (int i = 0; i < missingStationIds.size(); i++) {
      lists.get(i % MAX_FTP_CONNECTIONS).add(missingStationIds.get(i));
    }

    // Create a parallel executor. We don't impose strict timeout.
    final ThreadPoolExecutor executor = new ThreadPoolExecutor(0, Integer.MAX_VALUE, 1,
        TimeUnit.DAYS, new SynchronousQueue<>());

    // Start the parallel tasks, get a future for each.
    final List<Future<String>> futures = new ArrayList<>();
    for (int i = 0; i < MAX_FTP_CONNECTIONS; i++) {
      Future<String> future = executor.submit(new FetchTask("Task-" + i, lists.get(i)));
      futures.add(future);
    }

    // Wait for all futures to complete.
    for (Future<String> future : futures) {
      final String status = future.get();
      out.printf("Task status: %s\n", status);
    }

    executor.shutdownNow();
  }

  // This wrapps fetchStationFiles as a callback that can be run by a parallel executor.
  class FetchTask implements Callable<String> {
    private final String taskId;
    private final List<String> stationIds;

    FetchTask(String taskId, List<String> stationIds) {
      this.taskId = taskId;
      this.stationIds = stationIds;
    }

    @Override
    public String call() throws Exception {
      out.printf("Fetcher task %s started\n", taskId);
      fetchStationFiles(taskId, stationIds);
      return taskId + "-OK";
    }
  }

  /* Given a list of station records, make all of them having their data file cached on local disk.
   * This method fetched the missing data files
   */
  public void cacheStationsFilesByRecords(List<StationRecord> stations) throws Exception {
    cacheStationsFilesByIds(stations.stream().map(s -> s.id).collect(Collectors.toList()));
  }

  /**
   * Construct a File that points to the GHCN stations file in the local cache. The file itself
   * may or may not already be in the cache.
   */
  public File stationsListLocalFile() throws Exception {
    return new File(cacheDir, "ghcnd-stations.txt");
  }

  /**
   * Makes sure that the stations file is in the local cache. If not, it fetches it.
   *
   * @throws Exception
   */
  public void cacheStationsListFile() throws Exception {
    // TODO: consider cache N stations in parallel.

    final File localStationsFile = stationsListLocalFile();
    if (stationsListLocalFile().exists()) {
      return;
    }

    out.printf("Connecting to ftp.ncdc.noaa.gov to stations list file\n");
    FTPClient client = new FTPClient();
    client.connect("ftp.ncdc.noaa.gov");
    client.login("anonymous", "ftp4j");
    client.download("pub/data/ghcn/daily/ghcnd-stations.txt",
        stationsListLocalFile());
    out.printf("Closing connection to ftp.ncdc.noaa.gov\n");
    client.disconnect(true);
  }
}
