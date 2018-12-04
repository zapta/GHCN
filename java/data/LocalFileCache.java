package data;

import it.sauronsoftware.ftp4j.FTPClient;

import java.io.File;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.List;
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

  private static final String DEFAULT_CACHE_DIR_PATH = "/tmp/ghcn_cache";

  private final File cacheDir;

  public LocalFileCache() throws Exception {
    this(DEFAULT_CACHE_DIR_PATH);
  }

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
   */
  private void fetchStationFiles(List<String> stationIds) throws Exception {
    if (stationIds.isEmpty()) {
      return;
    }
    out.printf("Connecting to ftp.ncdc.noaa.gov to fetch %d station files\n", stationIds.size());
    FTPClient client = new FTPClient();
    client.connect("ftp.ncdc.noaa.gov");
    client.login("anonymous", "ftp4j");
    int counter = 0;
    for (String missingStationId : stationIds) {
      counter++;
      out.printf("[%d/%d] Loading %s.dly\n", counter, stationIds.size(), missingStationId);
      client.download("pub/data/ghcn/daily/all/" + missingStationId + ".dly",
        stationDataLocalFile(missingStationId));
    }
    out.printf("Closing connection to ftp.ncdc.noaa.gov\n");
    client.disconnect(true);
  }


  /**
   * Given a list of station ids, make all of them having their data file cached on local disk.
   * This method fetched the missing data files
   */
  public void cacheStationsFilesByIds(List<String> stationIds) throws Exception {
    final List<String> missingStationIds = findMissingLocalStationFiles(stationIds);
    fetchStationFiles(missingStationIds);
  }

   /* Given a list of station records, make all of them having their data file cached on local disk.
   * This method fetched the missing data files
   */
  public void cacheStationsFilesByRecords(List<StationRecord> stations) throws Exception {
    cacheStationsFilesByIds(stations.stream().map(s->s.id).collect(Collectors.toList()));
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
