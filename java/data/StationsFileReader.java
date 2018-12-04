package data;

import com.sun.istack.internal.Nullable;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

public class StationsFileReader {

  @Nullable
  private BufferedReader reader;

  @Nullable
  private String textLine;


  public StationsFileReader open(File stationsFile) throws Exception {
    reader = new BufferedReader(new FileReader(stationsFile));
    return this;
  }

  public void close() throws IOException {
    if (reader != null) {
      textLine = null;
      reader.close();
    }
  }

  public boolean readNext() throws IOException {
    for (; ; ) {
      textLine = reader.readLine();
      // End of file
      if (textLine == null) {
        return false;
      }
      // Apply filter
      if (!StationRecord.isAcceptedTextLine(textLine)) {
        continue;
      }
      // We have a good station record.
      return true;
    }
  }

  public StationRecord parseTextLine() {
    return StationRecord.parseFromTextLine(textLine);
  }
}
