package data;

/**
 * Represents the a single station meatadata record read from the stations file.
 */
public class StationRecord {
  private final static double AVERAGE_RADIUS_OF_EARTH_METERS = 6371_000;
  // The original textLine from the stations file.
  public final String textLine;
  // Station ID id. E.g. "USW00093901"
  public final String id;
  // Station's latitude.
  public final float lat;
  // Station's longitude.
  public final float lon;
  // Elevation in meters.
  public final float elevation;
  // Two letters US state id such as "TX".
  public final String state;
  // Station name. Free text.
  public final String name;

  private StationRecord(String textLine, String id, float lat, float lon, float elevation, String state, String name) {
    this.textLine = textLine;
    this.id = id;
    this.lat = lat;
    this.lon = lon;
    this.elevation = elevation;
    this.state = state;
    this.name = name;
  }

  public static boolean isAcceptedTextLine(String textLine) {
    // TODO: expand support to non us stations.
    // TODO: explain rationale for rejecting station records shorter than 85 chars (copied from Heller).
    return textLine.length() >= 85 && textLine.startsWith("US");
  }

  /**
   * Parse a stations file record.
   *
   * @param textLine a text textLine from the stations file that passed
   *                 the isAcceptedTextLine criteria.
   * @return a data.StationRecord with the station's metadata.
   */
  public static StationRecord parseFromTextLine(String textLine) {
    assert isAcceptedTextLine(textLine) : textLine;

    // 0         1         2         3         4         5         6
    // US1COAR0087  39.6155 -104.7785 1780.9 CO CHERRY CREEK DAM 4.7 ESE
    final String code = textLine.substring(0, 11); // [0, 10]
    final float lat = Float.valueOf(textLine.substring(11, 20));  // [11, 19]
    final float lon = Float.valueOf(textLine.substring(21, 30));  // [21, 29]
    final float elevation = Float.valueOf(textLine.substring(30, 37));  // [30, 36]
    final String state = textLine.substring(38, 40);  // [38, 39]
    final String name = textLine.substring(41, 76).trim();  // [41, 75]
    return new StationRecord(textLine, code, lat, lon, elevation, state, name);
  }

  @Override
  public String toString() {
    return String.format("[%s] [%.4f,%.4f] [%.1fm] [%s] [%s]", id, lat, lon, elevation, state, name);
  }

  /**
   * Path to daily file. Can be fetched with wget (path).
   */
  public String noaaFtpPath() {
    return String.format("ftp://ftp.ncdc.noaa.gov/pub/data/ghcn/daily/all/%s.dly", id);
  }

  public boolean isInLatLonRange(float pt1Lat, float pt1Lon, float pt2Lat, float pt2Lon) {
    return ((lat >= pt1Lat && lat <= pt2Lat) || (lat >= pt2Lat && lat <= pt1Lat)) &&
      ((lon >= pt1Lon && lon <= pt2Lon) || (lon >= pt2Lon && lon <= pt1Lon));
  }

  // Based on an answer by whostolebenfrog@ here"
  // https://stackoverflow.com/questions/27928/calculate-distance-between-two-latitude-longitude-points-haversine-formula

  public double distanceMetersFromLatLng(double ptLat, double ptLon) {
    final double deltaLatRad = Math.toRadians(lat - ptLat);
    final double deltaLonRad = Math.toRadians(lon - ptLon);

    final double a = Math.sin(deltaLatRad / 2) * Math.sin(deltaLatRad / 2)
      + Math.cos(Math.toRadians(lat)) * Math.cos(Math.toRadians(ptLat))
      * Math.sin(deltaLonRad / 2) * Math.sin(deltaLonRad / 2);

    final double c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));

    return AVERAGE_RADIUS_OF_EARTH_METERS * c;
  }
}