import data.Processor;
import data.StationRecord;

public  class StationsSelectorByRadius implements Processor.StationSelector {
  private final float lat;
  private final float lng;
  private final int radiusKm;

  public StationsSelectorByRadius(float lat, float lng, int radiusKm) {
    this.lat = lat;
    this.lng = lng;
    this.radiusKm = radiusKm;
  }

  @Override
  public boolean onStation(StationRecord station) {
    return station.distanceMetersFromLatLng(lat, lng) < radiusKm * 1000;
  }
}
