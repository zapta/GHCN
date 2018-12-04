package geo;

public class GeoPoint {
  private final static double AVERAGE_RADIUS_OF_EARTH_METERS = 6371_000;

  public final float lat;
  public final float lon;

  public GeoPoint(float lat, float lon) {
    this.lat = lat;
    this.lon = lon;
  }

  // Based on an answer by whostolebenfrog@ here"
  // https://stackoverflow.com/questions/27928/calculate-distance-between-two-latitude-longitude-points-haversine-formula
  public double distanceMeters(GeoPoint other) {
    final double deltaLatRad = Math.toRadians(lat - other.lat);
    final double deltaLonRad = Math.toRadians(lon - other.lon);

    final double a = Math.sin(deltaLatRad / 2) * Math.sin(deltaLatRad / 2)
      + Math.cos(Math.toRadians(lat)) * Math.cos(Math.toRadians(other.lat))
      * Math.sin(deltaLonRad / 2) * Math.sin(deltaLonRad / 2);

    final double c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));

    return AVERAGE_RADIUS_OF_EARTH_METERS * c;
  }

  @Override
  public String toString() {
    return String.format("[%.4f,%.4f]", lat, lon);
  }
}
