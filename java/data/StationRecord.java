package data;

import geo.GeoPoint;

/**
 * Represents the a single station meatadata record read from the stations file.
 */
public class StationRecord {
  // The original textLine from the stations file.
  public final String textLine;
  // Station ID id. E.g. "USW00093901"
  public final String id;
  // Station's location.
  public final GeoPoint geoPoint;
  // Elevation in meters.
  public final float elevation;
  // Two letters US state id such as "TX".
  public final String state;
  // Station name. Free text.
  public final String name;

  private StationRecord(String textLine, String id, GeoPoint geoPoint, float elevation, String state, String name) {
    this.textLine = textLine;
    this.id = id;
    this.geoPoint = geoPoint;
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
    final GeoPoint geoPoint = new GeoPoint(lat, lon);
    final float elevation = Float.valueOf(textLine.substring(30, 37));  // [30, 36]
    final String state = textLine.substring(38, 40);  // [38, 39]
    final String name = textLine.substring(41, 76).trim();  // [41, 75]
    return new StationRecord(textLine, code, geoPoint, elevation, state, name);
  }

  @Override
  public String toString() {
    return String.format("[%s] %s [%.1fm] [%s] [%s]", id, geoPoint, elevation, state, name);
  }

  /**
   * Path to daily file. Can be fetched with wget (path).
   */
  public String noaaFtpPath() {
    return String.format("ftp://ftp.ncdc.noaa.gov/pub/data/ghcn/daily/all/%s.dly", id);
  }

  public double distanceMetersFromLatLng(GeoPoint point) {
    return geoPoint.distanceMeters(point);
  }
}