#include "gtfs_parser.h"

int main(int, char **) {
    {
        GtfsParser p("/home/tristram/mumoro/idf_gtfs", "20101101");
  //  p.save("dump");
    p.save_bin("dump_bin");
}
    {
  //  GtfsParser p2;
//    p2.load("dump");
 //   p2.load_bin("dump_bin");
}
}
