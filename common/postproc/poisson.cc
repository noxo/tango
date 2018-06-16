#include "data/file3d.h"
#include "data/mesh.h"
#include "postproc/poisson.h"

extern int main( int argc , char* argv[] );

namespace oc {
    void Poisson::Process(std::string filename) {
        int kSubdivisionSize = 20000;
        std::string pointcloud = filename + ".ply";

        //convert to ply
        {
            std::vector<Mesh> data;
            File3d(filename, false).ReadModel(kSubdivisionSize, data);
            File3d(pointcloud, true).WriteModel(data);
        }

        //poisson reconstruction
        std::vector<std::string> argv;
        argv.push_back("--in");
        argv.push_back(pointcloud);
        argv.push_back("--out");
        argv.push_back(pointcloud);
        std::vector<const char *> av;
        av.push_back(0);
        for (std::vector<std::string>::const_iterator i = argv.begin(); i != argv.end(); ++i)
            av.push_back(i->c_str());
        main((int) av.size(), (char **) &av[0]);

        //convert to obj
        std::vector<Mesh> data;
        File3d(pointcloud, false).ReadModel(kSubdivisionSize, data);
        File3d(filename, true).WriteModel(data);
    }
}
