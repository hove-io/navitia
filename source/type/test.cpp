#include "gtfs_parser.h"

int main(int, char **) {
    //GtfsParser p("/home/tristram/mumoro/idf_gtfs", "20101101");
//    p.save("dump");
    GtfsParser p2;
  //  p.save_bin("dump.bin");
    p2.load_bin("dump.bin");
}
