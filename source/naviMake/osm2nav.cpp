#include "osm2nav.h"
// used for va_list in debug-print methods
#include <stdarg.h>

// file io lib
#include <stdio.h>

// zlib compression is used inside the pbf blobs
#include <zlib.h>

// netinet provides the network-byte-order conversion function
#include <netinet/in.h>

// this is the header to pbf format
#include <osmpbf/osmpbf.h>

#include <iostream>
#include <boost/lexical_cast.hpp>

#include "type/data.h"

static void notice(const char *fmt, ...) {
    std::fprintf( stdout, "NOTICE: ");

    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stdout, fmt, ap);
    va_end(ap);

    std::fprintf(stdout, "\n");
}

// prints a formatted message to stderr, color coded to red
void err(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    exit(1);
}


char buffer[OSMPBF::max_uncompressed_blob_size];
char unpack_buffer[OSMPBF::max_uncompressed_blob_size];

// application main method
int main(int argc, char** argv) {
    if(argc != 3){
        std::cout << "Utilisation : " << argv[0] << "data.osm.pbf data.nav.lz4" << std::endl;
        return 1;
    }
    initGEOS(notice, notice);
    std::map<uint64_t, Node> nodes;
    std::map<uint64_t, Way> global_ways;
    std::vector<Way> ways;
    std::vector<Admin> admins;
    // buffer for reading a compressed blob from file

    // buffer for decompressing the blob


    // pbf struct of a BlobHeader
    OSMPBF::BlobHeader blobheader;

    // pbf struct of a Blob
    OSMPBF::Blob blob;

    // pbf struct of an OSM HeaderBlock
    OSMPBF::HeaderBlock headerblock;

    // pbf struct of an OSM PrimitiveBlock
    OSMPBF::PrimitiveBlock primblock;

    // open specified file
    FILE *fp = fopen(argv[1], "r");

    // read while the file has not reached its end
    while(!feof(fp)) {
        // storage of size, used multiple times
        int32_t sz;

        // read the first 4 bytes of the file, this is the size of the blob-header
        if(fread(&sz, sizeof(sz), 1, fp) != 1)
            break; // end of file reached

        // convert the size from network byte-order to host byte-order
        sz = ntohl(sz);

        // ensure the blob-header is smaller then MAX_BLOB_HEADER_SIZE
        if(sz > OSMPBF::max_blob_header_size)
            err("blob-header-size is bigger then allowed (%u > %u)", sz, OSMPBF::max_blob_header_size);

        // read the blob-header from the file
        if(fread(buffer, sz, 1, fp) != 1)
            err("unable to read blob-header from file");

        // parse the blob-header from the read-buffer
        if(!blobheader.ParseFromArray(buffer, sz))
            err("unable to parse blob header");

        // size of the following blob
        sz = blobheader.datasize();

        // ensure the blob is smaller then MAX_BLOB_SIZE
        if(sz > OSMPBF::max_uncompressed_blob_size)
            err("blob-size is bigger then allowed (%u > %u)", sz, OSMPBF::max_uncompressed_blob_size);

        // read the blob from the file
        if(fread(buffer, sz, 1, fp) != 1)
            err("unable to read blob from file");

        // parse the blob from the read-buffer
        if(!blob.ParseFromArray(buffer, sz))
            err("unable to parse blob");


        // set when we find at least one data stream
        bool found_data = false;

        // if the blob has uncompressed data
        if(blob.has_raw()) {
            // we have at least one datastream
            found_data = true;

            // size of the blob-data
            sz = blob.raw().size();

            // check that raw_size is set correctly
            if(sz != blob.raw_size())
                std::cout << "  reports wrong raw_size: "<< blob.raw_size() << " bytes" << std::endl;

            // copy the uncompressed data over to the unpack_buffer
            memcpy(unpack_buffer, buffer, sz);
        }

        // if the blob has zlib-compressed data
        if(blob.has_zlib_data()) {
            // issue a warning if there is more than one data steam, a blob may only contain one data stream
            if(found_data)
                std::cout << "  contains several data streams" << std::endl;

            // we have at least one datastream
            found_data = true;

            // the size of the compressesd data
            sz = blob.zlib_data().size();

            // zlib information
            z_stream z;

            // next byte to decompress
            z.next_in   = (unsigned char*) blob.zlib_data().c_str();

            // number of bytes to decompress
            z.avail_in  = sz;

            // place of next decompressed byte
            z.next_out  = (unsigned char*) unpack_buffer;

            // space for decompressed data
            z.avail_out = blob.raw_size();

            // misc
            z.zalloc    = Z_NULL;
            z.zfree     = Z_NULL;
            z.opaque    = Z_NULL;

            if(inflateInit(&z) != Z_OK) {
                err("  failed to init zlib stream");
            }
            if(inflate(&z, Z_FINISH) != Z_STREAM_END) {
                err("  failed to inflate zlib stream");
            }
            if(inflateEnd(&z) != Z_OK) {
                err("  failed to deinit zlib stream");
            }

            // unpacked size
            sz = z.total_out;
        }

        // if the blob has lzma-compressed data
        if(blob.has_lzma_data()) {
            // issue a warning if there is more than one data steam, a blob may only contain one data stream
            if(found_data)
                std::cout << "  contains several data streams" << std::endl;

            // we have at least one datastream
            found_data = true;

            // issue a warning, lzma compression is not yet supported
            err("  lzma-decompression is not supported");
        }

        // check we have at least one data-stream
        if(!found_data)
            err("  does not contain any known data stream");

        else if(blobheader.type() == "OSMData") {

            // parse the PrimitiveBlock from the blob
            if(!primblock.ParseFromArray(unpack_buffer, sz))
                err("unable to parse primitive block");

            // iterate over all PrimitiveGroups
            for(int i = 0, l = primblock.primitivegroup_size(); i < l; i++) {
                // one PrimitiveGroup from the the Block
                OSMPBF::PrimitiveGroup pg = primblock.primitivegroup(i);

                // Nœuds de types simple
                for(int i = 0; i < pg.nodes_size(); ++i) {
                    OSMPBF::Node n = pg.nodes(i);
                    Node node;
                    node.lon = 0.000000001 * (primblock.lat_offset() + (primblock.granularity() * n.lon())) ;
                    node.lat = 0.000000001 * (primblock.lat_offset() + (primblock.granularity() * n.lat())) ;
                    node.id = n.id();

                    nodes[n.id()] = node;
                }

                // Nœuds denses
                if(pg.has_dense()) {
                    OSMPBF::DenseNodes dn = pg.dense();
                    Node node;

                    for(int i = 0; i < dn.id_size(); ++i) {
                        node.id += dn.id(i);
                        node.lon +=  0.000000001 * (primblock.lat_offset() + (primblock.granularity() * dn.lon(i)));
                        node.lat +=  0.000000001 * (primblock.lat_offset() + (primblock.granularity() * dn.lat(i)));
                        nodes[node.id] = node;
                    }
                }


                // Ways (chemins)
                if(pg.ways_size() > 0) {
                    for(int i = 0; i < pg.ways_size(); ++i) {
                        OSMPBF::Way w = pg.ways(i);
                        std::map<std::string, std::string> attributes;
                        for(int j = 0; j < w.keys_size(); ++j) {
                            uint64_t key = w.keys(j);
                            uint64_t val = w.vals(j);
                            std::string key_string = primblock.stringtable().s(key);
                            std::string val_string = primblock.stringtable().s(val);
                            attributes[key_string] = val_string;
                        }

                        Way way;
                        way.id = w.id();
                        uint64_t ref = 0;
                        for(int j = 0; j < w.refs_size(); ++j){
                            ref += w.refs(j);
                            way.nodes.push_back(nodes[ref]);
                        }

                        // C'est une route
                        if(attributes.find("highway") != attributes.end() && attributes.find("name") != attributes.end() && way.nodes.size() > 1) {
                            GEOSCoordSequence* coord_seq = GEOSCoordSeq_create(way.nodes.size(), 2);
                            for(size_t j = 0; j < way.nodes.size(); ++j) {
                                GEOSCoordSeq_setX(coord_seq, j, way.nodes[j].lon);
                                GEOSCoordSeq_setY(coord_seq, j, way.nodes[j].lat);
                            }
                            way.name = attributes["name"];
                            way.geom = GEOSGeom_createLineString(coord_seq);
                            ways.push_back(way);
                        }
                        if(way.id == 59316052)
                            std::cout << "GOT IT" << std::endl;
                        global_ways[way.id] = way;
                    }
                }


                // Relations
                for(int i=0; i < pg.relations_size(); ++i){
                    OSMPBF::Relation rel = pg.relations(i);
                    std::map<std::string, std::string> attributes;
                    for(int j = 0; j < rel.keys_size(); ++j) {
                        uint64_t key = rel.keys(j);
                        uint64_t val = rel.vals(j);
                        std::string key_string = primblock.stringtable().s(key);
                        std::string val_string = primblock.stringtable().s(val);
                        attributes[key_string] = val_string;
                    }
                    // C'est une limite administrative
                    if(attributes.find("boundary") != attributes.end()
                            && attributes["boundary"] == "administrative"
                            && attributes.find("name") != attributes.end()
                            && attributes.find("admin_level") != attributes.end()){
                        Admin admin;
                        admin.id = rel.id();
                        admin.name = attributes["name"];

                        std::vector<Node> anodes;

                        // Le problème et que l'on a pas forcément les way dans l'ordre. Il faut donc le reconstruire ça
                        // On part du premier, et on tente de reconstruire tout ça
                        std::vector<int> ways_idx;

                        // On itère N fois sur les N éléments de la relation
                        uint64_t last_node = 0;
                        uint64_t id2 = 0;
                        for(int k = 0; k < rel.memids_size(); ++k){
                            uint64_t id = 0;
                            id2 += rel.memids(k);
                            if(global_ways.find(id2) == global_ways.end()){
                                std::cout << "Way pas trouvé : " << id << " pour la relation " << admin.id << std::endl;
                            }
                            else {
                                for(int l = 0; l < rel.memids_size(); ++l){
                                    id += rel.memids(l);
                                    if(rel.types(l) == OSMPBF::Relation::WAY && std::find(ways_idx.begin(), ways_idx.end(), id) == ways_idx.end()) {

                                        if(global_ways[id].nodes.back().id == last_node) {
                                            std::reverse(global_ways[id].nodes.begin(), global_ways[id].nodes.end());
                                        }
                                        if(last_node == 0 || global_ways[id].nodes.front().id == last_node){
                                            last_node  = global_ways[id].nodes.back().id;
                                            ways_idx.push_back(id);
                                        }
                                    }
                                }
                            }

                            if(rel.types(k) == OSMPBF::Relation::NODE && primblock.stringtable().s(rel.roles_sid(k)) == "admin_centre") {
                                admin.center = nodes[id2];
                            }
                        }

                        if(ways_idx.size() > 0 ){
                            for(auto idx : ways_idx) {
                                for(auto node : global_ways[idx].nodes){
                                    anodes.push_back(node);
                                }
                            }
                            admins.push_back(admin);
                        }
                        else {
                            std::cout << "Des soucis avec la relation " << admin.id << std::endl;
                        }


                        try{
                            admin.level = boost::lexical_cast<int>(attributes["admin_level"]);
                            if(anodes.size() > 3) {
                                GEOSCoordSequence* coord_seq = GEOSCoordSeq_create(anodes.size() + 1, 2);
                                for(size_t j = 0; j < anodes.size(); ++j) {
                                    Node n = anodes[j];
                                    GEOSCoordSeq_setX(coord_seq, j, n.lon);
                                    GEOSCoordSeq_setY(coord_seq, j, n.lat);
                                    if(j == 0) {
                                        GEOSCoordSeq_setX(coord_seq, anodes.size(), n.lon);
                                        GEOSCoordSeq_setY(coord_seq, anodes.size(), n.lat);
                                    }
                                }

                                GEOSGeometry * shell = GEOSGeom_createLinearRing(coord_seq);
                                admin.geom = GEOSGeom_createPolygon(shell, NULL, 0);

                                admins.push_back(admin);
                            }
                        } catch(...) {
                        }
                    }
                }

            }
        }

        else {
            // unknown blob type
            std::cout << "  unknown blob type: "<<  blobheader.type() << std::endl;
        }
    }
    // close the file pointer
    fclose(fp);

    // TODO : fusioner les segments ayant le même nom et des nœuds en commun

    std::cout << "Bilan : " << std::endl;
    std::cout << "    Nœuds : " << nodes.size() << std::endl;
    std::cout << "    Routes : " << ways.size() << std::endl;
    std::cout << "    Admin : " << admins.size() << std::endl;

    // On recolle les structures administratives aux rues
    for(Way & way : ways) {
        for(size_t i = 0; i < admins.size(); ++i) {
            if(admins[i].geom != nullptr && way.geom != nullptr && GEOSContains(admins[i].geom, way.geom)) {
                way.admin.push_back(i);
            }
        }
    }


    // clean up the protobuf lib
    google::protobuf::ShutdownProtobufLibrary();
    finishGEOS();

    //Et maintenant on remplit les données NAViTiA
    navitia::type::Data data;

    // Déjà les administrations
    int dep_idx = 0;
    int city_idx = 0;

    std::cout << "Remplissage des structures administratives" << std::endl;
    for(const Admin & admin : admins){
        if(admin.level == 6)   {
            navitia::type::Department dep;
            dep.name = admin.name;
            dep.id = boost::lexical_cast<std::string>(admin.id);
            dep.idx = dep_idx++;
            dep.external_code = dep.id;
            data.pt_data.departments.push_back(dep);
        }
        else if(admin.level == 8){
            navitia::type::City city;
            city.name = admin.name;
            city.id = boost::lexical_cast<std::string>(admin.id);
            city.idx = city_idx++;
            city.external_code = city.id;
            data.pt_data.cities.push_back(city);
        }
    }

    std::cout << "Remplissage des rues" << std::endl;
    for(const Way & osm_way : ways){
        navitia::streetnetwork::Way w;
        w.name = osm_way.name;
        for(auto admin_idx : osm_way.admin){
            for(auto city : data.pt_data.cities) {
                if(city.id == boost::lexical_cast<std::string>(admins[admin_idx].id)) {
                    w.city_idx = city.idx;
                }
            }
        }
        data.street_network.ways.push_back(w);
    }

    std::cout << "Construction de firstletter" << std::endl;
    data.build_first_letter();

    std::cout << "Sauvegarde" << std::endl;
    data.lz4(argv[2]);
}

