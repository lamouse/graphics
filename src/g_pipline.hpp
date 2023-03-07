#ifndef G_PIPLINE_HPP
#define G_PIPLINE_HPP
#include<string>
#include <vector>
namespace g{
    class Pipline
    {
    private:
        static ::std::vector<char> readFile(const std::string& filePath);
        void createGraphicsPipline(const ::std::string& vertFilePath, const ::std::string& fragFilePath);
    public:
        Pipline(const ::std::string& vertFilePath, const ::std::string& fragFilePath);
        ~Pipline();
    };
    

    
}

#endif