#include <string>
#include <vector>

int ghcnmain(std::vector<std::string> argv);

int main(int argc,char** argv)
{

    std::vector<std::string> argvector;

    for (int i = 0; i < argc; i++)
    {
        argvector.push_back(argv[i]);
    }

    ghcnmain(argvector);
}
