import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartPanel;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.renderer.AbstractRenderer;
import org.jfree.data.xy.XYSeries;
import org.jfree.data.xy.XYSeriesCollection;
import org.jfree.ui.ApplicationFrame;
import org.jfree.ui.RefineryUtilities;

import java.awt.*;
import java.io.PrintStream;

// TODO: cleanup

public class Chart extends ApplicationFrame {
  private final static PrintStream out = System.out;

  private Chart(int[] years, float[] values) {

    super("GHCN Analyzer");

    final XYSeries series = new XYSeries("days > 95F");
    for (int i = 0; i < years.length; i++) {
      series.add(years[i], values[i]);
    }

    final XYSeriesCollection data = new XYSeriesCollection(series);

    final JFreeChart chart = ChartFactory.createXYLineChart(
        "xxxxxxxxxx",
        "Year",
        "Days > 95F",
        data,
        PlotOrientation.VERTICAL,
        false,  // was true
        true,
        false
    );

    XYPlot plot = (XYPlot) chart.getPlot();

    AbstractRenderer r1 = (AbstractRenderer) plot.getRenderer(0);

    r1.setSeriesPaint(0, Color.BLUE);

    //r1.setBaseStroke(new BasicStroke(12));

    plot.setBackgroundPaint(Color.WHITE);

    final ChartPanel chartPanel = new ChartPanel(chart);

    chartPanel.setPreferredSize(new java.awt.Dimension(1000, 400));


    //chartPanel.setBackground(Color.YELLOW);

    setContentPane(chartPanel);

  }

  public static void plot(int[] years, float[] values) {
    out.println("Starting chart");
    final Chart demo = new Chart(years, values);
    demo.pack();
    RefineryUtilities.centerFrameOnScreen(demo);
    demo.setVisible(true);
  }

  public static void printTest(String msg) {
    out.println(msg);
  }
}

