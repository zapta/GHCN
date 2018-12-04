package data;

import com.sun.istack.internal.Nullable;

import java.io.*;

/** A reader for GHCN's station data files. It reads the .dly file and
 * provides the records as DataRecord instances. */
public class DataFileReader {

  @Nullable
  private BufferedReader reader;

  // Current text line.
  @Nullable
  private String textLine;


  /** Open on given .dly file. */
  public  DataFileReader open(File file) throws FileNotFoundException {
    reader = new BufferedReader(new FileReader(file));
    return this;
  }

  /** Close. Call before discarding the reader. */
  public void close() throws IOException {
    if (reader != null) {
      textLine = null;
      reader.close();
    }
  }

  /**
   * Reads next line.  Skips lines that are rejected by the DataRecord parser.
   * @return true if a new line is available. False if at end of file.
   * @throws IOException
   */
  public boolean readNext() throws IOException {
    for (; ; ) {
      textLine = reader.readLine();
      // End of file
      if (textLine == null) {
        return false;
      }
      // Apply filter
      if (!DataRecord.isAcceptedTextLine(textLine)) {
        continue;
      }
      // We have a good station record.
      return true;
    }
  }

  /**
   * Parses the current line and return as a new RecordData instance.
   */
  public DataRecord parseTextLine() {
    return DataRecord.parseFromTextLine(textLine);
  }
}
