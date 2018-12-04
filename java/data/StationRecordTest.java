package data;

import org.junit.Test;

import static org.junit.Assert.*;

public class StationRecordTest {

  private static final float DELTA = 0.00001f;

  @Test
  public void testIsAcceptedTextLine() {
    assertFalse(StationRecord.isAcceptedTextLine(""));
    // good.
    assertTrue(StationRecord.isAcceptedTextLine("USW00093987  31.2361  -94.7544   87.8" +
      " TX LUFKIN ANGELINA CO AP                       "));
    // Non "US"
    assertFalse(StationRecord.isAcceptedTextLine("XSW00093987  31.2361  -94.7544   87.8" +
      " TX LUFKIN ANGELINA CO AP                       "));
  }

  @Test
  public void testParseFromTextLine() {
    final StationRecord sr = StationRecord.parseFromTextLine("USW00093987  31.2361  -94.7544   87.8" +
      " TX LUFKIN ANGELINA CO AP                       ");
    assertEquals("USW00093987", sr.id);
    assertEquals(31.2361, sr.geoPoint.lat, DELTA);
    assertEquals(-94.7544, sr.geoPoint.lon, DELTA);
    assertEquals(87.8, sr.elevation, DELTA);
    assertEquals("TX", sr.state);
    assertEquals("LUFKIN ANGELINA CO AP", sr.name);
  }
}
