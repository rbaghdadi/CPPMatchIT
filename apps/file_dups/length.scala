import scala.io.Source

val filename = "filepaths.txt"
for (line <- Source.fromFile(filename).getLines) {
  println(line + "#" + line.length);
}
