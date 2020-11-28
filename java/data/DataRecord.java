package data;

import com.sun.istack.internal.Nullable;

/**
 * Represents the a single station meatadata record read from the stations file. This
 * includes the daily values of a single metric for a given month.
 */
public class DataRecord {
  private static final int MAX_DAYS_IN_MONTH = 31;
  private static final int NUMBER_OF_MONTHS_PER_YEAR = 12;

  public enum Type {
    PRCP("PRCP", 0.1f),
    TAVG("TAVG", 0.1f),
    TMIN("TMIN", 0.1f),
    TMAX("TMAX", 0.1f);

    // Type code string in GHCN files.
    private final String typeCode;
    // Scalar for converting raw int values to proper units. Multiply the int value
    // in the data files with this const.
    private final Float valueScalar;

    private Type(String typeCode, float valueScaler) {
      this.typeCode = typeCode;
      this.valueScalar = valueScaler;
    }

    /**
     * Matches a record type code from the file and return the matching enum
     * value or null if not found.
     */
    @Nullable
    public static Type parseType(String typeStr) {
      for (Type type : values()) {
        if (type.typeCode.equals(typeStr)) {
          return type;
        }
      }
      return null;
    }
  }

  // The original textLine from the stations file.
  public final String textLine;

  // Parsed data
  public final String stationCode;
  public final String country;
  public final int year;
  public final int month;
  public final Type type;

  // The numeric values for the month's days. Missing values are indicated with
  // null. Would be nice to make immutable.
  public Float[] values;


  DataRecord(String textLine, String stationCode, String country, int year, int month, Type type, Float[] values) {
    this.textLine = textLine;
    this.stationCode = stationCode;
    this.country = country;
    this.year = year;
    this.month = month;
    this.type = type;
    this.values = values;
  }

  static boolean isAcceptedTextLine(String textLine) {
    // TODO: explain rationale for rejecting station records shorter than 269 chars (copied from Heller).
    if (textLine.length() < 269) {
      return false;
    }

    // For now we support only TMIN and TMAX
    final String typeStr = textLine.substring(17, 21);
    return Type.parseType(typeStr) != null;
  }

  /**
   * Parse a station file record.
   *
   * @param textLine a text textLine from the stations file that passed
   *                 the isAcceptedTextLine criteria.
   * @return a data.StationRecord with the station's metadata.
   */
  static DataRecord parseFromTextLine(String textLine) {
    assert isAcceptedTextLine(textLine) : textLine;

    final String stationCode = textLine.substring(0, 11);
    final String country = textLine.substring(0, 2);
    final int year = Integer.valueOf(textLine.substring(11, 15));
    final int month = Integer.valueOf(textLine.substring(15, 17));
    final String typeStr = textLine.substring(17, 21);

    // Parse the type. We already verified in isAcceptedTextLine that it's recognized.
    final Type type = Type.parseType(typeStr);

    int position = 21;
    final Float[] values = new Float[MAX_DAYS_IN_MONTH];
    for (int i = 0; i < MAX_DAYS_IN_MONTH; i++) {
      final String strValue = textLine.substring(position, position + 5).trim();
      final int intValue = Integer.valueOf(strValue);
      // -9999 is the 'no value'
      values[i] = (intValue == -9999) ? null : intValue * type.valueScalar;
      position += 8;
    }

    return new DataRecord(textLine, stationCode, country, year, month, type, values);
  }


  /** For debugging. */
  @Override
  public String toString() {
    final StringBuilder builder = new StringBuilder();
    for (int i = 0; i < values.length; i++) {
      if (i > 0) {
        builder.append(" ");
      }
      builder.append(values[i] == null ? " __ " : String.format("%.1f", values[i]));
    }
    return String.format("[%s] [%s] [%d/%02d] [%s] [%s]", stationCode, country, year, month, type, builder);
  }
}