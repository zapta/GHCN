import geo.GeoPoint;
import data.DataProcessor;
import data.StationRecord;

public  class StationsSelectorByRadius extends DataProcessor.StationSelector {
  private final GeoPoint center;
//  private final float lat;
//  private final float lng;
  private final double radiusKm;

  public StationsSelectorByRadius(GeoPoint center, double radiusKm) {
    this.center = center;
    this.radiusKm = radiusKm;
  }

  @Override
  public boolean onStation(StationRecord station) {
    return station.distanceMetersFromLatLng(center) < radiusKm * 1000;
  }
}
