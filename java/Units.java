public class Units {

  private static final float KM_PER_MILE = 1.609344f;

  public static float farenheitToCelcius(float f) {
    return (f - 32) * 5 / 9;
  }

  public static float celciusTofarenheit(float c) {
    return (c * 9 / 5) + 32;
  }

  public static float kmToMiles(float km) {
    return km / KM_PER_MILE;
  }

  public static float milesToKm(float miles) {
    return miles * KM_PER_MILE;
  }
}
